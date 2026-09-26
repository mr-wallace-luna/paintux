#include "ExifLoader.h"
#include <QFile>
#include <QTransform>

namespace {

constexpr int kExifTagOrientation = 0x0112;
constexpr int kExifIfdEntrySize   = 12;
constexpr int kExifHeaderLen      = 14;
constexpr int kExifMaxOrientation = 8;

constexpr uchar kMarkerSoi      = 0xD8;
constexpr uchar kMarkerExifApp1 = 0xE1;
constexpr uchar kMarkerRstStart = 0xD0;
constexpr uchar kMarkerRstEnd   = 0xD9;

struct TiffReader {
    const uchar *tiff = nullptr;
    int tiffLen = 0;
    bool little = true;

    int rd16(int off) const {
        if (off < 0 || off + 1 >= tiffLen) return 0;
        return little ? (tiff[off] | (tiff[off + 1] << 8))
                      : ((tiff[off] << 8) | tiff[off + 1]);
    }
    int rd32(int off) const {
        if (off < 0 || off + 3 >= tiffLen) return 0;
        return little
            ? (tiff[off] | (tiff[off+1] << 8) | (tiff[off+2] << 16) | (tiff[off+3] << 24))
            : ((tiff[off] << 24) | (tiff[off+1] << 16) | (tiff[off+2] << 8) | tiff[off+3]);
    }
};

inline bool isJpegStandaloneMarker(uchar m) {
    return m == kMarkerSoi || m == 0x01 ||
           (m >= kMarkerRstStart && m <= kMarkerRstEnd);
}

inline bool isExifApp1Segment(const uchar *seg, int segData) {
    return segData >= kExifHeaderLen &&
           seg[0] == 'E' && seg[1] == 'x' && seg[2] == 'i' && seg[3] == 'f' &&
           seg[4] == 0 && seg[5] == 0;
}

inline bool isLittleEndianTIFF(const uchar *tiff) {
    return tiff[0] == 'I' && tiff[1] == 'I';
}
inline bool isBigEndianTIFF(const uchar *tiff) {
    return tiff[0] == 'M' && tiff[1] == 'M';
}

int findOrientationInIFD0(const TiffReader &reader) {
    const int ifd0 = reader.rd32(4);
    if (ifd0 <= 0 || ifd0 + 2 > reader.tiffLen) return 1;

    const int entries = reader.rd16(ifd0);
    for (int i = 0; i < entries; ++i) {
        const int entry = ifd0 + 2 + i * kExifIfdEntrySize;
        if (entry + kExifIfdEntrySize > reader.tiffLen) break;
        if (reader.rd16(entry) != kExifTagOrientation) continue;
        const int val = reader.rd16(entry + 8);
        return (val >= 1 && val <= kExifMaxOrientation) ? val : 1;
    }
    return 1;
}

int parseExifSegment(const uchar *seg, int segData) {
    if (!isExifApp1Segment(seg, segData)) return 1;

    const uchar *tiff = seg + 6;
    const int tiffLen = segData - 6;
    if (tiffLen < 8) return 1;

    TiffReader reader;
    if (isLittleEndianTIFF(tiff)) {
        reader.tiff = tiff; reader.tiffLen = tiffLen; reader.little = true;
    } else if (isBigEndianTIFF(tiff)) {
        reader.tiff = tiff; reader.tiffLen = tiffLen; reader.little = false;
    } else {
        return 1;
    }
    return findOrientationInIFD0(reader);
}

}

namespace ExifLoader {

int readOrientation(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return 1;
    const QByteArray data = file.read(65536);
    file.close();

    const uchar *d = reinterpret_cast<const uchar *>(data.constData());
    const int len = data.size();
    if (len < 4 || d[0] != 0xFF || d[1] != kMarkerSoi) return 1;

    int pos = 2;
    while (pos + 4 < len) {
        if (d[pos] != 0xFF) { ++pos; continue; }

        const uchar marker = d[pos + 1];
        if (isJpegStandaloneMarker(marker)) { pos += 2; continue; }

        const int segLen = (d[pos + 2] << 8) | d[pos + 3];
        if (segLen < 2) break;

        if (marker == kMarkerExifApp1 && pos + 2 + segLen <= len) {
            const int result = parseExifSegment(d + pos + 4, segLen - 2);
            if (result > 1) return result;
        }
        pos += 2 + segLen;
    }
    return 1;
}

QImage applyOrientation(const QImage &img, int orientation) {
    switch (orientation) {
        case 2: return img.mirrored(true, false);
        case 3: return img.transformed(QTransform().rotate(180), Qt::FastTransformation);
        case 4: return img.mirrored(false, true);
        case 5: return img.transformed(QTransform().rotate(90),  Qt::FastTransformation).mirrored(true, false);
        case 6: return img.transformed(QTransform().rotate(90),  Qt::FastTransformation);
        case 7: return img.transformed(QTransform().rotate(270), Qt::FastTransformation).mirrored(true, false);
        case 8: return img.transformed(QTransform().rotate(270), Qt::FastTransformation);
        default: return img;
    }
}

QImage loadRespectingExif(const QString &filePath) {
    QImage img(filePath);
    if (img.isNull()) return img;
    int orient = readOrientation(filePath);
    if (orient > 1) img = applyOrientation(img, orient);
    return img;
}

}