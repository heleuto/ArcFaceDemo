#pragma execution_character_set("utf-8")
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDebug>
#include "ArcFace/ImageConverter.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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

    IplImage* image = cvLoadImage(fileName.toStdString().c_str());

    //图片加载失败
    if(!image){
        cvReleaseImage(&image);
        return;
    }

    ui->label->clear();
    _arcFaceManger.StaticImageMultiFaceOp(image);
    ui->label->setPixmap(QPixmap::fromImage(*IplImage2QImage(image)).scaled(ui->label->size(),Qt::KeepAspectRatio)); //内存泄漏?

}

//线程打开摄像头
void MainWindow::on_pushButton_2_clicked()
{
    if(ui->pushButton->text() == "打开摄像头"){
        ui->pushButton->setEnabled(false);
        ui->pushButton->setText(tr("关闭摄像头"));
    }else{
        ui->pushButton->setText(tr("打开摄像头"));
    }


}
