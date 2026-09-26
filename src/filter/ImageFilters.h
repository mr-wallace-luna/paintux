#ifndef IMAGE_FILTERS_H
#define IMAGE_FILTERS_H

#include <QDialog>
#include <QSlider>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QImage>
#include <QIcon>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QPixmap>
#include <QTabWidget>
#include <QComboBox>
#include <QVector>
#include <QKeyEvent>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QPointF>
#include <QColor>
#include <QSize>
#include <QHash>
#include <QPair>
#include <QGuiApplication>
#include <QStyleHints>
#include <QPalette>
#include <algorithm>
#include <functional>
#include <cmath>

// ============================================================
// Ajuste HSL por canal
// ============================================================
struct HSLAdjust {
    double hue = 0;
    double saturation = 0;
    double lightness = 0;
};

struct FilterParams {
    double brightness = 0, contrast = 0, saturation = 0, exposure = 0;
    double shadows = 0, highlights = 0, temperature = 0, vignette = 0;
    QColor shadowColor = Qt::black, highlightColor = Qt::white;
    bool invertColors = false, grayscale = false, sepia = false;
    bool blur = false; int blurRadius = 5;
    bool sharpen = false; int sharpenStrength = 50;
    bool pixelate = false; int pixelateSize = 8;
    bool halftone = false; int halftoneCell = 6;
    double rawTemp = 0, rawTint = 0, rawVibrance = 0, rawClarity = 0;
    double rawBlacks = 0, rawWhites = 0;
    int rawGamma = 100;
    bool colorize = false;
    double colorizeStrength = 1.0;
    HSLAdjust hsl[7];
    bool colorizeHSL = false;
    QVector<QPointF> curves[4];

    FilterParams() {
        for (int i = 0; i < 4; ++i)
            curves[i] = QVector<QPointF>{ QPointF(0, 0), QPointF(255, 255) };
    }

    static FilterParams identity() { return FilterParams(); }

    bool isIdentity() const {
        if (brightness != 0 || contrast != 0 || saturation != 0 || exposure != 0) return false;
        if (shadows != 0 || highlights != 0 || temperature != 0 || vignette != 0) return false;
        if (invertColors || grayscale || sepia) return false;
        if (blur || sharpen) return false;
        if (pixelate) return false;
        if (halftone) return false;
        if (colorize) return false;
        if (colorizeHSL) return false;
        for (int i = 0; i < 7; ++i)
            if (hsl[i].hue != 0 || hsl[i].saturation != 0 || hsl[i].lightness != 0) return false;
        if (rawTemp != 0 || rawTint != 0 || rawVibrance != 0 || rawClarity != 0) return false;
        if (rawBlacks != 0 || rawWhites != 0 || rawGamma != 100) return false;
        for (int i = 0; i < 4; ++i) {
            if (curves[i].size() != 2) return false;
            if (qAbs(curves[i][0].x()) > 0.01 || qAbs(curves[i][0].y()) > 0.01) return false;
            if (qAbs(curves[i][1].x() - 255.0) > 0.01 || qAbs(curves[i][1].y() - 255.0) > 0.01) return false;
        }
        return true;
    }
};

enum FilterPresetId {
    PresetNone = 0,
    PresetMMADRO,
    PresetMordor,
    PresetY2K,
    PresetChernobil,
    PresetPop,
    PresetPixelArt,
    PresetVaporwave,
    PresetNoir,
    PresetKodak80,
    PresetArctic,
    PresetPastel,
    PresetNeon,
    PresetNoMexico
};

enum AjusteIcono {
    IconoMontania = 0,
    IconoSol,
    IconoMitad,
    IconoCirculo,
    IconoMitadLineas,
    IconoMitadSolido,
    IconoAro,
    IconoGota,
    IconoTermometro
};

// ============================================================
// CurvEditor
// ============================================================
class CurvEditor : public QWidget {
    Q_OBJECT
public:
    explicit CurvEditor(QWidget *parent = nullptr);
    QVector<QPointF> getPoints(int channel) const;
    void setPoints(int channel, const QVector<QPointF> &pts);
    void setAllPoints(const QVector<QPointF> *pts);
    void setChannel(int channel);
    void setHistogram(const QVector<int> &hist);
    void resetChannel(int channel);
    void resetAll();
    void setDarkMode(bool dark);
    bool isDarkMode() const;
    static bool isIdentity(const QVector<QPointF> &pts);
    static void buildLUT(const QVector<QPointF> &pts, uchar lut[256]);
signals:
    void curveChanged();
protected:
    static QVector<QPointF> defaultPoints();
    QColor channelColor() const;
    double mapX(double v) const;
    double mapY(double v) const;
    double invX(double px) const;
    double invY(double py) const;
    int findPointAt(const QPointF &widgetPos) const;
    void notifyChanged();
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
private:
    static const int MARGIN = 10;
    int m_channel = 0;
    int m_selected = -1;
    bool m_dragging = false;
    bool m_dark = true;
    QVector<QPointF> m_points[4];
    QVector<int> m_histogram;

    // --- [REFACTOR] Spline monotono para buildLUT ---
    struct CurveSpline {
        QVector<double> xs, ys, m;
    };
    static CurveSpline computeMonotoneSpline(const QVector<QPointF> &pts);
    static void evaluateSplineLUT(const CurveSpline &sp, uchar lut[256]);

    // --- [REFACTOR] Helpers de paintEvent ---
    void paintBackgroundAndGrid(QPainter &p);
    void paintHistogram(QPainter &p);
    void paintGhostCurves(QPainter &p);
    void paintMainCurve(QPainter &p);
    void paintPoints(QPainter &p);
};

// ============================================================
// HueRangeBar
// ============================================================
class HueRangeBar : public QWidget {
    Q_OBJECT
public:
    explicit HueRangeBar(QWidget *parent = nullptr);
    void setChannel(int ch);
    int channel() const;
protected:
    void paintEvent(QPaintEvent *) override;
private:
    int m_ch = 0;
};

// ============================================================
// ImageFiltersDialog
// ============================================================
class ImageFiltersDialog : public QDialog {
    Q_OBJECT
private:
    QSlider *sliderBrightness, *sliderContrast, *sliderSaturation, *sliderExposure;
    QSlider *sliderShadows, *sliderHighlights, *sliderTemperature, *sliderVignette;
    QPushButton *btnShadowColor, *btnHighlightColor;
    QCheckBox *chkInvertColors, *chkGrayscale, *chkSepia, *chkBlur, *chkSharpen;
    QSlider *sliderBlurRadius, *sliderSharpenStrength;
    QLabel *lblBlurRadius = nullptr, *lblSharpenStrength = nullptr;
    QCheckBox *chkPixelate = nullptr;
    QSlider   *sliderPixelateSize = nullptr;
    QLabel    *lblPixelateSize = nullptr;
    QCheckBox *chkHalftone = nullptr;
    QSlider   *sliderHalftoneCell = nullptr;
    QLabel    *lblHalftoneCell = nullptr;
    QCheckBox *chkColorize = nullptr;
    QSlider   *sliderColorizeStrength = nullptr;
    QLabel    *lblColorizeStrength = nullptr;
    CurvEditor *curvEditor = nullptr;
    QComboBox *comboChannel;
    QSlider *sliderRawTint = nullptr, *sliderRawVibrance = nullptr;
    QComboBox *comboHslChannel = nullptr;
    QSlider   *sliderHslHue = nullptr, *sliderHslSat = nullptr, *sliderHslLum = nullptr;
    QCheckBox *chkColorizeHSL = nullptr;
    HueRangeBar *rangeBar = nullptr;
    HSLAdjust m_hsl[7];
    int  m_hslChannel = 0;
    bool colorizeHSL = false;
    QLabel *lblPreview = nullptr;
    QTabWidget *tabs;
    QComboBox   *comboPresets        = nullptr;
    QSlider     *sliderPresetStrength= nullptr;
    int  m_currentPresetId = PresetNone;
    bool m_applyingPreset  = false;
    QHash<QSlider*, QLabel*> m_sliderLabels;
    QImage originalImage;
    QImage workImage;
    QImage previewResult;

    double brightness = 0, contrast = 0, saturation = 0, exposure = 0;
    double shadows = 0, highlights = 0, temperature = 0, vignette = 0;
    QColor shadowColor = Qt::black, highlightColor = Qt::white;
    bool invertColors = false, grayscale = false, sepia = false;
    bool blur = false; int blurRadius = 5;
    bool sharpen = false; int sharpenStrength = 50;
    bool pixelate = false; int pixelateSize = 8;
    bool halftone = false; int halftoneCell = 6;
    double rawTemp = 0, rawTint = 0, rawVibrance = 0, rawClarity = 0;
    double rawBlacks = 0, rawWhites = 0;
    int rawGamma = 100;
    bool colorize = false;
    double colorizeStrength = 1.0;

    bool    m_dark = true;
    QString c_bg, c_panel, c_input, c_preview;
    QString c_text, c_textMuted, c_textDesc;
    QString c_border, c_borderStrong;
    QString c_accent, c_accentHover, c_hover, c_groove;

    void    setupTheme(bool dark);
    void    applyGlobalStyleSheet();
    QString labelStyle() const;
    QString descStyle() const;
    QString titleStyle() const;
    QString sliderStyle() const;
    QString comboStyle() const;
    QString frameStyle() const;
    QString smallBtnStyle() const;
    QString accentBtnStyle() const;
    QString groupBoxStyle() const;
    QString groupBoxAccentStyle() const;
    void    styleColorButton(QPushButton *btn, const QColor &col);
    QLabel* makeDescLabel(const QString &text) const;
    static bool detectDarkTheme();
    void    loadHslSliders();

public:
    ImageFiltersDialog(const QImage &img, QWidget *parent = nullptr,
                       bool floatingMode = false, int darkMode = -1);
    QGroupBox *makePresetGroup();
    void connectPresetControls();
    void markCustom();
    void applyPresetById(int id);
    QList<QWidget*> allParamWidgets() const;
    void setSliderValue(QSlider *s, int v);
    void setParams(const FilterParams &fp);
    static FilterParams presetParams(int id);
    static FilterParams blendPreset(const FilterParams &b, double k);
    void resetFilters();
    FilterParams getParams() const;
    QImage getFilteredImage();
    static void buildColorizeLUT(int lutR[256], int lutG[256], int lutB[256]);
    static void applyFilterParams(QImage &img, const FilterParams &fp,
                                  const QPoint &subOffset = QPoint(),
                                  const QSize &fullSize = QSize());

signals:
    void paramsChanged();

private:
    QGroupBox *makeSliderGroup(const QString &title, const QString &desc,
                               int min, int max, int initial,
                               QSlider **outSlider, std::function<void(int)> onChanged);
    QWidget *makeCompactRow(const QIcon &ic, const QString &label,
                            int min, int max, int initial,
                            QSlider **outSlider, std::function<void(int)> onChanged,
                            const QString &tooltip = QString(),
                            QWidget *rightWidget = nullptr);
    QFrame  *makeSeparator();
    QLabel  *makeSectionTitle(const QString &t);
    static QIcon iconoAjuste(int tipo, bool dark);
    void applyFilters();
    void applyPipeline(QImage &img);
    static void pixelateImage(QImage &img, int blockSize, const QPoint &subOffset = QPoint());
    static void halftoneImage(QImage &img, int cell, const QPoint &subOffset = QPoint());
    static void boxBlur(QImage &img, int radius);

    // ============================================================
    // [REFACTOR] Helpers extraídos de applyFilterParams
    // ============================================================
    struct ColorAdjustFactors {
        double expFactor = 1.0;
        double contrastFactor = 1.0;
        double gammaExp = 1.0;
    };
    static ColorAdjustFactors computeColorFactors(const FilterParams &fp);
    static bool hasColorAdjustments(const FilterParams &fp);
    static bool hasHslAdjustments(const FilterParams &fp);
    static bool hasCurveAdjustments(const FilterParams &fp);
    static QRgb applyColorPixel(double r, double g, double b, int a,
                                const FilterParams &fp,
                                const ColorAdjustFactors &f);
    static void applyColorPass(QImage &img, const FilterParams &fp,
                               const ColorAdjustFactors &f);
    static void applyColorizePass(QImage &img, const FilterParams &fp);
    static void applyHslPass(QImage &img, const FilterParams &fp);
    static void applyCurvesPass(QImage &img, const FilterParams &fp);
    static void applySharpenPass(QImage &img, const FilterParams &fp);
    static void applyVignettePass(QImage &img, const FilterParams &fp,
                                  const QPoint &subOffset, const QSize &fullSize);
};

#endif // IMAGE_FILTERS_H
