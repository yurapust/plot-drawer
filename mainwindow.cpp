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
    msgBox = new QMessageBox;
    msgBox->hide();

    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::open);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionFile_info, &QAction::triggered, msgBox, &QMessageBox::show);

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
                                                    lastOpenFile, "*.plot;;All files(*)");

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

    auto msg = createMsgAboutFileLoad(fileData);
    msgBox->setText(msg);
    ui->actionFile_info->setEnabled(true);

    thread.setPlotFileData(fileData);
    ui->centralWidget->renderNewFileData();
}

QString MainWindow::createMsgAboutFileLoad(DataLoader::FileData &fileData)
{
    QString msg("File info:\n");
    msg += "Loaded ";
    msg += std::to_string(fileData.points.size()).c_str();
    msg += " points\n";
    if (fileData.header.empty())
        msg += "File has't contain any info\n";
    else
        msg += fileData.header.c_str();

    if (fileData.error.empty())
        msg += "File loaded without errors\n";
    else {
        msg += "\nErrors:\n";
        msg += fileData.error.c_str();
    }

    return msg;
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
