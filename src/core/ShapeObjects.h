#ifndef SHAPEOBJECTS_H
#define SHAPEOBJECTS_H

#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QTransform>
#include <QPolygonF>
#include <QFont>
#include <QDialog>
#include <QSlider>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QColorDialog>
#include <QFileDialog>
#include <QDir>
#include <QPen>
#include <QBrush>
#include <QCursor>
#include <QFont>
#include <cmath>

#include "tools/PixelArtTools.h"
#include "core/CustomBrushes.h"

enum class ObjectType { Shape, Text, SelectionImage };

struct PaintObject {
    ObjectType type;
    QRectF bounds;
    double rotation = 0.0;
    double scaleX = 1.0;
    double scaleY = 1.0;
    bool selected = false;
    int layerIndex = -1;

    ToolType shapeTool = ToolRectangle;
    QColor fillColor;
    QColor strokeColor;
    int strokeWidth = 1;
    QPoint startPoint, endPoint;

    QString textContent;
    QFont textFont;
    QColor textColor;

    int selectionBufferId = -1;
    bool hollow = false;

    bool isFrame = false;
    QImage frameImage;
    double frameImageScale = 1.0;
    QPointF frameImageOffset = QPointF(0,0);

    QPointF center() const { return bounds.center(); }

    QRectF transformedBounds() const {
        if (qAbs(rotation) < 0.001 && qAbs(scaleX - 1.0) < 0.001 && qAbs(scaleY - 1.0) < 0.001) return bounds;
        QTransform t;
        t.translate(bounds.center().x(), bounds.center().y());
        t.rotate(rotation);
        t.scale(scaleX, scaleY);
        t.translate(-bounds.center().x(), -bounds.center().y());
        return t.mapRect(bounds);
    }
};

enum class ObjectHandle {
    None, Move, Rotate, EditText, EditShape,
    ScaleTL, ScaleTR, ScaleBL, ScaleBR,
    ScaleT, ScaleB, ScaleL, ScaleR,
    IntegrateToCanvas
};


inline bool isScaleHandle(ObjectHandle h) {
    int v = (int)h;
    return v >= (int)ObjectHandle::ScaleTL && v <= (int)ObjectHandle::ScaleR;
}


inline Qt::CursorShape cursorForHandle(ObjectHandle h) {
    switch (h) {
        case ObjectHandle::ScaleTL:
        case ObjectHandle::ScaleBR: return Qt::SizeFDiagCursor;   // ↘
        case ObjectHandle::ScaleTR:
        case ObjectHandle::ScaleBL: return Qt::SizeBDiagCursor;   // ↙
        case ObjectHandle::ScaleT:
        case ObjectHandle::ScaleB:  return Qt::SizeVerCursor;      // ↕
        case ObjectHandle::ScaleL:
        case ObjectHandle::ScaleR:  return Qt::SizeHorCursor;      // ↔
        case ObjectHandle::Move:    return Qt::SizeAllCursor;      // ✥
        case ObjectHandle::Rotate:  return Qt::CrossCursor;        // +
        case ObjectHandle::EditText:
        case ObjectHandle::EditShape:
        case ObjectHandle::IntegrateToCanvas:
                                    return Qt::PointingHandCursor; // 👆
        default:                    return Qt::ArrowCursor;
    }
}

// ============================================================
inline QPainterPath crearPathDeGeometria(ToolType tool, const QPoint &p1, const QPoint &p2) {
    QRect r = QRect(p1, p2).normalized();
    QPainterPath path;
    switch (tool) {
        case ToolLine: path.moveTo(p1); path.lineTo(p2); break;
        case ToolRectangle: path.addRect(r); break;
        case ToolEllipse: path.addEllipse(r); break;
        case ToolRoundRect: path.addRoundedRect(r, 12, 12); break;
        case ToolTriangle:
            path.moveTo((p1.x() + p2.x()) / 2.0, p1.y());
            path.lineTo(p1.x(), p2.y()); path.lineTo(p2.x(), p2.y());
            path.closeSubpath(); break;
        case ToolRightTriangle:
            path.moveTo(p1); path.lineTo(p1.x(), p2.y()); path.lineTo(p2);
            path.closeSubpath(); break;
        case ToolDiamond:
            path.moveTo((p1.x() + p2.x()) / 2.0, p1.y());
            path.lineTo(p2.x(), (p1.y() + p2.y()) / 2.0);
            path.lineTo((p1.x() + p2.x()) / 2.0, p2.y());
            path.lineTo(p1.x(), (p1.y() + p2.y()) / 2.0);
            path.closeSubpath(); break;
        case ToolPentagon: case ToolHexagon: case ToolStar: {
            int sides = (tool == ToolPentagon) ? 5 : (tool == ToolHexagon) ? 6 : 10;
            for (int i = 0; i < sides; ++i) {
                double angle = -M_PI/2 + i * 2 * M_PI / (tool == ToolStar ? 5 : sides);
                double f = (tool == ToolStar && i % 2 == 1) ? 0.45 : 1.0;
                QPointF pt(r.center().x() + r.width()/2.0 * f * cos(angle),
                           r.center().y() + r.height()/2.0 * f * sin(angle));
                if (i == 0) path.moveTo(pt); else path.lineTo(pt);
            }
            path.closeSubpath(); break;
        }
        case ToolArrowRight: case ToolArrowLeft: {
            bool right = (tool == ToolArrowRight);
            double ym = r.top() + r.height()/2.0;
            double xb = right ? r.left() + r.width()*0.55 : r.left() + r.width()*0.45;
            double tk = r.height()*0.25;
            QPolygonF poly;
            poly << QPointF(right ? r.left() : r.right(), ym - tk)
                 << QPointF(xb, ym - tk) << QPointF(xb, r.top())
                 << QPointF(right ? r.right() : r.left(), ym)
                 << QPointF(xb, r.bottom()) << QPointF(xb, ym + tk)
                 << QPointF(right ? r.left() : r.right(), ym + tk);
            path.addPolygon(poly); path.closeSubpath(); break;
        }
        case ToolHeart:
            path.moveTo(r.left() + r.width()/2.0, r.top() + r.height()*0.28);
            path.cubicTo(r.left() + r.width()*0.1, r.top() - r.height()*0.05,
                         r.left(), r.top() + r.height()*0.6, r.left() + r.width()/2.0, r.bottom());
            path.cubicTo(r.right(), r.top() + r.height()*0.6,
                         r.right() - r.width()*0.1, r.top() - r.height()*0.05,
                         r.left() + r.width()/2.0, r.top() + r.height()*0.28);
            path.closeSubpath(); break;
        default: path.addRect(r); break;
    }
    return path;
}

inline QTransform objectTransform(const PaintObject &obj) {
    QTransform t;
    t.translate(obj.bounds.center().x(), obj.bounds.center().y());
    t.rotate(obj.rotation);
    t.scale(obj.scaleX, obj.scaleY);
    t.translate(-obj.bounds.center().x(), -obj.bounds.center().y());
    return t;
}

inline QPolygonF objectCornersCanvas(const PaintObject &obj) {
    QPolygonF poly;
    poly << obj.bounds.topLeft() << obj.bounds.topRight()
         << obj.bounds.bottomRight() << obj.bounds.bottomLeft();
    return objectTransform(obj).map(poly);
}

inline QPointF objectPointToCanvas(const PaintObject &obj, const QPointF &local) {
    return objectTransform(obj).map(local);
}

inline bool objectContainsPoint(const PaintObject &obj, const QPointF &canvasPos) {
    bool invertible = false;
    QTransform inv = objectTransform(obj).inverted(&invertible);
    if (!invertible) return obj.transformedBounds().contains(canvasPos);
    QPointF local = inv.map(canvasPos);
    return obj.bounds.contains(local);
}


struct GizmoLayout {
    QPolygonF corners;                       // TL, TR, BR, BL en coords de canvas
    QPointF midTop, midRight, midBottom, midLeft;
    QPointF rotatePos;                       // centro del tirador de rotación
    QPointF upDir;                           // dirección "arriba" del rect rotado
};

inline GizmoLayout computeGizmoLayout(const QRectF &rect, double rotationDeg, double zoomFactor) {
    GizmoLayout L;
    QTransform t;
    t.translate(rect.center().x(), rect.center().y());
    t.rotate(rotationDeg);
    t.translate(-rect.center().x(), -rect.center().y());

    QPolygonF poly;
    poly << rect.topLeft() << rect.topRight() << rect.bottomRight() << rect.bottomLeft();
    L.corners = t.map(poly);

    L.midTop    = t.map(QPointF(rect.center().x(), rect.top()));
    L.midRight  = t.map(QPointF(rect.right(), rect.center().y()));
    L.midBottom = t.map(QPointF(rect.center().x(), rect.bottom()));
    L.midLeft   = t.map(QPointF(rect.left(), rect.center().y()));

    double rad = rotationDeg * M_PI / 180.0;
    L.upDir = QPointF(sin(rad), -cos(rad));
    L.rotatePos = L.midTop + L.upDir * (20.0 / zoomFactor);
    return L;
}

inline ObjectHandle hitTestGizmo(const GizmoLayout &L, const QPointF &canvasPos, double zoomFactor) {
    double threshold = 12.0 / zoomFactor;

    if (QLineF(canvasPos, L.rotatePos).length() <= threshold)  return ObjectHandle::Rotate;

    if (QLineF(canvasPos, L.corners.at(0)).length() <= threshold) return ObjectHandle::ScaleTL;
    if (QLineF(canvasPos, L.corners.at(1)).length() <= threshold) return ObjectHandle::ScaleTR;
    if (QLineF(canvasPos, L.corners.at(2)).length() <= threshold) return ObjectHandle::ScaleBR;
    if (QLineF(canvasPos, L.corners.at(3)).length() <= threshold) return ObjectHandle::ScaleBL;

    if (QLineF(canvasPos, L.midTop).length()    <= threshold) return ObjectHandle::ScaleT;
    if (QLineF(canvasPos, L.midRight).length()  <= threshold) return ObjectHandle::ScaleR;
    if (QLineF(canvasPos, L.midBottom).length() <= threshold) return ObjectHandle::ScaleB;
    if (QLineF(canvasPos, L.midLeft).length()   <= threshold) return ObjectHandle::ScaleL;

    if (QPolygonF(L.corners).containsPoint(canvasPos, Qt::OddEvenFill)) return ObjectHandle::Move;
    return ObjectHandle::None;
}


inline QRectF resizeRotatedRect(ObjectHandle handle,
                                const QRectF &origRect, double rotationDeg,
                                const QPointF &dragStartCanvas, const QPointF &curCanvasPos,
                                bool keepAspect, double minSize = 5.0) {
    if (!isScaleHandle(handle)) return origRect;

    double rad = rotationDeg * M_PI / 180.0;
    double cosA = cos(rad), sinA = sin(rad);
    QPointF c0 = origRect.center();

    auto toLocal = [&](const QPointF &p) {
        double dx = p.x() - c0.x(), dy = p.y() - c0.y();
        return QPointF(c0.x() + dx * cosA + dy * sinA,
                       c0.y() - dx * sinA + dy * cosA);
    };
    auto toCanvas = [&](const QPointF &p) {
        double dx = p.x() - c0.x(), dy = p.y() - c0.y();
        return QPointF(c0.x() + dx * cosA - dy * sinA,
                       c0.y() + dx * sinA + dy * cosA);
    };

    QPointF sL = toLocal(dragStartCanvas);
    QPointF eL = toLocal(curCanvasPos);
    double dx = eL.x() - sL.x();
    double dy = eL.y() - sL.y();

    QRectF r = origRect;
    switch (handle) {
        case ObjectHandle::ScaleL:  r.setLeft(origRect.left() + dx); break;
        case ObjectHandle::ScaleR:  r.setRight(origRect.right() + dx); break;
        case ObjectHandle::ScaleT:  r.setTop(origRect.top() + dy); break;
        case ObjectHandle::ScaleB:  r.setBottom(origRect.bottom() + dy); break;
        case ObjectHandle::ScaleTL: r.setLeft(origRect.left() + dx); r.setTop(origRect.top() + dy); break;
        case ObjectHandle::ScaleTR: r.setRight(origRect.right() + dx); r.setTop(origRect.top() + dy); break;
        case ObjectHandle::ScaleBL: r.setLeft(origRect.left() + dx); r.setBottom(origRect.bottom() + dy); break;
        case ObjectHandle::ScaleBR: r.setRight(origRect.right() + dx); r.setBottom(origRect.bottom() + dy); break;
        default: return origRect;
    }

    // Tamaño mínimo
    if (r.width() < minSize) {
        bool fromLeft = (handle == ObjectHandle::ScaleL || handle == ObjectHandle::ScaleTL || handle == ObjectHandle::ScaleBL);
        if (fromLeft) r.setLeft(r.right() - minSize); else r.setRight(r.left() + minSize);
    }
    if (r.height() < minSize) {
        bool fromTop = (handle == ObjectHandle::ScaleT || handle == ObjectHandle::ScaleTL || handle == ObjectHandle::ScaleTR);
        if (fromTop) r.setTop(r.bottom() - minSize); else r.setBottom(r.top() + minSize);
    }

    // Mantener proporción con Shift (solo esquinas)
    bool isCorner = (handle == ObjectHandle::ScaleTL || handle == ObjectHandle::ScaleTR ||
                     handle == ObjectHandle::ScaleBL || handle == ObjectHandle::ScaleBR);
    if (keepAspect && isCorner && origRect.width() > 0.001 && origRect.height() > 0.001) {
        double aspect = origRect.width() / origRect.height();
        if (r.width() / qMax(0.001, r.height()) > aspect) {
            double newW = r.height() * aspect;
            if (handle == ObjectHandle::ScaleTL || handle == ObjectHandle::ScaleBL) r.setLeft(r.right() - newW);
            else r.setRight(r.left() + newW);
        } else {
            double newH = r.width() / aspect;
            if (handle == ObjectHandle::ScaleTL || handle == ObjectHandle::ScaleTR) r.setTop(r.bottom() - newH);
            else r.setBottom(r.top() + newH);
        }
    }

    r = r.normalized();

    // Compensación del centro: el rect final (que se dibuja rotado sobre su
    // propio centro) debe coincidir con r rotado alrededor del centro original.
    QPointF newCenterCanvas = toCanvas(r.center());
    return QRectF(newCenterCanvas.x() - r.width() / 2.0,
                  newCenterCanvas.y() - r.height() / 2.0,
                  r.width(), r.height());
}



inline void drawGizmoBase(QPainter &painter, const GizmoLayout &L,
                          double zoomFactor, bool darkMode) {
    double penW = 1.5 / zoomFactor;
    double handleSize = 8.0 / zoomFactor;

    QColor mainBlue   = darkMode ? QColor("#60a5fa") : QColor("#2563eb");
    QColor handleFill = darkMode ? QColor("#1e293b") : Qt::white;
    QColor handleBrd  = darkMode ? QColor("#60a5fa") : QColor("#2563eb");
    QColor rotateFill = darkMode ? QColor("#60a5fa") : QColor("#3b82f6");

    // Contorno
    painter.setPen(QPen(mainBlue, penW, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    painter.drawPolygon(L.corners);

    auto drawSquareHandle = [&](const QPointF &p, double s) {
        QRectF hr(p.x() - s/2, p.y() - s/2, s, s);
        painter.setPen(QPen(handleBrd, penW));
        painter.setBrush(handleFill);
        painter.drawRect(hr);
    };

    // 4 esquinas
    for (const QPointF &p : L.corners) drawSquareHandle(p, handleSize);
    // 4 lados (más pequeños)
    drawSquareHandle(L.midTop,    handleSize * 0.7);
    drawSquareHandle(L.midRight,  handleSize * 0.7);
    drawSquareHandle(L.midBottom, handleSize * 0.7);
    drawSquareHandle(L.midLeft,   handleSize * 0.7);

    // Línea + tirador de rotación
    painter.setPen(QPen(mainBlue, penW, Qt::SolidLine));
    painter.drawLine(L.midTop, L.rotatePos);
    painter.setPen(QPen(mainBlue, penW));
    painter.setBrush(rotateFill);
    painter.drawEllipse(L.rotatePos, handleSize / 2.0, handleSize / 2.0);
}


inline QRectF objectVisualRect(const PaintObject &obj) {
    double w = obj.bounds.width()  * obj.scaleX;
    double h = obj.bounds.height() * obj.scaleY;
    return QRectF(obj.bounds.center().x() - w / 2.0,
                  obj.bounds.center().y() - h / 2.0, w, h);
}


inline void drawObjectGizmo(QPainter &painter, const PaintObject &obj,
                            double zoomFactor, bool darkMode) {
    double penW = 1.5 / zoomFactor;
    double handleSize = 8.0 / zoomFactor;

    QColor mainBlue   = darkMode ? QColor("#60a5fa") : QColor("#2563eb");
    QColor labelColor = darkMode ? QColor("#e5e5e5") : QColor("#1e293b");

    QRectF visual = objectVisualRect(obj);
    GizmoLayout L = computeGizmoLayout(visual, obj.rotation, zoomFactor);

    // Base: contorno, 8 tiradores, línea + círculo de rotación
    drawGizmoBase(painter, L, zoomFactor, darkMode);

    // Texto "Rotar"
    painter.setPen(labelColor);
    QFont rotFont;
    rotFont.setPixelSize(qMax(8, (int)(9.0 / zoomFactor)));
    painter.setFont(rotFont);
    painter.drawText(L.rotatePos + QPointF(handleSize, 0), QObject::tr("Rotar"));

    QPointF prevHandle = L.rotatePos;

    // Tirador de editar TEXTO
    if (obj.type == ObjectType::Text) {
        QPointF editCanvas = L.midTop + L.upDir * (40.0 / zoomFactor);
        double editSize = handleSize * 1.3;
        QRectF editRect(editCanvas.x() - editSize/2, editCanvas.y() - editSize/2,
                        editSize, editSize);
        painter.setPen(QPen(QColor("#f59e0b"), penW));
        painter.setBrush(QColor("#fbbf24"));
        painter.drawRect(editRect);
        painter.setPen(QColor("#92400e"));
        QFont editFont; editFont.setPixelSize(qMax(8, (int)(editSize * 0.7)));
        editFont.setBold(true);
        painter.setFont(editFont);
        painter.drawText(editRect, Qt::AlignCenter, "A");
        painter.setPen(QPen(QColor("#f59e0b"), penW, Qt::SolidLine));
        painter.drawLine(L.rotatePos, editCanvas);

        painter.setPen(labelColor);
        QFont lblFont; lblFont.setPixelSize(qMax(8, (int)(9.0 / zoomFactor)));
        painter.setFont(lblFont);
        painter.drawText(editCanvas + QPointF(editSize, 0), QObject::tr("Editar"));

        prevHandle = editCanvas;
    }

    // Tirador de editar FIGURA
    if (obj.type == ObjectType::Shape) {
        QPointF editCanvas = L.midTop + L.upDir * (40.0 / zoomFactor);
        double editSize = handleSize * 1.3;
        QRectF editRect(editCanvas.x() - editSize/2, editCanvas.y() - editSize/2,
                        editSize, editSize);
        painter.setPen(QPen(QColor("#10b981"), penW));
        painter.setBrush(QColor("#34d399"));
        painter.drawRect(editRect);
        double inset = editSize * 0.25;
        painter.setPen(QPen(QColor("#064e3b"), penW));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(editRect.adjusted(inset, inset, -inset, -inset));
        painter.setPen(QPen(QColor("#10b981"), penW, Qt::SolidLine));
        painter.drawLine(L.rotatePos, editCanvas);

        painter.setPen(labelColor);
        QFont lblFont; lblFont.setPixelSize(qMax(8, (int)(9.0 / zoomFactor)));
        painter.setFont(lblFont);
        painter.drawText(editCanvas + QPointF(editSize, 0), QObject::tr("Editar"));

        prevHandle = editCanvas;
    }

 

    {
        double offsetInt = (obj.type == ObjectType::Text || obj.type == ObjectType::Shape)
                           ? 62.0 : 40.0;
        QPointF intCanvas = L.midTop + L.upDir * (offsetInt / zoomFactor);
        double intSize = handleSize * 1.4;
        QRectF intRect(intCanvas.x() - intSize/2, intCanvas.y() - intSize/2,
                       intSize, intSize);

        painter.setPen(QPen(mainBlue, penW, Qt::SolidLine));
        painter.drawLine(prevHandle, intCanvas);

        QColor intBg   = darkMode ? QColor("#0ea5e9") : QColor("#0284c7");
        QColor intBrd  = darkMode ? QColor("#38bdf8") : QColor("#0ea5e9");
        painter.setPen(QPen(intBrd, penW));
        painter.setBrush(intBg);
        painter.drawRect(intRect);

        // Icono ⇄
        double cx = intCanvas.x(), cy = intCanvas.y();
        double arW = intSize * 0.55;
        double arH = intSize * 0.35;
        painter.setPen(QPen(Qt::white, penW));
        painter.drawLine(QPointF(cx - arW/2, cy - arH/2), QPointF(cx + arW/2, cy - arH/2));
        painter.drawLine(QPointF(cx + arW/2, cy - arH/2), QPointF(cx + arW/2 - arW*0.3, cy - arH/2 - arH*0.4));
        painter.drawLine(QPointF(cx + arW/2, cy - arH/2), QPointF(cx + arW/2 - arW*0.3, cy - arH/2 + arH*0.4));
        painter.drawLine(QPointF(cx + arW/2, cy + arH/2), QPointF(cx - arW/2, cy + arH/2));
        painter.drawLine(QPointF(cx - arW/2, cy + arH/2), QPointF(cx - arW/2 + arW*0.3, cy + arH/2 - arH*0.4));
        painter.drawLine(QPointF(cx - arW/2, cy + arH/2), QPointF(cx - arW/2 + arW*0.3, cy + arH/2 + arH*0.4));

        painter.setPen(labelColor);
        QFont lblFont; lblFont.setPixelSize(qMax(8, (int)(9.0 / zoomFactor)));
        lblFont.setBold(true);
        painter.setFont(lblFont);
        painter.drawText(intCanvas + QPointF(intSize, 0), QObject::tr("Integrar"));
    }
}



inline ObjectHandle findObjectGizmoHandle(const PaintObject &obj,
                                          const QPointF &canvasPos,
                                          double zoomFactor) {
    double threshold = 12.0 / zoomFactor;
    QRectF visual = objectVisualRect(obj);
    GizmoLayout L = computeGizmoLayout(visual, obj.rotation, zoomFactor);

    // Integrar
    double offsetInt = (obj.type == ObjectType::Text || obj.type == ObjectType::Shape)
                       ? 62.0 : 40.0;
    QPointF intCanvas = L.midTop + L.upDir * (offsetInt / zoomFactor);
    if (QLineF(canvasPos, intCanvas).length() <= threshold)
        return ObjectHandle::IntegrateToCanvas;

    // Editar (texto / figura)
    QPointF editCanvas = L.midTop + L.upDir * (40.0 / zoomFactor);
    if (obj.type == ObjectType::Text) {
        if (QLineF(canvasPos, editCanvas).length() <= threshold)
            return ObjectHandle::EditText;
    }
    if (obj.type == ObjectType::Shape) {
        if (QLineF(canvasPos, editCanvas).length() <= threshold)
            return ObjectHandle::EditShape;
    }

    // Resto: rotación, esquinas, lados, mover
    return hitTestGizmo(L, canvasPos, zoomFactor);
}

inline void renderShapeObject(QPainter &painter, const PaintObject &obj) {
    QRect origRect = QRect(obj.startPoint, obj.endPoint).normalized();
    painter.save();

    if (qAbs(obj.rotation) > 0.001 || qAbs(obj.scaleX - 1.0) > 0.001 || qAbs(obj.scaleY - 1.0) > 0.001) {
        painter.translate(obj.bounds.center());
        painter.rotate(obj.rotation);
        painter.scale(obj.scaleX, obj.scaleY);
        painter.translate(-origRect.center());
    } else {
        QPointF moveDelta = obj.bounds.topLeft() - QPointF(origRect.topLeft());
        painter.translate(moveDelta);
    }

    if (obj.shapeTool == ToolCube) {
        painter.setPen(QPen(obj.strokeColor, qMax(1, obj.strokeWidth)));
        painter.setBrush(obj.hollow ? Qt::NoBrush : QBrush(obj.fillColor));
        PaintEngine::drawGeometry(painter, obj.startPoint, obj.endPoint, obj.shapeTool);
        painter.restore();
        return;
    }

    QPainterPath shapePath = crearPathDeGeometria(obj.shapeTool, obj.startPoint, obj.endPoint);
    bool isLine = (obj.shapeTool == ToolLine);

    if (isLine) {
        painter.setPen(QPen(obj.strokeColor, qMax(1, obj.strokeWidth), Qt::SolidLine, Qt::RoundCap));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(shapePath);
    } else if (obj.isFrame) {
        painter.save();
        painter.setClipPath(shapePath);
        if (!obj.frameImage.isNull()) {
            QRectF box = shapePath.boundingRect();
            double imgW = obj.frameImage.width(), imgH = obj.frameImage.height();
            if (imgW > 0 && imgH > 0) {
                double cover = qMax(box.width() / imgW, box.height() / imgH);
                double s = cover * obj.frameImageScale;
                double dw = imgW * s, dh = imgH * s;
                double cx = box.center().x() + obj.frameImageOffset.x();
                double cy = box.center().y() + obj.frameImageOffset.y();
                painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
                painter.drawImage(QRectF(cx - dw/2, cy - dh/2, dw, dh), obj.frameImage);
            }
        } else {
            painter.fillPath(shapePath, QColor(225, 225, 225));
            painter.setPen(QPen(QColor(120, 120, 120), 1, Qt::DashLine));
            painter.setBrush(Qt::NoBrush);
            QRectF box = shapePath.boundingRect();
            double iw = box.width() * 0.32, ih = box.height() * 0.32;
            QRectF ic(box.center().x() - iw/2, box.center().y() - ih/2, iw, ih);
            painter.drawRect(ic);
            painter.drawLine(ic.left(), ic.bottom(), ic.left() + iw*0.4, ic.top() + ih*0.4);
            painter.drawLine(ic.left() + iw*0.4, ic.top() + ih*0.4, ic.left() + iw*0.7, ic.top() + ih*0.7);
            painter.drawLine(ic.left() + iw*0.7, ic.top() + ih*0.7, ic.right(), ic.bottom());
        }
        painter.restore();
        if (obj.strokeWidth > 0) {
            painter.setPen(QPen(obj.strokeColor, obj.strokeWidth));
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(shapePath);
        }
    } else if (obj.hollow) {
        painter.setPen(QPen(obj.strokeColor, qMax(1, obj.strokeWidth)));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(shapePath);
    } else {
        painter.setPen(QPen(obj.strokeColor, obj.strokeWidth));
        painter.setBrush(obj.fillColor);
        painter.drawPath(shapePath);
    }

    painter.restore();
}

class ShapePropertiesDialog : public QDialog {
    Q_OBJECT
private:
    bool m_dark;
    ToolType m_tool;
    QColor fillColor, strokeColor;
    int strokeWidth;
    bool hollow, isFrame;
    QImage frameImage;
    double frameImageScale;
    QPointF frameImageOffset;

    QPushButton *btnFill, *btnStroke, *btnLoadImage;
    QSpinBox *spinStroke;
    QCheckBox *chkHollow, *chkFrame;
    QSlider *sliderImgScale, *sliderOffX, *sliderOffY;
    QLabel *lblPreview, *lblImageInfo;
    QWidget *imageControls;

    void updateSwatch(QPushButton *btn, const QColor &c) {
        btn->setStyleSheet(QString("background-color: %1; border: 1px solid #555; border-radius: 4px;").arg(c.name()));
    }

    void updatePreview() {
        int pw = 240, ph = 240;
        QPixmap pm(pw, ph);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);

        QPixmap checker(10, 10); checker.fill(QColor(240, 240, 240));
        QPainter cp(&checker);
        cp.fillRect(0, 0, 5, 5, QColor(200, 200, 200));
        cp.fillRect(5, 5, 5, 5, QColor(200, 200, 200));
        cp.end();
        p.fillRect(0, 0, pw, ph, QBrush(checker));

        QRectF r(35, 35, pw - 70, ph - 70);
        QPoint p1(r.topLeft().toPoint()), p2(r.bottomRight().toPoint());

        if (m_tool == ToolCube) {
            p.setPen(QPen(strokeColor, qMax(1, strokeWidth)));
            p.setBrush(hollow ? Qt::NoBrush : QBrush(fillColor));
            PaintEngine::drawGeometry(p, p1, p2, ToolCube);
        } else {
            QPainterPath path = crearPathDeGeometria(m_tool, p1, p2);
            bool strokeOnlyShape = (m_tool == ToolLine);
            if (strokeOnlyShape) {
                p.setPen(QPen(strokeColor, qMax(1, strokeWidth), Qt::SolidLine, Qt::RoundCap));
                p.setBrush(Qt::NoBrush);
                p.drawPath(path);
            } else if (isFrame) {
                p.save();
                p.setClipPath(path);
                if (!frameImage.isNull()) {
                    double imgW = frameImage.width(), imgH = frameImage.height();
                    if (imgW > 0 && imgH > 0) {
                        double cover = qMax(r.width() / imgW, r.height() / imgH);
                        double s = cover * frameImageScale;
                        double dw = imgW * s, dh = imgH * s;
                        double cx = r.center().x() + frameImageOffset.x();
                        double cy = r.center().y() + frameImageOffset.y();
                        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
                        p.drawImage(QRectF(cx - dw/2, cy - dh/2, dw, dh), frameImage);
                    }
                } else {
                    p.fillPath(path, QColor(225, 225, 225));
                }
                p.restore();
                if (strokeWidth > 0) {
                    p.setPen(QPen(strokeColor, strokeWidth));
                    p.setBrush(Qt::NoBrush);
                    p.drawPath(path);
                }
            } else if (hollow) {
                p.setPen(QPen(strokeColor, qMax(1, strokeWidth)));
                p.setBrush(Qt::NoBrush);
                p.drawPath(path);
            } else {
                p.setPen(QPen(strokeColor, strokeWidth));
                p.setBrush(fillColor);
                p.drawPath(path);
            }
        }
        p.end();
        lblPreview->setPixmap(pm);

        QString info;
        if (isFrame) {
            info = frameImage.isNull()
                   ? tr("Sin imagen")
                   : tr("Imagen: %1 x %2 px").arg(frameImage.width()).arg(frameImage.height());
        } else {
            info = hollow ? tr("Figura hueca (solo contorno)") : tr("Figura rellena");
        }
        lblImageInfo->setText(info);
    }

public:
    ShapePropertiesDialog(bool dark, ToolType tool,
                          const QColor &fill, const QColor &stroke,
                          int sw, bool hol, bool fram,
                          const QImage &fimg,
                          double fScale, const QPointF &fOffset,
                          QWidget *parent = nullptr)
        : QDialog(parent), m_dark(dark), m_tool(tool),
          fillColor(fill), strokeColor(stroke),
          strokeWidth(sw), hollow(hol), isFrame(fram),
          frameImage(fimg),
          frameImageScale(fScale), frameImageOffset(fOffset)
    {
        setWindowTitle(tr("Propiedades de Figura"));
        setFixedSize(600, 460);

        QString bg   = dark ? "#1a1a1a" : "#fafafa";
        QString text = dark ? "#e5e5e5" : "#111827";
        QString sec  = dark ? "#b0b0b0" : "#4b5563";
        QString bd   = dark ? "#3a3a3a" : "#e5e7eb";

        setStyleSheet(QString("QDialog { background-color: %1; } QLabel { color: %2; font-size: 12px; } "
                              "QCheckBox { color: %2; font-size: 12px; }").arg(bg, text));

        QHBoxLayout *mainLayout = new QHBoxLayout(this);
        mainLayout->setContentsMargins(18, 18, 18, 18);
        mainLayout->setSpacing(18);

        QVBoxLayout *leftCol = new QVBoxLayout();
        QLabel *previewTitle = new QLabel(tr("Vista previa"));
        previewTitle->setStyleSheet(QString("color: %1; font-weight: 600;").arg(sec));
        leftCol->addWidget(previewTitle);
        lblPreview = new QLabel();
        lblPreview->setFixedSize(240, 240);
        lblPreview->setAlignment(Qt::AlignCenter);
        lblPreview->setStyleSheet(QString("border: 1px solid %1; border-radius: 8px;").arg(bd));
        leftCol->addWidget(lblPreview);
        leftCol->addStretch();
        mainLayout->addLayout(leftCol);

        QVBoxLayout *rightCol = new QVBoxLayout();
        rightCol->setSpacing(9);

        QHBoxLayout *fillRow = new QHBoxLayout();
        fillRow->addWidget(new QLabel(tr("Color de relleno:")));
        btnFill = new QPushButton(); btnFill->setFixedSize(56, 26);
        btnFill->setCursor(Qt::PointingHandCursor);
        fillRow->addWidget(btnFill); fillRow->addStretch();
        rightCol->addLayout(fillRow);

        QHBoxLayout *strokeRow = new QHBoxLayout();
        strokeRow->addWidget(new QLabel(tr("Color de borde:")));
        btnStroke = new QPushButton(); btnStroke->setFixedSize(56, 26);
        btnStroke->setCursor(Qt::PointingHandCursor);
        strokeRow->addWidget(btnStroke); strokeRow->addSpacing(12);
        strokeRow->addWidget(new QLabel(tr("Grosor:")));
        spinStroke = new QSpinBox(); spinStroke->setRange(0, 60);
        spinStroke->setValue(strokeWidth); spinStroke->setFixedWidth(64);
        strokeRow->addWidget(spinStroke); strokeRow->addStretch();
        rightCol->addLayout(strokeRow);

        chkHollow = new QCheckBox(tr("Figura HUECA (solo contorno, sin relleno)"));
        chkHollow->setChecked(hollow);
        rightCol->addWidget(chkHollow);

        chkFrame = new QCheckBox(tr("Usar como MARCO de imagen (estilo Canva)"));
        chkFrame->setChecked(isFrame);
        rightCol->addWidget(chkFrame);

        btnLoadImage = new QPushButton(tr("Cargar imagen en el marco..."));
        btnLoadImage->setCursor(Qt::PointingHandCursor);
        btnLoadImage->setFixedHeight(34);
        rightCol->addWidget(btnLoadImage);

        imageControls = new QWidget();
        QVBoxLayout *icLayout = new QVBoxLayout(imageControls);
        icLayout->setContentsMargins(0, 0, 0, 0);
        icLayout->setSpacing(5);
        icLayout->addWidget(new QLabel(tr("Escala de la imagen:")));
        sliderImgScale = new QSlider(Qt::Horizontal);
        sliderImgScale->setRange(20, 300);
        sliderImgScale->setValue((int)(frameImageScale * 100));
        icLayout->addWidget(sliderImgScale);
        icLayout->addWidget(new QLabel(tr("Desplazar horizontal (X):")));
        sliderOffX = new QSlider(Qt::Horizontal);
        sliderOffX->setRange(-300, 300);
        sliderOffX->setValue((int)frameImageOffset.x());
        icLayout->addWidget(sliderOffX);
        icLayout->addWidget(new QLabel(tr("Desplazar vertical (Y):")));
        sliderOffY = new QSlider(Qt::Horizontal);
        sliderOffY->setRange(-300, 300);
        sliderOffY->setValue((int)frameImageOffset.y());
        icLayout->addWidget(sliderOffY);
        rightCol->addWidget(imageControls);

        lblImageInfo = new QLabel();
        lblImageInfo->setWordWrap(true);
        lblImageInfo->setStyleSheet(QString("color: %1; font-size: 11px;").arg(sec));
        rightCol->addWidget(lblImageInfo);
        rightCol->addStretch();

        QHBoxLayout *btnRow = new QHBoxLayout();
        btnRow->addStretch();
        QPushButton *btnCancel = new QPushButton(tr("Cancelar"));
        btnCancel->setCursor(Qt::PointingHandCursor);
        btnCancel->setFixedHeight(36); btnCancel->setMinimumWidth(96);
        QPushButton *btnOk = new QPushButton(tr("Aplicar"));
        btnOk->setCursor(Qt::PointingHandCursor);
        btnOk->setFixedHeight(36); btnOk->setMinimumWidth(120);
        btnOk->setStyleSheet("QPushButton { background-color: #2563eb; color: white; border: none; "
                             "border-radius: 6px; font-weight: 600; } "
                             "QPushButton:hover { background-color: #1d4ed8; }");
        btnRow->addWidget(btnCancel); btnRow->addWidget(btnOk);
        rightCol->addLayout(btnRow);

        mainLayout->addLayout(rightCol);

        updateSwatch(btnFill, fillColor);
        updateSwatch(btnStroke, strokeColor);
        imageControls->setVisible(isFrame);

        connect(btnFill, &QPushButton::clicked, this, [this]() {
            QColor c = QColorDialog::getColor(fillColor, this, tr("Color de relleno"));
            if (c.isValid()) { fillColor = c; updateSwatch(btnFill, c); updatePreview(); }
        });
        connect(btnStroke, &QPushButton::clicked, this, [this]() {
            QColor c = QColorDialog::getColor(strokeColor, this, tr("Color de borde"));
            if (c.isValid()) { strokeColor = c; updateSwatch(btnStroke, c); updatePreview(); }
        });
        connect(spinStroke, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) {
            strokeWidth = v; updatePreview();
        });
        connect(chkHollow, &QCheckBox::toggled, this, [this](bool c) {
            hollow = c;
            if (c) { chkFrame->blockSignals(true); chkFrame->setChecked(false);
                     isFrame = false; imageControls->setVisible(false);
                     chkFrame->blockSignals(false); }
            updatePreview();
        });
        connect(chkFrame, &QCheckBox::toggled, this, [this](bool c) {
            isFrame = c;
            imageControls->setVisible(c);
            if (c) { chkHollow->blockSignals(true); chkHollow->setChecked(false);
                     hollow = false; chkHollow->blockSignals(false); }
            updatePreview();
        });
        connect(btnLoadImage, &QPushButton::clicked, this, [this]() {
            QString fn = QFileDialog::getOpenFileName(this, tr("Cargar imagen"),
                                                      QDir::currentPath(), tr("Imagenes (*.png *.jpg *.jpeg *.bmp *.gif)"));
            if (!fn.isEmpty()) {
                QImage img(fn);
                if (!img.isNull()) { frameImage = img; updatePreview(); }
            }
        });
        connect(sliderImgScale, &QSlider::valueChanged, this, [this](int v) {
            frameImageScale = v / 100.0; updatePreview();
        });
        connect(sliderOffX, &QSlider::valueChanged, this, [this](int v) {
            frameImageOffset.setX(v); updatePreview();
        });
        connect(sliderOffY, &QSlider::valueChanged, this, [this](int v) {
            frameImageOffset.setY(v); updatePreview();
        });
        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
        connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);

        updatePreview();
    }

    QColor getFillColor() const { return fillColor; }
    QColor getStrokeColor() const { return strokeColor; }
    int getStrokeWidth() const { return strokeWidth; }
    bool getHollow() const { return hollow; }
    bool getIsFrame() const { return isFrame; }
    QImage getFrameImage() const { return frameImage; }
    double getFrameImageScale() const { return frameImageScale; }
    QPointF getFrameImageOffset() const { return frameImageOffset; }
};

#endif 

// SHAPEOBJECTS_H