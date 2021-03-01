#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtConcurrent>

#include "dataloader.h"
#include "renderthread.h"

namespace Ui {
class MainWindow;
}

/*
 * Главное окно приложения
 *
 * Функционал:
 *  1. Настраивает связи между всеми классами приложения (механизм сигнал-слот Qt)
 *  2. Выполняет чтение данных из файла в отделном потоке с помощью QFuture (функция open())
 *  3. Запускает отрисовку графика (функция finished())
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void open();
    void finished();

private:
    Ui::MainWindow *ui;
    RenderThread thread;

    QFutureWatcher<DataLoader::FileData> *fileDataLoading;

    std::vector<DataLoader::Point> calcTestPoints(size_t quan);
};

#endif // MAINWINDOW_H
