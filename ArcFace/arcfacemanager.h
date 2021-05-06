#ifndef ARCFACEMANAGER_H
#define ARCFACEMANAGER_H

#include <QObject>
#include "ArcFaceEngine.h"

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
signals:

private:
    void ReadSetting(); //载入参数配置

    ArcFaceConfig m_config;

    ArcFaceEngine m_imageFaceEngine;
    ArcFaceEngine m_videoFaceEngine;

    IplImage* m_curStaticImage;					//当前选中的图片
    ASF_FaceFeature m_curStaticImageFeature;	//当前图片的人脸特征
};

#endif // ARCFACEMANAGER_H
