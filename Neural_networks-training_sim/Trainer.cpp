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
    delete[] sample;
    delete[] target;
}

bool Trainer::ensureInputDim()
{
    inputDim = ui.Spinbox_InputDim->value();

    if (inputDim == 0)
    {
        statusBar()->showMessage("Set Input Dim first");
        return false;
    }

    if (inputDim != 2)
    {
        statusBar()->showMessage("This 2D plane requires Input Dim = 2");
        return false;
    }

    return true;
}

void Trainer::onPointClicked(double x, double y)
{
    if (!ensureInputDim())
        return;

    int cls = ui.Spinbox_Class->value() - 1;

    double feature[2] = { x, y };
    addPoint(feature, cls);

    QColor c = classColor(cls);
    QGraphicsEllipseItem* dot = scene->addEllipse(x - 4, -y - 4, 8, 8, QPen(c), QBrush(c));
    dot->setZValue(1);

    statusBar()->showMessage(QString("x = %1   y = %2   sinif: %3   nokta sayisi: %4")
        .arg(x, 0, 'f', 1)
        .arg(y, 0, 'f', 1)
        .arg(cls + 1)
        .arg(point_count));
}

void Trainer::addPoint(const double* x, int cls)
{
    if (point_count == capacity)
    {
        int new_capacity = (capacity == 0) ? 8 : capacity * 2;

        double* new_sample = new double[new_capacity * inputDim];
        int* new_target = new int[new_capacity];

        for (int i = 0; i < point_count * inputDim; i++)
            new_sample[i] = sample[i];

        for (int i = 0; i < point_count; i++)
            new_target[i] = target[i];

        delete[] sample;
        delete[] target;

        sample = new_sample;
        target = new_target;
        capacity = new_capacity;
    }

    for (int d = 0; d < inputDim; d++)
        sample[inputDim * point_count + d] = x[d];

    target[point_count] = cls;
    point_count++;
}

void Trainer::save_data()
{
    if (!ensureInputDim())
        return;

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
        if (target[i] + 1 > class_count)
            class_count = target[i] + 1;

    file << inputDim << " " << 400 << " " << 400 << " " << class_count << "\n";

    for (int i = 0; i < point_count; i++)
    {
        for (int d = 0; d < inputDim; d++)
            file << sample[inputDim * i + d] << " ";

        file << target[i] << "\n";
    }

    file.close();

    statusBar()->showMessage(QString("Saved %1 points: %2").arg(point_count).arg(fileName));
}

void Trainer::load_data()
{
    if (!ensureInputDim())
        return;

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

    if (!(file >> dim >> w >> h >> class_count))
    {
        statusBar()->showMessage("Invalid file format");
        return;
    }

    if (dim != inputDim)
    {
        statusBar()->showMessage("File Input Dim does not match");
        return;
    }

    if (w <= 0 || h <= 0 || class_count < 2)
    {
        statusBar()->showMessage("Invalid file format");
        return;
    }

    delete[] sample;
    delete[] target;
    sample = nullptr;
    target = nullptr;
    point_count = 0;
    capacity = 0;

    PlaneScene* oldScene = scene;
    scene = new PlaneScene(this);
    ui.xy_plane->setScene(scene);
    delete oldScene;
    decisionMap = nullptr;
    connect(scene, &PlaneScene::pointClicked, this, &Trainer::onPointClicked);

    double* row = new double[inputDim];
    int label;

    while (true)
    {
        bool ok = true;

        for (int d = 0; d < inputDim; d++)
        {
            if (!(file >> row[d]))
            {
                ok = false;
                break;
            }
        }

        if (!ok)
            break;

        if (!(file >> label))
            break;

        if (label < 0 || label >= class_count || label > 8)
            continue;

        addPoint(row, label);

        QColor c = classColor(label);
        QGraphicsEllipseItem* dot = scene->addEllipse(row[0] - 4, -row[1] - 4, 8, 8, QPen(c), QBrush(c));
        dot->setZValue(1);
    }

    delete[] row;
    file.close();

    statusBar()->showMessage(QString("Loaded %1 points: %2").arg(point_count).arg(fileName));
}

void Trainer::train_network()
{
    if (!ensureInputDim())
        return;

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
        if (target[i] == 0)
            class0Count++;
        else if (target[i] == 1)
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
    double* mean = new double[inputDim];
    double* sigma = new double[inputDim];

    for (int d = 0; d < inputDim; d++)
    {
        mean[d] = 0.0;
        sigma[d] = 0.0;
    }

    for (int i = 0; i < N; i++)
    {
        for (int d = 0; d < inputDim; d++)
            mean[d] += sample[inputDim * i + d];
    }

    for (int d = 0; d < inputDim; d++)
        mean[d] /= N;

    for (int i = 0; i < N; i++)
    {
        for (int d = 0; d < inputDim; d++)
        {
            const double diff = sample[inputDim * i + d] - mean[d];
            sigma[d] += diff * diff;
        }
    }

    for (int d = 0; d < inputDim; d++)
    {
        sigma[d] = std::sqrt(sigma[d] / N);

        if (sigma[d] < 1e-8)
            sigma[d] = 1.0;
    }

    // Normalized copy of the samples.
    double* normalized = new double[N * inputDim];

    for (int i = 0; i < N; i++)
    {
        for (int d = 0; d < inputDim; d++)
        {
            normalized[inputDim * i + d] =
                (sample[inputDim * i + d] - mean[d]) / sigma[d];
        }
    }

    // Single neuron: inputDim weights and one bias.
    double* w = new double[inputDim];
    double b;

    for (int d = 0; d < inputDim; d++)
        w[d] = static_cast<double>(std::rand()) / RAND_MAX - 0.5;

    b = static_cast<double>(std::rand()) / RAND_MAX - 0.5;

    int epoch = 0;
    double totalerror = 0.0;

    const double LearningRate = ui.Spinbox_LearningRate->value();
    const double min_error = ui.Spinbox_MinError->value() * N;
    const int max_epoch = ui.Spinbox_MaxEpoch->value();

    do
    {
        totalerror = 0.0;

        for (int step = 0; step < N; step++)
        {
            double net = b;

            for (int d = 0; d < inputDim; d++)
                net += w[d] * normalized[inputDim * step + d];

            const double fnet = 2.0 / (1.0 + std::exp(-net)) - 1.0;
            const double fnet_der = 0.5 * (1.0 - fnet * fnet);

            const double d = (target[step] == 0) ? 1.0 : -1.0;
            const double err = d - fnet;
            const double delta = LearningRate * err * fnet_der;

            for (int j = 0; j < inputDim; j++)
                w[j] += delta * normalized[inputDim * step + j];

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
            const double feature[2] = {
                -400.0 + col + 0.5,
                400.0 - row - 0.5
            };

            double net = b;

            for (int d = 0; d < inputDim; d++)
                net += w[d] * ((feature[d] - mean[d]) / sigma[d]);

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

    QString weightText;
    for (int d = 0; d < inputDim; d++)
        weightText += QString("%1 ").arg(w[d], 0, 'f', 4);

    statusBar()->showMessage(
        QString("epoch=%1  totalerror=%2  w=(%3)  b=%4")
            .arg(epoch)
            .arg(totalerror)
            .arg(weightText)
            .arg(b, 0, 'f', 4));

    delete[] normalized;
    delete[] mean;
    delete[] sigma;
    delete[] w;
}
