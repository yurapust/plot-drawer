#include "renderthread.h"

#include <algorithm>
#include <QPixmap>
#include <QPainter>

#include <string>
#include <sstream>
#include <cmath>
#include <limits>

RenderThread::RenderThread(QObject *parent) : QThread(parent)
{

}

RenderThread::~RenderThread()
{
    stopThread();
}

void RenderThread::render(int pixmapOffset, double scaleFactor, QSize resultSize)
{
    if (plotFileData.points.empty())
        return;

    mutex.lock();
    exchData.pixmapOffset = pixmapOffset;
    exchData.scaleFactor  = scaleFactor;
    exchData.resultSize   = resultSize;
    restart = true;
    mutex.unlock();

    if (!isRunning()) {
        start(LowPriority);
    } else {
        condition.wakeOne();
    }
}

void RenderThread::setPlotFileData(DataLoader::FileData &plotFileData)
{
    stopThread();
    this->plotFileData = std::move(plotFileData);
    setupPlotData();
}

void RenderThread::stopThread()
{
    mutex.lock();
    abort = true;
    condition.wakeOne();
    mutex.unlock();

    wait();
}

void RenderThread::setupPlotData()
{
    abort = false;
    startPoint = 0;
    endPoint = this->plotFileData.points.size();
    pointsQuan = endPoint;
    findMinMaxValues();

    double maxScale = pointsQuan / minShownPoints;
    safeData.scaleFactor = maxScale;
    emit scaleMinMaxUpdated(1.0, maxScale);
}

void RenderThread::run()
{
    forever {
        mutex.lock();
        safeData.pixmapOffset = exchData.pixmapOffset;
        safeData.scaleFactor  = exchData.scaleFactor;
        safeData.resultSize   = exchData.resultSize;
        restart = false;
        mutex.unlock();

        calcStartAndEndPoints();
        std::vector<QPoint> plotPoints = calcPlottedPoints();
        QPixmap plot;
        if (!plotPoints.empty())
            plot = drawPixmap(plotPoints);

        auto pointsQuan = endPoint-startPoint;
        emit plotRendered(plot, safeData.scaleFactor, pointsQuan);

        mutex.lock();
        if (!restart)
            condition.wait(&mutex);
        if (abort) {
            mutex.unlock();
            return;
        }
        mutex.unlock();
    }
}

void RenderThread::calcStartAndEndPoints()
{
    auto displayedPoints = static_cast<size_t>( minShownPoints * safeData.scaleFactor);
    auto pointsOffset = calcPointsOffset(displayedPoints);

    calcStartPoint(displayedPoints, pointsOffset);
    calcEndPoint(displayedPoints);
}

void RenderThread::calcStartPoint(size_t displayedPoints, int pointsOffset)
{
    if (pointsOffset > 0) {
        startPoint += static_cast<size_t>(pointsOffset);
        if (startPoint + displayedPoints > pointsQuan) {
            startPoint = pointsQuan - displayedPoints;
        }
    } else {
        if (startPoint <= static_cast<size_t>(-pointsOffset)) {
            startPoint = 0;
        } else {
            startPoint -= static_cast<size_t>(-pointsOffset);
        }
    }

    if (startPoint + displayedPoints > pointsQuan)
        startPoint = pointsQuan - displayedPoints;
}

void RenderThread::calcEndPoint(size_t displayedPoints)
{
    endPoint = startPoint + displayedPoints;
    if (endPoint > pointsQuan)
        endPoint = pointsQuan;
}

int RenderThread::calcPointsOffset(size_t displayedPoints)
{
    if (displayedPoints == 0)
        return 0;

    int width = safeData.resultSize.width();
    auto displayed = convertToSigned(displayedPoints);
    int pointsOffset;

    if (width >= displayed) {
        int pixPerPoint = width / displayed;
        pointsOffset = safeData.pixmapOffset / pixPerPoint;
    } else {
        int pointsPerPix = displayed / width;
        pointsOffset = safeData.pixmapOffset * pointsPerPix;
    }

    return -pointsOffset;
}

int RenderThread::convertToSigned(size_t displayedPoints)
{
    int displayed;
    if (std::numeric_limits<int>::max() > displayedPoints)
        displayed = static_cast<int>(displayedPoints);
    else
        displayed = std::numeric_limits<int>::max();

    return displayed;
}

void RenderThread::findMinMaxValues()
{
    if (plotFileData.points.empty())
        return;

    using DataLoader::Point;
    auto minmax = std::minmax_element(plotFileData.points.cbegin(), plotFileData.points.cend(),
                                      [](const Point &a, const Point &b){ return a.value < b.value; });
    minValue = minmax.first->value;
    maxValue = minmax.second->value;
}

std::vector<QPoint> RenderThread::calcPlottedPoints()
{
    std::vector<QPoint> plotPoints;

    if ( pointsQuan == 0)
        return plotPoints;

    auto plotPointsQuan = endPoint - startPoint;
    plotPoints.reserve(plotPointsQuan);

    const size_t width = static_cast<size_t>(safeData.resultSize.width());
    const int height = safeData.resultSize.height();
    const double k = height / (maxValue-minValue);
    const double b = height * minValue / (minValue-maxValue);

    auto &points = plotFileData.points;
    int xpos, ypos;
    for (size_t abs = startPoint, rel = 0; abs < endPoint; ++abs, ++rel) {
        xpos = static_cast<int>(rel*width / (plotPointsQuan-1));
        ypos = static_cast<int>(k * points[abs].value + b);

        plotPoints.emplace_back(xpos, ypos);
    }

    return plotPoints;
}

QPixmap RenderThread::drawPixmap(const std::vector<QPoint> &plotPoints)
{
    QPixmap pix(safeData.resultSize.width(), safeData.resultSize.height());
    size_t width = static_cast<size_t>(safeData.resultSize.width());

    QPainter painter(&pix);
    painter.fillRect(pix.rect(), Qt::white);

    QPainterPath path;

    if (width >= plotPoints.size()) {
        path = drawAllPoints(plotPoints);
    } else if (width < plotPoints.size() && 2*width > plotPoints.size()) {
        path = drawPointsByMeanValue(plotPoints, width);
    } else {
        path = drawPointsByVertLines(plotPoints, width);
    }

    painter.setPen(Qt::SolidLine);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    return pix;
}

QPainterPath RenderThread::drawAllPoints(const std::vector<QPoint> &plotPoints)
{
    QPainterPath path;

    path.moveTo(plotPoints[0]);
    for (size_t i = 1; i < plotPoints.size(); ++i) {
        path.lineTo(plotPoints[i]);
    }

    return path;
}

QPainterPath RenderThread::drawPointsByMeanValue(const std::vector<QPoint> &plotPoints, size_t width)
{
    QPainterPath path;

    size_t pointPer10pix = 10*plotPoints.size() / width;
    size_t start, end;

    path.moveTo(plotPoints[0]);
    for (size_t i = 0; i < width; i += 10) {
        start = i*plotPoints.size() / width;
        end   = start + pointPer10pix;
        if (start >= plotPoints.size() || end >= plotPoints.size())
            break;

        auto summ = std::accumulate(plotPoints.cbegin()+start, plotPoints.cbegin()+end, 0.0,
                                          [](const double &a, const QPoint &b){ return a + b.y(); });

        auto mean = summ / pointPer10pix;
        path.lineTo(i, mean);
    }
    path.lineTo(plotPoints.back());

    return path;
}

QPainterPath RenderThread::drawPointsByVertLines(const std::vector<QPoint> &plotPoints, size_t width)
{
    QPainterPath path;

    size_t pointPerPix = plotPoints.size() / width;
    size_t start, end;
    double min, max;

    for (size_t i = 0; i < width; ++i) {
        start = i*plotPoints.size() / width;
        end   = start + pointPerPix;

        auto minmax = std::minmax_element(plotPoints.cbegin()+start, plotPoints.cbegin()+end,
                                          [](const QPoint &a, const QPoint &b){ return a.y() < b.y(); });
        min = minmax.first->y();
        max = minmax.second->y();

        path.moveTo(i, min);
        path.lineTo(i, max);
    }

    return path;
}
