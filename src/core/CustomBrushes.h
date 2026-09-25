#ifndef CUSTOMBRUSHES_H
#define CUSTOMBRUSHES_H
#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QRandomGenerator>
#include <QFile>
#include <QFileDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QSlider>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QToolButton>
#include <QFrame>
#include <QCursor>
#include <QButtonGroup>
#include <QDrag>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QRadioButton>
#include <QScrollArea>
#include <QApplication>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QTimer>
#include <QHash>
#include <QMessageBox>
#include <QMenu>
#include <cmath>
#include <vector>
#include "tools/PixelArtTools.h"
#include "tools/RetouchTools.h"
#include "tools/MagicWandTools.h"

// ============================================================
// ENUMS
// ============================================================
enum class ShapeType {
    Circle, Square, RoundedSquare, Diamond, Triangle, RightTriangle,
    Pentagon, Hexagon,
    Star4, Star5, Star6,
    Cross, Plus, X,
    Arrow, Heart,
    Line, PencilTip, FlatTip, ChiselTip,
    Leaf, Drop, Crescent, Ring, HalfCircle, Sparkle, Clover, Gear,
    Lightning, MusicNote, Flower, Butterfly, Cloud, Speech, LocationPin,
    Wave, Spiral, StarMany, Infinity, DiamondStar,
    CustomStamp
};

enum class DragMode { Continuous, Stamped, Dotted, Scattered, Ribbon };
enum class RotationMode { Fixed, FollowDirection, Random };

struct ShapeElement {
    ShapeType shape = ShapeType::Circle;
    double offsetX = 0.0, offsetY = 0.0, scale = 1.0, rotation = 0.0;
    int opacity = 100;
    QImage customImage;
};

struct BrushSettings {
    int size = 20;
    ShapeType shape = ShapeType::Circle;
    DragMode dragMode = DragMode::Continuous;
    RotationMode rotationMode = RotationMode::Fixed;
    int opacity = 100, scatter = 0;
    double angle = 0.0;
    int density = 1;
    int flow = 100;
    bool isAirbrush = false;
    int sizeJitter = 0, angleJitter = 0, opacityJitter = 0;
    double aspectRatio = 1.0;
    bool wetMix = false;
    int wetAmount = 50;
    bool granulation = false;
    bool mixSecondColor = false;
    QVector<ShapeElement> shapeElements;
    QImage customStampImage;
};

// ============================================================
// MOTOR DE DIBUJO
// ============================================================
class PaintEngine {
public:
    static QString shapeName(ShapeType s) {
        switch (s) {
        case ShapeType::Circle:       return QObject::tr("Circulo");
        case ShapeType::Square:       return QObject::tr("Cuadrado");
        case ShapeType::RoundedSquare:return QObject::tr("Cuadrado redondeado");
        case ShapeType::Diamond:      return QObject::tr("Rombo");
        case ShapeType::Triangle:     return QObject::tr("Triangulo");
        case ShapeType::RightTriangle:return QObject::tr("Triangulo rectangulo");
        case ShapeType::Pentagon:     return QObject::tr("Pentagono");
        case ShapeType::Hexagon:      return QObject::tr("Hexagono");
        case ShapeType::Star4:        return QObject::tr("Estrella 4 puntas");
        case ShapeType::Star5:        return QObject::tr("Estrella 5 puntas");
        case ShapeType::Star6:        return QObject::tr("Estrella 6 puntas");
        case ShapeType::Cross:        return QObject::tr("Cruz");
        case ShapeType::Plus:         return QObject::tr("Signo mas");
        case ShapeType::X:            return QObject::tr("Equis");
        case ShapeType::Arrow:        return QObject::tr("Flecha");
        case ShapeType::Heart:        return QObject::tr("Corazon");
        case ShapeType::Line:         return QObject::tr("Linea");
        case ShapeType::PencilTip:    return QObject::tr("Punta de lapiz");
        case ShapeType::FlatTip:      return QObject::tr("Punta plana");
        case ShapeType::ChiselTip:    return QObject::tr("Punta cincel");
        case ShapeType::Leaf:         return QObject::tr("Hoja");
        case ShapeType::Drop:         return QObject::tr("Gota");
        case ShapeType::Crescent:     return QObject::tr("Media luna");
        case ShapeType::Ring:         return QObject::tr("Anillo");
        case ShapeType::HalfCircle:   return QObject::tr("Semicirculo");
        case ShapeType::Sparkle:      return QObject::tr("Destello");
        case ShapeType::Clover:       return QObject::tr("Trebol");
        case ShapeType::Gear:         return QObject::tr("Engranaje");
        case ShapeType::Lightning:    return QObject::tr("Rayo");
        case ShapeType::MusicNote:    return QObject::tr("Nota musical");
        case ShapeType::Flower:       return QObject::tr("Flor");
        case ShapeType::Butterfly:    return QObject::tr("Mariposa");
        case ShapeType::Cloud:        return QObject::tr("Nube");
        case ShapeType::Speech:       return QObject::tr("Bocadillo");
        case ShapeType::LocationPin:  return QObject::tr("Pin de ubicacion");
        case ShapeType::Wave:         return QObject::tr("Onda");
        case ShapeType::Spiral:       return QObject::tr("Espiral");
        case ShapeType::StarMany:     return QObject::tr("Estrella muchos picos");
        case ShapeType::Infinity:     return QObject::tr("Infinito");
        case ShapeType::DiamondStar:  return QObject::tr("Rombo estrella");
        case ShapeType::CustomStamp:  return QObject::tr("PNG importado");
        }
        return QString();
    }

    static QList<ShapeType> allShapes() {
        return {
            ShapeType::Circle, ShapeType::Square, ShapeType::RoundedSquare, ShapeType::Diamond,
            ShapeType::Triangle, ShapeType::RightTriangle, ShapeType::Pentagon,
            ShapeType::Hexagon, ShapeType::Star4, ShapeType::Star5, ShapeType::Star6,
            ShapeType::Cross, ShapeType::Plus, ShapeType::X,
            ShapeType::Arrow, ShapeType::Heart, ShapeType::Line, ShapeType::PencilTip,
            ShapeType::FlatTip, ShapeType::ChiselTip, ShapeType::Leaf,
            ShapeType::Drop, ShapeType::Crescent, ShapeType::Ring, ShapeType::HalfCircle,
            ShapeType::Sparkle, ShapeType::Clover, ShapeType::Gear,
            ShapeType::Lightning, ShapeType::MusicNote, ShapeType::Flower, ShapeType::Butterfly,
            ShapeType::Cloud, ShapeType::Speech, ShapeType::LocationPin,
            ShapeType::Wave, ShapeType::Spiral, ShapeType::StarMany, ShapeType::Infinity,
            ShapeType::DiamondStar
        };
    }

    static QPainterPath baseShapePath(ShapeType shape) {
        static QHash<int, QPainterPath> cache;
        int key = (int)shape;
        auto it = cache.constFind(key);
        if (it != cache.constEnd()) return it.value();

        const QRectF r(-50.0, -50.0, 100.0, 100.0);
        QPainterPath path;
        const double cx = r.center().x(), cy = r.center().y();
        const double w = r.width(), h = r.height();

        switch (shape) {
        case ShapeType::Circle:       path.addEllipse(r); break;
        case ShapeType::Square:       path.addRect(r); break;
        case ShapeType::RoundedSquare:path.addRoundedRect(r, w * 0.2, h * 0.2); break;
        case ShapeType::Diamond:
            path.moveTo(cx, r.top()); path.lineTo(r.right(), cy);
            path.lineTo(cx, r.bottom()); path.lineTo(r.left(), cy);
            path.closeSubpath(); break;
        case ShapeType::Triangle:
            path.moveTo(cx, r.top()); path.lineTo(r.left(), r.bottom());
            path.lineTo(r.right(), r.bottom()); path.closeSubpath(); break;
        case ShapeType::RightTriangle:
            path.moveTo(r.topLeft()); path.lineTo(r.bottomLeft());
            path.lineTo(r.bottomRight()); path.closeSubpath(); break;
        case ShapeType::Pentagon: {
            for (int i = 0; i < 5; ++i) {
                double a = -M_PI / 2.0 + i * 2.0 * M_PI / 5.0;
                double px = cx + (w / 2.0) * cos(a), py = cy + (h / 2.0) * sin(a);
                if (i == 0) path.moveTo(px, py); else path.lineTo(px, py);
            } path.closeSubpath(); break;
        }
        case ShapeType::Hexagon: {
            for (int i = 0; i < 6; ++i) {
                double a = -M_PI / 2.0 + i * 2.0 * M_PI / 6.0;
                double px = cx + (w / 2.0) * cos(a), py = cy + (h / 2.0) * sin(a);
                if (i == 0) path.moveTo(px, py); else path.lineTo(px, py);
            } path.closeSubpath(); break;
        }
        case ShapeType::Star4: {
            for (int i = 0; i < 8; ++i) {
                double a = -M_PI / 2.0 + i * M_PI / 4.0;
                double f = (i % 2 == 0) ? 1.0 : 0.35;
                double px = cx + (w / 2.0) * f * cos(a), py = cy + (h / 2.0) * f * sin(a);
                if (i == 0) path.moveTo(px, py); else path.lineTo(px, py);
            } path.closeSubpath(); break;
        }
        case ShapeType::Star5: {
            for (int i = 0; i < 10; ++i) {
                double a = -M_PI / 2.0 + i * M_PI / 5.0;
                double f = (i % 2 == 0) ? 1.0 : 0.45;
                double px = cx + (w / 2.0) * f * cos(a), py = cy + (h / 2.0) * f * sin(a);
                if (i == 0) path.moveTo(px, py); else path.lineTo(px, py);
            } path.closeSubpath(); break;
        }
        case ShapeType::Star6: {
            for (int i = 0; i < 12; ++i) {
                double a = -M_PI / 2.0 + i * M_PI / 6.0;
                double f = (i % 2 == 0) ? 1.0 : 0.5;
                double px = cx + (w / 2.0) * f * cos(a), py = cy + (h / 2.0) * f * sin(a);
                if (i == 0) path.moveTo(px, py); else path.lineTo(px, py);
            } path.closeSubpath(); break;
        }
        case ShapeType::Cross: {
            double t = w * 0.3;
            path.moveTo(cx - t / 2, r.top()); path.lineTo(cx + t / 2, r.top());
            path.lineTo(cx + t / 2, cy - t / 2); path.lineTo(r.right(), cy - t / 2);
            path.lineTo(r.right(), cy + t / 2); path.lineTo(cx + t / 2, cy + t / 2);
            path.lineTo(cx + t / 2, r.bottom()); path.lineTo(cx - t / 2, r.bottom());
            path.lineTo(cx - t / 2, cy + t / 2); path.lineTo(r.left(), cy + t / 2);
            path.lineTo(r.left(), cy - t / 2); path.lineTo(cx - t / 2, cy - t / 2);
            path.closeSubpath(); break;
        }
        case ShapeType::Plus: {
            double t = w * 0.35;
            path.addRect(cx - t / 2, r.top(), t, h);
            path.addRect(r.left(), cy - t / 2, w, t); break;
        }
        case ShapeType::X: {
            double t = w * 0.25;
            QPolygonF poly;
            poly << QPointF(r.left() + t, r.top()) << QPointF(cx, cy - t)
                 << QPointF(r.right() - t, r.top()) << QPointF(r.right(), r.top() + t)
                 << QPointF(cx + t, cy) << QPointF(r.right(), r.bottom() - t)
                 << QPointF(r.right() - t, r.bottom()) << QPointF(cx, cy + t)
                 << QPointF(r.left() + t, r.bottom()) << QPointF(r.left(), r.bottom() - t)
                 << QPointF(cx - t, cy) << QPointF(r.left(), r.top() + t);
            path.addPolygon(poly); path.closeSubpath(); break;
        }
        case ShapeType::Arrow: {
            double bodyH = h * 0.4, headW = w * 0.45;
            path.moveTo(r.left(), cy - bodyH / 2);
            path.lineTo(r.right() - headW, cy - bodyH / 2);
            path.lineTo(r.right() - headW, r.top()); path.lineTo(r.right(), cy);
            path.lineTo(r.right() - headW, r.bottom());
            path.lineTo(r.right() - headW, cy + bodyH / 2);
            path.lineTo(r.left(), cy + bodyH / 2); path.closeSubpath(); break;
        }
        case ShapeType::Heart:
            path.moveTo(cx, r.top() + h * 0.3);
            path.cubicTo(r.left() + w * 0.1, r.top() - h * 0.05, r.left(), r.top() + h * 0.55, cx, r.bottom());
            path.cubicTo(r.right(), r.top() + h * 0.55, r.right() - w * 0.1, r.top() - h * 0.05, cx, r.top() + h * 0.3);
            break;
        case ShapeType::Line: {
            double t = qMax(1.0, h * 0.15);
            path.addRect(r.left(), cy - t / 2, w, t); break;
        }
        case ShapeType::PencilTip:
            path.moveTo(cx, r.top());
            path.lineTo(r.left() + w * 0.25, r.bottom());
            path.lineTo(r.right() - w * 0.25, r.bottom());
            path.closeSubpath(); break;
        case ShapeType::FlatTip: {
            double fh = h * 0.45;
            path.addRoundedRect(r.left(), cy - fh / 2, w, fh, fh * 0.3, fh * 0.3); break;
        }
        case ShapeType::ChiselTip:
            path.moveTo(r.left() + w * 0.15, r.top());
            path.lineTo(r.right() - w * 0.15, r.top());
            path.lineTo(r.right(), r.bottom()); path.lineTo(r.left(), r.bottom());
            path.closeSubpath(); break;
        case ShapeType::Leaf:
            path.moveTo(r.left(), r.bottom());
            path.cubicTo(r.left(), r.top() + h * 0.2, cx, r.top(), r.right(), r.top());
            path.cubicTo(r.right(), r.top() + h * 0.6, cx + w * 0.2, r.bottom(), r.left(), r.bottom());
            path.closeSubpath(); break;
        case ShapeType::Drop:
            path.moveTo(cx, r.top());
            path.cubicTo(cx + w * 0.4, r.top() + h * 0.4, r.right(), cy + h * 0.15, cx, r.bottom());
            path.cubicTo(r.left(), cy + h * 0.15, cx - w * 0.4, r.top() + h * 0.4, cx, r.top());
            path.closeSubpath(); break;
        case ShapeType::Crescent:
            path.moveTo(cx + w * 0.15, r.top());
            path.arcTo(r, 270, 180);
            path.cubicTo(cx - w * 0.05, r.top() + h * 0.75, cx - w * 0.05, r.top() + h * 0.25, cx + w * 0.15, r.top());
            path.closeSubpath(); break;
        case ShapeType::Ring: {
            path.addEllipse(r);
            double inset = w * 0.2;
            path.addEllipse(r.adjusted(inset, inset, -inset, -inset)); break;
        }
        case ShapeType::HalfCircle:
            path.moveTo(r.left(), cy); path.arcTo(r, 180, 180);
            path.closeSubpath(); break;
        case ShapeType::Sparkle: {
            double inner = 0.2;
            path.moveTo(cx, r.top());
            path.quadTo(cx + w * inner, cy - h * inner, r.right(), cy);
            path.quadTo(cx + w * inner, cy + h * inner, cx, r.bottom());
            path.quadTo(cx - w * inner, cy + h * inner, r.left(), cy);
            path.quadTo(cx - w * inner, cy - h * inner, cx, r.top());
            path.closeSubpath(); break;
        }
        case ShapeType::Clover: {
            double lr = w * 0.22;
            path.addEllipse(cx - lr, cy - lr * 2, lr * 2, lr * 2);
            path.addEllipse(cx - lr * 2, cy - lr, lr * 2, lr * 2);
            path.addEllipse(cx, cy - lr, lr * 2, lr * 2);
            path.addEllipse(cx - lr, cy, lr * 2, lr * 2); break;
        }
        case ShapeType::Gear: {
            int teeth = 8;
            double outerR = w / 2.0, innerR = w / 3.0;
            for (int i = 0; i < teeth * 2; ++i) {
                double a = i * M_PI / teeth;
                double rad = (i % 2 == 0) ? outerR : innerR;
                double px = cx + rad * cos(a), py = cy + rad * sin(a);
                if (i == 0) path.moveTo(px, py); else path.lineTo(px, py);
            } path.closeSubpath();
            double holeR = w * 0.12;
            path.addEllipse(cx - holeR, cy - holeR, holeR * 2, holeR * 2); break;
        }
        case ShapeType::Lightning: {
            path.moveTo(cx + w * 0.08, r.top());
            path.lineTo(cx - w * 0.28, cy + h * 0.05);
            path.lineTo(cx - w * 0.02, cy + h * 0.05);
            path.lineTo(cx - w * 0.12, r.bottom());
            path.lineTo(cx + w * 0.28, cy - h * 0.05);
            path.lineTo(cx + w * 0.02, cy - h * 0.05);
            path.lineTo(cx + w * 0.18, r.top());
            path.closeSubpath(); break;
        }
        case ShapeType::MusicNote: {
            double headR = w * 0.13;
            path.addEllipse(QPointF(cx - w * 0.10, cy + h * 0.28), headR, headR * 0.8);
            path.addRect(QRectF(cx + w * 0.02, r.top() + h * 0.05, w * 0.05, h * 0.45));
            path.moveTo(cx + w * 0.07, r.top() + h * 0.05);
            path.cubicTo(cx + w * 0.32, r.top() + h * 0.15,
                         cx + w * 0.32, r.top() + h * 0.35,
                         cx + w * 0.10, r.top() + h * 0.35);
            path.lineTo(cx + w * 0.07, r.top() + h * 0.28);
            path.cubicTo(cx + w * 0.20, r.top() + h * 0.28,
                         cx + w * 0.20, r.top() + h * 0.18,
                         cx + w * 0.07, r.top() + h * 0.18);
            path.closeSubpath(); break;
        }
        case ShapeType::Flower: {
            const int petals = 5;
            const double petalR = w * 0.20;
            for (int i = 0; i < petals; ++i) {
                double a = -M_PI / 2.0 + i * 2.0 * M_PI / petals;
                double px = cx + w * 0.20 * cos(a);
                double py = cy + h * 0.20 * sin(a);
                path.addEllipse(QPointF(px, py), petalR, petalR);
            }
            path.addEllipse(QPointF(cx, cy), w * 0.10, h * 0.10); break;
        }
        case ShapeType::Butterfly: {
            path.addEllipse(QPointF(cx, cy), w * 0.04, h * 0.30);
            path.moveTo(cx - w * 0.04, cy - h * 0.05);
            path.cubicTo(cx - w * 0.45, cy - h * 0.45, cx - w * 0.50, cy + h * 0.10, cx - w * 0.04, cy + h * 0.10);
            path.closeSubpath();
            path.moveTo(cx - w * 0.04, cy + h * 0.05);
            path.cubicTo(cx - w * 0.40, cy + h * 0.25, cx - w * 0.35, cy + h * 0.50, cx - w * 0.04, cy + h * 0.30);
            path.closeSubpath();
            path.moveTo(cx + w * 0.04, cy - h * 0.05);
            path.cubicTo(cx + w * 0.45, cy - h * 0.45, cx + w * 0.50, cy + h * 0.10, cx + w * 0.04, cy + h * 0.10);
            path.closeSubpath();
            path.moveTo(cx + w * 0.04, cy + h * 0.05);
            path.cubicTo(cx + w * 0.40, cy + h * 0.25, cx + w * 0.35, cy + h * 0.50, cx + w * 0.04, cy + h * 0.30);
            path.closeSubpath(); break;
        }
        case ShapeType::Cloud: {
            path.addEllipse(QPointF(cx - w * 0.20, cy + h * 0.05), w * 0.20, h * 0.20);
            path.addEllipse(QPointF(cx + w * 0.20, cy + h * 0.05), w * 0.22, h * 0.22);
            path.addEllipse(QPointF(cx - w * 0.05, cy - h * 0.10), w * 0.25, h * 0.25);
            path.addEllipse(QPointF(cx + w * 0.15, cy - h * 0.05), w * 0.20, h * 0.20);
            path.addEllipse(QPointF(cx, cy + h * 0.15), w * 0.30, h * 0.15); break;
        }
        case ShapeType::Speech: {
            double rad = w * 0.10;
            path.addRoundedRect(QRectF(r.left(), r.top(), w, h * 0.75), rad, rad);
            path.moveTo(cx - w * 0.15, r.top() + h * 0.75);
            path.lineTo(cx - w * 0.20, r.bottom());
            path.lineTo(cx + w * 0.05, r.top() + h * 0.75);
            path.closeSubpath(); break;
        }
        case ShapeType::LocationPin: {
            path.moveTo(cx, r.bottom());
            path.cubicTo(cx - w * 0.4, cy + h * 0.1, r.left(), r.top(), cx, r.top());
            path.cubicTo(r.right(), r.top(), cx + w * 0.4, cy + h * 0.1, cx, r.bottom());
            path.closeSubpath();
            double holeR = w * 0.12;
            path.addEllipse(QPointF(cx, cy - h * 0.05), holeR, holeR); break;
        }
        case ShapeType::Wave: {
            path.moveTo(r.left(), cy);
            path.cubicTo(r.left() + w * 0.15, r.top(), r.left() + w * 0.35, r.top(), cx, cy);
            path.cubicTo(cx + w * 0.15, r.bottom(), cx + w * 0.35, r.bottom(), r.right(), cy);
            path.cubicTo(cx + w * 0.35, cy + h * 0.35, cx + w * 0.15, cy + h * 0.35, cx, cy + h * 0.02);
            path.cubicTo(r.left() + w * 0.35, cy - h * 0.02, r.left() + w * 0.15, cy - h * 0.02, r.left(), cy);
            path.closeSubpath(); break;
        }
        case ShapeType::Spiral: {
            const int steps = 80;
            const double turns = 3.0;
            const double maxR = w * 0.45;
            for (int i = 0; i <= steps; ++i) {
                double t = (double)i / steps;
                double a = t * turns * 2.0 * M_PI;
                double rad = t * maxR;
                double px = cx + rad * cos(a);
                double py = cy + rad * sin(a);
                if (i == 0) path.moveTo(px, py); else path.lineTo(px, py);
            } break;
        }
        case ShapeType::StarMany: {
            const int points = 16;
            const double outerR = w * 0.5;
            const double innerR = w * 0.20;
            for (int i = 0; i < points * 2; ++i) {
                double a = -M_PI / 2.0 + i * M_PI / points;
                double rad = (i % 2 == 0) ? outerR : innerR;
                double px = cx + rad * cos(a);
                double py = cy + rad * sin(a);
                if (i == 0) path.moveTo(px, py); else path.lineTo(px, py);
            }
            path.closeSubpath(); break;
        }
        case ShapeType::Infinity: {
            const double rW = w * 0.28;
            const double rH = h * 0.28;
            const double off = w * 0.22;
            QPainterPath left, right;
            left.addEllipse(QPointF(cx - off, cy), rW, rH);
            right.addEllipse(QPointF(cx + off, cy), rW, rH);
            path = left.united(right);
            QPainterPath innerL, innerR;
            innerL.addEllipse(QPointF(cx - off, cy), rW * 0.55, rH * 0.55);
            innerR.addEllipse(QPointF(cx + off, cy), rW * 0.55, rH * 0.55);
            path = path.subtracted(innerL.united(innerR));
            QPainterPath center;
            center.addEllipse(QPointF(cx, cy), rW * 0.18, rH * 0.18);
            path = path.united(center);
            break;
        }
        case ShapeType::DiamondStar: {
            const double outerX = w * 0.5;
            const double outerY = h * 0.5;
            const double innerX = w * 0.15;
            const double innerY = h * 0.15;
            path.moveTo(cx, cy - outerY);
            path.lineTo(cx + innerX, cy - innerY);
            path.lineTo(cx + outerX, cy);
            path.lineTo(cx + innerX, cy + innerY);
            path.lineTo(cx, cy + outerY);
            path.lineTo(cx - innerX, cy + innerY);
            path.lineTo(cx - outerX, cy);
            path.lineTo(cx - innerX, cy - innerY);
            path.closeSubpath(); break;
        }
        case ShapeType::CustomStamp: break;
        }

        it = cache.insert(key, path);
        return it.value();
    }

    static QPainterPath transformedShapePath(ShapeType shape, const QRectF &r) {
        if (r.width() <= 0.0 || r.height() <= 0.0) return QPainterPath();
        if (shape == ShapeType::CustomStamp) return QPainterPath();
        QTransform t;
        t.translate(r.center().x(), r.center().y());
        t.scale(r.width() / 100.0, r.height() / 100.0);
        return t.map(baseShapePath(shape));
    }

    static QPainterPath shapePath(ShapeType shape, const QRectF &r) {
        return transformedShapePath(shape, r);
    }

    // Mantenemos tintImage por si alguien la quiere usar en el futuro
    static QImage tintImage(const QImage &src, const QColor &color) {
        if (src.isNull()) return src;
        QImage result = src.convertToFormat(QImage::Format_ARGB32);
        const int cr = color.red(), cg = color.green(), cb = color.blue();
        if (cr > 240 && cg > 240 && cb > 240) return result;
        for (int y = 0; y < result.height(); ++y) {
            QRgb *line = reinterpret_cast<QRgb*>(result.scanLine(y));
            for (int x = 0; x < result.width(); ++x) {
                const QRgb px = line[x];
                const int a = qAlpha(px);
                if (a == 0) continue;
                const int lum = qGray(px);
                const int nr = (cr * lum) / 255;
                const int ng = (cg * lum) / 255;
                const int nb = (cb * lum) / 255;
                line[x] = qRgba(nr, ng, nb, a);
            }
        }
        return result;
    }

    // ------------------------------------------------------------
    // Dibujar primitiva (PNG SE DIBUJA TAL CUAL, sin tintar)
    // ------------------------------------------------------------
    static void drawShapePrimitive(QPainter &painter, ShapeType shape, const QRectF &r, const QColor &color,
                                   const QImage &customImage = QImage()) {
        if (r.width() <= 0.0 || r.height() <= 0.0) return;
        if (shape == ShapeType::CustomStamp) {
            if (customImage.isNull()) return;
            painter.save();
            // PNG tal cual, respetando su aspecto original
            painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
            painter.drawImage(r, customImage);
            painter.restore();
            return;
        }
        painter.save();
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.translate(r.center());
        painter.scale(r.width() / 100.0, r.height() / 100.0);
        painter.drawPath(baseShapePath(shape));
        painter.restore();
    }

    static void drawShapeOutline(QPainter &painter, ShapeType shape, const QRectF &r,
                                 const QImage &customImage = QImage()) {
        if (r.width() <= 0.0 || r.height() <= 0.0) return;
        if (shape == ShapeType::CustomStamp) {
            if (customImage.isNull()) return;
            painter.save();
            painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
            painter.drawImage(r, customImage);
            painter.restore();
            return;
        }
        painter.drawPath(transformedShapePath(shape, r));
    }

    static void drawBrushSilhouette(QPainter &painter, const BrushSettings &config,
                                    double size, const QPointF &center) {
        painter.save();
        painter.translate(center);
        painter.rotate(config.angle);

        if (!config.shapeElements.isEmpty()) {
            double spread = size * 1.5;
            double scaleX = 1.0, scaleY = 1.0;
            if (config.aspectRatio < 1.0) scaleY = config.aspectRatio;
            else if (config.aspectRatio > 1.0) scaleX = config.aspectRatio;
            painter.scale(scaleX, scaleY);
            for (const ShapeElement &el : config.shapeElements) {
                double elCx = el.offsetX * spread;
                double elCy = el.offsetY * spread;
                double elSize = size * el.scale;
                QRectF r(elCx - elSize / 2, elCy - elSize / 2, elSize, elSize);
                painter.save();
                painter.translate(elCx, elCy);
                painter.rotate(el.rotation);
                painter.translate(-elCx, -elCy);
                drawShapeOutline(painter, el.shape, r, el.customImage);
                painter.restore();
            }
        } else {
            QRectF shapeRect(-size / 2.0, -size / 2.0, size, size);
            if (config.aspectRatio < 1.0) {
                double newH = size * config.aspectRatio;
                shapeRect = QRectF(-size / 2.0, -newH / 2.0, size, newH);
            } else if (config.aspectRatio > 1.0) {
                double newW = size * config.aspectRatio;
                shapeRect = QRectF(-newW / 2.0, -size / 2.0, newW, size);
            }
            drawShapeOutline(painter, config.shape, shapeRect, config.customStampImage);
        }
        painter.restore();
    }

    static double computeBreathFactor(double accumulatedLength, int brushSize, int flow) {
        if (flow >= 100) return 1.0;
        if (brushSize < 1) brushSize = 1;
        double strength = (100.0 - flow) / 100.0;
        double cycleLength = qMax(20.0, brushSize * 5.0);
        double phase = (accumulatedLength / cycleLength) * 2.0 * M_PI;
        double wave = 0.5 - 0.5 * cos(phase);
        double factor = 1.0 - strength * (1.0 - wave);
        double fadeInLen = brushSize * 1.0;
        if (accumulatedLength < fadeInLen) {
            double fadeIn = accumulatedLength / fadeInLen;
            factor *= 0.3 + 0.7 * fadeIn;
        }
        return qBound(0.05, factor, 1.0);
    }

    // ------------------------------------------------------------
    // Generar stamp (PNG tal cual, sin tintar)
    // ------------------------------------------------------------
    static QImage generateBrushStamp(const BrushSettings &config, const QColor &baseColor,
                                     int penOpacity, bool pixelArt,
                                     const QColor &secondColor = QColor()) {
        int size = qMax(1, config.size);
        int pad = 6;
        bool composite = !config.shapeElements.isEmpty();
        int canvasW, canvasH;
        if (composite) {
            int baseSize = size * 6 + pad * 2;
            canvasW = baseSize;
            canvasH = baseSize;
            if (config.aspectRatio > 1.0) canvasW = qMax(1, (int)(baseSize * config.aspectRatio));
        } else {
            canvasW = size + pad * 2;
            canvasH = size + pad * 2;
            if (config.aspectRatio > 1.0) canvasW = (int)(size * config.aspectRatio) + pad * 2;
        }
        QImage stamp(canvasW, canvasH, QImage::Format_ARGB32);
        stamp.fill(Qt::transparent);
        QPainter p(&stamp);
        p.setRenderHint(QPainter::Antialiasing, !pixelArt);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        double alpha = (config.opacity / 100.0) * (penOpacity / 255.0);
        bool doGradient = config.mixSecondColor && secondColor.isValid();

        // --- Custom stamp principal (PNG TAL CUAL) ---
        if (!composite && config.shape == ShapeType::CustomStamp) {
            if (config.customStampImage.isNull()) { p.end(); return stamp; }
            const double cx = canvasW / 2.0, cy = canvasH / 2.0;
            QRectF target(cx - size / 2.0, cy - size / 2.0, size, size);
            if (config.aspectRatio < 1.0) {
                target = QRectF(cx - size / 2.0, cy - (size * config.aspectRatio) / 2.0,
                                size, size * config.aspectRatio);
            } else if (config.aspectRatio > 1.0) {
                target = QRectF(cx - (size * config.aspectRatio) / 2.0, cy - size / 2.0,
                                size * config.aspectRatio, size);
            }
            p.setOpacity(qBound(0.0, alpha, 1.0));
            p.drawImage(target, config.customStampImage);
            p.end();
            return stamp;
        }

        if (composite) {
            double cx = canvasW / 2.0, cy = canvasH / 2.0;
            double spread = size * 1.5;
            p.save();
            p.translate(cx, cy);
            p.rotate(config.angle);
            for (const ShapeElement &el : config.shapeElements) {
                double elCx = el.offsetX * spread;
                double elCy = el.offsetY * spread;
                double elSize = size * el.scale;
                QRectF r(elCx - elSize / 2, elCy - elSize / 2, elSize, elSize);
                p.save();
                p.translate(elCx, elCy);
                p.rotate(el.rotation);
                p.translate(-elCx, -elCy);
                if (el.shape == ShapeType::CustomStamp) {
                    if (!el.customImage.isNull()) {
                        p.setOpacity(qBound(0.0, alpha * (el.opacity / 100.0), 1.0));
                        p.drawImage(r, el.customImage);
                        p.setOpacity(1.0);
                    }
                } else if (doGradient) {
                    QColor c1 = baseColor;
                    c1.setAlphaF(qBound(0.0, alpha * (el.opacity / 100.0), 1.0));
                    QColor c2 = secondColor;
                    c2.setAlphaF(qBound(0.0, alpha * (el.opacity / 100.0), 1.0));
                    QLinearGradient shapeGrad(r.topLeft(), r.bottomRight());
                    shapeGrad.setColorAt(0.0, c1);
                    shapeGrad.setColorAt(1.0, c2);
                    p.setPen(Qt::NoPen);
                    p.setBrush(shapeGrad);
                    p.drawPath(shapePath(el.shape, r));
                } else {
                    QColor c = baseColor;
                    c.setAlphaF(qBound(0.0, alpha * (el.opacity / 100.0), 1.0));
                    drawShapePrimitive(p, el.shape, r, c);
                }
                p.restore();
            }
            p.restore();
        } else {
            QRectF shapeRect(pad, pad, size, size);
            if (config.aspectRatio < 1.0) {
                double newH = size * config.aspectRatio;
                shapeRect = QRectF((canvasW - size) / 2.0, (canvasH - newH) / 2.0, size, newH);
            } else if (config.aspectRatio > 1.0) {
                double newW = size * config.aspectRatio;
                shapeRect = QRectF((canvasW - newW) / 2.0, (canvasH - size) / 2.0, newW, size);
            }
            p.save();
            p.translate(stamp.rect().center());
            p.rotate(config.angle);
            p.translate(-stamp.rect().center());
            if (config.shape == ShapeType::CustomStamp) {
                if (!config.customStampImage.isNull()) {
                    p.setOpacity(qBound(0.0, alpha, 1.0));
                    p.drawImage(shapeRect, config.customStampImage);
                    p.setOpacity(1.0);
                }
            } else if (doGradient) {
                QColor c1 = baseColor;
                c1.setAlphaF(qBound(0.0, alpha, 1.0));
                QColor c2 = secondColor;
                c2.setAlphaF(qBound(0.0, alpha, 1.0));
                QLinearGradient shapeGrad(shapeRect.topLeft(), shapeRect.bottomRight());
                shapeGrad.setColorAt(0.0, c1);
                shapeGrad.setColorAt(1.0, c2);
                p.setPen(Qt::NoPen);
                p.setBrush(shapeGrad);
                p.drawPath(shapePath(config.shape, shapeRect));
            } else {
                QColor stampColor = baseColor;
                stampColor.setAlphaF(qBound(0.0, alpha, 1.0));
                if (config.shape == ShapeType::Circle) {
                    QRadialGradient grad(stamp.rect().center(), size / 2.0);
                    QColor edge = stampColor;
                    edge.setAlpha(0);
                    grad.setColorAt(0.0, stampColor);
                    grad.setColorAt(0.8, stampColor);
                    grad.setColorAt(1.0, edge);
                    p.setPen(Qt::NoPen);
                    p.setBrush(grad);
                    p.drawEllipse(shapeRect);
                } else {
                    drawShapePrimitive(p, config.shape, shapeRect, stampColor);
                }
            }
            p.restore();
        }

        if (config.granulation) {
            p.setCompositionMode(QPainter::CompositionMode_SourceAtop);
            p.setRenderHint(QPainter::Antialiasing, false);
            double grainAngle = config.angle * M_PI / 180.0 + M_PI / 5.0;
            int numVeins = qMax(canvasW, canvasH) * 3;
            for (int i = 0; i < numVeins; ++i) {
                int cx = QRandomGenerator::global()->bounded(canvasW);
                int cy = QRandomGenerator::global()->bounded(canvasH);
                double a = grainAngle + (QRandomGenerator::global()->generateDouble() - 0.5) * 0.6;
                int len = 2 + QRandomGenerator::global()->bounded(qMax(3, qMax(canvasW, canvasH) / 8));
                double roll = QRandomGenerator::global()->generateDouble();
                QColor veinColor;
                if (roll < 0.5) veinColor = QColor(0, 0, 0, QRandomGenerator::global()->bounded(25, 70));
                else if (roll < 0.8) veinColor = QColor(255, 255, 255, QRandomGenerator::global()->bounded(15, 50));
                else veinColor = QColor(0, 0, 0, QRandomGenerator::global()->bounded(10, 35));
                p.setPen(QPen(veinColor, 1));
                p.drawLine(cx, cy, cx + (int)(cos(a) * len), cy + (int)(sin(a) * len));
            }
            int numSpecks = (canvasW * canvasH) / 20;
            for (int i = 0; i < numSpecks; ++i) {
                int nx = QRandomGenerator::global()->bounded(canvasW);
                int ny = QRandomGenerator::global()->bounded(canvasH);
                double roll = QRandomGenerator::global()->generateDouble();
                QColor speckColor;
                if (roll < 0.6) speckColor = QColor(0, 0, 0, QRandomGenerator::global()->bounded(15, 45));
                else speckColor = QColor(255, 255, 255, QRandomGenerator::global()->bounded(10, 35));
                p.setPen(speckColor);
                p.drawPoint(nx, ny);
            }
            p.setCompositionMode(QPainter::CompositionMode_SourceOver);
        }
        p.end();
        return stamp;
    }

    static void applyGraphitePencil(QImage &image, const QPoint &p1, const QPoint &p2,
                                    const QColor &color, int width, int opacity) {
        if (image.isNull()) return;
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);
        double dx = p2.x() - p1.x(), dy = p2.y() - p1.y();
        double dist = sqrt(dx * dx + dy * dy);
        int steps = qMax(1, (int)(dist / 1.5));
        double softness = 0.75;
        double coreRadius = width * (0.8 + softness * 0.7);
        double strokeAngle = atan2(dy, dx);
        for (int i = 0; i <= steps; ++i) {
            double t = (double)i / steps;
            double cx = p1.x() + t * dx;
            double cy = p1.y() + t * dy;
            int grains = 12;
            for (int g = 0; g < grains; ++g) {
                double ang = strokeAngle + (QRandomGenerator::global()->generateDouble() - 0.5) * 1.3;
                double r = pow(QRandomGenerator::global()->generateDouble(), 0.65) * coreRadius;
                int gx = (int)(cx + cos(ang) * r);
                int gy = (int)(cy + sin(ang) * r);
                if (gx < 0 || gx >= image.width() || gy < 0 || gy >= image.height()) continue;
                double pressure = 1.0 - (r / qMax(0.001, coreRadius));
                QColor gc = color;
                int h, s, l, a;
                gc.getHsl(&h, &s, &l, &a);
                int nl = qBound(0, l - (int)(35 * softness) + QRandomGenerator::global()->bounded(-18, 19), 255);
                gc.setHsl(h, (int)(s * 0.25), nl);
                gc.setAlpha((int)(opacity * (0.18 + 0.5 * softness) * (0.35 + 0.65 * pressure) * QRandomGenerator::global()->generateDouble()));
                painter.setPen(gc);
                int glen = 1 + QRandomGenerator::global()->bounded(3);
                painter.drawLine(gx, gy, gx + (int)(cos(ang) * glen), gy + (int)(sin(ang) * glen));
            }
        }
        painter.end();
    }

    static void applyEraserLine(QImage &image, const QPoint &p1, const QPoint &p2, int width, bool softEdge) {
        if (image.isNull()) return;
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        if (softEdge) {
            double dx = p2.x() - p1.x(), dy = p2.y() - p1.y();
            double dist = sqrt(dx * dx + dy * dy);
            int steps = qMax(1, (int)(dist / 2.0));
            for (int i = 0; i <= steps; ++i) {
                double t = (double)i / steps;
                int cx = (int)(p1.x() + t * dx);
                int cy = (int)(p1.y() + t * dy);
                QRadialGradient grad(cx, cy, width);
                grad.setColorAt(0.0, QColor(0, 0, 0, 255));
                grad.setColorAt(0.6, QColor(0, 0, 0, 180));
                grad.setColorAt(1.0, QColor(0, 0, 0, 0));
                painter.setPen(Qt::NoPen);
                painter.setBrush(grad);
                painter.drawEllipse(cx - width, cy - width, width * 2, width * 2);
            }
        } else {
            painter.setPen(QPen(QColor(0, 0, 0, 255), width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.drawLine(p1, p2);
        }
        painter.end();
    }

    static QColor sampleCanvasColor(const QImage &image, const QPoint &pos, int radius) {
        if (image.isNull()) return QColor();
        int x0 = qMax(0, pos.x() - radius);
        int y0 = qMax(0, pos.y() - radius);
        int x1 = qMin(image.width() - 1, pos.x() + radius);
        int y1 = qMin(image.height() - 1, pos.y() + radius);
        if (x0 > x1 || y0 > y1) return QColor();
        long long r = 0, g = 0, b = 0;
        int count = 0;
        int step = 1;
        if (radius > 8) step = 2;
        if (radius > 16) step = 3;
        if (image.format() == QImage::Format_ARGB32) {
            for (int y = y0; y <= y1; y += step) {
                const QRgb *line = reinterpret_cast<const QRgb*>(image.constScanLine(y));
                for (int x = x0; x <= x1; x += step) {
                    QRgb px = line[x];
                    int a = qAlpha(px);
                    if (a > 20) { r += qRed(px); g += qGreen(px); b += qBlue(px); count++; }
                }
            }
        } else {
            for (int y = y0; y <= y1; y += step) {
                for (int x = x0; x <= x1; x += step) {
                    QColor c = image.pixelColor(x, y);
                    if (c.alpha() > 20) { r += c.red(); g += c.green(); b += c.blue(); count++; }
                }
            }
        }
        if (count == 0) return QColor();
        return QColor((int)(r / count), (int)(g / count), (int)(b / count), 255);
    }

    static QImage wetMixStamp(const QImage &stamp, const QColor &canvasColor, double wetAmount) {
        QImage result = stamp.copy();
        if (result.isNull()) return result;
        double t = qBound(0.0, wetAmount / 100.0, 1.0);
        if (t <= 0.001) return result;
        if (result.format() != QImage::Format_ARGB32)
            result = result.convertToFormat(QImage::Format_ARGB32);
        int cr = canvasColor.red(), cg = canvasColor.green(), cb = canvasColor.blue();
        double inv = 1.0 - t;
        for (int y = 0; y < result.height(); ++y) {
            QRgb *line = reinterpret_cast<QRgb*>(result.scanLine(y));
            for (int x = 0; x < result.width(); ++x) {
                QRgb px = line[x];
                int a = qAlpha(px);
                if (a > 0) {
                    int nr = (int)(qRed(px) * inv + cr * t);
                    int ng = (int)(qGreen(px) * inv + cg * t);
                    int nb = (int)(qBlue(px) * inv + cb * t);
                    line[x] = qRgba(nr, ng, nb, a);
                }
            }
        }
        return result;
    }

    static void drawStampAt(QImage &image, const QPoint &pos, const QImage &drawStamp,
                            double angle, double opacity, double scale) {
        if (image.isNull() || drawStamp.isNull()) return;
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.save();
        painter.translate(pos.x(), pos.y());
        painter.rotate(angle);
        if (qAbs(scale - 1.0) > 0.001)
            painter.scale(scale, scale);
        painter.setOpacity(qBound(0.0, opacity, 1.0));
        painter.drawImage(-drawStamp.width() / 2.0, -drawStamp.height() / 2.0, drawStamp);
        painter.restore();
        painter.end();
    }

    static void applyCustomBrushStroke(QImage &image, const QPoint &pos,
                                       const QImage &stamp, const BrushSettings &config,
                                       double mouseSensitivity, double extraAngle = 0.0,
                                       double sizeScale = 1.0, double opacityScale = 1.0,
                                       const QColor &baseColor = QColor(),
                                       const QColor &secondColor = QColor(),
                                       const QColor &fallbackCanvasColor = QColor(),
                                       double taperFactor = 1.0) {
        Q_UNUSED(secondColor);
        if (image.isNull() || stamp.isNull()) return;
        int actX = pos.x(), actY = pos.y();
        if (config.scatter > 0) {
            int scatterAmount = (int)(config.scatter * mouseSensitivity);
            actX += QRandomGenerator::global()->bounded(-scatterAmount, scatterAmount + 1);
            actY += QRandomGenerator::global()->bounded(-scatterAmount, scatterAmount + 1);
        }
        double drawAngle = config.angle + extraAngle;
        if (config.rotationMode == RotationMode::Random)
            drawAngle += QRandomGenerator::global()->bounded(0, 360);
        if (config.angleJitter > 0)
            drawAngle += QRandomGenerator::global()->bounded(-config.angleJitter, config.angleJitter + 1);
        double drawOpacity = 1.0;
        if (config.isAirbrush) drawOpacity = 0.3;
        drawOpacity *= opacityScale;
        drawOpacity *= taperFactor;
        if (config.opacityJitter > 0) {
            double jitter = QRandomGenerator::global()->generateDouble() * (config.opacityJitter / 100.0);
            drawOpacity *= qBound(0.1, 1.0 - jitter, 1.0);
        }
        double scale = sizeScale * taperFactor;
        if (config.sizeJitter > 0) {
            double jitter = QRandomGenerator::global()->generateDouble() * (config.sizeJitter / 100.0);
            scale *= qBound(0.3, 1.0 - jitter + QRandomGenerator::global()->generateDouble() * jitter * 2, 1.7);
        }
        QImage drawStamp = stamp;
        if (config.wetMix && baseColor.isValid()) {
            int sampleRadius = qMax(2, config.size / 2);
            QColor sampled = sampleCanvasColor(image, QPoint(actX, actY), sampleRadius);
            if (!sampled.isValid() && fallbackCanvasColor.isValid())
                sampled = fallbackCanvasColor;
            if (sampled.isValid())
                drawStamp = wetMixStamp(stamp, sampled, config.wetAmount);
        }
        drawStampAt(image, QPoint(actX, actY), drawStamp, drawAngle, drawOpacity, scale);
    }

    static void applyCustomBrushLine(QImage &image, const QPointF &from, const QPointF &to,
                                     const QImage &stamp, const BrushSettings &config,
                                     double mouseSensitivity, QPointF &lastPoint,
                                     const QColor &baseColor = QColor(),
                                     const QColor &secondColor = QColor(),
                                     const QColor &fallbackCanvasColor = QColor(),
                                     double totalStrokeLength = 0.0,
                                     double accumulatedLength = 0.0) {
        Q_UNUSED(totalStrokeLength);
        if (image.isNull() || stamp.isNull()) return;
        double dist = sqrt(pow(to.x() - from.x(), 2) + pow(to.y() - from.y(), 2));
        if (dist < 0.5) return;
        double dirAngle = atan2(to.y() - from.y(), to.x() - from.x()) * 180.0 / M_PI;
        if (config.dragMode == DragMode::Ribbon) {
            QPainter painter(&image);
            painter.setRenderHint(QPainter::Antialiasing, true);
            QColor c = baseColor.isValid() ? baseColor : QColor(0, 0, 0);
            c.setAlphaF(qBound(0.0, config.opacity / 100.0, 1.0));
            double ribbonW = qMax(1.0, config.size * config.aspectRatio);
            painter.setPen(QPen(c, ribbonW, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
            painter.drawLine(from, to);
            painter.end();
            lastPoint = to;
            return;
        }
        double baseSpacing = qMax(1.0, (config.size * qMax(1, 6 - config.density)) / 100.0);
        switch (config.dragMode) {
        case DragMode::Continuous: baseSpacing = qMax(1.0, config.size * 0.08); break;
        case DragMode::Dotted:     baseSpacing = qMax((double)config.size * 2.0, baseSpacing * 2.5); break;
        case DragMode::Scattered:  baseSpacing = qMax(2.0, baseSpacing * 1.5); break;
        default: break;
        }
        baseSpacing *= (2.0 - mouseSensitivity);
        baseSpacing = qMax(1.0, baseSpacing);
        if (dist < baseSpacing) return;
        QImage lineStamp = stamp;
        if (config.wetMix && baseColor.isValid()) {
            QColor sampled = sampleCanvasColor(image, from.toPoint(), qMax(2, config.size / 2));
            if (!sampled.isValid() && fallbackCanvasColor.isValid())
                sampled = fallbackCanvasColor;
            if (sampled.isValid())
                lineStamp = wetMixStamp(stamp, sampled, config.wetAmount);
        }
        int steps = qMax(1, (int)(dist / baseSpacing));
        for (int s = 1; s <= steps; ++s) {
            double t = (double)s / steps;
            int cx = (int)(from.x() + t * (to.x() - from.x()));
            int cy = (int)(from.y() + t * (to.y() - from.y()));
            double taperFactor = 1.0;
            if (config.flow < 100) {
                double currentLen = accumulatedLength + t * dist;
                taperFactor = computeBreathFactor(currentLen, config.size, config.flow);
            }
            double extraAngle = 0.0;
            if (config.rotationMode == RotationMode::FollowDirection)
                extraAngle = dirAngle;
            double sizeScale = 1.0;
            if (config.dragMode == DragMode::Dotted)
                sizeScale = 0.7;
            int densityCount = qBound(1, config.density, 20);
            for (int d = 0; d < densityCount; ++d) {
                int dx = 0, dy = 0;
                if (densityCount > 1) {
                    int jitter = qMax(1, config.size / 4);
                    dx = QRandomGenerator::global()->bounded(-jitter, jitter + 1);
                    dy = QRandomGenerator::global()->bounded(-jitter, jitter + 1);
                }
                if (config.dragMode == DragMode::Scattered) {
                    int extraScatter = qMax(2, config.scatter + config.size / 2);
                    dx += QRandomGenerator::global()->bounded(-extraScatter, extraScatter + 1);
                    dy += QRandomGenerator::global()->bounded(-extraScatter, extraScatter + 1);
                }
                applyCustomBrushStroke(image, QPoint(cx + dx, cy + dy), lineStamp, config,
                                       1.0, extraAngle, sizeScale, 1.0,
                                       QColor(), secondColor, fallbackCanvasColor,
                                       taperFactor);
            }
        }
        lastPoint = to;
    }

    static void applyBlur(QImage &image, const QPoint &pos, int radius) {
        RetouchTools::applyBlur(image, pos, radius);
    }
    static void applyHeal(QImage &image, const QPoint &pos, int radius) {
        RetouchTools::applyHeal(image, pos, radius);
    }
    static void applyShadowBurn(QImage &image, const QPoint &pos, int radius, double sensitivity, int opacity) {
        RetouchTools::applyShadowBurn(image, pos, radius, sensitivity, opacity);
    }

    static void drawGeometry(QPainter &painter, const QPoint &p1, const QPoint &p2, ToolType tool) {
        QRect r = QRect(p1, p2).normalized();
        switch (tool) {
        case ToolLine: painter.drawLine(p1, p2); break;
        case ToolRectangle: painter.drawRect(r); break;
        case ToolEllipse: painter.drawEllipse(r); break;
        case ToolRoundRect: painter.drawRoundedRect(r, 12, 12); break;
        case ToolTriangle: {
            QPolygon t;
            t << QPoint((p1.x() + p2.x()) / 2, p1.y()) << QPoint(p1.x(), p2.y()) << QPoint(p2.x(), p2.y());
            painter.drawPolygon(t); break;
        }
        case ToolRightTriangle: {
            QPolygon t;
            t << p1 << QPoint(p1.x(), p2.y()) << p2;
            painter.drawPolygon(t); break;
        }
        case ToolDiamond: {
            QPolygon t;
            t << QPoint((p1.x() + p2.x()) / 2, p1.y()) << QPoint(p2.x(), (p1.y() + p2.y()) / 2)
              << QPoint((p1.x() + p2.x()) / 2, p2.y()) << QPoint(p1.x(), (p1.y() + p2.y()) / 2);
            painter.drawPolygon(t); break;
        }
        case ToolPentagon:
        case ToolHexagon:
        case ToolStar: {
            int sides = (tool == ToolPentagon) ? 5 : (tool == ToolHexagon) ? 6 : 10;
            QPolygon poly;
            for (int i = 0; i < sides; ++i) {
                double angle = -M_PI / 2 + i * 2 * M_PI / (tool == ToolStar ? 5 : sides);
                double f = (tool == ToolStar && i % 2 == 1) ? 0.45 : 1.0;
                poly << QPoint(r.center().x() + r.width() / 2 * f * cos(angle),
                               r.center().y() + r.height() / 2 * f * sin(angle));
            }
            painter.drawPolygon(poly); break;
        }
        case ToolArrowRight:
        case ToolArrowLeft: {
            bool right = (tool == ToolArrowRight);
            int ym = r.top() + r.height() / 2;
            int xb = right ? r.left() + r.width() * 0.55 : r.left() + r.width() * 0.45;
            int tk = r.height() * 0.25;
            QPolygon poly;
            poly << QPoint(right ? r.left() : r.right(), ym - tk) << QPoint(xb, ym - tk)
                 << QPoint(xb, r.top()) << QPoint(right ? r.right() : r.left(), ym)
                 << QPoint(xb, r.bottom()) << QPoint(xb, ym + tk)
                 << QPoint(right ? r.left() : r.right(), ym + tk);
            painter.drawPolygon(poly); break;
        }
        case ToolHeart: {
            QPainterPath path;
            path.moveTo(r.left() + r.width() / 2, r.top() + r.height() * 0.28);
            path.cubicTo(r.left() + r.width() * 0.1, r.top() - r.height() * 0.05,
                         r.left(), r.top() + r.height() * 0.6,
                         r.left() + r.width() / 2, r.bottom());
            path.cubicTo(r.right(), r.top() + r.height() * 0.6,
                         r.right() - r.width() * 0.1, r.top() - r.height() * 0.05,
                         r.left() + r.width() / 2, r.top() + r.height() * 0.28);
            painter.drawPath(path); break;
        }
        case ToolCube: {
            int offset = qMin(r.width(), r.height()) * 0.3;
            if (offset < 4) offset = 4;
            QRect front(r.left(), r.top() + offset, r.width() - offset, r.height() - offset);
            QRect back(r.left() + offset, r.top(), r.width() - offset, r.height() - offset);
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
    }

    static void floodFill(QImage &image, const QPoint &start, QColor fillCol) {
        floodFillPixelArt(image, start, fillCol);
    }
    static QImage magicWandMask(const QImage &image, const QPoint &pos, int tolerance) {
        return MagicWandTools::magicWandMask(image, pos, tolerance);
    }
};

// ============================================================
// PRESETS ARTÍSTICOS
// ============================================================
namespace ArtisticPresets {
inline BrushSettings watercolor() {
    BrushSettings s;
    s.size = 40; s.shape = ShapeType::Circle;
    s.dragMode = DragMode::Continuous; s.rotationMode = RotationMode::Random;
    s.opacity = 2; s.scatter = 25; s.angle = 0.0; s.density = 4; s.flow = 100;
    s.sizeJitter = 30; s.angleJitter = 45; s.opacityJitter = 25;
    s.aspectRatio = 1.0; s.wetMix = true; s.wetAmount = 50;
    return s;
}
inline BrushSettings oilBrush() {
    BrushSettings s;
    s.size = 30; s.shape = ShapeType::FlatTip;
    s.dragMode = DragMode::Continuous; s.rotationMode = RotationMode::FollowDirection;
    s.opacity = 95; s.scatter = 20; s.angle = 0.0; s.density = 2; s.flow = 100;
    s.sizeJitter = 10; s.angleJitter = 15; s.opacityJitter = 8;
    s.aspectRatio = 0.4; s.wetMix = true; s.wetAmount = 60;
    return s;
}
inline BrushSettings crayon() {
    BrushSettings s;
    s.size = 40; s.shape = ShapeType::Clover;
    s.dragMode = DragMode::Continuous; s.rotationMode = RotationMode::Random;
    s.opacity = 40; s.scatter = 15; s.angle = 40.0; s.density = 2; s.flow = 100;
    s.sizeJitter = 20; s.angleJitter = 30; s.opacityJitter = 100;
    s.aspectRatio = 1.0; s.wetMix = true; s.wetAmount = 70; s.granulation = true;
    return s;
}
inline BrushSettings marker() {
    BrushSettings s;
    s.size = 22; s.shape = ShapeType::RoundedSquare;
    s.dragMode = DragMode::Continuous; s.rotationMode = RotationMode::FollowDirection;
    s.opacity = 10; s.scatter = 1; s.angle = 0.0; s.density = 1; s.flow = 100;
    s.aspectRatio = 0.5; s.wetMix = true; s.wetAmount = 10; s.granulation = true;
    return s;
}
inline BrushSettings calligraphy() {
    BrushSettings s;
    s.size = 18; s.shape = ShapeType::ChiselTip;
    s.dragMode = DragMode::Continuous; s.rotationMode = RotationMode::Fixed;
    s.opacity = 90; s.angle = 45.0; s.density = 1; s.flow = 100; s.aspectRatio = 0.3;
    return s;
}
inline BrushSettings highlighter() {
    BrushSettings s;
    s.size = 28; s.shape = ShapeType::ChiselTip;
    s.dragMode = DragMode::Ribbon; s.rotationMode = RotationMode::Fixed;
    s.opacity = 40; s.angle = 0.0; s.density = 1; s.flow = 100; s.aspectRatio = 0.4;
    return s;
}
inline BrushSettings softBrush() {
    BrushSettings s;
    s.size = 30; s.shape = ShapeType::Circle;
    s.dragMode = DragMode::Continuous; s.rotationMode = RotationMode::Fixed;
    s.opacity = 85; s.scatter = 5; s.angle = 0.0; s.density = 1; s.flow = 100;
    s.sizeJitter = 10; s.opacityJitter = 5; s.aspectRatio = 1.0;
    return s;
}
inline BrushSettings sprayCan() {
    BrushSettings s;
    s.size = 20; s.shape = ShapeType::Circle;
    s.dragMode = DragMode::Scattered; s.rotationMode = RotationMode::Random;
    s.opacity = 70; s.scatter = 40; s.angle = 0.0; s.density = 3; s.flow = 100; s.isAirbrush = true;
    s.sizeJitter = 50; s.angleJitter = 180; s.opacityJitter = 40; s.aspectRatio = 1.0;
    return s;
}
inline BrushSettings presetForTool(ToolType t) {
    switch (t) {
    case ToolWatercolor:  return watercolor();
    case ToolOilBrush:    return oilBrush();
    case ToolCrayon:      return crayon();
    case ToolMarker:      return marker();
    case ToolCalligraphy: return calligraphy();
    case ToolHighlighter: return highlighter();
    case ToolBrush:       return softBrush();
    case ToolSpray:       return sprayCan();
    default:              return softBrush();
    }
}
inline bool isArtisticTool(ToolType t) {
    return t == ToolWatercolor || t == ToolOilBrush || t == ToolCrayon ||
           t == ToolMarker || t == ToolCalligraphy || t == ToolHighlighter ||
           t == ToolBrush || t == ToolSpray;
}
}

// ============================================================
// SHAPEBUTTON
// ============================================================
class ShapeButton : public QPushButton {
    Q_OBJECT
public:
    ShapeType shape;
    QImage customImage;
    bool isImportButton = false;
    bool isEmptyImport = true;

    ShapeButton(ShapeType s, QWidget *parent = nullptr, const QImage &img = QImage())
        : QPushButton(parent), shape(s), customImage(img) {
        setFixedSize(38, 38);
        setCheckable(true);
        setCursor(Qt::PointingHandCursor);
        setToolTip(PaintEngine::shapeName(s) + tr(" (arrastra para componer)"));
        setStyleSheet("QPushButton { background: transparent; border: none; }");
    }
    void setImportButton(bool isImport) {
        isImportButton = isImport;
        if (isImport) {
            setCheckable(true);
            setCursor(Qt::PointingHandCursor);
            setContextMenuPolicy(Qt::CustomContextMenu);
        }
    }
    void setCustomImage(const QImage &img) {
        customImage = img;
        isEmptyImport = img.isNull();
        update();
    }
    void setDarkMode(bool dark) { m_dark = dark; update(); }

protected:
    void mousePressEvent(QMouseEvent *e) override {
        dragStartPos = e->pos();
        QPushButton::mousePressEvent(e);
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if (isImportButton && isEmptyImport) return;
        if (!(e->buttons() & Qt::LeftButton)) return;
        if ((e->pos() - dragStartPos).manhattanLength() < QApplication::startDragDistance()) return;
        QDrag *drag = new QDrag(this);
        QMimeData *mime = new QMimeData;
        mime->setData("application/x-shape-type", QByteArray::number((int)shape));
        drag->setMimeData(mime);
        QPixmap pm = grab();
        drag->setPixmap(pm);
        drag->setHotSpot(e->pos());
        drag->exec(Qt::CopyAction);
    }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);

        QColor bg, border, fig;
        if (isImportButton && isEmptyImport) {
            // Vacío: verde llamativo
            if (underMouse()) {
                bg = m_dark ? QColor("#2a4a2a") : QColor("#dcfce7");
                border = QColor("#22c55e");
                fig = QColor("#22c55e");
            } else {
                bg = m_dark ? QColor("#1f3a1f") : QColor("#f0fdf4");
                border = QColor("#16a34a");
                fig = QColor("#22c55e");
            }
        } else if (isImportButton && !isEmptyImport) {
            // Con PNG: mismo estilo pero borde verde
            if (isChecked()) {
                bg = m_dark ? QColor("#1a3a5c") : QColor("#eff6ff");
                border = QColor("#3b82f6");
            } else if (underMouse()) {
                bg = m_dark ? QColor("#3a3a3a") : QColor("#f1f5f9");
                border = QColor("#22c55e");
            } else {
                bg = m_dark ? QColor("#2a2a2a") : QColor("#ffffff");
                border = QColor("#22c55e");
            }
        } else if (isChecked()) {
            bg     = m_dark ? QColor("#1a3a5c") : QColor("#eff6ff");
            border = QColor("#3b82f6");
        } else if (underMouse()) {
            bg     = m_dark ? QColor("#3a3a3a") : QColor("#f1f5f9");
            border = m_dark ? QColor("#555555") : QColor("#cbd5e1");
        } else {
            bg     = m_dark ? QColor("#2a2a2a") : QColor("#ffffff");
            border = m_dark ? QColor("#3a3a3a") : QColor("#d1d5db");
        }
        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 6, 6);
        p.setPen(QPen(border, isChecked() || (isImportButton && isEmptyImport) ? 2 : 1));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(rect().adjusted(1, 1, -2, -2), 6, 6);

        if (isImportButton && isEmptyImport) {
            const int cx = width() / 2;
            const int cy = height() / 2 - 4;
            QColor fg = (m_dark || !underMouse()) ? QColor("#22c55e") : QColor("#16a34a");
            if (m_dark) fg = QColor("#22c55e");
            else fg = underMouse() ? QColor("#15803d") : QColor("#16a34a");
            p.setPen(QPen(fg, 2.5, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(cx - 8, cy, cx + 8, cy);
            p.drawLine(cx, cy - 8, cx, cy + 8);
            p.setPen(fg);
            QFont f = p.font();
            f.setPointSize(7);
            f.setBold(true);
            p.setFont(f);
            p.drawText(QRect(0, height() - 14, width(), 12), Qt::AlignCenter, tr("PNG"));
        } else if (shape == ShapeType::CustomStamp && !customImage.isNull()) {
            // ⭐ PNG TAL CUAL, sin tintar
            QRectF shapeRect(4, 4, width() - 8, height() - 8);
            QImage scaled = customImage.scaled(shapeRect.size().toSize(),
                                               Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QRectF target(shapeRect.center().x() - scaled.width() / 2.0,
                          shapeRect.center().y() - scaled.height() / 2.0,
                          scaled.width(), scaled.height());
            p.drawImage(target, scaled);
        } else {
            QRectF shapeRect(7, 7, width() - 14, height() - 14);
            QColor fg = fig;
            if (isChecked()) fg = m_dark ? QColor("#60a5fa") : QColor("#1d4ed8");
            else if (underMouse()) fg = m_dark ? QColor("#c0c0c0") : QColor("#475569");
            else fg = m_dark ? QColor("#c0c0c0") : QColor("#475569");
            PaintEngine::drawShapePrimitive(p, shape, shapeRect, fg);
        }
    }
private:
    QPoint dragStartPos;
    bool m_dark = true;
};

// ============================================================
// SHAPEPREVIEW
// ============================================================
class ShapePreview : public QWidget {
    Q_OBJECT
private:
    QImage stampImage;
    bool m_dark = true;
public:
    ShapePreview(QWidget *parent = nullptr) : QWidget(parent) {
        setFixedSize(100, 100);
        setToolTip(tr("Stamp del pincel"));
    }
    void updateStamp(const QImage &stamp) { stampImage = stamp; update(); }
    void setDarkMode(bool dark) { m_dark = dark; update(); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p.fillRect(rect(), m_dark ? QColor("#1e1e1e") : QColor("#f3f4f6"));
        if (!stampImage.isNull()) {
            int maxW = width() - 12;
            int maxH = height() - 12;
            QImage scaled = stampImage.scaled(maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            p.drawImage((width() - scaled.width()) / 2, (height() - scaled.height()) / 2, scaled);
        }
        p.setPen(QPen(m_dark ? QColor("#3a3a3a") : QColor("#d1d5db"), 1));
        p.setBrush(Qt::NoBrush);
        p.drawRect(rect().adjusted(0, 0, -1, -1));
    }
};

// ============================================================
// COMPOSITEEDITOR
// ============================================================
class CompositeEditor : public QWidget {
    Q_OBJECT
private:
    QVector<ShapeElement> elements;
    int selectedIndex = -1;
    bool draggingElement = false;
    bool m_dark = true;
public:
    CompositeEditor(QWidget *parent = nullptr) : QWidget(parent) {
        setFixedSize(180, 180);
        setAcceptDrops(true);
        setCursor(Qt::CrossCursor);
    }
    void setElements(const QVector<ShapeElement> &elems) {
        elements = elems;
        if (selectedIndex >= elements.size())
            selectedIndex = elements.isEmpty() ? -1 : 0;
        update();
    }
    QVector<ShapeElement> getElements() const { return elements; }
    int getSelectedIndex() const { return selectedIndex; }
    void setSelectedIndex(int idx) { selectedIndex = idx; update(); emit selectionChanged(selectedIndex); }
    void addElement(ShapeType shape, double ox, double oy, const QImage &customImg = QImage()) {
        ShapeElement el;
        el.shape = shape;
        el.offsetX = ox;
        el.offsetY = oy;
        el.customImage = customImg;
        elements.append(el);
        selectedIndex = elements.size() - 1;
        update();
        emit elementsChanged();
        emit selectionChanged(selectedIndex);
    }
    void removeSelected() {
        if (selectedIndex >= 0 && selectedIndex < elements.size()) {
            elements.removeAt(selectedIndex);
            selectedIndex = elements.isEmpty() ? -1 : qMin(selectedIndex, elements.size() - 1);
            update();
            emit elementsChanged();
            emit selectionChanged(selectedIndex);
        }
    }
    void updateSelectedElement(const ShapeElement &el) {
        if (selectedIndex >= 0 && selectedIndex < elements.size()) {
            elements[selectedIndex] = el;
            update();
            emit elementsChanged();
        }
    }
    void clearAll() {
        elements.clear();
        selectedIndex = -1;
        update();
        emit elementsChanged();
        emit selectionChanged(-1);
    }
    int getElementCount() const { return elements.size(); }
    void setDarkMode(bool dark) { m_dark = dark; update(); }
signals:
    void elementsChanged();
    void selectionChanged(int index);
    void requestCustomStamp(ShapeType shape, const QImage &img);
protected:
    void dragEnterEvent(QDragEnterEvent *e) override {
        if (e->mimeData()->hasFormat("application/x-shape-type"))
            e->acceptProposedAction();
    }
    void dropEvent(QDropEvent *e) override {
        if (e->mimeData()->hasFormat("application/x-shape-type")) {
            int shapeInt = e->mimeData()->data("application/x-shape-type").toInt();
            ShapeType s = (ShapeType)shapeInt;
            QPointF localPos = e->position();
            double ox = qBound(-1.0, (localPos.x() - width() / 2.0) / (width() / 2.0), 1.0);
            double oy = qBound(-1.0, (localPos.y() - height() / 2.0) / (height() / 2.0), 1.0);
            if (s == ShapeType::CustomStamp) {
                emit requestCustomStamp(s, QImage());
            } else {
                addElement(s, ox, oy);
            }
            e->acceptProposedAction();
        }
    }
    void mousePressEvent(QMouseEvent *e) override {
        QPointF pos = e->position();
        int best = -1;
        double bestDist = 1e9;
        for (int i = 0; i < elements.size(); ++i) {
            double ex = width() / 2.0 + elements[i].offsetX * (width() / 2.0);
            double ey = height() / 2.0 + elements[i].offsetY * (height() / 2.0);
            double d = sqrt(pow(pos.x() - ex, 2) + pow(pos.y() - ey, 2));
            if (d < bestDist && d < 50) { bestDist = d; best = i; }
        }
        if (best >= 0) {
            selectedIndex = best;
            draggingElement = true;
            emit selectionChanged(selectedIndex);
            update();
        }
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if (!draggingElement || selectedIndex < 0 || selectedIndex >= elements.size()) return;
        QPointF pos = e->position();
        elements[selectedIndex].offsetX = qBound(-1.0, (pos.x() - width() / 2.0) / (width() / 2.0), 1.0);
        elements[selectedIndex].offsetY = qBound(-1.0, (pos.y() - height() / 2.0) / (height() / 2.0), 1.0);
        update();
        emit elementsChanged();
    }
    void mouseReleaseEvent(QMouseEvent *) override { draggingElement = false; }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p.fillRect(rect(), m_dark ? QColor("#1e1e1e") : QColor("#f3f4f6"));
        p.setPen(QPen(m_dark ? QColor("#373737") : QColor("#e2e8f0"), 1, Qt::DashLine));
        p.drawLine(width() / 2, 0, width() / 2, height());
        p.drawLine(0, height() / 2, width(), height() / 2);
        double baseSize = qMin(width(), height()) * 0.20;
        for (int i = 0; i < elements.size(); ++i) {
            const ShapeElement &el = elements[i];
            double cx = width() / 2.0 + el.offsetX * (width() / 2.0);
            double cy = height() / 2.0 + el.offsetY * (height() / 2.0);
            double sz = baseSize * el.scale;
            QRectF r(cx - sz / 2, cy - sz / 2, sz, sz);
            p.save();
            p.translate(cx, cy);
            p.rotate(el.rotation);
            p.translate(-cx, -cy);

            if (el.shape == ShapeType::CustomStamp && !el.customImage.isNull()) {
                // ⭐ PNG TAL CUAL con opacidad
                p.setOpacity(el.opacity / 100.0);
                p.drawImage(r, el.customImage);
                p.setOpacity(1.0);
            } else {
                QColor c = (i == selectedIndex) ? QColor("#3b82f6")
                                                : (m_dark ? QColor("#d0d0d0") : QColor("#475569"));
                c.setAlpha((int)(el.opacity * 2.55));
                PaintEngine::drawShapePrimitive(p, el.shape, r, c);
            }
            if (i == selectedIndex) {
                p.setPen(QPen(QColor("#3b82f6"), 1, Qt::DashLine));
                p.setBrush(Qt::NoBrush);
                p.drawRect(r.adjusted(-3, -3, 3, 3));
            }
            p.restore();
        }
        p.setPen(QPen(m_dark ? QColor("#3a3a3a") : QColor("#d1d5db"), 1));
        p.setBrush(Qt::NoBrush);
        p.drawRect(rect().adjusted(0, 0, -1, -1));
        if (elements.isEmpty()) {
            p.setPen(m_dark ? QColor("#8a8a8a") : QColor("#9ca3af"));
            p.setFont(QFont("Adwaita Sans", 8));
            p.drawText(rect(), Qt::AlignCenter, tr("Arrastra figuras\naqui para componer"));
        }
    }
};

// ============================================================
// STROKEPREVIEW
// ============================================================
class StrokePreview : public QWidget {
    Q_OBJECT
private:
    BrushSettings settings;
    QColor brushColor;
    QColor secondColor;
    QImage cachedPreview;
    bool dirty = true;
    bool m_dark = true;
public:
    StrokePreview(QWidget *parent = nullptr)
        : QWidget(parent), brushColor(Qt::black) {
        setMinimumHeight(100);
        setMinimumWidth(200);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
    void updatePreview(const BrushSettings &s, const QColor &c, const QColor &c2 = QColor()) {
        settings = s;
        brushColor = c;
        secondColor = c2;
        dirty = true;
        update();
    }
    void setDarkMode(bool dark) { m_dark = dark; dirty = true; update(); }
protected:
    void rebuildCache() {
        if (width() <= 4 || height() <= 4) { cachedPreview = QImage(); dirty = false; return; }
        QImage img(width(), height(), QImage::Format_ARGB32);
        img.fill(m_dark ? QColor("#1e1e1e") : QColor("#f3f4f6"));
        BrushSettings previewSettings = settings;
        previewSettings.density = qMin(previewSettings.density, 3);
        previewSettings.size = qMin(previewSettings.size, 28);
        QImage stamp = PaintEngine::generateBrushStamp(previewSettings, brushColor, 255, false, secondColor);
        int steps = qBound(24, width() / 10, 42);
        QVector<QPointF> points;
        points.reserve(steps + 1);
        for (int i = 0; i <= steps; ++i) {
            double t = (double)i / steps;
            double x = 20 + t * (width() - 40);
            double y = height() / 2.0 + sin(t * M_PI * 2.0) * (height() * 0.22);
            points.append(QPointF(x, y));
        }
        QPointF last(-1000, -1000);
        double accum = 0.0;
        for (int i = 0; i < points.size(); ++i) {
            QPointF cur = points[i];
            if (i == 0) {
                last = cur;
                PaintEngine::applyCustomBrushStroke(img, cur.toPoint(), stamp, previewSettings,
                                                    1.0, 0.0, 1.0, 1.0,
                                                    brushColor, secondColor, QColor(), 1.0);
            } else {
                double segLen = QLineF(last, cur).length();
                PaintEngine::applyCustomBrushLine(img, last, cur, stamp, previewSettings,
                                                  1.0, last, brushColor, secondColor, QColor(),
                                                  0.0, accum);
                accum += segLen;
                last = cur;
            }
        }
        cachedPreview = img;
        dirty = false;
    }
    void paintEvent(QPaintEvent *) override {
        if (dirty || cachedPreview.size() != size()) rebuildCache();
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.fillRect(rect(), m_dark ? QColor("#1e1e1e") : QColor("#f3f4f6"));
        if (!cachedPreview.isNull()) p.drawImage(0, 0, cachedPreview);
        p.setPen(QPen(m_dark ? QColor("#3a3a3a") : QColor("#d1d5db"), 1));
        p.setBrush(Qt::NoBrush);
        p.drawRect(rect().adjusted(0, 0, -1, -1));
        p.setPen(m_dark ? QColor("#c8c8c8") : QColor("#475569"));
        p.setFont(QFont("Adwaita Sans", 8));
        QString label = PaintEngine::shapeName(settings.shape);
        if (!settings.shapeElements.isEmpty())
            label = tr("Compuesto: %1 figuras").arg(settings.shapeElements.size());
        if (settings.mixSecondColor) label += tr(" + 2do color");
        if (settings.wetMix) label += tr(" + Wet");
        if (settings.granulation) label += tr(" + Granulado");
        if (settings.flow < 100) label += tr(" | Flujo %1%").arg(settings.flow);
        p.drawText(rect().adjusted(6, 4, -6, -4), Qt::AlignLeft | Qt::AlignTop, label);
    }
};

// ============================================================
// JITTERDIALOG
// ============================================================
class JitterDialog : public QDialog {
    Q_OBJECT
private:
    QSlider *sizeSlider, *angleSlider, *opacitySlider;
public:
    JitterDialog(bool darkMode, int sizeJ, int angleJ, int opacityJ, QWidget *parent = nullptr)
        : QDialog(parent) {
        setWindowTitle(tr("Variacion aleatoria (jitter)"));
        setFixedSize(360, 250);
        const QString bg     = darkMode ? "#1a1a1a" : "#f5f5f5";
        const QString input  = darkMode ? "#2a2a2a" : "#ffffff";
        const QString txt    = darkMode ? "#e5e5e5" : "#111827";
        const QString muted  = darkMode ? "#a0a0a0" : "#6b7280";
        const QString border = darkMode ? "#3a3a3a" : "#d1d5db";
        const QString accent = darkMode ? "#3b82f6" : "#2563eb";
        const QString groove = darkMode ? "#444444" : "#d1d5db";
        setStyleSheet(QString(
            "QDialog { background-color: %1; }"
            "QLabel { color: %2; font-size: 12px; background: transparent; }"
            "QSlider::groove:horizontal { height: 4px; background: %3; border-radius: 2px; }"
            "QSlider::sub-page:horizontal { background: %4; border-radius: 2px; }"
            "QSlider::handle:horizontal { background: %4; width: 14px; height: 14px; margin: -5px 0; border-radius: 7px; }"
            "QPushButton { background-color: %5; color: %2; border: 1px solid %6; border-radius: 5px; padding: 5px 12px; font-size: 12px; }"
            "QPushButton:hover { border: 1px solid %4; }")
            .arg(bg, txt, groove, accent, input, border));
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(16, 16, 16, 14);
        layout->setSpacing(10);
        auto addRow = [&](const QString &label, int min, int max, int val,
                          QSlider **out, const QString &suffix) {
            QVBoxLayout *box = new QVBoxLayout();
            box->setSpacing(3);
            QHBoxLayout *top = new QHBoxLayout();
            top->addWidget(new QLabel(label));
            top->addStretch();
            QLabel *v = new QLabel(QString::number(val) + suffix);
            v->setStyleSheet(QString("color: %1; font-size: 11px;").arg(muted));
            v->setMinimumWidth(44);
            v->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            top->addWidget(v);
            box->addLayout(top);
            QSlider *s = new QSlider(Qt::Horizontal);
            s->setRange(min, max);
            s->setValue(val);
            box->addWidget(s);
            layout->addLayout(box);
            connect(s, &QSlider::valueChanged, v, [v, suffix](int vv) {
                v->setText(QString::number(vv) + suffix);
            });
            *out = s;
        };
        addRow(tr("Tamano"), 0, 100, sizeJ, &sizeSlider, "%");
        addRow(tr("Angulo"), 0, 180, angleJ, &angleSlider, QString::fromUtf8("\xC2\xB0"));
        addRow(tr("Opacidad"), 0, 100, opacityJ, &opacitySlider, "%");
        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }
    int getSizeJitter() const { return sizeSlider->value(); }
    int getAngleJitter() const { return angleSlider->value(); }
    int getOpacityJitter() const { return opacitySlider->value(); }
};

// ============================================================
// CUSTOMBRUSHESDIALOG
// ============================================================
class CustomBrushesDialog : public QDialog {
    Q_OBJECT
private:
    BrushSettings presets[2];
    int activeIndex;
    bool m_dark;
    QColor previewColor, previewSecondColor;
    QString c_bg, c_panel, c_input, c_text, c_textMuted, c_border, c_borderStrong;
    QString c_accent, c_accentHover, c_hover, c_groove;
    QRadioButton *radio1, *radio2;
    StrokePreview *preview;
    ShapePreview *shapePreview;
    QGridLayout *shapeGrid;
    QList<ShapeButton*> shapeButtons;
    QButtonGroup *shapeGroup;
    QHBoxLayout *sizeRow;
    QLabel *sizeLabel;
    QSpinBox *sizeSpin;
    QSlider *opacitySlider, *scatterSlider, *angleSlider, *densitySlider, *flowSlider, *aspectSlider;
    QLabel *opacityLabel, *scatterLabel, *angleLabel, *densityLabel, *aspectLabel, *flowLabel;
    QComboBox *dragCombo, *rotationCombo;
    QCheckBox *airbrushCheck, *wetCheck, *granulationCheck, *mixColorCheck;
    QSlider *wetSlider;
    QLabel *wetLabel;
    QPushButton *btnJitter, *btnRemoveElement, *btnClearComposite;
    CompositeEditor *compositeEditor;
    QSlider *elemScaleSlider, *elemRotationSlider, *elemOpacitySlider;
    QLabel *elemScaleLabel, *elemRotationLabel, *elemOpacityLabel, *compositeInfo;
    QTimer *previewTimer = nullptr;

    QImage customStampImage;
    ShapeButton *importButton = nullptr;

    void setupTheme(bool dark) {
        m_dark = dark;
        if (dark) {
            c_bg = "#1a1a1a"; c_panel = "#242424"; c_input = "#2a2a2a";
            c_text = "#e5e5e5"; c_textMuted = "#a0a0a0";
            c_border = "#3a3a3a"; c_borderStrong = "#555555";
            c_accent = "#3b82f6"; c_accentHover = "#2563eb";
            c_hover = "#3a3a3a"; c_groove = "#444444";
        } else {
            c_bg = "#f5f5f5"; c_panel = "#ffffff"; c_input = "#ffffff";
            c_text = "#111827"; c_textMuted = "#6b7280";
            c_border = "#d1d5db"; c_borderStrong = "#9ca3af";
            c_accent = "#2563eb"; c_accentHover = "#1d4ed8";
            c_hover = "#e5e7eb"; c_groove = "#d1d5db";
        }
    }
    void applyStyleSheet() {
        QString comboTxt = m_dark ? "#e0e0e0" : "#1e293b";
        QString qss;
        qss += QString("QDialog { background-color: %1; font-family: 'Adwaita Sans', 'Noto Sans', sans-serif; }").arg(c_bg);
        qss += QString("QDialog QLabel { color: %1; font-size: 12px; background: transparent; }").arg(c_text);
        qss += QString("QCheckBox { color: %1; font-size: 12px; spacing: 6px; background: transparent; padding: 3px 2px; }").arg(c_text);
        qss += QString("QCheckBox::indicator { width: 15px; height: 15px; border: 1px solid %1; border-radius: 3px; background-color: %2; }").arg(c_borderStrong, c_input);
        qss += QString("QCheckBox::indicator:hover { border: 1px solid %1; }").arg(c_accent);
        qss += QString("QCheckBox::indicator:checked { background-color: %1; border: 1px solid %1; }").arg(c_accent);
        qss += QString("QRadioButton { color: %1; font-size: 13px; spacing: 6px; background: transparent; }").arg(c_text);
        qss += QString("QRadioButton::indicator { width: 15px; height: 15px; border: 1px solid %1; border-radius: 8px; background-color: %2; }").arg(c_borderStrong, c_input);
        qss += QString("QRadioButton::indicator:checked { background-color: %1; border: 1px solid %1; }").arg(c_accent);
        qss += QString("QSlider::groove:horizontal { height: 4px; background: %1; border-radius: 2px; }").arg(c_groove);
        qss += QString("QSlider::sub-page:horizontal { background: %1; border-radius: 2px; }").arg(c_accent);
        qss += QString("QSlider::handle:horizontal { background: %1; width: 14px; height: 14px; margin: -5px 0; border-radius: 7px; }").arg(c_accent);
        qss += QString("QSlider::handle:horizontal:hover { background: %1; }").arg(c_accentHover);
        qss += QString("QComboBox { background-color: %1; color: %2; border: 1px solid %3; border-radius: 5px; padding: 4px 8px; font-size: 12px; min-height: 22px; }").arg(c_input, comboTxt, c_border);
        qss += QString("QComboBox:hover { border: 1px solid %1; }").arg(c_accent);
        qss += QString("QComboBox::drop-down { border: none; width: 16px; }");
        qss += QString("QComboBox QAbstractItemView { background-color: %1; color: %2; border: 1px solid %3; selection-background-color: %4; selection-color: #ffffff; outline: none; }").arg(c_input, comboTxt, c_border, c_accent);
        qss += QString("QSpinBox { background-color: %1; color: %2; border: 1px solid %3; border-radius: 5px; padding: 4px 8px; font-size: 12px; font-weight: 600; }").arg(c_input, c_text, c_border);
        qss += QString("QSpinBox:focus { border: 1px solid %1; }").arg(c_accent);
        qss += QString("QToolTip { background-color: %1; color: %2; border: 1px solid %3; padding: 3px; font-size: 12px; }").arg(c_panel, c_text, c_border);
        setStyleSheet(qss);
    }
    QString labelStyle() const { return QString("color: %1; font-size: 11px; background: transparent;").arg(c_text); }
    QString titleStyle() const { return QString("color: %1; font-size: 10px; font-weight: 700; letter-spacing: 1.5px; background: transparent;").arg(c_textMuted); }
    QString sliderStyle() const {
        return QString("QSlider::groove:horizontal { height: 4px; background: %1; border-radius: 2px; }"
                       "QSlider::sub-page:horizontal { background: %2; border-radius: 2px; }"
                       "QSlider::handle:horizontal { background: %2; width: 14px; height: 14px; margin: -5px 0; border-radius: 7px; }")
            .arg(c_groove, c_accent);
    }
    QString comboStyle() const {
        QString txt = m_dark ? "#e0e0e0" : "#1e293b";
        return QString("QComboBox { background-color: %1; color: %2; border: 1px solid %3; border-radius: 5px; padding: 4px 8px; font-size: 12px; min-height: 22px; }")
            .arg(c_input, txt, c_border);
    }
    QString smallBtnStyle() const {
        QString txt = m_dark ? "#e0e0e0" : "#1e293b";
        return QString("QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: 5px; padding: 5px 10px; font-size: 11px; }"
                       "QPushButton:hover { background-color: %4; border: 1px solid %5; }"
                       "QPushButton:pressed { background-color: %4; }"
                       "QPushButton:disabled { color: %6; border: 1px solid %3; }")
            .arg(c_input, txt, c_border, c_hover, c_accent, c_textMuted);
    }
    QString accentBtnStyle() const {
        return QString("QPushButton { background-color: %1; color: white; border: none; border-radius: 5px; padding: 6px 14px; font-size: 13px; font-weight: 600; }"
                       "QPushButton:hover { background-color: %2; }"
                       "QPushButton:pressed { background-color: %2; }")
            .arg(c_accent, c_accentHover);
    }
    QLabel *makeSectionTitle(const QString &t) {
        QLabel *l = new QLabel(t);
        l->setStyleSheet(titleStyle());
        return l;
    }
    QSlider *makeSlider(int min, int max, int val) {
        QSlider *s = new QSlider(Qt::Horizontal);
        s->setRange(min, max);
        s->setValue(val);
        s->setFixedHeight(16);
        s->setStyleSheet(sliderStyle());
        return s;
    }

    void buildShapeGrid() {
        QList<ShapeType> shapes = PaintEngine::allShapes();
        const int cols = 7;
        int row = 0, col = 0;

        for (int i = 0; i < shapes.size(); ++i) {
            ShapeButton *btn = new ShapeButton(shapes[i]);
            btn->setDarkMode(m_dark);
            shapeButtons.append(btn);
            shapeGroup->addButton(btn, i);
            shapeGrid->addWidget(btn, row, col);
            col++;
            if (col >= cols) { col = 0; row++; }
        }

        importButton = new ShapeButton(ShapeType::CustomStamp);
        importButton->setDarkMode(m_dark);
        importButton->setImportButton(true);
        importButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        importButton->setMinimumWidth(80);
        importButton->setToolTip(tr("Click: importar PNG\nClic derecho: quitar PNG actual"));

        shapeGroup->addButton(importButton, shapes.size());
        shapeButtons.append(importButton);

        shapeGrid->addWidget(importButton, row, col, 1, cols - col);

        connect(importButton, &QPushButton::clicked, this, [this]() {
            importPngAsStamp(true);
        });

        connect(importButton, &QPushButton::customContextMenuRequested, this, [this](const QPoint &pos) {
            if (customStampImage.isNull()) return;
            QMenu menu(this);
            QAction *actRemove = menu.addAction(tr("Quitar PNG actual"));
            QAction *chosen = menu.exec(importButton->mapToGlobal(pos));
            if (chosen == actRemove) {
                customStampImage = QImage();
                importButton->setCustomImage(QImage());
                importButton->setToolTip(tr("Click: importar PNG"));
                saveControlsToPreset();
            }
        });
    }

    void importPngAsStamp(bool replace) {
        Q_UNUSED(replace);
        const QString path = QFileDialog::getOpenFileName(
            this, tr("Importar PNG como stamp"),
            QString(),
            tr("Imagenes PNG (*.png);;Todas las imagenes (*.png *.jpg *.jpeg *.bmp)"));
        if (path.isEmpty()) return;
        QImage img(path);
        if (img.isNull()) {
            QMessageBox::warning(this, tr("Error"), tr("No se pudo cargar el PNG."));
            return;
        }
        if (img.width() > 256 || img.height() > 256) {
            img = img.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        customStampImage = img;
        importButton->setCustomImage(img);
        importButton->setToolTip(tr("PNG: %1x%2\nClick: reemplazar\nClic derecho: quitar").arg(img.width()).arg(img.height()));

        importButton->setChecked(true);
        saveControlsToPreset();
    }

    void loadControlsFromPreset() {
        const BrushSettings &s = presets[activeIndex];
        sizeSpin->blockSignals(true); sizeSpin->setValue(s.size); sizeSpin->blockSignals(false);
        opacitySlider->blockSignals(true); opacitySlider->setValue(s.opacity); opacitySlider->blockSignals(false);
        opacityLabel->setText(tr("Opacidad: %1%").arg(s.opacity));
        scatterSlider->blockSignals(true); scatterSlider->setValue(s.scatter); scatterSlider->blockSignals(false);
        scatterLabel->setText(tr("Dispersion: %1").arg(s.scatter));
        angleSlider->blockSignals(true); angleSlider->setValue((int)s.angle); angleSlider->blockSignals(false);
        angleLabel->setText(tr("Angulo: %1%2").arg((int)s.angle).arg(QString::fromUtf8("\xC2\xB0")));
        densitySlider->blockSignals(true); densitySlider->setValue(s.density); densitySlider->blockSignals(false);
        densityLabel->setText(tr("Figuras por paso: %1").arg(s.density));
        flowSlider->blockSignals(true); flowSlider->setValue(s.flow); flowSlider->blockSignals(false);
        flowLabel->setText(tr("Flujo: %1%").arg(s.flow));
        aspectSlider->blockSignals(true); aspectSlider->setValue((int)(s.aspectRatio * 100)); aspectSlider->blockSignals(false);
        aspectLabel->setText(tr("Proporcion: %1%").arg((int)(s.aspectRatio * 100)));
        dragCombo->blockSignals(true); dragCombo->setCurrentIndex((int)s.dragMode); dragCombo->blockSignals(false);
        rotationCombo->blockSignals(true); rotationCombo->setCurrentIndex((int)s.rotationMode); rotationCombo->blockSignals(false);
        airbrushCheck->blockSignals(true); airbrushCheck->setChecked(s.isAirbrush); airbrushCheck->blockSignals(false);
        wetCheck->blockSignals(true); wetCheck->setChecked(s.wetMix); wetCheck->blockSignals(false);
        granulationCheck->blockSignals(true); granulationCheck->setChecked(s.granulation); granulationCheck->blockSignals(false);
        mixColorCheck->blockSignals(true); mixColorCheck->setChecked(s.mixSecondColor); mixColorCheck->blockSignals(false);
        wetSlider->blockSignals(true); wetSlider->setValue(s.wetAmount); wetSlider->blockSignals(false);
        wetSlider->setVisible(s.wetMix);
        wetLabel->setVisible(s.wetMix);
        wetLabel->setText(tr("Cantidad de mezcla: %1%").arg(s.wetAmount));

        if (s.shape == ShapeType::CustomStamp && !s.customStampImage.isNull()) {
            customStampImage = s.customStampImage;
            importButton->setCustomImage(customStampImage);
            importButton->setToolTip(tr("PNG: %1x%2\nClick: reemplazar\nClic derecho: quitar")
                .arg(customStampImage.width()).arg(customStampImage.height()));
        }

        for (int i = 0; i < shapeButtons.size(); ++i) {
            shapeButtons[i]->blockSignals(true);
            bool isThis = false;
            if (i < PaintEngine::allShapes().size()) {
                isThis = (PaintEngine::allShapes()[i] == s.shape);
            } else if (i == PaintEngine::allShapes().size()) {
                isThis = (s.shape == ShapeType::CustomStamp);
            }
            shapeButtons[i]->setChecked(isThis);
            shapeButtons[i]->blockSignals(false);
        }
        compositeEditor->blockSignals(true);
        compositeEditor->setElements(s.shapeElements);
        compositeEditor->blockSignals(false);
        updateCompositeInfo();
        loadElementControls();
        refreshPreviews();
    }

    void refreshPreviews() {
        const BrushSettings &s = presets[activeIndex];
        preview->updatePreview(s, previewColor, previewSecondColor);
        QColor secondForStamp = s.mixSecondColor ? previewSecondColor : QColor();
        QImage stamp = PaintEngine::generateBrushStamp(s, previewColor, 255, false, secondForStamp);
        shapePreview->updateStamp(stamp);
    }

    void schedulePreview() {
        if (!previewTimer) {
            previewTimer = new QTimer(this);
            previewTimer->setSingleShot(true);
            previewTimer->setInterval(35);
            connect(previewTimer, &QTimer::timeout, this, [this]() { refreshPreviews(); });
        }
        previewTimer->start();
    }

    void updateCompositeInfo() {
        int count = compositeEditor->getElementCount();
        compositeInfo->setText(count > 0 ? tr("Compuesto: %1 figuras").arg(count) : tr("Figura simple"));
    }

    void loadElementControls() {
        int idx = compositeEditor->getSelectedIndex();
        QVector<ShapeElement> elems = compositeEditor->getElements();
        bool valid = (idx >= 0 && idx < elems.size());
        elemScaleSlider->setEnabled(valid);
        elemRotationSlider->setEnabled(valid);
        elemOpacitySlider->setEnabled(valid);
        btnRemoveElement->setEnabled(valid);
        if (valid) {
            const ShapeElement &el = elems[idx];
            elemScaleSlider->blockSignals(true); elemScaleSlider->setValue((int)(el.scale * 100)); elemScaleSlider->blockSignals(false);
            elemScaleLabel->setText(tr("Escala: %1%").arg((int)(el.scale * 100)));
            elemRotationSlider->blockSignals(true); elemRotationSlider->setValue((int)el.rotation); elemRotationSlider->blockSignals(false);
            elemRotationLabel->setText(tr("Rotacion: %1%2").arg((int)el.rotation).arg(QString::fromUtf8("\xC2\xB0")));
            elemOpacitySlider->blockSignals(true); elemOpacitySlider->setValue(el.opacity); elemOpacitySlider->blockSignals(false);
            elemOpacityLabel->setText(tr("Opacidad: %1%").arg(el.opacity));
        } else {
            elemScaleLabel->setText(tr("Escala: -"));
            elemRotationLabel->setText(tr("Rotacion: -"));
            elemOpacityLabel->setText(tr("Opacidad: -"));
        }
    }

    void saveControlsToPreset() {
        BrushSettings &s = presets[activeIndex];
        s.size = sizeSpin->value();
        s.opacity = opacitySlider->value();
        s.scatter = scatterSlider->value();
        s.angle = angleSlider->value();
        s.density = densitySlider->value();
        s.flow = flowSlider->value();
        s.aspectRatio = aspectSlider->value() / 100.0;
        s.dragMode = (DragMode)dragCombo->currentIndex();
        s.rotationMode = (RotationMode)rotationCombo->currentIndex();
        s.isAirbrush = airbrushCheck->isChecked();
        s.wetMix = wetCheck->isChecked();
        s.wetAmount = wetSlider->value();
        s.granulation = granulationCheck->isChecked();
        s.mixSecondColor = mixColorCheck->isChecked();
        s.shapeElements = compositeEditor->getElements();
        int checkedId = shapeGroup->checkedId();
        QList<ShapeType> shapes = PaintEngine::allShapes();
        if (checkedId >= 0 && checkedId < shapes.size()) {
            s.shape = shapes[checkedId];
            s.customStampImage = QImage();
        } else if (checkedId == shapes.size()) {
            s.shape = ShapeType::CustomStamp;
            s.customStampImage = customStampImage;
        }
        opacityLabel->setText(tr("Opacidad: %1%").arg(s.opacity));
        scatterLabel->setText(tr("Dispersion: %1").arg(s.scatter));
        angleLabel->setText(tr("Angulo: %1%2").arg((int)s.angle).arg(QString::fromUtf8("\xC2\xB0")));
        densityLabel->setText(tr("Figuras por paso: %1").arg(s.density));
        flowLabel->setText(tr("Flujo: %1%").arg(s.flow));
        aspectLabel->setText(tr("Proporcion: %1%").arg((int)(s.aspectRatio * 100)));
        wetLabel->setText(tr("Cantidad de mezcla: %1%").arg(s.wetAmount));
        updateCompositeInfo();
        schedulePreview();
    }

public:
    CustomBrushesDialog(bool darkMode, BrushSettings p1, BrushSettings p2, int active, QWidget *parent = nullptr)
        : QDialog(parent), activeIndex(active), previewColor(Qt::black), previewSecondColor(Qt::white) {
        presets[0] = p1;
        presets[1] = p2;
        setupTheme(darkMode);
        setWindowTitle(tr("Configurar Pinceles Personalizados"));
        setMinimumSize(920, 720);
        applyStyleSheet();

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(12, 12, 12, 12);
        mainLayout->setSpacing(8);

        QHBoxLayout *topRow = new QHBoxLayout();
        radio1 = new QRadioButton(tr("Pincel 1"));
        radio2 = new QRadioButton(tr("Pincel 2"));
        radio1->setChecked(activeIndex == 0);
        radio2->setChecked(activeIndex == 1);
        topRow->addWidget(radio1);
        topRow->addWidget(radio2);
        topRow->addStretch();
        mainLayout->addLayout(topRow);

        QHBoxLayout *previewRow = new QHBoxLayout();
        previewRow->setSpacing(8);
        shapePreview = new ShapePreview();
        shapePreview->setDarkMode(m_dark);
        previewRow->addWidget(shapePreview);
        preview = new StrokePreview();
        preview->setDarkMode(m_dark);
        previewRow->addWidget(preview, 1);
        mainLayout->addLayout(previewRow);

        QHBoxLayout *contentRow = new QHBoxLayout();
        contentRow->setSpacing(14);

        QVBoxLayout *shapeCol = new QVBoxLayout();
        shapeCol->setSpacing(6);
        shapeCol->addWidget(makeSectionTitle(tr("CATALOGO · ARRASTRA AL COMPUESTO")));

        shapeGrid = new QGridLayout();
        shapeGrid->setContentsMargins(2, 2, 2, 2);
        shapeGrid->setSpacing(4);
        shapeGroup = new QButtonGroup(this);
        shapeGroup->setExclusive(true);
        buildShapeGrid();
        shapeCol->addLayout(shapeGrid);
        shapeCol->addSpacing(4);
        shapeCol->addWidget(makeSectionTitle(tr("MODO DE ARRASTRE")));
        dragCombo = new QComboBox();
        dragCombo->addItem(tr("Continuo"));
        dragCombo->addItem(tr("Estampado"));
        dragCombo->addItem(tr("Punteado"));
        dragCombo->addItem(tr("Disperso"));
        dragCombo->addItem(tr("Cinta"));
        dragCombo->setStyleSheet(comboStyle());
        shapeCol->addWidget(dragCombo);
        shapeCol->addSpacing(4);
        shapeCol->addWidget(makeSectionTitle(tr("ROTACION DEL STAMP")));
        rotationCombo = new QComboBox();
        rotationCombo->addItem(tr("Fijo"));
        rotationCombo->addItem(tr("Seguir direccion"));
        rotationCombo->addItem(tr("Aleatorio"));
        rotationCombo->setStyleSheet(comboStyle());
        shapeCol->addWidget(rotationCombo);
        shapeCol->addStretch();
        contentRow->addLayout(shapeCol);

        QVBoxLayout *compCol = new QVBoxLayout();
        compCol->setSpacing(5);
        compCol->addWidget(makeSectionTitle(tr("COMPOSICION DE FIGURAS")));
        compositeInfo = new QLabel(tr("Figura simple"));
        compositeInfo->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: 600; background: transparent;").arg(c_accent));
        compCol->addWidget(compositeInfo);
        compositeEditor = new CompositeEditor();
        compositeEditor->setDarkMode(m_dark);
        compCol->addWidget(compositeEditor, 0, Qt::AlignHCenter);
        QHBoxLayout *compBtnRow = new QHBoxLayout();
        compBtnRow->setSpacing(6);
        btnJitter = new QPushButton(tr("Variacion..."));
        btnJitter->setStyleSheet(smallBtnStyle());
        btnJitter->setCursor(Qt::PointingHandCursor);
        btnRemoveElement = new QPushButton(tr("Quitar"));
        btnRemoveElement->setStyleSheet(smallBtnStyle());
        btnRemoveElement->setCursor(Qt::PointingHandCursor);
        btnClearComposite = new QPushButton(tr("Limpiar"));
        btnClearComposite->setStyleSheet(smallBtnStyle());
        btnClearComposite->setCursor(Qt::PointingHandCursor);
        compBtnRow->addWidget(btnJitter);
        compBtnRow->addWidget(btnRemoveElement);
        compBtnRow->addWidget(btnClearComposite);
        compBtnRow->addStretch();
        compCol->addLayout(compBtnRow);
        compCol->addSpacing(2);
        compCol->addWidget(makeSectionTitle(tr("ELEMENTO SELECCIONADO")));
        elemScaleLabel = new QLabel(tr("Escala: -"));
        elemScaleLabel->setStyleSheet(labelStyle());
        compCol->addWidget(elemScaleLabel);
        elemScaleSlider = makeSlider(10, 250, 100);
        compCol->addWidget(elemScaleSlider);
        elemRotationLabel = new QLabel(tr("Rotacion: -"));
        elemRotationLabel->setStyleSheet(labelStyle());
        compCol->addWidget(elemRotationLabel);
        elemRotationSlider = makeSlider(0, 360, 0);
        compCol->addWidget(elemRotationSlider);
        elemOpacityLabel = new QLabel(tr("Opacidad: -"));
        elemOpacityLabel->setStyleSheet(labelStyle());
        compCol->addWidget(elemOpacityLabel);
        elemOpacitySlider = makeSlider(0, 100, 100);
        compCol->addWidget(elemOpacitySlider);
        compCol->addStretch();
        contentRow->addLayout(compCol);

        QVBoxLayout *ctrlCol = new QVBoxLayout();
        ctrlCol->setSpacing(4);
        ctrlCol->addWidget(makeSectionTitle(tr("TRAZO")));
        sizeRow = new QHBoxLayout();
        sizeRow->setSpacing(8);
        sizeLabel = new QLabel(tr("Tamano:"));
        sizeLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: 600; background: transparent;").arg(c_text));
        sizeSpin = new QSpinBox();
        sizeSpin->setRange(1, 200);
        sizeSpin->setFixedWidth(90);
        sizeRow->addWidget(sizeLabel);
        sizeRow->addWidget(sizeSpin);
        sizeRow->addStretch();
        ctrlCol->addLayout(sizeRow);
        opacityLabel = new QLabel(tr("Opacidad: 100%"));
        opacityLabel->setStyleSheet(labelStyle());
        ctrlCol->addWidget(opacityLabel);
        opacitySlider = makeSlider(1, 100, 100);
        ctrlCol->addWidget(opacitySlider);
        scatterLabel = new QLabel(tr("Dispersion: 0"));
        scatterLabel->setStyleSheet(labelStyle());
        ctrlCol->addWidget(scatterLabel);
        scatterSlider = makeSlider(0, 50, 0);
        ctrlCol->addWidget(scatterSlider);
        angleLabel = new QLabel(tr("Angulo: 0") + QString::fromUtf8("\xC2\xB0"));
        angleLabel->setStyleSheet(labelStyle());
        ctrlCol->addWidget(angleLabel);
        angleSlider = makeSlider(0, 360, 0);
        ctrlCol->addWidget(angleSlider);
        aspectLabel = new QLabel(tr("Proporcion: 100%"));
        aspectLabel->setStyleSheet(labelStyle());
        ctrlCol->addWidget(aspectLabel);
        aspectSlider = makeSlider(10, 300, 100);
        ctrlCol->addWidget(aspectSlider);
        densityLabel = new QLabel(tr("Figuras por paso: 1"));
        densityLabel->setStyleSheet(labelStyle());
        ctrlCol->addWidget(densityLabel);
        densitySlider = makeSlider(1, 20, 1);
        ctrlCol->addWidget(densitySlider);
        flowLabel = new QLabel(tr("Flujo: 100%"));
        flowLabel->setStyleSheet(labelStyle());
        ctrlCol->addWidget(flowLabel);
        flowSlider = makeSlider(1, 100, 100);
        ctrlCol->addWidget(flowSlider);
        ctrlCol->addSpacing(2);
        ctrlCol->addWidget(makeSectionTitle(tr("MODOS")));
        airbrushCheck = new QCheckBox(tr("Aerografo (acumula)"));
        ctrlCol->addWidget(airbrushCheck);
        granulationCheck = new QCheckBox(tr("Granulado (papel)"));
        ctrlCol->addWidget(granulationCheck);
        mixColorCheck = new QCheckBox(tr("Combinar con 2do color"));
        ctrlCol->addWidget(mixColorCheck);
        wetCheck = new QCheckBox(tr("Mezcla humeda (Wet)"));
        ctrlCol->addWidget(wetCheck);
        wetLabel = new QLabel(tr("Cantidad de mezcla: 50%"));
        wetLabel->setStyleSheet(labelStyle());
        ctrlCol->addWidget(wetLabel);
        wetSlider = makeSlider(0, 100, 50);
        ctrlCol->addWidget(wetSlider);
        ctrlCol->addStretch();
        contentRow->addLayout(ctrlCol);

        mainLayout->addLayout(contentRow, 1);

        QHBoxLayout *btnRow = new QHBoxLayout();
        btnRow->setSpacing(8);
        btnRow->addStretch();
        QPushButton *btnCancel = new QPushButton(tr("Cancelar"));
        btnCancel->setStyleSheet(smallBtnStyle());
        btnCancel->setCursor(Qt::PointingHandCursor);
        btnCancel->setFixedHeight(30);
        QPushButton *btnOk = new QPushButton(tr("Aceptar"));
        btnOk->setStyleSheet(accentBtnStyle());
        btnOk->setCursor(Qt::PointingHandCursor);
        btnOk->setFixedHeight(30);
        btnRow->addWidget(btnCancel);
        btnRow->addWidget(btnOk);
        mainLayout->addLayout(btnRow);

        connect(btnOk, &QPushButton::clicked, this, [this]() { saveControlsToPreset(); accept(); });
        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);

        connect(radio1, &QRadioButton::toggled, this, [this](bool checked) {
            if (checked) { saveControlsToPreset(); activeIndex = 0; loadControlsFromPreset(); }
        });
        connect(radio2, &QRadioButton::toggled, this, [this](bool checked) {
            if (checked) { saveControlsToPreset(); activeIndex = 1; loadControlsFromPreset(); }
        });
        connect(sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() { saveControlsToPreset(); });
        connect(opacitySlider, &QSlider::valueChanged, this, [this]() { saveControlsToPreset(); });
        connect(scatterSlider, &QSlider::valueChanged, this, [this]() { saveControlsToPreset(); });
        connect(angleSlider, &QSlider::valueChanged, this, [this]() { saveControlsToPreset(); });
        connect(densitySlider, &QSlider::valueChanged, this, [this]() { saveControlsToPreset(); });
        connect(flowSlider, &QSlider::valueChanged, this, [this]() { saveControlsToPreset(); });
        connect(aspectSlider, &QSlider::valueChanged, this, [this]() { saveControlsToPreset(); });
        connect(dragCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { saveControlsToPreset(); });
        connect(rotationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { saveControlsToPreset(); });
        connect(airbrushCheck, &QCheckBox::toggled, this, [this]() { saveControlsToPreset(); });
        connect(granulationCheck, &QCheckBox::toggled, this, [this]() { saveControlsToPreset(); });
        connect(mixColorCheck, &QCheckBox::toggled, this, [this]() { saveControlsToPreset(); });
        connect(wetCheck, &QCheckBox::toggled, this, [this](bool checked) {
            wetSlider->setVisible(checked);
            wetLabel->setVisible(checked);
            saveControlsToPreset();
        });
        connect(wetSlider, &QSlider::valueChanged, this, [this]() { saveControlsToPreset(); });
        connect(shapeGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, [this](int) {
            saveControlsToPreset();
        });

        connect(btnJitter, &QPushButton::clicked, this, [this]() {
            const BrushSettings &s = presets[activeIndex];
            JitterDialog dlg(m_dark, s.sizeJitter, s.angleJitter, s.opacityJitter, this);
            if (dlg.exec() == QDialog::Accepted) {
                presets[activeIndex].sizeJitter = dlg.getSizeJitter();
                presets[activeIndex].angleJitter = dlg.getAngleJitter();
                presets[activeIndex].opacityJitter = dlg.getOpacityJitter();
                schedulePreview();
            }
        });

        connect(compositeEditor, &CompositeEditor::elementsChanged, this, [this]() { saveControlsToPreset(); });
        connect(compositeEditor, &CompositeEditor::selectionChanged, this, [this](int) { loadElementControls(); });
        connect(btnRemoveElement, &QPushButton::clicked, this, [this]() { compositeEditor->removeSelected(); });
        connect(btnClearComposite, &QPushButton::clicked, this, [this]() { compositeEditor->clearAll(); });
        connect(elemScaleSlider, &QSlider::valueChanged, this, [this](int v) {
            int idx = compositeEditor->getSelectedIndex();
            QVector<ShapeElement> elems = compositeEditor->getElements();
            if (idx >= 0 && idx < elems.size()) {
                ShapeElement el = elems[idx];
                el.scale = v / 100.0;
                elemScaleLabel->setText(tr("Escala: %1%").arg(v));
                compositeEditor->updateSelectedElement(el);
            }
        });
        connect(elemRotationSlider, &QSlider::valueChanged, this, [this](int v) {
            int idx = compositeEditor->getSelectedIndex();
            QVector<ShapeElement> elems = compositeEditor->getElements();
            if (idx >= 0 && idx < elems.size()) {
                ShapeElement el = elems[idx];
                el.rotation = v;
                elemRotationLabel->setText(tr("Rotacion: %1%2").arg(v).arg(QString::fromUtf8("\xC2\xB0")));
                compositeEditor->updateSelectedElement(el);
            }
        });
        connect(elemOpacitySlider, &QSlider::valueChanged, this, [this](int v) {
            int idx = compositeEditor->getSelectedIndex();
            QVector<ShapeElement> elems = compositeEditor->getElements();
            if (idx >= 0 && idx < elems.size()) {
                ShapeElement el = elems[idx];
                el.opacity = v;
                elemOpacityLabel->setText(tr("Opacidad: %1%").arg(v));
                compositeEditor->updateSelectedElement(el);
            }
        });

        loadControlsFromPreset();
    }

    BrushSettings getPreset1() const { return presets[0]; }
    BrushSettings getPreset2() const { return presets[1]; }
    int getActivePresetIndex() const { return activeIndex; }
    void setPreviewColor(const QColor &c) { previewColor = c; refreshPreviews(); }
    void setPreviewSecondColor(const QColor &c) { previewSecondColor = c; refreshPreviews(); }
};

#endif // CUSTOMBRUSHES_H