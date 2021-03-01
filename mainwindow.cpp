#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QStandardPaths>
#include <QFileInfo>
#include <cmath>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::open);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);

    fileDataLoading = new QFutureWatcher<DataLoader::FileData>(this);
    connect(fileDataLoading, &QFutureWatcher<DataLoader::FileData>::finished, this, &MainWindow::finished);

    connect(&thread, &RenderThread::plotRendered, ui->centralWidget, &PlotDrawer::updatePlot);
    qRegisterMetaType<size_t>("size_t");
    connect(&thread, &RenderThread::scaleMinMaxUpdated, ui->centralWidget, &PlotDrawer::updateMinMaxScale);
    connect(ui->centralWidget, &PlotDrawer::render, &thread, &RenderThread::render);
}

MainWindow::~MainWindow()
{
    fileDataLoading->waitForFinished();
    delete ui;
}

void MainWindow::open()
{
    if (fileDataLoading->isRunning()) {
       fileDataLoading->waitForFinished();
    }

    static QString lastOpenFile = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Files"),
                                                    lastOpenFile, "*.ssd *.rsd;;All files(*)");

    if (fileName.isNull())
        return;

    QFileInfo fileInfo(fileName);
    setWindowTitle(fileInfo.fileName());

    lastOpenFile = fileInfo.path();
    fileDataLoading->setFuture( QtConcurrent::run(DataLoader::loadMeasurementData, fileName.toStdString()) );
}

void MainWindow::finished()
{
    auto fileData = fileDataLoading->result();
    thread.setPlotFileData(fileData);
    ui->centralWidget->renderNewFileData();
}

std::vector<DataLoader::Point> MainWindow::calcTestPoints(size_t quan)
{
    double period = 60*3.14;
    double rad, x, y;
    std::vector<DataLoader::Point> testPoints;

    testPoints.reserve(quan);
    for (size_t i = 0; i < quan; ++i) {
        rad = (i * period) / (quan-1);
        x = rad/period;
        y = std::sin(rad);
        testPoints.emplace_back(x,y);
    }

    return testPoints;
}
