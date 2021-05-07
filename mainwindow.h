#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ArcFace/arcfacemanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
class QTableView;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_6_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_7_clicked();

private:
    Ui::MainWindow *ui;

    ArcFaceManager _arcFaceManger;

    void ImageStep(bool step = true);
    bool m_imageStep = true;

    void VideoStep(bool step= true);

    bool initDatabase(QString dataBaseName = "Database.db");

    bool loadUserInfos();

    void loadFeaturesFromDataBase();

    void clearTableView(QTableView *view);

    void initTable();
};
#endif // MAINWINDOW_H
