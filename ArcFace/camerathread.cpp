#include "camerathread.h"
#include <QDebug>
#include <QMutex>

static QMutex g_mutex;

CameraThread::CameraThread(QObject *parent) :QThread(parent)
{

}

CameraThread::CameraThread(int cameraID, QObject *parent):QThread(parent),m_CameraID(cameraID)
{

}


bool CameraThread::OpenCamera(int id)
{
    if(!m_exitThread)   return false;

    if(id != -1)    m_CameraID = id;

    m_exitThread = false;
    this->start();

    return true;
}

CameraThread::~CameraThread()
{
    if(this->isRunning()){
        this->requestInterruption();
        this->quit();
        this->wait();
    }
}

void CameraThread::run()
{
    qDebug() << QString("Camera[%1] opening").arg(m_CameraID);
    msleep(1000);

    cv::Mat frame;
    cv::VideoCapture capture;

    m_videoOpened = capture.open(m_CameraID/*,cv::CAP_DSHOW*/);

    if(!m_videoOpened) qDebug() << QString("open Camera[%1] failed!").arg(m_CameraID) ;
    else qDebug() << QString("open Camera[%1] Successfully!").arg(m_CameraID) ;

    while(m_videoOpened && !isInterruptionRequested() && !m_exitThread){
        capture >> frame;
        if (frame.empty())    break;

        emit curFrame(frame);

        msleep(40);
    }
    capture.release();

    qDebug() << QString("Close Camera[%1]").arg(m_CameraID);
    m_exitThread = true;
}
