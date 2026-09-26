#ifndef EXIFLOADER_H
#define EXIFLOADER_H

#include <QImage>
#include <QString>

namespace ExifLoader {

int readOrientation(const QString &filePath);
QImage applyOrientation(const QImage &img, int orientation);
QImage loadRespectingExif(const QString &filePath);

}

#endif