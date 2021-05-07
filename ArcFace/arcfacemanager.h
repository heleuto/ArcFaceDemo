#ifndef ARCFACEMANAGER_H
#define ARCFACEMANAGER_H

#include <QObject>
#include "ArcFaceEngine.h"
#include <QMap>

struct ArcFaceConfig{
    QString tag;
    QString appID;
    QString  sdkKey;
    QString  activeKey;
    double rgbLiveThreshold;
    double irLiveThreshold;
    short rgbCameraId;
    short irCameraId;
};

class ArcFaceManager : public QObject
{
    Q_OBJECT
public:
    explicit ArcFaceManager(QObject *parent = nullptr);
    ~ArcFaceManager();

    //可选参数m_takeFeature-->是否提取特征点
    MRESULT StaticImageSingleFaceOp(IplImage* image , bool  m_takeFeature = false);   //通过图片注册人脸时使用

    MRESULT StaticImageMultiFaceOp(IplImage* image , bool  m_takeFeature = false);    //空闲检测人脸时使用

    MRESULT AddFaceFeature(int id, ASF_FaceFeature * feature ); //添加人脸特征到映射表Map
    MRESULT RemoveFaceFeature(int  id,ASF_FaceFeature *feature = NULL); //将人脸特征移除,默认传人脸id即可
    MRESULT ClearFaceFeatures();

    ASF_FaceFeature* LastFaceFeature();      //获取最近的人脸特征,外部必须判断是否为NULL
signals:

private:
    void ReadSetting(); //载入参数配置

    ArcFaceConfig m_config;

    ArcFaceEngine m_imageFaceEngine;
    ArcFaceEngine m_videoFaceEngine;

    IplImage* m_curStaticImage = NULL;					//当前选中的图片
    //ASF_FaceFeature m_curStaticImageFeature;	//当前图片的人脸特征
    QMap<int,ASF_FaceFeature*>  m_faceFeatures;
    ASF_FaceFeature* currentFaceFeature = NULL; //当前人脸特征
    void curFaceFeatureInit();
    void freeCurFaceFeature();  //如果没提取到特征点,需要释放此对象
};

#endif // ARCFACEMANAGER_H
