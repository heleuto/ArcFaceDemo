#ifndef IMAGECONVERTER_H
#define IMAGECONVERTER_H

#include <QImage>
#include <opencv2\opencv.hpp>

QImage* IplImage2QImage(IplImage *iplImg);

#endif // IMAGECONVERTER_H
