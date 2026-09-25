#ifndef PIXELANIMATIONTOOLS_H
#define PIXELANIMATIONTOOLS_H
#include <QImage>
#include <QList>
#include <QString>
#include <QWidget>
#include <QFrame>
#include <QPainter>
#include <QPixmap>
#include <QMouseEvent>
#include <QPen>
#include <QFile>
#include <QDataStream>
#include <QColor>
#include <QSet>
#include <vector>
#include <algorithm>
#include <climits>
#include <unordered_map>

// --- CLASE DE ANIMACIÓN PARA PIXEL ART ---
class PixelAnimationManager {
private:
    QList<QImage> pixelFrames;
    int currentFrameIdx;
    int pixelArtResolution;
    bool isActive;
public:
    PixelAnimationManager() : currentFrameIdx(0), pixelArtResolution(32), isActive(false) {}
    void init(int resolution) {
        pixelArtResolution = resolution;
        pixelFrames.clear();
        QImage nuevaImagen(pixelArtResolution, pixelArtResolution, QImage::Format_ARGB32);
        nuevaImagen.fill(Qt::transparent);
        pixelFrames.append(nuevaImagen);
        currentFrameIdx = 0;
        isActive = true;
    }
    void deinit() {
        pixelFrames.clear();
        currentFrameIdx = 0;
        isActive = false;
    }
    void addFrame() {
        if (!isActive) return;
        QImage nuevaImagen(pixelArtResolution, pixelArtResolution, QImage::Format_ARGB32);
        nuevaImagen.fill(Qt::transparent);
        pixelFrames.append(nuevaImagen);
        currentFrameIdx = pixelFrames.size() - 1;
    }
    void duplicateFrame() {
        if (!isActive || pixelFrames.isEmpty()) return;
        pixelFrames.insert(currentFrameIdx + 1, pixelFrames[currentFrameIdx].copy());
        currentFrameIdx++;
    }
    void deleteFrame() {
        if (!isActive || pixelFrames.size() <= 1) return;
        pixelFrames.removeAt(currentFrameIdx);
        if (currentFrameIdx >= pixelFrames.size()) currentFrameIdx = pixelFrames.size() - 1;
    }
    void goToFrame(int index) {
        if (!isActive || index < 0 || index >= pixelFrames.size()) return;
        currentFrameIdx = index;
    }
    void nextFrame() {
        if (isActive && currentFrameIdx < pixelFrames.size() - 1) goToFrame(currentFrameIdx + 1);
    }
    void prevFrame() {
        if (isActive && currentFrameIdx > 0) goToFrame(currentFrameIdx - 1);
    }
    void setCurrentFrameImage(const QImage &img) {
        if (isActive && currentFrameIdx >= 0 && currentFrameIdx < pixelFrames.size()) {
            pixelFrames[currentFrameIdx] = img;
        }
    }
    QImage getCurrentFrameImage() const {
        if (isActive && currentFrameIdx >= 0 && currentFrameIdx < pixelFrames.size()) {
            return pixelFrames[currentFrameIdx];
        }
        return QImage();
    }
    const QList<QImage>& getFrames() const { return pixelFrames; }
    int getCurrentFrameIndex() const { return currentFrameIdx; }
    bool getIsActive() const { return isActive; }
    int getResolution() const { return pixelArtResolution; }
};

// --- CLASE DE OPCIONES DE PIXEL ART ---
class PixelArtOptions {
private:
    bool isPixelArtMode;
    int pixelArtGridSize;
    int pixelArtResolution;
    bool gridActive;
public:
    PixelArtOptions() : isPixelArtMode(false), pixelArtGridSize(0), pixelArtResolution(32), gridActive(false) {}
    void setPixelArtMode(bool active, int resolution = 32) {
        isPixelArtMode = active;
        pixelArtResolution = resolution;
        gridActive = (pixelArtGridSize > 0);
    }
    void setGridSize(int size) {
        pixelArtGridSize = size;
        gridActive = (size > 0);
    }
    void setGridActive(bool active) { gridActive = active; }
    void setPixelArtResolution(int resolution) { pixelArtResolution = resolution; }
    bool getIsPixelArtMode() const { return isPixelArtMode; }
    int getGridSize() const { return pixelArtGridSize; }
    int getResolution() const { return pixelArtResolution; }
    bool isGridActive() const { return gridActive; }
};

// ============================================================
// 🎬 GIF ENCODER - Con soporte de ESCALADO para Pixel Art
// ============================================================
class GifEncoder {
private:
    struct Color {
        uint8_t r, g, b;
        bool operator==(const Color& o) const { return r == o.r && g == o.g && b == o.b; }
    };

    struct LZWEntry {
        std::vector<uint8_t> seq;
        int code;
    };

    QList<Color> palette;
    int transparentIndex;
    bool hasTransparency;

    static void writeLE16(QDataStream &out, uint16_t v) {
        out << (uint8_t)(v & 0xFF);
        out << (uint8_t)((v >> 8) & 0xFF);
    }

    int paletteSizePower() const {
        int n = palette.size();
        int p = 2;
        while (p < n) p *= 2;
        if (p < 2) p = 2;
        if (p > 256) p = 256;
        return p;
    }

    int paletteBits() const {
        int p = paletteSizePower();
        int bits = 1;
        while ((1 << bits) < p) bits++;
        return bits;
    }

    void writeGlobalColorTable(QDataStream &out) {
        int tableSize = paletteSizePower();
        for (int i = 0; i < tableSize; ++i) {
            if (i < palette.size()) {
                out << palette[i].r << palette[i].g << palette[i].b;
            } else {
                out << (uint8_t)0 << (uint8_t)0 << (uint8_t)0;
            }
        }
    }

    void buildPalette(const QList<QImage> &frames) {
        palette.clear();
        QSet<uint32_t> seen;
        hasTransparency = false;

        for (const QImage &fr : frames) {
            for (int y = 0; y < fr.height(); ++y) {
                for (int x = 0; x < fr.width(); ++x) {
                    QColor c = fr.pixelColor(x, y);
                    if (c.alpha() == 0) { hasTransparency = true; continue; }
                    uint32_t key = ((uint32_t)c.red() << 16) | ((uint32_t)c.green() << 8) | (uint32_t)c.blue();
                    if (!seen.contains(key)) {
                        seen.insert(key);
                        palette.append({(uint8_t)c.red(), (uint8_t)c.green(), (uint8_t)c.blue()});
                        if (palette.size() >= 255) break;
                    }
                }
                if (palette.size() >= 255) break;
            }
            if (palette.size() >= 255) break;
        }

        transparentIndex = -1;
        if (hasTransparency) {
            transparentIndex = palette.size();
            palette.append({0, 0, 0});
        }

        while (palette.size() < 2) {
            palette.append({0, 0, 0});
        }
    }

    int findClosest(const QColor &c) const {
        int best = 0;
        int bestDist = INT_MAX;
        int limit = (hasTransparency && transparentIndex >= 0) ? transparentIndex : palette.size();
        for (int i = 0; i < limit; ++i) {
            int dr = c.red() - palette[i].r;
            int dg = c.green() - palette[i].g;
            int db = c.blue() - palette[i].b;
            int d = dr*dr + dg*dg + db*db;
            if (d < bestDist) { bestDist = d; best = i; }
        }
        return best;
    }

    std::vector<uint8_t> quantizeFrame(const QImage &frame) {
        std::vector<uint8_t> idx(frame.width() * frame.height());
        for (int y = 0; y < frame.height(); ++y) {
            for (int x = 0; x < frame.width(); ++x) {
                QColor c = frame.pixelColor(x, y);
                if (c.alpha() == 0 && hasTransparency) {
                    idx[y * frame.width() + x] = (uint8_t)transparentIndex;
                } else {
                    idx[y * frame.width() + x] = (uint8_t)findClosest(c);
                }
            }
        }
        return idx;
    }

    std::vector<uint8_t> lzwCompress(const std::vector<uint8_t> &data, int minCodeSize) {
        std::vector<uint8_t> out;
        int clearCode = 1 << minCodeSize;
        int eoiCode = clearCode + 1;
        int codeSize = minCodeSize + 1;
        int nextCode = eoiCode + 1;
        int maxCode = (1 << codeSize);

        struct SeqHash {
            size_t operator()(const std::vector<uint8_t> &v) const {
                size_t h = 0;
                for (uint8_t b : v) h = h * 131 + b;
                return h;
            }
        };
        std::unordered_map<std::vector<uint8_t>, int, SeqHash> table;
        auto resetTable = [&]() {
            table.clear();
            for (int i = 0; i < clearCode; ++i) {
                table[{(uint8_t)i}] = i;
            }
            nextCode = eoiCode + 1;
            codeSize = minCodeSize + 1;
            maxCode = 1 << codeSize;
        };
        resetTable();

        uint32_t bitBuffer = 0;
        int bitsInBuffer = 0;

        auto writeBits = [&](int code, int nBits) {
            bitBuffer |= ((uint32_t)code << bitsInBuffer);
            bitsInBuffer += nBits;
            while (bitsInBuffer >= 8) {
                out.push_back((uint8_t)(bitBuffer & 0xFF));
                bitBuffer >>= 8;
                bitsInBuffer -= 8;
            }
        };

        writeBits(clearCode, codeSize);

        if (data.empty()) {
            writeBits(eoiCode, codeSize);
            if (bitsInBuffer > 0) out.push_back((uint8_t)(bitBuffer & 0xFF));
            return out;
        }

        std::vector<uint8_t> current;
        current.push_back(data[0]);

        for (size_t i = 1; i < data.size(); ++i) {
            uint8_t k = data[i];
            std::vector<uint8_t> next = current;
            next.push_back(k);

            auto it = table.find(next);
            if (it != table.end()) {
                current = next;
            } else {
                auto itCur = table.find(current);
                if (itCur != table.end()) {
                    writeBits(itCur->second, codeSize);
                }

                if (nextCode < 4096) {
                    table[next] = nextCode++;
                    if (nextCode > maxCode && codeSize < 12) {
                        codeSize++;
                        maxCode = 1 << codeSize;
                    }
                } else {
                    writeBits(clearCode, codeSize);
                    resetTable();
                }
                current = {k};
            }
        }

        auto itCur = table.find(current);
        if (itCur != table.end()) writeBits(itCur->second, codeSize);

        writeBits(eoiCode, codeSize);
        if (bitsInBuffer > 0) out.push_back((uint8_t)(bitBuffer & 0xFF));
        return out;
    }

    void writeSubBlocks(QDataStream &out, const std::vector<uint8_t> &data) {
        size_t pos = 0;
        while (pos < data.size()) {
            size_t chunk = std::min((size_t)255, data.size() - pos);
            out << (uint8_t)chunk;
            out.writeRawData(reinterpret_cast<const char*>(data.data() + pos), chunk);
            pos += chunk;
        }
        out << (uint8_t)0;
    }

public:
    // 🎯 NUEVO PARÁMETRO: scale (factor de escalado, 1 = sin escala, 8 = 8x más grande, etc.)
    // Usa FastTransformation para mantener la estética pixel art (sin suavizado)
    bool save(const QString &filename, const QList<QImage> &frames, int delayMs = 100, bool loop = true, int scale = 1) {
        if (frames.isEmpty()) return false;
        if (scale < 1) scale = 1;
        if (scale > 32) scale = 32;

        // 🎯 ESCALAR FRAMES si es necesario
        QList<QImage> scaledFrames;
        for (const QImage &fr : frames) {
            if (scale > 1) {
                // FastTransformation = sin suavizado, perfecto para pixel art
                QImage s = fr.scaled(fr.width() * scale, fr.height() * scale,
                                     Qt::KeepAspectRatio, Qt::FastTransformation);
                scaledFrames.append(s);
            } else {
                scaledFrames.append(fr);
            }
        }

        QFile file(filename);
        if (!file.open(QIODevice::WriteOnly)) return false;

        QDataStream out(&file);
        out.setByteOrder(QDataStream::LittleEndian);

        int width = scaledFrames[0].width();
        int height = scaledFrames[0].height();

        buildPalette(scaledFrames);
        int colorBits = paletteBits();

        out.writeRawData("GIF89a", 6);
        writeLE16(out, width);
        writeLE16(out, height);
        uint8_t packed = 0x80 | ((colorBits - 1) << 4) | (colorBits - 1);
        out << packed;
        out << (uint8_t)0;
        out << (uint8_t)0;

        writeGlobalColorTable(out);

        if (loop) {
            out << (uint8_t)0x21;
            out << (uint8_t)0xFF;
            out << (uint8_t)11;
            out.writeRawData("NETSCAPE2.0", 11);
            out << (uint8_t)3;
            out << (uint8_t)1;
            writeLE16(out, 0);
            out << (uint8_t)0;
        }

        int delayCentiseconds = qMax(2, delayMs / 10);
        for (const QImage &frame : scaledFrames) {
            out << (uint8_t)0x21;
            out << (uint8_t)0xF9;
            out << (uint8_t)4;
            uint8_t gcePacked = hasTransparency ? 0x09 : 0x08;
            out << gcePacked;
            writeLE16(out, (uint16_t)delayCentiseconds);
            out << (uint8_t)(hasTransparency ? transparentIndex : 0);
            out << (uint8_t)0;

            out << (uint8_t)0x2C;
            writeLE16(out, 0);
            writeLE16(out, 0);
            writeLE16(out, width);
            writeLE16(out, height);
            out << (uint8_t)0;

            int minCodeSize = qMax(2, colorBits);
            out << (uint8_t)minCodeSize;
            std::vector<uint8_t> indexed = quantizeFrame(frame);
            std::vector<uint8_t> compressed = lzwCompress(indexed, minCodeSize);
            writeSubBlocks(out, compressed);
        }

        out << (uint8_t)0x3B;
        file.close();
        return true;
    }
};

#endif // PIXELANIMATIONTOOLS_H