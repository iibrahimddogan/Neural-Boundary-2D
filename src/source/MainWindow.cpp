#include "MainWindow.h"
#include "Process.h"
#include <QPainter>
#include <iostream>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QFileDialog>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QFile>
#include <QTextStream>
#include <QColor>
#include <QImage>
#include <vector>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent){
    resize(1920, 1080);
    setWindowTitle("Data Collect 2D");


    QLabel *infolabel{new QLabel("Class Number", this)};
    infolabel->setGeometry(width() * 6 / 8, height() / 8, 200, 90);

    classInput = new QSpinBox{this};
    classInput->setGeometry(width() * 6 / 8 + 100, height() / 8, 200, 90);
    classInput->setRange(1, 5);

    QLabel *infoLabel{new QLabel("Label Number", this)};
    infoLabel->setGeometry(width() * 6 / 8, height() / 8 + 600, 200, 90);

    labelInput = new QSpinBox{this};
    labelInput->setGeometry(width() * 6 / 8 + 100, height() / 8 + 600, 200, 90);
    labelInput->setRange(1, 5);

    clearButton = new QPushButton{"Clear Data", this};
    clearButton->setGeometry(width() * 6 / 8 + 100, height() / 8 + 120, 200, 50);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearData);

    saveButton = new QPushButton{"Save Data", this};
    saveButton->setGeometry(width() * 6 / 8 + 100, height() / 8 + 240, 200, 50);
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::saveData);

    networkButton = new QPushButton{"Set Network", this};
    networkButton->setGeometry(width() * 6 / 8 + 100, height() / 8 + 360, 200, 50);
    connect(networkButton, &QPushButton::clicked, this, &MainWindow::initNetwork);

    testButton = new QPushButton{"Test Network", this};
    testButton->setGeometry(width() * 6 / 8 + 100, height() / 8 + 480, 200, 50);
    connect(testButton, &QPushButton::clicked, this, &MainWindow::testNetwork);
}

void MainWindow::saveData(){
    QString filename{QFileDialog::getSaveFileName(this, "Save Data", "", "CSV File (*.csv);;Text File (*.txt)")};

    if (filename.isEmpty()) {
        return;
    }

    QFile file{filename};
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        std::cout << "Can not open file\n";
        return;
    }

    QTextStream out{&file};
    out << "X,Y,Label\n";

    for (size_t i{0}; i < samples.size(); i += 2) {
        out << samples[i] << "," << samples[i+1] << "," << targets[i/2] << "\n";
    }

    file.close();
    std::cout << "Data saved: " << filename.toStdString() << std::endl;
}

void MainWindow::clearData(){
    samples.clear();
    targets.clear();
    update();
}

MainWindow::~MainWindow(){}

void MainWindow::mousePressEvent(QMouseEvent *event){
    int x {event->x()};
    int y {event->y()};

    int squareX {width() / 8};
    int squareY {height() / 4};
    int boxsize {600};

    if (x < squareX || x > squareX + boxsize || y < squareY || y > squareY + boxsize) {
        std::cout << "invalid input!\n";
        return;
    }

    int xCoordinates {x - squareX - 300};
    int yCoordinates {y - squareY - 300};
    std::cout << "x = " << xCoordinates << " y = " << -1 * yCoordinates;

    int currentClass{labelInput->value()};
    std::cout << " Class = " << currentClass << std::endl;

    samples.push_back(xCoordinates);
    samples.push_back(-1 * yCoordinates);
    targets.push_back(currentClass - 1);
    update();
}

void MainWindow::paintEvent(QPaintEvent *event){
    QPainter painter{this};

    painter.fillRect(rect(), Qt::white);

    QPen axisPen{Qt::black, 2};
    painter.setPen(axisPen);

    painter.drawLine(width() / 8 + 300, height() / 4 , width() / 8 + 300, height() / 4 + 600);
    painter.drawLine(width() / 8, height() / 4 + 300, width() / 8 + 600, height() / 4 + 300);
    painter.drawRect(width() / 8, height() / 4, 600, 600);

    if (!backgroundImage.isNull()) {
        painter.drawImage(width() / 8, height() / 4, backgroundImage);
    }

    int centerX {width() / 8 + 300};
    int centerY {height() / 4 + 300};

    for (size_t i{0}; i < samples.size(); i += 2) {

        int sampleX {static_cast<int>(samples[i])};
        int sampleY {static_cast<int>(samples[i+1])};
        int targetClass {targets[i/2]};

        QPen pointPen{Qt::black, 3};


        switch (targetClass) {
            case 0: pointPen.setColor(Qt::black); break;
            case 1: pointPen.setColor(Qt::red); break;
            case 2: pointPen.setColor(Qt::blue); break;
            case 3: pointPen.setColor(Qt::yellow); break;
            case 4: pointPen.setColor(Qt::green); break;
            default: pointPen.setColor(Qt::cyan);
        }
        painter.setPen(pointPen);

        int screenX {centerX + sampleX};
        int screenY {centerY - sampleY};

        painter.drawLine(screenX - 5, screenY, screenX + 5, screenY);
        painter.drawLine(screenX, screenY - 5, screenX, screenY + 5);
    }
}

void MainWindow::initNetwork(){

    class_count = classInput->value();

    if (class_count > 2) {
        weights = init_array_random(class_count * 2);
        bias = init_array_random(class_count);
    }
    else {
        weights = init_array_random(2);
        bias = init_array_random(1);
    }

    networkButton->setText("Network Ready");
}

void MainWindow::testNetwork(){

    if (samples.empty()) {
        std::cout << "Enter the data first\n";
        return;
    }


    if (weights.empty() || bias.empty()) {
        std::cout << "Network is not initialized! Click 'Set Network' first.\n";
        return;
    }

    std::vector<float> mean;
    std::vector<float> stdDev;
    Z_Score_Parameters(samples, mean, stdDev);


    if (stdDev[0] == 0.0f || stdDev[1] == 0.0f) {
        std::cout << "Not enough variance in data (StdDev is 0).\n";
        return;
    }

    backgroundImage = QImage(600, 600, QImage::Format_ARGB32);
    backgroundImage.fill(Qt::white);

    std::vector<float> x(2, 0.0f);
    int num;

    for (int row = 0; row < 600; row +=2) {
        for (int col = 0; col < 600; col +=2) {
            float pixelX { col - 300.0f};
            float pixelY { 300.0f - row};

            x[0] = (pixelX - mean[0]) / stdDev[0];
            x[1] = (pixelY - mean[1]) / stdDev[1];

            num = Test_Forward(x, weights, bias, class_count);

            QColor c;
            switch (num) {
                case 0: c = Qt::black; break;
                case 1: c = Qt::red; break;
                case 2: c = Qt::blue; break;
                case 3: c = Qt::yellow; break;
                case 4: c = Qt::green; break;
                default: c = Qt::cyan;
            }

            backgroundImage.setPixelColor(col, row, c);
            if (col + 1 < 600) backgroundImage.setPixelColor(col + 1, row, c);
            if (row + 1 < 600) backgroundImage.setPixelColor(col, row + 1, c);
            if (col + 1 < 600 && row + 1 < 600) backgroundImage.setPixelColor(col + 1, row + 1, c);
        }
    }
    update();
}
