#pragma once

#include <vector>
#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QTimer>
#include "ui_Trainer.h"

class PlaneScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit PlaneScene(QObject* parent = nullptr);

signals:
    void pointClicked(double x, double y);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
};

class Trainer : public QMainWindow
{
    Q_OBJECT
public:
    Trainer(QWidget* parent = nullptr);
    ~Trainer();

    void drawLine(double a, double b, double c);

private slots:
    void onPointClicked(double x, double y);
    void calculate_weights();
    void start_training();

private:
    Ui::TrainerClass ui;
    PlaneScene* scene;
    QGraphicsLineItem* line = nullptr;
    std::vector<float> samples;
    std::vector<float> targets;
    std::vector<float> weights;
    QTimer* timer;
    int epoch_count = 0;
};