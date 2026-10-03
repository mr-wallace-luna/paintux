#ifndef TOOLMANAGER_H
#define TOOLMANAGER_H

#include <QString>
#include <QColor>
#include <QList>
#include <Qt>
#include <QCoreApplication>

enum class ToolType {
    Pencil, Eraser, Bucket, Picker, Spray, Text,
    Select, SelectFree,
    Line, Rectangle, Ellipse, RoundRect, Triangle, RightTriangle,
    Diamond, Pentagon, Hexagon, ArrowRight, ArrowLeft, Star, Heart, Cube,
    Zoom, Brush, CustomBrush, Crayon, Marker,
    Watercolor, OilBrush, Calligraphy, Highlighter,
    MagicWand, LassoExtract, LassoDelete,
    PenBezier, NodeEdit,
    Blur, Heal, MirrorPen, PixelStroke, Lighten,
    Pencil3B, Pencil4B,
    Gradient, Clone, Move, ShadowBurn, Deform
};

namespace ToolManager {

using ToolId = quint32;
constexpr ToolId idOf(ToolType t) { return static_cast<ToolId>(t); }

enum Category : quint32 {
    None         = 0,
    Shape        = 1u << 0,
    Painting     = 1u << 1,
    Vectorizable = 1u << 2,
    BrushStamp   = 1u << 3,
    Artistic     = 1u << 4,
    PixelArt     = 1u << 5,
    Retouch      = 1u << 6,
    Selection    = 1u << 7,
    Transform    = 1u << 8,
};

struct Info {
    ToolType        tool;
    const char     *icon;
    const char     *name;
    quint32         cats;
    Qt::CursorShape cursor;
};

namespace detail {

inline const Info* tools() {
    static const Info arr[] = {
        { ToolType::Pencil,        "lapiz.svg",                 "Lápiz",                Painting|PixelArt,             Qt::CrossCursor },
        { ToolType::Eraser,        "goma.svg",                  "Goma",                 Painting|PixelArt,             Qt::CrossCursor },
        { ToolType::Bucket,        "cubeta-pintura.svg",        "Cubeta",               None,                          Qt::CrossCursor },
        { ToolType::Picker,        "gotero.svg",                "Gotero",               None,                          Qt::PointingHandCursor },
        { ToolType::Spray,         "spray.svg",                 "Spray",                Painting|BrushStamp|Artistic,  Qt::BlankCursor },
        { ToolType::Text,          nullptr,                     "Texto",                None,                          Qt::IBeamCursor },
        { ToolType::Select,        nullptr,                     "Selección",            Selection,                     Qt::CrossCursor },
        { ToolType::SelectFree,    nullptr,                     "Selección libre",      Selection|Vectorizable,        Qt::CrossCursor },
        { ToolType::Line,          "line.svg",                  "Línea",                Shape,                         Qt::CrossCursor },
        { ToolType::Rectangle,     "cuadrado.svg",              "Rectángulo",           Shape,                         Qt::CrossCursor },
        { ToolType::Ellipse,       "circle.svg",                "Elipse",               Shape,                         Qt::CrossCursor },
        { ToolType::RoundRect,     "rectangulo_redondeado.svg", "Rect. redondeado",     Shape,                         Qt::CrossCursor },
        { ToolType::Triangle,      "triangulo.svg",             "Triángulo",            Shape,                         Qt::CrossCursor },
        { ToolType::RightTriangle, "triangulo_rectangulo.svg",  "Triángulo rect.",      Shape,                         Qt::CrossCursor },
        { ToolType::Diamond,       "rombo.svg",                 "Rombo",                Shape,                         Qt::CrossCursor },
        { ToolType::Pentagon,      "pentagono.svg",             "Pentágono",            Shape,                         Qt::CrossCursor },
        { ToolType::Hexagon,       "hexagono.svg",              "Hexágono",             Shape,                         Qt::CrossCursor },
        { ToolType::ArrowRight,    "flecha_derecha.svg",        "Flecha derecha",       Shape,                         Qt::CrossCursor },
        { ToolType::ArrowLeft,     "flecha_izquierda.svg",      "Flecha izquierda",     Shape,                         Qt::CrossCursor },
        { ToolType::Star,          "estrella.svg",              "Estrella",             Shape,                         Qt::CrossCursor },
        { ToolType::Heart,         "corazon.svg",               "Corazón",              Shape,                         Qt::CrossCursor },
        { ToolType::Cube,          "cubo2.svg",                 "Cubo",                 Shape,                         Qt::CrossCursor },
        { ToolType::Zoom,          "lupa.svg",                  "Zoom",                 None,                          Qt::PointingHandCursor },
        { ToolType::Brush,         "pincel.svg",                "Pincel",               Painting|BrushStamp|Artistic,  Qt::BlankCursor },
        { ToolType::CustomBrush,   "custom_brush.svg",          "Pincel personalizado", Painting|BrushStamp,           Qt::BlankCursor },
        { ToolType::Crayon,        "crayon.svg",                "Crayón",               Painting|BrushStamp|Artistic,  Qt::BlankCursor },
        { ToolType::Marker,        "marker.svg",                "Marcador",             Painting|BrushStamp|Artistic,  Qt::BlankCursor },
        { ToolType::Watercolor,    "acuarela.svg",              "Acuarela",             Painting|BrushStamp|Artistic,  Qt::BlankCursor },
        { ToolType::OilBrush,      "oleo.svg",                  "Óleo",                 Painting|BrushStamp|Artistic,  Qt::BlankCursor },
        { ToolType::Calligraphy,   "caligrafia.svg",            "Caligrafía",           Painting|BrushStamp|Artistic,  Qt::BlankCursor },
        { ToolType::Highlighter,   "resaltador.svg",            "Resaltador",           Painting|BrushStamp|Artistic,  Qt::BlankCursor },
        { ToolType::MirrorPen,     "mirror.svg",                "Espejo",               Painting|PixelArt,             Qt::CrossCursor },
        { ToolType::PixelStroke,   "pixel_stroke.svg",          "Trazo Pixel",          Shape|PixelArt,                Qt::CrossCursor },
        { ToolType::Lighten,       "lighten.svg",               "Aclarar",              Painting|PixelArt,             Qt::CrossCursor },
        { ToolType::MagicWand,     "magic_wand.svg",            "Varita mágica",        Selection,                     Qt::CrossCursor },
        { ToolType::LassoExtract,  "lasso_extract.svg",         "Recortar dentro",      Selection|Vectorizable,        Qt::CrossCursor },
        { ToolType::LassoDelete,   "lasso_delete.svg",          "Recortar fuera",       Selection|Vectorizable,        Qt::CrossCursor },
        { ToolType::PenBezier,     "pen_bezier.svg",            "Pluma Bezier",         Vectorizable,                  Qt::CrossCursor },
        { ToolType::Blur,          "blur.svg",                  "Desenfoque",           Retouch,                       Qt::BlankCursor },
        { ToolType::Heal,          "heal.svg",                  "Corrector",            Retouch,                       Qt::BlankCursor },
        { ToolType::ShadowBurn,    "burn.svg",                  "Subexponer",           Retouch,                       Qt::BlankCursor },
        { ToolType::Gradient,      "gradient.svg",              "Gradiente",            None,                          Qt::CrossCursor },
        { ToolType::Clone,         "clone.svg",                 "Clonar",               None,                          Qt::CrossCursor },
        { ToolType::Move,          "move.svg",                  "Mover",                Transform,                     Qt::ArrowCursor },
        { ToolType::Deform,        nullptr,                     "Deformar",             Retouch|Transform,             Qt::BlankCursor },
    };
    return arr;
}

constexpr int kCount = 46;

} // namespace detail

inline const Info& info(ToolType t) {
    const Info *arr = detail::tools();
    for (int i = 0; i < detail::kCount; ++i)
        if (arr[i].tool == t) return arr[i];
    static const Info unknown{ ToolType::Pencil, nullptr, "?", None, Qt::ArrowCursor };
    return unknown;
}

inline QList<ToolType> all() {
    QList<ToolType> out;
    const Info *arr = detail::tools();
    out.reserve(detail::kCount);
    for (int i = 0; i < detail::kCount; ++i) out.append(arr[i].tool);
    return out;
}

inline QList<ToolType> byCategory(Category cat) {
    QList<ToolType> out;
    const Info *arr = detail::tools();
    for (int i = 0; i < detail::kCount; ++i)
        if (arr[i].cats & cat) out.append(arr[i].tool);
    return out;
}

inline bool isShape(ToolType t)        { return info(t).cats & Shape; }
inline bool isPainting(ToolType t)     { return info(t).cats & Painting; }
inline bool isVectorizable(ToolType t) { return info(t).cats & Vectorizable; }
inline bool usesBrushStamp(ToolType t) { return info(t).cats & BrushStamp; }
inline bool isArtistic(ToolType t)     { return info(t).cats & Artistic; }
inline bool isPixelArt(ToolType t)     { return info(t).cats & PixelArt; }
inline bool isRetouch(ToolType t)      { return info(t).cats & Retouch; }
inline bool isSelection(ToolType t)    { return info(t).cats & Selection; }
inline bool isTransform(ToolType t)    { return info(t).cats & Transform; }

inline QColor selectionColorFor(ToolType t) {
    if (t == ToolType::LassoDelete)  return QColor(236, 72, 153);
    if (t == ToolType::LassoExtract) return QColor(59, 130, 246);
    return QColor(16, 185, 129);
}

inline QString displayName(ToolType t) {
    return QCoreApplication::translate("ToolManager", info(t).name);
}

inline int retouchCode(ToolType t) {
    switch (t) {
        case ToolType::Blur:       return 201;
        case ToolType::Heal:       return 202;
        case ToolType::ShadowBurn: return 101;
        default:                   return 0;
    }
}

} // namespace ToolManager

#endif // TOOLMANAGER_H