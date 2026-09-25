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

#include "paintarea.h"
#include "filter/ImageFilters.h"
#include "core/CustomBrushes.h"



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

bool detectarTemaOscuroSistema();
QIcon crearIconoMascara(bool dark);
QIcon crearIconoDeformacion(bool dark);


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


class MaskThumbButton : public QPushButton {
    Q_OBJECT
public:
    MaskThumbButton(QWidget *parent = nullptr);
signals:
    void maskClicked(bool shiftHeld);
protected:
    void mousePressEvent(QMouseEvent *e) override;
};


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

    DraggableLayerItem(int idx, const QImage &img, const QString &name, bool vis, bool dark, QWidget *parent = nullptr);
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



class mainwind : public QMainWindow {
    Q_OBJECT
private:
    int currentWorkspaceAlpha = 191;
    PaintArea *paintArea;
    QScrollArea *scrollArea;
    QWidget *sysBarWidget, *ribbonWidget, *bottomBarWidget, *leftSidebarWidget, *contentAreaWidget;

    QPushButton *btnArchivoMenu, *btnConfiguraciones, *btnQuickSave, *btnUndo, *btnRedo, *btnRotateFlip,
                *btnThemeToggle, *btnImageFilters, *btnCustomBrushesTop, *btnViewMenu, *btnCanvasSize;
    QMenu *menuArchivoDesplegable, *menuConfiguracionesDesplegable, *menuCanvasSizeStd, *menuCanvasSizePixel;

    QAction *actNuevo = nullptr;
    QAction *actInsertarImagen = nullptr;
    QAction *actInsertarComoCapa = nullptr;
    QAction *actAbrirFondo = nullptr;
    QAction *actExportGif = nullptr;
    QAction *actSalir = nullptr;

    QGroupBox *boxShapes, *boxBrushesContainer, *boxClipboard, *boxAnimation;
    QPushButton *btnPrevFrame, *btnNextFrame, *btnAddFrame, *btnDupFrame, *btnDelFrame;
    QLabel *lblFrameIndicator;
    QScrollArea *framesScrollArea;
    QWidget *framesContainer;
    QHBoxLayout *framesLayout;
    QList<FrameThumbnail*> frameThumbnails;
    QPushButton *btnPlayAnimation;
    QSlider *animSpeedSlider;
    QTimer *animTimer;
    bool isAnimating = false;
    int animSpeed = 200;

    QGroupBox *boxAdvTools, *boxVectorTools, *boxLayersContainer;
    QPushButton *btnMagicWand, *btnBlurTool, *btnHealTool, *btnShadowTool;
    QPushButton *btnGradientTool, *btnCloneTool, *btnZoomToolSidebar;
    QToolButton *btnPenBezier;
    QPushButton *btnLassoTool;
    QMenu *menuLassoModos;
    QAction *actLassoAdentro, *actLassoAfuera;
    int lassoModoActual = 0;
    QPushButton *btnDeformTool;
    DeformSettingsBar *deformSettingsBar = nullptr;

    QScrollArea *layersScrollArea;
    QWidget *layersContainer;
    QVBoxLayout *layersLayout;
    QList<DraggableLayerItem*> draggableLayerItems;
    QPushButton *btnAddLayer, *btnDupLayer, *btnDelLayer, *btnLayerMask, *btnMoveLayerUp, *btnMoveLayerDown;
    QMenu *menuLayerMask;
    QAction *actAddMask, *actDelMask, *actToggleMask, *actInvertMask, *actApplyMask;
    QAction *actAddColorMask, *actDelColorMask, *actToggleColorMask;
    QLabel *lblCurrentLayer;
    QSlider *sliderLayerOpacity;
    QComboBox *comboBlendMode;
    QCheckBox *chkLayerLocked;

    QFrame *frameSelectorsColor;
    QPushButton *btnColor1, *btnColor2;
    QToolButton *btnEditColors;
    int colorObjetivoActivo = 1;

    bool darkMode = false;
    QList<QAbstractButton*> listaBotonesHerramientas;
    QList<QPushButton*> listaBotonesRecientes;
    int indiceRecienteActual = 0;

    QComboBox *comboZoom;
    QPushButton *btnZoomLess, *btnZoomMore;
    QLabel *lblZoomIndicator, *lblResolutionIndicator;
    QString modoActual = "Normal";

    QPushButton *btnPencil, *btnBucket, *btnText, *btnEraser, *btnPicker, *btnMoveTool;
    QToolButton *btnMirrorPen, *btnBrushTool, *btnSprayTool, *btnCustomToolAction, *btnCrayonTool,
                *btnLighten, *btnMarkerTool, *btnPixelStroke, *btnWatercolor, *btnOilBrush, *btnCalligraphy, *btnHighlighter;
    QFrame *separadorBarra;
    QActionGroup *transparencyGroup;

    TextEngine::FormatBar *textFormatBar = nullptr;

    bool actualizandoPanelCapas = false;
    QTimer *layerRefreshTimer = nullptr;
    QFrame *brushesGridFrame = nullptr;
    QFrame *pixelToolsPanel = nullptr;

    QString resolveAssetPath(const QString &filename);
    void abrirEditorMascaraColor(int idx);
    void actualizarBotonLazo();
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
    bool guardarComo(const QString &extension = QString());
    void exportarGif();

public:
    mainwind();
};


#endif