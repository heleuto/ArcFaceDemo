#pragma execution_character_set("utf-8")
#include "arcfacemanager.h"
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QDebug>

#define INI_FILE  ".\\setting.ini"

#define FACE_FEATURE_SIZE 1032

ArcFaceManager::ArcFaceManager(QObject *parent) : QObject(parent)
{    
    if(!QFile::exists(INI_FILE))    //创建默认配置文件
    {
        QFile n_file(INI_FILE);
        if(n_file.open(QIODevice::WriteOnly | QIODevice::Truncate)){
            QFile m_File(":/rc/setting.ini");
            if(m_File.open(QIODevice::ReadOnly)){
                QTextStream out(&n_file);
                out << m_File.readAll();
                m_File.close();
            }
            n_file.close();
        }
    }

    ReadSetting();

    //初始化时要根据需要设置需要初始化的属性
    MRESULT faceRes = m_imageFaceEngine.ActiveSDK((char*)m_config.appID.toStdString().c_str(),
                             (char*)m_config.sdkKey.toStdString().c_str(),(char*)m_config.activeKey.toStdString().c_str());
    qDebug() << QString("激活结果: %1").arg(faceRes);
    //获取激活文件信息
    ASF_ActiveFileInfo activeFileInfo = { 0 };
    m_imageFaceEngine.GetActiveFileInfo(activeFileInfo);

    if (faceRes == MOK)
    {
        faceRes = m_imageFaceEngine.InitEngine(ASF_DETECT_MODE_IMAGE);//Image
        qDebug() << QString("IMAGE模式下初始化结果: %1").arg(faceRes);
        faceRes = m_videoFaceEngine.InitEngine(ASF_DETECT_MODE_VIDEO);//Video
        qDebug() << QString("VIDEO模式下初始化结果: %1").arg(faceRes);
    }

    m_curStaticImageFeature.featureSize = FACE_FEATURE_SIZE;
    m_curStaticImageFeature.feature = (MByte *)malloc(m_curStaticImageFeature.featureSize * sizeof(MByte));
}

ArcFaceManager::~ArcFaceManager()
{
    //应用程序关闭时,必须销毁引擎,否则会造成内存泄漏。
    m_imageFaceEngine.UnInitEngine();
    m_videoFaceEngine.UnInitEngine();
}

void ArcFaceManager::ReadSetting()
{
    QSettings settings(INI_FILE,QSettings::IniFormat);
    m_config.tag = settings.value("tag","x86_free").toString();
    settings.beginGroup(m_config.tag);
    m_config.appID = settings.value("APPID","Gwqzu5GzvEFBV1SeA8rK3nkXPuiHSbKALukPinpSeozt").toString();
    m_config.sdkKey = settings.value("SDKKEY","BcPoaw9eh4hrdcTvgTsJLZV7o5f6ZksLoNatYTqJqhXM").toString();
    m_config.activeKey = settings.value("ACTIVE_KEY","").toString();
    m_config.rgbLiveThreshold = settings.value("rgbLiveThreshold",0.5).toDouble();
    m_config.irLiveThreshold = settings.value("irLiveThreshold",0.7).toDouble();
    m_config.rgbCameraId = settings.value("rgbCameraId",1).toInt();
    m_config.irCameraId = settings.value("irCameraId",0).toInt();
    settings.endGroup();
}
