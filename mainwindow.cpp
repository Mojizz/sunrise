#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFile>
#include <QMessageBox>
#include <QPixmap>
#include <QFileDialog>
#include <QFileInfo>


/**
 * @brief MainWindow::MainWindow
 * @param parent
 */
MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWindow),
    m_srcScene(this),
    m_dstScene(this)
{
    ui->setupUi(this);
    this->m_fileImageSrc = "";
    this->ui->imageSource->setScene(& this->m_srcScene);
    this->ui->imageDestination->setScene(& this->m_dstScene);
}

MainWindow::~MainWindow()
{
}

/**
 * @brief MainWindow::on_btnCrypt_clicked
 */
void MainWindow::on_btnCrypt_clicked()
{
    if(this->m_fileImageSrc.isEmpty())
    {
        QMessageBox::information(this,"Error", "Please select a source image.");
        return;
    }

    if(this->m_srcFileToEncode.isEmpty())
    {
        QMessageBox::information(this,"Error", "Please select a file to hide.");
        return;
    }

    QFileInfo fileInfo(this->m_srcFileToEncode);
    if (fileInfo.size() > (this->m_srcImage.width() * this->m_srcImage.height() - sizeof(int)))
    {
        QMessageBox::information(this,"Error", "Not enough pixels in the image to hide this file.");
        return;
    }

    int padding = ui->spinPadding->value();
    int spacing = ui->spinSpacing->value();


    // Loads the file to hide
    QFile file(this->m_srcFileToEncode);
    if (!file.open(QIODevice::ReadOnly ))
    {
        QMessageBox::information(this,"Error", "File not readable");
        return;
    }


    QByteArray data;
    data = file.readAll();

    QImage img = encode(this->m_srcImage, data, padding, spacing);
    this->m_dstPixmap = QPixmap::fromImage(img);


    this->m_dstScene.clear();
    this->m_dstScene.addPixmap(this->m_dstPixmap);


    QString fileToWrite = QFileDialog::getSaveFileName(this, tr("Save File"),
    QDir::currentPath(), tr("Images (*.png *.xpm *.bmp);;All files (*.*)"));

    // Checks if a file has been selected
    if(!fileToWrite.isEmpty())
    {
        QDir::setCurrent(QFileInfo(fileToWrite).absolutePath());

        if (!this->m_dstPixmap.save(fileToWrite))
        {
            QMessageBox::information(this,"Error", "Error while saving the modified image.");
        }
    }
}


/**
 * @brief MainWindow::encode: Encode a list of bytes into an image
 * @param img: source image to use to encode data
 * @param bytes: bytes to hide in the image
 * @param padding: pixel number to start the encoding
 * @param spacing: number of pixels between each encoded bytes
 * @return the modified image with the encoded byte array in it.
 */
QImage MainWindow::encode(QImage img, QByteArray bytes, int padding, int spacing)
{
    int w = img.width();
    int h = img.height();
    int pos = padding;

    int len = bytes.size();
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);

    stream << len;

    bytes = byteArray + bytes;

    for (int i=0; i< bytes.size(); i++)
    {
        int x = pos % w;
        int y = pos / w;

        // Encodes the #i byte
        unsigned char charToEncode = bytes.at(i);

        // Extracts the bytes to set into red (3 bits), green (3 bits) and blue (2 bits) for this pixel
        unsigned char r = charToEncode & 0b00000111;
        unsigned char g = (charToEncode & 0b00111000) >> 3;
        unsigned char b = (charToEncode & 0b11000000) >> 6;

        // Gets the pixel to modify and alterates it
        QColor pixel(img.pixel(x, y));
        int red = (pixel.red() & 0b11111000U) | r;
        int green = (pixel.green() & 0b11111000U) | g;
        int blue = (pixel.blue() & 0b11111100U) | b;

        // Sets the pixel into the image
        img.setPixelColor(x, y, QColor(red, green, blue));

        // Moves to next pixel according to given spacing
        pos += spacing;

        if (pos > w * h)
        {
            pos = (--padding);
        }
    }

    return img;
}


/**
 * @brief MainWindow::decode: Extracts the bytes hidden in the given image
 * @param img: image containing encoded bytes in it
 * @param padding: first pixel with encoded data in it
 * @param spacing: spacing between each encoded pixels
 * @return a byte array containing the decoded data.
 */
QByteArray MainWindow::decode(QImage img, int padding, int spacing)
{
    int messageSize = 0;
    QByteArray bytes;
    int pos = padding;
    int w = img.width();
    int h = img.height();

    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    QDataStream headerStream(&byteArray, QIODevice::ReadOnly);

    // Reads the message length
    for (unsigned int i = 0; i < sizeof(int); i++)
    {
        int x = pos % w;
        int y = pos / w;

        QColor pixel(img.pixel(x, y)); // Encoded pixel

        unsigned int red   = (unsigned int)(pixel.red()   & 0b00000111);
        unsigned int green = (unsigned int)(pixel.green() & 0b00000111) << 3;
        unsigned int blue  = (unsigned int)(pixel.blue()  & 0b00000011) << 6;

        unsigned char charDecoded = (unsigned char)(red | green | blue);

        stream << charDecoded;

        pos += spacing;
    }

    headerStream >> messageSize;

    // Checks the message size validity
    // It can't exceed the number of pixels
    if (messageSize > w * h)
    {
        QMessageBox::information(this,"Error", "Invalid message size");
    }

    // Reads the message
    for (int i=0; i < messageSize; i++)
    {
        int x = pos % w;
        int y = pos / w;

        // Decodes the #i byte
        QColor pixel(img.pixel(x, y)); // Encoded pixel

        int red = (pixel.red() & 0b00000111);
        int green = (pixel.green() & 0b00000111) << 3;
        int blue = (pixel.blue() & 0b00000011) << 6;

        unsigned char charDecoded = (unsigned char)(red | green | blue);

        bytes.append(charDecoded);

        pos += spacing;
        if (pos > w * h)
        {
            pos = (--padding);
        }
    }

    return bytes;
}


/**
 * @brief MainWindow::changeSourceImage: Loads an image from a file as source image to hide the data
 * @param srcImageFileName: Path to the image to load
 */
void MainWindow::changeSourceImage(QString srcImageFileName)
{
    this->m_srcImage.load(srcImageFileName);
    this->m_fileImageSrc = srcImageFileName;

    this->m_srcPixmap = QPixmap::fromImage(this->m_srcImage);

    this->m_srcScene.clear();
    this->m_srcScene.addPixmap(this->m_srcPixmap);

    QFileInfo fileInfo(srcImageFileName);
    QString strProp = QString("%1 - Size: %2 pixels").arg(fileInfo.fileName(), QString::number(this->m_srcImage.width() * this->m_srcImage.height()));

    ui->lblImage->setText(strProp);
}



/**
 * @brief MainWindow::on_btnSrcImage_clicked: Prompts a dialogbox to select the image source to encode the data
 *
 */
void MainWindow::on_btnSrcImage_clicked()
{
    QString srcImage = QFileDialog::getOpenFileName(this, tr("Open File"),
    QDir::currentPath(), tr("Images (*.jpg *.jpeg *.png *.xpm *.bmp);;All files (*.*)"));

    if(!srcImage.isEmpty())
    {
        QFileInfo fileInfo(srcImage);
        QDir::setCurrent(fileInfo.absolutePath());

        if (!fileInfo.exists())
        {
            QMessageBox::information(this,"Error", srcImage + " does not exist.");
        }
        else
        {
            changeSourceImage(srcImage);
        }
    }
}


/**
 * @brief MainWindow::on_btnSrcFile_clicked
 */
void MainWindow::on_btnSrcFile_clicked()
{
    this->m_srcFileToEncode = QFileDialog::getOpenFileName(this, tr("Open File"),
    QDir::currentPath(), tr("All files (*.*)"));

    if(!this->m_srcFileToEncode.isEmpty())
    {
        QFileInfo fileInfo(this->m_srcFileToEncode);

        QDir::setCurrent(fileInfo.absolutePath());

        if (!fileInfo.exists())
        {
            QMessageBox::information(this,"Error", this->m_srcFileToEncode + " does not exist.");
        }
        else
        {
            QString strFileProp = fileInfo.fileName();
            strFileProp += " - File size: ";
            strFileProp += QString::number(fileInfo.size());

            ui->lblFile->setText(strFileProp);
        }
    }
}


/**
 * @brief MainWindow::on_btnDecode_clicked
 */
void MainWindow::on_btnDecode_clicked()
{
    QString encodedImage = QFileDialog::getOpenFileName(this, tr("Open File"),
    QDir::currentPath(), tr("Images (*.png *.xpm *.bmp);;All files (*.*)"));

    // Checks if a file has been selected
    if(!encodedImage.isEmpty())
    {
        QFileInfo fileInfo(encodedImage);
        QDir::setCurrent(fileInfo.absolutePath());

        // Tries to recover data from the file

        if (!fileInfo.exists())
        {
            QMessageBox::information(this,"Error", encodedImage + " does not exist.");
        }
        else
        {
            // If all data are correct the extract the message
            QByteArray data;
            QImage image(encodedImage);
            int padding = ui->spinPadding->value();
            int spacing = ui->spinSpacing->value();

            data = decode(image, padding, spacing);

            QString fileToWrite = QFileDialog::getSaveFileName(this, tr("Save File"),
            QDir::currentPath(), tr("All files (*.*)"));

            // Checks if a file has been selected
            if(!fileToWrite.isEmpty())
            {
                QDir::setCurrent(QFileInfo(fileToWrite).absolutePath());
                QFile file(fileToWrite);

                if (! file.open(QIODevice::WriteOnly))
                {
                    QMessageBox::information(this,"Error", "Unable to open file " + fileToWrite);
                }
                else
                {
                    if (file.write(data) < 0)
                    {
                        QMessageBox::information(this,"Error", "Error while writing to file.");
                    }
                    else
                    {
                        QMessageBox::information(this, "Success", "Message successfully retrieved");
                    }
                    file.close();
                }
            }
        }
    }
}
