#pragma execution_character_set("utf-8")
#include "arcfacemanager.h"
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QMutex>

#define INI_FILE  ".\\setting.ini"

#define FACE_FEATURE_SIZE 1032

QMutex m_mutex;
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

//返回单人脸带边框的图片
MRESULT ArcFaceManager::StaticImageSingleFaceOp(IplImage *image)
{
    //FD
    ASF_SingleFaceInfo faceInfo = { 0 };
    MRESULT detectRes = m_imageFaceEngine.PreDetectSingleFace(image, faceInfo, true);

    if (MOK == detectRes)
    {
        {
            //QMutexLocker locker(&m_mutex);
            cv::Mat img(image);
            cv::rectangle(img,cvPoint(faceInfo.faceRect.left,faceInfo.faceRect.bottom),
                          cvPoint(faceInfo.faceRect.right,faceInfo.faceRect.top),CV_RGB(255, 0, 0),5);
            IplImage m_Image(img);
            image = &m_Image;
        }


        //age gender
        ASF_MultiFaceInfo multiFaceInfo = { 0 };
        multiFaceInfo.faceOrient = (MInt32*)malloc(sizeof(MInt32));
        multiFaceInfo.faceRect = (MRECT*)malloc(sizeof(MRECT));

        multiFaceInfo.faceNum = 1;
        multiFaceInfo.faceOrient[0] = faceInfo.faceOrient;
        multiFaceInfo.faceRect[0] = faceInfo.faceRect;

        ASF_AgeInfo ageInfo = { 0 };
        ASF_GenderInfo genderInfo = { 0 };
        ASF_Face3DAngle angleInfo = { 0 };
        ASF_LivenessInfo liveNessInfo = { 0 };

        //age 、gender 、3d angle 信息
        detectRes = m_imageFaceEngine.FaceASFProcess(multiFaceInfo, image,
            ageInfo, genderInfo, angleInfo, liveNessInfo);

        if (MOK == detectRes)
        {
            qDebug()<< QString("年龄:%1,性别:%2,活体:%3").arg(ageInfo.ageArray[0]
                        ).arg(genderInfo.genderArray[0] == 0 ? "男" : "女"
                        ).arg(liveNessInfo.isLive[0] == 1 ? "是" : "否");
        }

        free(multiFaceInfo.faceRect);
        free(multiFaceInfo.faceOrient);
    }

    return detectRes;
}

//返回多人脸带边框的图片
//Bug:图片压缩后,部分边框不显示...
MRESULT ArcFaceManager::StaticImageMultiFaceOp(IplImage *image)
{
    ASF_MultiFaceInfo detectedFaces = { 0 };//人脸检测
    MRESULT detectRes = m_imageFaceEngine.PreDetectMultiFace(image, detectedFaces, true);
    if (MOK == detectRes)
    {
        for (int i = 0; i < detectedFaces.faceNum; i++)
        {
            //QMutexLocker locker(&m_mutex);
            cv::Mat img(image);
            cv::rectangle(img,cvPoint(detectedFaces.faceRect[i].left,detectedFaces.faceRect[i].bottom),
                          cvPoint(detectedFaces.faceRect[i].right,detectedFaces.faceRect[i].top),CV_RGB(255, 0, 0),5);
            IplImage m_Image(img);
            image = &m_Image;

            //faceID用于判断人脸是否有变化
            if(detectedFaces.faceID){
                qDebug() << QString("人脸id:%1").arg(detectedFaces.faceID[i]);
            }
        }

        ASF_AgeInfo ageInfo = { 0 };
        ASF_GenderInfo genderInfo = { 0 };
        ASF_Face3DAngle angleInfo = { 0 };
        ASF_LivenessInfo liveNessInfo = { 0 };

        //age 、gender 、3d angle 信息
        MRESULT other_detectRes = m_imageFaceEngine.FaceASFProcess(detectedFaces, image,
            ageInfo, genderInfo, angleInfo, liveNessInfo);

        if(other_detectRes == MOK){
            for (int i = 0; i < ageInfo.num; i++){
                qDebug()<< QString("ID:%1,年龄:%2").arg(i).arg(ageInfo.ageArray[i]);
            }

            for (int i = 0; i < genderInfo.num; i++)
            {
                qDebug()<< QString("ID:%1,性别:%2").arg(i).arg(genderInfo.genderArray[i] == 0 ? "男" : "女");
            }
            for (int i = 0; i < angleInfo.num; i++){
               // qDebug()<< QString("ID:%1,3D:%2").arg(i).arg(angleInfo.[i]);
            }

            for (int i = 0; i < liveNessInfo.num; i++)
            {
                qDebug()<< QString("ID:%1,活体:%2").arg(i).arg(liveNessInfo.isLive[i] == 1 ? "是" :"否");
            }
        }
    }

    return detectRes;
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
