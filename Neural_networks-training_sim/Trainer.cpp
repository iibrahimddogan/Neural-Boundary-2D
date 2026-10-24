#include "Trainer.h"
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QFrame>
#include <QString>
#include <QFileDialog>
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
    scene->addEllipse(x - 4, -y - 4, 8, 8, QPen(c), QBrush(c));

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
    connect(scene, &PlaneScene::pointClicked, this, &Trainer::onPointClicked);

    double x, y;
    int label;

    while (file >> x >> y >> label)
    {
        if (label < 0 || label >= class_count || label > 8)
            continue;

        addPoint(x, y, label);

        QColor c = classColor(label);
        scene->addEllipse(x - 4, -y - 4, 8, 8, QPen(c), QBrush(c));
    }

    file.close();

    statusBar()->showMessage(QString("Loaded %1 points: %2").arg(point_count).arg(fileName));
}
