#ifndef PIXELARTTOOLS_H
#define PIXELARTTOOLS_H

#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPoint>
#include <QRandomGenerator>
#include <QPainterPath>
#include <QFile>
#include <cmath>
#include <vector>
#include "core/LayerStack.h"   // ← struct Layer ahora vive en core/

// --- ENUM DE HERRAMIENTAS ---
enum ToolType {
    ToolPencil,
    ToolEraser,
    ToolBucket,
    ToolPicker,
    ToolSpray,
    ToolText,
    ToolSelect,
    ToolSelectFree,
    ToolLine,
    ToolRectangle,
    ToolEllipse,
    ToolRoundRect,
    ToolTriangle,
    ToolRightTriangle,
    ToolDiamond,
    ToolPentagon,
    ToolHexagon,
    ToolArrowRight,
    ToolArrowLeft,
    ToolStar,
    ToolHeart,
    ToolZoom,
    ToolBrush,
    ToolCustomBrush,
    ToolCrayon,
    ToolMarker,
    ToolCube,
    ToolWatercolor,
    ToolOilBrush,
    ToolCalligraphy,
    ToolHighlighter,
    // Herramientas avanzadas
    ToolMagicWand,
    ToolLassoExtract,
    ToolLassoDelete,
    ToolPenBezier,
    ToolNodeEdit,
    ToolBlur,
    ToolHeal,
    // Herramientas Exclusivas Pixel Art
    ToolMirrorPen,
    ToolPixelStroke,
    ToolLighten,
    // Lapices de grafito por grado
    ToolPencil3B,
    ToolPencil4B,
    // ✅ NUEVAS HERRAMIENTAS (Fila 3 de Selección y Retoque)
    ToolGradient,   // Gradientes (lineal/radial/cónico)
    ToolClone,      // Clonar estilo GIMP (Alt+clic = fuente, pintar = duplica)
    ToolMove        // Mover capa / selección / texto / imagen
};

// --- RESOLVEDOR DE RUTAS PARA ASSETS ---
inline QString resolveAssetPath(const QString &fileName) {
    if (QFile::exists("assets/" + fileName)) return "assets/" + fileName;
    if (QFile::exists("../assets/" + fileName)) return "../assets/" + fileName;
    return fileName;
}

// --- FUNCIONES DE PIXEL ART ---
inline void applyPixelLighten(QImage &image, const QPoint &pos) {
    QColor currentPx = image.pixelColor(pos);
    int r = qMin(255, currentPx.red() + 30);
    int g = qMin(255, currentPx.green() + 30);
    int b = qMin(255, currentPx.blue() + 30);
    image.setPixelColor(pos, QColor(r, g, b, currentPx.alpha()));
}

inline void drawPixelArtPixel(QImage &image, const QPoint &pos, const QColor &color, ToolType tool, bool isMirror = false) {
    if (tool == ToolLighten) {
        applyPixelLighten(image, pos);
    } else {
        image.setPixelColor(pos, color);
        if (isMirror && tool == ToolMirrorPen) {
            image.setPixelColor(image.width() - 1 - pos.x(), pos.y(), color);
        }
    }
}

inline void drawPixelArtLine(QImage &image, const QPoint &p1, const QPoint &p2, const QColor &color, ToolType tool, bool isMirror = false) {
    int x1 = p1.x(), y1 = p1.y();
    int x2 = p2.x(), y2 = p2.y();
    int dx = abs(x2 - x1);
    int dy = -abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;
    while (true) {
        drawPixelArtPixel(image, QPoint(x1, y1), color, tool, isMirror);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

inline void drawPixelArtShape(QImage &image, const QPoint &p1, const QPoint &p2, const QColor &color, ToolType currentTool) {
    QRect r = QRect(p1, p2).normalized();
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(QPen(color, 1));
    switch (currentTool) {
        case ToolPixelStroke:
            drawPixelArtLine(image, p1, p2, color, ToolPixelStroke);
            break;
        case ToolLine: painter.drawLine(p1, p2); break;
        case ToolRectangle: painter.drawRect(r); break;
        case ToolEllipse: painter.drawEllipse(r); break;
        case ToolRoundRect: painter.drawRoundedRect(r, 12, 12); break;
        case ToolTriangle: {
            QPolygon t;
            t << QPoint((p1.x()+p2.x())/2, p1.y()) << QPoint(p1.x(), p2.y()) << QPoint(p2.x(), p2.y());
            painter.drawPolygon(t);
            break;
        }
        case ToolRightTriangle: {
            QPolygon t;
            t << p1 << QPoint(p1.x(), p2.y()) << p2;
            painter.drawPolygon(t);
            break;
        }
        case ToolDiamond: {
            QPolygon t;
            t << QPoint((p1.x()+p2.x())/2, p1.y()) << QPoint(p2.x(), (p1.y()+p2.y())/2)
              << QPoint((p1.x()+p2.x())/2, p2.y()) << QPoint(p1.x(), (p1.y()+p2.y())/2);
            painter.drawPolygon(t);
            break;
        }
        case ToolPentagon: case ToolHexagon: case ToolStar: {
            int sides = (currentTool == ToolPentagon) ? 5 : (currentTool == ToolHexagon) ? 6 : 10;
            QPolygon poly;
            for (int i = 0; i < sides; ++i) {
                double angle = -M_PI/2 + i * 2 * M_PI / (currentTool == ToolStar ? 5 : sides);
                double f = (currentTool == ToolStar && i % 2 == 1) ? 0.45 : 1.0;
                poly << QPoint(r.center().x() + r.width()/2 * f * cos(angle),
                               r.center().y() + r.height()/2 * f * sin(angle));
            }
            painter.drawPolygon(poly);
            break;
        }
        case ToolArrowRight: case ToolArrowLeft: {
            bool right = (currentTool == ToolArrowRight);
            int ym = r.top() + r.height()/2,
                xb = right ? r.left() + r.width()*0.55 : r.left() + r.width()*0.45,
                tk = r.height()*0.25;
            QPolygon poly;
            poly << QPoint(right ? r.left() : r.right(), ym - tk)
                 << QPoint(xb, ym - tk) << QPoint(xb, r.top())
                 << QPoint(right ? r.right() : r.left(), ym)
                 << QPoint(xb, r.bottom()) << QPoint(xb, ym + tk)
                 << QPoint(right ? r.left() : r.right(), ym + tk);
            painter.drawPolygon(poly);
            break;
        }
        case ToolHeart: {
            QPainterPath path;
            path.moveTo(r.left()+r.width()/2, r.top()+r.height()*0.28);
            path.cubicTo(r.left()+r.width()*0.1, r.top()-r.height()*0.05,
                         r.left(), r.top()+r.height()*0.6, r.left()+r.width()/2, r.bottom());
            path.cubicTo(r.right(), r.top()+r.height()*0.6,
                         r.right()-r.width()*0.1, r.top()-r.height()*0.05,
                         r.left()+r.width()/2, r.top()+r.height()*0.28);
            painter.drawPath(path);
            break;
        }
        case ToolCube: {
            int offset = qMin(r.width(), r.height())*0.3;
            if (offset < 4) offset = 4;
            QRect front(r.left(), r.top()+offset, r.width()-offset, r.height()-offset),
                  back(r.left()+offset, r.top(), r.width()-offset, r.height()-offset);
            painter.drawRect(front); painter.drawRect(back);
            painter.drawLine(front.topLeft(), back.topLeft());
            painter.drawLine(front.topRight(), back.topRight());
            painter.drawLine(front.bottomLeft(), back.bottomLeft());
            painter.drawLine(front.bottomRight(), back.bottomRight());
            break;
        }
        default: break;
    }
    painter.end();
}

inline void recomponerImagenCompleta(QImage &image, QList<Layer> &layers) {
    if (layers.isEmpty()) {
        image = QImage(800, 600, QImage::Format_ARGB32);
        image.fill(Qt::white);
        return;
    }
    QSize baseSize = layers[0].image.size();
    image = QImage(baseSize, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    for (const Layer &layer : layers) {
        if (!layer.visible) continue;
        painter.setOpacity(layer.opacity);
        switch (layer.blendMode) {
            case 0: painter.setCompositionMode(QPainter::CompositionMode_SourceOver); break;
            case 1: painter.setCompositionMode(QPainter::CompositionMode_Multiply); break;
            case 2: painter.setCompositionMode(QPainter::CompositionMode_Screen); break;
            case 3: painter.setCompositionMode(QPainter::CompositionMode_Overlay); break;
            case 4: painter.setCompositionMode(QPainter::CompositionMode_SoftLight); break;
            case 5: painter.setCompositionMode(QPainter::CompositionMode_Difference); break;
        }
        painter.drawImage(0, 0, layer.image);
    }
    painter.end();
}

inline void floodFillPixelArt(QImage &image, const QPoint &start, QColor fillCol) {
    QRgb targetRgb = image.pixel(start);
    if (targetRgb == fillCol.rgba()) return;
    std::vector<QPoint> stack;
    stack.push_back(start);
    int w = image.width(), h = image.height();
    while (!stack.empty()) {
        QPoint p = stack.back();
        stack.pop_back();
        int x = p.x(), y = p.y();
        if (x < 0 || x >= w || y < 0 || y >= h || image.pixel(x, y) != targetRgb)
            continue;
        image.setPixelColor(x, y, fillCol);
        stack.push_back(QPoint(x + 1, y));
        stack.push_back(QPoint(x - 1, y));
        stack.push_back(QPoint(x, y + 1));
        stack.push_back(QPoint(x, y - 1));
    }
}

inline void ejecutarEfectoSprayPixelArt(QImage &image, const QPoint &centro, QColor color, int penWidth) {
    QPainter painter(&image);
    painter.setPen(color);
    int radio = penWidth * 4 + 7;
    for (int i = 0; i < 15; ++i) {
        int dx = QRandomGenerator::global()->bounded(-radio, radio);
        int maxDy = (int)std::sqrt(qMax(0, radio*radio - dx*dx));
        int dy = QRandomGenerator::global()->bounded(-maxDy, maxDy + 1);
        painter.drawPoint(centro.x() + dx, centro.y() + dy);
    }
    painter.end();
}

inline void drawPixelArtGrid(QPainter &painter, const QImage &image, int gridSize, double zoomFactor) {
    if (gridSize <= 0) return;
    painter.setPen(QPen(QColor(128, 128, 128, 180), 0.5 / zoomFactor));
    for (int x = 0; x <= image.width(); x += gridSize)
        painter.drawLine(x, 0, x, image.height());
    for (int y = 0; y <= image.height(); y += gridSize)
        painter.drawLine(0, y, image.width(), y);
}

#endif // PIXELARTTOOLS_H