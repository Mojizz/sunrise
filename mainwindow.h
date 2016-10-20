#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QGraphicsScene>
#include <QByteArray>
#include <QImage>

namespace Ui {
class MainWindow;
}

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_btnCrypt_clicked();

    void on_btnSrcImage_clicked();

    void on_btnSrcFile_clicked();

    void on_btnDecode_clicked();

private:
    Ui::MainWindow *ui;
    QGraphicsScene m_srcScene;
    QGraphicsScene m_dstScene;
    QPixmap m_srcPixmap;
    QImage m_srcImage;
    QPixmap m_dstPixmap;

    QString m_fileImageSrc;

    QString m_srcFileToEncode;

    QImage encode(QImage img, QByteArray bytes, int padding, int spacing);
    QByteArray decode(QImage img, int padding, int spacing);
    void changeSourceImage(QString srcImageFileName);
};

#endif // MAINWINDOW_H
