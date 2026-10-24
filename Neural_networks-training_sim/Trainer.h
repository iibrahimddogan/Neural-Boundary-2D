#pragma once

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include "ui_Trainer.h"

class QGraphicsPixmapItem;

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

private slots:
    void onPointClicked(double x, double y);
    void save_data();
    void load_data();
    void train_network();

private:
    Ui::TrainerClass ui;
    PlaneScene* scene;

    double* points = nullptr;
    int* classes = nullptr;
    int point_count = 0;
    int capacity = 0;

    QGraphicsPixmapItem* decisionMap = nullptr;

    void addPoint(double x, double y, int cls);
};
