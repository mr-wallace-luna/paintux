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
#include <QVector>
#include <QString>
#include <QHash>
#include <QObject>
#include <QWidget>
#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>

#include "core/ToolManager.h"
#include "tools/RetouchTools.h"
#include "tools/MagicWandTools.h"

class QSlider;
class QSpinBox;
class QCheckBox;
class QComboBox;
class QLabel;
class QGridLayout;
class QHBoxLayout;
class QVBoxLayout;
class QGroupBox;
class QRadioButton;
class QButtonGroup;
class QTimer;
class QDragEnterEvent;
class QDropEvent;
class QMouseEvent;
class QPaintEvent;
class QDrag;
class QMimeData;

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

class ShapeNames {
public:
    static QString name(ShapeType s);
private:
    static QString lookup(ShapeType s);
};

class ShapePathBuilder {
public:
    struct Context {
        QRectF r{-50.0, -50.0, 100.0, 100.0};
        double cx = 0.0;
        double cy = 0.0;
        double w  = 100.0;
        double h  = 100.0;
        Context();
    };

    static void appendRegularPolygon(QPainterPath &path, const Context &ctx,
                                     int sides, double startAngle, double radiusFactor);
    static void appendStarPolygon(QPainterPath &path, const Context &ctx,
                                  int points, double innerFactor, double startAngle);

    static void buildCircle(QPainterPath &p, const Context &c);
    static void buildSquare(QPainterPath &p, const Context &c);
    static void buildRoundedSquare(QPainterPath &p, const Context &c);
    static void buildDiamond(QPainterPath &p, const Context &c);
    static void buildTriangle(QPainterPath &p, const Context &c);
    static void buildRightTriangle(QPainterPath &p, const Context &c);
    static void buildPentagon(QPainterPath &p, const Context &c);
    static void buildHexagon(QPainterPath &p, const Context &c);
    static void buildStar4(QPainterPath &p, const Context &c);
    static void buildStar5(QPainterPath &p, const Context &c);
    static void buildStar6(QPainterPath &p, const Context &c);
    static void buildCross(QPainterPath &p, const Context &c);
    static void buildPlus(QPainterPath &p, const Context &c);
    static void buildX(QPainterPath &p, const Context &c);
    static void buildArrow(QPainterPath &p, const Context &c);
    static void buildHeart(QPainterPath &p, const Context &c);
    static void buildLine(QPainterPath &p, const Context &c);
    static void buildPencilTip(QPainterPath &p, const Context &c);
    static void buildFlatTip(QPainterPath &p, const Context &c);
    static void buildChiselTip(QPainterPath &p, const Context &c);
    static void buildLeaf(QPainterPath &p, const Context &c);
    static void buildDrop(QPainterPath &p, const Context &c);
    static void buildCrescent(QPainterPath &p, const Context &c);
    static void buildRing(QPainterPath &p, const Context &c);
    static void buildHalfCircle(QPainterPath &p, const Context &c);
    static void buildSparkle(QPainterPath &p, const Context &c);
    static void buildClover(QPainterPath &p, const Context &c);
    static void buildGear(QPainterPath &p, const Context &c);
    static void buildLightning(QPainterPath &p, const Context &c);
    static void buildMusicNote(QPainterPath &p, const Context &c);
    static void buildFlower(QPainterPath &p, const Context &c);
    static void buildButterfly(QPainterPath &p, const Context &c);
    static void buildCloud(QPainterPath &p, const Context &c);
    static void buildSpeech(QPainterPath &p, const Context &c);
    static void buildLocationPin(QPainterPath &p, const Context &c);
    static void buildWave(QPainterPath &p, const Context &c);
    static void buildSpiral(QPainterPath &p, const Context &c);
    static void buildStarMany(QPainterPath &p, const Context &c);
    static void buildInfinity(QPainterPath &p, const Context &c);
    static void buildDiamondStar(QPainterPath &p, const Context &c);

    using BuilderFn = void(*)(QPainterPath&, const Context&);
    static BuilderFn builderFor(ShapeType s);
    static QPainterPath build(ShapeType shape);
};

class GeometryDrawer {
public:
    using PointDrawFn = void(*)(QPainter&, const QPoint&, const QPoint&);
    using RectDrawFn  = void(*)(QPainter&, const QRect&);

    static void draw(QPainter &painter, const QPoint &p1, const QPoint &p2, ToolType tool);

private:
    static PointDrawFn pointFnFor(ToolType tool);
    static RectDrawFn  rectFnFor(ToolType tool);
    static void drawLinePts(QPainter &p, const QPoint &a, const QPoint &b);
    static void drawTrianglePts(QPainter &p, const QPoint &a, const QPoint &b);
    static void drawRightTrianglePts(QPainter &p, const QPoint &a, const QPoint &b);
    static void drawDiamondPts(QPainter &p, const QPoint &a, const QPoint &b);
    static void drawRectangleR(QPainter &p, const QRect &r);
    static void drawEllipseR(QPainter &p, const QRect &r);
    static void drawRoundRectR(QPainter &p, const QRect &r);
    static void drawPentagonR(QPainter &p, const QRect &r);
    static void drawHexagonR(QPainter &p, const QRect &r);
    static void drawRegularPolygonR(QPainter &p, const QRect &r, int sides);
    static void drawStarR(QPainter &p, const QRect &r);
    static void drawArrowRightR(QPainter &p, const QRect &r);
    static void drawArrowLeftR(QPainter &p, const QRect &r);
    static void drawArrowR(QPainter &p, const QRect &r, bool right);
    static void drawHeartR(QPainter &p, const QRect &r);
    static void drawCubeR(QPainter &p, const QRect &r);
};

class PaintEngine {
public:
    static QString shapeName(ShapeType s);
    static QList<ShapeType> allShapes();
    static QPainterPath baseShapePath(ShapeType shape);
    static QPainterPath transformedShapePath(ShapeType shape, const QRectF &r);
    static QPainterPath shapePath(ShapeType shape, const QRectF &r);
    static QImage tintImage(const QImage &src, const QColor &color);
    static QRectF aspectRect(double cx, double cy, double size, double aspectRatio);
    static QRectF aspectRectInCanvas(double canvasW, double canvasH,
                                     double size, double aspectRatio);

    static void drawShapePrimitive(QPainter &painter, ShapeType shape, const QRectF &r,
                                   const QColor &color, const QImage &customImage = QImage());
    static void drawShapeOutline(QPainter &painter, ShapeType shape, const QRectF &r,
                                 const QImage &customImage = QImage());
    static void drawBrushSilhouette(QPainter &painter, const BrushSettings &config,
                                    double size, const QPointF &center);
    static void drawCompositeSilhouette(QPainter &painter, const BrushSettings &config, double size);
    static void drawSingleSilhouette(QPainter &painter, const BrushSettings &config, double size);

    static double computeBreathFactor(double accumulatedLength, int brushSize, int flow);
    static void applyGranulation(QPainter &p, int canvasW, int canvasH, double angle);

    static void paintShapeWithColor(QPainter &p, ShapeType shape, const QRectF &r,
                                    const QColor &color, bool doGradient,
                                    const QColor &c1, const QColor &c2);
    static void paintSingleStampShape(QPainter &p, const BrushSettings &config,
                                      const QColor &baseColor, const QColor &secondColor,
                                      double alpha, int canvasW, int canvasH,
                                      int size, int pad);
    static void paintCompositeStamp(QPainter &p, const BrushSettings &config,
                                    const QColor &baseColor, const QColor &secondColor,
                                    double alpha, double size);
    static void paintCompositeElement(QPainter &p, const ShapeElement &el,
                                      const QColor &baseColor, const QColor &secondColor,
                                      double alpha, double spread, bool doGradient);

    static QImage generateBrushStamp(const BrushSettings &config, const QColor &baseColor,
                                     int penOpacity, bool pixelArt,
                                     const QColor &secondColor = QColor());

    static void applyGraphitePencil(QImage &image, const QPoint &p1, const QPoint &p2,
                                    const QColor &color, int width, int opacity);
    static void drawGrainPoint(QPainter &painter, const QImage &image, const QColor &color,
                               int opacity, double softness, double coreRadius,
                               double strokeAngle, double cx, double cy);

    static void applyEraserLine(QImage &image, const QPoint &p1, const QPoint &p2,
                                int width, bool softEdge);
    static void eraseSoft(QPainter &painter, const QPoint &p1, const QPoint &p2, int width);

    static QColor sampleCanvasColor(const QImage &image, const QPoint &pos, int radius);
    static QImage wetMixStamp(const QImage &stamp, const QColor &canvasColor, double wetAmount);

    static void drawStampAt(QImage &image, const QPoint &pos, const QImage &drawStamp,
                            double angle, double opacity, double scale);

    static void applyCustomBrushStroke(QImage &image, const QPoint &pos,
                                       const QImage &stamp, const BrushSettings &config,
                                       double mouseSensitivity, double extraAngle = 0.0,
                                       double sizeScale = 1.0, double opacityScale = 1.0,
                                       const QColor &baseColor = QColor(),
                                       const QColor &secondColor = QColor(),
                                       const QColor &fallbackCanvasColor = QColor(),
                                       double taperFactor = 1.0);

    static QPoint computeScatterOffset(const QPoint &pos, const BrushSettings &config,
                                       double mouseSensitivity);
    static double computeDrawAngle(const BrushSettings &config, double extraAngle);
    static double computeDrawOpacity(const BrushSettings &config, double opacityScale,
                                     double taperFactor);
    static double computeDrawScale(const BrushSettings &config, double sizeScale,
                                   double taperFactor);
    static double computeSpacing(const BrushSettings &config, double mouseSensitivity);

    static void applyRibbonLine(QImage &image, const QPointF &from, const QPointF &to,
                                const BrushSettings &config, const QColor &baseColor,
                                QPointF &lastPoint);

    static void applyCustomBrushLine(QImage &image, const QPointF &from, const QPointF &to,
                                     const QImage &stamp, const BrushSettings &config,
                                     double mouseSensitivity, QPointF &lastPoint,
                                     const QColor &baseColor = QColor(),
                                     const QColor &secondColor = QColor(),
                                     const QColor &fallbackCanvasColor = QColor(),
                                     double totalStrokeLength = 0.0,
                                     double accumulatedLength = 0.0);

    static QImage resolveLineStamp(const QImage &image, const QPointF &from,
                                   const QImage &stamp, const BrushSettings &config,
                                   const QColor &baseColor,
                                   const QColor &fallbackCanvasColor);

    static void paintSegmentsAlongLine(QImage &image, const QPointF &from, const QPointF &to,
                                       double dist, const QImage &lineStamp,
                                       const BrushSettings &config, double dirAngle,
                                       const QColor &secondColor,
                                       const QColor &fallbackCanvasColor,
                                       double accumulatedLength, double baseSpacing);

    static double computeTaperFactor(const BrushSettings &config, double accumulatedLength,
                                     double dist, double t);
    static QPoint computeDensityOffset(const BrushSettings &config, int densityCount);

    static void applyBlur(QImage &image, const QPoint &pos, int radius);
    static void applyHeal(QImage &image, const QPoint &pos, int radius);
    static void applyShadowBurn(QImage &image, const QPoint &pos, int radius,
                                double sensitivity, int opacity);

    static void drawGeometry(QPainter &painter, const QPoint &p1, const QPoint &p2, ToolType tool);
    static void floodFill(QImage &image, const QPoint &start, QColor fillCol);
    static QImage magicWandMask(const QImage &image, const QPoint &pos, int tolerance);
};

namespace ArtisticPresets {

BrushSettings makePreset(int size, ShapeType shape, DragMode drag, RotationMode rot,
                         int opacity, int scatter, double angle, int density, int flow,
                         int sizeJitter, int angleJitter, int opacityJitter,
                         double aspectRatio, bool wet, int wetAmount, bool granulation,
                         bool isAirbrush = false);

BrushSettings watercolor();
BrushSettings oilBrush();
BrushSettings crayon();
BrushSettings marker();
BrushSettings calligraphy();
BrushSettings highlighter();
BrushSettings softBrush();
BrushSettings sprayCan();
BrushSettings presetForTool(ToolType t);

}

class ShapeButton : public QPushButton {
    Q_OBJECT
public:
    ShapeType shape;
    QImage customImage;
    bool isImportButton = false;
    bool isEmptyImport = true;

    ShapeButton(ShapeType s, QWidget *parent = nullptr, const QImage &img = QImage());

    void setImportButton(bool isImport);
    void setCustomImage(const QImage &img);
    void setDarkMode(bool dark);

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void paintEvent(QPaintEvent *) override;

private:
    enum class ButtonKind { ImportEmpty, ImportFilled, Regular };

    struct ButtonLook {
        ButtonKind kind = ButtonKind::Regular;
        QColor bg;
        QColor border;
        QColor fig;
    };

    ButtonLook resolveLook() const;
    ButtonLook lookImportEmpty() const;
    ButtonLook lookImportFilled() const;
    ButtonLook lookRegular() const;

    void paintBackground(QPainter &p, const ButtonLook &look);
    void paintBorder(QPainter &p, const ButtonLook &look);
    void paintImportEmptyIcon(QPainter &p);
    void paintCustomImage(QPainter &p);
    void paintShapeIcon(QPainter &p, const QColor &fig);

    QPoint dragStartPos;
    bool m_dark = true;
};

class ShapePreview : public QWidget {
    Q_OBJECT
public:
    ShapePreview(QWidget *parent = nullptr);
    void updateStamp(const QImage &stamp);
    void setDarkMode(bool dark);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QImage stampImage;
    bool m_dark = true;
};

class CompositeEditor : public QWidget {
    Q_OBJECT
public:
    CompositeEditor(QWidget *parent = nullptr);

    void setElements(const QVector<ShapeElement> &elems);
    QVector<ShapeElement> getElements() const;
    int getSelectedIndex() const;
    void setSelectedIndex(int idx);
    void addElement(ShapeType shape, double ox, double oy, const QImage &customImg = QImage());
    void removeSelected();
    void updateSelectedElement(const ShapeElement &el);
    void clearAll();
    int getElementCount() const;
    void setDarkMode(bool dark);

signals:
    void elementsChanged();
    void selectionChanged(int index);
    void requestCustomStamp(ShapeType shape, const QImage &img);

protected:
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void paintEvent(QPaintEvent *) override;

private:
    void paintCrosshair(QPainter &p);
    void paintElements(QPainter &p);
    void paintSingleElement(QPainter &p, const ShapeElement &el, int idx, double baseSize);
    void paintBorder(QPainter &p);
    void paintEmptyHint(QPainter &p);

    QVector<ShapeElement> elements;
    int selectedIndex = -1;
    bool draggingElement = false;
    bool m_dark = true;
};

class StrokePreview : public QWidget {
    Q_OBJECT
public:
    StrokePreview(QWidget *parent = nullptr);
    void updatePreview(const BrushSettings &s, const QColor &c, const QColor &c2 = QColor());
    void setDarkMode(bool dark);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    void rebuildCache();
    QVector<QPointF> buildPreviewPath() const;
    void paintPreviewStroke(QImage &img, const QImage &stamp,
                            const BrushSettings &previewSettings,
                            const QVector<QPointF> &points);
    QString buildLabel() const;

    BrushSettings settings;
    QColor brushColor;
    QColor secondColor;
    QImage cachedPreview;
    bool dirty = true;
    bool m_dark = true;
};

class JitterDialog : public QDialog {
    Q_OBJECT
public:
    JitterDialog(bool darkMode, int sizeJ, int angleJ, int opacityJ, QWidget *parent = nullptr);

    int getSizeJitter() const;
    int getAngleJitter() const;
    int getOpacityJitter() const;

private:
    void applyTheme(bool darkMode);
    void addSliderRow(QVBoxLayout *layout, const QString &label, int min, int max,
                      int val, QSlider **out, const QString &suffix, bool darkMode);

    QSlider *sizeSlider = nullptr;
    QSlider *angleSlider = nullptr;
    QSlider *opacitySlider = nullptr;
};

class CustomBrushesDialog : public QDialog {
    Q_OBJECT
public:
    CustomBrushesDialog(bool darkMode, BrushSettings p1, BrushSettings p2, int active,
                        QWidget *parent = nullptr);

    BrushSettings getPreset1() const;
    BrushSettings getPreset2() const;
    int getActivePresetIndex() const;
    void setPreviewColor(const QColor &c);
    void setPreviewSecondColor(const QColor &c);

private:
    void setupTheme(bool dark);
    void applyStyleSheet();
    QString labelStyle() const;
    QString titleStyle() const;
    QString sliderStyle() const;
    QString comboStyle() const;
    QString smallBtnStyle() const;
    QString accentBtnStyle() const;

    QLabel *makeSectionTitle(const QString &t);
    QSlider *makeSlider(int min, int max, int val);
    QLabel *makeLabel(const QString &text);
    QPushButton *makeSmallButton(const QString &text);
    QPushButton *makeAccentButton(const QString &text);

    void buildTopRow(QVBoxLayout *mainLayout);
    void buildPreviewRow(QVBoxLayout *mainLayout);
    void buildShapeColumn(QVBoxLayout *shapeCol);
    void buildCompositeColumn(QVBoxLayout *compCol);
    void buildControlsColumn(QVBoxLayout *ctrlCol);
    void addSliderControl(QVBoxLayout *parent, const QString &labelText,
                          int min, int max, int val,
                          QSlider **slider, QLabel **label);
    void buildButtonRow(QVBoxLayout *mainLayout);
    void buildShapeGrid();
    void buildImportButton(int buttonId, int row, int col, int cols);

    void importPngAsStamp();
    void loadControlsFromPreset();
    void setControlValue(QSlider *slider, int value);
    void setControlValue(QSpinBox *spin, int value);
    void setControlValue(QComboBox *combo, int value);
    void setControlValue(QCheckBox *check, bool value);
    void updateShapeButtonsSelection(ShapeType selected);
    void updateAllLabels();
    void saveControlsToPreset();
    void refreshPreviews();
    void schedulePreview();
    void updateCompositeInfo();
    void loadElementControls();
    void connectSignals();

    void updateElementScale(int v);
    void updateElementRotation(int v);
    void updateElementOpacity(int v);

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
};

#endif