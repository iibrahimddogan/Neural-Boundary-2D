#include "Trainer.h"
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QFrame>
#include <QString>
#include <QFileDialog>
#include <QGraphicsEllipseItem>
#include <QGraphicsPixmapItem>
#include <QImage>
#include <QPixmap>
#include <cmath>
#include <cstdlib>
#include <fstream>

static const QColor renkler[] = {
    Qt::red, Qt::blue, Qt::darkGreen, QColor(255, 140, 0), Qt::darkMagenta,
    QColor(0, 160, 160), Qt::darkYellow, QColor(150, 75, 0), Qt::black
};

static QColor classColor(int cls)
{
    return renkler[cls % 9];
}

PlaneScene::PlaneScene(QObject* parent) : QGraphicsScene(parent)
{
    setSceneRect(-400, -400, 800, 800);

    QPen gridPen(QColor(120, 120, 120), 1);
    for (int i = -400; i <= 400; i += 100) {
        addLine(i, -400, i, 400, gridPen);
        addLine(-400, i, 400, i, gridPen);
    }

    QPen axisPen(Qt::black, 2);
    addLine(-400, 0, 400, 0, axisPen);
    addLine(0, -400, 0, 400, axisPen);
}

void PlaneScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    QPointF p = event->scenePos();
    emit pointClicked(p.x(), -p.y());
    QGraphicsScene::mousePressEvent(event);
}

Trainer::Trainer(QWidget* parent) : QMainWindow(parent)
{
    ui.setupUi(this);

    scene = new PlaneScene(this);
    ui.xy_plane->setScene(scene);
    ui.xy_plane->setFrameShape(QFrame::NoFrame);
    ui.xy_plane->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui.xy_plane->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(scene, &PlaneScene::pointClicked, this, &Trainer::onPointClicked);
    connect(ui.SaveButton, &QPushButton::clicked, this, &Trainer::save_data);
    connect(ui.LoadButton, &QPushButton::clicked, this, &Trainer::load_data);
    connect(ui.TrainButton, &QPushButton::clicked, this, &Trainer::train_network);
}

Trainer::~Trainer()
{
    delete[] points;
    delete[] classes;
}

void Trainer::onPointClicked(double x, double y)
{
    int cls = ui.Spinbox_Class->value() - 1;
    addPoint(x, y, cls);

    QColor c = classColor(cls);
    QGraphicsEllipseItem* dot = scene->addEllipse(x - 4, -y - 4, 8, 8, QPen(c), QBrush(c));
    dot->setZValue(1);

    statusBar()->showMessage(QString("x = %1   y = %2   sinif: %3   nokta sayisi: %4")
        .arg(x, 0, 'f', 1)
        .arg(y, 0, 'f', 1)
        .arg(cls + 1)
        .arg(point_count));
}

void Trainer::addPoint(double x, double y, int cls)
{
    if (point_count == capacity)
    {
        int new_capacity = (capacity == 0) ? 8 : capacity * 2;

        double* new_points = new double[new_capacity * 2];
        int* new_classes = new int[new_capacity];

        for (int i = 0; i < point_count * 2; i++)
            new_points[i] = points[i];

        for (int i = 0; i < point_count; i++)
            new_classes[i] = classes[i];

        delete[] points;
        delete[] classes;

        points = new_points;
        classes = new_classes;
        capacity = new_capacity;
    }

    points[2 * point_count] = x;
    points[2 * point_count + 1] = y;
    classes[point_count] = cls;
    point_count++;
}

void Trainer::save_data()
{
    if (point_count == 0)
    {
        statusBar()->showMessage("No points to save");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Save Data", "Samples.txt", "Text Files (*.txt)");
    if (fileName.isEmpty())
        return;

    std::ofstream file(fileName.toLocal8Bit().constData());

    if (!file.is_open())
    {
        statusBar()->showMessage("File could not be opened");
        return;
    }

    int class_count = 2;
    for (int i = 0; i < point_count; i++)
        if (classes[i] + 1 > class_count)
            class_count = classes[i] + 1;

    file << 2 << " " << 400 << " " << 400 << " " << class_count << "\n";

    for (int i = 0; i < point_count; i++)
        file << points[2 * i] << " " << points[2 * i + 1] << " " << classes[i] << "\n";

    file.close();

    statusBar()->showMessage(QString("Saved %1 points: %2").arg(point_count).arg(fileName));
}

void Trainer::load_data()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load Data", "Samples.txt", "Text Files (*.txt)");
    if (fileName.isEmpty())
        return;

    std::ifstream file(fileName.toLocal8Bit().constData());

    if (!file.is_open())
    {
        statusBar()->showMessage("File could not be opened");
        return;
    }

    int dim, w, h, class_count;

    if (!(file >> dim >> w >> h >> class_count) || dim != 2 || w <= 0 || h <= 0 || class_count < 2)
    {
        statusBar()->showMessage("Invalid file format");
        return;
    }

    delete[] points;
    delete[] classes;
    points = nullptr;
    classes = nullptr;
    point_count = 0;
    capacity = 0;

    PlaneScene* oldScene = scene;
    scene = new PlaneScene(this);
    ui.xy_plane->setScene(scene);
    delete oldScene;
    decisionMap = nullptr;
    connect(scene, &PlaneScene::pointClicked, this, &Trainer::onPointClicked);

    double x, y;
    int label;

    while (file >> x >> y >> label)
    {
        if (label < 0 || label >= class_count || label > 8)
            continue;

        addPoint(x, y, label);

        QColor c = classColor(label);
        QGraphicsEllipseItem* dot = scene->addEllipse(x - 4, -y - 4, 8, 8, QPen(c), QBrush(c));
        dot->setZValue(1);
    }

    file.close();

    statusBar()->showMessage(QString("Loaded %1 points: %2").arg(point_count).arg(fileName));
}

void Trainer::train_network()
{
    const int N = point_count;

    if (N < 2)
    {
        statusBar()->showMessage("Add at least two points (one from each class)");
        return;
    }

    int class0Count = 0;
    int class1Count = 0;

    for (int i = 0; i < N; i++)
    {
        if (classes[i] == 0)
            class0Count++;
        else if (classes[i] == 1)
            class1Count++;
        else
        {
            statusBar()->showMessage("Training supports only classes 1 and 2");
            return;
        }
    }

    if (class0Count == 0 || class1Count == 0)
    {
        statusBar()->showMessage("Both classes need at least one sample");
        return;
    }

    if (decisionMap)
    {
        scene->removeItem(decisionMap);
        delete decisionMap;
        decisionMap = nullptr;
    }

    // Z-score parameters: mean and standard deviation of the training set.
    double mean[2] = { 0.0, 0.0 };
    double sigma[2] = { 0.0, 0.0 };

    for (int i = 0; i < N; i++)
    {
        mean[0] += points[2 * i];
        mean[1] += points[2 * i + 1];
    }

    mean[0] /= N;
    mean[1] /= N;

    for (int i = 0; i < N; i++)
    {
        const double dx = points[2 * i] - mean[0];
        const double dy = points[2 * i + 1] - mean[1];

        sigma[0] += dx * dx;
        sigma[1] += dy * dy;
    }

    sigma[0] = std::sqrt(sigma[0] / N);
    sigma[1] = std::sqrt(sigma[1] / N);

    if (sigma[0] < 1e-8)
        sigma[0] = 1.0;
    if (sigma[1] < 1e-8)
        sigma[1] = 1.0;

    // Normalized copy of the samples (sample[2*i], sample[2*i+1]).
    double* sample = new double[N * 2];

    for (int i = 0; i < N; i++)
    {
        sample[2 * i] = (points[2 * i] - mean[0]) / sigma[0];
        sample[2 * i + 1] = (points[2 * i + 1] - mean[1]) / sigma[1];
    }

    // Single neuron: two weights and one bias.
    double w[2];
    double b;

    w[0] = static_cast<double>(std::rand()) / RAND_MAX - 0.5;
    w[1] = static_cast<double>(std::rand()) / RAND_MAX - 0.5;
    b = static_cast<double>(std::rand()) / RAND_MAX - 0.5;

    int epoch = 0;
    double totalerror = 0.0;

    const double LearningRate = 0.1;
    const double min_error = 0.001 * N;
    const int max_epoch = 20000;

    do
    {
        totalerror = 0.0;

        for (int step = 0; step < N; step++)
        {
            const double net = w[0] * sample[2 * step]
                             + w[1] * sample[2 * step + 1]
                             + b;

            const double fnet = 2.0 / (1.0 + std::exp(-net)) - 1.0;
            const double fnet_der = 0.5 * (1.0 - fnet * fnet);

            const double d = (classes[step] == 0) ? 1.0 : -1.0;
            const double err = d - fnet;
            const double delta = LearningRate * err * fnet_der;

            w[0] += delta * sample[2 * step];
            w[1] += delta * sample[2 * step + 1];
            b += delta;

            totalerror += 0.5 * err * err;
        }

        epoch++;

    } while (totalerror > min_error && epoch < max_epoch);

    // Classify every pixel with the trained model and paint the plane.
    QImage image(800, 800, QImage::Format_RGB32);

    for (int row = 0; row < 800; row++)
    {
        for (int col = 0; col < 800; col++)
        {
            // Pixel centre -> model coordinates (clicked y is -scene y).
            const double x = -400.0 + col + 0.5;
            const double y = 400.0 - row - 0.5;

            const double xn = (x - mean[0]) / sigma[0];
            const double yn = (y - mean[1]) / sigma[1];

            const double net = w[0] * xn + w[1] * yn + b;
            const double fnet = 2.0 / (1.0 + std::exp(-net)) - 1.0;

            const int cls = (fnet > 0.0) ? 0 : 1;

            image.setPixel(col, row,
                (cls == 0) ? qRgb(255, 205, 205)
                           : qRgb(205, 220, 255));
        }
    }

    decisionMap = scene->addPixmap(QPixmap::fromImage(image));
    decisionMap->setPos(-400, -400);
    decisionMap->setZValue(-1);

    delete[] sample;

    statusBar()->showMessage(
        QString("epoch=%1  totalerror=%2  w=(%3, %4)  b=%5")
            .arg(epoch)
            .arg(totalerror)
            .arg(w[0], 0, 'f', 4)
            .arg(w[1], 0, 'f', 4)
            .arg(b, 0, 'f', 4));
}
