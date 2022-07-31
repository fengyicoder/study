#ifndef MANDELBROTCALCULATOR_H
#define MANDELBROTCALCULATOR_H

#include <QObject>
#include <QSize>
#include <QPointF>
#include <QElapsedTimer>
#include <QList>

#include "jobresult.h"

class Job;

class MandelbrotCalculator: public QObject
{
    Q_OBJECT
public:
    explicit MandelbrotCalculator(QObject *parent=0);
    void init(QSize imageSize);
signals:
    void pictureLinearsGenerated(QList<JobResult> jobResults);
    void abortAllJobs();
public slots:
    void generatePicture(QSize areaSize, QPointF moveOffset, double scaleFactor, int iterationMax);
    void process(JobResult jobResult);
private:
    Job* createJob(int pixelPositionY);
    void clearJobs();
private:
    QPointF mMoveOffset;
    double mScaleFactor;
    QSize mAreaSize;
    int mIterationMax;
    int mReceiverJobResults;
    QList<JobResult> mJobResults;
    QElapsedTimer mTimer;
};

#endif // MANDELBROTCALCULATOR_H
