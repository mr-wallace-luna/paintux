#ifndef EXIFLOADER_H
#define EXIFLOADER_H

#include <QImage>
#include <QString>
#include <QDateTime>
#include <QList>
#include <QColor>
#include <QRectF>
#include <QFont>

// Forward declarations — evitamos arrastrar headers pesados
class LayerStack;
class SelectionManager;
class PixelAnimationManager;
class PaintArea;
struct FilterParams;
struct BrushSettings;
struct PaintObject;
struct ShapeElement;

namespace ExifLoader {

int readOrientation(const QString &filePath);
QImage applyOrientation(const QImage &img, int orientation);
QImage loadRespectingExif(const QString &filePath);

} 

namespace ProjectIO {

// --- Constantes del formato ---
constexpr quint32 MAGIC_HEADER = 0x31585450;  // "PTX1" en LE
constexpr quint32 MAGIC_FOOTER = 0x45585450;  // "PTXE" en LE
constexpr quint16 FORMAT_VERSION = 1;
constexpr const char* EXTENSION = ".ptx";
constexpr const char* FILE_FILTER = "Proyectos Paint-UX (*.ptx)";
constexpr quint64 COMPRESS_THRESHOLD = 4096;  // 4 KB

// --- IDs de sección ---
enum SectionId : quint32 {
    SectionCanvas     = 0x01,
    SectionLayers     = 0x02,
    SectionMasksBN    = 0x03,
    SectionMasksColor = 0x04,
    SectionFrames     = 0x05,
    SectionObjects    = 0x06,
    SectionObjBuffers = 0x07,
    SectionBrushes    = 0x08,
    SectionColors     = 0x09,
    SectionToolState  = 0x0A,
    SectionMode       = 0x0B,
    SectionThumbnail  = 0x0C,
    SectionMetadata   = 0x0D,
    SectionCustom     = 0x0E,
};

// --- Flags de sección ---
enum SectionFlags : quint32 {
    FlagCompressed = 1u << 0,
};

// --- Opciones de guardado ---
struct SaveOptions {
    bool includeThumbnail = true;
    bool compressLargeSections = true;
    int  compressionLevel = 6;
    QString projectName;
    QString author;
};

// --- Resultado de carga ---
struct LoadResult {
    bool success = false;
    QString errorMessage;
    quint16 fileVersion = 0;
    QString appVersion;
    QList<quint32> unknownSections;
    QList<QString> warnings;
};

// --- Info rápida (sin cargar todo) ---
struct Info {
    bool valid = false;
    quint16 version = 0;
    QString appVersion;
    int canvasWidth = 0;
    int canvasHeight = 0;
    int layerCount = 0;
    QString projectName;
    QString author;
    QDateTime created;
    QDateTime modified;
    QImage thumbnail;
};



bool save(const QString &filePath,
          const LayerStack &stack,
          const SelectionManager &selMgr,
          const PixelAnimationManager &animManager,
          const PaintArea &paintArea,
          const QString &modeName,
          const SaveOptions &opts = SaveOptions());

LoadResult load(const QString &filePath,
                LayerStack &stack,
                SelectionManager &selMgr,
                PixelAnimationManager &animManager,
                PaintArea &paintArea,
                QString &outModeName);

Info peek(const QString &filePath);

bool isProjectFile(const QString &filePath);
QString suggestedFileName(const QString &baseName = QStringLiteral("Sin titulo"));

} 

#endif // EXIFLOADER_H