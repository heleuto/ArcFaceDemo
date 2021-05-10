#pragma execution_character_set("utf-8")

#include "facedetecter.h"
#include <QDebug>
#include "ArcFaceEngine.h"

#include <QMutex>

static QMutex rgbMutex,irMutex;

FaceDetecter::FaceDetecter(QObject *parent):QThread(parent)
{

}

FaceDetecter::~FaceDetecter()
{
    if(this->isRunning()){
        this->requestInterruption();
        this->quit();
        this->wait();
    }
}

void FaceDetecter::run()
{
    ArcFaceEngine m_videoFaceEngine;
    MRESULT faceRes = m_videoFaceEngine.InitEngine(ASF_DETECT_MODE_VIDEO);//Video
    qDebug() << QString("VIDEO模式下初始化结果:%1").arg(faceRes);
    if(faceRes != MOK){
        m_videoFaceEngine.UnInitEngine();
        return;
    }

    qDebug() << "Face detecting";
    m_exitThread = false;
    while(!isInterruptionRequested()){

        //





        msleep(50);
    }

    m_exitThread = true;
    m_videoFaceEngine.UnInitEngine();
}

void FaceDetecter::recvRgbFrame(cv::Mat frame)
{
    if(frame.empty())   return;

    {
        QMutexLocker locker(&rgbMutex);
        m_curRgbVideoFrame.release();
        m_curRgbVideoFrame = frame.clone();
        //qDebug() << "rgbFrame";
    }
}

void FaceDetecter::recvIrFrame(cv::Mat frame)
{
    if(frame.empty())   return;

    {
        QMutexLocker locker(&irMutex);
        m_curIrVideoFrame.release();
        m_curIrVideoFrame = frame.clone();
        //qDebug() << "irFrame";
    }
}
