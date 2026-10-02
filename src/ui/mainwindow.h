#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QApplication>
#include <QToolBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QComboBox>
#include <QLabel>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QToolButton>
#include <QScrollArea>
#include <QFrame>
#include <QInputDialog>
#include <QButtonGroup>
#include <QDir>
#include <QActionGroup>
#include <QScreen>
#include <QSlider>
#include <QTimer>
#include <QCheckBox>
#include <QSpinBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTranslator>
#include <QSettings>
#include <QProcess>
#include <QStyleFactory>
#include <QSvgRenderer>
#include <QDebug>
#include <QColorDialog>
#include <QStyleHints>
#include <QMimeData>
#include <QDrag>
#include <QPalette>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QWidgetAction>
#include <QScrollBar>
#include <QCloseEvent>

#include "paintarea.h"
#include "core/ToolManager.h"
#include "filter/ImageFilters.h"
#include "core/CustomBrushes.h"


// ============================================================
// Paleta de colores del tema (dark / light)
// ============================================================
struct ThemeColors {
    bool dark;
    QString bgDialog, bgPanel, bgPreview, bgInput, bgHover, bgSelected;
    QString textPrimary, textSecondary, textMuted, textAccent;
    QString border, borderStrong, borderAccent;
    QString accent, accentHover, accentPressed;
    QString success, warning, danger;

    ThemeColors(bool isDark) : dark(isDark) {
        if (isDark) {
            bgDialog = "#1a1a1a"; bgPanel = "#242424"; bgPreview = "#1e1e1e"; bgInput = "#2a2a2a";
            bgHover = "#333333"; bgSelected = "#1a3a5c";
            textPrimary = "#e5e5e5"; textSecondary = "#b0b0b0"; textMuted = "#808080"; textAccent = "#60a5fa";
            border = "#3a3a3a"; borderStrong = "#555555"; borderAccent = "#3b82f6";
            accent = "#3b82f6"; accentHover = "#2563eb"; accentPressed = "#1d4ed8";
            success = "#10b981"; warning = "#f59e0b"; danger = "#ef4444";
        } else {
            bgDialog = "#fafafa"; bgPanel = "#ffffff"; bgPreview = "#f3f4f6"; bgInput = "#ffffff";
            bgHover = "#f1f5f9"; bgSelected = "#e8f0fe";
            textPrimary = "#111827"; textSecondary = "#4b5563"; textMuted = "#9ca3af"; textAccent = "#1d4ed8";
            border = "#e5e7eb"; borderStrong = "#d1d5db"; borderAccent = "#3b82f6";
            accent = "#3b82f6"; accentHover = "#2563eb"; accentPressed = "#1d4ed8";
            success = "#059669"; warning = "#d97706"; danger = "#dc2626";
        }
    }
};


// ============================================================
// Funciones libres
// ============================================================
bool detectarTemaOscuroSistema();
QIcon crearIconoMascara(bool dark);
QIcon crearIconoDeformacion(bool dark);
QIcon crearIconoSelectFree(int subMode, bool dark);


// ============================================================
// Diálogo de Nuevo Lienzo
// ============================================================
class NewCanvasDialog : public QDialog {
    Q_OBJECT
private:
    QSpinBox *sbWidth, *sbHeight;
    QPushButton *btnWhite, *btnTransparent;
    QLabel *previewLabel, *lblInfo;
    int selectedBg = 0;
    QList<QPushButton*> presetButtons;
    QPushButton *activePresetBtn = nullptr;
    ThemeColors colors;

    void updatePresetStyle(QPushButton *btn, bool active);
    void setActivePreset(QPushButton *btn);
    void applyPreset(int w, int h);
    void updatePreview();

public:
    NewCanvasDialog(bool darkMode, QWidget *parent = nullptr);
    int getWidth() const;
    int getHeight() const;
    bool isTransparent() const;
};


// ============================================================
// Botón de máscara (click + shift+click)
// ============================================================
class MaskThumbButton : public QPushButton {
    Q_OBJECT
public:
    MaskThumbButton(QWidget *parent = nullptr);
signals:
    void maskClicked(bool shiftHeld);
protected:
    void mousePressEvent(QMouseEvent *e) override;
};


// ============================================================
// Item de capa arrastrable
// ============================================================
class DraggableLayerItem : public QFrame {
    Q_OBJECT
public:
    int layerIndex;
    bool isSelected, layerVisible;
    bool isDarkMode;
    QPoint dragStartPosition;
    QPushButton *btnEye;
    QLabel *thumbLabel;
    QFrame *maskThumbFrame;
    MaskThumbButton *btnMaskThumb;
    QFrame *colorMaskThumbFrame;
    MaskThumbButton *btnColorMaskThumb;
    QLabel *nameLabel;

    DraggableLayerItem(int idx, const QImage &img, const QString &name,
                       bool vis, bool dark, QWidget *parent = nullptr);
    void updateMaskState(bool hasMask, bool enabled, bool editing, const QImage &preview);
    void updateColorMaskState(bool hasMask, bool enabled, const QImage &preview);
    void refreshFrom(const QImage &img, const QString &name, bool vis, bool dark);
    void updateDarkMode(bool dark);
    void setSelected(bool selected);

private:
    void updateEyeButton();
    void updateStyle();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;

signals:
    void clicked(int layerIndex);
    void layerMoved(int fromIndex, int toIndex);
    void visibilityToggled(int layerIndex, bool visible);
    void maskClicked(int layerIndex, bool shiftHeld);
    void colorMaskClicked(int layerIndex, bool shiftHeld);
};


// ============================================================
// Ventana principal
// ============================================================
class mainwind : public QMainWindow {
    Q_OBJECT
private:
    // === Configuración global ===
    int currentWorkspaceAlpha = 191;
    PaintArea *paintArea = nullptr;
    QScrollArea *scrollArea = nullptr;

    // === Estado de archivo / proyecto ===
    QString currentProjectPath;
    QString lastImagePath;
    QString lastImageExtension = "png";
    bool hasUnsavedChanges = false;
    bool suppressUnsavedWarning = false;

    // === Widgets de la estructura ===
    QWidget *sysBarWidget = nullptr;
    QWidget *ribbonWidget = nullptr;
    QWidget *bottomBarWidget = nullptr;
    QWidget *leftSidebarWidget = nullptr;
    QWidget *contentAreaWidget = nullptr;

    // === Barra superior ===
    QPushButton *btnArchivoMenu = nullptr;
    QPushButton *btnViewMenu = nullptr;
    QPushButton *btnConfiguraciones = nullptr;
    QPushButton *btnQuickSave = nullptr;
    QPushButton *btnUndo = nullptr;
    QPushButton *btnRedo = nullptr;
    QPushButton *btnRotateFlip = nullptr;
    QPushButton *btnThemeToggle = nullptr;
    QPushButton *btnImageFilters = nullptr;
    QPushButton *btnCustomBrushesTop = nullptr;
    QPushButton *btnCanvasSize = nullptr;

    QMenu *menuArchivoDesplegable = nullptr;
    QMenu *menuConfiguracionesDesplegable = nullptr;
    QMenu *menuCanvasSizeStd = nullptr;
    QMenu *menuCanvasSizePixel = nullptr;

    // === Acciones del menú Archivo ===
    QAction *actNuevo = nullptr;
    QAction *actAbrirProyecto = nullptr;
    QAction *actAbrir = nullptr;
    QAction *actGuardarImagen = nullptr;
    QAction *actGuardarImagenComo = nullptr;
    QAction *actGuardarProyecto = nullptr;
    QAction *actGuardarProyectoComo = nullptr;
    QAction *actInsertarImagen = nullptr;
    QAction *actInsertarComoCapa = nullptr;
    QAction *actAbrirFondo = nullptr;
    QAction *actExportGif = nullptr;
    QAction *actSalir = nullptr;

    // === Ribbon: cajas ===
    QGroupBox *boxShapes = nullptr;
    QGroupBox *boxBrushesContainer = nullptr;
    QGroupBox *boxClipboard = nullptr;
    QGroupBox *boxAnimation = nullptr;

    // === Ribbon: animación ===
    QPushButton *btnPrevFrame = nullptr;
    QPushButton *btnNextFrame = nullptr;
    QPushButton *btnAddFrame = nullptr;
    QPushButton *btnDupFrame = nullptr;
    QPushButton *btnDelFrame = nullptr;
    QLabel *lblFrameIndicator = nullptr;
    QScrollArea *framesScrollArea = nullptr;
    QWidget *framesContainer = nullptr;
    QHBoxLayout *framesLayout = nullptr;
    QList<FrameThumbnail*> frameThumbnails;
    QPushButton *btnPlayAnimation = nullptr;
    QSlider *animSpeedSlider = nullptr;
    QTimer *animTimer = nullptr;
    bool isAnimating = false;
    int animSpeed = 200;

    // === Barra lateral izquierda ===
    QGroupBox *boxAdvTools = nullptr;
    QGroupBox *boxVectorTools = nullptr;
    QGroupBox *boxLayersContainer = nullptr;

    QPushButton *btnMagicWand = nullptr;
    QPushButton *btnBlurTool = nullptr;
    QPushButton *btnHealTool = nullptr;
    QPushButton *btnShadowTool = nullptr;
    QPushButton *btnGradientTool = nullptr;
    QPushButton *btnCloneTool = nullptr;
    QPushButton *btnZoomToolSidebar = nullptr;
    QToolButton *btnPenBezier = nullptr;

    QPushButton *btnLassoTool = nullptr;
    QMenu *menuLassoModos = nullptr;
    QAction *actLassoAdentro = nullptr;
    QAction *actLassoAfuera = nullptr;
    int lassoModoActual = 0;

    // === Selección Libre (3 submodos) ===
    QPushButton *btnSelectBox = nullptr;
    QPushButton *btnSelectFree = nullptr;
    QMenu *menuSelectFreeModos = nullptr;
    QAction *actFreeLasso = nullptr;
    QAction *actFreeVector = nullptr;
    QAction *actFreeElement = nullptr;
    int selectFreeMode = 0;

    QPushButton *btnDeformTool = nullptr;
    DeformSettingsBar *deformSettingsBar = nullptr;

    // === Panel de capas ===
    QScrollArea *layersScrollArea = nullptr;
    QWidget *layersContainer = nullptr;
    QVBoxLayout *layersLayout = nullptr;
    QList<DraggableLayerItem*> draggableLayerItems;
    QPushButton *btnAddLayer = nullptr;
    QPushButton *btnDupLayer = nullptr;
    QPushButton *btnDelLayer = nullptr;
    QPushButton *btnLayerMask = nullptr;
    QPushButton *btnMoveLayerUp = nullptr;
    QPushButton *btnMoveLayerDown = nullptr;
    QMenu *menuLayerMask = nullptr;
    QAction *actAddMask = nullptr;
    QAction *actDelMask = nullptr;
    QAction *actToggleMask = nullptr;
    QAction *actInvertMask = nullptr;
    QAction *actApplyMask = nullptr;
    QAction *actAddColorMask = nullptr;
    QAction *actDelColorMask = nullptr;
    QAction *actToggleColorMask = nullptr;
    QLabel *lblCurrentLayer = nullptr;
    QSlider *sliderLayerOpacity = nullptr;
    QComboBox *comboBlendMode = nullptr;
    QCheckBox *chkLayerLocked = nullptr;

    // === Panel de colores ===
    QFrame *frameSelectorsColor = nullptr;
    QPushButton *btnColor1 = nullptr;
    QPushButton *btnColor2 = nullptr;
    QToolButton *btnEditColors = nullptr;
    int colorObjetivoActivo = 1;

    // === Estado global ===
    bool darkMode = false;
    QList<QAbstractButton*> listaBotonesHerramientas;
    QList<QPushButton*> listaBotonesRecientes;
    int indiceRecienteActual = 0;

    // === Barra inferior ===
    QComboBox *comboZoom = nullptr;
    QPushButton *btnZoomLess = nullptr;
    QPushButton *btnZoomMore = nullptr;
    QLabel *lblZoomIndicator = nullptr;
    QLabel *lblResolutionIndicator = nullptr;
    QString modoActual = "Normal";

    // === Herramientas del ribbon ===
    QPushButton *btnPencil = nullptr;
    QPushButton *btnBucket = nullptr;
    QPushButton *btnText = nullptr;
    QPushButton *btnEraser = nullptr;
    QPushButton *btnPicker = nullptr;
    QPushButton *btnMoveTool = nullptr;

    QToolButton *btnMirrorPen = nullptr;
    QToolButton *btnBrushTool = nullptr;
    QToolButton *btnSprayTool = nullptr;
    QToolButton *btnCustomToolAction = nullptr;
    QToolButton *btnCrayonTool = nullptr;
    QToolButton *btnLighten = nullptr;
    QToolButton *btnMarkerTool = nullptr;
    QToolButton *btnPixelStroke = nullptr;
    QToolButton *btnWatercolor = nullptr;
    QToolButton *btnOilBrush = nullptr;
    QToolButton *btnCalligraphy = nullptr;
    QToolButton *btnHighlighter = nullptr;

    QFrame *separadorBarra = nullptr;
    QActionGroup *transparencyGroup = nullptr;

    TextEngine::FormatBar *textFormatBar = nullptr;

    bool actualizandoPanelCapas = false;
    QTimer *layerRefreshTimer = nullptr;
    QFrame *brushesGridFrame = nullptr;
    QFrame *pixelToolsPanel = nullptr;

    // ============================================================
    // Métodos privados
    // ============================================================
    QString resolveAssetPath(const QString &filename);
    void abrirEditorMascaraColor(int idx);

    void actualizarBotonLazo();
    void actualizarBotonSelectFree();
    void aplicarSelectFreeMode(int mode);

    void handleZoomRequest(double newFactor, QPoint viewportPos);
    void solicitarRefreshCapas();
    void cambiarIdioma(const QString &langCode);
    void actualizarPanelCapas();
    void iniciarAnimacion();
    void detenerAnimacion();
    void actualizarMiniaturasFrames(const QList<QImage> &frames, int currentIndex);
    bool preguntarGuardarCambios();
    bool cambiarModo(const QString &nuevoModo);
    void refrescarDatosZoomUI(double factor);
    void compilarHojasDeEstiloGlobales();
    void inyectarColorAObjeto(const QColor &color);
    void sincronizarGoteroUI(int target, const QColor &color);
    void actualizarEstilosDePrevisualizacion();
    void abrirPaletaAvanzada();
    void subirImagenDisco();
    void exportarGif();

    // === Métodos de archivo / proyecto ===
    void nuevoLienzo();
    void abrirImagen();
    bool abrirProyecto();
    bool guardarImagen();
    bool guardarImagenComo();
    bool guardarProyecto();
    bool guardarProyectoComo();
    void actualizarTitulo();

protected:
    void closeEvent(QCloseEvent *event) override;

public:
    mainwind();
};


#endif // MAINWINDOW_H