#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ArcFace/arcfacemanager.h"
#include "ArcFace/ccommon.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
class QTableView;
class QPushButton;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    enum StepType{
        ImageType,
        VideoType,
        UnknownType = -1
    };
private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_6_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_7_clicked();

    void on_pushButton_8_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_5_clicked();

    void rcvRgbFram(cv::Mat);

    void errorSlot(const UserFaceInformation &info);
private:
    Ui::MainWindow *ui;

    ArcFaceManager _arcFaceManger;

    void ImageOrVideo(StepType m_type = ImageType);

    void ImageStep(bool step = true);

    void VideoStep(bool step= true);

    bool initDatabase(QString dataBaseName = "Database.db");

    bool loadUserInfos();

    void loadFeaturesFromDataBase();

    void selectFaceFeatureById(int id);

    void clearTableView(QTableView *view);

    void initTable();

    StepType m_FaceType = ImageType;
    void DisableOtherBtns(QPushButton* btn = NULL);
    QList<QPushButton*> m_btns;
    //PurposeFlag m_flag = UnknownRequest;
    ASF_Flag m_flag;

    int curUserID = 0;

    void updateLocalFaceFeature();
    ASF_FaceFeature m_curDatabaseFeature;   //当前从数据库获取的人脸特征

    bool isClosing = false;

    void createDatabase();
protected:
    void closeEvent(QCloseEvent *event);
};
#endif // MAINWINDOW_H
