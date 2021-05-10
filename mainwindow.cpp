#pragma execution_character_set("utf-8")
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDebug>
#include "ArcFace/ImageConverter.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardItemModel>
#include <QSqlError>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle(tr("人脸识别"));
    initTable();
    ImageStep();
    if(initDatabase()){
        qDebug() << "数据库加载成功!";
        loadUserInfos();
    }else qDebug() << "数据库加载失败!";

    m_btns << ui->pushButton
           << ui->pushButton_2
           << ui->pushButton_3
           << ui->pushButton_4
           << ui->pushButton_5
           << ui->pushButton_6
           << ui->pushButton_7;

    m_curDatabaseFeature.featureSize = FACE_FEATURE_SIZE;
    m_curDatabaseFeature.feature = (MByte *)malloc(m_curDatabaseFeature.featureSize * sizeof(MByte));
    connect(&_arcFaceManger,&ArcFaceManager::curRgbFrame,this,&MainWindow::rcvRgbFram);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open Image"), "", tr("Picture Files (*.png *.jpg *.jpeg *.bmp)"));
    if (fileName.isEmpty()){
        return;
    }
    qDebug()<< fileName;
    ui->textBrowser->append(QString("载入图片:%1").arg(fileName));

    IplImage* image = cvLoadImage(fileName.toStdString().c_str());

    //图片加载失败
    if(!image){
        cvReleaseImage(&image);
        return;
    }

    ui->label->clear();
    MRESULT ret = _arcFaceManger.StaticImageMultiFaceOp(image,
                                                        (m_flag == RegisterFace )|| (m_flag == Identify) ||
                                                        (m_flag == RecognizeLocal) ? true : false);
    if(MOK == ret){
        ui->textBrowser->append(tr("检测到人脸"));
        updateLocalFaceFeature();
    }
    else if(ret == MERR_ASF_OTHER_FACE_COUNT_ERROR){
        ui->textBrowser->append(tr("人脸数量过多,请选择正确的图片!"));
    }
    else{
        ui->textBrowser->append(tr("未检测到人脸,请选择正确的图片!"));
    }

    ui->label->setPixmap(QPixmap::fromImage(*IplImage2QImage(image)).scaled(ui->label->size(),Qt::KeepAspectRatio)); //内存泄漏?

    //m_flag = UnknownRequest;
}

//线程打开摄像头
void MainWindow::on_pushButton_2_clicked()
{
    if(ui->pushButton_2->text() == "打开摄像头"){
        ImageOrVideo(VideoType);

    }else{
        ImageOrVideo(ImageType);

    }
}

void MainWindow::ImageStep(bool step)
{
    if(step){
        ui->pushButton->setEnabled(true);
        ui->lineEdit->setEnabled(false);
    }else{
        ui->pushButton->setEnabled(false);
        ui->lineEdit->setEnabled(true);
    }
}

void MainWindow::VideoStep(bool step)
{
    if(step){
        ui->pushButton_2->setText(tr("关闭摄像头"));
    }else{
        ui->pushButton_2->setText(tr("打开摄像头"));
    }
}

bool MainWindow::initDatabase(QString dataBaseName)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setHostName("localhost");
    db.setDatabaseName(dataBaseName);
    //db.setUserName("mojito");
    //db.setPassword("J0a1m8");
    bool ok = db.open();
    return ok;
}

bool MainWindow::loadUserInfos()
{
    clearTableView(ui->tableView);
    bool ok =false;
    QSqlQuery query;

    //不能通过null判断
    ok = query.exec("select USERID,STAFFID,NAME,FACEINFO from Manager ");

    if(!ok){
        qDebug() <<QString("加载用户信息失败:%1").arg(query.lastError().text());
    }

    QStandardItemModel *m = new QStandardItemModel;
    m->setColumnCount(4);
    m->setHeaderData(0,Qt::Horizontal,"ID");
    m->setHeaderData(1,Qt::Horizontal,"工号");
    m->setHeaderData(2,Qt::Horizontal,"姓名");
    m->setHeaderData(3,Qt::Horizontal,"人脸信息");

    while(query.next()){
        QList<QStandardItem*> list;
        list << new  QStandardItem(query.value("USERID").toString())
             << new  QStandardItem(query.value("STAFFID").toString())
             << new QStandardItem(query.value("NAME").toString());
        QByteArray _info = query.value("FACEINFO").toByteArray();
        if(_info.length() > 0){
            QStandardItem *Item = new QStandardItem("已注册");
            Item->setForeground(QBrush(QColor(255, 0, 0)));
            list << Item ;
        }else{
            list << new QStandardItem("未注册");
        }
        m->appendRow(list);
    }
    ui->tableView->setModel(m);


    query.clear();
    loadFeaturesFromDataBase();
    return ok;
}

void MainWindow::loadFeaturesFromDataBase()
{
    QSqlQuery query;
    query.exec("select USERID, FACEINFO from Manager where FACEINFO is not null");
    while(query.next()){
        int faceId = query.value("USERID").toInt();
        QByteArray faceInfo =query.value("FACEINFO").toByteArray();

        ASF_FaceFeature feature;
        feature.featureSize = faceInfo.length();
        feature.feature = (MByte *)malloc(feature.featureSize * sizeof(MByte));
        memset(feature.feature,0,sizeof (feature.featureSize));
        memcpy(feature.feature,(MByte*)faceInfo.data(),faceInfo.length());
        _arcFaceManger.AddFaceFeature(faceId,feature);
    }
    query.clear();
}

void MainWindow::selectFaceFeatureById(int id)
{
    memset(m_curDatabaseFeature.feature, 0, m_curDatabaseFeature.featureSize);

    QSqlQuery query;
    query.prepare("select FACEINFO from Manager where USERID = :USERID");
    query.bindValue(":USERID",id);
    query.exec();
    if(query.next()){
        QByteArray faceInfo =query.value(0).toByteArray();
        memcpy(m_curDatabaseFeature.feature, (MByte*)faceInfo.data(), faceInfo.length());
    }

    query.clear();
}

void MainWindow::clearTableView(QTableView *view)
{
    QItemSelectionModel *m = view->selectionModel();
    view->setModel(NULL);
    if(m != NULL)    delete m;
    view->clearSpans();
}

void MainWindow::initTable()
{
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->verticalHeader()->setVisible(false);
    ui->tableView->verticalHeader()->setHighlightSections(false);
    ui->tableView->horizontalHeader()->setHighlightSections(false);
    ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    //ui->tableView->resizeColumnsToContents();
    //ui->tableView->resizeRowsToContents();
}

void MainWindow::DisableOtherBtns(QPushButton *btn)
{
    //默认启用所有按钮
    if(btn == NULL){
        foreach(QPushButton *m_btn,m_btns){
            m_btn->setEnabled(true);
        }
    }else{
        foreach(QPushButton *m_btn,m_btns){
            if(btn == m_btn)            m_btn->setEnabled(true);
            else m_btn->setEnabled(false);
        }
    }
}

void MainWindow::updateLocalFaceFeature()
{
    if(m_flag == RegisterFace){
        ASF_FaceFeature curFaceFeature  =_arcFaceManger.LastFaceFeature();
        QByteArray faceData ((const char*)curFaceFeature.feature,curFaceFeature.featureSize);

        QSqlQuery query;
        query.prepare("update `Manager` set FACEINFO=:FACEINFO where USERID=:USERID");
        query.bindValue(":FACEINFO", faceData, QSql::Binary);
        query.bindValue(":USERID",curUserID);
        if(query.exec()){
            ui->textBrowser->append("人员注册成功!");
        }else{
            ui->textBrowser->append("人员注册失败!");
        }
        query.clear();
        loadUserInfos();
    }else if(m_flag == Identify){

        ASF_FaceFeature curFaceFeature  =_arcFaceManger.LastFaceFeature();

        MFloat m_confidence;
        MRESULT ok = _arcFaceManger.FacePairMatching(m_confidence,curFaceFeature,m_curDatabaseFeature);
        if(ok == MOK){
            ui->textBrowser->append(QString("对比相识度:%1").arg(m_confidence));
        }else {
            ui->textBrowser->append("对比失败!");
        }
    }else if(m_flag == RecognizeLocal){ //1:N
        ASF_FaceFeature curFaceFeature  =_arcFaceManger.LastFaceFeature();
        MFloat m_confidence;
        int id =0;
        MRESULT ok = _arcFaceManger.FaceMultiMathing(m_confidence,curFaceFeature,id);
        if(ok == MOK){
            ui->textBrowser->append(QString("对比相识度:%1,人员ID:%2").arg(m_confidence).arg(id));
        }else{
            ui->textBrowser->append("未能从本地数据库中找到相应的人员信息!");
        }
    }

    on_pushButton_8_clicked();
}

void MainWindow::on_pushButton_6_clicked()
{
    QItemSelectionModel *m = ui->tableView->selectionModel();
    if(m->selectedIndexes().count() < 1){
        QMessageBox box(QMessageBox::Information,tr("提示"),tr("请先选择要删除的人员"));
        box.exec();
        return;
    }

    QMessageBox box(QMessageBox::Warning,tr("警告"),tr("此操作将清除当前用户的人脸信息"),QMessageBox::Yes | QMessageBox::No);
    if(box.exec() == QMessageBox::No){
        return;
    }

    int id = m->selectedIndexes().at(0).data().toInt();
    QSqlQuery query;
    query.prepare("update Manager set FACEINFO = null where  USERID = :USERID");
    query.bindValue(":USERID",id);
    if(query.exec()){
        ui->textBrowser->append(QString("删除人员:%1(工号:%2)的人脸信息成功"
                                        ).arg(m->selectedIndexes().at(2).data().toString()
                                              ).arg(m->selectedIndexes().at(1).data().toString()));
    }else{
        ui->textBrowser->append(QString("删除人员:%1(工号:%2)的人脸信息失败"
                                        ).arg(m->selectedIndexes().at(2).data().toString()
                                              ).arg(m->selectedIndexes().at(1).data().toString()));
    }

    query.clear();
    loadUserInfos();
}

void MainWindow::on_pushButton_4_clicked()
{
    QItemSelectionModel *m = ui->tableView->selectionModel();
    if(m->selectedIndexes().count() < 1){
        QMessageBox box(QMessageBox::Information,tr("提示"),tr("请先选择要注册的人员"));
        box.exec();
        return;
    }

    curUserID= m->selectedIndexes().at(0).data().toInt();
    ui->textBrowser->append(QString("当前注册人员:%1(工号:%2)"
                                    ).arg(m->selectedIndexes().at(2).data().toString()
                                          ).arg(m->selectedIndexes().at(1).data().toString()));
    if(m_FaceType == ImageType){
        ui->textBrowser->append("请选择识别照");
        DisableOtherBtns(ui->pushButton);
    }else if(m_FaceType == VideoType){
        ui->textBrowser->append("请将头部对准摄像头");
        DisableOtherBtns(ui->pushButton_2);
    }

    m_flag = RegisterFace;
}

void MainWindow::on_pushButton_7_clicked()
{
    QMessageBox box(QMessageBox::Warning,tr("警告"),tr("此操作将清空所有用户的人脸信息"),QMessageBox::Yes | QMessageBox::No);

    int ret = box.exec();
    if(ret == QMessageBox::Yes){
        QSqlQuery query;
        if(query.exec("update Manager set FACEINFO = null")){
            ui->textBrowser->append("清空所有人员的人脸信息成功!");
        }else{
            ui->textBrowser->append("清空所有人员的人脸信息失败!");
        }
    }
    loadUserInfos();
}

void MainWindow::ImageOrVideo(MainWindow::StepType m_type)
{
    m_FaceType = m_type;
    if(m_FaceType == ImageType){
        VideoStep(false);
        ImageStep();
    }else if(m_FaceType == VideoType){
        VideoStep();
        ImageStep(false);
    }
}

void MainWindow::on_pushButton_8_clicked()
{
    DisableOtherBtns();
    ui->tableView->clearSelection();
    //ui->textBrowser->append("操作已取消!");
    m_flag = UnknownRequest;
    curUserID = 0;
}

void MainWindow::on_pushButton_3_clicked()
{
    if(m_FaceType == ImageType){
        ui->textBrowser->append("请选择需要对比的照片");
        DisableOtherBtns(ui->pushButton);
    }else if(m_FaceType == VideoType){
        ui->textBrowser->append("请将头部对准摄像头");
        DisableOtherBtns(ui->pushButton_2);
    }
    m_flag = RecognizeLocal;
}

void MainWindow::on_pushButton_5_clicked()
{
    QItemSelectionModel *m = ui->tableView->selectionModel();
    if(m->selectedIndexes().count() < 1){
        QMessageBox box(QMessageBox::Information,tr("提示"),tr("请先选择要对比的人员"));
        box.exec();
        return;
    }

    if(m->selectedIndexes().at(3).data().toString() == "未注册"){
        QMessageBox box(QMessageBox::Information,tr("提示"),tr("当前人员未注册,请先注册!"));
        box.exec();
        return;
    }

    selectFaceFeatureById(m->selectedIndexes().at(0).data().toInt());

    if(m_FaceType == ImageType){
        ui->textBrowser->append("请选择需要对比的照片");
        DisableOtherBtns(ui->pushButton);
    }else if(m_FaceType == VideoType){
        ui->textBrowser->append("请将头部对准摄像头");
        DisableOtherBtns(ui->pushButton_2);
    }
    m_flag = Identify;
}

void MainWindow::rcvRgbFram(cv::Mat frame)
{
    if(m_FaceType == VideoType){
        QImage img = cvMat2QImage(frame);
        if(!img.isNull()){
            ui->label->setPixmap(QPixmap::fromImage(img).scaled(ui->label->width(),ui->label->height(),Qt::KeepAspectRatio));
            update();
        }

    }
}
