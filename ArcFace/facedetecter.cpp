#pragma execution_character_set("utf-8")

#include "facedetecter.h"
#include <QDebug>
#include "ArcFaceEngine.h"

static QMutex rgbMutex,irMutex;

QMutex m_mutex;

#define FreeIplImage(x) { if(x != NULL){ cvReleaseImage(&x);} x = NULL; }

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

void FaceDetecter::setCameraState(bool m_state)
{
    cameraOpened = m_state;
}

void FaceDetecter::setMap(QMap<int, ASF_FaceFeature> &m_map)
{
    QMutexLocker locker(&m_mutex);
    m_featuresMap = &m_map;
}

void FaceDetecter::clearImages()
{
    {
        QMutexLocker locker(&rgbMutex);
        if(m_curRgbVideoImage){
            cvReleaseImage(&m_curRgbVideoImage);            
        }
        m_curRgbVideoImage = NULL;
    }

    {
        QMutexLocker locker(&irMutex);
        if(m_curIrVideoImage){
            cvReleaseImage(&m_curIrVideoImage);            
        }
        m_curIrVideoImage = NULL;
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

    faceRes = m_videoFaceEngine.SetLivenessThreshold(g_rgbLiveThreshold,g_irLiveThreshold);
    qDebug() << QString("设置活体阈值:%1").arg(faceRes == MOK ? "成功" : "失败");

    qDebug() << "Face detecter ready";
    m_exitThread = false;

    clock_t start, count = 0;

    //初始化特征
    ASF_FaceFeature faceFeature = { 0 };
    faceFeature.featureSize = FACE_FEATURE_SIZE;
    faceFeature.feature = (MByte *)malloc(faceFeature.featureSize * sizeof(MByte));

    ASF_MultiFaceInfo rgbFaceInfos = { 0 };
    rgbFaceInfos.faceOrient = (MInt32*)malloc(sizeof(MInt32));
    rgbFaceInfos.faceRect = (MRECT*)malloc(sizeof(MRECT));

    ASF_MultiFaceInfo irFaceInfos = { 0 };
    irFaceInfos.faceOrient = (MInt32*)malloc(sizeof(MInt32));
    irFaceInfos.faceRect = (MRECT*)malloc(sizeof(MRECT));

    while(!isInterruptionRequested() && !m_exitThread ){

        msleep(500);

        if(!cameraOpened){  //等待摄像头就绪
            qDebug() << "Wait for Camera ready";
            continue;
        }

        IplImage *rgbImage = NULL;
        IplImage *irImage = NULL;

        //IR活体检测
        bool isIRAlive = false;
        bool isRGBAlive = false;

        {
            QMutexLocker locker(&irMutex);
            if(m_curIrVideoImage){
                irImage = cvCloneImage(m_curIrVideoImage);
            }
        }

        {
            QMutexLocker locker(&rgbMutex);
            if(m_curRgbVideoImage){
                rgbImage = cvCloneImage(m_curRgbVideoImage);
            }
        }

        //双目摄像头模式
        if(dualCamera){
            //检测是否是有效IR人脸
            if(irImage){
                start = clock();

                MRESULT detectRes = m_videoFaceEngine.PreDetectMultiFace(irImage, irFaceInfos, true);

                count = clock() - start;
                std::cout << "IR Detect multi face time: "
                    << 1000.0 * count / CLOCKS_PER_SEC << "ms" << std::endl;

                if(detectRes == MOK){

                    ASF_LivenessInfo irLiveNessInfo = { 0 };
                    MRESULT irRes = m_videoFaceEngine.FaceASFProcess_IR(irFaceInfos, irImage, irLiveNessInfo);
                    if (irRes == MOK && irLiveNessInfo.num > 0)
                    {
                        if (irLiveNessInfo.isLive[0] == 1)
                        {
                            //dialog->m_curIRVideoShowString = "IR活体";
                            isIRAlive = true;
                        }
                        else if (irLiveNessInfo.isLive[0] == 0)
                        {
                            qDebug() << "IR假体";
                        }
                        else
                        {
                            //-1：不确定；-2:传入人脸数>1； -3: 人脸过小；-4: 角度过大；-5: 人脸超出边界
                            //qDebug() << QString("unknown IR live:%1").arg(liveNessInfo.isLive[0]) ;
                        }
                    }
//                    else
//                    {
                            //qDebug() << QString("IR FaceASFProcess:%1").arg(detectRes);
//                    }

//                    qDebug() << QString("IR face count:%1").arg(irFaceInfos.faceNum);
//                    QStringList m_strList;
//                    if(irFaceInfos.faceNum > 0){
//                        for(int i = 0 ;i < irFaceInfos.faceNum ;i++){
//                            m_strList << QString::number( irFaceInfos.faceID[i]);
//                        }
//                        qDebug() <<"IR Face IDs:" << m_strList;
//                    }

                }else{
                    qDebug() << QString("IR PreDetectMultiFace result :%1").arg(detectRes);
                }

                FreeIplImage(irImage);
            }
        }else{
            isIRAlive = true;   //单目摄像头默认IR摄像头检测到活体
        }

        if(rgbImage){

            start = clock();
            MRESULT detectRes = m_videoFaceEngine.PreDetectMultiFace(rgbImage, rgbFaceInfos, true);

            count = clock() - start;
            std::cout << "RGB Detect multi face time: "
                << 1000.0 * count / CLOCKS_PER_SEC << "ms" << std::endl;

            if(detectRes == MOK){
                //RGB属性检测
                //ASF_AgeInfo ageInfo = { 0 };
                ASF_GenderInfo genderInfo = { 0 };
                //ASF_Face3DAngle angleInfo = { 0 };
                ASF_LivenessInfo liveNessInfo = { 0 };

                MRESULT detectRes = m_videoFaceEngine.FaceASFProcessVideo(rgbFaceInfos, rgbImage,
                             genderInfo,  liveNessInfo);

                if (detectRes == 0 && liveNessInfo.num > 0)
                {
                    if (liveNessInfo.isLive[0] == 1)
                    {
                        isRGBAlive = true;
                    }
                    else if (liveNessInfo.isLive[0] == 0)
                    {
                        qDebug() << "RGB假体";
                    }
                    else
                    {
                        //-1：不确定；-2:传入人脸数>1； -3: 人脸过小；-4: 角度过大；-5: 人脸超出边界
                        //qDebug() << QString("unknown rgb live:%1").arg(liveNessInfo.isLive[0]) ;
                    }
                }
                /*else{
                    qDebug() << QString("rgb FaceASFProcess:%1").arg(detectRes);
                }*/

//                qDebug() << QString("RGB face count:%1").arg(rgbFaceInfos.faceNum);
//                QStringList m_strList;
//                if(rgbFaceInfos.faceNum > 0){
//                    for(int i = 0 ;i < rgbFaceInfos.faceNum ;i++){
//                        m_strList << QString::number( rgbFaceInfos.faceID[i]);
//                    }
//                    qDebug() <<"RGB Face IDs:" << m_strList;
//                }
            }else{
                qDebug() << QString("RGB PreDetectMultiFace result :%1").arg(detectRes);
            }
        }

        //非活体不做处理
        if (!(isRGBAlive && isIRAlive))
        {
            if (isRGBAlive && !isIRAlive)
            {
                qDebug() << "RGB活体";
            }
            FreeIplImage(rgbImage);
            continue;
        }

        if(m_featuresMap != NULL){
        //if((rgbFaceInfos.faceNum == 1)  && (m_featuresMap != NULL) && (irFaceInfos.faceNum == 1) ){

            //m_featuresMap如果在对比时被释放某个元素，有崩溃的风险...
            QMap<int,ASF_FaceFeature> tmp_map;

            {
                QMutexLocker locker(&m_mutex);
                tmp_map = *m_featuresMap;
            }

            if(tmp_map.size() > 0){
                qDebug() << "Matching";

                if (!(isRGBAlive && isIRAlive))
                {
                    if (isRGBAlive && !isIRAlive)
                    {
                        qDebug() << "RGB活体,IR假体";
                    }
                    FreeIplImage(rgbImage);
                    continue;
                }

                //特征提取
                start = clock();
                ASF_SingleFaceInfo m_curFaceInfo  = {rgbFaceInfos.faceRect[0],rgbFaceInfos.faceOrient[0]};

                MRESULT detectRes = m_videoFaceEngine.PreExtractFeature(rgbImage,
                    faceFeature, m_curFaceInfo);

                count = clock() - start;
                std::cout << "Extract Feature time: "
                    << 1000.0 * count / CLOCKS_PER_SEC << "ms" << std::endl;

                FreeIplImage(rgbImage);

                if (MOK != detectRes)
                {
                    continue;
                }

                MFloat maxThreshold = 0.0;

                MRESULT ret = MOK;
                int id = -1;

                start = clock();

                QMapIterator<int,ASF_FaceFeature> i(tmp_map);
                while (i.hasNext()) {
                    i.next();
                    ASF_FaceFeature feature2 = i.value();
                    MFloat confidenceLevel = 0;
                    ret = m_videoFaceEngine.FacePairMatching(confidenceLevel, faceFeature, feature2);
                    if(ret == MOK && confidenceLevel > maxThreshold){
                        id = i.key();
                        maxThreshold = confidenceLevel;
                        if(maxThreshold > g_compareThreshold)   break;
                    }
                }

                count = clock() - start;
                std::cout << "FacePair matched time: "
                    << 1000.0 * count / CLOCKS_PER_SEC << "ms" << std::endl;

                //超过阈值则表示对比成功

                if ((g_compareThreshold >= 0) &&
                    (maxThreshold >= g_compareThreshold) &&
                    isRGBAlive && isIRAlive)
                {
                    qDebug() << QString("ID:%1,Level:%2,RGB活体").arg(id).arg(maxThreshold);
                }
                else if (isRGBAlive)
                {
                    qDebug() << QString("RGB活体,level:%2").arg(maxThreshold);
                }

            }
            else{
                FreeIplImage(rgbImage);
            }
        }
        else{
            FreeIplImage(rgbImage);
            msleep(500);
            continue;
        }
    }

        //SafeFree(rgbFaceInfos.faceRect);
   // SafeFree(rgbFaceInfos.faceOrient);

        //SafeFree(irFaceInfos.faceRect);
    //SafeFree(irFaceInfos.faceOrient);

    SafeFree(faceFeature.feature);

    m_exitThread = true;
    m_videoFaceEngine.UnInitEngine();
}

void FaceDetecter::recvRgbFrame(cv::Mat frame)
{
    if(frame.empty())   return;

    {
        QMutexLocker locker(&rgbMutex);
        IplImage rgbImage(frame);
        if(m_curRgbVideoImage)
            cvReleaseImage(&m_curRgbVideoImage);

        m_curRgbVideoImage = cvCloneImage(&rgbImage);
    }
}

void FaceDetecter::recvIrFrame(cv::Mat frame)
{
    if(frame.empty())   return;

    {
        QMutexLocker locker(&irMutex);
        IplImage irImage(frame);
         if(m_curIrVideoImage)
             cvReleaseImage(&m_curIrVideoImage);

        m_curIrVideoImage = cvCloneImage(&irImage);
    }
}
