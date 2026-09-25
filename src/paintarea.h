#ifndef PAINTAREA_H
#define PAINTAREA_H

#include <QWidget>
#include <QMainWindow>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QPainterPath>
#include <QImage>
#include <QPoint>
#include <QMenu>
#include <QAction>
#include <QColorDialog>
#include <QMessageBox>
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
#include <QRandomGenerator>
#include <QButtonGroup>
#include <QScreen>
#include <QSlider>
#include <QTimer>
#include <QCheckBox>
#include <QSpinBox>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QTextEdit>
#include <QFontDatabase>
#include <QTextLayout>
#include <QTextLine>
#include <QStandardItemModel>
#include <QAbstractItemView>
#include <QScrollArea>
#include <QScrollBar>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <vector>
#include <cmath>
#include <cstring>
#include <QTransform>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QConicalGradient>
#include <QSvgGenerator>
#include <QPaintEvent>
#include <QStack>
#include <algorithm>
#include <QDialog>
#include <QDialogButtonBox>
#include <QRadioButton>
#include <QDrag>
#include <QSet>
#include <QColor>
#include <QMap>
#include <QClipboard>
#include <QApplication>

#include "tools/PixelArtTools.h"
#include "tools/PixelAnimationTools.h"
#include "tools/LassoTools.h"
#include "filter/ImageFilters.h"
#include "tools/RetouchTools.h"
#include "core/LayerStack.h"
#include "core/CustomBrushes.h"
#include "core/ShapeObjects.h"
#include "tools/TextEngine.h"
#include "core/MaskEditController.h"
#include "tools/GradientTools.h"
#include "tools/MagicWandTools.h"
#include "tools/DeformTools.h"
#include "tools/SelectTools.h"
#include "tools/BezierPathTool.h"


const ToolType ToolShadowBurn = static_cast<ToolType>(101);
const ToolType ToolDeform     = static_cast<ToolType>(102);

class mainwind;

inline bool esHerramientaFigura(ToolType t) {
    switch (t) {
        case ToolLine: case ToolRectangle: case ToolEllipse: case ToolRoundRect:
        case ToolTriangle: case ToolRightTriangle: case ToolDiamond:
        case ToolPentagon: case ToolHexagon:
        case ToolArrowRight: case ToolArrowLeft:
        case ToolStar: case ToolHeart: case ToolCube:
            return true;
        default:
            return false;
    }
}

int leerOrientacionExif(const QString &filePath);
QImage aplicaOrientacionExif(const QImage &img, int orientation);
QImage cargarImagenRespetandoExif(const QString &filePath);

// ============================================================
// FrameThumbnail — miniatura de frame de animación
// ============================================================
class FrameThumbnail : public QFrame {
    Q_OBJECT
private:
    QImage frameImage;
    bool isSelected;
    int frameIndex;
    static const int THUMB_SIZE = 48;
public:
    FrameThumbnail(const QImage &img, int index, QWidget *parent = nullptr);
    void setSelected(bool selected);
    void setFrameImage(const QImage &img);
    int getFrameIndex() const;
protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
signals:
    void clicked(int frameIndex);
};


class PaintArea : public QWidget {
    Q_OBJECT

private:
    ///capas y mascaras
    LayerStack stack;

    void recomponerImagen() { stack.recompose(); }
    void asegurarImagenCompleta() { stack.ensureComposited(); }

    QPoint startPoint, lastPoint, hoverPos;
    bool drawing = false;
    Qt::MouseButton activeMouseButton = Qt::NoButton;

    bool resizingCanvas = false;
    int resizeMode = 0;
    const int HANDLE_SIZE = 8;
    QPoint previewCanvasSize;

    // Colores y pincel
    QColor penColor1 = Qt::black, penColor2 = Qt::white;
    int penWidth = 3, penOpacity = 255;
    ToolType currentTool = ToolPencil;
    double zoomFactor = 0.50;

    //  Pinceles personalizados
    BrushSettings customBrushPresets[2];
    int activeCustomBrushIndex = 0;
    QPointF lastCustomPoint;
    QImage customBrushStamp, customBrushStampRight;

    BrushSettings classicToolPreset;
    QImage classicToolStamp, classicToolStampRight;
    QPointF lastClassicPoint;

    QColor strokeCanvasFallback;
    double strokeTotalLength = 0.0;
    double strokeAccumulatedLength = 0.0;
    bool strokeInProgress = false;

    // Timer de dibujo continuo
    QTimer *continuousDrawTimer;
    QPoint currentMousePos;
    QPoint previousMousePos;

    // =============================================
    // SELECCIÓN Y OBJETOS → SelectionManager
    // =============================================
    SelectionManager selMgr;
    MaskEditController m_maskEdit;

    // --- Modo vector ---
    bool vectorEditMode = false;
    QVector<QPointF> vectorPoints;
    int selectedVectorPoint = -1;
    bool draggingVectorPoint = false;
    QPointF vectorHoverPos;

    // Tema
    bool darkModeActive = false;

    //  Pixel Art
    PixelArtOptions pixelOptions;
    PixelAnimationManager animManager;

    // PLUMA BEZIER →
    BezierPathTool bezierTool;

    //  Sensibilidad
    double mouseSensitivity = 1.0;
    int activeColorTarget = 1;

    TextEngine::EditSession textEdit;

    // Historial de pasos hacia atras
    QList<QImage> undoStack, redoStack;
    const int MAX_HISTORY = 30; //numero de pasos

    // gradiente
    enum GradientType { GradientLinear = 0, GradientRadial = 1, GradientConic = 2 };
    GradientType gradientType = GradientLinear;
    QPoint gradientStart, gradientEnd;
    bool drawingGradient = false;
    int gradientOpacity = 255;
    int gradientAngle = 0;
    bool gradientReverse = false;
    bool gradientDither = false;
    int gradientBlendMode = 0;
    bool gradientUseSecondColor = false;

    // Clonar
    bool cloneSourceSet = false;
    QPoint cloneSource;
    QPoint cloneInitialDest;
    QImage cloneBuffer;
    bool cloneIsStamping = false;

    // deformacion
    DeformController m_deform;

    bool movingLayer = false;
    QPoint moveStartPos;
    QImage moveLayerBackup;
    QPoint moveLayerOffset;
    int moveLayerIdx = -1;

    QFont lastUsedTextFont; //fuentes

    friend class mainwind;

    // ============================================================
    // Helpers internos (colapsan duplicaciones)
    // ============================================================
    QColor selBlue() const;
    QColor selBlueLight() const;
    bool capaValida(int idx = -1) const {
        return (idx >= 0) ? stack.validIndex(idx) : stack.currentValid();
    }
    bool puedeEditarCapaActual() const {
        return capaValida() && !stack.currentLocked();
    }
    bool herramientaVectorizable() const;
    QColor colorVectorActivo() const;
    bool herramientaDePintura() const;

    // --- Predicados y estado del pincel activo ---
    bool usaStampDePincel() const {
        return ArtisticPresets::isArtisticTool(currentTool) || currentTool == ToolCustomBrush;
    }
    const BrushSettings& activePreset() const;
    const QImage& activeStamp() const;

    // --- Notificación estándar (recompose + emit + update) ---
    void refreshAndNotify(bool recompose = true);

    // --- Horneado de pendientes (selección, bezier, texto, objetos, vector) ---
    void bakeAllPending();

    // --- Preparar edición: baker + guardar historial ---
    void beginEdit();

    void configureMaskEditController();

    void renderTiles(QPainter &painter, const QRect &visibleWidgetRect);
    void sincronizarCapasConFrameActual();
    void guardarFrameActualEnAnimador();
    void procesarDibujoContinuo();
    QColor obtenerColorDeTrabajo(Qt::MouseButton button);
    void applyClonStamp(QImage &target, const QPoint &destPos);
    void aplicarGradienteConfigurado(const QPoint &p1, const QPoint &p2);
    void openGradientSettings();

    int margenHerramienta() const;
    QRect rectCanvasAWidget(const QRect &r) const;
    void repintarZonaCanvas(const QRect &canvasRect);
    QRect rectSiluetaWidget(const QPoint &widgetPos) const;
    void invalidarTrazo(const QPoint &a, const QPoint &b, const QRect &extraCanvas = QRect());
    void invalidarPreviewClone(const QPoint &cursorPos);

    // Métodos que usan SelectionManager
    void convertSelectionToObject();
    void integrateSelectedObjects();
    void loadTextObjectForEditing(int idx);
    void loadShapeObjectForEditing(int idx);
    void bakeObjectIntoLayer(int idx);
    void bakeAllObjects();

    //  Handles de lienzo
    QRect getRightHandle() const;
    QRect getBottomHandle() const;
    QRect getBottomRightHandle() const;

    //  Vector
    int findVectorPointAt(const QPointF &canvasPos) const;
    bool isNearFirstPoint(const QPointF &canvasPos) const;
    void finalizeVectorPath();
    void cancelVectorMode();

    void updateClassicToolStamp();
    void updateCustomBrushStamp();

    TextEngine::Style currentTextStyle() const {
        return darkModeActive ? TextEngine::Style::forDarkMode()
                              : TextEngine::Style::forLightMode();
    }

public:
    explicit PaintArea(QWidget *parent = nullptr);

    void setActiveColorTarget(int target);
    void setMouseSensitivity(double sens);
    double getMouseSensitivity() const;
    void setDeformOptc(const DeformOptc &o) { m_deform.setOptions(o); }
    DeformOptc getDeformOptc() const { return m_deform.options(); }
    void bakeActivePath();
    void setCustomBrushPresets(BrushSettings p1, BrushSettings p2, int activeIndex);
    BrushSettings getCustomBrush(int index) const;
    int getActiveCustomBrushIndex() const;

    // Máscaras B/N
    bool hasLayerMask(int layerIndex) const;
    bool isLayerMaskEnabled(int layerIndex) const;
    int getMaskEditLayer() const;
    bool isEditingMask() const;
    QImage getMaskPreview(int layerIndex) const;
    void addLayerMask(int layerIndex);
    void removeLayerMask(int layerIndex);
    void toggleLayerMaskEnabled(int layerIndex);
    void selectMaskForEditing(int layerIndex);
    void selectLayerContentForEditing();
    void applyMaskToLayer(int layerIndex);
    void invertLayerMask(int layerIndex);

    // Máscaras de COLOR
    bool hasLayerColorMask(int layerIndex) const;
    bool isLayerColorMaskEnabled(int layerIndex) const;
    FilterParams getLayerColorMaskParams(int layerIndex) const;
    QImage getColorMaskPreview(int layerIndex) const;
    void addLayerColorMask(int layerIndex, const FilterParams &fp);
    void removeLayerColorMask(int layerIndex);
    void toggleLayerColorMaskEnabled(int layerIndex);
    void setLiveColorMaskPreview(int layerIndex, const FilterParams &fp);
    void clearLiveColorMaskPreview(int layerIndex);

    // Capas
    void addLayer();
    void addImageLayer(const QImage &img);
    void insertImageAsObject(const QImage &img);
    void duplicateLayer();
    void deleteLayer();
    void mergeDown();
    void moveLayerUp();
    void moveLayerDown();
    void reorderLayer(int fromIndex, int toIndex);
    void setCurrentLayer(int index);
    void setLayerVisibility(int index, bool visible);
    void setLayerOpacity(int index, double opacity);
    void setLayerBlendMode(int index, int mode);
    void setLayerLocked(int index, bool locked);
    const QList<Layer>& getLayers() const;
    int getCurrentLayerIndex() const;
    QImage getImage();
    void applyImageFilters(const QImage &filteredImage);
    void flipCurrentLayer(bool horizontal, bool vertical);
    void rotateCurrentLayer(int angle);

    // Selección
    void magicWandSelect(const QPoint &pos, int tolerance = 32);

    // Pixel Art
    void setPixelArtMode(bool active, int resolution = 32);
    void setGridSize(int size);
    void setGridActive(bool active);
    bool isGridActive() const;
    void setPixelArtResolution(int resolution);

    // Animación
    void addFrame();
    void duplicateFrame();
    void deleteFrame();
    void goToFrame(int index);
    void nextFrame();
    void prevFrame();

    // Historial
    void clearHistory();
    void saveHistoryState();
    void undo();
    void redo();

    // Colores
    void setPenColor1(const QColor &c);
    void setPenColor2(const QColor &c);
    void refreshBrushStamps();
    QColor getPenColor1() const;
    QColor getPenColor2() const;

    // Pincel
    void setPenWidth(int newWidth);
    void setPenOpacity(int opacity);

    // Zoom
    double getZoomFactor() const;
    void setZoomFactor(double factor);
    void actualizarDimensionesFisicas();
    QSize canvasSize() const;

    // Tema
    void setDarkMode(bool enabled);
    bool getDarkMode() const;

    // Gradiente
    void setGradientType(int t);
    int getGradientType() const;
    int getGradientOpacity() const;
    void setGradientOpacity(int o);
    int getGradientAngle() const;
    void setGradientAngle(int a);
    bool getGradientReverse() const;
    void setGradientReverse(bool r);
    bool getGradientDither() const;
    void setGradientDither(bool d);
    int getGradientBlendMode() const;
    void setGradientBlendMode(int m);
    bool getGradientUseSecondColor() const;
    void setGradientUseSecondColor(bool v);

    // Clonar
    bool isCloneSourceSet() const;
    void resetCloneSource();

    // Herramienta
    void setTool(ToolType tool);

    // Imagen
    void clearImage();
    void crearNuevoLienzo(int w, int h, bool transparent);
    bool abrirImagen(const QString &fileName);
    bool guardarImagen(const QString &fileName, const char *fileFormat);
    bool guardarComoSvg(const QString &fileName);
    bool guardarComoGif(const QString &fileName, int delayMs = 100, int scale = 1);

    // Selección
    void bakeSelection();
    void bakeTextFrame();
    void cancelTextFrame();
    void updateTextFrame(const QRect &rect, const QFont &font, const QColor &color);
    bool isTextFrameActive() const;
    QRect getTextFrameRect() const;
    QString getTextFrameContent() const;
    QFont getTextFrameFont() const;
    QColor getTextFrameColor() const;
    void insertTextChar(const QString &ch);
    void deleteTextChar();

    void copiarSeleccion(); // Portapapeles
    void cortarSeleccion();
    void pegarClipboard();
    void borrarSeleccion();

    // Drag & Drop
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    // Lienzo
    void cambiarDimensionesLienzo(int nuevoW, int nuevoH);

    // Frames
    const QList<QImage>& getFrames() const;
    int getCurrentFrameIndex() const;
    bool getIsPixelArtMode() const;
    int getPixelGridSize() const;
    int getPixelResolution() const;
    bool getVectorEditMode() const;
    void editTextObject(int idx);

signals:
    void colorPicked(int target, const QColor &color);
    void zoomChanged(double factor);
    void zoomRequested(double newFactor, QPoint viewportPos);
    void resolutionChanged(int width, int height);
    void framesChanged(const QList<QImage> &frames, int currentIndex);
    void layersChanged();
    void statusBarMessage(const QString &msg);
    void textFrameClicked(const QPoint &canvasPos);
    void textFrameCancelled();
    void vectorModeChanged(bool active);
    void editTextObjectRequested(int objectIndex, const QString &text, const QFont &font, const QColor &color, const QRect &bounds);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
};

#endif