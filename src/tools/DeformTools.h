#ifndef DEFORM_TOOLS_H
#define DEFORM_TOOLS_H

#include <QImage>
#include <QPoint>
#include <QRect>
#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <Qt>

#include <cmath>
#include <algorithm>


// ============================================================
// Opciones
// ============================================================
struct DeformOptc {
    int mode = 0;       // Deformher::DeformMode
    int radius = 40;    // px
    int strength = 50;  // %
};


// ============================================================
// Motor de deformación sobre QImage
// ============================================================
namespace Deformher {

enum DeformMode {
    ModePush     = 0,   // Empujar (arrastre direccional)
    ModeTwirlCW  = 1,   // Girar horario
    ModeTwirlCCW = 2,   // Girar antihorario
    ModeInflate  = 3,   // Inflar
    ModePinch    = 4,   // Desinflar
    ModeWaves    = 5    // Ondas
};


template <typename MapFn>
inline void applyInCircle(QImage &img, const QPoint &center, int radius, int margin, MapFn mapSrc)
{
    if (img.isNull() || radius <= 0) return;
    if (img.format() != QImage::Format_ARGB32 && img.format() != QImage::Format_RGB32)
        img = img.convertToFormat(QImage::Format_ARGB32);

    QRect region(center.x() - radius - margin, center.y() - radius - margin,
                 (radius + margin) * 2 + 1, (radius + margin) * 2 + 1);
    region = region.intersected(img.rect());
    if (region.isEmpty()) return;

    const QImage src = img.copy(region);   // original ANTES de deformar
    const double r = radius;
    const double cx = center.x(), cy = center.y();

    for (int y = region.top(); y <= region.bottom(); ++y) {
        QRgb *line = (QRgb*)img.scanLine(y);
        for (int x = region.left(); x <= region.right(); ++x) {
            const double dx = x - cx, dy = y - cy;
            const double d = std::sqrt(dx * dx + dy * dy);
            if (d >= r) continue;

            double sx = x, sy = y;
            mapSrc((double)x, (double)y, dx, dy, d, sx, sy);

            if (sx < 0 || sy < 0 || sx > img.width() - 1 || sy > img.height() - 1) {
                line[x] = 0;
                continue;
            }

            double lx = sx - region.x();
            double ly = sy - region.y();
            if (lx < 0 || ly < 0 || lx > src.width() - 1 || ly > src.height() - 1) continue;

            int x0 = (int)std::floor(lx), y0 = (int)std::floor(ly);
            int x1 = std::min(x0 + 1, src.width() - 1);
            int y1 = std::min(y0 + 1, src.height() - 1);

            double fx = lx - x0, fy = ly - y0;

            const QRgb *r0 = (const QRgb*)src.constScanLine(y0);
            const QRgb *r1 = (const QRgb*)src.constScanLine(y1);
            const QRgb p00 = r0[x0], p10 = r0[x1], p01 = r1[x0], p11 = r1[x1];
            const double w00 = (1.0 - fx) * (1.0 - fy), w10 = fx * (1.0 - fy);
            const double w01 = (1.0 - fx) * fy,         w11 = fx * fy;

            int a  = (int)(qAlpha(p00)*w00 + qAlpha(p10)*w10 + qAlpha(p01)*w01 + qAlpha(p11)*w11);
            int rr = (int)(qRed(p00)*w00   + qRed(p10)*w10   + qRed(p01)*w01   + qRed(p11)*w11);
            int g  = (int)(qGreen(p00)*w00 + qGreen(p10)*w10 + qGreen(p01)*w01 + qGreen(p11)*w11);
            int b  = (int)(qBlue(p00)*w00  + qBlue(p10)*w10  + qBlue(p01)*w01  + qBlue(p11)*w11);
            line[x] = qRgba(rr, g, b, a);
        }
    }
}


inline void applyPush(QImage &img, const QPoint &from, const QPoint &to,
                      int radius, double strength)
{
    if (radius <= 0 || strength == 0.0) return;

    const double mdx = to.x() - from.x(), mdy = to.y() - from.y();
    const double dist = std::sqrt(mdx * mdx + mdy * mdy);
    if (dist < 0.5) return;

    const int margin = (int)(dist * std::abs(strength)) + 4;
    const double r = radius;

    applyInCircle(img, to, radius, margin,
        [&](double x, double y, double, double, double d, double &sx, double &sy) {
            double t = 1.0 - d / r;
            double falloff = t * t;
            sx = x - mdx * strength * falloff;
            sy = y - mdy * strength * falloff;
        });
}


inline void applyTwirl(QImage &img, const QPoint &center, int radius, double angleRad)
{
    if (radius <= 0 || angleRad == 0.0) return;
    const double r = radius;
    const double cxx = center.x(), cyy = center.y();

    applyInCircle(img, center, radius, 2,
        [&](double, double, double dx, double dy, double d, double &sx, double &sy) {
            double t = 1.0 - d / r;
            double a = angleRad * t * t;
            double ca = std::cos(a), sa = std::sin(a);
            sx = cxx + dx * ca + dy * sa;
            sy = cyy - dx * sa + dy * ca;
        });
}


inline void applyScale(QImage &img, const QPoint &center, int radius, double amount)
{
    if (radius <= 0 || amount == 0.0) return;
    amount = std::max(-0.95, std::min(0.95, amount));

    const int margin = (int)(radius * std::abs(amount)) + 2;
    const double r = radius;
    const double cxx = center.x(), cyy = center.y();

    applyInCircle(img, center, radius, margin,
        [&](double, double, double dx, double dy, double d, double &sx, double &sy) {
            double t = 1.0 - d / r;
            double f = 1.0 - amount * t;
            if (f < 0.05) f = 0.05;
            sx = cxx + dx * f;
            sy = cyy + dy * f;
        });
}


inline void applyWaves(QImage &img, const QPoint &center, int radius,
                       double amplitude, double phase)
{
    if (radius <= 0 || amplitude == 0.0) return;

    const int margin = (int)std::abs(amplitude) + 3;
    const double r = radius;

    applyInCircle(img, center, radius, margin,
        [&](double x, double y, double dx, double dy, double d, double &sx, double &sy) {
            if (d < 0.001) { sx = x; sy = y; return; }
            double t = 1.0 - d / r;
            double wave = std::sin(d * 0.25 + phase) * amplitude * t;
            double ux = dx / d, uy = dy / d;
            sx = x - ux * wave;
            sy = y - uy * wave;
        });
}

} // namespace Deformher


// ============================================================
// DeformSettingsBar — UI flotante de opciones
// (antes vivía en paintarea.h / paintarea.cpp)
// ============================================================
class DeformSettingsBar : public QWidget {
    Q_OBJECT
public:
    explicit DeformSettingsBar(QWidget *parent = nullptr)
        : QWidget(parent, Qt::Window | Qt::WindowStaysOnTopHint | Qt::Tool)
    {
        setWindowTitle(tr("Pincel de Deformacion"));
        setFixedWidth(250);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(10, 8, 10, 10);
        mainLayout->setSpacing(6);

        QHBoxLayout *modeRow = new QHBoxLayout();
        modeRow->addWidget(new QLabel(tr("Modo:")));
        comboMode = new QComboBox();
        comboMode->addItem(tr("Empujar"),           (int)Deformher::ModePush);
        comboMode->addItem(tr("Girar horario"),     (int)Deformher::ModeTwirlCW);
        comboMode->addItem(tr("Girar antihorario"), (int)Deformher::ModeTwirlCCW);
        comboMode->addItem(tr("Inflar"),            (int)Deformher::ModeInflate);
        comboMode->addItem(tr("Desinflar"),         (int)Deformher::ModePinch);
        comboMode->addItem(tr("Ondas"),             (int)Deformher::ModeWaves);
        modeRow->addWidget(comboMode, 1);
        mainLayout->addLayout(modeRow);

        lblRadius = new QLabel(tr("Radio: 40 px"));
        sliderRadius = new QSlider(Qt::Horizontal);
        sliderRadius->setRange(5, 300);
        sliderRadius->setValue(40);
        mainLayout->addWidget(lblRadius);
        mainLayout->addWidget(sliderRadius);

        lblStrength = new QLabel(tr("Fuerza: 50%"));
        sliderStrength = new QSlider(Qt::Horizontal);
        sliderStrength->setRange(1, 100);
        sliderStrength->setValue(50);
        mainLayout->addWidget(lblStrength);
        mainLayout->addWidget(sliderStrength);

        QLabel *lblHint = new QLabel(tr("Arrastra con clic izq.: deforma\nClic der.: invierte el efecto"));
        lblHint->setStyleSheet("font-size: 10px; color: #888;");
        mainLayout->addWidget(lblHint);

        connect(comboMode, &QComboBox::currentIndexChanged,
                this, [this]() { emit optionsChanged(); });
        connect(sliderRadius, &QSlider::valueChanged, this, [this](int v) {
            lblRadius->setText(tr("Radio: %1 px").arg(v));
            emit optionsChanged();
        });
        connect(sliderStrength, &QSlider::valueChanged, this, [this](int v) {
            lblStrength->setText(tr("Fuerza: %1%").arg(v));
            emit optionsChanged();
        });
    }

    DeformOptc getOptions() const {
        DeformOptc o;
        o.mode = comboMode->currentData().toInt();
        o.radius = sliderRadius->value();
        o.strength = sliderStrength->value();
        return o;
    }

signals:
    void optionsChanged();

private:
    QComboBox *comboMode = nullptr;
    QSlider *sliderRadius = nullptr, *sliderStrength = nullptr;
    QLabel *lblRadius = nullptr, *lblStrength = nullptr;
};


// ============================================================
// DeformController — estado + lógica de aplicación
// PaintArea solo lo usa como caja negra.
// ============================================================
class DeformController {
public:
    void setOptions(const DeformOptc &o) { m_opts = o; }
    DeformOptc options() const { return m_opts; }

    bool isActive() const { return m_active; }

    /// Comienza un trazo. Guarda el puntero a la imagen objetivo.
    /// `invert` = true si el botón es derecho (invierte el signo de la fuerza).
    void begin(const QPoint &pos, QImage *target, bool invert) {
        if (!target || target->isNull()) return;
        m_target = target;
        m_active = true;
        m_invert = invert;
        m_lastPoint = pos;
        m_wavePhase = 0.0;
        apply(pos, pos);
    }

    /// Continúa el trazo desde el último punto.
    void continueStroke(const QPoint &pos) {
        if (!m_active || !m_target) return;
        apply(m_lastPoint, pos);
        m_lastPoint = pos;
    }

    /// Termina el trazo.
    void end() {
        m_active = false;
        m_target = nullptr;
    }

    /// Reset por cambio de herramienta / cancelación.
    void reset() {
        m_active = false;
        m_target = nullptr;
        m_lastPoint = QPoint();
        m_wavePhase = 0.0;
    }

    /// Dibuja el círculo indicador bajo el cursor. Devuelve el rect
    /// afectado en coordenadas de canvas (para invalidar repaint).
    void paintOverlay(QPainter &painter, const QPoint &hoverPos,
                      double zoom, bool darkMode) const
    {
        const double rW = m_opts.radius * zoom;
        painter.setPen(QPen(darkMode ? QColor(255, 255, 255, 200)
                                     : QColor(0, 0, 0, 170),
                            1.5));
        painter.setBrush(QColor(147, 100, 255, 35));
        painter.drawEllipse(QPointF(hoverPos), rW, rW);
        painter.drawLine(hoverPos.x() - 6, hoverPos.y(),
                         hoverPos.x() + 6, hoverPos.y());
        painter.drawLine(hoverPos.x(), hoverPos.y() - 6,
                         hoverPos.x(), hoverPos.y() + 6);
    }

    /// Rect (en coords de canvas) que ocupa el pincel bajo el cursor.
    QRect cursorRect(const QPoint &canvasPos) const {
        const int r = m_opts.radius + 6;
        return QRect(canvasPos.x() - r, canvasPos.y() - r, r * 2 + 1, r * 2 + 1);
    }

private:
    DeformOptc m_opts;
    QImage *m_target = nullptr;
    bool m_active = false;
    bool m_invert = false;
    QPoint m_lastPoint;
    double m_wavePhase = 0.0;

    /// Aplica la deformación de `from` a `to` sobre m_target.
    void apply(const QPoint &from, const QPoint &to) {
        if (!m_target || m_target->isNull()) return;

        const int r = qMax(3, m_opts.radius);
        double st = m_opts.strength / 100.0;
        if (m_invert) st = -st;

        switch (m_opts.mode) {
            case Deformher::ModePush:
                Deformher::applyPush(*m_target, from, to, r, st);
                break;
            case Deformher::ModeTwirlCW:
                Deformher::applyTwirl(*m_target, to, r,  0.30 * st);
                break;
            case Deformher::ModeTwirlCCW:
                Deformher::applyTwirl(*m_target, to, r, -0.30 * st);
                break;
            case Deformher::ModeInflate:
                Deformher::applyScale(*m_target, to, r,  0.10 * st);
                break;
            case Deformher::ModePinch:
                Deformher::applyScale(*m_target, to, r, -0.10 * st);
                break;
            case Deformher::ModeWaves:
                m_wavePhase += 0.5 * st;
                Deformher::applyWaves(*m_target, to, r, r * 0.15 * st, m_wavePhase);
                break;
            default:
                break;
        }
    }
};

#endif // DEFORM_TOOLS_H