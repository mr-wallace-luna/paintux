#include "ExifLoader.h"

#include <QFile>
#include <QSaveFile>
#include <QBuffer>
#include <QDataStream>
#include <QTransform>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QElapsedTimer>

#include "core/LayerStack.h"
#include "core/CustomBrushes.h"
#include "core/ShapeObjects.h"
#include "tools/SelectTools.h"
#include "tools/PixelAnimationTools.h"
#include "filter/ImageFilters.h"
#include "paintarea.h"


#define PTX_LOG(msg) do { \
    qDebug().noquote() << QStringLiteral("[PTX]") << msg; \
} while(0)



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

} // namespace anónimo (EXIF)

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


namespace {

using namespace ProjectIO;

void writeString(QDataStream &out, const QString &s) {
    out << s;
}

QString readString(QDataStream &in) {
    QString s;
    in >> s;
    return s;
}

void writeByteArray(QDataStream &out, const QByteArray &ba) {
    out << ba;
}

QByteArray readByteArray(QDataStream &in) {
    QByteArray ba;
    in >> ba;
    return ba;
}

void writeImage(QDataStream &out, const QImage &img) {
    QByteArray png;
    if (!img.isNull()) {
        QBuffer buf(&png);
        buf.open(QIODevice::WriteOnly);
        img.save(&buf, "PNG");
        buf.close();
    }
    out << png;
}

QImage readImage(QDataStream &in) {
    QByteArray png;
    in >> png;
    if (png.isEmpty()) return QImage();
    QImage img;
    if (!img.loadFromData(png, "PNG")) {
        PTX_LOG(QStringLiteral("      WARN: PNG corrupto, size=%1").arg(png.size()));
        return QImage();
    }
    return img;
}

void writeColor(QDataStream &out, const QColor &c) {
    out << c;
}

QColor readColor(QDataStream &in) {
    QColor c;
    in >> c;
    return c;
}


void writeFilterParams(QDataStream &out, const FilterParams &fp) {
    out << fp.brightness << fp.contrast << fp.saturation << fp.exposure;
    out << fp.shadows << fp.highlights << fp.temperature << fp.vignette;
    out << fp.shadowColor << fp.highlightColor;
    out << fp.invertColors << fp.grayscale << fp.sepia;
    out << fp.blur << qint32(fp.blurRadius);
    out << fp.sharpen << qint32(fp.sharpenStrength);
    out << fp.pixelate << qint32(fp.pixelateSize);
    out << fp.halftone << qint32(fp.halftoneCell);
    out << fp.rawTemp << fp.rawTint << fp.rawVibrance << fp.rawClarity;
    out << fp.rawBlacks << fp.rawWhites << qint32(fp.rawGamma);
    out << fp.colorize << fp.colorizeStrength;
    out << fp.colorizeHSL;

    for (int i = 0; i < 7; ++i) {
        out << fp.hsl[i].hue << fp.hsl[i].saturation << fp.hsl[i].lightness;
    }

    for (int i = 0; i < 4; ++i) {
        out << qint32(fp.curves[i].size());
        for (const QPointF &p : fp.curves[i]) {
            out << p.x() << p.y();
        }
    }
}

FilterParams readFilterParams(QDataStream &in) {
    FilterParams fp;
    in >> fp.brightness >> fp.contrast >> fp.saturation >> fp.exposure;
    in >> fp.shadows >> fp.highlights >> fp.temperature >> fp.vignette;
    in >> fp.shadowColor >> fp.highlightColor;
    in >> fp.invertColors >> fp.grayscale >> fp.sepia;
    in >> fp.blur;
    qint32 br = 0; in >> br; fp.blurRadius = br;
    in >> fp.sharpen;
    qint32 ss = 0; in >> ss; fp.sharpenStrength = ss;
    in >> fp.pixelate;
    qint32 ps = 0; in >> ps; fp.pixelateSize = ps;
    in >> fp.halftone;
    qint32 hc = 0; in >> hc; fp.halftoneCell = hc;
    in >> fp.rawTemp >> fp.rawTint >> fp.rawVibrance >> fp.rawClarity;
    in >> fp.rawBlacks >> fp.rawWhites;
    qint32 rg = 100; in >> rg; fp.rawGamma = rg;
    in >> fp.colorize >> fp.colorizeStrength;
    in >> fp.colorizeHSL;

    for (int i = 0; i < 7; ++i) {
        in >> fp.hsl[i].hue >> fp.hsl[i].saturation >> fp.hsl[i].lightness;
    }

    for (int i = 0; i < 4; ++i) {
        qint32 n = 0; in >> n;
        fp.curves[i].clear();
        for (qint32 j = 0; j < n; ++j) {
            qreal x = 0, y = 0;
            in >> x >> y;
            fp.curves[i].append(QPointF(x, y));
        }
        if (fp.curves[i].isEmpty()) {
            fp.curves[i] = QVector<QPointF>{ QPointF(0, 0), QPointF(255, 255) };
        }
    }
    return fp;
}

void writeShapeElement(QDataStream &out, const ShapeElement &se) {
    out << qint32((int)se.shape);
    out << se.offsetX << se.offsetY << se.scale << se.rotation;
    out << qint32(se.opacity);
    writeImage(out, se.customImage);
}

ShapeElement readShapeElement(QDataStream &in) {
    ShapeElement se;
    qint32 shapeInt = 0; in >> shapeInt;
    se.shape = (ShapeType)shapeInt;
    in >> se.offsetX >> se.offsetY >> se.scale >> se.rotation;
    qint32 op = 100; in >> op; se.opacity = op;
    se.customImage = readImage(in);
    return se;
}

void writeBrushSettings(QDataStream &out, const BrushSettings &bs) {
    out << qint32(bs.size);
    out << qint32((int)bs.shape);
    out << qint32((int)bs.dragMode);
    out << qint32((int)bs.rotationMode);
    out << qint32(bs.opacity) << qint32(bs.scatter);
    out << bs.angle;
    out << qint32(bs.density) << qint32(bs.flow);
    out << bs.isAirbrush;
    out << qint32(bs.sizeJitter) << qint32(bs.angleJitter) << qint32(bs.opacityJitter);
    out << bs.aspectRatio;
    out << bs.wetMix << qint32(bs.wetAmount);
    out << bs.granulation << bs.mixSecondColor;

    out << qint32(bs.shapeElements.size());
    for (const ShapeElement &se : bs.shapeElements) {
        writeShapeElement(out, se);
    }

    writeImage(out, bs.customStampImage);
}

BrushSettings readBrushSettings(QDataStream &in) {
    BrushSettings bs;
    qint32 size = 20; in >> size; bs.size = size;
    qint32 shapeInt = 0; in >> shapeInt; bs.shape = (ShapeType)shapeInt;
    qint32 dragInt = 0; in >> dragInt; bs.dragMode = (DragMode)dragInt;
    qint32 rotInt = 0; in >> rotInt; bs.rotationMode = (RotationMode)rotInt;
    qint32 op = 100; in >> op; bs.opacity = op;
    qint32 sc = 0; in >> sc; bs.scatter = sc;
    in >> bs.angle;
    qint32 den = 1; in >> den; bs.density = den;
    qint32 fl = 100; in >> fl; bs.flow = fl;
    in >> bs.isAirbrush;
    qint32 sj = 0, aj = 0, oj = 0;
    in >> sj >> aj >> oj;
    bs.sizeJitter = sj; bs.angleJitter = aj; bs.opacityJitter = oj;
    in >> bs.aspectRatio;
    in >> bs.wetMix;
    qint32 wa = 50; in >> wa; bs.wetAmount = wa;
    in >> bs.granulation >> bs.mixSecondColor;

    qint32 nElems = 0; in >> nElems;
    bs.shapeElements.clear();
    for (qint32 i = 0; i < nElems; ++i) {
        bs.shapeElements.append(readShapeElement(in));
    }

    bs.customStampImage = readImage(in);
    return bs;
}


void writePaintObject(QDataStream &out, const PaintObject &o) {
    out << qint32((int)o.type);
    out << o.bounds;
    out << o.rotation << o.scaleX << o.scaleY;
    out << qint32(o.layerIndex);
    out << o.selected;

    out << qint32((int)o.shapeTool);
    out << o.fillColor << o.strokeColor;
    out << qint32(o.strokeWidth);
    out << o.startPoint << o.endPoint;
    out << o.hollow << o.isFrame;
    out << o.frameImageScale << o.frameImageOffset;
    writeImage(out, o.frameImage);

    writeString(out, o.textContent);
    out << o.textFont << o.textColor;

    out << qint32(o.selectionBufferId);
}

PaintObject readPaintObject(QDataStream &in) {
    PaintObject o;
    qint32 typeInt = 0; in >> typeInt; o.type = (ObjectType)typeInt;
    in >> o.bounds;
    in >> o.rotation >> o.scaleX >> o.scaleY;
    qint32 layer = -1; in >> layer; o.layerIndex = layer;
    in >> o.selected;

    qint32 shapeInt = 0; in >> shapeInt; o.shapeTool = (ToolType)shapeInt;
    in >> o.fillColor >> o.strokeColor;
    qint32 sw = 1; in >> sw; o.strokeWidth = sw;
    in >> o.startPoint >> o.endPoint;
    in >> o.hollow >> o.isFrame;
    in >> o.frameImageScale >> o.frameImageOffset;
    o.frameImage = readImage(in);

    o.textContent = readString(in);
    in >> o.textFont >> o.textColor;

    qint32 bufId = -1; in >> bufId; o.selectionBufferId = bufId;
    return o;
}



bool writeHeader(QDataStream &out) {
    out << quint32(MAGIC_HEADER);
    out << quint16(FORMAT_VERSION);
    out << quint16(0);
    out << quint32(0);
    return out.status() == QDataStream::Ok;
}

bool readHeader(QDataStream &in, quint16 &version) {
    quint32 magic = 0;
    in >> magic;
    if (magic != MAGIC_HEADER) return false;
    quint16 flags = 0;
    quint32 reserved32 = 0;
    in >> version;
    in >> flags;
    in >> reserved32;
    Q_UNUSED(flags); Q_UNUSED(reserved32);
    return in.status() == QDataStream::Ok;
}

bool writeFooter(QDataStream &out) {
    out << quint32(MAGIC_FOOTER);
    return out.status() == QDataStream::Ok;
}

bool readFooter(QDataStream &in) {
    quint32 magic = 0;
    in >> magic;
    return magic == MAGIC_FOOTER;
}

bool writeSection(QDataStream &out, quint32 id, const QByteArray &payload,
                  bool allowCompress, int level) {
    QByteArray data = payload;
    quint32 flags = 0;

    if (allowCompress && data.size() > (int)COMPRESS_THRESHOLD) {
        QByteArray compressed = qCompress(data, level);
        if (compressed.size() < data.size()) {
            data = compressed;
            flags |= FlagCompressed;
        }
    }

    qDebug().noquote() << QStringLiteral("[SAVE] Section id=0x%1 payload=%2 compressed=%3 flags=%4")
        .arg(QString::number(id, 16))
        .arg(payload.size())
        .arg(data.size())
        .arg(flags);

    out << quint32(id);
    out << quint32(flags);
    out << quint64(data.size());
    if (!data.isEmpty()) {
        out.writeRawData(data.constData(), data.size());
    }
    return out.status() == QDataStream::Ok;
}

bool readSectionHeader(QDataStream &in, quint32 &id, quint32 &flags, quint64 &size) {
    in >> id;
    in >> flags;
    in >> size;
    return in.status() == QDataStream::Ok;
}

QByteArray readSectionPayload(QDataStream &in, quint32 flags, quint64 size) {
    if (size == 0) return QByteArray();
    if (size > (quint64)512 * 1024 * 1024) {
        PTX_LOG(QStringLiteral("      WARN: sección demasiado grande (%1 bytes)").arg(size));
        return QByteArray();
    }
    QByteArray data;
    data.resize((int)size);
    int read = in.readRawData(data.data(), (int)size);
    if (read != (int)size) {
        PTX_LOG(QStringLiteral("      WARN: readRawData leyó %1 de %2").arg(read).arg(size));
        return QByteArray();
    }
    if (flags & FlagCompressed) {
        QByteArray uncompressed = qUncompress(data);
        if (uncompressed.isEmpty()) {
            PTX_LOG(QStringLiteral("      WARN: qUncompress falló, retornando raw"));
            return data;
        }
        data = uncompressed;
    }
    return data;
}


QByteArray serializeCanvas(const LayerStack &stack) {
    QByteArray ba;
    QDataStream s(&ba, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_6_0);
    s.setByteOrder(QDataStream::LittleEndian);
    s << qint32(stack.width()) << qint32(stack.height());
    s << qint32(stack.currentIndex());
    return ba;
}

QByteArray serializeLayers(const LayerStack &stack) {
    QByteArray ba;
    QDataStream s(&ba, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_6_0);
    s.setByteOrder(QDataStream::LittleEndian);

    const QList<Layer> &layers = stack.layers();

    qDebug().noquote() << QStringLiteral("====== serializeLayers START ======");
    qDebug().noquote() << QStringLiteral("  Capas a escribir: %1").arg(layers.size());

    s << qint32(layers.size());

    for (int i = 0; i < layers.size(); ++i) {
        const Layer &L = layers[i];
        qDebug().noquote() << QStringLiteral("  Capa %1: name='%2' vis=%3 op=%4 blend=%5 locked=%6 img=%7x%8")
            .arg(i)
            .arg(L.name)
            .arg(L.visible)
            .arg(L.opacity)
            .arg(L.blendMode)
            .arg(L.locked)
            .arg(L.image.width())
            .arg(L.image.height());

        writeString(s, L.name);
        s << L.visible << L.opacity << qint32(L.blendMode) << L.locked;
        writeImage(s, L.image);
    }

    qDebug().noquote() << QStringLiteral("  Bytes escritos: %1").arg(ba.size());
    qDebug().noquote() << QStringLiteral("====== serializeLayers DONE ======");
    return ba;
}

QByteArray serializeMasksBN(const LayerStack &stack) {
    QByteArray ba;
    QDataStream s(&ba, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_6_0);
    s.setByteOrder(QDataStream::LittleEndian);

    QList<int> indices;
    for (int i = 0; i < stack.count(); ++i) {
        if (stack.hasMask(i)) indices.append(i);
    }
    s << qint32(indices.size());
    for (int idx : indices) {
        s << qint32(idx);
        s << stack.isMaskEnabled(idx);
        writeImage(s, stack.mask(idx));
    }
    return ba;
}

QByteArray serializeMasksColor(const LayerStack &stack) {
    QByteArray ba;
    QDataStream s(&ba, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_6_0);
    s.setByteOrder(QDataStream::LittleEndian);

    QList<int> indices;
    for (int i = 0; i < stack.count(); ++i) {
        if (stack.hasColorMask(i)) indices.append(i);
    }
    s << qint32(indices.size());
    for (int idx : indices) {
        s << qint32(idx);
        s << stack.isColorMaskEnabled(idx);
        writeFilterParams(s, stack.colorMaskParams(idx));
    }
    return ba;
}

QByteArray serializeFrames(const PixelAnimationManager &anim) {
    QByteArray ba;
    QDataStream s(&ba, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_6_0);
    s.setByteOrder(QDataStream::LittleEndian);

    s << anim.getIsActive();
    s << qint32(anim.getResolution());
    s << qint32(anim.getCurrentFrameIndex());

    const QList<QImage> &frames = anim.getFrames();
    s << qint32(frames.size());
    for (const QImage &f : frames) {
        writeImage(s, f);
    }
    return ba;
}

QByteArray serializeObjects(const SelectionManager &selMgr) {
    QByteArray ba;
    QDataStream s(&ba, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_6_0);
    s.setByteOrder(QDataStream::LittleEndian);

    const QList<PaintObject> &objs = selMgr.allObjects();
    s << qint32(objs.size());
    for (const PaintObject &o : objs) {
        writePaintObject(s, o);
    }
    return ba;
}

QByteArray serializeObjBuffers(const SelectionManager &selMgr) {
    QByteArray ba;
    QDataStream s(&ba, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_6_0);
    s.setByteOrder(QDataStream::LittleEndian);

    QList<int> usedIds;
    for (const PaintObject &o : selMgr.allObjects()) {
        if (o.type == ObjectType::SelectionImage && o.selectionBufferId >= 0) {
            if (!usedIds.contains(o.selectionBufferId)) {
                usedIds.append(o.selectionBufferId);
            }
        }
    }
    s << qint32(usedIds.size());
    for (int id : usedIds) {
        s << qint32(id);
        writeImage(s, selMgr.selectionBufferFor(id));
    }
    return ba;
}

QByteArray serializeColors(const PaintArea &pa) {
    QByteArray ba;
    QDataStream s(&ba, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_6_0);
    s.setByteOrder(QDataStream::LittleEndian);
    s << pa.getPenColor1() << pa.getPenColor2();
    return ba;
}

QByteArray serializeToolState(const PaintArea &pa) {
    QByteArray ba;
    QDataStream s(&ba, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_6_0);
    s.setByteOrder(QDataStream::LittleEndian);

    s << pa.getZoomFactor();
    s << qint32(pa.getGradientType());
    s << qint32(pa.getGradientOpacity());
    s << qint32(pa.getGradientAngle());
    s << pa.getGradientReverse();
    s << pa.getGradientDither();
    s << qint32(pa.getGradientBlendMode());
    s << pa.getGradientUseSecondColor();
    return ba;
}

QByteArray serializeMode(const QString &mode) {
    QByteArray ba;
    QDataStream s(&ba, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_6_0);
    s.setByteOrder(QDataStream::LittleEndian);
    s << mode;
    return ba;
}

QByteArray serializeThumbnail(const LayerStack &stack) {
    QByteArray ba;
    QDataStream s(&ba, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_6_0);
    s.setByteOrder(QDataStream::LittleEndian);
    QImage full = const_cast<LayerStack &>(stack).compositedImage();
    QImage thumb = full.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    writeImage(s, thumb);
    return ba;
}

QByteArray serializeBrushes(const PaintArea &pa) {
    QByteArray ba;
    QDataStream s(&ba, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_6_0);
    s.setByteOrder(QDataStream::LittleEndian);

    s << qint32(pa.getActiveCustomBrushIndex());
    writeBrushSettings(s, pa.getCustomBrush(0));
    writeBrushSettings(s, pa.getCustomBrush(1));
    return ba;
}

QByteArray serializeMetadata(const SaveOptions &opts,
                             const QString &appVersion) {
    QByteArray ba;
    QDataStream s(&ba, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_6_0);
    s.setByteOrder(QDataStream::LittleEndian);

    s << appVersion;
    s << QDateTime::currentDateTimeUtc();
    writeString(s, opts.projectName);
    writeString(s, opts.author);
    return ba;
}


void deserializeCanvas(QDataStream &in, LayerStack &stack) {
    qint32 w = 0, h = 0, cur = 0;
    in >> w >> h >> cur;
    PTX_LOG(QStringLiteral("      Canvas: %1x%2, current=%3").arg(w).arg(h).arg(cur));
}

void deserializeLayers(QDataStream &in, LayerStack &stack) {
    qint32 count = 0;
    in >> count;

    PTX_LOG(QStringLiteral("      Layers: %1 capas declaradas").arg(count));

    if (count < 0 || count > 1000) {
        PTX_LOG(QStringLiteral("      ERROR: count fuera de rango (%1), abortando").arg(count));
        return;
    }

    stack.clearEverything();

    for (qint32 i = 0; i < count; ++i) {
        QString name = readString(in);
        bool visible = true;
        double opacity = 1.0;
        qint32 blend = 0;
        bool locked = false;
        in >> visible >> opacity >> blend >> locked;
        PTX_LOG(QStringLiteral("      Capa #%1: name='%2' vis=%3 op=%4 blend=%5")
                    .arg(i).arg(name).arg(visible).arg(opacity).arg(blend));

        QImage img = readImage(in);
        PTX_LOG(QStringLiteral("        img=%1x%2").arg(img.width()).arg(img.height()));

        if (i == 0) {
            stack.appendFirstLayer(img, name);
        } else {
            stack.addLayerSilent(name);
            stack.layerAt(stack.count() - 1).image = img;
        }
        int idx = stack.count() - 1;
        stack.layerAt(idx).visible = visible;
        stack.layerAt(idx).opacity = opacity;
        stack.layerAt(idx).blendMode = blend;
        stack.layerAt(idx).locked = locked;
    }
    PTX_LOG(QStringLiteral("      Layers DONE, total=%1").arg(stack.count()));
}

void deserializeMasksBN(QDataStream &in, LayerStack &stack) {
    qint32 count = 0;
    in >> count;
    PTX_LOG(QStringLiteral("      MasksBN: %1 máscaras").arg(count));

    if (count < 0 || count > 1000) {
        PTX_LOG(QStringLiteral("      ERROR: count fuera de rango, abortando"));
        return;
    }

    for (qint32 i = 0; i < count; ++i) {
        qint32 idx = 0;
        bool enabled = true;
        in >> idx >> enabled;
        QImage mask = readImage(in);
        if (idx < 0 || idx >= stack.count()) continue;

        stack.addMask(idx, mask.size());
        stack.maskRef(idx) = mask;

        bool currentEnabled = stack.isMaskEnabled(idx);
        if (currentEnabled != enabled) {
            stack.toggleMaskEnabled(idx);
        }
    }
    PTX_LOG(QStringLiteral("      MasksBN DONE"));
}

void deserializeMasksColor(QDataStream &in, LayerStack &stack) {
    qint32 count = 0;
    in >> count;
    PTX_LOG(QStringLiteral("      MasksColor: %1 máscaras").arg(count));

    if (count < 0 || count > 1000) {
        PTX_LOG(QStringLiteral("      ERROR: count fuera de rango, abortando"));
        return;
    }

    for (qint32 i = 0; i < count; ++i) {
        qint32 idx = 0;
        bool enabled = true;
        in >> idx >> enabled;
        FilterParams fp = readFilterParams(in);
        if (idx < 0 || idx >= stack.count()) continue;

        stack.addColorMask(idx, fp);
        bool currentEnabled = stack.isColorMaskEnabled(idx);
        if (currentEnabled != enabled) {
            stack.toggleColorMaskEnabled(idx);
        }
    }
    PTX_LOG(QStringLiteral("      MasksColor DONE"));
}

void deserializeFrames(QDataStream &in, PixelAnimationManager &anim) {
    bool active = false;
    qint32 res = 32, curIdx = 0, count = 0;
    in >> active >> res >> curIdx >> count;
    PTX_LOG(QStringLiteral("      Frames: active=%1 res=%2 cur=%3 count=%4")
                .arg(active).arg(res).arg(curIdx).arg(count));

    if (!active) {
        anim.deinit();
        return;
    }

    if (count < 0 || count > 10000) {
        PTX_LOG(QStringLiteral("      ERROR: count fuera de rango, abortando"));
        return;
    }

    anim.init(res);

    for (qint32 i = 0; i < count; ++i) {
        QImage frame = readImage(in);
        if (i == 0) {
            anim.setCurrentFrameImage(frame);
        } else {
            anim.addFrame();
            anim.setCurrentFrameImage(frame);
        }
    }
    anim.goToFrame(curIdx);
    PTX_LOG(QStringLiteral("      Frames DONE"));
}

void deserializeObjects(QDataStream &in, SelectionManager &selMgr) {
    qint32 count = 0;
    in >> count;
    PTX_LOG(QStringLiteral("      Objects: %1 objetos").arg(count));

    if (count < 0 || count > 100000) {
        PTX_LOG(QStringLiteral("      ERROR: count fuera de rango, abortando"));
        return;
    }

    selMgr.clearObjects();
    for (qint32 i = 0; i < count; ++i) {
        PaintObject o = readPaintObject(in);
        o.selected = false;
        selMgr.addObject(o);
    }
    PTX_LOG(QStringLiteral("      Objects DONE"));
}

void deserializeObjBuffers(QDataStream &in, SelectionManager &selMgr,
                           const QList<PaintObject> &objs) {
    qint32 count = 0;
    in >> count;
    PTX_LOG(QStringLiteral("      ObjBuffers: %1 buffers").arg(count));

    if (count < 0 || count > 100000) {
        PTX_LOG(QStringLiteral("      ERROR: count fuera de rango, abortando"));
        return;
    }

    QMap<int, QImage> loadedBuffers;
    for (qint32 i = 0; i < count; ++i) {
        qint32 id = 0;
        in >> id;
        QImage buf = readImage(in);
        loadedBuffers.insert(id, buf);
    }

    QList<PaintObject> &allObjs = selMgr.allObjects();
    for (PaintObject &o : allObjs) {
        if (o.type == ObjectType::SelectionImage) {
            int oldId = o.selectionBufferId;
            if (loadedBuffers.contains(oldId)) {
                int newId = selMgr.registerBufferForLoad(loadedBuffers.value(oldId));
                o.selectionBufferId = newId;
            }
        }
    }
    Q_UNUSED(objs);
    PTX_LOG(QStringLiteral("      ObjBuffers DONE"));
}

void deserializeColors(QDataStream &in, PaintArea &pa) {
    QColor c1, c2;
    in >> c1 >> c2;
    pa.setPenColor1(c1);
    pa.setPenColor2(c2);
}

void deserializeToolState(QDataStream &in, PaintArea &pa) {
    double zoom = 1.0;
    qint32 gt = 0, go = 255, ga = 0;
    bool gr = false, gd = false;
    qint32 gbm = 0;
    bool gusc = false;

    in >> zoom;
    in >> gt >> go >> ga;
    in >> gr >> gd >> gbm >> gusc;

    if (zoom > 0.01 && zoom < 64.0) pa.setZoomFactor(zoom);
    pa.setGradientType(gt);
    pa.setGradientOpacity(go);
    pa.setGradientAngle(ga);
    pa.setGradientReverse(gr);
    pa.setGradientDither(gd);
    pa.setGradientBlendMode(gbm);
    pa.setGradientUseSecondColor(gusc);
}

void deserializeMode(QDataStream &in, QString &outMode) {
    in >> outMode;
}

QImage deserializeThumbnail(QDataStream &in) {
    return readImage(in);
}

void deserializeMetadata(QDataStream &in, ProjectIO::Info &outInfo) {
    in >> outInfo.appVersion;
    in >> outInfo.created;
    outInfo.projectName = readString(in);
    outInfo.author = readString(in);
}

void deserializeBrushes(QDataStream &in, PaintArea &pa) {
    qint32 activeIdx = 0;
    in >> activeIdx;
    BrushSettings b1 = readBrushSettings(in);
    BrushSettings b2 = readBrushSettings(in);
    pa.setCustomBrushPresets(b1, b2, activeIdx);
}

}

namespace ProjectIO {

bool save(const QString &filePath,
          const LayerStack &stack,
          const SelectionManager &selMgr,
          const PixelAnimationManager &animManager,
          const PaintArea &paintArea,
          const QString &modeName,
          const SaveOptions &opts)
{
    if (filePath.isEmpty()) return false;

    QElapsedTimer totalTimer;
    totalTimer.start();

    qDebug().noquote() << "";
    qDebug().noquote() << "================ SAVE ================";
    qDebug().noquote() << "Archivo:" << filePath;
    qDebug().noquote() << "Capas en stack:" << stack.count();

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug().noquote() << "ERROR: no se pudo abrir para escritura";
        return false;
    }

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_6_0);
    out.setByteOrder(QDataStream::LittleEndian);

    if (!writeHeader(out)) {
        qDebug().noquote() << "ERROR: fallo al escribir header";
        return false;
    }

    QString appVersion = QCoreApplication::applicationVersion();
    if (appVersion.isEmpty()) appVersion = QStringLiteral("0.0.1");

    writeSection(out, SectionMetadata,
                 serializeMetadata(opts, appVersion),
                 opts.compressLargeSections, opts.compressionLevel);

    writeSection(out, SectionCanvas,
                 serializeCanvas(stack),
                 opts.compressLargeSections, opts.compressionLevel);

    writeSection(out, SectionLayers,
                 serializeLayers(stack),
                 opts.compressLargeSections, opts.compressionLevel);

    QByteArray masksBN = serializeMasksBN(stack);
    if (masksBN.size() > 4) {
        writeSection(out, SectionMasksBN, masksBN,
                     opts.compressLargeSections, opts.compressionLevel);
    }

    QByteArray masksColor = serializeMasksColor(stack);
    if (masksColor.size() > 4) {
        writeSection(out, SectionMasksColor, masksColor,
                     opts.compressLargeSections, opts.compressionLevel);
    }

    if (animManager.getIsActive()) {
        writeSection(out, SectionFrames,
                     serializeFrames(animManager),
                     opts.compressLargeSections, opts.compressionLevel);
    }

    QByteArray objs = serializeObjects(selMgr);
    if (objs.size() > 4) {
        writeSection(out, SectionObjects, objs,
                     opts.compressLargeSections, opts.compressionLevel);

        QByteArray bufs = serializeObjBuffers(selMgr);
        if (bufs.size() > 4) {
            writeSection(out, SectionObjBuffers, bufs,
                         opts.compressLargeSections, opts.compressionLevel);
        }
    }

    writeSection(out, SectionBrushes,
                 serializeBrushes(paintArea),
                 opts.compressLargeSections, opts.compressionLevel);

    writeSection(out, SectionColors,
                 serializeColors(paintArea),
                 false, opts.compressionLevel);

    writeSection(out, SectionToolState,
                 serializeToolState(paintArea),
                 false, opts.compressionLevel);

    writeSection(out, SectionMode,
                 serializeMode(modeName),
                 false, opts.compressionLevel);

    if (opts.includeThumbnail) {
        writeSection(out, SectionThumbnail,
                     serializeThumbnail(stack),
                     false, opts.compressionLevel);
    }

    if (!writeFooter(out)) {
        qDebug().noquote() << "ERROR: fallo al escribir footer";
        return false;
    }

    if (!file.commit()) {
        qDebug().noquote() << "ERROR: fallo al commit";
        return false;
    }

    qDebug().noquote() << QStringLiteral("SAVE COMPLETO (%1 ms, %2 bytes)")
        .arg(totalTimer.elapsed())
        .arg(file.size());
    return true;
}


LoadResult load(const QString &filePath,
                LayerStack &stack,
                SelectionManager &selMgr,
                PixelAnimationManager &animManager,
                PaintArea &paintArea,
                QString &outModeName)
{
    LoadResult result;

    QElapsedTimer totalTimer;
    totalTimer.start();

    qDebug().noquote() << "";
    qDebug().noquote() << "================ LOAD ================";
    qDebug().noquote() << "Archivo:" << filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.errorMessage = QStringLiteral("No se pudo abrir el archivo");
        qDebug().noquote() << "ERROR: no se pudo abrir";
        return result;
    }
    qDebug().noquote() << "Tamaño:" << file.size() << "bytes";

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_0);
    in.setByteOrder(QDataStream::LittleEndian);

    quint16 version = 0;
    if (!readHeader(in, version)) {
        result.errorMessage = QStringLiteral("Formato no reconocido (magic inválido)");
        qDebug().noquote() << "ERROR: header inválido";
        return result;
    }
    result.fileVersion = version;
    qDebug().noquote() << "Header OK, versión:" << version;

    if (version > FORMAT_VERSION) {
        result.errorMessage = QStringLiteral(
            "El archivo fue creado con una versión más reciente (%1 > %2)")
            .arg(version).arg(FORMAT_VERSION);
        return result;
    }

    stack.suspendCompose(true);
    qDebug().noquote() << "Composición suspendida";

    stack.clearEverything();
    selMgr.clearAll();
    animManager.deinit();
    qDebug().noquote() << "Estado reseteado";

    QByteArray objectsPayload;
    QByteArray objBuffersPayload;
    QList<PaintObject> pendingObjects;

    int sectionCount = 0;
    bool sawFooter = false;
    while (!in.atEnd()) {
        sectionCount++;

        qint64 posBefore = file.pos();
        quint32 maybeMagic = 0;
        in >> maybeMagic;
        if (maybeMagic == MAGIC_FOOTER) {
            qDebug().noquote() << ">>> FOOTER encontrado tras"
                               << sectionCount << "secciones";
            sawFooter = true;
            break;
        }
        file.seek(posBefore);
        in.device()->seek(posBefore);

        quint32 id = 0, flags = 0;
        quint64 size = 0;
        if (!readSectionHeader(in, id, flags, size)) {
            qDebug().noquote() << "ERROR leyendo cabecera de sección";
            result.warnings.append(QStringLiteral("Error leyendo cabecera de sección"));
            break;
        }

        qDebug().noquote() << QStringLiteral("--- Sección #%1: id=0x%2 flags=%3 size=%4")
            .arg(sectionCount)
            .arg(QString::number(id, 16))
            .arg(flags)
            .arg(size);

        QByteArray payload = readSectionPayload(in, flags, size);
        qDebug().noquote() << QStringLiteral("    payload=%1 bytes").arg(payload.size());

        if (payload.isEmpty() && size > 0) {
            result.warnings.append(
                QStringLiteral("Sección 0x%1 vacía o corrupta").arg(id, 0, 16));
            continue;
        }

        QDataStream s(&payload, QIODevice::ReadOnly);
        s.setVersion(QDataStream::Qt_6_0);
        s.setByteOrder(QDataStream::LittleEndian);

        switch (id) {
        case SectionCanvas:
            deserializeCanvas(s, stack);
            break;

        case SectionLayers:
            deserializeLayers(s, stack);
            break;

        case SectionMasksBN:
            deserializeMasksBN(s, stack);
            break;

        case SectionMasksColor:
            deserializeMasksColor(s, stack);
            break;

        case SectionFrames:
            deserializeFrames(s, animManager);
            break;

        case SectionObjects:
            objectsPayload = payload;
            break;

        case SectionObjBuffers:
            objBuffersPayload = payload;
            break;

        case SectionBrushes:
            deserializeBrushes(s, paintArea);
            break;

        case SectionColors:
            deserializeColors(s, paintArea);
            break;

        case SectionToolState:
            deserializeToolState(s, paintArea);
            break;

        case SectionMode:
            deserializeMode(s, outModeName);
            break;

        case SectionThumbnail:
            break;

        case SectionMetadata: {
            Info info;
            deserializeMetadata(s, info);
            result.appVersion = info.appVersion;
            break;
        }

        default:
            result.unknownSections.append(id);
            break;
        }
    }

    if (!objectsPayload.isEmpty()) {
        QDataStream s(&objectsPayload, QIODevice::ReadOnly);
        s.setVersion(QDataStream::Qt_6_0);
        s.setByteOrder(QDataStream::LittleEndian);
        deserializeObjects(s, selMgr);

        if (!objBuffersPayload.isEmpty()) {
            QDataStream bs(&objBuffersPayload, QIODevice::ReadOnly);
            bs.setVersion(QDataStream::Qt_6_0);
            bs.setByteOrder(QDataStream::LittleEndian);
            deserializeObjBuffers(bs, selMgr, pendingObjects);
        }
    }

    stack.suspendCompose(false);
    stack.recompose();
    stack.ensureComposited();

    if (!sawFooter) {
        result.warnings.append(QStringLiteral("Footer ausente — archivo posiblemente truncado"));
    }

    qDebug().noquote() << QStringLiteral("LOAD COMPLETO (%1 ms)")
        .arg(totalTimer.elapsed());
    result.success = true;
    return result;
}


Info peek(const QString &filePath) {
    Info info;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return info;

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_0);
    in.setByteOrder(QDataStream::LittleEndian);

    quint16 version = 0;
    if (!readHeader(in, version)) return info;

    info.valid = true;
    info.version = version;

    while (!in.atEnd()) {
        qint64 posBefore = file.pos();
        quint32 maybeMagic = 0;
        in >> maybeMagic;
        if (maybeMagic == MAGIC_FOOTER) break;
        file.seek(posBefore);
        in.device()->seek(posBefore);

        quint32 id = 0, flags = 0;
        quint64 size = 0;
        if (!readSectionHeader(in, id, flags, size)) break;

        QByteArray payload = readSectionPayload(in, flags, size);

        QDataStream s(&payload, QIODevice::ReadOnly);
        s.setVersion(QDataStream::Qt_6_0);
        s.setByteOrder(QDataStream::LittleEndian);

        switch (id) {
        case SectionCanvas: {
            qint32 w = 0, h = 0, cur = 0;
            s >> w >> h >> cur;
            info.canvasWidth = w;
            info.canvasHeight = h;
            break;
        }
        case SectionLayers: {
            qint32 count = 0;
            s >> count;
            info.layerCount = count;
            break;
        }
        case SectionMetadata: {
            deserializeMetadata(s, info);
            break;
        }
        case SectionThumbnail: {
            info.thumbnail = deserializeThumbnail(s);
            break;
        }
        default:
            break;
        }
    }

    file.close();
    return info;
}


bool isProjectFile(const QString &filePath) {
    return QFileInfo(filePath).suffix().toLower() == QStringLiteral("ptx");
}


QString suggestedFileName(const QString &baseName) {
    QString base = baseName.isEmpty() ? QStringLiteral("Sin titulo") : baseName;
    return base + QString::fromLatin1(EXTENSION);
}

} // namespace ProjectIO