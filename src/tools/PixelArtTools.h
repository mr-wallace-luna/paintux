#ifndef PIXELARTTOOLS_H
#define PIXELARTTOOLS_H

#include "core/ToolManager.h"
#include "core/LayerStack.h"

#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPoint>
#include <QRandomGenerator>
#include <QPainterPath>
#include <cmath>
#include <vector>

namespace PixelArt {

inline void applyPixelLighten(QImage &image, const QPoint &pos) {
    QColor currentPx = image.pixelColor(pos);
    int r = qMin(255, currentPx.red() + 30);
    int g = qMin(255, currentPx.green() + 30);
    int b = qMin(255, currentPx.blue() + 30);
    image.setPixelColor(pos, QColor(r, g, b, currentPx.alpha()));
}

inline void drawPixel(QImage &image, const QPoint &pos, const QColor &color,
                      ToolType tool, bool isMirror = false) {
    if (tool == ToolType::Lighten) {
        applyPixelLighten(image, pos);
        return;
    }
    image.setPixelColor(pos, color);
    if (isMirror && tool == ToolType::MirrorPen) {
        image.setPixelColor(image.width() - 1 - pos.x(), pos.y(), color);
    }
}

inline void drawLine(QImage &image, const QPoint &p1, const QPoint &p2,
                     const QColor &color, ToolType tool, bool isMirror = false) {
    int x1 = p1.x(), y1 = p1.y();
    int x2 = p2.x(), y2 = p2.y();
    int dx = abs(x2 - x1);
    int dy = -abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;
    while (true) {
        drawPixel(image, QPoint(x1, y1), color, tool, isMirror);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

inline void drawShape(QImage &image, const QPoint &p1, const QPoint &p2,
                      const QColor &color, ToolType currentTool) {
    QRect r = QRect(p1, p2).normalized();
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(QPen(color, 1));
    switch (currentTool) {
        case ToolType::PixelStroke:
            drawLine(image, p1, p2, color, ToolType::PixelStroke);
            break;
        case ToolType::Line: painter.drawLine(p1, p2); break;
        case ToolType::Rectangle: painter.drawRect(r); break;
        case ToolType::Ellipse: painter.drawEllipse(r); break;
        case ToolType::RoundRect: painter.drawRoundedRect(r, 12, 12); break;
        case ToolType::Triangle: {
            QPolygon t;
            t << QPoint((p1.x()+p2.x())/2, p1.y())
              << QPoint(p1.x(), p2.y())
              << QPoint(p2.x(), p2.y());
            painter.drawPolygon(t);
            break;
        }
        case ToolType::RightTriangle: {
            QPolygon t;
            t << p1 << QPoint(p1.x(), p2.y()) << p2;
            painter.drawPolygon(t);
            break;
        }
        case ToolType::Diamond: {
            QPolygon t;
            t << QPoint((p1.x()+p2.x())/2, p1.y())
              << QPoint(p2.x(), (p1.y()+p2.y())/2)
              << QPoint((p1.x()+p2.x())/2, p2.y())
              << QPoint(p1.x(), (p1.y()+p2.y())/2);
            painter.drawPolygon(t);
            break;
        }
        case ToolType::Pentagon: case ToolType::Hexagon: case ToolType::Star: {
            int sides = (currentTool == ToolType::Pentagon) ? 5
                      : (currentTool == ToolType::Hexagon) ? 6 : 10;
            QPolygon poly;
            for (int i = 0; i < sides; ++i) {
                double angle = -M_PI/2 + i * 2 * M_PI / (currentTool == ToolType::Star ? 5 : sides);
                double f = (currentTool == ToolType::Star && i % 2 == 1) ? 0.45 : 1.0;
                poly << QPoint(r.center().x() + r.width()/2 * f * cos(angle),
                               r.center().y() + r.height()/2 * f * sin(angle));
            }
            painter.drawPolygon(poly);
            break;
        }
        case ToolType::ArrowRight: case ToolType::ArrowLeft: {
            bool right = (currentTool == ToolType::ArrowRight);
            int ym = r.top() + r.height()/2;
            int xb = right ? r.left() + r.width()*0.55
                           : r.left() + r.width()*0.45;
            int tk = r.height()*0.25;
            QPolygon poly;
            poly << QPoint(right ? r.left() : r.right(), ym - tk)
                 << QPoint(xb, ym - tk) << QPoint(xb, r.top())
                 << QPoint(right ? r.right() : r.left(), ym)
                 << QPoint(xb, r.bottom()) << QPoint(xb, ym + tk)
                 << QPoint(right ? r.left() : r.right(), ym + tk);
            painter.drawPolygon(poly);
            break;
        }
        case ToolType::Heart: {
            QPainterPath path;
            path.moveTo(r.left()+r.width()/2, r.top()+r.height()*0.28);
            path.cubicTo(r.left()+r.width()*0.1, r.top()-r.height()*0.05,
                         r.left(), r.top()+r.height()*0.6,
                         r.left()+r.width()/2, r.bottom());
            path.cubicTo(r.right(), r.top()+r.height()*0.6,
                         r.right()-r.width()*0.1, r.top()-r.height()*0.05,
                         r.left()+r.width()/2, r.top()+r.height()*0.28);
            painter.drawPath(path);
            break;
        }
        case ToolType::Cube: {
            int offset = qMin(r.width(), r.height())*0.3;
            if (offset < 4) offset = 4;
            QRect front(r.left(), r.top()+offset, r.width()-offset, r.height()-offset);
            QRect back(r.left()+offset, r.top(), r.width()-offset, r.height()-offset);
            painter.drawRect(front);
            painter.drawRect(back);
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

inline void floodFill(QImage &image, const QPoint &start, QColor fillCol) {
    if (image.isNull()) return;
    const int w = image.width();
    const int h = image.height();
    if (start.x() < 0 || start.x() >= w || start.y() < 0 || start.y() >= h) return;

    const QRgb targetRgb = image.pixel(start);
    if (targetRgb == fillCol.rgba()) return;

    std::vector<QPoint> stack;
    stack.reserve(1024);
    stack.push_back(start);
    while (!stack.empty()) {
        const QPoint p = stack.back();
        stack.pop_back();
        const int x = p.x();
        const int y = p.y();
        if (x < 0 || x >= w || y < 0 || y >= h) continue;
        if (image.pixel(x, y) != targetRgb) continue;
        image.setPixelColor(x, y, fillCol);
        stack.push_back(QPoint(x + 1, y));
        stack.push_back(QPoint(x - 1, y));
        stack.push_back(QPoint(x, y + 1));
        stack.push_back(QPoint(x, y - 1));
    }
}

inline void spray(QImage &image, const QPoint &centro,
                  QColor color, int penWidth) {
    QPainter painter(&image);
    painter.setPen(color);
    const int radio = penWidth * 4 + 7;
    for (int i = 0; i < 15; ++i) {
        const int dx = QRandomGenerator::global()->bounded(-radio, radio);
        const int maxDy = (int)std::sqrt(qMax(0, radio*radio - dx*dx));
        const int dy = QRandomGenerator::global()->bounded(-maxDy, maxDy + 1);
        painter.drawPoint(centro.x() + dx, centro.y() + dy);
    }
    painter.end();
}

inline void drawGrid(QPainter &painter, const QImage &image,
                     int gridSize, double zoomFactor) {
    if (gridSize <= 0) return;
    painter.setPen(QPen(QColor(128, 128, 128, 180), 0.5 / zoomFactor));
    for (int x = 0; x <= image.width(); x += gridSize)
        painter.drawLine(x, 0, x, image.height());
    for (int y = 0; y <= image.height(); y += gridSize)
        painter.drawLine(0, y, image.width(), y);
}

inline void stampClone(QImage &target,
                       const QImage &source,
                       const QPoint &sourceOffset,
                       const QPoint &destPos,
                       int brushRadius) {
    if (target.isNull() || source.isNull()) return;
    if (brushRadius < 1) brushRadius = 1;

    const QPoint currentSource = destPos + sourceOffset;
    const int r2 = brushRadius * brushRadius;

    for (int y = -brushRadius; y <= brushRadius; ++y) {
        for (int x = -brushRadius; x <= brushRadius; ++x) {
            if (x*x + y*y > r2) continue;

            const QPoint srcP = currentSource + QPoint(x, y);
            const QPoint dstP = destPos + QPoint(x, y);

            if (srcP.x() < 0 || srcP.x() >= source.width()) continue;
            if (srcP.y() < 0 || srcP.y() >= source.height()) continue;
            if (dstP.x() < 0 || dstP.x() >= target.width()) continue;
            if (dstP.y() < 0 || dstP.y() >= target.height()) continue;

            const QColor srcColor = source.pixelColor(srcP);
            if (srcColor.alpha() > 0) target.setPixelColor(dstP, srcColor);
        }
    }
}

} // namespace PixelArt

#endif // PIXELARTTOOLS_H