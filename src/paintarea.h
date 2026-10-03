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
#include <memory>
#include <QTransform>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QConicalGradient>
#include <QGradient>
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

#include "core/ToolManager.h"
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

class mainwind;

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
public:
    explicit PaintArea(QWidget *parent = nullptr);

    LayerStack& getLayerStack() { return stack; }
    const LayerStack& getLayerStack() const { return stack; }
    SelectionManager& getSelectionManager() { return selMgr; }
    const SelectionManager& getSelectionManager() const { return selMgr; }
    PixelAnimationManager& getAnimationManager() { return animManager; }
    const PixelAnimationManager& getAnimationManager() const { return animManager; }

    void setActiveColorTarget(int target);
    void setMouseSensitivity(double sens);
    double getMouseSensitivity() const;

    void setDeformOptc(const DeformOptc &o) { m_deform.setOptions(o); }
    DeformOptc getDeformOptc() const { return m_deform.options(); }

    void bakeActivePath();

    void setCustomBrushPresets(BrushSettings p1, BrushSettings p2, int activeIndex);
    BrushSettings getCustomBrush(int index) const;
    int getActiveCustomBrushIndex() const;

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

    bool hasLayerColorMask(int layerIndex) const;
    bool isLayerColorMaskEnabled(int layerIndex) const;
    FilterParams getLayerColorMaskParams(int layerIndex) const;
    QImage getColorMaskPreview(int layerIndex) const;
    void addLayerColorMask(int layerIndex, const FilterParams &fp);
    void removeLayerColorMask(int layerIndex);
    void toggleLayerColorMaskEnabled(int layerIndex);
    void setLiveColorMaskPreview(int layerIndex, const FilterParams &fp);
    void clearLiveColorMaskPreview(int layerIndex);

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

    void magicWandSelect(const QPoint &pos, int tolerance = 32);

    void setPixelArtMode(bool active, int resolution = 32);
    void setGridSize(int size);
    void setGridActive(bool active);
    bool isGridActive() const;
    void setPixelArtResolution(int resolution);

    void addFrame();
    void duplicateFrame();
    void deleteFrame();
    void goToFrame(int index);
    void nextFrame();
    void prevFrame();

    void clearHistory();
    void saveHistoryState();
    void undo();
    void redo();

    void setPenColor1(const QColor &c);
    void setPenColor2(const QColor &c);
    void refreshBrushStamps();
    QColor getPenColor1() const;
    QColor getPenColor2() const;
    void setPenWidth(int newWidth);
    void setPenOpacity(int opacity);

    double getZoomFactor() const;
    void setZoomFactor(double factor);
    void actualizarDimensionesFisicas();
    QSize canvasSize() const;

    void setDarkMode(bool enabled);
    bool getDarkMode() const;

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

    bool isCloneSourceSet() const;
    void resetCloneSource();

    void setTool(ToolType tool);

    void clearImage();
    void crearNuevoLienzo(int w, int h, bool transparent);
    bool abrirImagen(const QString &fileName);
    bool guardarImagen(const QString &fileName, const char *fileFormat);
    bool guardarComoSvg(const QString &fileName);
    bool guardarComoGif(const QString &fileName, int delayMs = 100, int scale = 1);

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

    void copiarSeleccion();
    void cortarSeleccion();
    void pegarClipboard();
    void borrarSeleccion();

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    void cambiarDimensionesLienzo(int nuevoW, int nuevoH);

    const QList<QImage>& getFrames() const;
    int getCurrentFrameIndex() const;
    bool getIsPixelArtMode() const;
    int getPixelGridSize() const;
    int getPixelResolution() const;
    bool getVectorEditMode() const;
    void editTextObject(int idx);

    void setSelectFreeSubMode(SelectionManager::FreeSubMode mode);
    SelectionManager::FreeSubMode getSelectFreeSubMode() const;
    void cancelSelectFreeVector();
    void cancelSelectFreeElement();

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
    void selectFreeSubModeChanged(int mode);
    void selectionFinalizedByElement();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    LayerStack stack;
    void recomponerImagen() { stack.recompose(); }

    QPoint startPoint, lastPoint, hoverPos;
    bool drawing = false;
    Qt::MouseButton activeMouseButton = Qt::NoButton;
    bool resizingCanvas = false;
    int resizeMode = 0;
    const int HANDLE_SIZE = 8;
    QPoint previewCanvasSize;

    QColor penColor1 = Qt::black, penColor2 = Qt::white;
    int penWidth = 3, penOpacity = 255;
    ToolType currentTool = ToolType::Pencil;
    double zoomFactor = 0.50;

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
    QTimer *continuousDrawTimer;
    QPoint currentMousePos;
    QPoint previousMousePos;

    SelectionManager selMgr;
    MaskEditController m_maskEdit;

    QVector<QPointF> vectorPoints;
    int selectedVectorPoint = -1;
    bool draggingVectorPoint = false;
    QPointF vectorHoverPos;

    bool darkModeActive = false;

    PixelArtOptions pixelOptions;
    PixelAnimationManager animManager;
    BezierPathTool bezierTool;

    double mouseSensitivity = 1.0;
    int activeColorTarget = 1;

    TextEngine::EditSession textEdit;

    QList<QImage> undoStack, redoStack;
    const int MAX_HISTORY = 30;

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

    bool cloneSourceSet = false;
    QPoint cloneSource;
    QPoint cloneInitialDest;
    QImage cloneBuffer;
    bool cloneIsStamping = false;

    DeformController m_deform;

    bool movingLayer = false;
    QPoint moveStartPos;
    QImage moveLayerBackup;
    QPoint moveLayerOffset;
    int moveLayerIdx = -1;

    QFont lastUsedTextFont;

    QPoint elementLastHover;
    static constexpr int ELEMENT_HOVER_MIN_DIST = 6;

    bool preserveSelectionOnToolSwitch = false;

    friend class mainwind;

    // ========================================================
    // Helpers básicos
    // ========================================================
    QColor selBlue() const;
    QColor selBlueLight() const;

    bool capaValida(int idx = -1) const {
        return (idx >= 0) ? stack.validIndex(idx) : stack.currentValid();
    }
    bool puedeEditarCapaActual() const {
        return capaValida() && !stack.currentLocked();
    }
    bool herramientaDePintura() const    { return ToolManager::isPainting(currentTool); }
    bool usaStampDePincel() const        { return ToolManager::usesBrushStamp(currentTool); }

    QColor colorVectorActivo() const;
    const BrushSettings& activePreset() const;
    const QImage& activeStamp() const;

    void refreshAndNotify(bool recompose = true);
    void bakeAllPending();
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

    void convertSelectionToObject();
    void integrateSelectedObjects();
    void loadTextObjectForEditing(int idx);
    void loadShapeObjectForEditing(int idx);
    void bakeObjectIntoLayer(int idx);
    void bakeAllObjects();

    QRect getRightHandle() const;
    QRect getBottomHandle() const;
    QRect getBottomRightHandle() const;

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

    // ========================================================
    // PRESS - dispatchers nivel 1 (estado global)
    // ========================================================
    bool onPressMaskBezier(const QPoint &pos);
    bool onPressMaskPaint(const QPoint &pos);
    bool onPressTextActive(const QPoint &pos);
    bool onPressCanvasHandles(const QPoint &rawPos);
    bool onPressSelectionGizmo(const QPoint &pos);
    bool onPressZoom(QMouseEvent *event, const QPoint &rawPos);

    // PRESS - dispatcher nivel 2 (por herramienta)
    void dispatchPressTool(QMouseEvent *event, const QPoint &pos, int scaledWidth);
    void pressPenBezier(const QPoint &pos);
    void pressSelectRect(const QPoint &pos);
    void pressSelectFree(const QPoint &pos);
    void pressSelectFreeVector(const QPoint &pos);
    void pressSelectFreeElement(const QPoint &pos);
    void pressBucket(const QPoint &pos, const QColor &colorDeUso);
    void pressPicker(const QPoint &pos);
    void pressTextNew(const QPoint &pos, const QColor &colorDeUso);
    void pressGradient(QMouseEvent *event, const QPoint &pos);
    void pressClone(QMouseEvent *event, const QPoint &pos);
    void pressMove(QMouseEvent *event, const QPoint &pos);
    void pressDeform(const QPoint &pos);
    void pressStroke(const QPoint &pos, int scaledWidth);

    // Helpers de pressStroke
    void pressInitStroke(const QPoint &pos);
    void pressResolveColors(QColor &colorDeUso, QColor &colorOpuesto) const;
    void pressBrushStamp(const QPoint &pos, int scaledWidth,
                         const QColor &colorDeUso, const QColor &colorOpuesto);
    void pressPixelArt(const QPoint &pos, const QColor &colorDeUso);
    void pressPencil(const QPoint &pos, int scaledWidth, const QColor &colorDeUso);
    void pressEraser(const QPoint &pos, int scaledWidth);
    void pressRetouch(const QPoint &pos);

    // Helpers de Move
    bool tryGrabActiveObjectGizmo(const QPointF &canvasPos);
    void beginObjectSelectionDrag(int idx, const QPointF &canvasPos);
    void beginLayerMove(const QPoint &pos);

    // ========================================================
    // MOVE - dispatchers
    // ========================================================
    bool onMoveMaskBezier(const QPoint &pos);
    bool onMoveMaskPaint(const QPoint &pos);
    bool onMoveSelectFreeVector(const QPoint &pos);
    bool onMoveSelectFreeElement(const QPoint &pos);
    bool onMoveTextActive(const QPoint &pos);
    bool onMoveCanvasResize(const QPoint &rawPos);
    bool onMoveSelectionRotate(QMouseEvent *event, const QPoint &pos);
    bool onMoveSelectionResize(QMouseEvent *event, const QPoint &pos);
    bool onMoveSelectionDrag(const QPoint &pos);
    bool onMoveGradientPreview(const QPoint &pos);
    bool onMoveClone(const QPoint &pos, Qt::MouseButtons buttons);
    bool onMoveDeform(const QPoint &pos, Qt::MouseButtons buttons);
    bool onMoveDrawing(const QPoint &pos, int scaledWidth);
    bool onMoveTextHover(const QPoint &pos);

    void dispatchMoveTool(QMouseEvent *event, const QPoint &pos);
    bool moveObjectManipulation(QMouseEvent *event, const QPoint &pos);
    bool moveMovingLayer(const QPoint &pos);
    bool moveToolHover(const QPoint &pos);
    bool moveSelectionHover(const QPoint &pos);

    void moveCursorUpdate(const QPoint &pos);
    void applyToolCursor();
    void updateClonePreviewOnHover(const QPoint &pos);
    void updateSilhouetteOnHover();
    void updateBezierPreviewOnHover(const QPoint &pos);

    // Helpers de drawing
    void dispatchDrawingByTool(const QPoint &pos, int scaledWidth,
                               const QColor &colorDeUso, const QColor &colorOpuesto);
    bool isPixelArtModeActive() const;
    bool isPixelArtStrokeTool() const;
    void drawingBrushStroke(const QPoint &pos,
                            const QColor &colorDeUso,
                            const QColor &colorOpuesto);
    void drawingPixelArt(const QPoint &pos, const QColor &colorDeUso);
    void drawingSelectionRect(const QPoint &pos);
    void drawingSelectionFree(const QPoint &pos);
    void drawingPencil(const QPoint &pos, const QColor &colorDeUso, int scaledWidth);
    void drawingEraser(const QPoint &pos, int scaledWidth);
    void drawingRetouch(const QPoint &pos);

    void applyBrushStroke(const QPoint &pos, const QColor &colorDeUso, const QColor &colorOpuesto);
    void applyPixelArtStroke(const QPoint &pos, const QColor &colorDeUso);
    void applyPencilStroke(const QPoint &pos, const QColor &colorDeUso, int scaledWidth);
    void applyEraserStroke(const QPoint &pos, int scaledWidth);
    void applyRetouchStroke(const QPoint &pos);
    void updateSelectionPreview(const QRect &selPrevia, const QPoint &puntoPrevio, const QPoint &pos);

    // ========================================================
    // RELEASE - dispatchers
    // ========================================================
    bool onReleaseMaskPaint();
    bool onReleaseSelectFreeVector();
    bool onReleaseTextDragResize();
    bool onReleaseCanvasResize();
    bool onReleaseSelectionGizmo();
    bool onReleaseGradient();
    bool onReleaseClone();
    bool onReleaseDeform();
    bool onReleaseMove();

    void dispatchReleaseTool(const QPoint &finalPoint, int scaledWidth);
    void releaseFinishLasso();
    void releaseFinishSelection(const QPoint &finalPoint);
    void releaseFinishShape(const QPoint &finalPoint, int scaledWidth);

    // ========================================================
    // KEY - dispatchers
    // ========================================================
    bool onKeySelectFree(QKeyEvent *event);
    bool onKeyMaskBezier(QKeyEvent *event);
    bool onKeyClipboard(QKeyEvent *event);
    bool onKeyDelete(QKeyEvent *event);
    bool onKeyObjectShortcuts(QKeyEvent *event);
    bool onKeyTextEditing(QKeyEvent *event);
    bool onKeyBezierPen(QKeyEvent *event);
    bool onKeyToolSpecific(QKeyEvent *event);

    // ========================================================
    // RENDER (paintEvent + helpers)
    // ========================================================
    void paintCheckerboard(QPainter &painter, const QRect &canvasRect);
    void paintMaskOverlay(QPainter &painter);
    void paintBezierOverlay(QPainter &painter);
    void paintCanvasResizePreview(QPainter &painter);
    void paintShapePreview(QPainter &painter, int scaledWidth);
    void paintActiveSelection(QPainter &painter);
    void paintSelectionPreview(QPainter &painter);
    void paintTextFrame(QPainter &painter);
    void paintGradientPreview(QPainter &painter);
    void paintCloneOverlay(QPainter &painter);
    void paintSelectionGizmos(QPainter &painter);
    void paintCanvasHandles(QPainter &painter);
    void paintCursorSilhouette(QPainter &painter, int scaledWidth);
    void paintSelectFreeVectorOverlay(QPainter &painter);
    void paintSelectFreeElementOverlay(QPainter &painter);
    void paintMaskBrushSilhouette(QPainter &painter);
    void paintRetouchSilhouette(QPainter &painter, int scaledWidth);
    void paintDeformSilhouette(QPainter &painter);
    void paintCloneSilhouette(QPainter &painter, int scaledWidth);
    void paintBrushStampSilhouette(QPainter &painter);

    std::unique_ptr<QGradient> buildGradientForPreview() const;
    void paintGradientFill(QPainter &painter, QGradient *grad);
    void paintGradientHandleOverlay(QPainter &painter);

    // ========================================================
    // Cambio de herramienta
    // ========================================================
    bool handleGradientToolReentry(ToolType tool);
    void resetStateForToolSwitch(ToolType tool);
    void applyToolPreset(ToolType tool);
};

#endif // PAINTAREA_H