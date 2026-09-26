#ifndef GRADIENTTOOLS_H
#define GRADIENTTOOLS_H

#include <QWidget>
#include <QDialog>
#include <QComboBox>
#include <QSlider>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QConicalGradient>
#include <QPixmap>
#include <QImage>
#include <QColor>
#include <cmath>


class GradientDirectionWidget : public QWidget {
    Q_OBJECT

private:
    double m_angle = 0.0;
    bool dragging = false;
public:

    GradientDirectionWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setFixedSize(100, 100);
        setCursor(Qt::CrossCursor);
        setToolTip(tr("Arrastra para establecer la dirección del gradiente"));
    }
    void setAngle(double a) { m_angle = a; update(); }
    double getAngle() const { return m_angle; }
signals:

    void angleChanged(double angle);
protected:
    void mousePressEvent(QMouseEvent *e) override {
        if (e->button() == Qt::LeftButton) { dragging = true; updateFromPos(e->position()); }
    }
    void mouseMoveEvent(QMouseEvent *e) override { if (dragging) updateFromPos(e->position()); }
    void mouseReleaseEvent(QMouseEvent *) override { dragging = false; }
    void updateFromPos(const QPointF &pos) {
        double cx = width() / 2.0, cy = height() / 2.0;
        double dx = pos.x() - cx, dy = pos.y() - cy;
        m_angle = atan2(dy, dx) * 180.0 / M_PI;
        if (m_angle < 0) m_angle += 360.0;
        emit angleChanged(m_angle);
        update();
    }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        QRectF outer(6, 6, width() - 12, height() - 12);
        p.setPen(QPen(QColor("#555"), 1.5));
        p.setBrush(QColor("#2a2a2a"));
        p.drawEllipse(outer);
        double cx = width() / 2.0, cy = height() / 2.0;
        double radius = (width() - 12) / 2.0;
        p.setPen(QPen(QColor("#555"), 1));
        for (int i = 0; i < 8; ++i) {
            double a = i * 45.0 * M_PI / 180.0;
            p.drawLine(QPointF(cx + cos(a) * (radius - 5), cy + sin(a) * (radius - 5)),
                       QPointF(cx + cos(a) * radius, cy + sin(a) * radius));
        }
        double rad = m_angle * M_PI / 180.0;
        double arrowLen = radius - 10;
        QPointF tip(cx + cos(rad) * arrowLen, cy + sin(rad) * arrowLen);
        QPointF tail(cx - cos(rad) * arrowLen * 0.35, cy - sin(rad) * arrowLen * 0.35);
        QLinearGradient lineGrad(tail, tip);
        lineGrad.setColorAt(0.0, QColor("#3b82f6"));
        lineGrad.setColorAt(1.0, QColor("#60a5fa"));
        p.setPen(QPen(QBrush(lineGrad), 3, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(tail, tip);
        double headLen = 9;
        double a1 = rad + M_PI * 0.80, a2 = rad - M_PI * 0.80;
        p.setPen(QPen(QColor("#60a5fa"), 2.5, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(tip, QPointF(tip.x() + cos(a1) * headLen, tip.y() + sin(a1) * headLen));
        p.drawLine(tip, QPointF(tip.x() + cos(a2) * headLen, tip.y() + sin(a2) * headLen));
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#3b82f6"));
        p.drawEllipse(QPointF(cx, cy), 3, 3);
        p.setBrush(QColor("#93c5fd"));
        p.drawEllipse(tip, 4, 4);
    }
};

// DIÁLOGO
class GradientDialog : public QDialog {
    Q_OBJECT
private:

    QComboBox *typeCombo;
    QSlider *opacitySlider;
    QSpinBox *opacitySpin;
    GradientDirectionWidget *dirWidget;
    QSlider *angleSlider;
    QSpinBox *angleSpin;
    QCheckBox *reverseCheck;
    QCheckBox *ditherCheck;
    QCheckBox *useSecondColorCheck;
    QComboBox *blendCombo;
    QLabel *previewLabel;
    QLabel *infoLabel;
    QColor color1, color2;
    int gradientType;
    int opacity;
    int angle;
    bool reverse;
    bool dither;
    bool useSecondColor;
    int blendMode;
    bool darkMode;
    QGroupBox *boxAngle;

    void updatePreview() {
        int pw = 320, ph = 70;
        QPixmap pm(pw, ph);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);
        QPixmap checker(10, 10);
        checker.fill(QColor(240, 240, 240));
        QPainter cp(&checker);
        cp.fillRect(0, 0, 5, 5, QColor(200, 200, 200));
        cp.fillRect(5, 5, 5, 5, QColor(200, 200, 200));
        cp.end();
        p.fillRect(0, 0, pw, ph, QBrush(checker));
        QGradient *grad = nullptr;
        if (gradientType == 0) {
            double rad = angle * M_PI / 180.0;
            double cx = pw / 2.0, cy = ph / 2.0;
            double half = pw / 2.0;
            grad = new QLinearGradient(QPointF(cx - half * cos(rad), cy - half * sin(rad)),
                                       QPointF(cx + half * cos(rad), cy + half * sin(rad)));
        } else if (gradientType == 1) {
            grad = new QRadialGradient(pw / 2.0, ph / 2.0, pw / 2.0);
        } else {
            grad = new QConicalGradient(pw / 2.0, ph / 2.0, angle);
        }
        if (grad) {
            QColor c1 = reverse ? (useSecondColor ? color2 : color1) : color1;
            QColor c2;
            if (useSecondColor) { c2 = reverse ? color1 : color2; }
            else { c2 = color1; c2.setAlpha(0); }
            c1.setAlpha(opacity);
            c2.setAlpha(useSecondColor ? opacity : 0);
            grad->setColorAt(0.0, c1);
            grad->setColorAt(1.0, c2);
            p.setBrush(*grad);
            p.setPen(Qt::NoPen);
            p.drawRect(0, 0, pw, ph);
            delete grad;
        }
        p.setPen(QPen(darkMode ? QColor(80, 80, 80) : QColor(180, 180, 180), 1));
        p.setBrush(Qt::NoBrush);
        p.drawRect(0, 0, pw - 1, ph - 1);
        p.end();
        previewLabel->setPixmap(pm);
        QString typeStr = (gradientType == 0) ? tr("Lineal") :
                          (gradientType == 1) ? tr("Radial") : tr("Cónico");
        int opPercent = qRound(opacity / 255.0 * 100);
        QString colorInfo = useSecondColor ? tr("2 colores") : tr("Color→Transp.");
        QString info = QString("%1 | %2° | %3% | %4 | %5")
            .arg(typeStr).arg(angle).arg(opPercent).arg(colorInfo).arg(blendCombo->currentText());
        if (reverse) info += tr(" | Invertido");
        if (dither) info += tr(" | Dither");
        infoLabel->setText(info);
    }

public:
    GradientDialog(bool dark, const QColor &c1, const QColor &c2,
                   int type = 0, int op = 255, int ang = 0,
                   bool rev = false, bool dith = false, int blend = 0,
                   bool useSecond = false,
                   QWidget *parent = nullptr)
        : QDialog(parent), color1(c1), color2(c2), gradientType(type),
          opacity(op), angle(ang), reverse(rev), dither(dith),
          useSecondColor(useSecond), blendMode(blend), darkMode(dark)
    {
        setWindowTitle(tr("Configurar Gradiente"));
        setFixedSize(540, 520);
        QString bg      = darkMode ? "#1a1a1a" : "#fafafa";
        QString input   = darkMode ? "#2a2a2a" : "#ffffff";
        QString text    = darkMode ? "#e5e5e5" : "#111827";
        QString textSec = darkMode ? "#b0b0b0" : "#4b5563";
        QString border  = darkMode ? "#3a3a3a" : "#e5e7eb";
        QString accent  = darkMode ? "#3b82f6" : "#2563eb";
        QString hover   = darkMode ? "#333333" : "#f1f5f9";
        setStyleSheet(QString("QDialog { background-color: %1; }").arg(bg));

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(16, 12, 16, 12);
        mainLayout->setSpacing(10);

        previewLabel = new QLabel();
        previewLabel->setFixedSize(320, 70);
        previewLabel->setAlignment(Qt::AlignCenter);
        previewLabel->setStyleSheet(QString("border: 1px solid %1; border-radius: 6px;").arg(border));
        mainLayout->addWidget(previewLabel, 0, Qt::AlignCenter);

        infoLabel = new QLabel();
        infoLabel->setAlignment(Qt::AlignCenter);
        infoLabel->setWordWrap(true);
        infoLabel->setStyleSheet(QString("color: %1; font-size: 10px; padding: 2px 0;").arg(textSec));
        mainLayout->addWidget(infoLabel);

        QHBoxLayout *row1 = new QHBoxLayout();
        row1->setSpacing(12);
        QVBoxLayout *typeCol = new QVBoxLayout();
        typeCol->setSpacing(4);
        QLabel *typeLbl = new QLabel(tr("Tipo de Gradiente"));
        typeLbl->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: 600;").arg(textSec));
        typeCol->addWidget(typeLbl);
        typeCombo = new QComboBox();
        typeCombo->addItem(tr("Lineal"));
        typeCombo->addItem(tr("Radial"));
        typeCombo->addItem(tr("Cónico"));
        typeCombo->setCurrentIndex(gradientType);
        typeCombo->setFixedHeight(32);
        typeCombo->setStyleSheet(QString(
            "QComboBox { background-color: %1; color: %2; border: 1px solid %3; border-radius: 6px; "
            "padding: 4px 10px; font-size: 12px; } "
            "QComboBox:hover { border-color: %4; } "
            "QComboBox::drop-down { border: none; width: 20px; } "
            "QComboBox QAbstractItemView { background-color: %1; color: %2; border: 1px solid %3; "
            "selection-background-color: %4; padding: 2px; }"
        ).arg(input, text, border, accent));
        typeCol->addWidget(typeCombo);
        row1->addLayout(typeCol, 1);

        QVBoxLayout *secondCol = new QVBoxLayout();
        secondCol->setSpacing(4);
        QLabel *secondLbl = new QLabel(tr("Segundo Color"));
        secondLbl->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: 600;").arg(textSec));
        secondCol->addWidget(secondLbl);
        useSecondColorCheck = new QCheckBox(tr("Usar Color 2"));
        useSecondColorCheck->setChecked(useSecondColor);
        useSecondColorCheck->setStyleSheet(QString(
            "QCheckBox { color: %1; font-size: 12px; spacing: 6px; } "
            "QCheckBox::indicator { width: 15px; height: 15px; border-radius: 3px; border: 1px solid %2; background: %3; } "
            "QCheckBox::indicator:checked { background: %4; border-color: %4; }"
        ).arg(text, border, input, accent));
        secondCol->addWidget(useSecondColorCheck);
        QHBoxLayout *swatchRow = new QHBoxLayout();
        swatchRow->setSpacing(6);
        QLabel *swatch1 = new QLabel();
        swatch1->setFixedSize(32, 22);
        swatch1->setStyleSheet(QString("background-color: %1; border: 1px solid %2; border-radius: 4px;")
            .arg(color1.name(), border));
        swatch1->setToolTip(tr("Color 1 (inicio)"));
        QLabel *arrowLbl = new QLabel("→");
        arrowLbl->setStyleSheet(QString("color: %1; font-size: 13px;").arg(textSec));
        QLabel *swatch2 = new QLabel();
        swatch2->setFixedSize(32, 22);
        swatch2->setStyleSheet(QString("background-color: %1; border: 1px solid %2; border-radius: 4px;")
            .arg(color2.name(), border));
        swatch2->setToolTip(tr("Color 2 (fin)"));
        QLabel *transLbl = new QLabel(tr("→ Transp."));
        transLbl->setStyleSheet(QString("color: %1; font-size: 10px;").arg(textSec));
        transLbl->setVisible(!useSecondColor);
        swatchRow->addWidget(swatch1);
        swatchRow->addWidget(arrowLbl);
        swatchRow->addWidget(swatch2);
        swatchRow->addWidget(transLbl);
        swatchRow->addStretch();
        secondCol->addLayout(swatchRow);
        row1->addLayout(secondCol, 1);
        mainLayout->addLayout(row1);

        QHBoxLayout *row2 = new QHBoxLayout();
        row2->setSpacing(12);
        QString sliderCss = QString(
            "QSlider::groove:horizontal { height: 5px; background: %1; border-radius: 2px; } "
            "QSlider::handle:horizontal { background: %2; width: 14px; height: 14px; margin: -5px 0; border-radius: 7px; } "
            "QSlider::sub-page:horizontal { background: %2; border-radius: 2px; }"
        ).arg(border, accent);

        QVBoxLayout *dirCol = new QVBoxLayout();
        dirCol->setSpacing(4);
        QLabel *dirLbl = new QLabel(tr("Dirección"));
        dirLbl->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: 600;").arg(textSec));
        dirCol->addWidget(dirLbl);
        QHBoxLayout *dirRow = new QHBoxLayout();
        dirRow->setSpacing(8);
        dirWidget = new GradientDirectionWidget();
        dirWidget->setAngle(angle);
        dirRow->addWidget(dirWidget, 0, Qt::AlignVCenter);
        QVBoxLayout *angleControls = new QVBoxLayout();
        angleControls->setSpacing(4);
        angleSlider = new QSlider(Qt::Horizontal);
        angleSlider->setRange(0, 360);
        angleSlider->setValue(angle);
        angleSlider->setStyleSheet(sliderCss);
        angleControls->addWidget(angleSlider);
        angleSpin = new QSpinBox();
        angleSpin->setRange(0, 360);
        angleSpin->setValue(angle);
        angleSpin->setSuffix("°");
        angleSpin->setFixedWidth(70);
        angleSpin->setFixedHeight(28);
        angleSpin->setStyleSheet(QString(
            "QSpinBox { background-color: %1; border: 1px solid %2; border-radius: 5px; "
            "padding: 3px 6px; font-size: 11px; color: %3; } "
            "QSpinBox:focus { border-color: %4; }"
        ).arg(input, border, text, accent));
        angleControls->addWidget(angleSpin);
        QHBoxLayout *angPresets = new QHBoxLayout();
        angPresets->setSpacing(3);
        auto makeAngBtn = [&](const QString &t, int val) {
            QPushButton *b = new QPushButton(t);
            b->setCursor(Qt::PointingHandCursor);
            b->setFixedHeight(24);
            b->setMinimumWidth(34);
            b->setStyleSheet(QString(
                "QPushButton { background-color: %1; border: 1px solid %2; border-radius: 4px; "
                "padding: 0 4px; font-size: 10px; color: %3; } "
                "QPushButton:hover { background-color: %4; border-color: %5; }"
            ).arg(input, border, text, hover, accent));
            connect(b, &QPushButton::clicked, this, [this, val]() { angleSlider->setValue(val); });
            return b;
        };
        angPresets->addWidget(makeAngBtn("0°", 0));
        angPresets->addWidget(makeAngBtn("45°", 45));
        angPresets->addWidget(makeAngBtn("90°", 90));
        angPresets->addWidget(makeAngBtn("180°", 180));
        angleControls->addLayout(angPresets);
        dirRow->addLayout(angleControls, 1);
        dirCol->addLayout(dirRow);
        row2->addLayout(dirCol, 1);

        QVBoxLayout *opCol = new QVBoxLayout();
        opCol->setSpacing(4);
        QLabel *opLbl = new QLabel(tr("Opacidad"));
        opLbl->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: 600;").arg(textSec));
        opCol->addWidget(opLbl);
        QHBoxLayout *opRow = new QHBoxLayout();
        opRow->setSpacing(6);
        opacitySlider = new QSlider(Qt::Horizontal);
        opacitySlider->setRange(0, 255);
        opacitySlider->setValue(opacity);
        opacitySlider->setStyleSheet(sliderCss);
        opRow->addWidget(opacitySlider, 1);
        opacitySpin = new QSpinBox();
        opacitySpin->setRange(0, 255);
        opacitySpin->setValue(opacity);
        opacitySpin->setFixedWidth(60);
        opacitySpin->setFixedHeight(28);
        opacitySpin->setStyleSheet(angleSpin->styleSheet());
        opRow->addWidget(opacitySpin);
        opCol->addLayout(opRow);
        QHBoxLayout *opPresets = new QHBoxLayout();
        opPresets->setSpacing(3);
        auto makeOpBtn = [&](const QString &t, int val) {
            QPushButton *b = new QPushButton(t);
            b->setCursor(Qt::PointingHandCursor);
            b->setFixedHeight(24);
            b->setMinimumWidth(38);
            b->setStyleSheet(QString(
                "QPushButton { background-color: %1; border: 1px solid %2; border-radius: 4px; "
                "padding: 0 5px; font-size: 10px; color: %3; } "
                "QPushButton:hover { background-color: %4; border-color: %5; }"
            ).arg(input, border, text, hover, accent));
            connect(b, &QPushButton::clicked, this, [this, val]() { opacitySlider->setValue(val); });
            return b;
        };
        opPresets->addWidget(makeOpBtn("100%", 255));
        opPresets->addWidget(makeOpBtn("75%", 191));
        opPresets->addWidget(makeOpBtn("50%", 127));
        opPresets->addWidget(makeOpBtn("25%", 64));
        opCol->addLayout(opPresets);
        row2->addLayout(opCol, 1);
        mainLayout->addLayout(row2);

        QHBoxLayout *row3 = new QHBoxLayout();
        row3->setSpacing(12);
        QVBoxLayout *blendCol = new QVBoxLayout();
        blendCol->setSpacing(4);
        QLabel *blendLbl = new QLabel(tr("Modo de Mezcla"));
        blendLbl->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: 600;").arg(textSec));
        blendCol->addWidget(blendLbl);
        blendCombo = new QComboBox();
        blendCombo->addItem(tr("Normal"), 0);
        blendCombo->addItem(tr("Multiplicar"), 1);
        blendCombo->addItem(tr("Pantalla"), 2);
        blendCombo->addItem(tr("Superponer"), 3);
        blendCombo->addItem(tr("Luz suave"), 4);
        blendCombo->addItem(tr("Diferencia"), 5);
        blendCombo->addItem(tr("Oscurecer"), 6);
        blendCombo->addItem(tr("Aclarar"), 7);
        blendCombo->setCurrentIndex(qBound(0, blendMode, blendCombo->count() - 1));
        blendCombo->setFixedHeight(32);
        blendCombo->setStyleSheet(typeCombo->styleSheet());
        blendCol->addWidget(blendCombo);
        row3->addLayout(blendCol, 1);
        QVBoxLayout *optsCol = new QVBoxLayout();
        optsCol->setSpacing(4);
        QLabel *optsLbl = new QLabel(tr("Opciones"));
        optsLbl->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: 600;").arg(textSec));
        optsCol->addWidget(optsLbl);
        reverseCheck = new QCheckBox(tr("Invertir colores"));
        reverseCheck->setChecked(reverse);
        reverseCheck->setStyleSheet(useSecondColorCheck->styleSheet());
        optsCol->addWidget(reverseCheck);
        ditherCheck = new QCheckBox(tr("Dithering"));
        ditherCheck->setChecked(dither);
        ditherCheck->setStyleSheet(useSecondColorCheck->styleSheet());
        optsCol->addWidget(ditherCheck);
        row3->addLayout(optsCol, 1);
        mainLayout->addLayout(row3);

        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->setSpacing(10);
        btnLayout->addStretch();
        QPushButton *btnCancel = new QPushButton(tr("Cancelar"));
        btnCancel->setCursor(Qt::PointingHandCursor);
        btnCancel->setFixedHeight(38);
        btnCancel->setMinimumWidth(100);
        btnCancel->setStyleSheet(QString(
            "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: 7px; "
            "padding: 0 16px; font-size: 12px; font-weight: 500; } "
            "QPushButton:hover { background-color: %4; }"
        ).arg(input, text, border, hover));
        QPushButton *btnApply = new QPushButton(tr("Aplicar Gradiente"));
        btnApply->setCursor(Qt::PointingHandCursor);
        btnApply->setFixedHeight(38);
        btnApply->setMinimumWidth(140);
        btnApply->setStyleSheet(QString(
            "QPushButton { background-color: %1; color: white; border: none; border-radius: 7px; "
            "padding: 0 20px; font-size: 12px; font-weight: 600; } "
            "QPushButton:hover { background-color: %2; } "
            "QPushButton:pressed { background-color: %3; }"
        ).arg(accent, darkMode ? "#2563eb" : "#1d4ed8", darkMode ? "#1d4ed8" : "#1e40af"));
        btnLayout->addWidget(btnCancel);
        btnLayout->addWidget(btnApply);
        mainLayout->addLayout(btnLayout);

        boxAngle = new QGroupBox();
        boxAngle->setVisible(false);

        connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                [this](int idx) {
            gradientType = idx;
            bool enableAngle = (idx != 1);
            dirWidget->setEnabled(enableAngle);
            angleSlider->setEnabled(enableAngle);
            angleSpin->setEnabled(enableAngle);
            updatePreview();
        });
        connect(opacitySlider, &QSlider::valueChanged, this, [this](int val) {
            opacity = val;
            opacitySpin->blockSignals(true); opacitySpin->setValue(val); opacitySpin->blockSignals(false);
            updatePreview();
        });
        connect(opacitySpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
            opacity = val;
            opacitySlider->blockSignals(true); opacitySlider->setValue(val); opacitySlider->blockSignals(false);
            updatePreview();
        });
        connect(dirWidget, &GradientDirectionWidget::angleChanged, this, [this](double a) {
            int val = qRound(a);
            angle = val;
            angleSlider->blockSignals(true); angleSlider->setValue(val); angleSlider->blockSignals(false);
            angleSpin->blockSignals(true); angleSpin->setValue(val); angleSpin->blockSignals(false);
            updatePreview();
        });
        connect(angleSlider, &QSlider::valueChanged, this, [this](int val) {
            angle = val;
            angleSpin->blockSignals(true); angleSpin->setValue(val); angleSpin->blockSignals(false);
            dirWidget->blockSignals(true); dirWidget->setAngle(val); dirWidget->blockSignals(false);
            updatePreview();
        });
        connect(angleSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
            angle = val;
            angleSlider->blockSignals(true); angleSlider->setValue(val); angleSlider->blockSignals(false);
            dirWidget->blockSignals(true); dirWidget->setAngle(val); dirWidget->blockSignals(false);
            updatePreview();
        });
        connect(blendCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
            blendMode = idx; updatePreview();
        });
        connect(reverseCheck, &QCheckBox::toggled, this, [this](bool checked) { reverse = checked; updatePreview(); });
        connect(ditherCheck, &QCheckBox::toggled, this, [this](bool checked) { dither = checked; });
        connect(useSecondColorCheck, &QCheckBox::toggled, this, [this](bool checked) { useSecondColor = checked; updatePreview(); });
        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
        connect(btnApply, &QPushButton::clicked, this, &QDialog::accept);

        dirWidget->setEnabled(gradientType != 1);
        angleSlider->setEnabled(gradientType != 1);
        angleSpin->setEnabled(gradientType != 1);
        updatePreview();
    }

    int getGradientType() const { return gradientType; }
    int getOpacity() const { return opacity; }
    int getAngle() const { return angle; }
    bool getReverse() const { return reverse; }
    bool getDither() const { return dither; }
    int getBlendMode() const { return blendMode; }
    bool getUseSecondColor() const { return useSecondColor; }
    QColor getColor1() const { return color1; }
    QColor getColor2() const { return color2; }
};


// clases principal
class GradientTools {
public:
    static QPainter::CompositionMode compositionMode(int m) {
        switch (m) {
            case 1: return QPainter::CompositionMode_Multiply;
            case 2: return QPainter::CompositionMode_Screen;
            case 3: return QPainter::CompositionMode_Overlay;
            case 4: return QPainter::CompositionMode_SoftLight;
            case 5: return QPainter::CompositionMode_Difference;
            case 6: return QPainter::CompositionMode_Darken;
            case 7: return QPainter::CompositionMode_Lighten;
            default: return QPainter::CompositionMode_SourceOver;
        }
    }

    static void applyDithering(QImage &img) {
        for (int y = 0; y < img.height(); ++y) {
            for (int x = 0; x < img.width(); ++x) {
                QColor c = img.pixelColor(x, y);
                if (c.alpha() == 0) continue;
                int bayer = ((x & 1) ^ (y & 1)) ? 64 : -64;
                img.setPixelColor(x, y, QColor(
                    qBound(0, c.red() + bayer, 255),
                    qBound(0, c.green() + bayer, 255),
                    qBound(0, c.blue() + bayer, 255), c.alpha()));
            }
        }
    }

    static void applyGradient(QImage &target, const QPoint &p1, const QPoint &p2,
                              const QColor &color1, const QColor &color2,
                              int gradientType, int opacity, int angle,
                              bool reverse, bool dither, bool useSecondColor,
                              int blendMode) {
        if (target.isNull()) return;
        QColor c1 = reverse ? color2 : color1;
        QColor c2;
        if (useSecondColor) { c2 = reverse ? color1 : color2; }
        else { c2 = c1; c2.setAlpha(0); }
        c1.setAlpha(opacity);
        c2.setAlpha(useSecondColor ? opacity : 0);

        QGradient *grad = nullptr;
        switch (gradientType) {
            case 0: grad = new QLinearGradient(p1, p2); break;
            case 1: {
                int radius = qMax(1, (int)sqrt(pow(p2.x() - p1.x(), 2) + pow(p2.y() - p1.y(), 2)));
                grad = new QRadialGradient(p1, radius); break;
            }
            case 2: grad = new QConicalGradient(p1, angle); break;
            default: return;
        }
        grad->setColorAt(0.0, c1);
        grad->setColorAt(1.0, c2);

        if (blendMode == 0 && !dither) {
            QPainter painter(&target);
            painter.setPen(Qt::NoPen);
            painter.setBrush(*grad);
            painter.drawRect(target.rect());
            painter.end();
        } else {
            QImage tempImg(target.size(), QImage::Format_ARGB32);
            tempImg.fill(Qt::transparent);
            QPainter tp(&tempImg);
            tp.setPen(Qt::NoPen);
            tp.setBrush(*grad);
            tp.drawRect(tempImg.rect());
            tp.end();
            if (dither) applyDithering(tempImg);
            QPainter painter(&target);
            painter.setCompositionMode(compositionMode(blendMode));
            painter.drawImage(0, 0, tempImg);
            painter.end();
        }
        delete grad;
    }
};

#endif 

// GRADIENTTOOLS_H
