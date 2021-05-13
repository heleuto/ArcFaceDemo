#ifndef ARCFACEMANAGER_H
#define ARCFACEMANAGER_H

#include <QObject>
#include "ArcFaceEngine.h"
#include <QMap>
#include "ccommon.h"

struct ArcFaceConfig{
    QString tag;
    QString appID;
    QString  sdkKey;
    QString  activeKey;
    double rgbLiveThreshold;
    double irLiveThreshold;
    double compareThreshold;
    short rgbCameraId;
    short irCameraId;
    bool dualCamera;
};

QT_BEGIN_NAMESPACE
class CameraThread;
class FaceDetecter;
QT_END_NAMESPACE

class ArcFaceManager : public QObject
{
    Q_OBJECT
public:
    explicit ArcFaceManager(QObject *parent = nullptr);
    ~ArcFaceManager();

    bool OpenCameras();

    void CloseCameras();

    //可选参数m_takeFeature:是否提取特征点
    MRESULT StaticImageSingleFaceOp(IplImage* image , bool  m_takeFeature = false);   //通过图片注册人脸时使用

    MRESULT StaticImageMultiFaceOp(IplImage* image , bool  m_takeFeature = false);    //空闲检测人脸时使用

    MRESULT AddFaceFeature(int id, ASF_FaceFeature feature ); //添加人脸特征到映射表Map
    MRESULT RemoveFaceFeature(int  id); //将人脸特征移除,默认传人脸id即可
    MRESULT ClearFaceFeatures();

    ASF_FaceFeature LastFaceFeature();      //需要先判断提取特征是否成功!
    MRESULT FacePairMatching(MFloat &confidenceLevel, ASF_FaceFeature feature1, ASF_FaceFeature feature2,
        ASF_CompareModel compareModel = ASF_LIFE_PHOTO);
    //返回ID和匹配值
    MRESULT FaceMultiMathing(MFloat &confidenceLevel,ASF_FaceFeature feature, int& id, ASF_CompareModel compareModel = ASF_LIFE_PHOTO);

    //人脸对比,注册
    MRESULT DetecterControler(UserFaceInformation info);

signals:
    void curRgbFrame(cv::Mat);
    void curIrFrame(cv::Mat);
private:
    void ReadSetting(); //载入参数配置

    ArcFaceConfig m_config;

    ArcFaceEngine m_imageFaceEngine;

    IplImage* m_curStaticImage = NULL;          //当前选中的图片
    ASF_FaceFeature m_curStaticImageFeature;	//当前图片的人脸特征
    QMap<int,ASF_FaceFeature>  m_featuresMap;

    CameraThread *rgbCamera = NULL;
    CameraThread *irCamera = NULL;
    FaceDetecter *m_detecter = NULL;

    bool engineActived = false;

    bool rgbCameraOpened = false;
    bool irCameraOpened = false;

private slots:
    void CameraOpened(bool ok);    //用于获取摄像头实时状态
    void CameraClosed();
};

#endif // ARCFACEMANAGER_H
