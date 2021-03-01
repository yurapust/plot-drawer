#include "plotdrawer.h"

#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QRect>
#include <QKeyEvent>
#include <QPoint>
#include <QVector>
#include <cmath>
#include <limits>

const double ZoomInFactor = 0.8;
const double ZoomOutFactor = 1.0 / ZoomInFactor;
const int ScrollStep = 20;

PlotDrawer::PlotDrawer(QWidget *parent) : QWidget(parent)
{
#ifndef QT_NO_CURSOR
    setCursor(Qt::CrossCursor);
#endif
    resize(650, 400);
}

void PlotDrawer::renderNewFileData()
{
    curScale = maxScale;
    emit render(pixmapOffset, curScale, size());
}

void PlotDrawer::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);

    if (pixmap.isNull()) {
        drawHelpMessage(painter);
        return;
    }

    if ( qFuzzyCompare(curScale, pixmapScale) ) {
        drawPixmap(painter);
    } else {
        drawScaledPixmap(painter);
    }
}

void PlotDrawer::drawHelpMessage(QPainter &painter)
{
    painter.setPen(Qt::black);
    painter.fillRect(rect(), Qt::lightGray);
    painter.drawText(rect(), Qt::AlignCenter, tr("To open a file, select: File/Open"));
}

void PlotDrawer::drawPixmap(QPainter &painter)
{
    int step = width() / (shownPoints-1);
    if (step == 0)
        step = 1;
    int offset = pixmapOffset - pixmapOffset % step;
    painter.drawPixmap(offset, 0, pixmap);
}

void PlotDrawer::drawScaledPixmap(QPainter &painter)
{
    // Расчет масшатаба (scaleFactor) аналогично RenderThread. Так как масштаб пересчитывается для более удобного отображения
    auto pointsByScale = static_cast<int>(shownPoints / pixmapScale);
    auto displayedPoints = static_cast<int>( pointsByScale * curScale);
    auto curScale = static_cast<double>(displayedPoints) / pointsByScale;
    double scaleFactor = pixmapScale / curScale;

    painter.save();
    painter.scale(scaleFactor, 1.0);
    QRectF exposed = painter.matrix().inverted().mapRect(rect()).adjusted(-1, -1, 1, 1);
    painter.drawPixmap(exposed, pixmap, exposed);
    painter.restore();
}

void PlotDrawer::resizeEvent(QResizeEvent * /* event */)
{
    update();
    emit render(pixmapOffset, curScale, size());
}

void PlotDrawer::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Plus:
        zoom(ZoomInFactor);
        break;
    case Qt::Key_Minus:
        zoom(ZoomOutFactor);
        break;
    case Qt::Key_Left:
        scroll(-ScrollStep);
        break;
    case Qt::Key_Right:
        scroll(+ScrollStep);
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

#if QT_CONFIG(wheelevent)
void PlotDrawer::wheelEvent(QWheelEvent *event)
{
    int numDegrees = event->delta() / 8;
    double numSteps = static_cast<double>(numDegrees) / 15.0;
    zoom(pow(ZoomInFactor, numSteps));
}
#endif

void PlotDrawer::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        lastDragPos = event->pos().x();
}

void PlotDrawer::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        pixmapOffset += event->pos().x() - lastDragPos;
        lastDragPos = event->pos().x();
        update();
    }
}

void PlotDrawer::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        pixmapOffset += event->pos().x() - lastDragPos;
        lastDragPos = 0;

        scroll(pixmapOffset);
    }
}

void PlotDrawer::zoom(double zoomFactor)
{
    curScale *= zoomFactor;

    if (curScale < minScale)
        curScale = minScale;
    if (curScale > maxScale)
        curScale = maxScale;

    update();
    emit render(pixmapOffset, curScale, size());
}

void PlotDrawer::scroll(int pointsOffset)
{
    update();
    emit render(pointsOffset, curScale, size());
}

void PlotDrawer::updatePlot(const QPixmap &plot, double scaleFactor, size_t newShownPoints)
{
    if (lastDragPos)
        return;

    pixmap = plot;
    pixmapOffset = 0;
    lastDragPos = 0;
    pixmapScale = scaleFactor;
    shownPoints = newShownPoints;

    update();
}

void PlotDrawer::updateMinMaxScale(double min, double max)
{
    minScale = min;
    maxScale = max;
}

