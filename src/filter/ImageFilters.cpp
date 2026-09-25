#include "ImageFilters.h"

// ============================================================
// Helpers HSL por canal (estilo Photoshop Hue/Saturation)
// ============================================================

static double angDiffHue(double a, double b) {
    double d = fmod(a - b, 360.0);
    if (d > 180.0) d -= 360.0;
    if (d < -180.0) d += 360.0;
    return d;
}

// Peso del canal (1=Rojos..6=Magentas): efecto lleno ±15°, caída suave hasta ±45°
static double hslChannelWeight(double hue, int channel) {
    static const double centers[6] = { 0, 60, 120, 180, 240, 300 };
    double d = fabs(angDiffHue(hue, centers[channel - 1]));
    const double IN = 15.0, OUT = 45.0;
    if (d <= IN)  return 1.0;
    if (d >= OUT) return 0.0;
    double t = (d - IN) / (OUT - IN);
    return 1.0 - t * t * (3.0 - 2.0 * t); // smoothstep
}

static void rgb2hsl(double r, double g, double b, double &h, double &s, double &l) {
    r /= 255.0; g /= 255.0; b /= 255.0;
    double mx = qMax(r, qMax(g, b)), mn = qMin(r, qMin(g, b));
    l = (mx + mn) * 0.5;
    if (mx == mn) { h = 0; s = 0; return; }
    double d = mx - mn;
    s = (l > 0.5) ? d / (2.0 - mx - mn) : d / (mx + mn);
    if (mx == r)      h = (g - b) / d + (g < b ? 6 : 0);
    else if (mx == g) h = (b - r) / d + 2;
    else              h = (r - g) / d + 4;
    h *= 60.0;
}

static double hue2rgb(double p, double q, double t) {
    if (t < 0) t += 1;
    if (t > 1) t -= 1;
    if (t < 1.0/6.0) return p + (q - p) * 6 * t;
    if (t < 1.0/2.0) return q;
    if (t < 2.0/3.0) return p + (q - p) * (2.0/3.0 - t) * 6;
    return p;
}

static void hsl2rgb(double h, double s, double l, double &r, double &g, double &b) {
    h = fmod(h, 360.0); if (h < 0) h += 360; h /= 360.0;
    if (s < 0.0001) { r = g = b = l * 255.0; return; }
    double q = (l < 0.5) ? l * (1 + s) : l + s - l * s;
    double p = 2 * l - q;
    r = hue2rgb(p, q, h + 1.0/3.0) * 255.0;
    g = hue2rgb(p, q, h)           * 255.0;
    b = hue2rgb(p, q, h - 1.0/3.0) * 255.0;
}

// ============================================================
// Helpers de clamping de píxeles (evitan duplicación masiva)
// ============================================================

/// Luminancia perceptual segura (clamp a [0,255] antes de qGray).
static inline int safeLum(double r, double g, double b) {
    return qGray(qBound(0, (int)(r + 0.5), 255),
                 qBound(0, (int)(g + 0.5), 255),
                 qBound(0, (int)(b + 0.5), 255));
}

/// Empaqueta r,g,b (double, puede estar fuera de rango) y alpha (int) en QRgb,
/// con rounding +0.5 y clamp a [0,255].
static inline QRgb clampRgba(double r, double g, double b, int a) {
    return qRgba(qBound(0, (int)(r + 0.5), 255),
                 qBound(0, (int)(g + 0.5), 255),
                 qBound(0, (int)(b + 0.5), 255),
                 a);
}

// ============================================================
// HueRangeBar (barra de rangos de tono, visual)
// ============================================================

HueRangeBar::HueRangeBar(QWidget *parent) : QWidget(parent) {
    setFixedHeight(52);
    setToolTip(tr("Rango de tono del canal activo (lleno ±15°, caída ±45°)"));
}

void HueRangeBar::setChannel(int ch) { m_ch = qBound(0, ch, 6); update(); }
int HueRangeBar::channel() const { return m_ch; }

void HueRangeBar::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    int w = width();
    if (w <= 0) return;

    for (int x = 0; x < w; ++x) {
        QColor c; c.setHsv((int)(360.0 * x / w), 220, 235);
        p.setPen(c);
        p.drawLine(x, 6, x, 20);
        p.drawLine(x, 32, x, 46);
    }

    if (m_ch <= 0) return;

    double c0 = (m_ch - 1) * 60.0;
    auto X = [w](double hue) { return (int)(fmod(hue + 360.0, 360.0) / 360.0 * (w - 1)); };

    p.setPen(QPen(QColor(255, 255, 255, 110), 5));
    double a = fmod(c0 - 15.0 + 360.0, 360.0), b = fmod(c0 + 15.0, 360.0);
    if (a < b) p.drawLine(X(a), 26, X(b), 26);
    else { p.drawLine(X(a), 26, w - 1, 26); p.drawLine(0, 26, X(b), 26); }

    p.setPen(QPen(QColor(240, 240, 240), 2));
    const double hs[4] = { c0 - 45.0, c0 - 15.0, c0 + 15.0, c0 + 45.0 };
    for (double hd : hs) p.drawLine(X(hd), 22, X(hd), 30);
}

// ============================================================
// CurvEditor
// ============================================================

CurvEditor::CurvEditor(QWidget *parent) : QWidget(parent) {
    setFixedSize(280, 280);
    setCursor(Qt::CrossCursor);
    setFocusPolicy(Qt::StrongFocus);
    resetAll();
}

void CurvEditor::setDarkMode(bool dark) { m_dark = dark; update(); }
bool CurvEditor::isDarkMode() const { return m_dark; }

QVector<QPointF> CurvEditor::getPoints(int channel) const {
    if (channel < 0 || channel > 3) return QVector<QPointF>();
    return m_points[channel];
}

void CurvEditor::setPoints(int channel, const QVector<QPointF> &pts) {
    if (channel < 0 || channel > 3) return;
    m_points[channel] = pts.isEmpty() ? defaultPoints() : pts;
    notifyChanged();
}

void CurvEditor::setAllPoints(const QVector<QPointF> *pts) {
    if (!pts) { resetAll(); return; }
    for (int i = 0; i < 4; ++i)
        m_points[i] = pts[i].isEmpty() ? defaultPoints() : pts[i];
    notifyChanged();
}

void CurvEditor::setChannel(int channel) {
    m_channel = qBound(0, channel, 3);
    notifyChanged();
}

void CurvEditor::setHistogram(const QVector<int> &hist) { m_histogram = hist; update(); }

void CurvEditor::resetChannel(int channel) {
    if (channel < 0 || channel > 3) return;
    m_points[channel] = defaultPoints();
    notifyChanged();
}

void CurvEditor::resetAll() {
    for (int i = 0; i < 4; ++i) m_points[i] = defaultPoints();
    notifyChanged();
}

void CurvEditor::notifyChanged() {
    m_selected = -1;
    emit curveChanged();
    update();
}

bool CurvEditor::isIdentity(const QVector<QPointF> &pts) {
    if (pts.size() != 2) return false;
    return qAbs(pts[0].x()) < 0.01 && qAbs(pts[0].y()) < 0.01 &&
           qAbs(pts[1].x() - 255.0) < 0.01 && qAbs(pts[1].y() - 255.0) < 0.01;
}

void CurvEditor::buildLUT(const QVector<QPointF> &pts, uchar lut[256]) {
    QVector<QPointF> p = pts;
    std::sort(p.begin(), p.end(), [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); });
    if (p.size() < 2) { for (int i = 0; i < 256; ++i) lut[i] = (uchar)i; return; }
    if (isIdentity(p)) { for (int i = 0; i < 256; ++i) lut[i] = (uchar)i; return; }

    int n = p.size();
    QVector<double> xs(n), ys(n);
    for (int i = 0; i < n; ++i) {
        xs[i] = qBound(0.0, p[i].x(), 255.0);
        ys[i] = qBound(0.0, p[i].y(), 255.0);
    }
    QVector<double> delta(n - 1);
    for (int i = 0; i < n - 1; ++i) {
        double dx = xs[i + 1] - xs[i];
        delta[i] = (qAbs(dx) < 0.0001) ? 0.0 : (ys[i + 1] - ys[i]) / dx;
    }
    QVector<double> m(n);
    m[0] = delta[0];
    m[n - 1] = delta[n - 2];
    for (int i = 1; i < n - 1; ++i) {
        if (delta[i - 1] * delta[i] <= 0.0) m[i] = 0.0;
        else m[i] = (delta[i - 1] + delta[i]) / 2.0;
    }
    for (int i = 0; i < n - 1; ++i) {
        if (qAbs(delta[i]) < 0.000001) { m[i] = 0.0; m[i + 1] = 0.0; continue; }
        double a = m[i] / delta[i];
        double b = m[i + 1] / delta[i];
        double s = a * a + b * b;
        if (s > 9.0) {
            double t = 3.0 / sqrt(s);
            m[i] = t * a * delta[i];
            m[i + 1] = t * b * delta[i];
        }
    }

    int seg = 0;
    for (int x = 0; x < 256; ++x) {
        while (seg < n - 2 && x > xs[seg + 1]) seg++;
        double h = xs[seg + 1] - xs[seg];
        if (h < 0.0001) { lut[x] = (uchar)qBound(0.0, ys[seg], 255.0); continue; }
        double t  = (x - xs[seg]) / h;
        double t2 = t * t, t3 = t2 * t;
        double h00 = 2*t3 - 3*t2 + 1;
        double h10 = t3 - 2*t2 + t;
        double h01 = -2*t3 + 3*t2;
        double h11 = t3 - t2;
        double y = h00*ys[seg] + h10*h*m[seg] + h01*ys[seg+1] + h11*h*m[seg+1];
        lut[x] = (uchar)qBound(0, (int)(y + 0.5), 255);
    }
}

QVector<QPointF> CurvEditor::defaultPoints() { return { QPointF(0, 0), QPointF(255, 255) }; }

QColor CurvEditor::channelColor() const {
    switch (m_channel) {
        case 1: return QColor(239, 68, 68);
        case 2: return QColor(34, 197, 94);
        case 3: return QColor(59, 130, 246);
        default: return QColor(200, 200, 200);
    }
}

double CurvEditor::mapX(double v) const { return MARGIN + v / 255.0 * (width()  - 2 * MARGIN); }
double CurvEditor::mapY(double v) const { return height() - MARGIN - v / 255.0 * (height() - 2 * MARGIN); }
double CurvEditor::invX(double px) const { return qBound(0.0, (px - MARGIN) / (width()  - 2 * MARGIN) * 255.0, 255.0); }
double CurvEditor::invY(double py) const { return qBound(0.0, (height() - MARGIN - py) / (height() - 2 * MARGIN) * 255.0, 255.0); }

int CurvEditor::findPointAt(const QPointF &widgetPos) const {
    const QVector<QPointF> &pts = m_points[m_channel];
    for (int i = 0; i < pts.size(); ++i) {
        double dx = mapX(pts[i].x()) - widgetPos.x();
        double dy = mapY(pts[i].y()) - widgetPos.y();
        if (sqrt(dx*dx + dy*dy) <= 9.0) return i;
    }
    return -1;
}

void CurvEditor::paintEvent(QPaintEvent *) {
    const QColor bgCol    = m_dark ? QColor("#1e1e1e") : QColor("#f3f4f6");
    const QColor edgeCol  = m_dark ? QColor("#3a3a3a") : QColor("#d1d5db");
    const QColor gridCol  = m_dark ? QColor("#2e2e2e") : QColor("#e5e7eb");
    const QColor diagCol  = m_dark ? QColor("#555555") : QColor("#9ca3af");
    const QColor ptFill   = m_dark ? QColor("#2a2a2a") : QColor("#ffffff");
    const QColor ptPen    = m_dark ? QColor("#e5e5e5") : QColor("#111827");

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), bgCol);

    p.setPen(QPen(edgeCol, 1));
    p.drawRect(rect().adjusted(0, 0, -1, -1));

    p.setPen(QPen(gridCol, 1, Qt::DotLine));
    for (int q = 1; q < 4; ++q) {
        double v = q * 255.0 / 4.0;
        p.drawLine(QPointF(mapX(v), MARGIN), QPointF(mapX(v), height() - MARGIN));
        p.drawLine(QPointF(MARGIN, mapY(v)), QPointF(width() - MARGIN, mapY(v)));
    }

    if (!m_histogram.isEmpty()) {
        int maxV = 1;
        for (int v : m_histogram) if (v > maxV) maxV = v;
        QColor hcol = channelColor();
        hcol.setAlpha(70);
        if (m_channel == 0) hcol = m_dark ? QColor(160, 160, 170, 70) : QColor(110, 115, 125, 70);
        p.setPen(hcol);
        double logMax = log(1.0 + maxV);
        for (int x = 0; x < 256; ++x) {
            double hNorm = log(1.0 + m_histogram[x]) / logMax;
            int barH = (int)(hNorm * (height() - 2 * MARGIN));
            int sx = (int)mapX(x);
            p.drawLine(sx, height() - MARGIN, sx, height() - MARGIN - barH);
        }
    }

    // LÍNEAS FANTASMA: cuando el canal activo es RGB (Todos),
    // dibujar suavemente las curvas de R/G/B si fueron modificadas
    if (m_channel == 0) {
        for (int ch = 1; ch <= 3; ++ch) {
            if (isIdentity(m_points[ch])) continue;
            uchar ghostLut[256];
            buildLUT(m_points[ch], ghostLut);
            QPainterPath ghostCurve;
            ghostCurve.moveTo(mapX(0), mapY(ghostLut[0]));
            for (int x = 1; x < 256; ++x)
                ghostCurve.lineTo(mapX(x), mapY(ghostLut[x]));
            QColor ghostColor;
            if (ch == 1)      ghostColor = QColor(239, 68, 68, 55);
            else if (ch == 2) ghostColor = QColor(34, 197, 94, 55);
            else              ghostColor = QColor(59, 130, 246, 55);
            p.setPen(QPen(ghostColor, 1.5, Qt::SolidLine));
            p.setBrush(Qt::NoBrush);
            p.drawPath(ghostCurve);
        }
    }

    // Curva principal del canal activo
    uchar lut[256];
    buildLUT(m_points[m_channel], lut);
    QPainterPath curve;
    curve.moveTo(mapX(0), mapY(lut[0]));
    for (int x = 1; x < 256; ++x) curve.lineTo(mapX(x), mapY(lut[x]));
    p.setPen(QPen(channelColor(), 2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(curve);

    p.setPen(QPen(diagCol, 1, Qt::DashLine));
    p.drawLine(QPointF(mapX(0), mapY(0)), QPointF(mapX(255), mapY(255)));

    const QVector<QPointF> &pts = m_points[m_channel];
    for (int i = 0; i < pts.size(); ++i) {
        QPointF c(mapX(pts[i].x()), mapY(pts[i].y()));
        bool sel = (i == m_selected);
        p.setPen(QPen(sel ? QColor("#3b82f6") : ptPen, sel ? 2 : 1));
        p.setBrush(sel ? channelColor() : ptFill);
        p.drawEllipse(c, sel ? 7 : 5, sel ? 7 : 5);
    }
}

void CurvEditor::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        int idx = findPointAt(e->position());
        if (idx >= 0) {
            m_selected = idx;
            m_dragging = true;
        } else {
            double x = invX(e->position().x());
            double y = invY(e->position().y());
            if (x > 1.0 && x < 254.0) {
                QVector<QPointF> &pts = m_points[m_channel];
                int pos = 0;
                while (pos < pts.size() && pts[pos].x() < x) pos++;
                pts.insert(pos, QPointF(x, y));
                m_selected = pos;
                m_dragging = true;
                emit curveChanged();
            }
        }
        update();
    } else if (e->button() == Qt::RightButton) {
        int idx = findPointAt(e->position());
        if (idx > 0 && idx < m_points[m_channel].size() - 1) {
            m_points[m_channel].removeAt(idx);
            notifyChanged();
        }
    }
}

void CurvEditor::mouseMoveEvent(QMouseEvent *e) {
    if (!m_dragging || m_selected < 0) return;
    QVector<QPointF> &pts = m_points[m_channel];
    if (m_selected >= pts.size()) return;
    double nx = pts[m_selected].x();
    double ny = invY(e->position().y());
    if (m_selected > 0 && m_selected < pts.size() - 1) {
        nx = invX(e->position().x());
        nx = qBound(pts[m_selected - 1].x() + 1.0, nx, pts[m_selected + 1].x() - 1.0);
    }
    pts[m_selected] = QPointF(nx, ny);
    emit curveChanged();
    update();
}

void CurvEditor::mouseReleaseEvent(QMouseEvent *) { m_dragging = false; }

void CurvEditor::mouseDoubleClickEvent(QMouseEvent *e) {
    int idx = findPointAt(e->position());
    if (idx > 0 && idx < m_points[m_channel].size() - 1) {
        m_points[m_channel].removeAt(idx);
        notifyChanged();
    }
}

void CurvEditor::keyPressEvent(QKeyEvent *e) {
    if ((e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace) &&
        m_selected > 0 && m_selected < m_points[m_channel].size() - 1) {
        m_points[m_channel].removeAt(m_selected);
        notifyChanged();
        return;
    }
    QWidget::keyPressEvent(e);
}

// ============================================================
// Tema
// ============================================================

bool ImageFiltersDialog::detectDarkTheme() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    auto cs = QGuiApplication::styleHints()->colorScheme();
    if (cs == Qt::ColorScheme::Dark)  return true;
    if (cs == Qt::ColorScheme::Light) return false;
#endif
    QPalette pal = QGuiApplication::palette();
    QColor w = pal.color(QPalette::Window);
    int lum = (w.red() * 299 + w.green() * 587 + w.blue() * 114) / 1000;
    return lum < 128;
}

void ImageFiltersDialog::setupTheme(bool dark) {
    m_dark = dark;
    if (dark) {
        c_bg      = "#1a1a1a";
        c_panel   = "#242424";
        c_input   = "#2a2a2a";
        c_preview = "#1e1e1e";
        c_text    = "#e5e5e5";
        c_textMuted = "#a0a0a0";
        c_textDesc  = "#8a8a8a";
        c_border  = "#3a3a3a";
        c_borderStrong = "#555555";
        c_accent  = "#3b82f6";
        c_accentHover = "#2563eb";
        c_hover   = "#3a3a3a";
        c_groove  = "#444444";
    } else {
        c_bg      = "#f5f5f5";
        c_panel   = "#ffffff";
        c_input   = "#ffffff";
        c_preview = "#f3f4f6";
        c_text    = "#111827";
        c_textMuted = "#6b7280";
        c_textDesc  = "#9ca3af";
        c_border  = "#d1d5db";
        c_borderStrong = "#9ca3af";
        c_accent  = "#2563eb";
        c_accentHover = "#1d4ed8";
        c_hover   = "#e5e7eb";
        c_groove  = "#d1d5db";
    }
}

QString ImageFiltersDialog::labelStyle() const  { return QString("color: %1; font-size: 13px;").arg(c_text); }
QString ImageFiltersDialog::descStyle() const   { return QString("color: %1; font-size: 11px; font-style: italic;").arg(c_textDesc); }
QString ImageFiltersDialog::titleStyle() const  { return QString("color: %1; font-weight: bold; font-size: 13px;").arg(c_textMuted); }
QString ImageFiltersDialog::sliderStyle() const {
    return QString(
        "QSlider::groove:horizontal { height: 4px; background: %1; border-radius: 2px; }"
        "QSlider::sub-page:horizontal { background: %2; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: %2; width: 14px; height: 14px; margin: -5px 0; border-radius: 7px; }"
        "QSlider::handle:horizontal:hover { background: %3; }")
        .arg(c_groove, c_accent, c_accentHover);
}

QString ImageFiltersDialog::comboStyle() const {
    QString txt = m_dark ? "#e0e0e0" : "#1e293b";
    return QString(
        "QComboBox { background-color: %1; color: %2; border: 1px solid %3; border-radius: 5px; padding: 4px 8px; font-size: 12px; min-height: 22px; }"
        "QComboBox:hover { border: 1px solid %4; }"
        "QComboBox::drop-down { border: none; width: 16px; }"
        "QComboBox QAbstractItemView { background-color: %1; color: %2; border: 1px solid %3; selection-background-color: %4; selection-color: #ffffff; outline: none; }")
        .arg(c_input, txt, c_border, c_accent);
}

QString ImageFiltersDialog::frameStyle() const {
    return QString("QFrame { background-color: %1; border: 1px solid %2; border-radius: 6px; }").arg(c_panel, c_border);
}

QString ImageFiltersDialog::smallBtnStyle() const {
    QString txt   = m_dark ? "#e0e0e0" : "#1e293b";
    QString hover = m_dark ? "#3a3a3a" : "#e5e7eb";
    return QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: 5px; padding: 5px 10px; font-size: 12px; }"
        "QPushButton:hover { background-color: %4; border: 1px solid %5; }"
        "QPushButton:pressed { background-color: %4; }")
        .arg(c_input, txt, c_border, hover, c_accent);
}

QString ImageFiltersDialog::accentBtnStyle() const {
    return QString(
        "QPushButton { background-color: %1; color: white; border: none; border-radius: 5px; padding: 6px 14px; font-size: 13px; font-weight: 600; }"
        "QPushButton:hover { background-color: %2; }"
        "QPushButton:pressed { background-color: %2; }")
        .arg(c_accent, c_accentHover);
}

QString ImageFiltersDialog::groupBoxStyle() const {
    return QString(
        "QGroupBox { background-color: %1; color: %2; border: 1px solid %3; border-radius: 6px;"
        " margin-top: 12px; padding: 12px 10px 10px 10px; font-weight: 600; font-size: 13px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; color: %2; }")
        .arg(c_panel, c_textMuted, c_border);
}

QString ImageFiltersDialog::groupBoxAccentStyle() const {
    return QString(
        "QGroupBox { background-color: %1; color: %2; border: 1px solid %3; border-radius: 6px;"
        " margin-top: 12px; padding: 12px 10px 10px 10px; font-weight: 600; font-size: 13px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; color: %3; }")
        .arg(c_panel, c_textMuted, c_accent);
}

void ImageFiltersDialog::styleColorButton(QPushButton *btn, const QColor &col) {
    if (!btn) return;
    int lum = (col.red() * 299 + col.green() * 587 + col.blue() * 114) / 1000;
    QString txt = (lum > 150) ? "#111111" : "#ffffff";
    btn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: 5px;"
        " padding: 5px 10px; font-size: 12px; font-weight: 600; }"
        "QPushButton:hover { border: 1px solid %4; }")
        .arg(col.name(), txt, c_borderStrong, c_accent));
}

QLabel* ImageFiltersDialog::makeDescLabel(const QString &text) const {
    QLabel *lbl = new QLabel(text);
    lbl->setStyleSheet(descStyle());
    lbl->setWordWrap(true);
    return lbl;
}

// ============================================================
// Iconos circulares (filas compactas)
// ============================================================

QIcon ImageFiltersDialog::iconoAjuste(int tipo, bool dark) {
    const int S = 40;
    QPixmap pm(S, S);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QColor bg = dark ? QColor(72, 72, 72) : QColor(226, 229, 233);
    QColor fg = dark ? QColor(235, 235, 235) : QColor(52, 58, 66);
    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawEllipse(2, 2, S - 4, S - 4);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(fg, 2));
    QPointF c(S / 2.0, S / 2.0);

    switch (tipo) {
    case IconoSol: {
        p.setBrush(fg);
        p.drawEllipse(c, 5, 5);
        p.setBrush(Qt::NoBrush);
        for (int i = 0; i < 8; ++i) {
            double a = i * M_PI / 4.0;
            QPointF d(cos(a), sin(a));
            p.drawLine(QPointF(c.x() + d.x() * 8, c.y() + d.y() * 8),
                       QPointF(c.x() + d.x() * 12, c.y() + d.y() * 12));
        }
        break;
    }
    case IconoMitad: {
        p.drawEllipse(c, 9, 9);
        p.setPen(Qt::NoPen);
        p.setBrush(fg);
        QPainterPath half; half.addEllipse(c, 9, 9);
        QPainterPath clip; clip.addRect(QRectF(c.x() - 9, c.y() - 9, 9, 18));
        p.drawPath(half & clip);
        break;
    }
    case IconoCirculo:
        p.setPen(Qt::NoPen); p.setBrush(fg); p.drawEllipse(c, 7, 7);
        break;
    case IconoMitadLineas: {
        p.drawEllipse(c, 9, 9);
        p.setPen(QPen(fg, 1.6));
        for (int i = -1; i <= 1; ++i)
            p.drawLine(QPointF(c.x() - 6, c.y() + i * 4), QPointF(c.x(), c.y() + i * 4));
        break;
    }
    case IconoMitadSolido: {
        p.drawEllipse(c, 9, 9);
        p.setPen(Qt::NoPen); p.setBrush(fg);
        QPainterPath half; half.addEllipse(c, 9, 9);
        QPainterPath clip; clip.addRect(QRectF(c.x(), c.y() - 9, 9, 18));
        p.drawPath(half & clip);
        break;
    }
    case IconoAro:
        p.setPen(QPen(fg, 2.4));
        p.drawEllipse(c, 8, 8);
        break;
    case IconoGota: {
        QPainterPath path;
        path.moveTo(c.x(), c.y() - 10);
        path.cubicTo(c.x() + 8, c.y() - 1, c.x() + 7, c.y() + 8, c.x(), c.y() + 9);
        path.cubicTo(c.x() - 7, c.y() + 8, c.x() - 8, c.y() - 1, c.x(), c.y() - 10);
        p.setPen(Qt::NoPen); p.setBrush(fg); p.drawPath(path);
        break;
    }
    case IconoTermometro: {
        p.setPen(QPen(fg, 2));
        p.drawLine(QPointF(c.x(), c.y() - 9), QPointF(c.x(), c.y() + 4));
        p.setPen(Qt::NoPen); p.setBrush(fg);
        p.drawEllipse(QPointF(c.x(), c.y() + 7), 4, 4);
        p.setPen(QPen(fg, 1.4));
        p.drawLine(QPointF(c.x() + 2, c.y() - 6), QPointF(c.x() + 5, c.y() - 6));
        p.drawLine(QPointF(c.x() + 2, c.y() - 2), QPointF(c.x() + 5, c.y() - 2));
        break;
    }
    case IconoMontania:
    default: {
        p.setPen(Qt::NoPen); p.setBrush(fg);
        QPainterPath m;
        m.moveTo(c.x() - 10, c.y() + 8);
        m.lineTo(c.x() - 2, c.y() - 6);
        m.lineTo(c.x() + 3, c.y() + 2);
        m.lineTo(c.x() + 6, c.y() - 2);
        m.lineTo(c.x() + 10, c.y() + 8);
        m.closeSubpath();
        p.drawPath(m);
        p.drawEllipse(QPointF(c.x() + 5, c.y() - 7), 2.5, 2.5);
        break;
    }
    }
    p.end();
    return QIcon(pm);
}

// ============================================================
// Fila compacta: icono circular + nombre + valor + slider
// ============================================================

QWidget *ImageFiltersDialog::makeCompactRow(const QIcon &ic, const QString &label,
    int min, int max, int initial,
    QSlider **outSlider, std::function<void(int)> onChanged,
    const QString &tooltip, QWidget *rightWidget)
{
    QWidget *row = new QWidget();
    row->setStyleSheet("background: transparent;");
    QHBoxLayout *lay = new QHBoxLayout(row);
    lay->setContentsMargins(2, 5, 2, 5);
    lay->setSpacing(10);

    QLabel *ico = new QLabel();
    ico->setFixedSize(40, 40);
    ico->setPixmap(ic.pixmap(36, 36));
    ico->setStyleSheet("background: transparent; border: none;");
    lay->addWidget(ico);

    QVBoxLayout *col = new QVBoxLayout();
    col->setSpacing(3);
    QHBoxLayout *top = new QHBoxLayout();
    top->setSpacing(6);

    QLabel *lbl = new QLabel(label);
    lbl->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 600; background: transparent;").arg(c_text));
    top->addWidget(lbl);
    top->addStretch();

    QLabel *val = new QLabel(QString::number(initial));
    val->setStyleSheet(QString("color: %1; font-size: 11px; background: transparent;").arg(c_textMuted));
    val->setMinimumWidth(34);
    val->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    top->addWidget(val);

    if (rightWidget) top->addWidget(rightWidget);

    col->addLayout(top);

    QSlider *s = new QSlider(Qt::Horizontal);
    s->setRange(min, max);
    s->setValue(initial);
    s->setStyleSheet(sliderStyle());
    col->addWidget(s);
    lay->addLayout(col, 1);

    if (!tooltip.isEmpty()) {
        s->setToolTip(tooltip);
        lbl->setToolTip(tooltip);
        ico->setToolTip(tooltip);
        if (rightWidget && rightWidget->toolTip().isEmpty()) rightWidget->setToolTip(tooltip);
    }

    connect(s, &QSlider::valueChanged, this, [val, onChanged](int v) {
        val->setText(QString::number(v));
        onChanged(v);
    });
    m_sliderLabels.insert(s, val);
    if (outSlider) *outSlider = s;
    return row;
}

QFrame *ImageFiltersDialog::makeSeparator() {
    QFrame *f = new QFrame();
    f->setFrameShape(QFrame::HLine);
    f->setFixedHeight(1);
    f->setStyleSheet(QString("background-color: %1; border: none;").arg(c_border));
    return f;
}

QLabel *ImageFiltersDialog::makeSectionTitle(const QString &t) {
    QLabel *l = new QLabel(t);
    l->setStyleSheet(QString("color: %1; font-size: 10px; font-weight: 700; letter-spacing: 1.5px; background: transparent;").arg(c_textMuted));
    return l;
}

void ImageFiltersDialog::applyGlobalStyleSheet() {
    QString comboTxt = m_dark ? "#e0e0e0" : "#1e293b";
    QString btnTxt   = m_dark ? "#e0e0e0" : "#1e293b";
    QString qss;
    qss += QString("QDialog { background-color: %1; font-family: 'Adwaita Sans', 'Noto Sans', sans-serif; }").arg(c_bg);
    qss += QString("QDialog QLabel { color: %1; font-size: 13px; background: transparent; }").arg(c_text);
    qss += QString("QCheckBox { color: %1; font-size: 13px; spacing: 5px; background: transparent; }").arg(c_text);
    qss += QString("QCheckBox::indicator { width: 15px; height: 15px; border: 1px solid %1; border-radius: 3px; background-color: %2; }").arg(c_borderStrong, c_input);
    qss += QString("QCheckBox::indicator:hover { border: 1px solid %1; }").arg(c_accent);
    qss += QString("QCheckBox::indicator:checked { background-color: %1; border: 1px solid %1; }").arg(c_accent);
    qss += QString("QRadioButton { color: %1; font-size: 13px; }").arg(c_text);
    qss += groupBoxStyle();
    qss += QString("QTabWidget::pane { border: 1px solid %1; border-radius: 6px; background-color: %2; top: -1px; }").arg(c_border, c_bg);
    qss += QString("QTabBar::tab { background-color: %1; color: %2; padding: 6px 14px; border: 1px solid %3;"
                   " border-top-left-radius: 5px; border-top-right-radius: 5px; margin-right: 2px; font-size: 13px; }").arg(c_panel, c_textMuted, c_border);
    qss += QString("QTabBar::tab:hover { background-color: %1; color: %2; }").arg(c_hover, c_text);
    qss += QString("QTabBar::tab:selected { background-color: %1; color: %2; border-bottom: 2px solid %3; font-weight: 600; }").arg(c_input, c_text, c_accent);
    qss += QString("QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: 5px; padding: 5px 12px; font-size: 13px; }").arg(c_input, btnTxt, c_border);
    qss += QString("QPushButton:hover { background-color: %1; border: 1px solid %2; }").arg(c_hover, c_accent);
    qss += QString("QPushButton:pressed { background-color: %1; }").arg(c_hover);
    qss += sliderStyle();
    qss += comboStyle();
    qss += QString("QScrollArea { background-color: transparent; border: none; }");
    qss += QString("QScrollBar:vertical { background: transparent; width: 8px; margin: 2px; }");
    qss += QString("QScrollBar::handle:vertical { background: %1; border-radius: 4px; min-height: 20px; }").arg(c_borderStrong);
    qss += QString("QScrollBar::handle:vertical:hover { background: %1; }").arg(c_accent);
    qss += QString("QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }");
    qss += QString("QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }");
    qss += QString("QScrollBar:horizontal { background: transparent; height: 8px; margin: 2px; }");
    qss += QString("QScrollBar::handle:horizontal { background: %1; border-radius: 4px; min-width: 20px; }").arg(c_borderStrong);
    qss += QString("QScrollBar::handle:horizontal:hover { background: %1; }").arg(c_accent);
    qss += QString("QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }");
    qss += QString("QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }");
    qss += QString("QToolTip { background-color: %1; color: %2; border: 1px solid %3; padding: 3px; font-size: 12px; }").arg(c_panel, c_text, c_border);
    setStyleSheet(qss);
}

// ============================================================
// Constructor
// ============================================================

ImageFiltersDialog::ImageFiltersDialog(const QImage &img, QWidget *parent, bool floatingMode, int darkMode)
    : QDialog(parent), originalImage(img)
{
    setupTheme(darkMode < 0 ? detectDarkTheme() : (darkMode != 0));
    applyGlobalStyleSheet();

    if (floatingMode) {
        setWindowTitle(tr("Máscara de COLOR - Ajustes y Filtros"));
        setWindowFlags(windowFlags() | Qt::Tool | Qt::WindowStaysOnTopHint);
        setMinimumSize(400, 580);
    } else {
        setWindowTitle(tr("Filtros y Ajustes de Imagen"));
        setMinimumSize(860, 600);
    }

    workImage = img.scaled(640, 520, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                   .convertToFormat(QImage::Format_ARGB32);
    if (workImage.isNull()) workImage = QImage(16, 16, QImage::Format_ARGB32);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    tabs = new QTabWidget();
    tabs->setFixedWidth(400);

    // ========================================================
    // PESTAÑA BÁSICO
    // ========================================================
    QWidget *basicWidget = new QWidget();
    basicWidget->setObjectName("basicTabWidget");
    basicWidget->setStyleSheet(QString("QWidget#basicTabWidget { background-color: %1; }").arg(c_bg));
    QVBoxLayout *controlsLayout = new QVBoxLayout(basicWidget);
    controlsLayout->setSpacing(2);
    controlsLayout->setContentsMargins(6, 8, 6, 8);

    controlsLayout->addWidget(makeSectionTitle(tr("LUZ")));
    controlsLayout->addWidget(makeCompactRow(iconoAjuste(IconoSol, m_dark), tr("Brillo"),
        -100, 100, 0, &sliderBrightness, [this](int v) { brightness = v / 100.0; applyFilters(); },
        tr("Luz general")));
    controlsLayout->addWidget(makeCompactRow(iconoAjuste(IconoMitad, m_dark), tr("Contraste"),
        -100, 100, 0, &sliderContrast, [this](int v) { contrast = v / 100.0; applyFilters(); },
        tr("Luces vs sombras")));
    controlsLayout->addWidget(makeCompactRow(iconoAjuste(IconoMontania, m_dark), tr("Exposición"),
        -100, 100, 0, &sliderExposure, [this](int v) { exposure = v / 100.0; applyFilters(); },
        tr("Luz tipo cámara")));

    controlsLayout->addWidget(makeSeparator());
    controlsLayout->addWidget(makeSectionTitle(tr("COLOR")));
    controlsLayout->addWidget(makeCompactRow(iconoAjuste(IconoGota, m_dark), tr("Saturación"),
        -100, 100, 0, &sliderSaturation, [this](int v) { saturation = v / 100.0; applyFilters(); },
        tr("Intensidad de color")));
    controlsLayout->addWidget(makeCompactRow(iconoAjuste(IconoTermometro, m_dark), tr("Temperatura"),
        -100, 100, 0, &sliderTemperature, [this](int v) { temperature = v / 100.0; applyFilters(); },
        tr("Cálido o frío")));

    controlsLayout->addWidget(makeSeparator());
    controlsLayout->addWidget(makeSectionTitle(tr("ZONAS")));

    btnHighlightColor = new QPushButton();
    btnHighlightColor->setFixedSize(26, 26);
    btnHighlightColor->setCursor(Qt::PointingHandCursor);
    btnHighlightColor->setToolTip(tr("Color de luces"));
    styleColorButton(btnHighlightColor, highlightColor);
    connect(btnHighlightColor, &QPushButton::clicked, this, [this]() {
        QColor color = QColorDialog::getColor(highlightColor, this, tr("Seleccionar Color de Iluminaciones"));
        if (color.isValid()) { highlightColor = color; styleColorButton(btnHighlightColor, highlightColor); markCustom(); applyFilters(); }
    });
    controlsLayout->addWidget(makeCompactRow(iconoAjuste(IconoMitadLineas, m_dark), tr("Iluminaciones"),
        -100, 100, 0, &sliderHighlights, [this](int v) { highlights = v / 100.0; applyFilters(); },
        tr("Zonas brillantes"), btnHighlightColor));

    btnShadowColor = new QPushButton();
    btnShadowColor->setFixedSize(26, 26);
    btnShadowColor->setCursor(Qt::PointingHandCursor);
    btnShadowColor->setToolTip(tr("Color de sombras"));
    styleColorButton(btnShadowColor, shadowColor);
    connect(btnShadowColor, &QPushButton::clicked, this, [this]() {
        QColor color = QColorDialog::getColor(shadowColor, this, tr("Seleccionar Color de Sombras"));
        if (color.isValid()) { shadowColor = color; styleColorButton(btnShadowColor, shadowColor); markCustom(); applyFilters(); }
    });
    controlsLayout->addWidget(makeCompactRow(iconoAjuste(IconoMitadSolido, m_dark), tr("Sombras"),
        -100, 100, 0, &sliderShadows, [this](int v) { shadows = v / 100.0; applyFilters(); },
        tr("Zonas oscuras"), btnShadowColor));

    controlsLayout->addWidget(makeCompactRow(iconoAjuste(IconoAro, m_dark), tr("Viñeta"),
        0, 100, 0, &sliderVignette, [this](int v) { vignette = v / 100.0; applyFilters(); },
        tr("Bordes oscuros")));

    controlsLayout->addWidget(makeSeparator());
    controlsLayout->addWidget(makePresetGroup());

    QGroupBox *groupEffects = new QGroupBox(tr("Efectos"));
    QVBoxLayout *layoutEffects = new QVBoxLayout(groupEffects);
    layoutEffects->setSpacing(4);

    chkInvertColors = new QCheckBox(tr("Invertir"));
    chkInvertColors->setToolTip(tr("Negativo"));
    connect(chkInvertColors, &QCheckBox::toggled, this, [this](bool c) { invertColors = c; applyFilters(); });
    layoutEffects->addWidget(chkInvertColors);

    chkGrayscale = new QCheckBox(tr("Grises"));
    chkGrayscale->setToolTip(tr("Sin color"));
    connect(chkGrayscale, &QCheckBox::toggled, this, [this](bool c) {
        grayscale = c; if (c) chkSepia->setChecked(false); applyFilters();
    });
    layoutEffects->addWidget(chkGrayscale);

    chkSepia = new QCheckBox(tr("Sepia"));
    chkSepia->setToolTip(tr("Tono antiguo"));
    connect(chkSepia, &QCheckBox::toggled, this, [this](bool c) {
        sepia = c; if (c) chkGrayscale->setChecked(false); applyFilters();
    });
    layoutEffects->addWidget(chkSepia);

    chkColorize = new QCheckBox(tr("Colorizar B/N"));
    chkColorize->setToolTip(tr("B/N a color"));
    connect(chkColorize, &QCheckBox::toggled, this, [this](bool c) {
        colorize = c; if (c) { chkGrayscale->setChecked(false); chkSepia->setChecked(false); } applyFilters();
    });
    sliderColorizeStrength = new QSlider(Qt::Horizontal);
    sliderColorizeStrength->setRange(0, 100); sliderColorizeStrength->setValue(100);
    sliderColorizeStrength->setStyleSheet(sliderStyle());
    lblColorizeStrength = new QLabel("100%");
    lblColorizeStrength->setStyleSheet(labelStyle());
    connect(sliderColorizeStrength, &QSlider::valueChanged, this, [this](int v) {
        colorizeStrength = v / 100.0;
        lblColorizeStrength->setText(QString("%1%").arg(v));
        if (colorize) applyFilters();
    });
    QHBoxLayout *layoutColorize = new QHBoxLayout();
    layoutColorize->addWidget(chkColorize);
    layoutColorize->addWidget(sliderColorizeStrength, 1);
    layoutColorize->addWidget(lblColorizeStrength);
    layoutEffects->addLayout(layoutColorize);

    chkBlur = new QCheckBox(tr("Desenfoque"));
    chkBlur->setToolTip(tr("Suavizar"));
    sliderBlurRadius = new QSlider(Qt::Horizontal);
    sliderBlurRadius->setRange(1, 20); sliderBlurRadius->setValue(5);
    sliderBlurRadius->setStyleSheet(sliderStyle());
    lblBlurRadius = new QLabel(tr("Radio: 5"));
    lblBlurRadius->setStyleSheet(labelStyle());
    connect(chkBlur, &QCheckBox::toggled, this, [this](bool c) { blur = c; applyFilters(); });
    connect(sliderBlurRadius, &QSlider::valueChanged, this, [this](int v) {
        blurRadius = v; lblBlurRadius->setText(tr("Radio: %1").arg(v)); if (blur) applyFilters();
    });
    QHBoxLayout *layoutBlur = new QHBoxLayout();
    layoutBlur->addWidget(chkBlur); layoutBlur->addWidget(sliderBlurRadius); layoutBlur->addWidget(lblBlurRadius);
    layoutEffects->addLayout(layoutBlur);

    chkSharpen = new QCheckBox(tr("Enfocar"));
    chkSharpen->setToolTip(tr("Nitidez"));
    sliderSharpenStrength = new QSlider(Qt::Horizontal);
    sliderSharpenStrength->setRange(10, 100); sliderSharpenStrength->setValue(50);
    sliderSharpenStrength->setStyleSheet(sliderStyle());
    lblSharpenStrength = new QLabel(tr("Fuerza: 50"));
    lblSharpenStrength->setStyleSheet(labelStyle());
    connect(chkSharpen, &QCheckBox::toggled, this, [this](bool c) { sharpen = c; applyFilters(); });
    connect(sliderSharpenStrength, &QSlider::valueChanged, this, [this](int v) {
        sharpenStrength = v; lblSharpenStrength->setText(tr("Fuerza: %1").arg(v)); if (sharpen) applyFilters();
    });
    QHBoxLayout *layoutSharpen = new QHBoxLayout();
    layoutSharpen->addWidget(chkSharpen); layoutSharpen->addWidget(sliderSharpenStrength); layoutSharpen->addWidget(lblSharpenStrength);
    layoutEffects->addLayout(layoutSharpen);

    chkPixelate = new QCheckBox(tr("Pixelar"));
    chkPixelate->setToolTip(tr("Mosaico"));
    sliderPixelateSize = new QSlider(Qt::Horizontal);
    sliderPixelateSize->setRange(2, 64); sliderPixelateSize->setValue(8);
    sliderPixelateSize->setStyleSheet(sliderStyle());
    lblPixelateSize = new QLabel(tr("Bloque: 8 px"));
    lblPixelateSize->setStyleSheet(labelStyle());
    connect(chkPixelate, &QCheckBox::toggled, this, [this](bool c) { pixelate = c; applyFilters(); });
    connect(sliderPixelateSize, &QSlider::valueChanged, this, [this](int v) {
        pixelateSize = v; lblPixelateSize->setText(tr("Bloque: %1 px").arg(v)); if (pixelate) applyFilters();
    });
    QHBoxLayout *layoutPixelate = new QHBoxLayout();
    layoutPixelate->addWidget(chkPixelate);
    layoutPixelate->addWidget(sliderPixelateSize);
    layoutPixelate->addWidget(lblPixelateSize);
    layoutEffects->addLayout(layoutPixelate);

    chkHalftone = new QCheckBox(tr("Halftone"));
    chkHalftone->setToolTip(tr("Puntos de imprenta"));
    sliderHalftoneCell = new QSlider(Qt::Horizontal);
    sliderHalftoneCell->setRange(2, 40); sliderHalftoneCell->setValue(6);
    sliderHalftoneCell->setStyleSheet(sliderStyle());
    lblHalftoneCell = new QLabel(tr("Celda: 6 px"));
    lblHalftoneCell->setStyleSheet(labelStyle());
    connect(chkHalftone, &QCheckBox::toggled, this, [this](bool c) { halftone = c; applyFilters(); });
    connect(sliderHalftoneCell, &QSlider::valueChanged, this, [this](int v) {
        halftoneCell = v; lblHalftoneCell->setText(tr("Celda: %1 px").arg(v)); if (halftone) applyFilters();
    });
    QHBoxLayout *layoutHalftone = new QHBoxLayout();
    layoutHalftone->addWidget(chkHalftone);
    layoutHalftone->addWidget(sliderHalftoneCell);
    layoutHalftone->addWidget(lblHalftoneCell);
    layoutEffects->addLayout(layoutHalftone);

    controlsLayout->addWidget(groupEffects);
    controlsLayout->addStretch();

    QScrollArea *basicScroll = new QScrollArea();
    basicScroll->setWidgetResizable(true);
    basicScroll->setWidget(basicWidget);
    tabs->addTab(basicScroll, tr("Básico"));

    // ========================================================
    // PESTAÑA CURVAS
    // ========================================================
    QWidget *curvesTab = new QWidget();
    curvesTab->setObjectName("curvesTabWidget");
    curvesTab->setStyleSheet(QString("QWidget#curvesTabWidget { background-color: %1; }").arg(c_bg));
    QVBoxLayout *curvesLayout = new QVBoxLayout(curvesTab);
    curvesLayout->setSpacing(6);
    curvesLayout->setContentsMargins(4, 4, 4, 4);

    curvesLayout->addWidget(makeDescLabel(tr("Arrastra los puntos · doble clic elimina")));

    QHBoxLayout *channelRow = new QHBoxLayout();
    QLabel *lblChannel = new QLabel(tr("Canal:"));
    lblChannel->setStyleSheet(labelStyle());
    channelRow->addWidget(lblChannel);
    comboChannel = new QComboBox();
    comboChannel->addItem(tr("RGB (Todos)"));
    comboChannel->addItem(tr("Rojo"));
    comboChannel->addItem(tr("Verde"));
    comboChannel->addItem(tr("Azul"));
    comboChannel->setStyleSheet(comboStyle());
    comboChannel->setCursor(Qt::PointingHandCursor);
    channelRow->addWidget(comboChannel, 1);
    curvesLayout->addLayout(channelRow);

    curvEditor = new CurvEditor();
    curvEditor->setDarkMode(m_dark);
    curvesLayout->addWidget(curvEditor, 0, Qt::AlignCenter);

    QHBoxLayout *curveBtnRow = new QHBoxLayout();
    QPushButton *btnResetChannel = new QPushButton(tr("Reiniciar canal"));
    QPushButton *btnResetCurves  = new QPushButton(tr("Reiniciar todo"));
    btnResetChannel->setCursor(Qt::PointingHandCursor);
    btnResetCurves->setCursor(Qt::PointingHandCursor);
    btnResetChannel->setStyleSheet(smallBtnStyle());
    btnResetCurves->setStyleSheet(smallBtnStyle());
    curveBtnRow->addWidget(btnResetChannel);
    curveBtnRow->addWidget(btnResetCurves);
    curvesLayout->addLayout(curveBtnRow);
    curvesLayout->addStretch();

    tabs->addTab(curvesTab, tr("Curvas"));

    connect(comboChannel, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        curvEditor->setChannel(idx);
    });
    connect(curvEditor, &CurvEditor::curveChanged, this, [this]() { applyFilters(); });
    connect(btnResetChannel, &QPushButton::clicked, this, [this]() {
        curvEditor->resetChannel(comboChannel->currentIndex());
    });
    connect(btnResetCurves, &QPushButton::clicked, this, [this]() {
        curvEditor->resetAll();
    });

    // ========================================================
    // PESTAÑA COLOR
    // ========================================================
    QWidget *rawTab = new QWidget();
    rawTab->setObjectName("rawTabWidget");
    rawTab->setStyleSheet(QString("QWidget#rawTabWidget { background-color: %1; }").arg(c_bg));
    QVBoxLayout *rawLayout = new QVBoxLayout(rawTab);
    rawLayout->setSpacing(2);
    rawLayout->setContentsMargins(6, 8, 6, 8);

    rawLayout->addWidget(makeSectionTitle(tr("COLOR")));
    rawLayout->addWidget(makeCompactRow(iconoAjuste(IconoMitad, m_dark), tr("Tinte"),
        -100, 100, 0, &sliderRawTint, [this](int v) { rawTint = v; applyFilters(); },
        tr("Verde o magenta")));
    rawLayout->addWidget(makeCompactRow(iconoAjuste(IconoGota, m_dark), tr("Intensidad"),
        0, 100, 0, &sliderRawVibrance, [this](int v) { rawVibrance = v; applyFilters(); },
        tr("Satura solo lo apagado (automático)")));

    rawLayout->addWidget(makeSeparator());
    rawLayout->addWidget(makeSectionTitle(tr("HSL POR CANAL")));

    comboHslChannel = new QComboBox();
    comboHslChannel->addItems({ tr("Master"), tr("Rojos"), tr("Amarillos"), tr("Verdes"),
                                tr("Cianes"), tr("Azules"), tr("Magentas") });
    comboHslChannel->setStyleSheet(comboStyle());
    comboHslChannel->setCursor(Qt::PointingHandCursor);
    comboHslChannel->setToolTip(tr("Canal de color a ajustar (caída suave ±45°)"));
    rawLayout->addWidget(comboHslChannel);

    rawLayout->addWidget(makeCompactRow(iconoAjuste(IconoGota, m_dark), tr("Tono"),
        -180, 180, 0, &sliderHslHue, [this](int v) { m_hsl[m_hslChannel].hue = v; applyFilters(); },
        tr("Rota el tono del canal")));
    rawLayout->addWidget(makeCompactRow(iconoAjuste(IconoCirculo, m_dark), tr("Saturación"),
        -100, 100, 0, &sliderHslSat, [this](int v) { m_hsl[m_hslChannel].saturation = v; applyFilters(); },
        tr("Satura o desatura solo este rango de color")));
    rawLayout->addWidget(makeCompactRow(iconoAjuste(IconoSol, m_dark), tr("Luminosidad"),
        -100, 100, 0, &sliderHslLum, [this](int v) { m_hsl[m_hslChannel].lightness = v; applyFilters(); },
        tr("Aclara u oscurece este rango")));

    chkColorizeHSL = new QCheckBox(tr("Colorizar (tono único)"));
    chkColorizeHSL->setToolTip(tr("Convierte todo a un solo tono usando Tono/Saturación del Master (Colorize de Photoshop)"));
    connect(chkColorizeHSL, &QCheckBox::toggled, this, [this](bool c) { colorizeHSL = c; applyFilters(); });
    rawLayout->addWidget(chkColorizeHSL);

    rangeBar = new HueRangeBar();
    rawLayout->addWidget(rangeBar);

    rawLayout->addWidget(makeDescLabel(tr("Efecto completo ±15° alrededor del canal · caída suave hasta ±45°. "
        "Temperatura, contraste, zonas y luz están en la pestaña Básico.")));
    rawLayout->addStretch();

    QScrollArea *rawScroll = new QScrollArea();
    rawScroll->setWidgetResizable(true);
    rawScroll->setWidget(rawTab);
    tabs->addTab(rawScroll, tr("Color"));

    connect(comboHslChannel, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        m_hslChannel = qBound(0, idx, 6);
        loadHslSliders();
    });
    loadHslSliders();

    if (floatingMode) {
        QVBoxLayout *leftCol = new QVBoxLayout();
        leftCol->setSpacing(6);
        leftCol->addWidget(tabs, 1);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Reset);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &ImageFiltersDialog::resetFilters);
        if (QPushButton *bOk     = buttonBox->button(QDialogButtonBox::Ok))     { bOk->setText(tr("Aplicar máscara")); bOk->setStyleSheet(accentBtnStyle()); bOk->setCursor(Qt::PointingHandCursor); }
        if (QPushButton *bCancel = buttonBox->button(QDialogButtonBox::Cancel)) { bCancel->setText(tr("Cancelar"));    bCancel->setStyleSheet(smallBtnStyle()); bCancel->setCursor(Qt::PointingHandCursor); }
        if (QPushButton *bReset  = buttonBox->button(QDialogButtonBox::Reset))  { bReset->setText(tr("Restablecer"));  bReset->setStyleSheet(smallBtnStyle());  bReset->setCursor(Qt::PointingHandCursor); }
        leftCol->addWidget(buttonBox);
        mainLayout->addLayout(leftCol, 1);
    } else {
        mainLayout->addWidget(tabs);

        QVBoxLayout *previewLayout = new QVBoxLayout();
        previewLayout->setSpacing(6);

        QLabel *lblPreviewTitle = new QLabel(tr("Vista Previa"));
        lblPreviewTitle->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        lblPreviewTitle->setStyleSheet(QString("color: #ffffff; font-weight: bold; font-size: 16px; padding: 2px 4px;"));

        QScrollArea *previewScroll = new QScrollArea();
        previewScroll->setWidgetResizable(true);
        previewScroll->setAlignment(Qt::AlignCenter);
        previewScroll->setStyleSheet(QString("QScrollArea { background-color: %1; border: 1px solid %2; border-radius: 6px; }").arg(c_preview, c_border));

        lblPreview = new QLabel();
        lblPreview->setAlignment(Qt::AlignCenter);
        lblPreview->setStyleSheet("background-color: transparent; border: none;");
        previewScroll->setWidget(lblPreview);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Reset);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &ImageFiltersDialog::resetFilters);
        if (QPushButton *bOk     = buttonBox->button(QDialogButtonBox::Ok))     { bOk->setText(tr("Aceptar"));    bOk->setStyleSheet(accentBtnStyle()); bOk->setCursor(Qt::PointingHandCursor); }
        if (QPushButton *bCancel = buttonBox->button(QDialogButtonBox::Cancel)) { bCancel->setText(tr("Cancelar")); bCancel->setStyleSheet(smallBtnStyle()); bCancel->setCursor(Qt::PointingHandCursor); }
        if (QPushButton *bReset  = buttonBox->button(QDialogButtonBox::Reset))  { bReset->setText(tr("Restablecer")); bReset->setStyleSheet(smallBtnStyle()); bReset->setCursor(Qt::PointingHandCursor); }

        previewLayout->addWidget(lblPreviewTitle);
        previewLayout->addWidget(previewScroll, 1);
        previewLayout->addWidget(buttonBox);
        mainLayout->addLayout(previewLayout, 1);
    }

    setLayout(mainLayout);

    QVector<int> hist(256, 0);
    for (int y = 0; y < workImage.height(); ++y) {
        const QRgb *line = (const QRgb*)workImage.constScanLine(y);
        for (int x = 0; x < workImage.width(); ++x) hist[qGray(line[x])]++;
    }
    curvEditor->setHistogram(hist);

    connectPresetControls();
    if (comboPresets) { comboPresets->setCurrentIndex(0); }
    applyFilters();
}

// ============================================================
// Presets UI
// ============================================================

QGroupBox *ImageFiltersDialog::makePresetGroup() {
    QGroupBox *box = new QGroupBox(tr("Filtros rápidos (presets)"));
    box->setStyleSheet(groupBoxAccentStyle());
    QVBoxLayout *lay = new QVBoxLayout(box);
    lay->setSpacing(6);

    comboPresets = new QComboBox();
    comboPresets->setCursor(Qt::PointingHandCursor);
    comboPresets->setMinimumHeight(28);
    comboPresets->setStyleSheet(comboStyle() + "QComboBox { font-weight: 600; }");
    comboPresets->addItem(tr("Sin filtro / Manual"),                  (int)PresetNone);
    comboPresets->insertSeparator(comboPresets->count());
    comboPresets->addItem(tr("MMADRO"),                               (int)PresetMMADRO);
    comboPresets->addItem(tr("POP"),                                  (int)PresetPop);
    comboPresets->addItem(tr("Y2K"),                                  (int)PresetY2K);
    comboPresets->addItem(tr("Chernóbil"),                            (int)PresetChernobil);
    comboPresets->addItem(tr("Pixel Art"),                            (int)PresetPixelArt);
    comboPresets->insertSeparator(comboPresets->count());
    comboPresets->addItem(tr("Vaporwave"),                            (int)PresetVaporwave);
    comboPresets->addItem(tr("Noir"),                                 (int)PresetNoir);
    comboPresets->addItem(tr("Kodak 80s"),                            (int)PresetKodak80);
    comboPresets->addItem(tr("Ártico"),                               (int)PresetArctic);
    comboPresets->addItem(tr("Pastel Dream"),                         (int)PresetPastel);
    comboPresets->addItem(tr("Neón Nocturno"),                        (int)PresetNeon);
    comboPresets->addItem(tr("NoMéxico IR"),                          (int)PresetNoMexico);
    lay->addWidget(comboPresets);

    sliderPresetStrength = new QSlider(Qt::Horizontal);
    sliderPresetStrength->setRange(0, 100);
    sliderPresetStrength->setValue(50);
    sliderPresetStrength->setStyleSheet(sliderStyle());
    sliderPresetStrength->setToolTip(tr("Intensidad del preset"));
    lay->addWidget(sliderPresetStrength);

    return box;
}

void ImageFiltersDialog::connectPresetControls() {
    if (!comboPresets) return;
    connect(comboPresets, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        if (m_applyingPreset || idx < 0) return;
        applyPresetById(comboPresets->itemData(idx).toInt());
    });
    if (sliderPresetStrength) {
        connect(sliderPresetStrength, &QSlider::valueChanged, this, [this](int) {
            if (m_applyingPreset) return;
            if (m_currentPresetId != PresetNone) applyPresetById(m_currentPresetId);
        });
    }
    const QList<QWidget*> ws = allParamWidgets();
    for (QWidget *w : ws) {
        if (QSlider *s = qobject_cast<QSlider*>(w))
            connect(s, &QSlider::valueChanged, this, [this](int) { markCustom(); });
        else if (QCheckBox *c = qobject_cast<QCheckBox*>(w))
            connect(c, &QCheckBox::toggled, this, [this](bool) { markCustom(); });
    }
    if (curvEditor)
        connect(curvEditor, &CurvEditor::curveChanged, this, [this]() { markCustom(); });
}

void ImageFiltersDialog::markCustom() {
    if (m_applyingPreset) return;
    if (comboPresets && comboPresets->currentData().toInt() != PresetNone) {
        comboPresets->blockSignals(true);
        comboPresets->setCurrentIndex(0);
        comboPresets->blockSignals(false);
    }
    m_currentPresetId = PresetNone;
}

void ImageFiltersDialog::applyPresetById(int id) {
    m_currentPresetId = id;
    if (comboPresets) {
        int idx = comboPresets->findData(id);
        if (idx >= 0 && idx != comboPresets->currentIndex()) {
            comboPresets->blockSignals(true);
            comboPresets->setCurrentIndex(idx);
            comboPresets->blockSignals(false);
        }
    }
    if (id == PresetNone) {
        setParams(FilterParams());
    } else {
        double k = sliderPresetStrength ? (sliderPresetStrength->value() / 100.0) : 0.5;
        setParams(blendPreset(presetParams(id), k));
    }
}

QList<QWidget*> ImageFiltersDialog::allParamWidgets() const {
    QList<QWidget*> ws;
    ws << sliderBrightness << sliderContrast << sliderSaturation << sliderExposure
       << sliderShadows << sliderHighlights << sliderTemperature << sliderVignette
       << sliderBlurRadius << sliderSharpenStrength
       << sliderPixelateSize
       << sliderHalftoneCell
       << sliderColorizeStrength
       << sliderRawTint << sliderRawVibrance
       << sliderHslHue << sliderHslSat << sliderHslLum
       << chkInvertColors << chkGrayscale << chkSepia << chkBlur << chkSharpen
       << chkPixelate
       << chkHalftone
       << chkColorize
       << chkColorizeHSL;
    return ws;
}

void ImageFiltersDialog::setSliderValue(QSlider *s, int v) {
    if (!s) return;
    v = qBound(s->minimum(), v, s->maximum());
    s->blockSignals(true);
    s->setValue(v);
    s->blockSignals(false);
    if (QLabel *lbl = m_sliderLabels.value(s, nullptr)) lbl->setText(QString::number(v));
}

void ImageFiltersDialog::loadHslSliders() {
    setSliderValue(sliderHslHue, qRound(m_hsl[m_hslChannel].hue));
    setSliderValue(sliderHslSat, qRound(m_hsl[m_hslChannel].saturation));
    setSliderValue(sliderHslLum, qRound(m_hsl[m_hslChannel].lightness));
    if (rangeBar) rangeBar->setChannel(m_hslChannel);
}

void ImageFiltersDialog::setParams(const FilterParams &fp) {
    m_applyingPreset = true;
    const QList<QWidget*> ws = allParamWidgets();
    for (QWidget *w : ws) if (w) w->blockSignals(true);
    if (curvEditor) curvEditor->blockSignals(true);

    brightness  = fp.brightness;   contrast      = fp.contrast;
    saturation  = fp.saturation;   exposure      = fp.exposure;
    shadows     = fp.shadows;      highlights    = fp.highlights;
    temperature = fp.temperature;  vignette      = fp.vignette;
    shadowColor = fp.shadowColor;  highlightColor= fp.highlightColor;
    invertColors= fp.invertColors; grayscale     = fp.grayscale;  sepia = fp.sepia;
    blur        = fp.blur;         blurRadius    = fp.blurRadius;
    sharpen     = fp.sharpen;      sharpenStrength = fp.sharpenStrength;
    pixelate    = fp.pixelate;     pixelateSize  = fp.pixelateSize;
    halftone    = fp.halftone;     halftoneCell  = fp.halftoneCell;
    rawTemp     = fp.rawTemp;      rawTint       = fp.rawTint;
    rawVibrance = fp.rawVibrance;  rawClarity    = fp.rawClarity;
    rawBlacks   = fp.rawBlacks;    rawWhites     = fp.rawWhites;
    rawGamma    = fp.rawGamma;
    colorize         = fp.colorize;
    colorizeStrength = fp.colorizeStrength;
    for (int i = 0; i < 7; ++i) m_hsl[i] = fp.hsl[i];
    colorizeHSL = fp.colorizeHSL;

    setSliderValue(sliderBrightness,  qRound(brightness  * 100));
    setSliderValue(sliderContrast,    qRound(contrast    * 100));
    setSliderValue(sliderSaturation,  qRound(saturation  * 100));
    setSliderValue(sliderExposure,    qRound(exposure    * 100));
    setSliderValue(sliderShadows,     qRound(shadows     * 100));
    setSliderValue(sliderHighlights,  qRound(highlights  * 100));
    setSliderValue(sliderTemperature, qRound(temperature * 100));
    setSliderValue(sliderVignette,    qRound(vignette    * 100));
    styleColorButton(btnShadowColor,    shadowColor);
    styleColorButton(btnHighlightColor, highlightColor);

    if (chkInvertColors) chkInvertColors->setChecked(invertColors);
    if (chkGrayscale)    chkGrayscale->setChecked(grayscale);
    if (chkSepia)        chkSepia->setChecked(sepia);
    if (chkBlur)          chkBlur->setChecked(blur);
    setSliderValue(sliderBlurRadius, blurRadius);
    if (lblBlurRadius)    lblBlurRadius->setText(tr("Radio: %1").arg(blurRadius));
    if (chkSharpen)       chkSharpen->setChecked(sharpen);
    setSliderValue(sliderSharpenStrength, sharpenStrength);
    if (lblSharpenStrength) lblSharpenStrength->setText(tr("Fuerza: %1").arg(sharpenStrength));
    if (chkPixelate)        chkPixelate->setChecked(pixelate);
    setSliderValue(sliderPixelateSize, pixelateSize);
    if (lblPixelateSize)    lblPixelateSize->setText(tr("Bloque: %1 px").arg(pixelateSize));
    if (chkHalftone)        chkHalftone->setChecked(halftone);
    setSliderValue(sliderHalftoneCell, halftoneCell);
    if (lblHalftoneCell)    lblHalftoneCell->setText(tr("Celda: %1 px").arg(halftoneCell));
    if (chkColorize)        chkColorize->setChecked(colorize);
    setSliderValue(sliderColorizeStrength, qRound(colorizeStrength * 100));
    if (lblColorizeStrength) lblColorizeStrength->setText(QString("%1%").arg(qRound(colorizeStrength * 100)));
    setSliderValue(sliderRawTint,     qRound(rawTint));
    setSliderValue(sliderRawVibrance, qRound(rawVibrance));
    if (chkColorizeHSL) chkColorizeHSL->setChecked(colorizeHSL);
    loadHslSliders();
    if (curvEditor) curvEditor->setAllPoints(fp.curves);

    for (QWidget *w : ws) if (w) w->blockSignals(false);
    if (curvEditor) curvEditor->blockSignals(false);
    m_applyingPreset = false;
    applyFilters();
}

FilterParams ImageFiltersDialog::presetParams(int id) {
    FilterParams fp;
    switch (id) {
    case PresetMMADRO:
        fp.brightness = 0.05; fp.contrast = 0.18; fp.saturation = 0.30; fp.exposure = 0.06;
        fp.shadows = 0.18; fp.highlights = 0.12; fp.temperature = 0.30; fp.vignette = 0.18;
        fp.shadowColor    = QColor(36, 58, 88);
        fp.highlightColor = QColor(255, 214, 130);
        fp.rawTemp = 28; fp.rawTint = -18; fp.rawVibrance = 50; fp.rawClarity = 12;
        fp.rawBlacks = -6; fp.rawWhites = 10; fp.rawGamma = 104;
        fp.sharpen = true; fp.sharpenStrength = 35;
        fp.hsl[1].hue = -8;  fp.hsl[1].saturation = 18;
        fp.hsl[3].hue = 6;   fp.hsl[3].saturation = -22;
        fp.hsl[5].saturation = 10;
        fp.curves[0] = QVector<QPointF>{ QPointF(0,4), QPointF(64,72), QPointF(192,206), QPointF(255,251) };
        fp.curves[1] = QVector<QPointF>{ QPointF(0,8), QPointF(128,142), QPointF(255,255) };
        fp.curves[2] = QVector<QPointF>{ QPointF(0,6), QPointF(128,138), QPointF(255,250) };
        fp.curves[3] = QVector<QPointF>{ QPointF(0,14), QPointF(128,108), QPointF(255,222) };
        break;
    case PresetMordor:
        fp.brightness = -0.04; fp.contrast = 0.32; fp.saturation = -0.18; fp.exposure = 0.06;
        fp.shadows = 0.22; fp.highlights = -0.12; fp.temperature = 0.42; fp.vignette = 0.38;
        fp.shadowColor   = QColor(70, 32, 8);
        fp.highlightColor= QColor(255, 186, 110);
        fp.rawTemp = 38; fp.rawTint = 10; fp.rawVibrance = 32; fp.rawClarity = 30;
        fp.rawBlacks = -18; fp.rawWhites = 12; fp.rawGamma = 94;
        fp.sharpen = true; fp.sharpenStrength = 45;
        fp.curves[0] = QVector<QPointF>{ QPointF(0,0), QPointF(64,46), QPointF(192,216), QPointF(255,255) };
        fp.curves[1] = QVector<QPointF>{ QPointF(0,8), QPointF(128,148), QPointF(255,255) };
        fp.curves[2] = QVector<QPointF>{ QPointF(0,2), QPointF(128,122), QPointF(255,246) };
        fp.curves[3] = QVector<QPointF>{ QPointF(0,0), QPointF(64,34), QPointF(128,86), QPointF(192,152), QPointF(255,214) };
        break;
    case PresetPop:
        fp.halftone = true; fp.halftoneCell = 6;
        fp.brightness = 0.06; fp.contrast = 0.34; fp.saturation = 0.45; fp.exposure = 0.04;
        fp.shadows = 0.10; fp.highlights = 0.10; fp.temperature = 0.10; fp.vignette = 0.12;
        fp.shadowColor    = QColor(60, 20, 10);
        fp.highlightColor = QColor(255, 250, 230);
        fp.rawTemp = 12; fp.rawTint = 8; fp.rawVibrance = 60; fp.rawClarity = 30;
        fp.rawBlacks = -12; fp.rawWhites = 14; fp.rawGamma = 96;
        fp.curves[0] = QVector<QPointF>{ QPointF(0,0), QPointF(70,44), QPointF(185,222), QPointF(255,255) };
        fp.curves[1] = QVector<QPointF>{ QPointF(0,6), QPointF(128,140), QPointF(255,255) };
        break;
    case PresetY2K:
        fp.brightness = 0.08; fp.contrast = 0.20; fp.saturation = 0.34; fp.exposure = 0.10;
        fp.shadows = 0.18; fp.highlights = 0.22; fp.temperature = -0.14; fp.vignette = 0.10;
        fp.shadowColor   = QColor(255, 64, 190);
        fp.highlightColor= QColor(130, 240, 255);
        fp.rawTemp = -14; fp.rawTint = 20; fp.rawVibrance = 58; fp.rawClarity = 16;
        fp.rawBlacks = 10; fp.rawWhites = 20; fp.rawGamma = 116;
        fp.sharpen = true; fp.sharpenStrength = 62;
        fp.curves[0] = QVector<QPointF>{ QPointF(0,10), QPointF(64,78), QPointF(192,208), QPointF(255,252) };
        fp.curves[1] = QVector<QPointF>{ QPointF(0,18), QPointF(128,138), QPointF(255,248) };
        fp.curves[2] = QVector<QPointF>{ QPointF(0,8),  QPointF(128,132), QPointF(255,255) };
        fp.curves[3] = QVector<QPointF>{ QPointF(0,26), QPointF(128,142), QPointF(255,250) };
        break;
    case PresetChernobil:
        fp.brightness = -0.08; fp.contrast = 0.30; fp.saturation = -0.55; fp.exposure = -0.04;
        fp.shadows = 0.28; fp.highlights = -0.22; fp.temperature = -0.22; fp.vignette = 0.55;
        fp.shadowColor   = QColor(18, 28, 10);
        fp.highlightColor= QColor(178, 214, 110);
        fp.rawTemp = -26; fp.rawTint = -32; fp.rawVibrance = 8; fp.rawClarity = 46;
        fp.rawBlacks = -26; fp.rawWhites = -14; fp.rawGamma = 86;
        fp.sharpen = true; fp.sharpenStrength = 72;
        fp.curves[0] = QVector<QPointF>{ QPointF(0,0), QPointF(64,50), QPointF(128,118), QPointF(192,176), QPointF(255,232) };
        fp.curves[1] = QVector<QPointF>{ QPointF(0,0), QPointF(128,108), QPointF(255,222) };
        fp.curves[2] = QVector<QPointF>{ QPointF(0,14), QPointF(128,144), QPointF(255,242) };
        fp.curves[3] = QVector<QPointF>{ QPointF(0,0), QPointF(128,102), QPointF(255,206) };
        break;
    case PresetPixelArt:
        fp.pixelate = true; fp.pixelateSize = 10;
        fp.brightness = 0.02; fp.contrast = 0.22; fp.saturation = 0.30; fp.exposure = 0.0;
        fp.rawVibrance = 45; fp.rawClarity = 25;
        fp.rawBlacks = -8; fp.rawWhites = 8; fp.rawGamma = 104;
        fp.sharpen = true; fp.sharpenStrength = 60;
        fp.curves[0] = QVector<QPointF>{
            QPointF(0,20),   QPointF(32,20),  QPointF(43,66),
            QPointF(75,66),  QPointF(86,112), QPointF(118,112),
            QPointF(129,158),QPointF(161,158),QPointF(172,204),
            QPointF(204,204),QPointF(215,250),QPointF(255,250)
        };
        break;
    case PresetVaporwave:
        fp.brightness = 0.05; fp.contrast = 0.22; fp.saturation = 0.45; fp.exposure = 0.06;
        fp.shadows = 0.25; fp.highlights = 0.18; fp.temperature = -0.18; fp.vignette = 0.22;
        fp.shadowColor   = QColor(120, 30, 200);
        fp.highlightColor= QColor(255, 120, 220);
        fp.rawTemp = -18; fp.rawTint = 34; fp.rawVibrance = 62; fp.rawClarity = 12;
        fp.rawBlacks = 6; fp.rawWhites = 16; fp.rawGamma = 118;
        fp.sharpen = true; fp.sharpenStrength = 40;
        fp.hsl[4].saturation = 25;
        fp.hsl[6].hue = 10; fp.hsl[6].saturation = 20;
        fp.curves[0] = QVector<QPointF>{ QPointF(0,6),  QPointF(128,136), QPointF(255,250) };
        fp.curves[1] = QVector<QPointF>{ QPointF(0,14), QPointF(128,142), QPointF(255,252) };
        fp.curves[2] = QVector<QPointF>{ QPointF(0,4),  QPointF(128,124), QPointF(255,250) };
        fp.curves[3] = QVector<QPointF>{ QPointF(0,24), QPointF(128,146), QPointF(255,248) };
        break;
    case PresetNoir:
        fp.grayscale = true;
        fp.brightness = -0.04; fp.contrast = 0.46; fp.saturation = -1.0; fp.exposure = 0.0;
        fp.shadows = 0.15; fp.highlights = -0.15; fp.vignette = 0.48;
        fp.rawClarity = 42; fp.rawBlacks = -32; fp.rawWhites = 22; fp.rawGamma = 92;
        fp.sharpen = true; fp.sharpenStrength = 58;
        fp.curves[0] = QVector<QPointF>{ QPointF(0,0), QPointF(48,26), QPointF(128,124), QPointF(208,224), QPointF(255,255) };
        break;
    case PresetKodak80:
        fp.brightness = 0.06; fp.contrast = 0.14; fp.saturation = 0.20; fp.exposure = 0.05;
        fp.shadows = 0.20; fp.highlights = 0.10; fp.temperature = 0.26; fp.vignette = 0.20;
        fp.shadowColor   = QColor(90, 60, 30);
        fp.highlightColor= QColor(255, 226, 170);
        fp.rawTemp = 26; fp.rawTint = 8; fp.rawVibrance = 28; fp.rawClarity = 10;
        fp.rawBlacks = 12; fp.rawWhites = 8; fp.rawGamma = 112;
        fp.blur = true; fp.blurRadius = 2;
        fp.sharpen = true; fp.sharpenStrength = 30;
        fp.curves[0] = QVector<QPointF>{ QPointF(0,12), QPointF(128,138), QPointF(255,248) };
        fp.curves[1] = QVector<QPointF>{ QPointF(0,14), QPointF(128,140), QPointF(255,250) };
        fp.curves[2] = QVector<QPointF>{ QPointF(0,10), QPointF(128,132), QPointF(255,248) };
        fp.curves[3] = QVector<QPointF>{ QPointF(0,6),  QPointF(128,124), QPointF(255,242) };
        break;
    case PresetArctic:
        fp.brightness = 0.10; fp.contrast = 0.18; fp.saturation = -0.12; fp.exposure = 0.06;
        fp.shadows = 0.10; fp.highlights = 0.15; fp.temperature = -0.55; fp.vignette = 0.14;
        fp.shadowColor   = QColor(20, 50, 90);
        fp.highlightColor= QColor(220, 245, 255);
        fp.rawTemp = -52; fp.rawTint = -8; fp.rawVibrance = 18; fp.rawClarity = 26;
        fp.rawBlacks = 4; fp.rawWhites = 18; fp.rawGamma = 110;
        fp.sharpen = true; fp.sharpenStrength = 45;
        fp.hsl[5].hue = -6; fp.hsl[5].saturation = 15;
        fp.curves[0] = QVector<QPointF>{ QPointF(0,4),  QPointF(128,132), QPointF(255,252) };
        fp.curves[3] = QVector<QPointF>{ QPointF(0,16), QPointF(128,146), QPointF(255,255) };
        break;
    case PresetPastel:
        fp.brightness = 0.14; fp.contrast = -0.22; fp.saturation = -0.15; fp.exposure = 0.10;
        fp.shadows = 0.30; fp.highlights = 0.20; fp.temperature = 0.05; fp.vignette = 0.0;
        fp.shadowColor   = QColor(210, 190, 235);
        fp.highlightColor= QColor(255, 245, 240);
        fp.rawTemp = 6; fp.rawTint = 12; fp.rawVibrance = 55; fp.rawClarity = -25;
        fp.rawBlacks = 25; fp.rawWhites = 10; fp.rawGamma = 125;
        fp.blur = true; fp.blurRadius = 1;
        fp.curves[0] = QVector<QPointF>{ QPointF(0,32), QPointF(64,96), QPointF(192,222), QPointF(255,250) };
        fp.curves[1] = QVector<QPointF>{ QPointF(0,34), QPointF(128,140), QPointF(255,250) };
        fp.curves[2] = QVector<QPointF>{ QPointF(0,30), QPointF(128,136), QPointF(255,248) };
        fp.curves[3] = QVector<QPointF>{ QPointF(0,38), QPointF(128,146), QPointF(255,252) };
        break;
    case PresetNeon:
        fp.brightness = -0.14; fp.contrast = 0.42; fp.saturation = 0.55; fp.exposure = -0.05;
        fp.shadows = 0.18; fp.highlights = -0.10; fp.temperature = -0.12; fp.vignette = 0.52;
        fp.shadowColor   = QColor(20, 10, 70);
        fp.highlightColor= QColor(255, 60, 200);
        fp.rawTemp = -10; fp.rawTint = 26; fp.rawVibrance = 70; fp.rawClarity = 40;
        fp.rawBlacks = -34; fp.rawWhites = 16; fp.rawGamma = 92;
        fp.sharpen = true; fp.sharpenStrength = 66;
        fp.hsl[6].saturation = 25;
        fp.hsl[4].saturation = 20;
        fp.curves[0] = QVector<QPointF>{ QPointF(0,0), QPointF(64,40), QPointF(192,226), QPointF(255,255) };
        fp.curves[1] = QVector<QPointF>{ QPointF(0,6), QPointF(128,134), QPointF(255,255) };
        fp.curves[3] = QVector<QPointF>{ QPointF(0,18), QPointF(128,140), QPointF(255,252) };
        break;
    case PresetNoMexico:
        fp.brightness = 0.04; fp.contrast = 0.26; fp.saturation = 0.35; fp.exposure = 0.05;
        fp.temperature = 0.55; fp.vignette = 0.18;
        fp.shadowColor   = QColor(40, 20, 60);
        fp.highlightColor= QColor(255, 240, 220);
        fp.rawTemp = 55; fp.rawTint = 20; fp.rawVibrance = 40; fp.rawClarity = 25;
        fp.rawBlacks = -10; fp.rawWhites = 15; fp.rawGamma = 100;
        fp.sharpen = true; fp.sharpenStrength = 50;
        fp.hsl[3].hue = -65; fp.hsl[3].saturation = 30; fp.hsl[3].lightness = 10;
        fp.curves[1] = QVector<QPointF>{ QPointF(0,22), QPointF(64,110), QPointF(128,186), QPointF(255,255) };
        fp.curves[2] = QVector<QPointF>{ QPointF(0,10), QPointF(128,150), QPointF(255,244) };
        fp.curves[3] = QVector<QPointF>{ QPointF(0,0),  QPointF(128,58),  QPointF(255,138) };
        break;
    case PresetNone:
    default:
        return FilterParams();
    }
    return fp;
}

FilterParams ImageFiltersDialog::blendPreset(const FilterParams &b, double k) {
    k = qBound(0.0, k, 1.0);
    if (k >= 0.9999) return b;
    if (k <= 0.0001) return FilterParams();
    FilterParams fp;
    fp.brightness  = b.brightness  * k;
    fp.contrast    = b.contrast    * k;
    fp.saturation  = b.saturation  * k;
    fp.exposure    = b.exposure    * k;
    fp.shadows     = b.shadows     * k;
    fp.highlights  = b.highlights  * k;
    fp.temperature = b.temperature * k;
    fp.vignette    = b.vignette    * k;
    fp.shadowColor    = b.shadowColor;
    fp.highlightColor = b.highlightColor;
    fp.invertColors = b.invertColors && (k > 0.50);
    fp.grayscale    = b.grayscale    && (k > 0.35);
    fp.sepia        = b.sepia        && (k > 0.35);
    fp.blur         = b.blur         && (k > 0.05);
    fp.blurRadius   = qBound(1, qRound(b.blurRadius * k), 20);
    fp.sharpen      = b.sharpen      && (k > 0.05);
    fp.sharpenStrength = qBound(10, qRound(b.sharpenStrength * k), 100);
    fp.pixelate     = b.pixelate     && (k > 0.05);
    fp.pixelateSize = qBound(2, qRound(b.pixelateSize * k), 64);
    fp.halftone     = b.halftone     && (k > 0.05);
    fp.halftoneCell = qBound(2, qRound(b.halftoneCell * k), 40);
    fp.colorize         = b.colorize && (k > 0.05);
    fp.colorizeStrength = b.colorizeStrength * k;
    fp.rawTemp     = b.rawTemp     * k;
    fp.rawTint     = b.rawTint     * k;
    fp.rawVibrance = b.rawVibrance * k;
    fp.rawClarity  = b.rawClarity  * k;
    fp.rawBlacks   = b.rawBlacks   * k;
    fp.rawWhites   = b.rawWhites   * k;
    fp.rawGamma    = 100.0 + (b.rawGamma - 100.0) * k;
    for (int i = 0; i < 7; ++i) {
        fp.hsl[i].hue        = b.hsl[i].hue        * k;
        fp.hsl[i].saturation = b.hsl[i].saturation * k;
        fp.hsl[i].lightness  = b.hsl[i].lightness  * k;
    }
    fp.colorizeHSL = b.colorizeHSL && (k > 0.50);
    for (int i = 0; i < 4; ++i) {
        QVector<QPointF> out;
        const QVector<QPointF> &src = b.curves[i];
        if (src.isEmpty()) continue;
        for (const QPointF &p : src)
            out << QPointF(p.x(), p.x() + (p.y() - p.x()) * k);
        fp.curves[i] = out;
    }
    return fp;
}

void ImageFiltersDialog::resetFilters() {
    m_applyingPreset = true;
    if (sliderPresetStrength) {
        sliderPresetStrength->blockSignals(true);
        sliderPresetStrength->setValue(50);
        sliderPresetStrength->blockSignals(false);
    }
    if (comboPresets) {
        comboPresets->blockSignals(true);
        comboPresets->setCurrentIndex(0);
        comboPresets->blockSignals(false);
    }
    m_currentPresetId = PresetNone;
    m_applyingPreset = false;
    setParams(FilterParams());
}

FilterParams ImageFiltersDialog::getParams() const {
    FilterParams fp;
    fp.brightness = brightness; fp.contrast = contrast;
    fp.saturation = saturation; fp.exposure = exposure;
    fp.shadows = shadows; fp.highlights = highlights;
    fp.temperature = temperature; fp.vignette = vignette;
    fp.shadowColor = shadowColor; fp.highlightColor = highlightColor;
    fp.invertColors = invertColors; fp.grayscale = grayscale; fp.sepia = sepia;
    fp.blur = blur; fp.blurRadius = blurRadius;
    fp.sharpen = sharpen; fp.sharpenStrength = sharpenStrength;
    fp.pixelate = pixelate; fp.pixelateSize = pixelateSize;
    fp.halftone = halftone; fp.halftoneCell = halftoneCell;
    fp.rawTemp = rawTemp; fp.rawTint = rawTint;
    fp.rawVibrance = rawVibrance; fp.rawClarity = rawClarity;
    fp.rawBlacks = rawBlacks; fp.rawWhites = rawWhites;
    fp.rawGamma = rawGamma;
    fp.colorize         = colorize;
    fp.colorizeStrength = colorizeStrength;
    for (int i = 0; i < 7; ++i) fp.hsl[i] = m_hsl[i];
    fp.colorizeHSL = colorizeHSL;
    for (int i = 0; i < 4; ++i) fp.curves[i] = curvEditor->getPoints(i);
    return fp;
}

QImage ImageFiltersDialog::getFilteredImage() {
    QImage full = originalImage.convertToFormat(QImage::Format_ARGB32);
    applyPipeline(full);
    return full;
}

void ImageFiltersDialog::buildColorizeLUT(int lutR[256], int lutG[256], int lutB[256]) {
    struct Stop { int pos; double r, g, b; };
    static const Stop stops[] = {
        {   0,   5.0,   5.0,  15.0 },
        {  40,  20.0,  45.0,  75.0 },
        {  85,  95.0,  70.0,  60.0 },
        { 130, 190.0, 125.0,  80.0 },
        { 180, 230.0, 190.0, 140.0 },
        { 220, 245.0, 235.0, 210.0 },
        { 255, 255.0, 255.0, 255.0 }
    };
    const int n = (int)(sizeof(stops) / sizeof(stops[0]));
    for (int l = 0; l < 256; ++l) {
        int i = 0;
        while (i < n - 2 && l > stops[i + 1].pos) ++i;
        double t = (double)(l - stops[i].pos) / qMax(1, stops[i + 1].pos - stops[i].pos);
        t = qBound(0.0, t, 1.0);
        t = t * t * (3.0 - 2.0 * t);
        double cr = stops[i].r + (stops[i + 1].r - stops[i].r) * t;
        double cg = stops[i].g + (stops[i + 1].g - stops[i].g) * t;
        double cb = stops[i].b + (stops[i + 1].b - stops[i].b) * t;
        lutR[l] = qBound(0, (int)(cr + 0.5), 255);
        lutG[l] = qBound(0, (int)(cg + 0.5), 255);
        lutB[l] = qBound(0, (int)(cb + 0.5), 255);
    }
}

// ============================================================
// Pipeline de filtros
// ============================================================

void ImageFiltersDialog::applyFilterParams(QImage &img, const FilterParams &fp,
    const QPoint &subOffset, const QSize &fullSize)
{
    if (img.isNull()) return;
    if (img.format() != QImage::Format_ARGB32 && img.format() != QImage::Format_RGB32)
        img = img.convertToFormat(QImage::Format_ARGB32);
    int w = img.width(), h = img.height();

    bool doColor =
        fp.brightness != 0 || fp.contrast != 0 || fp.saturation != 0 || fp.exposure != 0 ||
        fp.shadows != 0 || fp.highlights != 0 || fp.temperature != 0 ||
        fp.invertColors || fp.grayscale || fp.sepia ||
        fp.rawTemp != 0 || fp.rawTint != 0 || fp.rawVibrance != 0 || fp.rawClarity != 0 ||
        fp.rawBlacks != 0 || fp.rawWhites != 0 || fp.rawGamma != 100;

    if (doColor) {
        double expFactor = pow(2.0, fp.exposure);
        double contrastFactor = (fp.contrast != 0)
            ? (259.0 * (fp.contrast * 255 + 255)) / (255.0 * (259.0 - fp.contrast * 255))
            : 1.0;
        double gammaExp = (fp.rawGamma != 100) ? 1.0 / (fp.rawGamma / 100.0) : 1.0;

        for (int y = 0; y < h; ++y) {
            QRgb *line = (QRgb*)img.scanLine(y);
            for (int x = 0; x < w; ++x) {
                QRgb px = line[x];
                int a = qAlpha(px);
                if (a == 0) continue;
                double r = qRed(px), g = qGreen(px), b = qBlue(px);

                if (fp.exposure != 0) { r *= expFactor; g *= expFactor; b *= expFactor; }
                if (fp.brightness != 0) { double add = fp.brightness * 255; r += add; g += add; b += add; }
                if (fp.contrast != 0) {
                    r = contrastFactor * (r - 128) + 128;
                    g = contrastFactor * (g - 128) + 128;
                    b = contrastFactor * (b - 128) + 128;
                }
                if (fp.temperature != 0) {
                    if (fp.temperature > 0) { r += fp.temperature * 50; b -= fp.temperature * 50; }
                    else { b += -fp.temperature * 50; r -= -fp.temperature * 50; }
                }
                if (fp.rawTemp != 0) { r += fp.rawTemp * 0.8; b -= fp.rawTemp * 0.8; }
                if (fp.rawTint != 0) { g -= fp.rawTint * 0.6; }
                if (fp.rawBlacks != 0) {
                    double f = fp.rawBlacks * 0.5;
                    r += f * (1.0 - r / 255.0); g += f * (1.0 - g / 255.0); b += f * (1.0 - b / 255.0);
                }
                if (fp.rawWhites != 0) {
                    double f = fp.rawWhites * 0.5;
                    r += f * (r / 255.0); g += f * (g / 255.0); b += f * (b / 255.0);
                }
                if (fp.rawGamma != 100) {
                    r = 255.0 * pow(qMax(0.0, r / 255.0), gammaExp);
                    g = 255.0 * pow(qMax(0.0, g / 255.0), gammaExp);
                    b = 255.0 * pow(qMax(0.0, b / 255.0), gammaExp);
                }
                if (fp.rawClarity != 0) {
                    double k = 1.0 + fp.rawClarity / 100.0 * 0.8;
                    r = 128.0 + (r - 128.0) * k; g = 128.0 + (g - 128.0) * k; b = 128.0 + (b - 128.0) * k;
                }
                if (fp.rawVibrance != 0) {
                    int lum = safeLum(r, g, b);
                    int mx = qMax((int)r, qMax((int)g, (int)b));
                    int mn = qMin((int)r, qMin((int)g, (int)b));
                    double sat = (mx - mn) / 255.0;
                    double boost = fp.rawVibrance / 100.0 * (1.0 - sat);
                    r += (r - lum) * boost; g += (g - lum) * boost; b += (b - lum) * boost;
                }
                if (fp.saturation != 0) {
                    int lum = safeLum(r, g, b);
                    r = lum + (r - lum) * (1.0 + fp.saturation);
                    g = lum + (g - lum) * (1.0 + fp.saturation);
                    b = lum + (b - lum) * (1.0 + fp.saturation);
                }
                if (fp.shadows != 0) {
                    int lum = safeLum(r, g, b);
                    if (lum < 128) {
                        double f = 1.0 + fp.shadows * (1.0 - lum / 128.0);
                        r = r * f + fp.shadowColor.red()   * fp.shadows * 0.3;
                        g = g * f + fp.shadowColor.green() * fp.shadows * 0.3;
                        b = b * f + fp.shadowColor.blue()  * fp.shadows * 0.3;
                    }
                }
                if (fp.highlights != 0) {
                    int lum = safeLum(r, g, b);
                    if (lum > 128) {
                        double f = 1.0 + fp.highlights * (lum / 128.0 - 1.0);
                        r = r * f + fp.highlightColor.red()   * fp.highlights * 0.3;
                        g = g * f + fp.highlightColor.green() * fp.highlights * 0.3;
                        b = b * f + fp.highlightColor.blue()  * fp.highlights * 0.3;
                    }
                }
                if (fp.grayscale) { int lum = safeLum(r, g, b); r = g = b = lum; }
                if (fp.sepia) {
                    double nr = r * 0.393 + g * 0.769 + b * 0.189;
                    double ng = r * 0.349 + g * 0.686 + b * 0.168;
                    double nb = r * 0.272 + g * 0.534 + b * 0.131;
                    r = nr; g = ng; b = nb;
                }
                if (fp.invertColors) { r = 255 - r; g = 255 - g; b = 255 - b; }
                line[x] = clampRgba(r, g, b, a);
            }
        }
    }

    if (fp.colorize && fp.colorizeStrength > 0.001) {
        int lutR[256], lutG[256], lutB[256];
        buildColorizeLUT(lutR, lutG, lutB);
        double k = qBound(0.0, fp.colorizeStrength, 1.0);
        for (int y = 0; y < h; ++y) {
            QRgb *line = (QRgb*)img.scanLine(y);
            for (int x = 0; x < w; ++x) {
                QRgb px = line[x];
                int a = qAlpha(px);
                if (a == 0) continue;
                int lum = qGray(qRed(px), qGreen(px), qBlue(px));
                double nr = qRed(px)   * (1.0 - k) + lutR[lum] * k;
                double ng = qGreen(px) * (1.0 - k) + lutG[lum] * k;
                double nb = qBlue(px)  * (1.0 - k) + lutB[lum] * k;
                line[x] = clampRgba(nr, ng, nb, a);
            }
        }
    }

    bool hslActive = fp.colorizeHSL;
    for (int i = 0; i < 7 && !hslActive; ++i)
        if (fp.hsl[i].hue != 0 || fp.hsl[i].saturation != 0 || fp.hsl[i].lightness != 0) hslActive = true;

    if (hslActive) {
        for (int y = 0; y < h; ++y) {
            QRgb *line = (QRgb*)img.scanLine(y);
            for (int x = 0; x < w; ++x) {
                int a = qAlpha(line[x]);
                if (a == 0) continue;
                double r = qRed(line[x]), g = qGreen(line[x]), b = qBlue(line[x]);
                double hh, ss, ll;
                rgb2hsl(r, g, b, hh, ss, ll);
                if (fp.colorizeHSL) {
                    hh = fmod(fp.hsl[0].hue + 180.0, 360.0);
                    ss = qBound(0.0, (fp.hsl[0].saturation + 100.0) / 200.0, 1.0);
                } else {
                    double dH = fp.hsl[0].hue;
                    double fS = fp.hsl[0].saturation / 100.0;
                    double fL = fp.hsl[0].lightness  / 100.0;
                    for (int c = 1; c <= 6; ++c) {
                        const HSLAdjust &ch = fp.hsl[c];
                        if (ch.hue == 0 && ch.saturation == 0 && ch.lightness == 0) continue;
                        double wgt = hslChannelWeight(hh, c);
                        if (wgt <= 0.001) continue;
                        dH += wgt * ch.hue;
                        fS += wgt * ch.saturation / 100.0;
                        fL += wgt * ch.lightness  / 100.0;
                    }
                    hh += dH;
                    ss  = qBound(0.0, ss * (1.0 + fS), 1.0);
                    ll  = qBound(0.0, ll + fL * 0.5, 1.0);
                }
                hsl2rgb(hh, ss, ll, r, g, b);
                line[x] = clampRgba(r, g, b, a);
            }
        }
    }

    uchar lutM[256], lutR[256], lutG[256], lutB[256];
    const QVector<QPointF> &pM = fp.curves[0];
    const QVector<QPointF> &pR = fp.curves[1];
    const QVector<QPointF> &pG = fp.curves[2];
    const QVector<QPointF> &pB = fp.curves[3];
    bool curvesActive = !CurvEditor::isIdentity(pM) || !CurvEditor::isIdentity(pR) ||
                        !CurvEditor::isIdentity(pG) || !CurvEditor::isIdentity(pB);
    if (curvesActive) {
        CurvEditor::buildLUT(pM, lutM);
        CurvEditor::buildLUT(pR, lutR);
        CurvEditor::buildLUT(pG, lutG);
        CurvEditor::buildLUT(pB, lutB);
        for (int y = 0; y < h; ++y) {
            QRgb *line = (QRgb*)img.scanLine(y);
            for (int x = 0; x < w; ++x) {
                QRgb px = line[x];
                int a = qAlpha(px);
                if (a == 0) continue;
                int r = lutR[lutM[qRed(px)]];
                int g = lutG[lutM[qGreen(px)]];
                int b = lutB[lutM[qBlue(px)]];
                line[x] = qRgba(r, g, b, a);
            }
        }
    }

    if (fp.pixelate && fp.pixelateSize > 1) pixelateImage(img, fp.pixelateSize, subOffset);
    if (fp.halftone && fp.halftoneCell > 1) halftoneImage(img, fp.halftoneCell, subOffset);
    if (fp.blur && fp.blurRadius > 0) boxBlur(img, fp.blurRadius);

    if (fp.sharpen) {
        QImage tmp = img.copy();
        double strength = fp.sharpenStrength / 50.0;
        for (int y = 1; y < h - 1; ++y) {
            const QRgb *lC = (const QRgb*)tmp.constScanLine(y);
            const QRgb *lT = (const QRgb*)tmp.constScanLine(y - 1);
            const QRgb *lB = (const QRgb*)tmp.constScanLine(y + 1);
            QRgb *out = (QRgb*)img.scanLine(y);
            for (int x = 1; x < w - 1; ++x) {
                QRgb c = lC[x], t = lT[x], b2 = lB[x], l = lC[x - 1], rr = lC[x + 1];
                int r = qBound(0, (int)(qRed(c)   * (1 + 4*strength) - (qRed(t)   + qRed(b2)   + qRed(l)   + qRed(rr))   * strength), 255);
                int g = qBound(0, (int)(qGreen(c) * (1 + 4*strength) - (qGreen(t) + qGreen(b2) + qGreen(l) + qGreen(rr)) * strength), 255);
                int b = qBound(0, (int)(qBlue(c)  * (1 + 4*strength) - (qBlue(t)  + qBlue(b2)  + qBlue(l)  + qBlue(rr))  * strength), 255);
                out[x] = qRgba(r, g, b, qAlpha(c));
            }
        }
    }

    if (fp.vignette > 0) {
        double fsW = fullSize.isValid() ? fullSize.width()  : img.width();
        double fsH = fullSize.isValid() ? fullSize.height() : img.height();
        double cx = fsW / 2.0 - subOffset.x();
        double cy = fsH / 2.0 - subOffset.y();
        double maxDist = sqrt((fsW/2.0)*(fsW/2.0) + (fsH/2.0)*(fsH/2.0));
        if (maxDist < 0.001) maxDist = 0.001;
        for (int y = 0; y < h; ++y) {
            QRgb *line = (QRgb*)img.scanLine(y);
            for (int x = 0; x < w; ++x) {
                double dist = sqrt((x - cx)*(x - cx) + (y - cy)*(y - cy));
                double f = qMax(0.0, 1.0 - fp.vignette * (dist / maxDist));
                QRgb px = line[x];
                line[x] = qRgba((int)(qRed(px) * f), (int)(qGreen(px) * f), (int)(qBlue(px) * f), qAlpha(px));
            }
        }
    }
}

// ============================================================
// Helpers de UI varios
// ============================================================

QGroupBox *ImageFiltersDialog::makeSliderGroup(const QString &title, const QString &desc,
    int min, int max, int initial, QSlider **outSlider, std::function<void(int)> onChanged)
{
    QGroupBox *box = new QGroupBox(title);
    QVBoxLayout *lay = new QVBoxLayout(box);
    lay->setSpacing(3);
    lay->addWidget(makeDescLabel(desc));
    QSlider *s = new QSlider(Qt::Horizontal);
    s->setRange(min, max); s->setValue(initial);
    s->setStyleSheet(sliderStyle());
    QLabel *lbl = new QLabel(QString::number(initial));
    lbl->setStyleSheet(labelStyle());
    lbl->setMinimumWidth(32);
    lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_sliderLabels.insert(s, lbl);
    connect(s, &QSlider::valueChanged, this, [lbl, onChanged](int v) {
        lbl->setText(QString::number(v));
        onChanged(v);
    });
    QHBoxLayout *row = new QHBoxLayout();
    row->addWidget(s, 1); row->addWidget(lbl);
    lay->addLayout(row);
    *outSlider = s;
    return box;
}

void ImageFiltersDialog::applyFilters() {
    if (lblPreview) {
        previewResult = workImage.copy();
        applyPipeline(previewResult);
        lblPreview->setPixmap(QPixmap::fromImage(previewResult));
    }
    emit paramsChanged();
}

void ImageFiltersDialog::applyPipeline(QImage &img) {
    applyFilterParams(img, getParams());
}

void ImageFiltersDialog::pixelateImage(QImage &img, int blockSize, const QPoint &subOffset) {
    if (img.isNull() || blockSize <= 1) return;
    int w = img.width(), h = img.height();
    if (w <= 0 || h <= 0) return;
    if (img.format() != QImage::Format_ARGB32 && img.format() != QImage::Format_RGB32)
        img = img.convertToFormat(QImage::Format_ARGB32);
    int startX = -(((subOffset.x() % blockSize) + blockSize) % blockSize);
    int startY = -(((subOffset.y() % blockSize) + blockSize) % blockSize);
    for (int by = startY; by < h; by += blockSize) {
        int y0 = qMax(0, by); int y1 = qMin(h, by + blockSize);
        if (y0 >= y1) continue;
        for (int bx = startX; bx < w; bx += blockSize) {
            int x0 = qMax(0, bx); int x1 = qMin(w, bx + blockSize);
            if (x0 >= x1) continue;
            long long sr = 0, sg = 0, sb = 0, sa = 0; int n = 0;
            for (int y = y0; y < y1; ++y) {
                const QRgb *src = (const QRgb*)img.constScanLine(y);
                for (int x = x0; x < x1; ++x) { sr += qRed(src[x]); sg += qGreen(src[x]); sb += qBlue(src[x]); sa += qAlpha(src[x]); ++n; }
            }
            if (n == 0) continue;
            QRgb avg = qRgba((int)(sr / n), (int)(sg / n), (int)(sb / n), (int)(sa / n));
            for (int y = y0; y < y1; ++y) {
                QRgb *dst = (QRgb*)img.scanLine(y);
                for (int x = x0; x < x1; ++x) dst[x] = avg;
            }
        }
    }
}

void ImageFiltersDialog::halftoneImage(QImage &img, int cell, const QPoint &subOffset) {
    if (img.isNull() || cell < 2) return;
    int w = img.width(), h = img.height();
    if (w <= 0 || h <= 0) return;
    if (img.format() != QImage::Format_ARGB32 && img.format() != QImage::Format_RGB32)
        img = img.convertToFormat(QImage::Format_ARGB32);
    const QImage src = img.copy();
    QImage &out = img;
    int startX = -(((subOffset.x() % cell) + cell) % cell);
    int startY = -(((subOffset.y() % cell) + cell) % cell);
    const double maxRadius = cell * 0.7071;
    for (int by = startY; by < h; by += cell) {
        int y0 = qMax(0, by); int y1 = qMin(h, by + cell);
        if (y0 >= y1) continue;
        for (int bx = startX; bx < w; bx += cell) {
            int x0 = qMax(0, bx); int x1 = qMin(w, bx + cell);
            if (x0 >= x1) continue;
            long long sr = 0, sg = 0, sb = 0; int n = 0;
            for (int y = y0; y < y1; ++y) {
                const QRgb *line = (const QRgb*)src.constScanLine(y);
                for (int x = x0; x < x1; ++x) {
                    if (qAlpha(line[x]) == 0) continue;
                    sr += qRed(line[x]); sg += qGreen(line[x]); sb += qBlue(line[x]); ++n;
                }
            }
            if (n == 0) continue;
            int ar = (int)(sr / n), ag = (int)(sg / n), ab = (int)(sb / n);
            int luma = qGray(ar, ag, ab);
            double dark = 1.0 - (luma / 255.0);
            dark = qBound(0.0, dark, 1.0);
            double radius = maxRadius * sqrt(dark);
            int dotR = qBound(0, (int)(ar * 0.82), 255);
            int dotG = qBound(0, (int)(ag * 0.82), 255);
            int dotB = qBound(0, (int)(ab * 0.82), 255);
            int papR = ar + (int)((255 - ar) * 0.88);
            int papG = ag + (int)((255 - ag) * 0.88);
            int papB = ab + (int)((255 - ab) * 0.88);
            double cx = bx + cell * 0.5;
            double cy = by + cell * 0.5;
            double r2 = radius * radius;
            for (int y = y0; y < y1; ++y) {
                const QRgb *srcLine = (const QRgb*)src.constScanLine(y);
                QRgb *dstLine = (QRgb*)out.scanLine(y);
                double dy = (y + 0.5) - cy;
                for (int x = x0; x < x1; ++x) {
                    int origA = qAlpha(srcLine[x]);
                    if (origA == 0) { dstLine[x] = qRgba(0, 0, 0, 0); continue; }
                    double dx = (x + 0.5) - cx;
                    if (dx*dx + dy*dy <= r2) dstLine[x] = qRgba(dotR, dotG, dotB, origA);
                    else                     dstLine[x] = qRgba(papR, papG, papB, origA);
                }
            }
        }
    }
}

void ImageFiltersDialog::boxBlur(QImage &img, int radius) {
    int w = img.width(), h = img.height();
    if (w <= 0 || h <= 0 || radius <= 0) return;
    QImage tmp(w, h, img.format());
    for (int y = 0; y < h; ++y) {
        const QRgb *src = (const QRgb*)img.constScanLine(y);
        QRgb *dst = (QRgb*)tmp.scanLine(y);
        long long r = 0, g = 0, b = 0;
        for (int i = -radius; i <= radius; ++i) {
            int xx = qBound(0, i, w - 1);
            r += qRed(src[xx]); g += qGreen(src[xx]); b += qBlue(src[xx]);
        }
        double div = radius * 2 + 1;
        for (int x = 0; x < w; ++x) {
            dst[x] = qRgba((int)(r / div), (int)(g / div), (int)(b / div), qAlpha(src[x]));
            int addX = qBound(0, x + radius + 1, w - 1);
            int subX = qBound(0, x - radius, w - 1);
            r += qRed(src[addX])   - qRed(src[subX]);
            g += qGreen(src[addX]) - qGreen(src[subX]);
            b += qBlue(src[addX])  - qBlue(src[subX]);
        }
    }
    img = tmp;
}

#include "moc_ImageFilters.cpp"