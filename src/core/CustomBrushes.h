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
// SHAPE NAME TABLE — Replace Conditional with Polymorphism
// ============================================================
class ShapeNames {
public:
    static QString name(ShapeType s) {
        const QString n = lookup(s);
        return n.isEmpty() ? QString() : QObject::tr(n.toUtf8().constData());
    }

private:
    static QString lookup(ShapeType s) {
        static const QHash<int, QString> table = {
            {(int)ShapeType::Circle,        "Circulo"},
            {(int)ShapeType::Square,        "Cuadrado"},
            {(int)ShapeType::RoundedSquare, "Cuadrado redondeado"},
            {(int)ShapeType::Diamond,       "Rombo"},
            {(int)ShapeType::Triangle,      "Triangulo"},
            {(int)ShapeType::RightTriangle, "Triangulo rectangulo"},
            {(int)ShapeType::Pentagon,      "Pentagono"},
            {(int)ShapeType::Hexagon,       "Hexagono"},
            {(int)ShapeType::Star4,         "Estrella 4 puntas"},
            {(int)ShapeType::Star5,         "Estrella 5 puntas"},
            {(int)ShapeType::Star6,         "Estrella 6 puntas"},
            {(int)ShapeType::Cross,         "Cruz"},
            {(int)ShapeType::Plus,          "Signo mas"},
            {(int)ShapeType::X,             "Equis"},
            {(int)ShapeType::Arrow,         "Flecha"},
            {(int)ShapeType::Heart,         "Corazon"},
            {(int)ShapeType::Line,          "Linea"},
            {(int)ShapeType::PencilTip,     "Punta de lapiz"},
            {(int)ShapeType::FlatTip,       "Punta plana"},
            {(int)ShapeType::ChiselTip,     "Punta cincel"},
            {(int)ShapeType::Leaf,          "Hoja"},
            {(int)ShapeType::Drop,          "Gota"},
            {(int)ShapeType::Crescent,      "Media luna"},
            {(int)ShapeType::Ring,          "Anillo"},
            {(int)ShapeType::HalfCircle,    "Semicirculo"},
            {(int)ShapeType::Sparkle,       "Destello"},
            {(int)ShapeType::Clover,        "Trebol"},
            {(int)ShapeType::Gear,          "Engranaje"},
            {(int)ShapeType::Lightning,     "Rayo"},
            {(int)ShapeType::MusicNote,     "Nota musical"},
            {(int)ShapeType::Flower,        "Flor"},
            {(int)ShapeType::Butterfly,     "Mariposa"},
            {(int)ShapeType::Cloud,         "Nube"},
            {(int)ShapeType::Speech,        "Bocadillo"},
            {(int)ShapeType::LocationPin,   "Pin de ubicacion"},
            {(int)ShapeType::Wave,          "Onda"},
            {(int)ShapeType::Spiral,        "Espiral"},
            {(int)ShapeType::StarMany,      "Estrella muchos picos"},
            {(int)ShapeType::Infinity,      "Infinito"},
            {(int)ShapeType::DiamondStar,   "Rombo estrella"},
            {(int)ShapeType::CustomStamp,   "PNG importado"},
        };
        auto it = table.constFind((int)s);
        return (it != table.constEnd()) ? it.value() : QString();
    }
};

// ============================================================
// SHAPE PATH BUILDER — tabla de builders
// ============================================================
class ShapePathBuilder {
public:
    struct Context {
        QRectF r{-50.0, -50.0, 100.0, 100.0};
        double cx = 0.0;
        double cy = 0.0;
        double w  = 100.0;
        double h  = 100.0;

        Context() {
            cx = r.center().x();
            cy = r.center().y();
            w  = r.width();
            h  = r.height();
        }
    };

    static void appendRegularPolygon(QPainterPath &path, const Context &ctx,
                                     int sides, double startAngle, double radiusFactor) {
        for (int i = 0; i < sides; ++i) {
            const double a = startAngle + i * 2.0 * M_PI / sides;
            const double px = ctx.cx + (ctx.w / 2.0) * radiusFactor * cos(a);
            const double py = ctx.cy + (ctx.h / 2.0) * radiusFactor * sin(a);
            if (i == 0) path.moveTo(px, py);
            else        path.lineTo(px, py);
        }
        path.closeSubpath();
    }

    static void appendStarPolygon(QPainterPath &path, const Context &ctx,
                                  int points, double innerFactor, double startAngle) {
        const int total = points * 2;
        for (int i = 0; i < total; ++i) {
            const double a = startAngle + i * M_PI / points;
            const double f = (i % 2 == 0) ? 1.0 : innerFactor;
            const double px = ctx.cx + (ctx.w / 2.0) * f * cos(a);
            const double py = ctx.cy + (ctx.h / 2.0) * f * sin(a);
            if (i == 0) path.moveTo(px, py);
            else        path.lineTo(px, py);
        }
        path.closeSubpath();
    }

    static void buildCircle(QPainterPath &p, const Context &c)          { p.addEllipse(c.r); }
    static void buildSquare(QPainterPath &p, const Context &c)          { p.addRect(c.r); }
    static void buildRoundedSquare(QPainterPath &p, const Context &c)   { p.addRoundedRect(c.r, c.w * 0.2, c.h * 0.2); }

    static void buildDiamond(QPainterPath &p, const Context &c) {
        p.moveTo(c.cx, c.r.top());
        p.lineTo(c.r.right(), c.cy);
        p.lineTo(c.cx, c.r.bottom());
        p.lineTo(c.r.left(), c.cy);
        p.closeSubpath();
    }

    static void buildTriangle(QPainterPath &p, const Context &c) {
        p.moveTo(c.cx, c.r.top());
        p.lineTo(c.r.left(), c.r.bottom());
        p.lineTo(c.r.right(), c.r.bottom());
        p.closeSubpath();
    }

    static void buildRightTriangle(QPainterPath &p, const Context &c) {
        p.moveTo(c.r.topLeft());
        p.lineTo(c.r.bottomLeft());
        p.lineTo(c.r.bottomRight());
        p.closeSubpath();
    }

    static void buildPentagon(QPainterPath &p, const Context &c) { appendRegularPolygon(p, c, 5, -M_PI / 2.0, 1.0); }
    static void buildHexagon(QPainterPath &p, const Context &c)  { appendRegularPolygon(p, c, 6, -M_PI / 2.0, 1.0); }
    static void buildStar4(QPainterPath &p, const Context &c)    { appendStarPolygon(p, c, 4, 0.35, -M_PI / 2.0); }
    static void buildStar5(QPainterPath &p, const Context &c)    { appendStarPolygon(p, c, 5, 0.45, -M_PI / 2.0); }
    static void buildStar6(QPainterPath &p, const Context &c)    { appendStarPolygon(p, c, 6, 0.5,  -M_PI / 2.0); }

    static void buildCross(QPainterPath &p, const Context &c) {
        const double t = c.w * 0.3;
        p.moveTo(c.cx - t / 2, c.r.top());
        p.lineTo(c.cx + t / 2, c.r.top());
        p.lineTo(c.cx + t / 2, c.cy - t / 2);
        p.lineTo(c.r.right(), c.cy - t / 2);
        p.lineTo(c.r.right(), c.cy + t / 2);
        p.lineTo(c.cx + t / 2, c.cy + t / 2);
        p.lineTo(c.cx + t / 2, c.r.bottom());
        p.lineTo(c.cx - t / 2, c.r.bottom());
        p.lineTo(c.cx - t / 2, c.cy + t / 2);
        p.lineTo(c.r.left(), c.cy + t / 2);
        p.lineTo(c.r.left(), c.cy - t / 2);
        p.lineTo(c.cx - t / 2, c.cy - t / 2);
        p.closeSubpath();
    }

    static void buildPlus(QPainterPath &p, const Context &c) {
        const double t = c.w * 0.35;
        p.addRect(c.cx - t / 2, c.r.top(), t, c.h);
        p.addRect(c.r.left(), c.cy - t / 2, c.w, t);
    }

    static void buildX(QPainterPath &p, const Context &c) {
        const double t = c.w * 0.25;
        QPolygonF poly;
        poly << QPointF(c.r.left() + t, c.r.top())
             << QPointF(c.cx, c.cy - t)
             << QPointF(c.r.right() - t, c.r.top())
             << QPointF(c.r.right(), c.r.top() + t)
             << QPointF(c.cx + t, c.cy)
             << QPointF(c.r.right(), c.r.bottom() - t)
             << QPointF(c.r.right() - t, c.r.bottom())
             << QPointF(c.cx, c.cy + t)
             << QPointF(c.r.left() + t, c.r.bottom())
             << QPointF(c.r.left(), c.r.bottom() - t)
             << QPointF(c.cx - t, c.cy)
             << QPointF(c.r.left(), c.r.top() + t);
        p.addPolygon(poly);
        p.closeSubpath();
    }

    static void buildArrow(QPainterPath &p, const Context &c) {
        const double bodyH = c.h * 0.4;
        const double headW = c.w * 0.45;
        p.moveTo(c.r.left(), c.cy - bodyH / 2);
        p.lineTo(c.r.right() - headW, c.cy - bodyH / 2);
        p.lineTo(c.r.right() - headW, c.r.top());
        p.lineTo(c.r.right(), c.cy);
        p.lineTo(c.r.right() - headW, c.r.bottom());
        p.lineTo(c.r.right() - headW, c.cy + bodyH / 2);
        p.lineTo(c.r.left(), c.cy + bodyH / 2);
        p.closeSubpath();
    }

    static void buildHeart(QPainterPath &p, const Context &c) {
        p.moveTo(c.cx, c.r.top() + c.h * 0.3);
        p.cubicTo(c.r.left() + c.w * 0.1, c.r.top() - c.h * 0.05,
                  c.r.left(), c.r.top() + c.h * 0.55,
                  c.cx, c.r.bottom());
        p.cubicTo(c.r.right(), c.r.top() + c.h * 0.55,
                  c.r.right() - c.w * 0.1, c.r.top() - c.h * 0.05,
                  c.cx, c.r.top() + c.h * 0.3);
    }

    static void buildLine(QPainterPath &p, const Context &c) {
        const double t = qMax(1.0, c.h * 0.15);
        p.addRect(c.r.left(), c.cy - t / 2, c.w, t);
    }

    static void buildPencilTip(QPainterPath &p, const Context &c) {
        p.moveTo(c.cx, c.r.top());
        p.lineTo(c.r.left() + c.w * 0.25, c.r.bottom());
        p.lineTo(c.r.right() - c.w * 0.25, c.r.bottom());
        p.closeSubpath();
    }

    static void buildFlatTip(QPainterPath &p, const Context &c) {
        const double fh = c.h * 0.45;
        p.addRoundedRect(c.r.left(), c.cy - fh / 2, c.w, fh, fh * 0.3, fh * 0.3);
    }

    static void buildChiselTip(QPainterPath &p, const Context &c) {
        p.moveTo(c.r.left() + c.w * 0.15, c.r.top());
        p.lineTo(c.r.right() - c.w * 0.15, c.r.top());
        p.lineTo(c.r.right(), c.r.bottom());
        p.lineTo(c.r.left(), c.r.bottom());
        p.closeSubpath();
    }

    static void buildLeaf(QPainterPath &p, const Context &c) {
        p.moveTo(c.r.left(), c.r.bottom());
        p.cubicTo(c.r.left(), c.r.top() + c.h * 0.2, c.cx, c.r.top(), c.r.right(), c.r.top());
        p.cubicTo(c.r.right(), c.r.top() + c.h * 0.6, c.cx + c.w * 0.2, c.r.bottom(), c.r.left(), c.r.bottom());
        p.closeSubpath();
    }

    static void buildDrop(QPainterPath &p, const Context &c) {
        p.moveTo(c.cx, c.r.top());
        p.cubicTo(c.cx + c.w * 0.4, c.r.top() + c.h * 0.4, c.r.right(), c.cy + c.h * 0.15, c.cx, c.r.bottom());
        p.cubicTo(c.r.left(), c.cy + c.h * 0.15, c.cx - c.w * 0.4, c.r.top() + c.h * 0.4, c.cx, c.r.top());
        p.closeSubpath();
    }

    static void buildCrescent(QPainterPath &p, const Context &c) {
        p.moveTo(c.cx + c.w * 0.15, c.r.top());
        p.arcTo(c.r, 270, 180);
        p.cubicTo(c.cx - c.w * 0.05, c.r.top() + c.h * 0.75,
                  c.cx - c.w * 0.05, c.r.top() + c.h * 0.25,
                  c.cx + c.w * 0.15, c.r.top());
        p.closeSubpath();
    }

    static void buildRing(QPainterPath &p, const Context &c) {
        p.addEllipse(c.r);
        const double inset = c.w * 0.2;
        p.addEllipse(c.r.adjusted(inset, inset, -inset, -inset));
    }

    static void buildHalfCircle(QPainterPath &p, const Context &c) {
        p.moveTo(c.r.left(), c.cy);
        p.arcTo(c.r, 180, 180);
        p.closeSubpath();
    }

    static void buildSparkle(QPainterPath &p, const Context &c) {
        const double inner = 0.2;
        p.moveTo(c.cx, c.r.top());
        p.quadTo(c.cx + c.w * inner, c.cy - c.h * inner, c.r.right(), c.cy);
        p.quadTo(c.cx + c.w * inner, c.cy + c.h * inner, c.cx, c.r.bottom());
        p.quadTo(c.cx - c.w * inner, c.cy + c.h * inner, c.r.left(), c.cy);
        p.quadTo(c.cx - c.w * inner, c.cy - c.h * inner, c.cx, c.r.top());
        p.closeSubpath();
    }

    static void buildClover(QPainterPath &p, const Context &c) {
        const double lr = c.w * 0.22;
        p.addEllipse(c.cx - lr, c.cy - lr * 2, lr * 2, lr * 2);
        p.addEllipse(c.cx - lr * 2, c.cy - lr, lr * 2, lr * 2);
        p.addEllipse(c.cx, c.cy - lr, lr * 2, lr * 2);
        p.addEllipse(c.cx - lr, c.cy, lr * 2, lr * 2);
    }

    static void buildGear(QPainterPath &p, const Context &c) {
        const int teeth = 8;
        const double outerR = c.w / 2.0;
        const double innerR = c.w / 3.0;
        for (int i = 0; i < teeth * 2; ++i) {
            const double a = i * M_PI / teeth;
            const double rad = (i % 2 == 0) ? outerR : innerR;
            const double px = c.cx + rad * cos(a);
            const double py = c.cy + rad * sin(a);
            if (i == 0) p.moveTo(px, py);
            else        p.lineTo(px, py);
        }
        p.closeSubpath();
        const double holeR = c.w * 0.12;
        p.addEllipse(c.cx - holeR, c.cy - holeR, holeR * 2, holeR * 2);
    }

    static void buildLightning(QPainterPath &p, const Context &c) {
        p.moveTo(c.cx + c.w * 0.08, c.r.top());
        p.lineTo(c.cx - c.w * 0.28, c.cy + c.h * 0.05);
        p.lineTo(c.cx - c.w * 0.02, c.cy + c.h * 0.05);
        p.lineTo(c.cx - c.w * 0.12, c.r.bottom());
        p.lineTo(c.cx + c.w * 0.28, c.cy - c.h * 0.05);
        p.lineTo(c.cx + c.w * 0.02, c.cy - c.h * 0.05);
        p.lineTo(c.cx + c.w * 0.18, c.r.top());
        p.closeSubpath();
    }

    static void buildMusicNote(QPainterPath &p, const Context &c) {
        const double headR = c.w * 0.13;
        p.addEllipse(QPointF(c.cx - c.w * 0.10, c.cy + c.h * 0.28), headR, headR * 0.8);
        p.addRect(QRectF(c.cx + c.w * 0.02, c.r.top() + c.h * 0.05, c.w * 0.05, c.h * 0.45));
        p.moveTo(c.cx + c.w * 0.07, c.r.top() + c.h * 0.05);
        p.cubicTo(c.cx + c.w * 0.32, c.r.top() + c.h * 0.15,
                  c.cx + c.w * 0.32, c.r.top() + c.h * 0.35,
                  c.cx + c.w * 0.10, c.r.top() + c.h * 0.35);
        p.lineTo(c.cx + c.w * 0.07, c.r.top() + c.h * 0.28);
        p.cubicTo(c.cx + c.w * 0.20, c.r.top() + c.h * 0.28,
                  c.cx + c.w * 0.20, c.r.top() + c.h * 0.18,
                  c.cx + c.w * 0.07, c.r.top() + c.h * 0.18);
        p.closeSubpath();
    }

    static void buildFlower(QPainterPath &p, const Context &c) {
        const int petals = 5;
        const double petalR = c.w * 0.20;
        for (int i = 0; i < petals; ++i) {
            const double a = -M_PI / 2.0 + i * 2.0 * M_PI / petals;
            const double px = c.cx + c.w * 0.20 * cos(a);
            const double py = c.cy + c.h * 0.20 * sin(a);
            p.addEllipse(QPointF(px, py), petalR, petalR);
        }
        p.addEllipse(QPointF(c.cx, c.cy), c.w * 0.10, c.h * 0.10);
    }

    static void buildButterfly(QPainterPath &p, const Context &c) {
        p.addEllipse(QPointF(c.cx, c.cy), c.w * 0.04, c.h * 0.30);
        p.moveTo(c.cx - c.w * 0.04, c.cy - c.h * 0.05);
        p.cubicTo(c.cx - c.w * 0.45, c.cy - c.h * 0.45,
                  c.cx - c.w * 0.50, c.cy + c.h * 0.10,
                  c.cx - c.w * 0.04, c.cy + c.h * 0.10);
        p.closeSubpath();
        p.moveTo(c.cx - c.w * 0.04, c.cy + c.h * 0.05);
        p.cubicTo(c.cx - c.w * 0.40, c.cy + c.h * 0.25,
                  c.cx - c.w * 0.35, c.cy + c.h * 0.50,
                  c.cx - c.w * 0.04, c.cy + c.h * 0.30);
        p.closeSubpath();
        p.moveTo(c.cx + c.w * 0.04, c.cy - c.h * 0.05);
        p.cubicTo(c.cx + c.w * 0.45, c.cy - c.h * 0.45,
                  c.cx + c.w * 0.50, c.cy + c.h * 0.10,
                  c.cx + c.w * 0.04, c.cy + c.h * 0.10);
        p.closeSubpath();
        p.moveTo(c.cx + c.w * 0.04, c.cy + c.h * 0.05);
        p.cubicTo(c.cx + c.w * 0.40, c.cy + c.h * 0.25,
                  c.cx + c.w * 0.35, c.cy + c.h * 0.50,
                  c.cx + c.w * 0.04, c.cy + c.h * 0.30);
        p.closeSubpath();
    }

    static void buildCloud(QPainterPath &p, const Context &c) {
        p.addEllipse(QPointF(c.cx - c.w * 0.20, c.cy + c.h * 0.05), c.w * 0.20, c.h * 0.20);
        p.addEllipse(QPointF(c.cx + c.w * 0.20, c.cy + c.h * 0.05), c.w * 0.22, c.h * 0.22);
        p.addEllipse(QPointF(c.cx - c.w * 0.05, c.cy - c.h * 0.10), c.w * 0.25, c.h * 0.25);
        p.addEllipse(QPointF(c.cx + c.w * 0.15, c.cy - c.h * 0.05), c.w * 0.20, c.h * 0.20);
        p.addEllipse(QPointF(c.cx, c.cy + c.h * 0.15), c.w * 0.30, c.h * 0.15);
    }

    static void buildSpeech(QPainterPath &p, const Context &c) {
        const double rad = c.w * 0.10;
        p.addRoundedRect(QRectF(c.r.left(), c.r.top(), c.w, c.h * 0.75), rad, rad);
        p.moveTo(c.cx - c.w * 0.15, c.r.top() + c.h * 0.75);
        p.lineTo(c.cx - c.w * 0.20, c.r.bottom());
        p.lineTo(c.cx + c.w * 0.05, c.r.top() + c.h * 0.75);
        p.closeSubpath();
    }

    static void buildLocationPin(QPainterPath &p, const Context &c) {
        p.moveTo(c.cx, c.r.bottom());
        p.cubicTo(c.cx - c.w * 0.4, c.cy + c.h * 0.1, c.r.left(), c.r.top(), c.cx, c.r.top());
        p.cubicTo(c.r.right(), c.r.top(), c.cx + c.w * 0.4, c.cy + c.h * 0.1, c.cx, c.r.bottom());
        p.closeSubpath();
        const double holeR = c.w * 0.12;
        p.addEllipse(QPointF(c.cx, c.cy - c.h * 0.05), holeR, holeR);
    }

    static void buildWave(QPainterPath &p, const Context &c) {
        p.moveTo(c.r.left(), c.cy);
        p.cubicTo(c.r.left() + c.w * 0.15, c.r.top(), c.r.left() + c.w * 0.35, c.r.top(), c.cx, c.cy);
        p.cubicTo(c.cx + c.w * 0.15, c.r.bottom(), c.cx + c.w * 0.35, c.r.bottom(), c.r.right(), c.cy);
        p.cubicTo(c.cx + c.w * 0.35, c.cy + c.h * 0.35, c.cx + c.w * 0.15, c.cy + c.h * 0.35, c.cx, c.cy + c.h * 0.02);
        p.cubicTo(c.r.left() + c.w * 0.35, c.cy - c.h * 0.02, c.r.left() + c.w * 0.15, c.cy - c.h * 0.02, c.r.left(), c.cy);
        p.closeSubpath();
    }

    static void buildSpiral(QPainterPath &p, const Context &c) {
        const int steps = 80;
        const double turns = 3.0;
        const double maxR = c.w * 0.45;
        for (int i = 0; i <= steps; ++i) {
            const double t = (double)i / steps;
            const double a = t * turns * 2.0 * M_PI;
            const double rad = t * maxR;
            const double px = c.cx + rad * cos(a);
            const double py = c.cy + rad * sin(a);
            if (i == 0) p.moveTo(px, py);
            else        p.lineTo(px, py);
        }
    }

    static void buildStarMany(QPainterPath &p, const Context &c) {
        const int points = 16;
        const double outerR = c.w * 0.5;
        const double innerR = c.w * 0.20;
        for (int i = 0; i < points * 2; ++i) {
            const double a = -M_PI / 2.0 + i * M_PI / points;
            const double rad = (i % 2 == 0) ? outerR : innerR;
            const double px = c.cx + rad * cos(a);
            const double py = c.cy + rad * sin(a);
            if (i == 0) p.moveTo(px, py);
            else        p.lineTo(px, py);
        }
        p.closeSubpath();
    }

    static void buildInfinity(QPainterPath &p, const Context &c) {
        const double rW = c.w * 0.28;
        const double rH = c.h * 0.28;
        const double off = c.w * 0.22;
        QPainterPath left, right;
        left.addEllipse(QPointF(c.cx - off, c.cy), rW, rH);
        right.addEllipse(QPointF(c.cx + off, c.cy), rW, rH);
        p = left.united(right);
        QPainterPath innerL, innerR;
        innerL.addEllipse(QPointF(c.cx - off, c.cy), rW * 0.55, rH * 0.55);
        innerR.addEllipse(QPointF(c.cx + off, c.cy), rW * 0.55, rH * 0.55);
        p = p.subtracted(innerL.united(innerR));
        QPainterPath center;
        center.addEllipse(QPointF(c.cx, c.cy), rW * 0.18, rH * 0.18);
        p = p.united(center);
    }

    static void buildDiamondStar(QPainterPath &p, const Context &c) {
        const double outerX = c.w * 0.5;
        const double outerY = c.h * 0.5;
        const double innerX = c.w * 0.15;
        const double innerY = c.h * 0.15;
        p.moveTo(c.cx, c.cy - outerY);
        p.lineTo(c.cx + innerX, c.cy - innerY);
        p.lineTo(c.cx + outerX, c.cy);
        p.lineTo(c.cx + innerX, c.cy + innerY);
        p.lineTo(c.cx, c.cy + outerY);
        p.lineTo(c.cx - innerX, c.cy + innerY);
        p.lineTo(c.cx - outerX, c.cy);
        p.lineTo(c.cx - innerX, c.cy - innerY);
        p.closeSubpath();
    }

    using BuilderFn = void(*)(QPainterPath&, const Context&);

    static BuilderFn builderFor(ShapeType s) {
        static const QHash<int, BuilderFn> table = {
            {(int)ShapeType::Circle,        &buildCircle},
            {(int)ShapeType::Square,        &buildSquare},
            {(int)ShapeType::RoundedSquare, &buildRoundedSquare},
            {(int)ShapeType::Diamond,       &buildDiamond},
            {(int)ShapeType::Triangle,      &buildTriangle},
            {(int)ShapeType::RightTriangle, &buildRightTriangle},
            {(int)ShapeType::Pentagon,      &buildPentagon},
            {(int)ShapeType::Hexagon,       &buildHexagon},
            {(int)ShapeType::Star4,         &buildStar4},
            {(int)ShapeType::Star5,         &buildStar5},
            {(int)ShapeType::Star6,         &buildStar6},
            {(int)ShapeType::Cross,         &buildCross},
            {(int)ShapeType::Plus,          &buildPlus},
            {(int)ShapeType::X,             &buildX},
            {(int)ShapeType::Arrow,         &buildArrow},
            {(int)ShapeType::Heart,         &buildHeart},
            {(int)ShapeType::Line,          &buildLine},
            {(int)ShapeType::PencilTip,     &buildPencilTip},
            {(int)ShapeType::FlatTip,       &buildFlatTip},
            {(int)ShapeType::ChiselTip,     &buildChiselTip},
            {(int)ShapeType::Leaf,          &buildLeaf},
            {(int)ShapeType::Drop,          &buildDrop},
            {(int)ShapeType::Crescent,      &buildCrescent},
            {(int)ShapeType::Ring,          &buildRing},
            {(int)ShapeType::HalfCircle,    &buildHalfCircle},
            {(int)ShapeType::Sparkle,       &buildSparkle},
            {(int)ShapeType::Clover,        &buildClover},
            {(int)ShapeType::Gear,          &buildGear},
            {(int)ShapeType::Lightning,     &buildLightning},
            {(int)ShapeType::MusicNote,     &buildMusicNote},
            {(int)ShapeType::Flower,        &buildFlower},
            {(int)ShapeType::Butterfly,     &buildButterfly},
            {(int)ShapeType::Cloud,         &buildCloud},
            {(int)ShapeType::Speech,        &buildSpeech},
            {(int)ShapeType::LocationPin,   &buildLocationPin},
            {(int)ShapeType::Wave,          &buildWave},
            {(int)ShapeType::Spiral,        &buildSpiral},
            {(int)ShapeType::StarMany,      &buildStarMany},
            {(int)ShapeType::Infinity,      &buildInfinity},
            {(int)ShapeType::DiamondStar,   &buildDiamondStar},
        };
        auto it = table.constFind((int)s);
        return (it != table.constEnd()) ? it.value() : nullptr;
    }

    static QPainterPath build(ShapeType shape) {
        QPainterPath path;
        BuilderFn fn = builderFor(shape);
        if (!fn) return path;
        Context ctx;
        fn(path, ctx);
        return path;
    }
};

// ============================================================
// GEOMETRY DRAWER — draw dispatch con tabla de funciones
// ============================================================
class GeometryDrawer {
public:
    // ------------------------------------------------------------
    // Tipos de firma de dibujo por herramienta
    // ------------------------------------------------------------
    using PointDrawFn = void(*)(QPainter&, const QPoint&, const QPoint&);
    using RectDrawFn  = void(*)(QPainter&, const QRect&);

    // ------------------------------------------------------------
    // Entrada pública — dispatcher delgado
    // ------------------------------------------------------------
    static void draw(QPainter &painter, const QPoint &p1, const QPoint &p2, ToolType tool) {
        const QRect r = QRect(p1, p2).normalized();

        if (PointDrawFn pfn = pointFnFor(tool)) {
            pfn(painter, p1, p2);
            return;
        }
        if (RectDrawFn rfn = rectFnFor(tool)) {
            rfn(painter, r);
            return;
        }
    }

private:
    // ------------------------------------------------------------
    // Tablas de dispatch
    // ------------------------------------------------------------
    static PointDrawFn pointFnFor(ToolType tool) {
        static const QHash<int, PointDrawFn> table = {
            {(int)ToolLine,          &drawLinePts},
            {(int)ToolTriangle,      &drawTrianglePts},
            {(int)ToolRightTriangle, &drawRightTrianglePts},
            {(int)ToolDiamond,       &drawDiamondPts},
        };
        auto it = table.constFind((int)tool);
        return (it != table.constEnd()) ? it.value() : nullptr;
    }

    static RectDrawFn rectFnFor(ToolType tool) {
        static const QHash<int, RectDrawFn> table = {
            {(int)ToolRectangle,    &drawRectangleR},
            {(int)ToolEllipse,      &drawEllipseR},
            {(int)ToolRoundRect,    &drawRoundRectR},
            {(int)ToolPentagon,     &drawPentagonR},
            {(int)ToolHexagon,      &drawHexagonR},
            {(int)ToolStar,         &drawStarR},
            {(int)ToolArrowRight,   &drawArrowRightR},
            {(int)ToolArrowLeft,    &drawArrowLeftR},
            {(int)ToolHeart,        &drawHeartR},
            {(int)ToolCube,         &drawCubeR},
        };
        auto it = table.constFind((int)tool);
        return (it != table.constEnd()) ? it.value() : nullptr;
    }

    // ------------------------------------------------------------
    // Implementaciones — firma (QPainter, QPoint, QPoint)
    // ------------------------------------------------------------
    static void drawLinePts(QPainter &p, const QPoint &a, const QPoint &b) {
        p.drawLine(a, b);
    }

    static void drawTrianglePts(QPainter &p, const QPoint &a, const QPoint &b) {
        QPolygon t;
        t << QPoint((a.x() + b.x()) / 2, a.y())
          << QPoint(a.x(), b.y())
          << QPoint(b.x(), b.y());
        p.drawPolygon(t);
    }

    static void drawRightTrianglePts(QPainter &p, const QPoint &a, const QPoint &b) {
        QPolygon t;
        t << a << QPoint(a.x(), b.y()) << b;
        p.drawPolygon(t);
    }

    static void drawDiamondPts(QPainter &p, const QPoint &a, const QPoint &b) {
        QPolygon t;
        t << QPoint((a.x() + b.x()) / 2, a.y())
          << QPoint(b.x(), (a.y() + b.y()) / 2)
          << QPoint((a.x() + b.x()) / 2, b.y())
          << QPoint(a.x(), (a.y() + b.y()) / 2);
        p.drawPolygon(t);
    }

    // ------------------------------------------------------------
    // Implementaciones — firma (QPainter, QRect)
    // ------------------------------------------------------------
    static void drawRectangleR(QPainter &p, const QRect &r) {
        p.drawRect(r);
    }

    static void drawEllipseR(QPainter &p, const QRect &r) {
        p.drawEllipse(r);
    }

    static void drawRoundRectR(QPainter &p, const QRect &r) {
        p.drawRoundedRect(r, 12, 12);
    }

    static void drawPentagonR(QPainter &p, const QRect &r) {
        drawRegularPolygonR(p, r, 5);
    }

    static void drawHexagonR(QPainter &p, const QRect &r) {
        drawRegularPolygonR(p, r, 6);
    }

    static void drawRegularPolygonR(QPainter &p, const QRect &r, int sides) {
        QPolygon poly;
        for (int i = 0; i < sides; ++i) {
            const double angle = -M_PI / 2 + i * 2 * M_PI / sides;
            poly << QPoint(r.center().x() + r.width() / 2 * cos(angle),
                           r.center().y() + r.height() / 2 * sin(angle));
        }
        p.drawPolygon(poly);
    }

    static void drawStarR(QPainter &p, const QRect &r) {
        QPolygon poly;
        const int points = 5;
        for (int i = 0; i < points * 2; ++i) {
            const double angle = -M_PI / 2 + i * M_PI / points;
            const double f = (i % 2 == 1) ? 0.45 : 1.0;
            poly << QPoint(r.center().x() + r.width() / 2 * f * cos(angle),
                           r.center().y() + r.height() / 2 * f * sin(angle));
        }
        p.drawPolygon(poly);
    }

    static void drawArrowRightR(QPainter &p, const QRect &r) {
        drawArrowR(p, r, true);
    }

    static void drawArrowLeftR(QPainter &p, const QRect &r) {
        drawArrowR(p, r, false);
    }

    static void drawArrowR(QPainter &p, const QRect &r, bool right) {
        const int ym = r.top() + r.height() / 2;
        const int xb = right ? r.left() + r.width() * 0.55 : r.left() + r.width() * 0.45;
        const int tk = r.height() * 0.25;
        QPolygon poly;
        poly << QPoint(right ? r.left() : r.right(), ym - tk)
             << QPoint(xb, ym - tk)
             << QPoint(xb, r.top())
             << QPoint(right ? r.right() : r.left(), ym)
             << QPoint(xb, r.bottom())
             << QPoint(xb, ym + tk)
             << QPoint(right ? r.left() : r.right(), ym + tk);
        p.drawPolygon(poly);
    }

    static void drawHeartR(QPainter &p, const QRect &r) {
        QPainterPath path;
        path.moveTo(r.left() + r.width() / 2, r.top() + r.height() * 0.28);
        path.cubicTo(r.left() + r.width() * 0.1, r.top() - r.height() * 0.05,
                     r.left(), r.top() + r.height() * 0.6,
                     r.left() + r.width() / 2, r.bottom());
        path.cubicTo(r.right(), r.top() + r.height() * 0.6,
                     r.right() - r.width() * 0.1, r.top() - r.height() * 0.05,
                     r.left() + r.width() / 2, r.top() + r.height() * 0.28);
        p.drawPath(path);
    }

    static void drawCubeR(QPainter &p, const QRect &r) {
        int offset = qMin(r.width(), r.height()) * 0.3;
        if (offset < 4) offset = 4;
        const QRect front(r.left(), r.top() + offset, r.width() - offset, r.height() - offset);
        const QRect back(r.left() + offset, r.top(), r.width() - offset, r.height() - offset);
        p.drawRect(front);
        p.drawRect(back);
        p.drawLine(front.topLeft(), back.topLeft());
        p.drawLine(front.topRight(), back.topRight());
        p.drawLine(front.bottomLeft(), back.bottomLeft());
        p.drawLine(front.bottomRight(), back.bottomRight());
    }
};

// ============================================================
// MOTOR DE DIBUJO
// ============================================================
class PaintEngine {
public:
    // ------------------------------------------------------------
    // Nombres de figuras — delega en ShapeNames
    // ------------------------------------------------------------
    static QString shapeName(ShapeType s) {
        return ShapeNames::name(s);
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

    // ------------------------------------------------------------
    // baseShapePath — delega en ShapePathBuilder
    // ------------------------------------------------------------
    static QPainterPath baseShapePath(ShapeType shape) {
        static QHash<int, QPainterPath> cache;
        const int key = (int)shape;
        auto it = cache.constFind(key);
        if (it != cache.constEnd()) return it.value();

        QPainterPath path = ShapePathBuilder::build(shape);
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

    // ------------------------------------------------------------
    // Tintado
    // ------------------------------------------------------------
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
                line[x] = qRgba((cr * lum) / 255, (cg * lum) / 255, (cb * lum) / 255, a);
            }
        }
        return result;
    }

    // ------------------------------------------------------------
    // Helpers de aspect ratio
    // ------------------------------------------------------------
    static QRectF aspectRect(double cx, double cy, double size, double aspectRatio) {
        QRectF target(cx - size / 2.0, cy - size / 2.0, size, size);
        if (aspectRatio < 1.0) {
            const double newH = size * aspectRatio;
            target = QRectF(cx - size / 2.0, cy - newH / 2.0, size, newH);
        } else if (aspectRatio > 1.0) {
            const double newW = size * aspectRatio;
            target = QRectF(cx - newW / 2.0, cy - size / 2.0, newW, size);
        }
        return target;
    }

    static QRectF aspectRectInCanvas(double canvasW, double canvasH,
                                     double size, double aspectRatio) {
        QRectF target((canvasW - size) / 2.0, (canvasH - size) / 2.0, size, size);
        if (aspectRatio < 1.0) {
            const double newH = size * aspectRatio;
            target = QRectF((canvasW - size) / 2.0, (canvasH - newH) / 2.0, size, newH);
        } else if (aspectRatio > 1.0) {
            const double newW = size * aspectRatio;
            target = QRectF((canvasW - newW) / 2.0, (canvasH - size) / 2.0, newW, size);
        }
        return target;
    }

    // ------------------------------------------------------------
    // Dibujar primitiva
    // ------------------------------------------------------------
    static void drawShapePrimitive(QPainter &painter, ShapeType shape, const QRectF &r,
                                   const QColor &color, const QImage &customImage = QImage()) {
        if (r.width() <= 0.0 || r.height() <= 0.0) return;

        if (shape == ShapeType::CustomStamp) {
            if (customImage.isNull()) return;
            painter.save();
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

    // ------------------------------------------------------------
    // Silueta del pincel
    // ------------------------------------------------------------
    static void drawBrushSilhouette(QPainter &painter, const BrushSettings &config,
                                    double size, const QPointF &center) {
        painter.save();
        painter.translate(center);
        painter.rotate(config.angle);

        if (!config.shapeElements.isEmpty()) {
            drawCompositeSilhouette(painter, config, size);
        } else {
            drawSingleSilhouette(painter, config, size);
        }
        painter.restore();
    }

    static void drawCompositeSilhouette(QPainter &painter, const BrushSettings &config, double size) {
        const double spread = size * 1.5;
        double scaleX = 1.0, scaleY = 1.0;
        if (config.aspectRatio < 1.0) scaleY = config.aspectRatio;
        else if (config.aspectRatio > 1.0) scaleX = config.aspectRatio;
        painter.scale(scaleX, scaleY);

        for (const ShapeElement &el : config.shapeElements) {
            const double elCx = el.offsetX * spread;
            const double elCy = el.offsetY * spread;
            const double elSize = size * el.scale;
            const QRectF r(elCx - elSize / 2, elCy - elSize / 2, elSize, elSize);
            painter.save();
            painter.translate(elCx, elCy);
            painter.rotate(el.rotation);
            painter.translate(-elCx, -elCy);
            drawShapeOutline(painter, el.shape, r, el.customImage);
            painter.restore();
        }
    }

    static void drawSingleSilhouette(QPainter &painter, const BrushSettings &config, double size) {
        QRectF shapeRect(-size / 2.0, -size / 2.0, size, size);
        if (config.aspectRatio < 1.0) {
            const double newH = size * config.aspectRatio;
            shapeRect = QRectF(-size / 2.0, -newH / 2.0, size, newH);
        } else if (config.aspectRatio > 1.0) {
            const double newW = size * config.aspectRatio;
            shapeRect = QRectF(-newW / 2.0, -size / 2.0, newW, size);
        }
        drawShapeOutline(painter, config.shape, shapeRect, config.customStampImage);
    }

    // ------------------------------------------------------------
    // Breath factor
    // ------------------------------------------------------------
    static double computeBreathFactor(double accumulatedLength, int brushSize, int flow) {
        if (flow >= 100) return 1.0;
        if (brushSize < 1) brushSize = 1;
        const double strength = (100.0 - flow) / 100.0;
        const double cycleLength = qMax(20.0, brushSize * 5.0);
        const double phase = (accumulatedLength / cycleLength) * 2.0 * M_PI;
        const double wave = 0.5 - 0.5 * cos(phase);
        double factor = 1.0 - strength * (1.0 - wave);
        const double fadeInLen = brushSize * 1.0;
        if (accumulatedLength < fadeInLen) {
            const double fadeIn = accumulatedLength / fadeInLen;
            factor *= 0.3 + 0.7 * fadeIn;
        }
        return qBound(0.05, factor, 1.0);
    }

    // ------------------------------------------------------------
    // Granulación
    // ------------------------------------------------------------
    static void applyGranulation(QPainter &p, int canvasW, int canvasH, double angle) {
        p.setCompositionMode(QPainter::CompositionMode_SourceAtop);
        p.setRenderHint(QPainter::Antialiasing, false);

        const double grainAngle = angle * M_PI / 180.0 + M_PI / 5.0;
        const int numVeins = qMax(canvasW, canvasH) * 3;

        for (int i = 0; i < numVeins; ++i) {
            const int cx = QRandomGenerator::global()->bounded(canvasW);
            const int cy = QRandomGenerator::global()->bounded(canvasH);
            const double a = grainAngle + (QRandomGenerator::global()->generateDouble() - 0.5) * 0.6;
            const int len = 2 + QRandomGenerator::global()->bounded(qMax(3, qMax(canvasW, canvasH) / 8));
            const double roll = QRandomGenerator::global()->generateDouble();
            QColor veinColor;
            if (roll < 0.5)      veinColor = QColor(0, 0, 0, QRandomGenerator::global()->bounded(25, 70));
            else if (roll < 0.8) veinColor = QColor(255, 255, 255, QRandomGenerator::global()->bounded(15, 50));
            else                 veinColor = QColor(0, 0, 0, QRandomGenerator::global()->bounded(10, 35));
            p.setPen(QPen(veinColor, 1));
            p.drawLine(cx, cy,
                       cx + (int)(cos(a) * len),
                       cy + (int)(sin(a) * len));
        }

        const int numSpecks = (canvasW * canvasH) / 20;
        for (int i = 0; i < numSpecks; ++i) {
            const int nx = QRandomGenerator::global()->bounded(canvasW);
            const int ny = QRandomGenerator::global()->bounded(canvasH);
            const double roll = QRandomGenerator::global()->generateDouble();
            QColor speckColor;
            if (roll < 0.6) speckColor = QColor(0, 0, 0, QRandomGenerator::global()->bounded(15, 45));
            else            speckColor = QColor(255, 255, 255, QRandomGenerator::global()->bounded(10, 35));
            p.setPen(speckColor);
            p.drawPoint(nx, ny);
        }

        p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    }

    // ------------------------------------------------------------
    // Pintar shape con o sin gradiente
    // ------------------------------------------------------------
    static void paintShapeWithColor(QPainter &p, ShapeType shape, const QRectF &r,
                                    const QColor &color, bool doGradient,
                                    const QColor &c1, const QColor &c2) {
        if (doGradient) {
            QLinearGradient shapeGrad(r.topLeft(), r.bottomRight());
            shapeGrad.setColorAt(0.0, c1);
            shapeGrad.setColorAt(1.0, c2);
            p.setPen(Qt::NoPen);
            p.setBrush(shapeGrad);
            p.drawPath(shapePath(shape, r));
        } else {
            drawShapePrimitive(p, shape, r, color);
        }
    }

    // ------------------------------------------------------------
    // Stamp simple
    // ------------------------------------------------------------
    static void paintSingleStampShape(QPainter &p, const BrushSettings &config,
                                      const QColor &baseColor, const QColor &secondColor,
                                      double alpha, int canvasW, int canvasH,
                                      int size, int pad) {
        QRectF shapeRect(pad, pad, size, size);
        if (config.aspectRatio != 1.0)
            shapeRect = aspectRectInCanvas(canvasW, canvasH, size, config.aspectRatio);

        p.save();
        p.translate(canvasW / 2.0, canvasH / 2.0);
        p.rotate(config.angle);
        p.translate(-canvasW / 2.0, -canvasH / 2.0);

        const bool doGradient = config.mixSecondColor && secondColor.isValid();

        if (config.shape == ShapeType::CustomStamp) {
            if (!config.customStampImage.isNull()) {
                p.setOpacity(qBound(0.0, alpha, 1.0));
                p.drawImage(shapeRect, config.customStampImage);
                p.setOpacity(1.0);
            }
        } else if (doGradient) {
            QColor c1 = baseColor; c1.setAlphaF(qBound(0.0, alpha, 1.0));
            QColor c2 = secondColor; c2.setAlphaF(qBound(0.0, alpha, 1.0));
            paintShapeWithColor(p, config.shape, shapeRect, baseColor, true, c1, c2);
        } else {
            QColor stampColor = baseColor;
            stampColor.setAlphaF(qBound(0.0, alpha, 1.0));

            if (config.shape == ShapeType::Circle) {
                QRadialGradient grad(canvasW / 2.0, canvasH / 2.0, size / 2.0);
                QColor edge = stampColor; edge.setAlpha(0);
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

    // ------------------------------------------------------------
    // Stamp compuesto
    // ------------------------------------------------------------
    static void paintCompositeStamp(QPainter &p, const BrushSettings &config,
                                    const QColor &baseColor, const QColor &secondColor,
                                    double alpha, double size) {
        const double centerX = p.device()->width() / 2.0;
        const double centerY = p.device()->height() / 2.0;
        const double spread = size * 1.5;
        const bool doGradient = config.mixSecondColor && secondColor.isValid();

        p.save();
        p.translate(centerX, centerY);
        p.rotate(config.angle);

        for (const ShapeElement &el : config.shapeElements) {
            paintCompositeElement(p, el, baseColor, secondColor, alpha, spread, doGradient);
        }
        p.restore();
    }

    static void paintCompositeElement(QPainter &p, const ShapeElement &el,
                                      const QColor &baseColor, const QColor &secondColor,
                                      double alpha, double spread, bool doGradient) {
        const double elCx = el.offsetX * spread;
        const double elCy = el.offsetY * spread;
        const double elSize = el.scale * (spread / 1.5);
        const QRectF r(elCx - elSize / 2, elCy - elSize / 2, elSize, elSize);
        const double elAlpha = alpha * (el.opacity / 100.0);

        p.save();
        p.translate(elCx, elCy);
        p.rotate(el.rotation);
        p.translate(-elCx, -elCy);

        if (el.shape == ShapeType::CustomStamp) {
            if (!el.customImage.isNull()) {
                p.setOpacity(qBound(0.0, elAlpha, 1.0));
                p.drawImage(r, el.customImage);
                p.setOpacity(1.0);
            }
        } else if (doGradient) {
            QColor c1 = baseColor; c1.setAlphaF(qBound(0.0, elAlpha, 1.0));
            QColor c2 = secondColor; c2.setAlphaF(qBound(0.0, elAlpha, 1.0));
            paintShapeWithColor(p, el.shape, r, baseColor, true, c1, c2);
        } else {
            QColor c = baseColor;
            c.setAlphaF(qBound(0.0, elAlpha, 1.0));
            drawShapePrimitive(p, el.shape, r, c);
        }
        p.restore();
    }

    // ------------------------------------------------------------
    // generateBrushStamp
    // ------------------------------------------------------------
    static QImage generateBrushStamp(const BrushSettings &config, const QColor &baseColor,
                                     int penOpacity, bool pixelArt,
                                     const QColor &secondColor = QColor()) {
        const int size = qMax(1, config.size);
        const int pad = 6;
        const bool composite = !config.shapeElements.isEmpty();

        int canvasW, canvasH;
        if (composite) {
            const int baseSize = size * 6 + pad * 2;
            canvasW = baseSize;
            canvasH = baseSize;
            if (config.aspectRatio > 1.0)
                canvasW = qMax(1, (int)(baseSize * config.aspectRatio));
        } else {
            canvasW = size + pad * 2;
            canvasH = size + pad * 2;
            if (config.aspectRatio > 1.0)
                canvasW = (int)(size * config.aspectRatio) + pad * 2;
        }

        QImage stamp(canvasW, canvasH, QImage::Format_ARGB32);
        stamp.fill(Qt::transparent);

        QPainter p(&stamp);
        p.setRenderHint(QPainter::Antialiasing, !pixelArt);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);

        const double alpha = (config.opacity / 100.0) * (penOpacity / 255.0);

        if (!composite && config.shape == ShapeType::CustomStamp) {
            if (config.customStampImage.isNull()) { p.end(); return stamp; }
            const double cx = canvasW / 2.0, cy = canvasH / 2.0;
            const QRectF target = aspectRect(cx, cy, size, config.aspectRatio);
            p.setOpacity(qBound(0.0, alpha, 1.0));
            p.drawImage(target, config.customStampImage);
            p.end();
            return stamp;
        }

        if (composite) {
            paintCompositeStamp(p, config, baseColor, secondColor, alpha, size);
        } else {
            paintSingleStampShape(p, config, baseColor, secondColor, alpha, canvasW, canvasH, size, pad);
        }

        if (config.granulation)
            applyGranulation(p, canvasW, canvasH, config.angle);

        p.end();
        return stamp;
    }

    // ------------------------------------------------------------
    // Lápiz grafito
    // ------------------------------------------------------------
    static void applyGraphitePencil(QImage &image, const QPoint &p1, const QPoint &p2,
                                    const QColor &color, int width, int opacity) {
        if (image.isNull()) return;
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const double dx = p2.x() - p1.x();
        const double dy = p2.y() - p1.y();
        const double dist = sqrt(dx * dx + dy * dy);
        const int steps = qMax(1, (int)(dist / 1.5));
        const double softness = 0.75;
        const double coreRadius = width * (0.8 + softness * 0.7);
        const double strokeAngle = atan2(dy, dx);

        for (int i = 0; i <= steps; ++i) {
            const double t = (double)i / steps;
            const double cx = p1.x() + t * dx;
            const double cy = p1.y() + t * dy;
            for (int g = 0; g < 12; ++g) {
                drawGrainPoint(painter, image, color, opacity, softness,
                               coreRadius, strokeAngle, cx, cy);
            }
        }
        painter.end();
    }

    static void drawGrainPoint(QPainter &painter, const QImage &image, const QColor &color,
                               int opacity, double softness, double coreRadius,
                               double strokeAngle, double cx, double cy) {
        const double ang = strokeAngle + (QRandomGenerator::global()->generateDouble() - 0.5) * 1.3;
        const double r = pow(QRandomGenerator::global()->generateDouble(), 0.65) * coreRadius;
        const int gx = (int)(cx + cos(ang) * r);
        const int gy = (int)(cy + sin(ang) * r);
        if (gx < 0 || gx >= image.width() || gy < 0 || gy >= image.height()) return;

        const double pressure = 1.0 - (r / qMax(0.001, coreRadius));
        QColor gc = color;
        int h, s, l, a;
        gc.getHsl(&h, &s, &l, &a);
        const int nl = qBound(0, l - (int)(35 * softness) + QRandomGenerator::global()->bounded(-18, 19), 255);
        gc.setHsl(h, (int)(s * 0.25), nl);
        gc.setAlpha((int)(opacity * (0.18 + 0.5 * softness) * (0.35 + 0.65 * pressure) *
                          QRandomGenerator::global()->generateDouble()));
        painter.setPen(gc);
        const int glen = 1 + QRandomGenerator::global()->bounded(3);
        painter.drawLine(gx, gy, gx + (int)(cos(ang) * glen), gy + (int)(sin(ang) * glen));
    }

    // ------------------------------------------------------------
    // Goma
    // ------------------------------------------------------------
    static void applyEraserLine(QImage &image, const QPoint &p1, const QPoint &p2,
                                int width, bool softEdge) {
        if (image.isNull()) return;
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setCompositionMode(QPainter::CompositionMode_Clear);

        if (softEdge) {
            eraseSoft(painter, p1, p2, width);
        } else {
            painter.setPen(QPen(QColor(0, 0, 0, 255), width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.drawLine(p1, p2);
        }
        painter.end();
    }

    static void eraseSoft(QPainter &painter, const QPoint &p1, const QPoint &p2, int width) {
        const double dx = p2.x() - p1.x();
        const double dy = p2.y() - p1.y();
        const double dist = sqrt(dx * dx + dy * dy);
        const int steps = qMax(1, (int)(dist / 2.0));
        for (int i = 0; i <= steps; ++i) {
            const double t = (double)i / steps;
            const int cx = (int)(p1.x() + t * dx);
            const int cy = (int)(p1.y() + t * dy);
            QRadialGradient grad(cx, cy, width);
            grad.setColorAt(0.0, QColor(0, 0, 0, 255));
            grad.setColorAt(0.6, QColor(0, 0, 0, 180));
            grad.setColorAt(1.0, QColor(0, 0, 0, 0));
            painter.setPen(Qt::NoPen);
            painter.setBrush(grad);
            painter.drawEllipse(cx - width, cy - width, width * 2, width * 2);
        }
    }

    // ------------------------------------------------------------
    // Muestreo de color
    // ------------------------------------------------------------
    static QColor sampleCanvasColor(const QImage &image, const QPoint &pos, int radius) {
        if (image.isNull()) return QColor();
        const int x0 = qMax(0, pos.x() - radius);
        const int y0 = qMax(0, pos.y() - radius);
        const int x1 = qMin(image.width() - 1, pos.x() + radius);
        const int y1 = qMin(image.height() - 1, pos.y() + radius);
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
                    const QRgb px = line[x];
                    if (qAlpha(px) > 20) { r += qRed(px); g += qGreen(px); b += qBlue(px); count++; }
                }
            }
        } else {
            for (int y = y0; y <= y1; y += step) {
                for (int x = x0; x <= x1; x += step) {
                    const QColor c = image.pixelColor(x, y);
                    if (c.alpha() > 20) { r += c.red(); g += c.green(); b += c.blue(); count++; }
                }
            }
        }

        if (count == 0) return QColor();
        return QColor((int)(r / count), (int)(g / count), (int)(b / count), 255);
    }

    // ------------------------------------------------------------
    // Wet mix
    // ------------------------------------------------------------
    static QImage wetMixStamp(const QImage &stamp, const QColor &canvasColor, double wetAmount) {
        QImage result = stamp.copy();
        if (result.isNull()) return result;

        const double t = qBound(0.0, wetAmount / 100.0, 1.0);
        if (t <= 0.001) return result;

        if (result.format() != QImage::Format_ARGB32)
            result = result.convertToFormat(QImage::Format_ARGB32);

        const int cr = canvasColor.red();
        const int cg = canvasColor.green();
        const int cb = canvasColor.blue();
        const double inv = 1.0 - t;

        for (int y = 0; y < result.height(); ++y) {
            QRgb *line = reinterpret_cast<QRgb*>(result.scanLine(y));
            for (int x = 0; x < result.width(); ++x) {
                const QRgb px = line[x];
                const int a = qAlpha(px);
                if (a > 0) {
                    line[x] = qRgba((int)(qRed(px) * inv + cr * t),
                                    (int)(qGreen(px) * inv + cg * t),
                                    (int)(qBlue(px) * inv + cb * t),
                                    a);
                }
            }
        }
        return result;
    }

    // ------------------------------------------------------------
    // Dibujar stamp puntual
    // ------------------------------------------------------------
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

    // ------------------------------------------------------------
    // Aplicar stamp puntual
    // ------------------------------------------------------------
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

        const QPoint scattered = computeScatterOffset(pos, config, mouseSensitivity);
        const double drawAngle = computeDrawAngle(config, extraAngle);
        const double drawOpacity = computeDrawOpacity(config, opacityScale, taperFactor);
        const double scale = computeDrawScale(config, sizeScale, taperFactor);

        QImage drawStamp = stamp;
        if (config.wetMix && baseColor.isValid()) {
            const int sampleRadius = qMax(2, config.size / 2);
            QColor sampled = sampleCanvasColor(image, scattered, sampleRadius);
            if (!sampled.isValid() && fallbackCanvasColor.isValid())
                sampled = fallbackCanvasColor;
            if (sampled.isValid())
                drawStamp = wetMixStamp(stamp, sampled, config.wetAmount);
        }

        drawStampAt(image, scattered, drawStamp, drawAngle, drawOpacity, scale);
    }

    static QPoint computeScatterOffset(const QPoint &pos, const BrushSettings &config,
                                       double mouseSensitivity) {
        if (config.scatter <= 0) return pos;
        const int scatterAmount = (int)(config.scatter * mouseSensitivity);
        return QPoint(pos.x() + QRandomGenerator::global()->bounded(-scatterAmount, scatterAmount + 1),
                      pos.y() + QRandomGenerator::global()->bounded(-scatterAmount, scatterAmount + 1));
    }

    static double computeDrawAngle(const BrushSettings &config, double extraAngle) {
        double angle = config.angle + extraAngle;
        if (config.rotationMode == RotationMode::Random)
            angle += QRandomGenerator::global()->bounded(0, 360);
        if (config.angleJitter > 0)
            angle += QRandomGenerator::global()->bounded(-config.angleJitter, config.angleJitter + 1);
        return angle;
    }

    static double computeDrawOpacity(const BrushSettings &config, double opacityScale,
                                     double taperFactor) {
        double opacity = config.isAirbrush ? 0.3 : 1.0;
        opacity *= opacityScale;
        opacity *= taperFactor;
        if (config.opacityJitter > 0) {
            const double jitter = QRandomGenerator::global()->generateDouble() * (config.opacityJitter / 100.0);
            opacity *= qBound(0.1, 1.0 - jitter, 1.0);
        }
        return opacity;
    }

    static double computeDrawScale(const BrushSettings &config, double sizeScale,
                                   double taperFactor) {
        double scale = sizeScale * taperFactor;
        if (config.sizeJitter > 0) {
            const double jitter = QRandomGenerator::global()->generateDouble() * (config.sizeJitter / 100.0);
            scale *= qBound(0.3, 1.0 - jitter + QRandomGenerator::global()->generateDouble() * jitter * 2, 1.7);
        }
        return scale;
    }

    // ------------------------------------------------------------
    // Spacing y ribbon
    // ------------------------------------------------------------
    static double computeSpacing(const BrushSettings &config, double mouseSensitivity) {
        double baseSpacing = qMax(1.0, (config.size * qMax(1, 6 - config.density)) / 100.0);
        switch (config.dragMode) {
        case DragMode::Continuous: baseSpacing = qMax(1.0, config.size * 0.08); break;
        case DragMode::Dotted:     baseSpacing = qMax((double)config.size * 2.0, baseSpacing * 2.5); break;
        case DragMode::Scattered:  baseSpacing = qMax(2.0, baseSpacing * 1.5); break;
        default: break;
        }
        baseSpacing *= (2.0 - mouseSensitivity);
        return qMax(1.0, baseSpacing);
    }

    static void applyRibbonLine(QImage &image, const QPointF &from, const QPointF &to,
                                const BrushSettings &config, const QColor &baseColor,
                                QPointF &lastPoint) {
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QColor c = baseColor.isValid() ? baseColor : QColor(0, 0, 0);
        c.setAlphaF(qBound(0.0, config.opacity / 100.0, 1.0));
        const double ribbonW = qMax(1.0, config.size * config.aspectRatio);
        painter.setPen(QPen(c, ribbonW, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
        painter.drawLine(from, to);
        painter.end();
        lastPoint = to;
    }

    // ------------------------------------------------------------
    // Aplicar línea de pincel
    // ------------------------------------------------------------
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

        const double dist = sqrt(pow(to.x() - from.x(), 2) + pow(to.y() - from.y(), 2));
        if (dist < 0.5) return;

        if (config.dragMode == DragMode::Ribbon) {
            applyRibbonLine(image, from, to, config, baseColor, lastPoint);
            return;
        }

        const double baseSpacing = computeSpacing(config, mouseSensitivity);
        if (dist < baseSpacing) return;

        const QImage lineStamp = resolveLineStamp(image, from, stamp, config,
                                                  baseColor, fallbackCanvasColor);
        const double dirAngle = atan2(to.y() - from.y(), to.x() - from.x()) * 180.0 / M_PI;

        paintSegmentsAlongLine(image, from, to, dist, lineStamp, config,
                               dirAngle, secondColor, fallbackCanvasColor,
                               accumulatedLength, baseSpacing);

        lastPoint = to;
    }

    static QImage resolveLineStamp(const QImage &image, const QPointF &from,
                                   const QImage &stamp, const BrushSettings &config,
                                   const QColor &baseColor,
                                   const QColor &fallbackCanvasColor) {
        if (!config.wetMix || !baseColor.isValid()) return stamp;
        QColor sampled = sampleCanvasColor(image, from.toPoint(), qMax(2, config.size / 2));
        if (!sampled.isValid() && fallbackCanvasColor.isValid())
            sampled = fallbackCanvasColor;
        if (sampled.isValid())
            return wetMixStamp(stamp, sampled, config.wetAmount);
        return stamp;
    }

    static void paintSegmentsAlongLine(QImage &image, const QPointF &from, const QPointF &to,
                                       double dist, const QImage &lineStamp,
                                       const BrushSettings &config, double dirAngle,
                                       const QColor &secondColor,
                                       const QColor &fallbackCanvasColor,
                                       double accumulatedLength, double baseSpacing) {
        const int steps = qMax(1, (int)(dist / baseSpacing));
        for (int s = 1; s <= steps; ++s) {
            const double t = (double)s / steps;
            const int cx = (int)(from.x() + t * (to.x() - from.x()));
            const int cy = (int)(from.y() + t * (to.y() - from.y()));

            const double taperFactor = computeTaperFactor(config, accumulatedLength, dist, t);
            const double extraAngle = (config.rotationMode == RotationMode::FollowDirection) ? dirAngle : 0.0;
            const double sizeScale = (config.dragMode == DragMode::Dotted) ? 0.7 : 1.0;
            const int densityCount = qBound(1, config.density, 20);

            for (int d = 0; d < densityCount; ++d) {
                const QPoint offset = computeDensityOffset(config, densityCount);
                applyCustomBrushStroke(image, QPoint(cx + offset.x(), cy + offset.y()),
                                       lineStamp, config, 1.0, extraAngle, sizeScale, 1.0,
                                       QColor(), secondColor, fallbackCanvasColor, taperFactor);
            }
        }
    }

    static double computeTaperFactor(const BrushSettings &config, double accumulatedLength,
                                     double dist, double t) {
        if (config.flow >= 100) return 1.0;
        const double currentLen = accumulatedLength + t * dist;
        return computeBreathFactor(currentLen, config.size, config.flow);
    }

    static QPoint computeDensityOffset(const BrushSettings &config, int densityCount) {
        int dx = 0, dy = 0;
        if (densityCount > 1) {
            const int jitter = qMax(1, config.size / 4);
            dx = QRandomGenerator::global()->bounded(-jitter, jitter + 1);
            dy = QRandomGenerator::global()->bounded(-jitter, jitter + 1);
        }
        if (config.dragMode == DragMode::Scattered) {
            const int extraScatter = qMax(2, config.scatter + config.size / 2);
            dx += QRandomGenerator::global()->bounded(-extraScatter, extraScatter + 1);
            dy += QRandomGenerator::global()->bounded(-extraScatter, extraScatter + 1);
        }
        return QPoint(dx, dy);
    }

    // ------------------------------------------------------------
    // Wrappers de retoque
    // ------------------------------------------------------------
    static void applyBlur(QImage &image, const QPoint &pos, int radius) {
        RetouchTools::applyBlur(image, pos, radius);
    }
    static void applyHeal(QImage &image, const QPoint &pos, int radius) {
        RetouchTools::applyHeal(image, pos, radius);
    }
    static void applyShadowBurn(QImage &image, const QPoint &pos, int radius,
                                double sensitivity, int opacity) {
        RetouchTools::applyShadowBurn(image, pos, radius, sensitivity, opacity);
    }

    // ------------------------------------------------------------
    // Geometría (delega en GeometryDrawer)
    // ------------------------------------------------------------
    static void drawGeometry(QPainter &painter, const QPoint &p1, const QPoint &p2, ToolType tool) {
        GeometryDrawer::draw(painter, p1, p2, tool);
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

inline BrushSettings makePreset(int size, ShapeType shape, DragMode drag, RotationMode rot,
                                int opacity, int scatter, double angle, int density, int flow,
                                int sizeJitter, int angleJitter, int opacityJitter,
                                double aspectRatio, bool wet, int wetAmount, bool granulation,
                                bool isAirbrush = false) {
    BrushSettings s;
    s.size = size;
    s.shape = shape;
    s.dragMode = drag;
    s.rotationMode = rot;
    s.opacity = opacity;
    s.scatter = scatter;
    s.angle = angle;
    s.density = density;
    s.flow = flow;
    s.sizeJitter = sizeJitter;
    s.angleJitter = angleJitter;
    s.opacityJitter = opacityJitter;
    s.aspectRatio = aspectRatio;
    s.wetMix = wet;
    s.wetAmount = wetAmount;
    s.granulation = granulation;
    s.isAirbrush = isAirbrush;
    return s;
}

inline BrushSettings watercolor() {
    return makePreset(40, ShapeType::Circle, DragMode::Continuous, RotationMode::Random,
                      2, 25, 0.0, 4, 100, 30, 45, 25, 1.0, true, 50, false);
}
inline BrushSettings oilBrush() {
    return makePreset(30, ShapeType::FlatTip, DragMode::Continuous, RotationMode::FollowDirection,
                      95, 20, 0.0, 2, 100, 10, 15, 8, 0.4, true, 60, false);
}
inline BrushSettings crayon() {
    return makePreset(40, ShapeType::Clover, DragMode::Continuous, RotationMode::Random,
                      40, 15, 40.0, 2, 100, 20, 30, 100, 1.0, true, 70, true);
}
inline BrushSettings marker() {
    return makePreset(22, ShapeType::RoundedSquare, DragMode::Continuous, RotationMode::FollowDirection,
                      10, 1, 0.0, 1, 100, 0, 0, 0, 0.5, true, 10, true);
}
inline BrushSettings calligraphy() {
    return makePreset(18, ShapeType::ChiselTip, DragMode::Continuous, RotationMode::Fixed,
                      90, 0, 45.0, 1, 100, 0, 0, 0, 0.3, false, 50, false);
}
inline BrushSettings highlighter() {
    return makePreset(28, ShapeType::ChiselTip, DragMode::Ribbon, RotationMode::Fixed,
                      40, 0, 0.0, 1, 100, 0, 0, 0, 0.4, false, 50, false);
}
inline BrushSettings softBrush() {
    return makePreset(30, ShapeType::Circle, DragMode::Continuous, RotationMode::Fixed,
                      85, 5, 0.0, 1, 100, 10, 0, 5, 1.0, false, 50, false);
}
inline BrushSettings sprayCan() {
    return makePreset(20, ShapeType::Circle, DragMode::Scattered, RotationMode::Random,
                      70, 40, 0.0, 3, 100, 50, 180, 40, 1.0, false, 50, false, true);
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

} // namespace ArtisticPresets

// ============================================================
// SHAPE BUTTON
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
        drag->setPixmap(grab());
        drag->setHotSpot(e->pos());
        drag->exec(Qt::CopyAction);
    }

    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);

        const ButtonLook look = resolveLook();
        paintBackground(p, look);
        paintBorder(p, look);

        if (look.kind == ButtonKind::ImportEmpty) {
            paintImportEmptyIcon(p);
        } else if (shape == ShapeType::CustomStamp && !customImage.isNull()) {
            paintCustomImage(p);
        } else {
            paintShapeIcon(p, look.fig);
        }
    }

private:
    enum class ButtonKind { ImportEmpty, ImportFilled, Regular };

    struct ButtonLook {
        ButtonKind kind = ButtonKind::Regular;
        QColor bg;
        QColor border;
        QColor fig;
    };

    ButtonLook resolveLook() const {
        if (isImportButton && isEmptyImport) return lookImportEmpty();
        if (isImportButton && !isEmptyImport) return lookImportFilled();
        return lookRegular();
    }

    ButtonLook lookImportEmpty() const {
        ButtonLook look;
        look.kind = ButtonKind::ImportEmpty;
        if (underMouse()) {
            look.bg     = m_dark ? QColor("#2a4a2a") : QColor("#dcfce7");
            look.border = QColor("#22c55e");
            look.fig    = QColor("#22c55e");
        } else {
            look.bg     = m_dark ? QColor("#1f3a1f") : QColor("#f0fdf4");
            look.border = QColor("#16a34a");
            look.fig    = QColor("#22c55e");
        }
        return look;
    }

    ButtonLook lookImportFilled() const {
        ButtonLook look;
        look.kind = ButtonKind::ImportFilled;
        if (isChecked()) {
            look.bg     = m_dark ? QColor("#1a3a5c") : QColor("#eff6ff");
            look.border = QColor("#3b82f6");
        } else if (underMouse()) {
            look.bg     = m_dark ? QColor("#3a3a3a") : QColor("#f1f5f9");
            look.border = QColor("#22c55e");
        } else {
            look.bg     = m_dark ? QColor("#2a2a2a") : QColor("#ffffff");
            look.border = QColor("#22c55e");
        }
        return look;
    }

    ButtonLook lookRegular() const {
        ButtonLook look;
        look.kind = ButtonKind::Regular;
        if (isChecked()) {
            look.bg     = m_dark ? QColor("#1a3a5c") : QColor("#eff6ff");
            look.border = QColor("#3b82f6");
        } else if (underMouse()) {
            look.bg     = m_dark ? QColor("#3a3a3a") : QColor("#f1f5f9");
            look.border = m_dark ? QColor("#555555") : QColor("#cbd5e1");
        } else {
            look.bg     = m_dark ? QColor("#2a2a2a") : QColor("#ffffff");
            look.border = m_dark ? QColor("#3a3a3a") : QColor("#d1d5db");
        }
        return look;
    }

    void paintBackground(QPainter &p, const ButtonLook &look) {
        p.setPen(Qt::NoPen);
        p.setBrush(look.bg);
        p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 6, 6);
    }

    void paintBorder(QPainter &p, const ButtonLook &look) {
        const bool strong = isChecked() || (isImportButton && isEmptyImport);
        p.setPen(QPen(look.border, strong ? 2 : 1));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(rect().adjusted(1, 1, -2, -2), 6, 6);
    }

    void paintImportEmptyIcon(QPainter &p) {
        const int cx = width() / 2;
        const int cy = height() / 2 - 4;
        QColor fg;
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
    }

    void paintCustomImage(QPainter &p) {
        const QRectF shapeRect(4, 4, width() - 8, height() - 8);
        const QImage scaled = customImage.scaled(shapeRect.size().toSize(),
                                                 Qt::KeepAspectRatio,
                                                 Qt::SmoothTransformation);
        const QRectF target(shapeRect.center().x() - scaled.width() / 2.0,
                            shapeRect.center().y() - scaled.height() / 2.0,
                            scaled.width(), scaled.height());
        p.drawImage(target, scaled);
    }

    void paintShapeIcon(QPainter &p, const QColor &fig) {
        const QRectF shapeRect(7, 7, width() - 14, height() - 14);
        QColor fg = fig;
        if (isChecked())        fg = m_dark ? QColor("#60a5fa") : QColor("#1d4ed8");
        else if (underMouse())  fg = m_dark ? QColor("#c0c0c0") : QColor("#475569");
        else                    fg = m_dark ? QColor("#c0c0c0") : QColor("#475569");
        PaintEngine::drawShapePrimitive(p, shape, shapeRect, fg);
    }

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
            const int maxW = width() - 12;
            const int maxH = height() - 12;
            const QImage scaled = stampImage.scaled(maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
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

    void setSelectedIndex(int idx) {
        selectedIndex = idx;
        update();
        emit selectionChanged(selectedIndex);
    }

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
        if (!e->mimeData()->hasFormat("application/x-shape-type")) return;
        const int shapeInt = e->mimeData()->data("application/x-shape-type").toInt();
        const ShapeType s = (ShapeType)shapeInt;
        const QPointF localPos = e->position();
        const double ox = qBound(-1.0, (localPos.x() - width() / 2.0) / (width() / 2.0), 1.0);
        const double oy = qBound(-1.0, (localPos.y() - height() / 2.0) / (height() / 2.0), 1.0);
        if (s == ShapeType::CustomStamp) {
            emit requestCustomStamp(s, QImage());
        } else {
            addElement(s, ox, oy);
        }
        e->acceptProposedAction();
    }

    void mousePressEvent(QMouseEvent *e) override {
        const QPointF pos = e->position();
        int best = -1;
        double bestDist = 1e9;
        for (int i = 0; i < elements.size(); ++i) {
            const double ex = width() / 2.0 + elements[i].offsetX * (width() / 2.0);
            const double ey = height() / 2.0 + elements[i].offsetY * (height() / 2.0);
            const double d = sqrt(pow(pos.x() - ex, 2) + pow(pos.y() - ey, 2));
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
        const QPointF pos = e->position();
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
        paintCrosshair(p);
        paintElements(p);
        paintBorder(p);
        if (elements.isEmpty()) paintEmptyHint(p);
    }

private:
    void paintCrosshair(QPainter &p) {
        p.setPen(QPen(m_dark ? QColor("#373737") : QColor("#e2e8f0"), 1, Qt::DashLine));
        p.drawLine(width() / 2, 0, width() / 2, height());
        p.drawLine(0, height() / 2, width(), height() / 2);
    }

    void paintElements(QPainter &p) {
        const double baseSize = qMin(width(), height()) * 0.20;
        for (int i = 0; i < elements.size(); ++i) {
            paintSingleElement(p, elements[i], i, baseSize);
        }
    }

    void paintSingleElement(QPainter &p, const ShapeElement &el, int idx, double baseSize) {
        const double cx = width() / 2.0 + el.offsetX * (width() / 2.0);
        const double cy = height() / 2.0 + el.offsetY * (height() / 2.0);
        const double sz = baseSize * el.scale;
        const QRectF r(cx - sz / 2, cy - sz / 2, sz, sz);

        p.save();
        p.translate(cx, cy);
        p.rotate(el.rotation);
        p.translate(-cx, -cy);

        if (el.shape == ShapeType::CustomStamp && !el.customImage.isNull()) {
            p.setOpacity(el.opacity / 100.0);
            p.drawImage(r, el.customImage);
            p.setOpacity(1.0);
        } else {
            QColor c = (idx == selectedIndex)
                           ? QColor("#3b82f6")
                           : (m_dark ? QColor("#d0d0d0") : QColor("#475569"));
            c.setAlpha((int)(el.opacity * 2.55));
            PaintEngine::drawShapePrimitive(p, el.shape, r, c);
        }

        if (idx == selectedIndex) {
            p.setPen(QPen(QColor("#3b82f6"), 1, Qt::DashLine));
            p.setBrush(Qt::NoBrush);
            p.drawRect(r.adjusted(-3, -3, 3, 3));
        }
        p.restore();
    }

    void paintBorder(QPainter &p) {
        p.setPen(QPen(m_dark ? QColor("#3a3a3a") : QColor("#d1d5db"), 1));
        p.setBrush(Qt::NoBrush);
        p.drawRect(rect().adjusted(0, 0, -1, -1));
    }

    void paintEmptyHint(QPainter &p) {
        p.setPen(m_dark ? QColor("#8a8a8a") : QColor("#9ca3af"));
        p.setFont(QFont("Adwaita Sans", 8));
        p.drawText(rect(), Qt::AlignCenter, tr("Arrastra figuras\naqui para componer"));
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
        if (width() <= 4 || height() <= 4) {
            cachedPreview = QImage();
            dirty = false;
            return;
        }

        QImage img(width(), height(), QImage::Format_ARGB32);
        img.fill(m_dark ? QColor("#1e1e1e") : QColor("#f3f4f6"));

        BrushSettings previewSettings = settings;
        previewSettings.density = qMin(previewSettings.density, 3);
        previewSettings.size = qMin(previewSettings.size, 28);

        const QImage stamp = PaintEngine::generateBrushStamp(
            previewSettings, brushColor, 255, false, secondColor);

        const QVector<QPointF> points = buildPreviewPath();
        paintPreviewStroke(img, stamp, previewSettings, points);

        cachedPreview = img;
        dirty = false;
    }

    QVector<QPointF> buildPreviewPath() const {
        const int steps = qBound(24, width() / 10, 42);
        QVector<QPointF> points;
        points.reserve(steps + 1);
        for (int i = 0; i <= steps; ++i) {
            const double t = (double)i / steps;
            const double x = 20 + t * (width() - 40);
            const double y = height() / 2.0 + sin(t * M_PI * 2.0) * (height() * 0.22);
            points.append(QPointF(x, y));
        }
        return points;
    }

    void paintPreviewStroke(QImage &img, const QImage &stamp,
                            const BrushSettings &previewSettings,
                            const QVector<QPointF> &points) {
        QPointF last(-1000, -1000);
        double accum = 0.0;
        for (int i = 0; i < points.size(); ++i) {
            const QPointF cur = points[i];
            if (i == 0) {
                last = cur;
                PaintEngine::applyCustomBrushStroke(img, cur.toPoint(), stamp, previewSettings,
                                                    1.0, 0.0, 1.0, 1.0,
                                                    brushColor, secondColor, QColor(), 1.0);
            } else {
                const double segLen = QLineF(last, cur).length();
                PaintEngine::applyCustomBrushLine(img, last, cur, stamp, previewSettings,
                                                  1.0, last, brushColor, secondColor, QColor(),
                                                  0.0, accum);
                accum += segLen;
                last = cur;
            }
        }
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
        p.drawText(rect().adjusted(6, 4, -6, -4), Qt::AlignLeft | Qt::AlignTop, buildLabel());
    }

private:
    QString buildLabel() const {
        QString label = PaintEngine::shapeName(settings.shape);
        if (!settings.shapeElements.isEmpty())
            label = tr("Compuesto: %1 figuras").arg(settings.shapeElements.size());
        if (settings.mixSecondColor) label += tr(" + 2do color");
        if (settings.wetMix)         label += tr(" + Wet");
        if (settings.granulation)    label += tr(" + Granulado");
        if (settings.flow < 100)     label += tr(" | Flujo %1%").arg(settings.flow);
        return label;
    }
};

// ============================================================
// JITTERDIALOG
// ============================================================
class JitterDialog : public QDialog {
    Q_OBJECT
private:
    QSlider *sizeSlider = nullptr;
    QSlider *angleSlider = nullptr;
    QSlider *opacitySlider = nullptr;
public:
    JitterDialog(bool darkMode, int sizeJ, int angleJ, int opacityJ, QWidget *parent = nullptr)
        : QDialog(parent) {
        setWindowTitle(tr("Variacion aleatoria (jitter)"));
        setFixedSize(360, 250);
        applyTheme(darkMode);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(16, 16, 16, 14);
        layout->setSpacing(10);

        addSliderRow(layout, tr("Tamano"), 0, 100, sizeJ, &sizeSlider, "%", darkMode);
        addSliderRow(layout, tr("Angulo"), 0, 180, angleJ, &angleSlider,
                     QString::fromUtf8("\xC2\xB0"), darkMode);
        addSliderRow(layout, tr("Opacidad"), 0, 100, opacityJ, &opacitySlider, "%", darkMode);

        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    int getSizeJitter()    const { return sizeSlider->value(); }
    int getAngleJitter()   const { return angleSlider->value(); }
    int getOpacityJitter() const { return opacitySlider->value(); }

private:
    void applyTheme(bool darkMode) {
        const QString bg     = darkMode ? "#1a1a1a" : "#f5f5f5";
        const QString input  = darkMode ? "#2a2a2a" : "#ffffff";
        const QString txt    = darkMode ? "#e5e5e5" : "#111827";
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
    }

    void addSliderRow(QVBoxLayout *layout, const QString &label, int min, int max,
                      int val, QSlider **out, const QString &suffix, bool darkMode) {
        const QString muted = darkMode ? "#a0a0a0" : "#6b7280";

        QVBoxLayout *box = new QVBoxLayout();
        box->setSpacing(3);

        QHBoxLayout *top = new QHBoxLayout();
        top->addWidget(new QLabel(label));
        top->addStretch();

        QLabel *valueLabel = new QLabel(QString::number(val) + suffix);
        valueLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(muted));
        valueLabel->setMinimumWidth(44);
        valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        top->addWidget(valueLabel);
        box->addLayout(top);

        QSlider *slider = new QSlider(Qt::Horizontal);
        slider->setRange(min, max);
        slider->setValue(val);
        box->addWidget(slider);
        layout->addLayout(box);

        connect(slider, &QSlider::valueChanged, valueLabel, [valueLabel, suffix](int vv) {
            valueLabel->setText(QString::number(vv) + suffix);
        });

        *out = slider;
    }
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
    QColor previewColor;
    QColor previewSecondColor;

    QString c_bg, c_panel, c_input, c_text, c_textMuted, c_border, c_borderStrong;
    QString c_accent, c_accentHover, c_hover, c_groove;

    QRadioButton *radio1 = nullptr;
    QRadioButton *radio2 = nullptr;
    StrokePreview *preview = nullptr;
    ShapePreview *shapePreview = nullptr;
    QGridLayout *shapeGrid = nullptr;
    QList<ShapeButton*> shapeButtons;
    QButtonGroup *shapeGroup = nullptr;
    QHBoxLayout *sizeRow = nullptr;
    QLabel *sizeLabel = nullptr;
    QSpinBox *sizeSpin = nullptr;
    QSlider *opacitySlider = nullptr;
    QSlider *scatterSlider = nullptr;
    QSlider *angleSlider = nullptr;
    QSlider *densitySlider = nullptr;
    QSlider *flowSlider = nullptr;
    QSlider *aspectSlider = nullptr;
    QLabel *opacityLabel = nullptr;
    QLabel *scatterLabel = nullptr;
    QLabel *angleLabel = nullptr;
    QLabel *densityLabel = nullptr;
    QLabel *aspectLabel = nullptr;
    QLabel *flowLabel = nullptr;
    QComboBox *dragCombo = nullptr;
    QComboBox *rotationCombo = nullptr;
    QCheckBox *airbrushCheck = nullptr;
    QCheckBox *wetCheck = nullptr;
    QCheckBox *granulationCheck = nullptr;
    QCheckBox *mixColorCheck = nullptr;
    QSlider *wetSlider = nullptr;
    QLabel *wetLabel = nullptr;
    QPushButton *btnJitter = nullptr;
    QPushButton *btnRemoveElement = nullptr;
    QPushButton *btnClearComposite = nullptr;
    CompositeEditor *compositeEditor = nullptr;
    QSlider *elemScaleSlider = nullptr;
    QSlider *elemRotationSlider = nullptr;
    QSlider *elemOpacitySlider = nullptr;
    QLabel *elemScaleLabel = nullptr;
    QLabel *elemRotationLabel = nullptr;
    QLabel *elemOpacityLabel = nullptr;
    QLabel *compositeInfo = nullptr;
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
        const QString comboTxt = m_dark ? "#e0e0e0" : "#1e293b";
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

    QString labelStyle() const {
        return QString("color: %1; font-size: 11px; background: transparent;").arg(c_text);
    }

    QString titleStyle() const {
        return QString("color: %1; font-size: 10px; font-weight: 700; letter-spacing: 1.5px; background: transparent;").arg(c_textMuted);
    }

    QString sliderStyle() const {
        return QString(
            "QSlider::groove:horizontal { height: 4px; background: %1; border-radius: 2px; }"
            "QSlider::sub-page:horizontal { background: %2; border-radius: 2px; }"
            "QSlider::handle:horizontal { background: %2; width: 14px; height: 14px; margin: -5px 0; border-radius: 7px; }")
            .arg(c_groove, c_accent);
    }

    QString comboStyle() const {
        const QString txt = m_dark ? "#e0e0e0" : "#1e293b";
        return QString(
            "QComboBox { background-color: %1; color: %2; border: 1px solid %3; border-radius: 5px; padding: 4px 8px; font-size: 12px; min-height: 22px; }")
            .arg(c_input, txt, c_border);
    }

    QString smallBtnStyle() const {
        const QString txt = m_dark ? "#e0e0e0" : "#1e293b";
        return QString(
            "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: 5px; padding: 5px 10px; font-size: 11px; }"
            "QPushButton:hover { background-color: %4; border: 1px solid %5; }"
            "QPushButton:pressed { background-color: %4; }"
            "QPushButton:disabled { color: %6; border: 1px solid %3; }")
            .arg(c_input, txt, c_border, c_hover, c_accent, c_textMuted);
    }

    QString accentBtnStyle() const {
        return QString(
            "QPushButton { background-color: %1; color: white; border: none; border-radius: 5px; padding: 6px 14px; font-size: 13px; font-weight: 600; }"
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

    QLabel *makeLabel(const QString &text) {
        QLabel *l = new QLabel(text);
        l->setStyleSheet(labelStyle());
        return l;
    }

    QPushButton *makeSmallButton(const QString &text) {
        QPushButton *b = new QPushButton(text);
        b->setStyleSheet(smallBtnStyle());
        b->setCursor(Qt::PointingHandCursor);
        return b;
    }

    QPushButton *makeAccentButton(const QString &text) {
        QPushButton *b = new QPushButton(text);
        b->setStyleSheet(accentBtnStyle());
        b->setCursor(Qt::PointingHandCursor);
        b->setFixedHeight(30);
        return b;
    }

    void buildTopRow(QVBoxLayout *mainLayout) {
        QHBoxLayout *topRow = new QHBoxLayout();
        radio1 = new QRadioButton(tr("Pincel 1"));
        radio2 = new QRadioButton(tr("Pincel 2"));
        radio1->setChecked(activeIndex == 0);
        radio2->setChecked(activeIndex == 1);
        topRow->addWidget(radio1);
        topRow->addWidget(radio2);
        topRow->addStretch();
        mainLayout->addLayout(topRow);
    }

    void buildPreviewRow(QVBoxLayout *mainLayout) {
        QHBoxLayout *previewRow = new QHBoxLayout();
        previewRow->setSpacing(8);
        shapePreview = new ShapePreview();
        shapePreview->setDarkMode(m_dark);
        previewRow->addWidget(shapePreview);

        preview = new StrokePreview();
        preview->setDarkMode(m_dark);
        previewRow->addWidget(preview, 1);
        mainLayout->addLayout(previewRow);
    }

    void buildShapeColumn(QVBoxLayout *shapeCol) {
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
    }

    void buildCompositeColumn(QVBoxLayout *compCol) {
        compCol->setSpacing(5);
        compCol->addWidget(makeSectionTitle(tr("COMPOSICION DE FIGURAS")));

        compositeInfo = new QLabel(tr("Figura simple"));
        compositeInfo->setStyleSheet(
            QString("color: %1; font-size: 11px; font-weight: 600; background: transparent;")
                .arg(c_accent));
        compCol->addWidget(compositeInfo);

        compositeEditor = new CompositeEditor();
        compositeEditor->setDarkMode(m_dark);
        compCol->addWidget(compositeEditor, 0, Qt::AlignHCenter);

        QHBoxLayout *compBtnRow = new QHBoxLayout();
        compBtnRow->setSpacing(6);
        btnJitter = makeSmallButton(tr("Variacion..."));
        btnRemoveElement = makeSmallButton(tr("Quitar"));
        btnClearComposite = makeSmallButton(tr("Limpiar"));
        compBtnRow->addWidget(btnJitter);
        compBtnRow->addWidget(btnRemoveElement);
        compBtnRow->addWidget(btnClearComposite);
        compBtnRow->addStretch();
        compCol->addLayout(compBtnRow);
        compCol->addSpacing(2);

        compCol->addWidget(makeSectionTitle(tr("ELEMENTO SELECCIONADO")));
        elemScaleLabel = makeLabel(tr("Escala: -"));
        compCol->addWidget(elemScaleLabel);
        elemScaleSlider = makeSlider(10, 250, 100);
        compCol->addWidget(elemScaleSlider);
        elemRotationLabel = makeLabel(tr("Rotacion: -"));
        compCol->addWidget(elemRotationLabel);
        elemRotationSlider = makeSlider(0, 360, 0);
        compCol->addWidget(elemRotationSlider);
        elemOpacityLabel = makeLabel(tr("Opacidad: -"));
        compCol->addWidget(elemOpacityLabel);
        elemOpacitySlider = makeSlider(0, 100, 100);
        compCol->addWidget(elemOpacitySlider);
        compCol->addStretch();
    }

    void buildControlsColumn(QVBoxLayout *ctrlCol) {
        ctrlCol->setSpacing(4);
        ctrlCol->addWidget(makeSectionTitle(tr("TRAZO")));

        sizeRow = new QHBoxLayout();
        sizeRow->setSpacing(8);
        sizeLabel = new QLabel(tr("Tamano:"));
        sizeLabel->setStyleSheet(
            QString("color: %1; font-size: 12px; font-weight: 600; background: transparent;")
                .arg(c_text));
        sizeSpin = new QSpinBox();
        sizeSpin->setRange(1, 200);
        sizeSpin->setFixedWidth(90);
        sizeRow->addWidget(sizeLabel);
        sizeRow->addWidget(sizeSpin);
        sizeRow->addStretch();
        ctrlCol->addLayout(sizeRow);

        addSliderControl(ctrlCol, tr("Opacidad: 100%"), 1, 100, 100, &opacitySlider, &opacityLabel);
        addSliderControl(ctrlCol, tr("Dispersion: 0"), 0, 50, 0, &scatterSlider, &scatterLabel);
        addSliderControl(ctrlCol, tr("Angulo: 0") + QString::fromUtf8("\xC2\xB0"),
                         0, 360, 0, &angleSlider, &angleLabel);
        addSliderControl(ctrlCol, tr("Proporcion: 100%"), 10, 300, 100, &aspectSlider, &aspectLabel);
        addSliderControl(ctrlCol, tr("Figuras por paso: 1"), 1, 20, 1, &densitySlider, &densityLabel);
        addSliderControl(ctrlCol, tr("Flujo: 100%"), 1, 100, 100, &flowSlider, &flowLabel);

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

        wetLabel = makeLabel(tr("Cantidad de mezcla: 50%"));
        ctrlCol->addWidget(wetLabel);
        wetSlider = makeSlider(0, 100, 50);
        ctrlCol->addWidget(wetSlider);
        ctrlCol->addStretch();
    }

    void addSliderControl(QVBoxLayout *parent, const QString &labelText,
                          int min, int max, int val,
                          QSlider **slider, QLabel **label) {
        *label = makeLabel(labelText);
        parent->addWidget(*label);
        *slider = makeSlider(min, max, val);
        parent->addWidget(*slider);
    }

    void buildButtonRow(QVBoxLayout *mainLayout) {
        QHBoxLayout *btnRow = new QHBoxLayout();
        btnRow->setSpacing(8);
        btnRow->addStretch();

        QPushButton *btnCancel = makeSmallButton(tr("Cancelar"));
        btnCancel->setFixedHeight(30);
        QPushButton *btnOk = makeAccentButton(tr("Aceptar"));

        btnRow->addWidget(btnCancel);
        btnRow->addWidget(btnOk);
        mainLayout->addLayout(btnRow);

        connect(btnOk, &QPushButton::clicked, this, [this]() {
            saveControlsToPreset();
            accept();
        });
        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    }

    void buildShapeGrid() {
        const QList<ShapeType> shapes = PaintEngine::allShapes();
        const int cols = 7;
        int row = 0, col = 0;

        for (int i = 0; i < shapes.size(); ++i) {
            ShapeButton *btn = new ShapeButton(shapes[i]);
            btn->setDarkMode(m_dark);
            shapeButtons.append(btn);
            shapeGroup->addButton(btn, i);
            shapeGrid->addWidget(btn, row, col);
            if (++col >= cols) { col = 0; row++; }
        }

        buildImportButton(shapes.size(), row, col, cols);
    }

    void buildImportButton(int buttonId, int row, int col, int cols) {
        importButton = new ShapeButton(ShapeType::CustomStamp);
        importButton->setDarkMode(m_dark);
        importButton->setImportButton(true);
        importButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        importButton->setMinimumWidth(80);
        importButton->setToolTip(tr("Click: importar PNG\nClic derecho: quitar PNG actual"));

        shapeGroup->addButton(importButton, buttonId);
        shapeButtons.append(importButton);
        shapeGrid->addWidget(importButton, row, col, 1, cols - col);

        connect(importButton, &QPushButton::clicked, this, [this]() {
            importPngAsStamp();
        });

        connect(importButton, &QPushButton::customContextMenuRequested, this,
                [this](const QPoint &pos) {
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

    void importPngAsStamp() {
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
        importButton->setToolTip(tr("PNG: %1x%2\nClick: reemplazar\nClic derecho: quitar")
            .arg(img.width()).arg(img.height()));
        importButton->setChecked(true);
        saveControlsToPreset();
    }

    void loadControlsFromPreset() {
        const BrushSettings &s = presets[activeIndex];

        setControlValue(sizeSpin, s.size);
        setControlValue(opacitySlider, s.opacity);
        setControlValue(scatterSlider, s.scatter);
        setControlValue(angleSlider, (int)s.angle);
        setControlValue(densitySlider, s.density);
        setControlValue(flowSlider, s.flow);
        setControlValue(aspectSlider, (int)(s.aspectRatio * 100));
        setControlValue(dragCombo, (int)s.dragMode);
        setControlValue(rotationCombo, (int)s.rotationMode);
        setControlValue(airbrushCheck, s.isAirbrush);
        setControlValue(wetCheck, s.wetMix);
        setControlValue(granulationCheck, s.granulation);
        setControlValue(mixColorCheck, s.mixSecondColor);
        setControlValue(wetSlider, s.wetAmount);

        wetSlider->setVisible(s.wetMix);
        wetLabel->setVisible(s.wetMix);

        if (s.shape == ShapeType::CustomStamp && !s.customStampImage.isNull()) {
            customStampImage = s.customStampImage;
            importButton->setCustomImage(customStampImage);
            importButton->setToolTip(tr("PNG: %1x%2\nClick: reemplazar\nClic derecho: quitar")
                .arg(customStampImage.width()).arg(customStampImage.height()));
        }

        updateShapeButtonsSelection(s.shape);

        compositeEditor->blockSignals(true);
        compositeEditor->setElements(s.shapeElements);
        compositeEditor->blockSignals(false);

        updateAllLabels();
        updateCompositeInfo();
        loadElementControls();
        refreshPreviews();
    }

    void setControlValue(QSlider *slider, int value) {
        slider->blockSignals(true);
        slider->setValue(value);
        slider->blockSignals(false);
    }

    void setControlValue(QSpinBox *spin, int value) {
        spin->blockSignals(true);
        spin->setValue(value);
        spin->blockSignals(false);
    }

    void setControlValue(QComboBox *combo, int value) {
        combo->blockSignals(true);
        combo->setCurrentIndex(value);
        combo->blockSignals(false);
    }

    void setControlValue(QCheckBox *check, bool value) {
        check->blockSignals(true);
        check->setChecked(value);
        check->blockSignals(false);
    }

    void updateShapeButtonsSelection(ShapeType selected) {
        const QList<ShapeType> shapes = PaintEngine::allShapes();
        for (int i = 0; i < shapeButtons.size(); ++i) {
            shapeButtons[i]->blockSignals(true);
            bool isThis = false;
            if (i < shapes.size()) {
                isThis = (shapes[i] == selected);
            } else if (i == shapes.size()) {
                isThis = (selected == ShapeType::CustomStamp);
            }
            shapeButtons[i]->setChecked(isThis);
            shapeButtons[i]->blockSignals(false);
        }
    }

    void updateAllLabels() {
        const BrushSettings &s = presets[activeIndex];
        opacityLabel->setText(tr("Opacidad: %1%").arg(s.opacity));
        scatterLabel->setText(tr("Dispersion: %1").arg(s.scatter));
        angleLabel->setText(tr("Angulo: %1%2").arg((int)s.angle).arg(QString::fromUtf8("\xC2\xB0")));
        densityLabel->setText(tr("Figuras por paso: %1").arg(s.density));
        flowLabel->setText(tr("Flujo: %1%").arg(s.flow));
        aspectLabel->setText(tr("Proporcion: %1%").arg((int)(s.aspectRatio * 100)));
        wetLabel->setText(tr("Cantidad de mezcla: %1%").arg(s.wetAmount));
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

        const int checkedId = shapeGroup->checkedId();
        const QList<ShapeType> shapes = PaintEngine::allShapes();
        if (checkedId >= 0 && checkedId < shapes.size()) {
            s.shape = shapes[checkedId];
            s.customStampImage = QImage();
        } else if (checkedId == shapes.size()) {
            s.shape = ShapeType::CustomStamp;
            s.customStampImage = customStampImage;
        }

        updateAllLabels();
        updateCompositeInfo();
        schedulePreview();
    }

    void refreshPreviews() {
        const BrushSettings &s = presets[activeIndex];
        preview->updatePreview(s, previewColor, previewSecondColor);
        const QColor secondForStamp = s.mixSecondColor ? previewSecondColor : QColor();
        const QImage stamp = PaintEngine::generateBrushStamp(s, previewColor, 255, false, secondForStamp);
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
        const int count = compositeEditor->getElementCount();
        compositeInfo->setText(count > 0
            ? tr("Compuesto: %1 figuras").arg(count)
            : tr("Figura simple"));
    }

    void loadElementControls() {
        const int idx = compositeEditor->getSelectedIndex();
        const QVector<ShapeElement> elems = compositeEditor->getElements();
        const bool valid = (idx >= 0 && idx < elems.size());

        elemScaleSlider->setEnabled(valid);
        elemRotationSlider->setEnabled(valid);
        elemOpacitySlider->setEnabled(valid);
        btnRemoveElement->setEnabled(valid);

        if (valid) {
            const ShapeElement &el = elems[idx];
            setControlValue(elemScaleSlider, (int)(el.scale * 100));
            setControlValue(elemRotationSlider, (int)el.rotation);
            setControlValue(elemOpacitySlider, el.opacity);
            elemScaleLabel->setText(tr("Escala: %1%").arg((int)(el.scale * 100)));
            elemRotationLabel->setText(tr("Rotacion: %1%2").arg((int)el.rotation).arg(QString::fromUtf8("\xC2\xB0")));
            elemOpacityLabel->setText(tr("Opacidad: %1%").arg(el.opacity));
        } else {
            elemScaleLabel->setText(tr("Escala: -"));
            elemRotationLabel->setText(tr("Rotacion: -"));
            elemOpacityLabel->setText(tr("Opacidad: -"));
        }
    }

    void connectSignals() {
        connect(radio1, &QRadioButton::toggled, this, [this](bool checked) {
            if (checked) { saveControlsToPreset(); activeIndex = 0; loadControlsFromPreset(); }
        });
        connect(radio2, &QRadioButton::toggled, this, [this](bool checked) {
            if (checked) { saveControlsToPreset(); activeIndex = 1; loadControlsFromPreset(); }
        });

        connect(sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, [this]() { saveControlsToPreset(); });

        const QList<QSlider*> allSliders = {
            opacitySlider, scatterSlider, angleSlider, densitySlider,
            flowSlider, aspectSlider, wetSlider
        };
        for (QSlider *s : allSliders) {
            connect(s, &QSlider::valueChanged, this, [this]() { saveControlsToPreset(); });
        }

        connect(dragCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this]() { saveControlsToPreset(); });
        connect(rotationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this]() { saveControlsToPreset(); });

        const QList<QCheckBox*> allChecks = {
            airbrushCheck, granulationCheck, mixColorCheck
        };
        for (QCheckBox *c : allChecks) {
            connect(c, &QCheckBox::toggled, this, [this]() { saveControlsToPreset(); });
        }

        connect(wetCheck, &QCheckBox::toggled, this, [this](bool checked) {
            wetSlider->setVisible(checked);
            wetLabel->setVisible(checked);
            saveControlsToPreset();
        });

        connect(shapeGroup, QOverload<int>::of(&QButtonGroup::idClicked),
                this, [this](int) { saveControlsToPreset(); });

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

        connect(compositeEditor, &CompositeEditor::elementsChanged,
                this, [this]() { saveControlsToPreset(); });
        connect(compositeEditor, &CompositeEditor::selectionChanged,
                this, [this](int) { loadElementControls(); });
        connect(btnRemoveElement, &QPushButton::clicked,
                this, [this]() { compositeEditor->removeSelected(); });
        connect(btnClearComposite, &QPushButton::clicked,
                this, [this]() { compositeEditor->clearAll(); });

        connect(elemScaleSlider, &QSlider::valueChanged, this, [this](int v) {
            updateElementScale(v);
        });
        connect(elemRotationSlider, &QSlider::valueChanged, this, [this](int v) {
            updateElementRotation(v);
        });
        connect(elemOpacitySlider, &QSlider::valueChanged, this, [this](int v) {
            updateElementOpacity(v);
        });
    }

    void updateElementScale(int v) {
        const int idx = compositeEditor->getSelectedIndex();
        QVector<ShapeElement> elems = compositeEditor->getElements();
        if (idx < 0 || idx >= elems.size()) return;
        ShapeElement el = elems[idx];
        el.scale = v / 100.0;
        elemScaleLabel->setText(tr("Escala: %1%").arg(v));
        compositeEditor->updateSelectedElement(el);
    }

    void updateElementRotation(int v) {
        const int idx = compositeEditor->getSelectedIndex();
        QVector<ShapeElement> elems = compositeEditor->getElements();
        if (idx < 0 || idx >= elems.size()) return;
        ShapeElement el = elems[idx];
        el.rotation = v;
        elemRotationLabel->setText(tr("Rotacion: %1%2").arg(v).arg(QString::fromUtf8("\xC2\xB0")));
        compositeEditor->updateSelectedElement(el);
    }

    void updateElementOpacity(int v) {
        const int idx = compositeEditor->getSelectedIndex();
        QVector<ShapeElement> elems = compositeEditor->getElements();
        if (idx < 0 || idx >= elems.size()) return;
        ShapeElement el = elems[idx];
        el.opacity = v;
        elemOpacityLabel->setText(tr("Opacidad: %1%").arg(v));
        compositeEditor->updateSelectedElement(el);
    }

public:
    CustomBrushesDialog(bool darkMode, BrushSettings p1, BrushSettings p2, int active,
                        QWidget *parent = nullptr)
        : QDialog(parent), activeIndex(active),
          previewColor(Qt::black), previewSecondColor(Qt::white) {
        presets[0] = p1;
        presets[1] = p2;
        setupTheme(darkMode);
        setWindowTitle(tr("Configurar Pinceles Personalizados"));
        setMinimumSize(920, 720);
        applyStyleSheet();

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(12, 12, 12, 12);
        mainLayout->setSpacing(8);

        buildTopRow(mainLayout);
        buildPreviewRow(mainLayout);

        QHBoxLayout *contentRow = new QHBoxLayout();
        contentRow->setSpacing(14);

        QVBoxLayout *shapeCol = new QVBoxLayout();
        buildShapeColumn(shapeCol);
        contentRow->addLayout(shapeCol);

        QVBoxLayout *compCol = new QVBoxLayout();
        buildCompositeColumn(compCol);
        contentRow->addLayout(compCol);

        QVBoxLayout *ctrlCol = new QVBoxLayout();
        buildControlsColumn(ctrlCol);
        contentRow->addLayout(ctrlCol);

        mainLayout->addLayout(contentRow, 1);
        buildButtonRow(mainLayout);

        connectSignals();
        loadControlsFromPreset();
    }

    BrushSettings getPreset1() const { return presets[0]; }
    BrushSettings getPreset2() const { return presets[1]; }
    int getActivePresetIndex() const { return activeIndex; }

    void setPreviewColor(const QColor &c) { previewColor = c; refreshPreviews(); }
    void setPreviewSecondColor(const QColor &c) { previewSecondColor = c; refreshPreviews(); }
};

#endif // CUSTOMBRUSHES_H
