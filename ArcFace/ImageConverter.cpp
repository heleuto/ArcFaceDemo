#include "ImageConverter.h"

QImage *IplImage2QImage(IplImage *iplImg)
{
    if(!iplImg) return NULL;

    cv::Mat image = cv::Mat(iplImg);
    QImage *img =new QImage(image.data,image.cols,image.rows,image.step,QImage::Format_RGB888);
    *img = img->rgbSwapped();
    return img;
}
