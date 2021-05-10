#ifndef FACEDETECTER_H
#define FACEDETECTER_H

#include <QThread>
#include <opencv2\opencv.hpp>

//线程对比      1:1,1:N
//人脸注册由主界面进行
class FaceDetecter : public QThread
{
    Q_OBJECT
public:
    explicit FaceDetecter(QObject *parent = nullptr);
    ~FaceDetecter();
    bool m_exitThread = true;
private:
    int cameraCount = 2;    //默认为双目摄像头    
    cv::Mat m_curRgbVideoFrame, m_curIrVideoFrame;							//rgb摄像头、ir摄像头当前帧
protected:
    void run() override;
public slots:
    void recvRgbFrame(cv::Mat frame);
    void recvIrFrame(cv::Mat frame);
};

#endif // FACEDETECTER_H
