#include "MandelbrotCalculator.h"
#include <QDebug>
#include <QThreadPool>
#include "job.h"

const int JOB_RESULT_THRESHOLD = 10;

MandelbrotCalculator::MandelbrotCalculator(QObject *parent):
    QObject(parent), mMoveOffset(0.0, 0.0), mScaleFactor(0.005), mAreaSize(0, 0),
    mIterationMax(10), mReceiverJobResults(0), mJobResults(), mTimer()
{  
}

void MandelbrotCalculator::generatePicture(QSize areaSize, QPointF moveOffset, double scaleFactor, int iterationMax)
{
    if (areaSize.isEmpty()) return;
    mTimer.start();
    clearJobs();
    mAreaSize = areaSize;
    mMoveOffset = moveOffset;
    mScaleFactor = scaleFactor;
    mIterationMax = iterationMax;

    for (int pixelPositionY=0; pixelPositionY<mAreaSize.height(); pixelPositionY++)
    {
        QThreadPool::globalInstance()->start(createJob(pixelPositionY));
    }
}

Job* MandelbrotCalculator::createJob(int pixelPositionY)
{
    Job* job = new Job();
    job->setPixelPositionY(pixelPositionY);
    job->setMoveOffset(mMoveOffset);
    job->setScaleFactor(mScaleFactor);
    job->setAreaSize(mAreaSize);
    job->setIterationMax(mIterationMax);
    connect(this, &MandelbrotCalculator::abortAllJobs, job, &Job::abort);
    connect(job, &Job::jobCompleted, this, &MandelbrotCalculator::process);
    return job;
}

void MandelbrotCalculator::clearJobs()
{
    mReceiverJobResults = 0;
    emit abortAllJobs();
    QThreadPool::globalInstance()->clear();
}

void MandelbrotCalculator::process(JobResult jobResult)
{
    if (jobResult.areaSize != mAreaSize ||
            jobResult.moveOffset != mMoveOffset ||
            jobResult.scaleFactor != mScaleFactor) {
        return;
    }
    mReceiverJobResults++;
    mJobResults.append(jobResult);
    if (mJobResults.size() >= JOB_RESULT_THRESHOLD ||
            mReceiverJobResults == mAreaSize.height()) {
        emit pictureLinearsGenerated(mJobResults);
        mJobResults.clear();
    }
    if (mReceiverJobResults == mAreaSize.height()) {
        qDebug() << "Generated in " << mTimer.elapsed() << " ms";
    }
}
