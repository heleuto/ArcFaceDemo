#pragma execution_character_set("utf-8")

#include "facedetecter.h"
#include <QDebug>
#include "ccommon.h"

static QMutex rgbMutex,irMutex;

QMutex m_mutex;

FaceDetecter::FaceDetecter(QObject *parent):QThread(parent),curFlag(DoNothing)
{

}

FaceDetecter::~FaceDetecter()
{
    curFlag = DoNothing;
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

ASF_Flag FaceDetecter::DetecterSate()
{
    return curFlag;
}

void FaceDetecter::Controler(UserFaceInformation info)
{
    QMutexLocker locker(&m_mutex);
    if(info._asfFlag == DoNothing){
        m_loadFeatures = false;
        curUserInfo = UserFaceInformation();    //清空当前用户信息
    }else    curUserInfo = info;
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

void FaceDetecter::clearUserInfo()
{
    QMutexLocker locker(&m_mutex);
    curUserInfo = UserFaceInformation();    //清空当前用户信息
    m_loadFeatures = false;
}

void FaceDetecter::run()
{
    m_exitThread = false;
    ArcFaceEngine m_videoFaceEngine;
    MRESULT faceRes = m_videoFaceEngine.InitEngine(ASF_DETECT_MODE_VIDEO);//Video
    qDebug() << QString("VIDEO模式下初始化结果:%1").arg(faceRes);
    if(faceRes != MOK){
        m_videoFaceEngine.UnInitEngine();
        m_exitThread = true;
        return;
    }

    faceRes = m_videoFaceEngine.SetLivenessThreshold(g_rgbLiveThreshold,g_irLiveThreshold);
    qDebug() << QString("设置活体阈值:%1").arg(faceRes == MOK ? "成功" : "失败");

    qDebug() << "Face detecter ready";

    clock_t start, count = 0;

    //初始化特征
    ASF_FaceFeature faceFeature = { 0 };
    faceFeature.featureSize = FACE_FEATURE_SIZE;
    faceFeature.feature = (MByte *)malloc(faceFeature.featureSize * sizeof(MByte));

    ASF_MultiFaceInfo rgbFaceInfos = { 0 };
    ASF_MultiFaceInfo irFaceInfos = { 0 };

    QMap<int,ASF_FaceFeature> curFeatureMap;

    m_loadFeatures = false;

    while(!isInterruptionRequested() && !m_exitThread ){

        msleep(500);

        if(!cameraOpened){  //等待摄像头就绪
            //qDebug() << "Wait for Camera ready";
            continue;
        }

        UserFaceInformation _userInfo;
        {
            QMutexLocker locker(&m_mutex);
            _userInfo = curUserInfo;
        }

        curFlag = _userInfo._asfFlag;

        //qDebug() << "Wait for user's signal";

        if(curFlag == DoNothing) continue;

        //载入最新的特征库,如果人脸特征库为空，只能进行注册不能对比

        if(!m_loadFeatures)
        {
            QMutexLocker locker(&m_mutex);
            if(m_featuresMap != NULL)            curFeatureMap = *m_featuresMap;
            m_loadFeatures = true;
        }

        if( curFeatureMap.size() == 0 &&  curFlag != SignUpByFace){
            _userInfo.errorStr = "人脸数据库为空,请先注册人脸!";
            emit error(_userInfo);//发送信号告诉调用者错误原因
            clearUserInfo();
            continue;
        }

        //识别人脸并提取特征
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
//                std::cout << "IR Detect multi face time: "
//                    << 1000.0 * count / CLOCKS_PER_SEC << "ms" << std::endl;

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
                }else{
                    //qDebug() << QString("IR PreDetectMultiFace result :%1").arg(detectRes);
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
//            std::cout << "RGB Detect multi face time: "
//                << 1000.0 * count / CLOCKS_PER_SEC << "ms" << std::endl;

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
            }else{
                //qDebug() << QString("RGB PreDetectMultiFace result :%1").arg(detectRes);
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

        //注册或者对比前提取特征

        //特征提取
        start = clock();
        ASF_SingleFaceInfo m_curFaceInfo  = {rgbFaceInfos.faceRect[0],rgbFaceInfos.faceOrient[0]};

        MRESULT detectRes = m_videoFaceEngine.PreExtractFeature(rgbImage,
                                                                faceFeature, m_curFaceInfo);

        count = clock() - start;

//        std::cout << "Extract Feature time: "
//            << 1000.0 * count / CLOCKS_PER_SEC << "ms" << std::endl;

        FreeIplImage(rgbImage); //???BUG

        if (MOK != detectRes)
        {
            continue;
        }

         //if((rgbFaceInfos.faceNum == 1) && (irFaceInfos.faceNum == 1) ){
            //}
        switch (curFlag) {
        case OneMatchMany:  //1:N
        case SignUpByFace:  //人脸注册,注册前先检索是否有冲突的人脸
        {
            MFloat maxThreshold = 0.0;
            MRESULT ret = MOK;
            int id = -1;

            start = clock();
            QMapIterator<int,ASF_FaceFeature> i(curFeatureMap);
            while (i.hasNext()) {
                i.next();
                ASF_FaceFeature feature2 = i.value();
                MFloat confidenceLevel = 0;
                ret = m_videoFaceEngine.FacePairMatching(confidenceLevel, faceFeature, feature2);
                if(ret == MOK && confidenceLevel > maxThreshold){
                    id = i.key();
                    maxThreshold = confidenceLevel;
                    if(maxThreshold > g_compareThreshold)   break;
                    if(curFlag == DoNothing)    break;
                }
            }

            count = clock() - start;
            std::cout << "FacePair matched time: "
                << 1000.0 * count / CLOCKS_PER_SEC << "ms" << std::endl;

            qDebug() << QString("FacePair Threshold:%1").arg(maxThreshold);

            if(curFlag == DoNothing)    break;      //外部结束对比

            //超过阈值则表示对比成功

            if ((g_compareThreshold >= 0) &&
                    (maxThreshold >= g_compareThreshold))
            {
                qDebug() << QString("ID:%1,Level:%2,RGB活体").arg(id).arg(maxThreshold);
                if(curFlag == OneMatchMany){
                    _userInfo.errorStr = QString("人员识别成功!ID:%1").arg(id);
                    _userInfo.resCode = 0;
                    _userInfo.trackID = id;
                    emit error(_userInfo);//人员ID
                }else{
                    _userInfo.errorStr = QString("已有注册人员,ID为:%1").arg(id);
                    _userInfo.trackID = id;
                    _userInfo.clashId = id;
                    emit error(_userInfo);//人员ID,人员冲突,注册失败
                }
            }
            else if((g_compareThreshold >= 0) && (maxThreshold < g_compareThreshold) ){
                if(curFlag == SignUpByFace){
                    if(_userInfo.trackID == -1){
                        _userInfo.resCode = -1;
                        _userInfo.errorStr = "人员ID错误,人脸注册失败!";
                    }else{

                        //注册人员信息
                        {
                            QMutexLocker locker(&m_mutex);
                            if(m_featuresMap != NULL){
                                m_featuresMap->insert(_userInfo.trackID,faceFeature);
                            }
                        }
                        _userInfo.errorStr =QString("人脸注册成功!ID:%1").arg(_userInfo.trackID) ;
                        _userInfo.resCode = 0;
                    }

                    emit error(_userInfo);

                }else{
                    _userInfo.errorStr = "人员识别失败!";
                    emit error(_userInfo);//人员识别失败
                }
            }
            else{
                //参数错误
            }
        }
            break;
        case PairMatch:     //1:1
        {
            MFloat maxThreshold = 0.0;
            MRESULT ret = MOK;

            if(curFeatureMap.contains(_userInfo.trackID)){
                start = clock();

                ASF_FaceFeature feature2 = curFeatureMap.value(_userInfo.trackID);
                if(feature2.feature != NULL){
                    MFloat confidenceLevel = 0;
                    ret = m_videoFaceEngine.FacePairMatching(confidenceLevel, faceFeature, feature2);
                    if(ret == MOK && confidenceLevel > maxThreshold){
                        maxThreshold = confidenceLevel;
                    }
                }

                count = clock() - start;
                std::cout << "FacePair matched time: "
                    << 1000.0 * count / CLOCKS_PER_SEC << "ms" << std::endl;

                qDebug() << QString("FacePair Threshold:%1").arg(maxThreshold);

                //超过阈值则表示对比成功
                if ((g_compareThreshold >= 0) &&
                        (maxThreshold >= g_compareThreshold))
                {
                    qDebug() << QString("ID:%1,Level:%2,RGB活体").arg(_userInfo.trackID).arg(maxThreshold);
                        _userInfo.resCode = 0;
                        _userInfo.errorStr = QString("1:1对比成功!对比ID:%1").arg(_userInfo.trackID);
                        emit error(_userInfo);//人员ID
                }
                else if((g_compareThreshold >= 0) && (maxThreshold < g_compareThreshold) ){
                        _userInfo.errorStr = QString("1:1对比失败!对比ID:%1").arg(_userInfo.trackID);
                        emit error(_userInfo);//人员识别失败
                }
                else{
                    _userInfo.errorStr = QString("1:1对比失败!对比ID:%1,原因:阈值错误").arg(_userInfo.trackID);
                    emit error(_userInfo);//人员识别失败
                }
            }
            else{
                _userInfo.errorStr = QString("1:1对比失败!ID错误,错误ID:%1").arg(_userInfo.trackID);
                emit error(_userInfo);//人员识别失败
            }
        }
            break;
        case DoNothing:
            break;
        default:
            break;
        }

        //操作完成后,进入等待状态
        clearUserInfo();
    }

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
