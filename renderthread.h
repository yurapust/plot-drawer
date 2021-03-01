#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include <QMutex>
#include <QSize>
#include <QThread>
#include <QWaitCondition>
#include <vector>
#include <atomic>
#include <QPoint>
#include <QPainter>

#include "dataloader.h"

/*
 * Класс для вывода графика на QPixmap
 *
 * Функционал
 *  1. Принимает данные для отрисовки (setPlotFileData());
 *  2. По сигналу render() принимает данные, с информацией о том, какую часть графика отрисовавывать,
 *     и запускает отрисову в отдельном потоке;
 *  3. Для потокобезопасности используются два контейнера данных exchData и safeData;
 *  4. Данные (exchData, safeData, abort и restart), которые могут изменять разные потоки, защищены мьютексом.
 */
class RenderThread : public QThread
{
    Q_OBJECT

public:
    RenderThread(QObject *parent = nullptr);
    ~RenderThread() override;

    void setPlotFileData(DataLoader::FileData &plotFileData);

public slots:
    void render(int pixmapOffset, double scaleFactor, QSize resultSize);

signals:
    void scaleMinMaxUpdated(double min, double max);
    void plotRendered(const QPixmap &plot, double settedScaleFactor, size_t shownPoints);

protected:
    void run() override;

private:
    void stopThread();
    void setupPlotData();
    void findMinMaxValues();
    std::vector<QPoint> calcPlottedPoints();

    QPixmap drawPixmap(const std::vector<QPoint> &plotPoints);
    QPainterPath drawAllPoints(const std::vector<QPoint> &plotPoints);
    QPainterPath drawPointsByMeanValue(const std::vector<QPoint> &plotPoints, size_t width);
    QPainterPath drawPointsByVertLines(const std::vector<QPoint> &plotPoints, size_t width);

    int calcPointsOffset(size_t displayedPoints);
    int convertToSigned(size_t displayedPoints);
    void calcStartAndEndPoints();
    void calcStartPoint(size_t displayedPoints, int pointsOffset);
    void calcEndPoint(size_t displayedPoints);

    QMutex mutex;
    QWaitCondition condition;
    bool abort = false;
    bool restart = false;

    struct ExchData {
        int pixmapOffset;
        double scaleFactor;
        QSize resultSize;
    };

    ExchData exchData;
    ExchData safeData;

    size_t startPoint = 0;
    size_t endPoint = 0;

    size_t pointsQuan = 0;
    size_t minShownPoints = 2;

    DataLoader::FileData plotFileData;
    double minValue = 0.0;
    double maxValue = 0.0;
};

#endif // RENDERTHREAD_H
