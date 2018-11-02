#include "mthread.h"
#include <QCoreApplication>
#include <QDebug>
#include <QFile>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio/videoio.hpp>

void mthread::run() {
    cv::Mat img2 = cv::imread("C:\\Users\\muryliang\\Pictures\\test images\\new2.png", -1);

    float scale = 0.25;
    cv::Size dsize = cv::Size(img2.cols*scale, img2.rows*scale);
    cv::Mat img = cv::Mat(dsize,CV_32S);
    cv::resize(img2, img, dsize);


       cv::imshow("123", img);    //“123”是显示框的名字
       cv::waitKey(100);     //通过整型变量a获取waitKey函数的返回值

       /*
       waitKey(x)  表示等x毫秒，在这期间如果有按键按下，则返回按键的ascii码，等待结束则会返回-1.
       如果x=0，那么无限等待下去，直到有按键按下
       ！！！另外，在imshow之后如果没有waitKey语句则不会正常显示图像。即imshow不会生效！！！
       */
//       destroyAllWindows();   //销毁窗口

       QVector<QByteArray> qb;
       for (int i = 0; i < 10; ++i) {
           cv::Rect rect(0, img.rows / 10 * i, img.cols, img.rows / 10);   //创建一个Rect框，属于cv中的类，四个参数代表x,y,width,height
           cv::Mat image_cut = cv::Mat(img, rect);      //从img中按照rect进行切割，此时修改image_cut时image中对应部分也会修改，因此需要copy
   //        cv::imshow("456",image_cut);
   //        cv::waitKey();
           if (image_cut.channels() == 4) {
               cv::Mat planes[4];
               cv::split(image_cut, planes);
               image_cut = planes[1];  // get green channel
           }

           qDebug() << "width and height and size " << image_cut.cols << image_cut.rows << image_cut.total() << image_cut.channels();
//           std::string strpath = "C:\\Users\\muryliang\\Pictures\\test images\\tmp\\cut";
           std::string strpath = "C:/Users/muryliang/Pictures/test images/tmp/cut";
           QFile f(QString::fromStdString(strpath) + QString::number(i) + ".raw");
           f.open(QIODevice::WriteOnly | QIODevice::Truncate);
           f.write((const char*)image_cut.data, image_cut.total());
           f.close();
           cv::imwrite(strpath + std::to_string(i) + ".bmp", image_cut);
           qb.append(QByteArray((const char*)image_cut.data, image_cut.total()));

           QImage tmpimg((const uchar*)image_cut.data, 640 * scale, 48 * scale, QImage::Format_Indexed8);

           emit sendImg(tmpimg);
           QCoreApplication::processEvents();

//           QLabel *label = new QLabel;
//           label->setPixmap(QPixmap::fromImage(tmpimg));
//           ui->verticalLayout->addWidget(label);


       }

       QByteArray barr;
       for (int i = 0; i < 10; ++i) {
           barr.append(QByteArray((const char*)qb[i], 640 * scale * 48 * scale));
       }
       QImage mergedImg((const uchar*)barr.data(), 640 * scale, 480 * scale, QImage::Format_Indexed8);
       emit sendImg(mergedImg);
//       QLabel *label = new QLabel;
//       label->setPixmap(QPixmap::fromImage(mergedImg));
//       ui->verticalLayout->addWidget(label);
}
