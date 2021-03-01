#ifndef PLOTDRAWER_H
#define PLOTDRAWER_H

#include <QWidget>
#include <QPixmap>

/*
 * Класс для отображения графика
 *
 * Функционал:
 *  1. Реализован на базе примера Qt mandelbrot
 *  2. Отображает QPixmap, расчитанный в потоке RenderThread;
 *  3. Формирует данные (координаты, размер и масшаб), которые необходимы для отрисовки графика и
 *     отправляет их в RenderThread для отрисовки;
 *  4. Пока новые данные расчитываются, масшабирует или передвигает текующий QPixmap;
 */
class PlotDrawer : public QWidget
{
    Q_OBJECT
public:
    explicit PlotDrawer(QWidget *parent = nullptr);
    void renderNewFileData();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *event) override;
#endif
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
    void render(int pixmapOffset, double scaleFactor, QSize resultSize);

public slots:
    void updatePlot(const QPixmap &plot, double scaleFactor, size_t newShownPoints);
    void updateMinMaxScale(double min, double max);

private:
    void zoom(double zoomFactor);
    void scroll(int pointsOffset);

    void drawHelpMessage(QPainter &painter);
    void drawPixmap(QPainter &painter);
    void drawScaledPixmap(QPainter &painter);

    QPixmap pixmap;
    int pixmapOffset = 0;
    int lastDragPos = 0;

    double pixmapScale = 1.0;
    double curScale = 1.0;
    size_t shownPoints  = 10;

    double minScale = 1.0;
    double maxScale = 10.0;
};

#endif // PLOTDRAWER_H
