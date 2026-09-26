#ifndef MAGICWANDTOOLS_H
#define MAGICWANDTOOLS_H

#include <QImage>
#include <QPoint>
#include <QColor>
#include <QRect>
#include <QtGlobal>
#include <QVector>
#include <QPainter>
#include <cstring>
#include <cmath>
#include <queue>
#include <vector>
#include <algorithm>


// Resultado de la varita mágica
struct MagicWandResult {
    QImage mask;          // ARGB32: alpha=255 selección dura, alpha<255 borde suave
    QRect boundingBox;    // Rect que envuelve la selección
    int pixelsSelected;   // Píxeles con alpha > 0
    int pixelsSoft;       // Píxeles con alpha parcial (borde suave)

    MagicWandResult() : pixelsSelected(0), pixelsSoft(0) {}
};


// Opciones configurables
struct MagicWandOptions {
    int tolerance       = 30;    // 0-255
    bool contiguous     = true;  // true=solo conectados; false=todo el lienzo
    bool includeAlpha   = true;  // considerar canal alpha
    bool usePerceptual  = false; // false=RGB (original), true=Lab (calibrado)
    bool softEdges      = true;  // borde con alpha gradual
    int sampleRadius    = 0;     // radio de muestreo del color semilla (0=pixel exacto)
    int feather         = 0;     // blur gaussiano posterior (0=off)
    double edgeRefine   = 0.0;   // >0 contrae hacia bordes reales (Sobel); <0 expande

    MagicWandOptions() {}
};


// Color Lab para modo perceptual (opcional)
struct LabColor { double L, a, b; };

static inline double srgbCompToLinear(int c) {
    double cs = c / 255.0;
    return (cs <= 0.04045) ? (cs / 12.92) : std::pow((cs + 0.055) / 1.055, 2.4);
}

static inline LabColor rgbToLab(int r, int g, int b) {
    double rl = srgbCompToLinear(r);
    double gl = srgbCompToLinear(g);
    double bl = srgbCompToLinear(b);

    double X = (rl * 0.4124564 + gl * 0.3575761 + bl * 0.1804375) * 100.0;
    double Y = (rl * 0.2126729 + gl * 0.7151522 + bl * 0.0721750) * 100.0;
    double Z = (rl * 0.0193339 + gl * 0.1191920 + bl * 0.9503041) * 100.0;

    const double Xn = 95.047, Yn = 100.0, Zn = 108.883;
    const double eps = 216.0 / 24389.0;
    const double k   = 24389.0 / 27.0;

    double fx = (X / Xn > eps) ? std::cbrt(X / Xn) : (k * (X / Xn) + 16.0) / 116.0;
    double fy = (Y / Yn > eps) ? std::cbrt(Y / Yn) : (k * (Y / Yn) + 16.0) / 116.0;
    double fz = (Z / Zn > eps) ? std::cbrt(Z / Zn) : (k * (Z / Zn) + 16.0) / 116.0;

    LabColor lab;
    lab.L = 116.0 * fy - 16.0;
    lab.a = 500.0 * (fx - fy);
    lab.b = 200.0 * (fy - fz);
    return lab;
}


// Nodo para region growing con prioridad

struct WandNode { double dist; int x, y; };
struct WandNodeCmp {
    bool operator()(const WandNode &a, const WandNode &b) const { return a.dist > b.dist; }
};

// MagicWandTools
class MagicWandTools {
public:
    static MagicWandResult applyMagicWand(const QImage &image, const QPoint &pos,
                                          int tolerance = 32,
                                          bool contiguous = true,
                                          bool includeAlpha = true) {
        MagicWandOptions opts;
        opts.tolerance     = tolerance;
        opts.contiguous    = contiguous;
        opts.includeAlpha  = includeAlpha;
        opts.usePerceptual = false;   // RGB como el original que funcionaba
        opts.softEdges     = true;    // única mejora visible: borde suave
        opts.sampleRadius  = 0;
        opts.feather       = 0;
        opts.edgeRefine    = 0.0;
        return applyMagicWand(image, pos, opts);
    }


    // API nueva con opciones completas

    static MagicWandResult applyMagicWand(const QImage &image, const QPoint &pos,
                                          const MagicWandOptions &opts) {
        MagicWandResult result;
        if (image.isNull()) return result;
        if (pos.x() < 0 || pos.x() >= image.width() ||
            pos.y() < 0 || pos.y() >= image.height()) return result;

        QImage img = image;
        if (img.format() != QImage::Format_ARGB32 &&
            img.format() != QImage::Format_ARGB32_Premultiplied &&
            img.format() != QImage::Format_RGB32) {
            img = img.convertToFormat(QImage::Format_ARGB32);
        }

        const int w = img.width(), h = img.height();

        // --- Color semilla (muestreo por radio opcional) ---
        int seedR, seedG, seedB, seedA;
        sampleSeedColor(img, pos, qMax(0, opts.sampleRadius), seedR, seedG, seedB, seedA);
        LabColor seedLab = rgbToLab(seedR, seedG, seedB);

        // --- Criterio de similitud (idéntico al original para RGB) ---
        int numCh = opts.includeAlpha ? 4 : 3;
        double tolDist = 0.0, softLimitDist = 0.0;
        const double softFactor = 1.35; // banda suave hasta 35% más del umbral

        if (opts.usePerceptual) {
            // Delta E calibrado: tolerance=32 -> ~17.6 Delta E (NO selecciona todo)
            tolDist = opts.tolerance * 0.55;
        } else {
            // RGB euclidiano: igual al original (tolerance^2 * canales)
            tolDist = opts.tolerance * std::sqrt((double)numCh);
        }
        softLimitDist = tolDist * softFactor;
        if (tolDist <= 0.0) { tolDist = 0.5; softLimitDist = opts.softEdges ? 1.5 : 0.5; }

        // Función de distancia según modo
        auto pixelDist = [&](int r, int g, int b, int a) -> double {
            if (opts.usePerceptual) {
                LabColor lab = rgbToLab(r, g, b);
                double dL = lab.L - seedLab.L;
                double da = lab.a - seedLab.a;
                double db = lab.b - seedLab.b;
                double d = std::sqrt(dL*dL + da*da + db*db);
                if (opts.includeAlpha) {
                    double dAlpha = (a - seedA) * (100.0 / 255.0);
                    d = std::sqrt(d*d + dAlpha*dAlpha);
                }
                return d;
            } else {
                double dr = r - seedR, dg = g - seedG, db = b - seedB;
                double dSq = dr*dr + dg*dg + db*db;
                if (opts.includeAlpha) { double dA = a - seedA; dSq += dA*dA; }
                return std::sqrt(dSq);
            }
        };

        // Alpha según distancia (selección dura + banda suave)
        auto alphaFromDist = [&](double d) -> int {
            if (d <= tolDist) return 255;
            if (opts.softEdges && d <= softLimitDist && softLimitDist > tolDist) {
                double t = (d - tolDist) / (softLimitDist - tolDist);
                int al = (int)(255.0 * (1.0 - t) + 0.5);
                return qBound(0, al, 255);
            }
            return 0;
        };

        // --- Máscara de salida ---
        result.mask = QImage(w, h, QImage::Format_ARGB32);
        result.mask.fill(Qt::transparent);
        uchar *maskBits = result.mask.bits();
        const int maskStride = result.mask.bytesPerLine();

        int minX = w, minY = h, maxX = 0, maxY = 0;
        auto markPixel = [&](int x, int y, int alpha) {
            uchar *mLine = maskBits + y * maskStride;
            uchar *mPx = mLine + x * 4;
            mPx[0] = 255; mPx[1] = 255; mPx[2] = 255; mPx[3] = (uchar)alpha;
            result.pixelsSelected++;
            if (alpha > 0 && alpha < 255) result.pixelsSoft++;
            if (x < minX) minX = x; if (x > maxX) maxX = x;
            if (y < minY) minY = y; if (y > maxY) maxY = y;
        };

        if (opts.contiguous) {
            // === Region growing con cola de prioridad (más coherente) ===
            QVector<bool> visited(w * h, false);
            std::priority_queue<WandNode, std::vector<WandNode>, WandNodeCmp> pq;

            auto tryPush = [&](int x, int y) {
                if (x < 0 || x >= w || y < 0 || y >= h) return;
                int idx = y * w + x;
                if (visited[idx]) return;
                visited[idx] = true;
                QRgb px = ((const QRgb*)img.constScanLine(y))[x];
                double d = pixelDist(qRed(px), qGreen(px), qBlue(px), qAlpha(px));
                pq.push({d, x, y});
            };

            tryPush(pos.x(), pos.y());

            while (!pq.empty()) {
                WandNode n = pq.top(); pq.pop();
                int alpha = alphaFromDist(n.dist);
                if (alpha <= 0) continue; // la región se detiene aquí
                markPixel(n.x, n.y, alpha);
                tryPush(n.x - 1, n.y);
                tryPush(n.x + 1, n.y);
                tryPush(n.x, n.y - 1);
                tryPush(n.x, n.y + 1);
            }
        } else {
            // === GLOBAL: todos los píxeles similares del lienzo ===
            for (int y = 0; y < h; ++y) {
                const QRgb *line = (const QRgb*)img.constScanLine(y);
                for (int x = 0; x < w; ++x) {
                    QRgb px = line[x];
                    double d = pixelDist(qRed(px), qGreen(px), qBlue(px), qAlpha(px));
                    int alpha = alphaFromDist(d);
                    if (alpha > 0) markPixel(x, y, alpha);
                }
            }
        }

        if (result.pixelsSelected > 0)
            result.boundingBox = QRect(minX, minY, maxX - minX + 1, maxY - minY + 1);

        // --- Post-proceso: refinar bordes con Sobel ---
        if (opts.edgeRefine != 0.0 && result.pixelsSelected > 0) {
            result.mask = refineMaskWithEdges(result.mask, img, opts.edgeRefine);
            recomputeMaskStats(result);
        }
        // --- Post-proceso: feather gaussiano ---
        if (opts.feather > 0 && result.pixelsSelected > 0) {
            featherMask(result.mask, opts.feather);
            recomputeMaskStats(result);
        }

        return result;
    }


    static QImage extractMaskedRegion(const QImage &source, const QImage &mask, const QRect &bbox) {
        if (source.isNull() || mask.isNull()) return QImage();
        QRect r = bbox.intersected(source.rect()).intersected(mask.rect());
        if (r.isEmpty()) return QImage();

        QImage src = source;
        if (src.format() != QImage::Format_ARGB32) src = src.convertToFormat(QImage::Format_ARGB32);
        QImage msk = mask;
        if (msk.format() != QImage::Format_ARGB32) msk = msk.convertToFormat(QImage::Format_ARGB32);

        QImage out(r.size(), QImage::Format_ARGB32);
        out.fill(Qt::transparent);

        for (int y = 0; y < r.height(); ++y) {
            const QRgb *srcLine = (const QRgb*)src.constScanLine(r.y() + y);
            const QRgb *mskLine = (const QRgb*)msk.constScanLine(r.y() + y);
            QRgb *outLine = (QRgb*)out.scanLine(y);
            for (int x = 0; x < r.width(); ++x) {
                int sx = r.x() + x;
                int ma = qAlpha(mskLine[sx]);
                if (ma <= 0) continue;
                QRgb px = srcLine[sx];
                int sa = qAlpha(px);
                int na = (sa * ma) / 255;
                if (na <= 0) continue;
                outLine[x] = qRgba(qRed(px), qGreen(px), qBlue(px), na);
            }
        }
        return out;
    }


    // clearMaskedRegion: borra SOLO la forma de la máscara
    static void clearMaskedRegion(QImage &target, const QImage &mask) {
        if (target.isNull() || mask.isNull()) return;
        if (target.size() != mask.size()) return;
        QPainter p(&target);
        p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
        p.drawImage(0, 0, mask);
        p.end();
    }


    // featherMask: blur gaussiano real del borde
    static void featherMask(QImage &mask, int radius) {
        if (mask.isNull() || radius <= 0) return;
        if (mask.format() != QImage::Format_ARGB32) mask = mask.convertToFormat(QImage::Format_ARGB32);
        double sigma = radius / 2.0;
        if (sigma < 0.5) sigma = 0.5;
        gaussianBlurAlpha(mask, sigma);
    }


    // smoothMask: blur gaussiano con sigma explícito
    static void smoothMask(QImage &mask, double sigma) {
        if (mask.isNull() || sigma <= 0) return;
        if (mask.format() != QImage::Format_ARGB32) mask = mask.convertToFormat(QImage::Format_ARGB32);
        gaussianBlurAlpha(mask, sigma);
    }

    // ------------------------------------------------------------
    // expandMask: con distance transform (precisión sub-pixel)
    // ------------------------------------------------------------
    static QImage expandMask(const QImage &mask, int pixels) {
        if (mask.isNull() || pixels <= 0) return mask;
        QImage m = mask.copy();
        if (m.format() != QImage::Format_ARGB32) m = m.convertToFormat(QImage::Format_ARGB32);
        const int w = m.width(), h = m.height();

        QVector<bool> src(w * h);
        for (int y = 0; y < h; ++y) {
            const QRgb *line = (const QRgb*)m.constScanLine(y);
            for (int x = 0; x < w; ++x) src[y*w + x] = (qAlpha(line[x]) > 127);
        }
        QVector<double> d = computeDistanceField(src, w, h);
        double thr = (double)pixels;

        for (int y = 0; y < h; ++y) {
            QRgb *line = (QRgb*)m.scanLine(y);
            for (int x = 0; x < w; ++x) {
                int i = y*w + x;
                int orig = qAlpha(line[x]);
                int newA;
                if (d[i] <= thr - 0.5) newA = 255;
                else if (d[i] <= thr + 0.5) newA = (int)(255.0 * (thr + 0.5 - d[i]) + 0.5);
                else newA = 0;
                newA = qMax(orig, newA);
                line[x] = qRgba(255, 255, 255, newA);
            }
        }
        return m;
    }


    // contractMask: con distance transform
    static QImage contractMask(const QImage &mask, int pixels) {
        if (mask.isNull() || pixels <= 0) return mask;
        QImage m = mask.copy();
        if (m.format() != QImage::Format_ARGB32) m = m.convertToFormat(QImage::Format_ARGB32);
        const int w = m.width(), h = m.height();

        QVector<bool> src(w * h); // fuente = NO seleccionados
        for (int y = 0; y < h; ++y) {
            const QRgb *line = (const QRgb*)m.constScanLine(y);
            for (int x = 0; x < w; ++x) src[y*w + x] = (qAlpha(line[x]) <= 127);
        }
        QVector<double> d = computeDistanceField(src, w, h);
        double thr = (double)pixels;

        for (int y = 0; y < h; ++y) {
            QRgb *line = (QRgb*)m.scanLine(y);
            for (int x = 0; x < w; ++x) {
                int i = y*w + x;
                int orig = qAlpha(line[x]);
                if (orig == 0) continue;
                int newA;
                if (d[i] >= thr + 0.5) newA = orig;
                else if (d[i] >= thr - 0.5) newA = (int)(orig * (d[i] - (thr - 0.5)) + 0.5);
                else newA = 0;
                newA = qBound(0, newA, 255);
                line[x] = qRgba(255, 255, 255, newA);
            }
        }
        return m;
    }

    // sharpenMaskEdges: realza el contraste del borde de la máscara
    static QImage sharpenMaskEdges(const QImage &mask, double amount) {
        if (mask.isNull() || amount <= 0.0) return mask;
        QImage m = mask.copy();
        if (m.format() != QImage::Format_ARGB32) m = m.convertToFormat(QImage::Format_ARGB32);
        QImage blurred = m.copy();
        gaussianBlurAlpha(blurred, 1.0);

        const int w = m.width(), h = m.height();
        for (int y = 0; y < h; ++y) {
            QRgb *mline = (QRgb*)m.scanLine(y);
            const QRgb *bline = (const QRgb*)blurred.constScanLine(y);
            for (int x = 0; x < w; ++x) {
                int a  = qAlpha(mline[x]);
                int ba = qAlpha(bline[x]);
                int sharpened = (int)(a + (a - ba) * amount + 0.5);
                sharpened = qBound(0, sharpened, 255);
                mline[x] = qRgba(255, 255, 255, sharpened);
            }
        }
        return m;
    }


    static QImage refineMaskWithEdges(const QImage &mask, const QImage &source, double strength) {
        if (mask.isNull() || source.isNull()) return mask;
        if (mask.size() != source.size()) return mask;
        if (strength == 0.0) return mask;

        QImage m = mask.copy();
        if (m.format() != QImage::Format_ARGB32) m = m.convertToFormat(QImage::Format_ARGB32);
        QImage src = source;
        if (src.format() != QImage::Format_ARGB32) src = src.convertToFormat(QImage::Format_ARGB32);

        const int w = m.width(), h = m.height();
        if (w < 3 || h < 3) return m;

        QVector<double> gray(w * h);
        for (int y = 0; y < h; ++y) {
            const QRgb *sl = (const QRgb*)src.constScanLine(y);
            for (int x = 0; x < w; ++x) gray[y*w + x] = qGray(sl[x]);
        }

        QVector<double> mag(w * h, 0.0);
        for (int y = 1; y < h - 1; ++y) {
            for (int x = 1; x < w - 1; ++x) {
                int i = y*w + x;
                double gx = -gray[i-w-1] + gray[i-w+1]
                            -2.0*gray[i-1] + 2.0*gray[i+1]
                            -gray[i+w-1] + gray[i+w+1];
                double gy = -gray[i-w-1] - 2.0*gray[i-w] - gray[i-w+1]
                            + gray[i+w-1] + 2.0*gray[i+w] + gray[i+w+1];
                mag[i] = std::sqrt(gx*gx + gy*gy);
            }
        }

        const double maxMag = 1442.0;

        for (int y = 1; y < h - 1; ++y) {
            QRgb *mline = (QRgb*)m.scanLine(y);
            const uchar *maUp = m.constScanLine(y - 1);
            const uchar *maDn = m.constScanLine(y + 1);
            for (int x = 1; x < w - 1; ++x) {
                int a = qAlpha(mline[x]);
                if (a == 0) continue;
                bool border = (qAlpha(mline[x-1]) == 0) ||
                              (qAlpha(mline[x+1]) == 0) ||
                              (maUp[x*4 + 3] == 0) ||
                              (maDn[x*4 + 3] == 0);
                if (!border) continue;
                double e = mag[y*w + x] / maxMag;
                double factor = 1.0 - strength * e;
                factor = qBound(0.0, factor, 2.0);
                int newA = (int)(a * factor + 0.5);
                newA = qBound(0, newA, 255);
                mline[x] = qRgba(255, 255, 255, newA);
            }
        }
        return m;
    }


    // estimateAutoTolerance: sugiere tolerancia según varianza local
    static int estimateAutoTolerance(const QImage &image, const QPoint &pos, int radius = 5) {
        if (image.isNull()) return 32;
        if (pos.x() < 0 || pos.x() >= image.width() || pos.y() < 0 || pos.y() >= image.height()) return 32;

        QImage img = image;
        if (img.format() != QImage::Format_ARGB32) img = img.convertToFormat(QImage::Format_ARGB32);

        int x0 = qMax(0, pos.x() - radius), x1 = qMin(img.width() - 1, pos.x() + radius);
        int y0 = qMax(0, pos.y() - radius), y1 = qMin(img.height() - 1, pos.y() + radius);

        double sr = 0, sg = 0, sb = 0; int count = 0;
        for (int y = y0; y <= y1; ++y) {
            const QRgb *line = (const QRgb*)img.constScanLine(y);
            for (int x = x0; x <= x1; ++x) {
                sr += qRed(line[x]); sg += qGreen(line[x]); sb += qBlue(line[x]); count++;
            }
        }
        if (count == 0) return 32;
        double mr = sr/count, mg = sg/count, mb = sb/count;

        double var = 0;
        for (int y = y0; y <= y1; ++y) {
            const QRgb *line = (const QRgb*)img.constScanLine(y);
            for (int x = x0; x <= x1; ++x) {
                double dr = qRed(line[x]) - mr;
                double dg = qGreen(line[x]) - mg;
                double db = qBlue(line[x]) - mb;
                var += (dr*dr + dg*dg + db*db) / 3.0;
            }
        }
        var /= count;
        double stddev = std::sqrt(var);
        return qBound(8, (int)(2.0 * stddev), 128);
    }

    // visualizeMask: convierte la máscara a imagen visible (debug)
    static QImage visualizeMask(const QImage &mask) {
        if (mask.isNull()) return QImage();
        QImage m = mask;
        if (m.format() != QImage::Format_ARGB32) m = m.convertToFormat(QImage::Format_ARGB32);
        QImage out(m.size(), QImage::Format_ARGB32);
        for (int y = 0; y < m.height(); ++y) {
            const QRgb *mline = (const QRgb*)m.constScanLine(y);
            QRgb *oline = (QRgb*)out.scanLine(y);
            for (int x = 0; x < m.width(); ++x) {
                int a = qAlpha(mline[x]);
                oline[x] = qRgba(a, a, a, 255);
            }
        }
        return out;
    }

    // Wrapper compatible con la API vieja de PaintEngine
    static QImage magicWandMask(const QImage &image, const QPoint &pos, int tolerance) {
        MagicWandResult r = applyMagicWand(image, pos, tolerance, true, true);
        return r.mask;
    }

private:
    // Recalcula bounding box y contadores tras un post-proceso
    static void recomputeMaskStats(MagicWandResult &result) {
        result.pixelsSelected = 0;
        result.pixelsSoft = 0;
        result.boundingBox = QRect();
        if (result.mask.isNull()) return;

        const int w = result.mask.width(), h = result.mask.height();
        int minX = w, minY = h, maxX = 0, maxY = 0;

        for (int y = 0; y < h; ++y) {
            const QRgb *line = (const QRgb*)result.mask.constScanLine(y);
            for (int x = 0; x < w; ++x) {
                int a = qAlpha(line[x]);
                if (a <= 0) continue;
                result.pixelsSelected++;
                if (a < 255) result.pixelsSoft++;
                if (x < minX) minX = x; if (x > maxX) maxX = x;
                if (y < minY) minY = y; if (y > maxY) maxY = y;
            }
        }
        if (result.pixelsSelected > 0)
            result.boundingBox = QRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
    }


    // Muestreo del color semilla (promedio en un radio)
    static void sampleSeedColor(const QImage &img, const QPoint &pos, int radius,
                                int &r, int &g, int &b, int &a) {
        int x0 = qMax(0, pos.x() - radius), x1 = qMin(img.width() - 1, pos.x() + radius);
        int y0 = qMax(0, pos.y() - radius), y1 = qMin(img.height() - 1, pos.y() + radius);
        long long sr = 0, sg = 0, sb = 0, sa = 0; int count = 0;
        for (int y = y0; y <= y1; ++y) {
            const QRgb *line = (const QRgb*)img.constScanLine(y);
            for (int x = x0; x <= x1; ++x) {
                QRgb p = line[x];
                sr += qRed(p); sg += qGreen(p); sb += qBlue(p); sa += qAlpha(p); count++;
            }
        }
        if (count == 0) { r = g = b = a = 0; return; }
        r = (int)(sr / count); g = (int)(sg / count);
        b = (int)(sb / count); a = (int)(sa / count);
    }

    // Gaussian blur separable sobre el canal alpha
    static void gaussianBlurAlpha(QImage &img, double sigma) {
        if (img.isNull() || sigma <= 0) return;
        if (img.format() != QImage::Format_ARGB32) img = img.convertToFormat(QImage::Format_ARGB32);

        int radius = (int)std::ceil(sigma * 3.0);
        if (radius < 1) radius = 1;
        int ksize = radius * 2 + 1;
        QVector<double> kernel(ksize);
        double ksum = 0;
        for (int i = 0; i < ksize; ++i) {
            double x = i - radius;
            kernel[i] = std::exp(-(x*x) / (2.0 * sigma * sigma));
            ksum += kernel[i];
        }
        for (int i = 0; i < ksize; ++i) kernel[i] /= ksum;

        const int w = img.width(), h = img.height();
        QVector<double> src(w * h), tmp(w * h);

        for (int y = 0; y < h; ++y) {
            const uchar *line = img.constScanLine(y);
            for (int x = 0; x < w; ++x) src[y*w + x] = line[x*4 + 3];
        }
        // Horizontal
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                double acc = 0;
                for (int k = -radius; k <= radius; ++k) {
                    int xx = x + k;
                    if (xx < 0) xx = 0; if (xx >= w) xx = w - 1;
                    acc += src[y*w + xx] * kernel[k + radius];
                }
                tmp[y*w + x] = acc;
            }
        }
        // Vertical + escribir alpha
        for (int y = 0; y < h; ++y) {
            uchar *line = img.bits() + y * img.bytesPerLine();
            for (int x = 0; x < w; ++x) {
                double acc = 0;
                for (int k = -radius; k <= radius; ++k) {
                    int yy = y + k;
                    if (yy < 0) yy = 0; if (yy >= h) yy = h - 1;
                    acc += tmp[yy*w + x] * kernel[k + radius];
                }
                int va = (int)(acc + 0.5);
                va = qBound(0, va, 255);
                line[x*4 + 3] = (uchar)va;
            }
        }
    }


    static QVector<double> computeDistanceField(const QVector<bool> &zeroSet, int w, int h) {
        const double INF = 1e30;
        const double D1 = 1.0, D2 = 1.41421356237;
        QVector<double> d(w * h, INF);
        for (int i = 0; i < w * h; ++i) if (zeroSet[i]) d[i] = 0.0;

        // Forward
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int i = y*w + x;
                if (x > 0) d[i] = std::min(d[i], d[i-1] + D1);
                if (y > 0) {
                    d[i] = std::min(d[i], d[i-w] + D1);
                    if (x > 0)     d[i] = std::min(d[i], d[i-w-1] + D2);
                    if (x < w - 1) d[i] = std::min(d[i], d[i-w+1] + D2);
                }
            }
        }
        // Backward
        for (int y = h - 1; y >= 0; --y) {
            for (int x = w - 1; x >= 0; --x) {
                int i = y*w + x;
                if (x < w - 1) d[i] = std::min(d[i], d[i+1] + D1);
                if (y < h - 1) {
                    d[i] = std::min(d[i], d[i+w] + D1);
                    if (x > 0)     d[i] = std::min(d[i], d[i+w-1] + D2);
                    if (x < w - 1) d[i] = std::min(d[i], d[i+w+1] + D2);
                }
            }
        }
        return d;
    }
};

#endif 

// MAGICWANDTOOLS_H
