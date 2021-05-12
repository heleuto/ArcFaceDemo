#ifndef CAMERATHREAD_H
#define CAMERATHREAD_H

#include <QThread>
#include <opencv2\opencv.hpp>
//摄像头采集

class CameraThread : public QThread
{
    Q_OBJECT
public:
    explicit CameraThread(QObject *parent = nullptr);
    CameraThread(int cameraID,QObject* parent = nullptr);
    bool OpenCamera(int id = -1);   //返回false,需要先关闭摄像头
    void CloseCamera(int msec = 600);
    ~CameraThread();
    bool m_exitThread = true;      //外部线程判断摄像头是否正常启用
protected:
    void run() override;
    int m_CameraID = 0;//摄像头ID
    bool m_videoOpened = false;
signals:
    void curFrame(cv::Mat);
    void cameraOpened(bool);
};

#endif // CAMERATHREAD_H
