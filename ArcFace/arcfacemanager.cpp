#pragma execution_character_set("utf-8")
#include "arcfacemanager.h"
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QMutex>
#include "camerathread.h"
#include "facedetecter.h"

#define INI_FILE  ".\\setting.ini"

#define SafeFree(x) { if(x != NULL){ free(x); x = NULL;} }

QMutex m_mutex;
ArcFaceManager::ArcFaceManager(QObject *parent) : QObject(parent)
{    
    qRegisterMetaType<cv::Mat>("cv::Mat");

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

    rgbCamera = new CameraThread(0);
    irCamera = new CameraThread(1);

    m_detecter = new FaceDetecter;

    connect(rgbCamera,&CameraThread::curFrame,m_detecter,&FaceDetecter::recvRgbFrame);
    connect(irCamera,&CameraThread::curFrame,m_detecter,&FaceDetecter::recvIrFrame);

    connect(rgbCamera,&CameraThread::curFrame,this,&ArcFaceManager::curRgbFrame);
    connect(irCamera,&CameraThread::curFrame,this,&ArcFaceManager::curIrFrame);

    //初始化时要根据需要设置需要初始化的属性
    MRESULT faceRes = m_imageFaceEngine.ActiveSDK((char*)m_config.appID.toStdString().c_str(),
                                                  (char*)m_config.sdkKey.toStdString().c_str(),(char*)m_config.activeKey.toStdString().c_str());
    qDebug() << QString("激活结果:%1").arg(faceRes);
    //获取激活文件信息
    ASF_ActiveFileInfo activeFileInfo = { 0 };
    m_imageFaceEngine.GetActiveFileInfo(activeFileInfo);

    if (faceRes == MOK)
    {
        engineActived = true;               //引擎初始成功
        faceRes = m_imageFaceEngine.InitEngine(ASF_DETECT_MODE_IMAGE);//Image
        qDebug() << QString("IMAGE模式下初始化结果:%1").arg(faceRes);

        rgbCamera->OpenCamera();
        irCamera->OpenCamera();

        m_detecter->start();
    }

    m_curStaticImageFeature.featureSize = FACE_FEATURE_SIZE;
    m_curStaticImageFeature.feature = (MByte *)malloc(m_curStaticImageFeature.featureSize * sizeof(MByte));
}

ArcFaceManager::~ArcFaceManager()
{
    delete m_detecter;
    delete rgbCamera;
    delete irCamera;
    //应用程序关闭时,必须销毁引擎,否则会造成内存泄漏。
    m_imageFaceEngine.UnInitEngine();
    ClearFaceFeatures();
    SafeFree(m_curStaticImage);
}

//返回单人脸带边框的图片
MRESULT ArcFaceManager::StaticImageSingleFaceOp(IplImage *image,bool  m_takeFeature)
{
    //FD
    ASF_SingleFaceInfo faceInfo = { 0 };
    MRESULT detectRes = m_imageFaceEngine.PreDetectSingleFace(image, faceInfo, true);

    if (MOK == detectRes)
    {
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

        //获取特征
        if(m_takeFeature){
            detectRes = m_imageFaceEngine.PreExtractFeature(image, m_curStaticImageFeature/**currentFaceFeature*/, faceInfo);
        }

        {
            //QMutexLocker locker(&m_mutex);
            cv::Mat img(image);
            cv::rectangle(img,cvPoint(faceInfo.faceRect.left,faceInfo.faceRect.bottom),
                          cvPoint(faceInfo.faceRect.right,faceInfo.faceRect.top),CV_RGB(255, 0, 0),5);
            IplImage m_Image(img);
            image = &m_Image;
        }
    }

    return detectRes;
}

//返回多人脸带边框的图片
//Bug:图片压缩后,部分边框不显示...
MRESULT ArcFaceManager::StaticImageMultiFaceOp(IplImage *image,bool  m_takeFeature)
{
    ASF_MultiFaceInfo detectedFaces = { 0 };        //人脸检测
    MRESULT detectRes = m_imageFaceEngine.PreDetectMultiFace(image, detectedFaces, true);
    if (MOK == detectRes )
    {
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

        if(m_takeFeature){
            if(detectedFaces.faceNum > 1 )       detectRes = MERR_ASF_OTHER_FACE_COUNT_ERROR; //人脸数量过多
            else{
                ASF_SingleFaceInfo faceInfo = {
                    detectedFaces.faceRect[0],
                    detectedFaces.faceOrient[0]
                };

                detectRes = m_imageFaceEngine.PreExtractFeature(image, m_curStaticImageFeature, faceInfo);
            }
        }

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
    }

    return detectRes;
}

MRESULT ArcFaceManager::AddFaceFeature(int id, ASF_FaceFeature feature)
{
    MRESULT faceRes = MOK;

    {
        QMutexLocker locker(&m_mutex);
        //删除旧数据
        if(m_featuresMap.contains(id)){
            ASF_FaceFeature mFeature = m_featuresMap.take(id);
            SafeFree(mFeature.feature);
        }

        m_featuresMap.insert(id,feature);
    }

    return faceRes;
}

MRESULT ArcFaceManager::RemoveFaceFeature(int id)
{
    MRESULT faceRes = MOK;
    {
        QMutexLocker locker(&m_mutex);
        if(m_featuresMap.contains(id)){
            ASF_FaceFeature mFeature = m_featuresMap.take(id);
            SafeFree(mFeature.feature);
        }else faceRes = -1;
    }

    return faceRes;
}

MRESULT ArcFaceManager::ClearFaceFeatures()
{
    {
        QMutexLocker locker(&m_mutex);
        foreach(ASF_FaceFeature feature,m_featuresMap){
            SafeFree(feature.feature);
        }
        m_featuresMap.clear();
    }

    return MOK;
}

ASF_FaceFeature ArcFaceManager::LastFaceFeature()
{
    return m_curStaticImageFeature;
}

MRESULT ArcFaceManager::FacePairMatching(MFloat &confidenceLevel, ASF_FaceFeature feature1, ASF_FaceFeature feature2, ASF_CompareModel compareModel)
{
    return m_imageFaceEngine.FacePairMatching(confidenceLevel,feature1,feature2,compareModel);
}

MRESULT ArcFaceManager::FaceMultiMathing(MFloat &confidenceLevel, ASF_FaceFeature feature, int& id, ASF_CompareModel compareModel)
{
    MRESULT ret = MOK;
    QMapIterator<int,ASF_FaceFeature> i(m_featuresMap);
    while (i.hasNext()) {
        i.next();
        ASF_FaceFeature feature2 = i.value();
        ret = m_imageFaceEngine.FacePairMatching(confidenceLevel,feature,feature2,compareModel);
        if(ret == MOK){
            id = i.key();
            break;
        }
    }

    return ret;
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
