#include "Trainer.h"
#include <random>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QGraphicsLineItem>
#include <cmath>
#include <QString>

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

    weights.resize(3);
    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

    for (int i = 0; i < weights.size(); i++)
    {
        weights[i] = dis(gen);
    }

    connect(scene, &PlaneScene::pointClicked, this, &Trainer::onPointClicked);

    drawLine(weights[0] / 400.0f, weights[1] / 400.0f, weights[2]);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Trainer::calculate_weights);
    connect(ui.StartButton, &QPushButton::clicked, this, &Trainer::start_training);
}

Trainer::~Trainer() {}

void Trainer::onPointClicked(double x, double y)
{
    statusBar()->showMessage(QString("x = %1   y = %2").arg(x, 0, 'f', 0).arg(y, 0, 'f', 0));

    static const QColor renkler[] = { Qt::black, Qt::red, Qt::blue, Qt::darkYellow, Qt::green,
                                      Qt::cyan, Qt::magenta, Qt::gray, Qt::darkGreen };
    int sinif = ui.Spinbox_Class->value() - 1;
    samples.push_back(static_cast<float>(x));
    samples.push_back(static_cast<float>(y));
    targets.push_back(sinif);

    QColor c = renkler[sinif % 9];

    scene->addEllipse(x - 5, -y - 5, 10, 10, QPen(c), QBrush(c));
}

void Trainer::drawLine(double a, double b, double c) {
    double x1, x2, y1, y2;

    if (std::abs(b) < 1e-9)
    {
        if (std::abs(a) < 1e-9) return;
        x1 = x2 = -c / a;
        y1 = -400;
        y2 = 400;
    }
    else
    {
        x1 = -400;
        x2 = 400;
        y1 = -(a * x1 + c) / b;
        y2 = -(a * x2 + c) / b;
    }

    if (!line)
    {
        line = scene->addLine(x1, -y1, x2, -y2, QPen(Qt::red, 3));
    }
    else
    {
        line->setLine(x1, -y1, x2, -y2);
    }
}

int sgn(float val) {
    return (val >= 0.0f) ? 1 : -1;
}

void Trainer::start_training() {
    if (samples.empty()) return;

    epoch_count = 0;
    timer->start(100);
    statusBar()->showMessage(QString::number(epoch_count));
}

void Trainer::calculate_weights() {
    float r = 0.01f;

    if (samples.empty()) return;

    epoch_count++;

    int num_points = samples.size() / 2;
    bool error_found = false;

    for (int i = 0; i < num_points; ++i) {
        float x = samples[2 * i];
        float y = samples[2 * i + 1];

        float x_norm = x / 400.0f;
        float y_norm = y / 400.0f;

        int target = (targets[i] == 0) ? -1 : 1;

        float net = (weights[0] * x_norm) + (weights[1] * y_norm) + weights[2];

        int output = sgn(net);
        float error = target - output;

        if (error != 0.0f) {
            weights[0] += error * r * x_norm;
            weights[1] += error * r * y_norm;
            weights[2] += error * r * 1.0f;
            error_found = true;
        }
    }

    drawLine(weights[0] / 400.0f, weights[1] / 400.0f, weights[2]);

    if (!error_found) {
        timer->stop();
        statusBar()->showMessage(QString("%1 epoch").arg(epoch_count));
    }
}