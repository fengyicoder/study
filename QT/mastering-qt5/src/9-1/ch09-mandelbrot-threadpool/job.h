#ifndef JOB_H
#define JOB_H
#include <QObject>
#include <QRunnable>
#include <QPointF>
#include <QSize>
#include <QAtomicInteger>
#include "jobresult.h"

class Job: public QObject, public QRunnable
{
    Q_OBJECT
public:
    Job(QObject *parent=0);
    void run() override;

    void setPixelPositionY(int value);
    void setMoveOffset(const QPointF& value);
    void setScaleFactor(double value);
    void setAreaSize(const QSize& value);
    void setIterationMax(int value);
private:
    int mPixelPositionY;
    QPointF mMoveOffset;
    double mScaleFactor;
    QSize mAreaSize;
    int mIterationMax;
signals:
    void jobCompleted(JobResult jobResult);
public slots:
    void abort();
private:
    QAtomicInteger<bool> mAbort;
};

#endif // JOB_H
