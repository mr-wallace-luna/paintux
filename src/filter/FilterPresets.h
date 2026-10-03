#ifndef FILTER_PRESETS_H
#define FILTER_PRESETS_H

#include <QString>
#include <QVector>
#include <QPointF>
#include <QColor>
#include <QtGlobal>
#include <cmath>

#include "ImageFilters.h"

// ============================================================
// FilterPresets
// ------------------------------------------------------------
// Catálogo de presets artísticos. Header-only.
//
// Cada preset es una combinación de:
//   - Ajustes de color (brillo, contraste, saturación, temperatura)
//   - Zonas (sombras, luces) con color propio
//   - HSL por canal
//   - Curvas (spline monotónico)
//   - Efectos (sharpen, blur, pixelate, halftone, colorize)
// ============================================================
namespace FilterPresets {

struct Entry {
    int id;
    QString name;
    QString description;
};

inline QVector<Entry> all() {
    QVector<Entry> list;
    list.reserve(14);

    list.append({ 0,  QStringLiteral("Sin filtro"),
                      QStringLiteral("Sin ajustes") });
    list.append({ 1,  QStringLiteral("MMADRO"),
                      QStringLiteral("Teal & orange cinematográfico") });
    list.append({ 2,  QStringLiteral("Mordor"),
                      QStringLiteral("Cálido intenso, sombras profundas") });
    list.append({ 3,  QStringLiteral("POP"),
                      QStringLiteral("Halftone retro, colores punchy") });
    list.append({ 4,  QStringLiteral("Y2K"),
                      QStringLiteral("Rosa/cian brillante, era 2000") });
    list.append({ 5,  QStringLiteral("Chernóbil"),
                      QStringLiteral("Verde radiactivo, alto contraste") });
    list.append({ 6,  QStringLiteral("Pixel Art"),
                      QStringLiteral("Pixelado + paleta escalonada") });
    list.append({ 7,  QStringLiteral("Vaporwave"),
                      QStringLiteral("Púrpura/rosa synthwave") });
    list.append({ 8,  QStringLiteral("Noir"),
                      QStringLiteral("Blanco y negro, alto contraste") });
    list.append({ 9,  QStringLiteral("Kodak 80s"),
                      QStringLiteral("Cálido nostálgico, grano suave") });
    list.append({ 10, QStringLiteral("Ártico"),
                      QStringLiteral("Frío azulado, limpio") });
    list.append({ 11, QStringLiteral("Pastel Dream"),
                      QStringLiteral("Suave, luminoso, bajo contraste") });
    list.append({ 12, QStringLiteral("Neón Nocturno"),
                      QStringLiteral("Saturado, sombras densas") });
    list.append({ 13, QStringLiteral("NoMéxico IR"),
                      QStringLiteral("Infrarrojo falso, tonos rojizos") });

    return list;
}

inline FilterParams get(int id) {
    FilterParams fp;
    switch (id) {
    case PresetMMADRO:
        fp.brightness = 0.05; fp.contrast = 0.18; fp.saturation = 0.30; fp.exposure = 0.06;
        fp.shadows = 0.18; fp.highlights = 0.12; fp.temperature = 0.30; fp.vignette = 0.18;
        fp.shadowColor    = QColor(36, 58, 88);
        fp.highlightColor = QColor(255, 214, 130);
        fp.rawTemp = 28; fp.rawTint = -18; fp.rawVibrance = 50; fp.rawClarity = 12;
        fp.rawBlacks = -6; fp.rawWhites = 10; fp.rawGamma = 104;
        fp.sharpen = true; fp.sharpenStrength = 35;
        fp.hsl[1].hue = -8;  fp.hsl[1].saturation = 18;
        fp.hsl[3].hue = 6;   fp.hsl[3].saturation = -22;
        fp.hsl[5].saturation = 10;
        fp.curves[0] = QVector<QPointF>{ QPointF(0,4), QPointF(64,72), QPointF(192,206), QPointF(255,251) };
        fp.curves[1] = QVector<QPointF>{ QPointF(0,8), QPointF(128,142), QPointF(255,255) };
        fp.curves[2] = QVector<QPointF>{ QPointF(0,6), QPointF(128,138), QPointF(255,250) };
        fp.curves[3] = QVector<QPointF>{ QPointF(0,14), QPointF(128,108), QPointF(255,222) };
        break;
    case PresetMordor:
        fp.brightness = -0.04; fp.contrast = 0.32; fp.saturation = -0.18; fp.exposure = 0.06;
        fp.shadows = 0.22; fp.highlights = -0.12; fp.temperature = 0.42; fp.vignette = 0.38;
        fp.shadowColor   = QColor(70, 32, 8);
        fp.highlightColor= QColor(255, 186, 110);
        fp.rawTemp = 38; fp.rawTint = 10; fp.rawVibrance = 32; fp.rawClarity = 30;
        fp.rawBlacks = -18; fp.rawWhites = 12; fp.rawGamma = 94;
        fp.sharpen = true; fp.sharpenStrength = 45;
        fp.curves[0] = QVector<QPointF>{ QPointF(0,0), QPointF(64,46), QPointF(192,216), QPointF(255,255) };
        fp.curves[1] = QVector<QPointF>{ QPointF(0,8), QPointF(128,148), QPointF(255,255) };
        fp.curves[2] = QVector<QPointF>{ QPointF(0,2), QPointF(128,122), QPointF(255,246) };
        fp.curves[3] = QVector<QPointF>{ QPointF(0,0), QPointF(64,34), QPointF(128,86), QPointF(192,152), QPointF(255,214) };
        break;
    case PresetPop:
        fp.halftone = true; fp.halftoneCell = 6;
        fp.brightness = 0.06; fp.contrast = 0.34; fp.saturation = 0.45; fp.exposure = 0.04;
        fp.shadows = 0.10; fp.highlights = 0.10; fp.temperature = 0.10; fp.vignette = 0.12;
        fp.shadowColor    = QColor(60, 20, 10);
        fp.highlightColor = QColor(255, 250, 230);
        fp.rawTemp = 12; fp.rawTint = 8; fp.rawVibrance = 60; fp.rawClarity = 30;
        fp.rawBlacks = -12; fp.rawWhites = 14; fp.rawGamma = 96;
        fp.curves[0] = QVector<QPointF>{ QPointF(0,0), QPointF(70,44), QPointF(185,222), QPointF(255,255) };
        fp.curves[1] = QVector<QPointF>{ QPointF(0,6), QPointF(128,140), QPointF(255,255) };
        break;
    case PresetY2K:
        fp.brightness = 0.08; fp.contrast = 0.20; fp.saturation = 0.34; fp.exposure = 0.10;
        fp.shadows = 0.18; fp.highlights = 0.22; fp.temperature = -0.14; fp.vignette = 0.10;
        fp.shadowColor   = QColor(255, 64, 190);
        fp.highlightColor= QColor(130, 240, 255);
        fp.rawTemp = -14; fp.rawTint = 20; fp.rawVibrance = 58; fp.rawClarity = 16;
        fp.rawBlacks = 10; fp.rawWhites = 20; fp.rawGamma = 116;
        fp.sharpen = true; fp.sharpenStrength = 62;
        fp.curves[0] = QVector<QPointF>{ QPointF(0,10), QPointF(64,78), QPointF(192,208), QPointF(255,252) };
        fp.curves[1] = QVector<QPointF>{ QPointF(0,18), QPointF(128,138), QPointF(255,248) };
        fp.curves[2] = QVector<QPointF>{ QPointF(0,8),  QPointF(128,132), QPointF(255,255) };
        fp.curves[3] = QVector<QPointF>{ QPointF(0,26), QPointF(128,142), QPointF(255,250) };
        break;
    case PresetChernobil:
        fp.brightness = -0.08; fp.contrast = 0.30; fp.saturation = -0.55; fp.exposure = -0.04;
        fp.shadows = 0.28; fp.highlights = -0.22; fp.temperature = -0.22; fp.vignette = 0.55;
        fp.shadowColor   = QColor(18, 28, 10);
        fp.highlightColor= QColor(178, 214, 110);
        fp.rawTemp = -26; fp.rawTint = -32; fp.rawVibrance = 8; fp.rawClarity = 46;
        fp.rawBlacks = -26; fp.rawWhites = -14; fp.rawGamma = 86;
        fp.sharpen = true; fp.sharpenStrength = 72;
        fp.curves[0] = QVector<QPointF>{ QPointF(0,0), QPointF(64,50), QPointF(128,118), QPointF(192,176), QPointF(255,232) };
        fp.curves[1] = QVector<QPointF>{ QPointF(0,0), QPointF(128,108), QPointF(255,222) };
        fp.curves[2] = QVector<QPointF>{ QPointF(0,14), QPointF(128,144), QPointF(255,242) };
        fp.curves[3] = QVector<QPointF>{ QPointF(0,0), QPointF(128,102), QPointF(255,206) };
        break;
    case PresetPixelArt:
        fp.pixelate = true; fp.pixelateSize = 10;
        fp.brightness = 0.02; fp.contrast = 0.22; fp.saturation = 0.30; fp.exposure = 0.0;
        fp.rawVibrance = 45; fp.rawClarity = 25;
        fp.rawBlacks = -8; fp.rawWhites = 8; fp.rawGamma = 104;
        fp.sharpen = true; fp.sharpenStrength = 60;
        fp.curves[0] = QVector<QPointF>{
            QPointF(0,20),   QPointF(32,20),  QPointF(43,66),
            QPointF(75,66),  QPointF(86,112), QPointF(118,112),
            QPointF(129,158),QPointF(161,158),QPointF(172,204),
            QPointF(204,204),QPointF(215,250),QPointF(255,250)
        };
        break;
    case PresetVaporwave:
        fp.brightness = 0.05; fp.contrast = 0.22; fp.saturation = 0.45; fp.exposure = 0.06;
        fp.shadows = 0.25; fp.highlights = 0.18; fp.temperature = -0.18; fp.vignette = 0.22;
        fp.shadowColor   = QColor(120, 30, 200);
        fp.highlightColor= QColor(255, 120, 220);
        fp.rawTemp = -18; fp.rawTint = 34; fp.rawVibrance = 62; fp.rawClarity = 12;
        fp.rawBlacks = 6; fp.rawWhites = 16; fp.rawGamma = 118;
        fp.sharpen = true; fp.sharpenStrength = 40;
        fp.hsl[4].saturation = 25;
        fp.hsl[6].hue = 10; fp.hsl[6].saturation = 20;
        fp.curves[0] = QVector<QPointF>{ QPointF(0,6),  QPointF(128,136), QPointF(255,250) };
        fp.curves[1] = QVector<QPointF>{ QPointF(0,14), QPointF(128,142), QPointF(255,252) };
        fp.curves[2] = QVector<QPointF>{ QPointF(0,4),  QPointF(128,124), QPointF(255,250) };
        fp.curves[3] = QVector<QPointF>{ QPointF(0,24), QPointF(128,146), QPointF(255,248) };
        break;
    case PresetNoir:
        fp.grayscale = true;
        fp.brightness = -0.04; fp.contrast = 0.46; fp.saturation = -1.0; fp.exposure = 0.0;
        fp.shadows = 0.15; fp.highlights = -0.15; fp.vignette = 0.48;
        fp.rawClarity = 42; fp.rawBlacks = -32; fp.rawWhites = 22; fp.rawGamma = 92;
        fp.sharpen = true; fp.sharpenStrength = 58;
        fp.curves[0] = QVector<QPointF>{ QPointF(0,0), QPointF(48,26), QPointF(128,124), QPointF(208,224), QPointF(255,255) };
        break;
    case PresetKodak80:
        fp.brightness = 0.06; fp.contrast = 0.14; fp.saturation = 0.20; fp.exposure = 0.05;
        fp.shadows = 0.20; fp.highlights = 0.10; fp.temperature = 0.26; fp.vignette = 0.20;
        fp.shadowColor   = QColor(90, 60, 30);
        fp.highlightColor= QColor(255, 226, 170);
        fp.rawTemp = 26; fp.rawTint = 8; fp.rawVibrance = 28; fp.rawClarity = 10;
        fp.rawBlacks = 12; fp.rawWhites = 8; fp.rawGamma = 112;
        fp.blur = true; fp.blurRadius = 2;
        fp.sharpen = true; fp.sharpenStrength = 30;
        fp.curves[0] = QVector<QPointF>{ QPointF(0,12), QPointF(128,138), QPointF(255,248) };
        fp.curves[1] = QVector<QPointF>{ QPointF(0,14), QPointF(128,140), QPointF(255,250) };
        fp.curves[2] = QVector<QPointF>{ QPointF(0,10), QPointF(128,132), QPointF(255,248) };
        fp.curves[3] = QVector<QPointF>{ QPointF(0,6),  QPointF(128,124), QPointF(255,242) };
        break;
    case PresetArctic:
        fp.brightness = 0.10; fp.contrast = 0.18; fp.saturation = -0.12; fp.exposure = 0.06;
        fp.shadows = 0.10; fp.highlights = 0.15; fp.temperature = -0.55; fp.vignette = 0.14;
        fp.shadowColor   = QColor(20, 50, 90);
        fp.highlightColor= QColor(220, 245, 255);
        fp.rawTemp = -52; fp.rawTint = -8; fp.rawVibrance = 18; fp.rawClarity = 26;
        fp.rawBlacks = 4; fp.rawWhites = 18; fp.rawGamma = 110;
        fp.sharpen = true; fp.sharpenStrength = 45;
        fp.hsl[5].hue = -6; fp.hsl[5].saturation = 15;
        fp.curves[0] = QVector<QPointF>{ QPointF(0,4),  QPointF(128,132), QPointF(255,252) };
        fp.curves[3] = QVector<QPointF>{ QPointF(0,16), QPointF(128,146), QPointF(255,255) };
        break;
    case PresetPastel:
        fp.brightness = 0.14; fp.contrast = -0.22; fp.saturation = -0.15; fp.exposure = 0.10;
        fp.shadows = 0.30; fp.highlights = 0.20; fp.temperature = 0.05; fp.vignette = 0.0;
        fp.shadowColor   = QColor(210, 190, 235);
        fp.highlightColor= QColor(255, 245, 240);
        fp.rawTemp = 6; fp.rawTint = 12; fp.rawVibrance = 55; fp.rawClarity = -25;
        fp.rawBlacks = 25; fp.rawWhites = 10; fp.rawGamma = 125;
        fp.blur = true; fp.blurRadius = 1;
        fp.curves[0] = QVector<QPointF>{ QPointF(0,32), QPointF(64,96), QPointF(192,222), QPointF(255,250) };
        fp.curves[1] = QVector<QPointF>{ QPointF(0,34), QPointF(128,140), QPointF(255,250) };
        fp.curves[2] = QVector<QPointF>{ QPointF(0,30), QPointF(128,136), QPointF(255,248) };
        fp.curves[3] = QVector<QPointF>{ QPointF(0,38), QPointF(128,146), QPointF(255,252) };
        break;
    case PresetNeon:
        fp.brightness = -0.14; fp.contrast = 0.42; fp.saturation = 0.55; fp.exposure = -0.05;
        fp.shadows = 0.18; fp.highlights = -0.10; fp.temperature = -0.12; fp.vignette = 0.52;
        fp.shadowColor   = QColor(20, 10, 70);
        fp.highlightColor= QColor(255, 60, 200);
        fp.rawTemp = -10; fp.rawTint = 26; fp.rawVibrance = 70; fp.rawClarity = 40;
        fp.rawBlacks = -34; fp.rawWhites = 16; fp.rawGamma = 92;
        fp.sharpen = true; fp.sharpenStrength = 66;
        fp.hsl[6].saturation = 25;
        fp.hsl[4].saturation = 20;
        fp.curves[0] = QVector<QPointF>{ QPointF(0,0), QPointF(64,40), QPointF(192,226), QPointF(255,255) };
        fp.curves[1] = QVector<QPointF>{ QPointF(0,6), QPointF(128,134), QPointF(255,255) };
        fp.curves[3] = QVector<QPointF>{ QPointF(0,18), QPointF(128,140), QPointF(255,252) };
        break;
    case PresetNoMexico:
        fp.brightness = 0.04; fp.contrast = 0.26; fp.saturation = 0.35; fp.exposure = 0.05;
        fp.temperature = 0.55; fp.vignette = 0.18;
        fp.shadowColor   = QColor(40, 20, 60);
        fp.highlightColor= QColor(255, 240, 220);
        fp.rawTemp = 55; fp.rawTint = 20; fp.rawVibrance = 40; fp.rawClarity = 25;
        fp.rawBlacks = -10; fp.rawWhites = 15; fp.rawGamma = 100;
        fp.sharpen = true; fp.sharpenStrength = 50;
        fp.hsl[3].hue = -65; fp.hsl[3].saturation = 30; fp.hsl[3].lightness = 10;
        fp.curves[1] = QVector<QPointF>{ QPointF(0,22), QPointF(64,110), QPointF(128,186), QPointF(255,255) };
        fp.curves[2] = QVector<QPointF>{ QPointF(0,10), QPointF(128,150), QPointF(255,244) };
        fp.curves[3] = QVector<QPointF>{ QPointF(0,0),  QPointF(128,58),  QPointF(255,138) };
        break;
    case PresetNone:
    default:
        return FilterParams();
    }
    return fp;
}

inline FilterParams blend(const FilterParams &b, double k) {
    k = qBound(0.0, k, 1.0);
    if (k >= 0.9999) return b;
    if (k <= 0.0001) return FilterParams();
    FilterParams fp;
    fp.brightness  = b.brightness  * k;
    fp.contrast    = b.contrast    * k;
    fp.saturation  = b.saturation  * k;
    fp.exposure    = b.exposure    * k;
    fp.shadows     = b.shadows     * k;
    fp.highlights  = b.highlights  * k;
    fp.temperature = b.temperature * k;
    fp.vignette    = b.vignette    * k;
    fp.shadowColor    = b.shadowColor;
    fp.highlightColor = b.highlightColor;
    fp.invertColors = b.invertColors && (k > 0.50);
    fp.grayscale    = b.grayscale    && (k > 0.35);
    fp.sepia        = b.sepia        && (k > 0.35);
    fp.blur         = b.blur         && (k > 0.05);
    fp.blurRadius   = qBound(1, qRound(b.blurRadius * k), 20);
    fp.sharpen      = b.sharpen      && (k > 0.05);
    fp.sharpenStrength = qBound(10, qRound(b.sharpenStrength * k), 100);
    fp.pixelate     = b.pixelate     && (k > 0.05);
    fp.pixelateSize = qBound(2, qRound(b.pixelateSize * k), 64);
    fp.halftone     = b.halftone     && (k > 0.05);
    fp.halftoneCell = qBound(2, qRound(b.halftoneCell * k), 40);
    fp.colorize         = b.colorize && (k > 0.05);
    fp.colorizeStrength = b.colorizeStrength * k;
    fp.rawTemp     = b.rawTemp     * k;
    fp.rawTint     = b.rawTint     * k;
    fp.rawVibrance = b.rawVibrance * k;
    fp.rawClarity  = b.rawClarity  * k;
    fp.rawBlacks   = b.rawBlacks   * k;
    fp.rawWhites   = b.rawWhites   * k;
    fp.rawGamma    = 100.0 + (b.rawGamma - 100.0) * k;
    for (int i = 0; i < 7; ++i) {
        fp.hsl[i].hue        = b.hsl[i].hue        * k;
        fp.hsl[i].saturation = b.hsl[i].saturation * k;
        fp.hsl[i].lightness  = b.hsl[i].lightness  * k;
    }
    fp.colorizeHSL = b.colorizeHSL && (k > 0.50);
    for (int i = 0; i < 4; ++i) {
        QVector<QPointF> out;
        const QVector<QPointF> &src = b.curves[i];
        if (src.isEmpty()) continue;
        for (const QPointF &p : src)
            out << QPointF(p.x(), p.x() + (p.y() - p.x()) * k);
        fp.curves[i] = out;
    }
    return fp;
}

} // namespace FilterPresets

#endif // FILTER_PRESETS_H
