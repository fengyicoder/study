#ifndef MANDELBROTWIDGET_H
#define MANDELBROTWIDGET_H

#include <QWidget>
#include <memory>
#include <QPoint>
#include <QThread>
#include <QList>

#include "MandelbrotCalculator.h"

class QResizeEvent;

class MandelbrotWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MandelbrotWidget(QWidget *parent = nullptr);
    ~MandelbrotWidget();

private:
    QRgb generateColorFromIteration(int iteration);

    MandelbrotCalculator mMandelbrotCalculator;
    QThread mThreadCalculator;
    double mScaleFactor;
    QPoint mLastMouseMovePosition;
    QPointF mMoveOffset;
    QSize mAreaSize;
    int mIterationMax;
    std::unique_ptr<QImage> mImage;

signals:
    void requestPicture(QSize areaSize, QPointF moveOffset, double scaleFactor,
                        int iterationMax);
protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

public slots:
    void processJobResults(QList<JobResult> jobResults);
};

#endif // MANDELBROTWIDGET_H
