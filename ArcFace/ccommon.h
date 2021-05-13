#ifndef CCOMMON_H
#define CCOMMON_H
#include <QString>

#define FACE_FEATURE_SIZE 1032

#define SafeFree(p) { if ((p)) free(p); (p) = NULL; }
#define SafeArrayDelete(p) { if ((p)) delete [] (p); (p) = NULL; }
#define SafeDelete(p) { if ((p)) delete (p); (p) = NULL; }

#define FreeIplImage(p) {if((p)) cvReleaseImage(&(p)); (p) = NULL;}

typedef enum{
    DoNothing,
    OneMatchMany,
    PairMatch,
    SignUpByFace,
}ASF_Flag ;

struct UserFaceInformation{
    ASF_Flag _asfFlag = DoNothing;
    int trackID = -1;            //人脸id
    int clashId = -1;            //冲突人脸id
    int resCode = -1;            //操作结果 0:操作成功
    QString errorStr = QString();
    void clear(){
        _asfFlag = DoNothing;
        trackID = -1;
        clashId = -1;
        resCode = -1;
        errorStr.clear();
    }
};
#endif // CCOMMON_H
