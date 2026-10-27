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

static const QColor Colors[9] = {
    QColor(228,  26,  28),   // 1 - kirmizi
    QColor( 55, 126, 184),   // 2 - mavi
    QColor( 77, 175,  74),   // 3 - yesil
    QColor(152,  78, 163),   // 4 - mor
    QColor(255, 127,   0),   // 5 - turuncu
    QColor(166,  86,  40),   // 6 - kahverengi
    QColor(247, 129, 191),   // 7 - pembe
    QColor(  0, 160, 160),   // 8 - turkuaz
    QColor( 80,  80,  80)    // 9 - koyu gri
};

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

    if (inputDim > 8)
    {
        statusBar()->showMessage("Input Dim must be at most 8");
        return false;
    }

    return true;
}

void Trainer::onPointClicked(double x, double y)
{
    if (!ensureInputDim())
        return;

    int cls = ui.Spinbox_Class->value() - 1;

    if (cls < 0 || cls >= 9)
    {
        statusBar()->showMessage("Class must be between 1 and 9");
        return;
    }

    double feature[8] = {0};
    feature[0] = x;

    if (inputDim >= 2)
        feature[1] = y;

    addPoint(feature, cls);

    QColor c = Colors[cls];
    QGraphicsEllipseItem* dot = scene->addEllipse(x - 4, -y - 4, 8, 8, QPen(c), QBrush(c));
    dot->setZValue(1);

    statusBar()->showMessage(QString("x = %1   y = %2   sinif: %3   nokta sayisi: %4")
        .arg(x, 0, 'f', 1)
        .arg(y, 0, 'f', 1)
        .arg(cls + 1)
        .arg(numSample));
}

void Trainer::addPoint(const double* x, int cls)
{
    if (numSample == capacity)
    {
        int new_capacity = (capacity == 0) ? 8 : capacity * 2;

        double* new_sample = new double[new_capacity * inputDim];
        int* new_target = new int[new_capacity];

        for (int i = 0; i < numSample * inputDim; i++)
            new_sample[i] = sample[i];

        for (int i = 0; i < numSample; i++)
            new_target[i] = target[i];

        delete[] sample;
        delete[] target;

        sample = new_sample;
        target = new_target;
        capacity = new_capacity;
    }

    for (int d = 0; d < inputDim; d++)
        sample[inputDim * numSample + d] = x[d];

    target[numSample] = cls;
    numSample++;
}

void Trainer::save_data()
{
    if (!ensureInputDim())
        return;

    if (numSample == 0)
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
    for (int i = 0; i < numSample; i++)
        if (target[i] + 1 > class_count)
            class_count = target[i] + 1;

    file << inputDim << " " << 400 << " " << 400 << " " << class_count << "\n";

    for (int i = 0; i < numSample; i++)
    {
        for (int d = 0; d < inputDim; d++)
            file << sample[inputDim * i + d] << " ";

        file << target[i] << "\n";
    }

    file.close();

    statusBar()->showMessage(QString("Saved %1 points: %2").arg(numSample).arg(fileName));
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
    numSample = 0;
    capacity = 0;

    PlaneScene* oldScene = scene;
    scene = new PlaneScene(this);
    ui.xy_plane->setScene(scene);
    delete oldScene;
    decisionMap = nullptr;
    connect(scene, &PlaneScene::pointClicked, this, &Trainer::onPointClicked);

    double row[8] = {0};
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

        const double drawX = row[0];
        const double drawY = (inputDim >= 2) ? row[1] : 0.0;

        QColor c = Colors[label];
        QGraphicsEllipseItem* dot = scene->addEllipse(drawX - 4, -drawY - 4, 8, 8, QPen(c), QBrush(c));
        dot->setZValue(1);
    }

    file.close();

    statusBar()->showMessage(QString("Loaded %1 points: %2").arg(numSample).arg(fileName));
}

void Trainer::train_network()
{
    if (!ensureInputDim())
        return;

    const int N = numSample;

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

    // mean-variance
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

    
    double* normalized = new double[N * inputDim];

    for (int i = 0; i < N; i++)
    {
        for (int d = 0; d < inputDim; d++)
        {
            normalized[inputDim * i + d] =
                (sample[inputDim * i + d] - mean[d]) / sigma[d];
        }
    }

    double* w = new double[inputDim];
    double* b = new double[1];

    for (int d = 0; d < inputDim; d++)
        w[d] = static_cast<double>(std::rand()) / RAND_MAX - 0.5;

    b[0] = static_cast<double>(std::rand()) / RAND_MAX - 0.5;

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
            double net = 0.0;

            for (int d = 0; d < inputDim; d++)
                net += w[d] * normalized[inputDim * step + d];

            net += b[0];

            const double fnet = 2.0 / (1.0 + std::exp(-net)) - 1.0;
            const double fnet_der = 0.5 * (1.0 - fnet * fnet);

            double d;
            if (target[step] == 0)
            {
                d = 1.0;
            }
            else d = -1.0;

 
            const double err = d - fnet;
            const double delta = LearningRate * err * fnet_der;

            for (int j = 0; j < inputDim; j++)
                w[j] += delta * normalized[inputDim * step + j];

            b[0] += delta;

            totalerror += 0.5 * err * err;
        }

        epoch++;

    } while (totalerror > min_error && epoch < max_epoch);

   
    QImage image(800, 800, QImage::Format_RGB32);

    for (int row = 0; row < 800; row++)
    {
        for (int col = 0; col < 800; col++)
        {
           
            double feature[8] = {0};
            feature[0] = -400.0 + col + 0.5;

            if (inputDim >= 2)
                feature[1] = 400.0 - row - 0.5;

            for (int d = 2; d < inputDim; d++)
                feature[d] = mean[d];

            double net = 0.0;

            for (int d = 0; d < inputDim; d++)
                net += w[d] * ((feature[d] - mean[d]) / sigma[d]);

            net += b[0];

            const double fnet = 2.0 / (1.0 + std::exp(-net)) - 1.0;
            const int cls = (fnet > 0.0) ? 0 : 1;

            image.setPixel(col, row, Colors[cls].lighter(160).rgb());
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
            .arg(b[0], 0, 'f', 4));

    delete[] normalized;
    delete[] mean;
    delete[] sigma;
    delete[] w;
    delete[] b;
}
