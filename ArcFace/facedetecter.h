#ifndef FACEDETECTER_H
#define FACEDETECTER_H

#include <QThread>
#include <opencv2\opencv.hpp>
#include <QMap>
#include "ArcFaceEngine.h"
#include <QMutex>
#include "ccommon.h"

extern QMutex m_mutex;

//线程对比      1:1,1:N
//人脸注册由主界面进行
class FaceDetecter : public QThread
{
    Q_OBJECT
public:
    explicit FaceDetecter(QObject *parent = nullptr);
    ~FaceDetecter();
    bool m_exitThread = true;
    void setCameraState(bool m_state);
    void setMap(QMap<int,ASF_FaceFeature> &m_map);
    void setDualCamera(bool m_dual){
        dualCamera = m_dual;
    }
    void setRgbLiveThreshold(float val){
        g_rgbLiveThreshold = val;
    }
    void setIrLiveThreshold(float val){
        g_irLiveThreshold = val;
    }

    void setCompareThreshold(float val){
        g_compareThreshold = val;
    }

    bool cameraState(){
        return cameraOpened;
    }

    ASF_Flag DetecterSate();

    //ADD
    void Controler(UserFaceInformation info);

private:   
    IplImage* m_curRgbVideoImage = NULL;   //rgb摄像头、ir摄像头当前帧
    IplImage* m_curIrVideoImage = NULL;
    void clearImages();

    bool cameraOpened = false;  //所有摄像头准备就绪?
    QMap<int,ASF_FaceFeature>  *m_featuresMap = NULL;
    bool dualCamera = true;     //默认为双目摄像头

    float g_rgbLiveThreshold = 0.0;
    float g_irLiveThreshold = 0.0;
    float g_compareThreshold = 0.8;

    UserFaceInformation curUserInfo;
    ASF_Flag curFlag;
    bool m_loadFeatures = false;    //是否需要载入特征库
    void clearUserInfo();

protected:
    void run() override;
public slots:
    void recvRgbFrame(cv::Mat frame);
    void recvIrFrame(cv::Mat frame);

signals:
    void error(const UserFaceInformation &info);
};

#endif // FACEDETECTER_H
