#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMouseEvent>
#include <QPaintEvent>
#include <qevent.h>
#include <qimage.h>
#include <qmainwindow.h>
#include <qobjectdefs.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <vector>

class MainWindow : public QMainWindow{
    Q_OBJECT

    public:
        void initNetwork();
        void testNetwork();
        MainWindow(QWidget *parent = nullptr);
        void clearData();
        void saveData();
        ~MainWindow();

    protected:
        void mousePressEvent(QMouseEvent *event) override;
        void paintEvent(QPaintEvent *event) override;

    private:
        std::vector<float> samples;
        std::vector<int> targets;
        std::vector<float> weights;
        std::vector<float> bias;

        QImage backgroundImage;
        QPushButton *testButton;
        QPushButton * networkButton;
        QPushButton *saveButton;
        QPushButton *clearButton;
        QSpinBox *classInput;
        QSpinBox *labelInput;
        int class_count = 0;
        int inputDim = 2;
};

#endif
