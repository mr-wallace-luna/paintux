#include "mainwindow.h"


bool detectarTemaOscuroSistema() {

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    auto colorScheme = QGuiApplication::styleHints()->colorScheme();
    if (colorScheme == Qt::ColorScheme::Dark) return true;
    if (colorScheme == Qt::ColorScheme::Light) return false;
#endif
    QPalette palette = QApplication::palette();
    QColor windowColor = palette.color(QPalette::Window);
    int luminance = (windowColor.red() * 299 + windowColor.green() * 587 + windowColor.blue() * 114) / 1000;
    return luminance < 128;
}

QIcon crearIconoMascara(bool dark) {

    QPixmap pm(18, 18);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(1, 3, 16, 12, Qt::white);
    p.setPen(QPen(dark ? QColor("#cccccc") : QColor("#333333"), 1));
    p.drawRect(1, 3, 16, 12);
    p.setBrush(Qt::black);
    p.setPen(Qt::NoPen);
    p.drawEllipse(6, 6, 6, 6);
    p.end();
    return QIcon(pm);
}

QIcon crearIconoDeformacion(bool dark) {
    QPixmap pm(20, 20);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QColor col = dark ? QColor("#cccccc") : QColor("#333333");
    p.setPen(QPen(col, 2));
    p.setBrush(Qt::NoBrush);
    p.drawArc(3, 3, 14, 14, 40 * 16, 250 * 16);
    p.setPen(Qt::NoPen);
    p.setBrush(col);
    QPolygonF arrow;
    arrow << QPointF(17.0, 5.0) << QPointF(12.5, 6.5) << QPointF(16.0, 10.0);
    p.drawPolygon(arrow);
    p.drawEllipse(8.5, 8.5, 3, 3);
    p.end();
    return QIcon(pm);
}

// ============================================================
// 📐 NEW CANVAS DIALOG
// ============================================================

void NewCanvasDialog::updatePresetStyle(QPushButton *btn, bool active) {
    btn->setStyleSheet(active ?
        QString("QPushButton { background-color: %1; border: 2px solid %2; border-radius: 10px; "
                "padding: 14px 20px; text-align: left; font-size: 14px; font-weight: 600; color: %3; }")
            .arg(colors.bgSelected, colors.borderAccent, colors.textAccent) :
        QString("QPushButton { background-color: transparent; border: 2px solid %1; border-radius: 10px; "
                "padding: 14px 20px; text-align: left; font-size: 14px; color: %2; }"
                "QPushButton:hover { background-color: %3; border-color: %4; }")
            .arg(colors.border, colors.textPrimary, colors.bgHover, colors.borderAccent));
}

void NewCanvasDialog::setActivePreset(QPushButton *btn) {
    if (activePresetBtn) updatePresetStyle(activePresetBtn, false);
    activePresetBtn = btn;
    if (btn) updatePresetStyle(btn, true);
}

void NewCanvasDialog::applyPreset(int w, int h) {
    sbWidth->blockSignals(true); sbWidth->setValue(w); sbWidth->blockSignals(false);
    sbHeight->blockSignals(true); sbHeight->setValue(h); sbHeight->blockSignals(false);
    updatePreview();
}

void NewCanvasDialog::updatePreview() {
    int w = sbWidth->value(), h = sbHeight->value();
    int previewW = 280, previewH = 190;
    QPixmap pm(previewW, previewH);
    pm.fill(QColor(colors.bgPreview));
    QPainter p(&pm);

    QPixmap checker(10, 10);
    checker.fill(QColor(colors.bgPanel));
    QPainter cp(&checker);
    cp.fillRect(0, 0, 5, 5, QColor(colors.bgHover));
    cp.fillRect(5, 5, 5, 5, QColor(colors.bgHover));
    cp.end();

    double margin = 20;
    double availableW = previewW - margin * 2;
    double availableH = previewH - margin * 2 - 24;
    double scale = qMin(availableW / w, availableH / h);
    int pw = qMax(1, (int)(w * scale));
    int ph = qMax(1, (int)(h * scale));
    int px = (previewW - pw) / 2;
    int py = (previewH - ph - 24) / 2;

    if (selectedBg == 0) p.fillRect(px, py, pw, ph, Qt::white);
    else p.fillRect(px, py, pw, ph, QBrush(checker));

    p.setPen(QPen(QColor(colors.accent), 2));
    p.drawRect(px, py, pw, ph);
    p.setPen(QColor(colors.textSecondary));
    p.setFont(QFont("Adwaita Sans", 9));
    p.drawText(QRect(0, previewH - 28, previewW, 28), Qt::AlignCenter, QString("%1 x %2 px").arg(w).arg(h));
    p.end();

    previewLabel->setPixmap(pm);
    previewLabel->setFixedSize(previewW, previewH);
    QString bgText = selectedBg == 0 ? tr("Fondo blanco") : tr("Fondo transparente");
    lblInfo->setText(QString("<b>%1 x %2</b> px  |  %3").arg(w).arg(h).arg(bgText));
}

NewCanvasDialog::NewCanvasDialog(bool darkMode, QWidget *parent) : QDialog(parent), colors(darkMode) {
    setWindowTitle(tr("Nuevo lienzo"));
    setStyleSheet(QString("QDialog { background-color: %1; }").arg(colors.bgDialog));
    setMinimumSize(620, 460);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(24);

    QVBoxLayout *leftCol = new QVBoxLayout();
    leftCol->setSpacing(12);

    QLabel *lblPresets = new QLabel(tr("Tamaños predefinidos"));
    lblPresets->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: 600; text-transform: uppercase; letter-spacing: 0.5px;").arg(colors.textMuted));
    leftCol->addWidget(lblPresets);

    struct Preset { QString name; int w; int h; };
    QList<Preset> presets = {
        {tr("HD (1280 x 720)"), 1280, 720},
        {tr("Full HD (1920 x 1080)"), 1920, 1080},
        {tr("2K QHD (2560 x 1440)"), 2560, 1440},
        {tr("4K UHD (3840 x 2160)"), 3840, 2160},
        {tr("Cuadrado (1080 x 1080)"), 1080, 1080},
    };

    QScrollArea *presetScroll = new QScrollArea();
    presetScroll->setWidgetResizable(true);
    presetScroll->setFixedWidth(250);
    presetScroll->setFrameShape(QFrame::NoFrame);
    presetScroll->setStyleSheet(QString("QScrollArea { background-color: %1; border: none; } QScrollBar:vertical { width: 8px; background: transparent; } QScrollBar::handle:vertical { background: %2; border-radius: 4px; min-height: 20px; } QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }").arg(colors.bgDialog, colors.borderStrong));

    QWidget *presetContainer = new QWidget();
    presetContainer->setStyleSheet(QString("background-color: %1;").arg(colors.bgDialog));
    QVBoxLayout *presetLayout = new QVBoxLayout(presetContainer);
    presetLayout->setContentsMargins(0, 0, 0, 0);
    presetLayout->setSpacing(10);

    for (const auto &pr : presets) {
        QPushButton *btn = new QPushButton(pr.name);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumHeight(48);
        btn->setFont(QFont("Adwaita Sans", 12));
        int pw = pr.w, ph = pr.h;
        connect(btn, &QPushButton::clicked, this, [this, btn, pw, ph]() { setActivePreset(btn); applyPreset(pw, ph); });
        presetLayout->addWidget(btn);
        presetButtons.append(btn);
    }
    presetLayout->addStretch();
    presetScroll->setWidget(presetContainer);
    leftCol->addWidget(presetScroll);
    mainLayout->addLayout(leftCol);

    QVBoxLayout *rightCol = new QVBoxLayout();
    rightCol->setSpacing(16);

    QGroupBox *boxDim = new QGroupBox(tr("Dimensiones"));
    boxDim->setStyleSheet(QString("QGroupBox { background-color: %1; border: 1px solid %2; border-radius: 10px; margin-top: 14px; padding: 18px 16px 16px 16px; font-weight: 600; color: %3; } QGroupBox::title { subcontrol-origin: margin; left: 18px; padding: 0 10px; font-size: 13px; }").arg(colors.bgPanel, colors.border, colors.textPrimary));
    QGridLayout *dimLayout = new QGridLayout(boxDim);
    dimLayout->setSpacing(12); dimLayout->setVerticalSpacing(14);

    QString spinStyle = QString("QSpinBox { background-color: %1; border: 1px solid %2; border-radius: 8px; padding: 8px 12px; font-size: 13px; color: %3; } QSpinBox:focus { border: 1px solid %4; }").arg(colors.bgInput, colors.border, colors.textPrimary, colors.accent);

    QLabel *lblAncho = new QLabel(tr("Ancho:"));
    lblAncho->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 500;").arg(colors.textSecondary));
    dimLayout->addWidget(lblAncho, 0, 0);

    sbWidth = new QSpinBox(); sbWidth->setRange(50, 9999); sbWidth->setValue(1280);
    sbWidth->setSuffix(" px"); sbWidth->setFixedWidth(130); sbWidth->setStyleSheet(spinStyle);
    dimLayout->addWidget(sbWidth, 0, 1);

    QLabel *lblAlto = new QLabel(tr("Alto:"));
    lblAlto->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 500;").arg(colors.textSecondary));
    dimLayout->addWidget(lblAlto, 1, 0);

    sbHeight = new QSpinBox(); sbHeight->setRange(50, 9999); sbHeight->setValue(720);
    sbHeight->setSuffix(" px"); sbHeight->setFixedWidth(130); sbHeight->setStyleSheet(spinStyle);
    dimLayout->addWidget(sbHeight, 1, 1);

    QLabel *lblFondo = new QLabel(tr("Fondo:"));
    lblFondo->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 500;").arg(colors.textSecondary));
    dimLayout->addWidget(lblFondo, 2, 0);

    QHBoxLayout *bgRow = new QHBoxLayout(); bgRow->setSpacing(10);
    btnWhite = new QPushButton(tr("Blanco"));
    btnWhite->setCheckable(true); btnWhite->setChecked(true);
    btnWhite->setCursor(Qt::PointingHandCursor); btnWhite->setFixedHeight(38); btnWhite->setMinimumWidth(100);
    btnTransparent = new QPushButton(tr("Transparente"));
    btnTransparent->setCheckable(true);
    btnTransparent->setCursor(Qt::PointingHandCursor); btnTransparent->setFixedHeight(38); btnTransparent->setMinimumWidth(120);

    auto updateBgButtons = [this]() {
        if (selectedBg == 0) {
            btnWhite->setStyleSheet(QString("QPushButton { background-color: #ffffff; color: #111; border: 2px solid %1; border-radius: 8px; font-weight: 600; font-size: 13px; }").arg(colors.accent));
            btnTransparent->setStyleSheet(QString("QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: 8px; font-size: 13px; } QPushButton:hover { background-color: %4; }").arg(colors.bgInput, colors.textSecondary, colors.border, colors.bgHover));
        } else {
            btnTransparent->setStyleSheet(QString("QPushButton { background-color: %1; color: %2; border: 2px solid %3; border-radius: 8px; font-weight: 600; font-size: 13px; }").arg(colors.bgSelected, colors.textAccent, colors.accent));
            btnWhite->setStyleSheet(QString("QPushButton { background-color: #ffffff; color: #666; border: 1px solid %1; border-radius: 8px; font-size: 13px; } QPushButton:hover { border-color: %2; }").arg(colors.border, colors.accent));
        }
    };
    bgRow->addWidget(btnWhite); bgRow->addWidget(btnTransparent); bgRow->addStretch();
    dimLayout->addLayout(bgRow, 2, 1);
    rightCol->addWidget(boxDim);

    QGroupBox *boxPreview = new QGroupBox(tr("Vista previa"));
    boxPreview->setStyleSheet(boxDim->styleSheet());
    QVBoxLayout *prevLayout = new QVBoxLayout(boxPreview);
    prevLayout->setSpacing(10); prevLayout->setContentsMargins(14, 18, 14, 14);
    previewLabel = new QLabel();
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setStyleSheet(QString("background-color: %1; border: 1px solid %2; border-radius: 8px;").arg(colors.bgPreview, colors.border));
    prevLayout->addWidget(previewLabel, 0, Qt::AlignCenter);
    lblInfo = new QLabel();
    lblInfo->setAlignment(Qt::AlignCenter);
    lblInfo->setStyleSheet(QString("color: %1; font-size: 12px; padding: 6px;").arg(colors.textSecondary));
    prevLayout->addWidget(lblInfo);
    rightCol->addWidget(boxPreview);

    QHBoxLayout *btnLayout = new QHBoxLayout(); btnLayout->setSpacing(12); btnLayout->addStretch();
    QPushButton *btnCancel = new QPushButton(tr("Cancelar"));
    btnCancel->setCursor(Qt::PointingHandCursor); btnCancel->setFixedHeight(42); btnCancel->setMinimumWidth(110);
    btnCancel->setStyleSheet(QString("QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: 8px; padding: 0 20px; font-size: 14px; font-weight: 500; } QPushButton:hover { background-color: %4; }").arg(colors.bgInput, colors.textPrimary, colors.border, colors.bgHover));
    QPushButton *btnCreate = new QPushButton(tr("Crear lienzo"));
    btnCreate->setCursor(Qt::PointingHandCursor); btnCreate->setFixedHeight(42); btnCreate->setMinimumWidth(140);
    btnCreate->setStyleSheet(QString("QPushButton { background-color: %1; color: white; border: none; border-radius: 8px; padding: 0 24px; font-size: 14px; font-weight: 600; } QPushButton:hover { background-color: %2; } QPushButton:pressed { background-color: %3; }").arg(colors.accent, colors.accentHover, colors.accentPressed));
    btnLayout->addWidget(btnCancel); btnLayout->addWidget(btnCreate);
    rightCol->addLayout(btnLayout);

    mainLayout->addLayout(rightCol);

    connect(sbWidth, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() { setActivePreset(nullptr); updatePreview(); });
    connect(sbHeight, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() { setActivePreset(nullptr); updatePreview(); });
    connect(btnWhite, &QPushButton::clicked, this, [this, updateBgButtons]() { selectedBg = 0; btnWhite->setChecked(true); btnTransparent->setChecked(false); updateBgButtons(); updatePreview(); });
    connect(btnTransparent, &QPushButton::clicked, this, [this, updateBgButtons]() { selectedBg = 1; btnTransparent->setChecked(true); btnWhite->setChecked(false); updateBgButtons(); updatePreview(); });
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnCreate, &QPushButton::clicked, this, &QDialog::accept);

    setActivePreset(presetButtons[0]);
    updatePreview();
    updateBgButtons();
}

int NewCanvasDialog::getWidth() const { return sbWidth->value(); }
int NewCanvasDialog::getHeight() const { return sbHeight->value(); }
bool NewCanvasDialog::isTransparent() const { return selectedBg == 1; }

// ============================================================
// 🖼️ MASK THUMB BUTTON
// ============================================================

MaskThumbButton::MaskThumbButton(QWidget *parent) : QPushButton(parent) {
    setFixedSize(40, 40);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
    setStyleSheet("QPushButton { border: none; background-color: transparent; padding: 0px; }");
}

void MaskThumbButton::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) emit maskClicked(e->modifiers() & Qt::ShiftModifier);
    QPushButton::mousePressEvent(e);
}

// ============================================================
// 📚 DRAGGABLE LAYER ITEM
// ============================================================

DraggableLayerItem::DraggableLayerItem(int idx, const QImage &img, const QString &name, bool vis, bool dark, QWidget *parent)
    : QFrame(parent), layerIndex(idx), isSelected(false), layerVisible(vis), isDarkMode(dark) {
    setObjectName("layerItemFrame");
    setFixedHeight(52);
    setAcceptDrops(true);
    setCursor(Qt::OpenHandCursor);

    QHBoxLayout *itemLayout = new QHBoxLayout(this);
    itemLayout->setContentsMargins(4, 2, 4, 2);
    itemLayout->setSpacing(6);

    QLabel *dragHandle = new QLabel("::");
    dragHandle->setFixedWidth(16);
    dragHandle->setAlignment(Qt::AlignCenter);
    dragHandle->setStyleSheet(dark ? "color: #888; font-size: 14px; font-weight: bold;" : "color: #666; font-size: 14px; font-weight: bold;");
    dragHandle->setToolTip(tr("Arrastra para reordenar"));
    itemLayout->addWidget(dragHandle);

    thumbLabel = new QLabel();
    thumbLabel->setFixedSize(40, 40);
    thumbLabel->setStyleSheet("background-color: #fff; border: 1px solid #ccc; border-radius: 3px;");
    if (!img.isNull()) {
        QPixmap pm = QPixmap::fromImage(img.scaled(40, 40, Qt::KeepAspectRatio, Qt::FastTransformation));
        thumbLabel->setPixmap(pm);
    }
    itemLayout->addWidget(thumbLabel);

    btnMaskThumb = new MaskThumbButton();
    maskThumbFrame = new QFrame();
    maskThumbFrame->setObjectName("maskThumbFrame");
    maskThumbFrame->setFixedSize(46, 46);
    QHBoxLayout *maskFrameLayout = new QHBoxLayout(maskThumbFrame);
    maskFrameLayout->setContentsMargins(2, 2, 2, 2);
    maskFrameLayout->setSpacing(0);
    maskFrameLayout->addWidget(btnMaskThumb, 0, Qt::AlignCenter);
    maskThumbFrame->setVisible(false);
    itemLayout->addWidget(maskThumbFrame);
    connect(btnMaskThumb, &MaskThumbButton::maskClicked, this, [this](bool shiftHeld) { emit maskClicked(layerIndex, shiftHeld); });

    btnColorMaskThumb = new MaskThumbButton();
    colorMaskThumbFrame = new QFrame();
    colorMaskThumbFrame->setObjectName("colorMaskThumbFrame");
    colorMaskThumbFrame->setFixedSize(46, 46);
    QHBoxLayout *colorMaskFrameLayout = new QHBoxLayout(colorMaskThumbFrame);
    colorMaskFrameLayout->setContentsMargins(2, 2, 2, 2);
    colorMaskFrameLayout->setSpacing(0);
    colorMaskFrameLayout->addWidget(btnColorMaskThumb, 0, Qt::AlignCenter);
    colorMaskThumbFrame->setVisible(false);
    itemLayout->addWidget(colorMaskThumbFrame);
    connect(btnColorMaskThumb, &MaskThumbButton::maskClicked, this, [this](bool shiftHeld) { emit colorMaskClicked(layerIndex, shiftHeld); });

    nameLabel = new QLabel(name);
    nameLabel->setStyleSheet(dark ? "color: #ddd; font-size: 11px;" : "color: #333; font-size: 11px;");
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    itemLayout->addWidget(nameLabel, 1);

    btnEye = new QPushButton();
    btnEye->setFixedSize(30, 30);
    btnEye->setCursor(Qt::PointingHandCursor);
    btnEye->setCheckable(true);
    btnEye->setChecked(vis);
    updateEyeButton();
    connect(btnEye, &QPushButton::clicked, this, [this]() {
        layerVisible = !layerVisible;
        btnEye->setChecked(layerVisible);
        updateEyeButton();
        btnEye->setToolTip(layerVisible ? tr("Ocultar capa") : tr("Mostrar capa"));
        emit visibilityToggled(layerIndex, layerVisible);
    });
    itemLayout->addWidget(btnEye);
}

void DraggableLayerItem::updateMaskState(bool hasMask, bool enabled, bool editing, const QImage &preview) {
    if (!hasMask) { maskThumbFrame->setVisible(false); return; }
    maskThumbFrame->setVisible(true);
    QPixmap pm = QPixmap::fromImage(preview.scaled(38, 38, Qt::KeepAspectRatio, Qt::FastTransformation));
    if (pm.isNull()) pm = QPixmap(38, 38);
    if (!enabled) {
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(QPen(Qt::red, 3));
        p.drawLine(3, 3, pm.width() - 3, pm.height() - 3);
        p.drawLine(pm.width() - 3, 3, 3, pm.height() - 3);
        p.end();
    }
    btnMaskThumb->setIcon(QIcon(pm));
    btnMaskThumb->setIconSize(QSize(38, 38));

    QString frameStyle;
    if (editing) {
        frameStyle = isDarkMode
            ? "QFrame#maskThumbFrame { background-color: #1e1e1e; border: 2px solid #3b82f6; border-radius: 5px; }"
            : "QFrame#maskThumbFrame { background-color: #ffffff; border: 2px solid #3b82f6; border-radius: 5px; }";
    } else if (!enabled) {
        frameStyle = isDarkMode
            ? "QFrame#maskThumbFrame { background-color: #1e1e1e; border: 1px solid #ef4444; border-radius: 5px; }"
            : "QFrame#maskThumbFrame { background-color: #ffffff; border: 1px solid #dc2626; border-radius: 5px; }";
    } else {
        frameStyle = isDarkMode
            ? "QFrame#maskThumbFrame { background-color: #1e1e1e; border: 1px solid #3a3a3a; border-radius: 5px; }"
            : "QFrame#maskThumbFrame { background-color: #ffffff; border: 1px solid #d1d5db; border-radius: 5px; }";
    }
    maskThumbFrame->setStyleSheet(frameStyle);

    if (!enabled) btnMaskThumb->setToolTip(tr("Máscara DESACTIVADA\nShift+Click: activar"));
    else if (editing) btnMaskThumb->setToolTip(tr("Editando MÁSCARA\nNegro oculta · Blanco revela · Goma revela\nClick en la capa para salir"));
    else btnMaskThumb->setToolTip(tr("Máscara de capa\nClick: editar\nShift+Click: desactivar"));
}

void DraggableLayerItem::updateColorMaskState(bool hasMask, bool enabled, const QImage &preview) {
    if (!hasMask) { colorMaskThumbFrame->setVisible(false); return; }
    colorMaskThumbFrame->setVisible(true);
    QPixmap pm;
    if (!preview.isNull()) pm = QPixmap::fromImage(preview.scaled(38, 38, Qt::KeepAspectRatio, Qt::FastTransformation));
    if (pm.isNull()) pm = QPixmap(38, 38);
    if (!enabled) {
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(QPen(Qt::red, 3));
        p.drawLine(3, 3, pm.width() - 3, pm.height() - 3);
        p.drawLine(pm.width() - 3, 3, 3, pm.height() - 3);
        p.end();
    }
    btnColorMaskThumb->setIcon(QIcon(pm));
    btnColorMaskThumb->setIconSize(QSize(38, 38));

    QString frameStyle;
    if (enabled) {
        frameStyle = isDarkMode
            ? "QFrame#colorMaskThumbFrame { background-color: #1e1e1e; border: 2px solid #f59e0b; border-radius: 5px; }"
            : "QFrame#colorMaskThumbFrame { background-color: #ffffff; border: 2px solid #d97706; border-radius: 5px; }";
    } else {
        frameStyle = isDarkMode
            ? "QFrame#colorMaskThumbFrame { background-color: #1e1e1e; border: 1px solid #ef4444; border-radius: 5px; }"
            : "QFrame#colorMaskThumbFrame { background-color: #ffffff; border: 1px solid #dc2626; border-radius: 5px; }";
    }
    colorMaskThumbFrame->setStyleSheet(frameStyle);

    if (enabled) btnColorMaskThumb->setToolTip(tr("Máscara de COLOR (filtros en tiempo real)\nClick: re-editar filtros\nShift+Click: desactivar"));
    else btnColorMaskThumb->setToolTip(tr("Máscara de COLOR DESACTIVADA\nClick: re-editar filtros\nShift+Click: activar"));
}

void DraggableLayerItem::refreshFrom(const QImage &img, const QString &name, bool vis, bool dark) {
    isDarkMode = dark;
    if (!img.isNull()) thumbLabel->setPixmap(QPixmap::fromImage(img.scaled(40, 40, Qt::KeepAspectRatio, Qt::FastTransformation)));
    else thumbLabel->setPixmap(QPixmap());
    nameLabel->setText(name);
    nameLabel->setStyleSheet(dark ? "color: #ddd; font-size: 11px;" : "color: #333; font-size: 11px;");
    if (layerVisible != vis) { layerVisible = vis; btnEye->setChecked(vis); updateEyeButton(); }
}

void DraggableLayerItem::updateDarkMode(bool dark) { isDarkMode = dark; updateEyeButton(); updateStyle(); }

void DraggableLayerItem::updateEyeButton() {
    if (layerVisible) {
        btnEye->setIcon(QIcon("assets/ojo.svg"));
        btnEye->setIconSize(QSize(18, 18));
        btnEye->setText("");
        QString baseStyle = "QPushButton { background-color: transparent; border: 1px solid %1; border-radius: 15px; } QPushButton:hover { background-color: %2; } QPushButton:checked { background-color: %3; border-color: %4; }";
        if (isDarkMode) btnEye->setStyleSheet(baseStyle.arg("#888", "rgba(100, 150, 255, 0.2)", "rgba(100, 150, 255, 0.3)", "#60a5fa"));
        else btnEye->setStyleSheet(baseStyle.arg("#cbd5e1", "rgba(59, 130, 246, 0.1)", "rgba(59, 130, 246, 0.15)", "#93c5fd"));
    } else {
        btnEye->setIcon(QIcon());
        btnEye->setText("X");
        btnEye->setFont(QFont("Adwaita Sans", 14, QFont::Bold));
        QString hiddenStyle = "QPushButton { background-color: transparent; border: 1px solid %1; border-radius: 15px; font-weight: bold; color: %2; } QPushButton:hover { background-color: %3; color: %4; }";
        if (isDarkMode) btnEye->setStyleSheet(hiddenStyle.arg("#555", "#888", "rgba(255, 255, 255, 0.1)", "#ccc"));
        else btnEye->setStyleSheet(hiddenStyle.arg("#d1d5db", "#9ca3af", "rgba(0, 0, 0, 0.05)", "#6b7280"));
    }
}

void DraggableLayerItem::setSelected(bool selected) { isSelected = selected; updateStyle(); }

void DraggableLayerItem::updateStyle() {
    if (isSelected) {
        if (isDarkMode) setStyleSheet("QFrame#layerItemFrame { background-color: #1a3a5c; border: 1px solid #3b82f6; border-radius: 4px; }");
        else setStyleSheet("QFrame#layerItemFrame { background-color: #e8f0fe; border: 1px solid #93c5fd; border-radius: 4px; }");
    } else { setStyleSheet(""); }
}

void DraggableLayerItem::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) { emit clicked(layerIndex); dragStartPosition = event->pos(); }
    QFrame::mousePressEvent(event);
}

void DraggableLayerItem::mouseMoveEvent(QMouseEvent *event) {
    if (!(event->buttons() & Qt::LeftButton)) return;
    if ((event->pos() - dragStartPosition).manhattanLength() < QApplication::startDragDistance()) return;
    setCursor(Qt::ClosedHandCursor);
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    mimeData->setData("application/x-layer-index", QByteArray::number(layerIndex));
    drag->setMimeData(mimeData);
    QPixmap pixmap = grab();
    drag->setPixmap(pixmap.scaled(pixmap.width(), 30, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    drag->setHotSpot(event->pos());
    drag->exec(Qt::MoveAction);
    setCursor(Qt::OpenHandCursor);
}

void DraggableLayerItem::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasFormat("application/x-layer-index")) {
        int sourceIndex = event->mimeData()->data("application/x-layer-index").toInt();
        if (sourceIndex != layerIndex) {
            event->acceptProposedAction();
            if (isDarkMode) setStyleSheet("QFrame#layerItemFrame { background-color: #1e3a5f; border: 2px solid #60a5fa; border-radius: 4px; }");
            else setStyleSheet("QFrame#layerItemFrame { background-color: #dbeafe; border: 2px solid #60a5fa; border-radius: 4px; }");
        }
    }
}

void DraggableLayerItem::dragLeaveEvent(QDragLeaveEvent *event) { updateStyle(); QFrame::dragLeaveEvent(event); }

void DraggableLayerItem::dropEvent(QDropEvent *event) {
    if (event->mimeData()->hasFormat("application/x-layer-index")) {
        int sourceIndex = event->mimeData()->data("application/x-layer-index").toInt();
        if (sourceIndex != layerIndex) emit layerMoved(sourceIndex, layerIndex);
        event->acceptProposedAction();
    }
    updateStyle();
}

void DraggableLayerItem::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasFormat("application/x-layer-index")) event->acceptProposedAction();
}

QString mainwind::resolveAssetPath(const QString &filename) {
    QStringList paths = {
        QDir::currentPath() + "/assets/" + filename,
        QApplication::applicationDirPath() + "/assets/" + filename,
        "/usr/share/paintux/assets/" + filename
    };
    for (const QString &path : paths) if (QFile::exists(path)) return path;
    return filename;
}





void mainwind::abrirEditorMascaraColor(int idx) {
    const QList<Layer> &layers = paintArea->getLayers();
    if (idx < 0 || idx >= layers.size()) return;
    if (layers[idx].locked) { statusBar()->showMessage(tr("Capa bloqueada"), 2000); return; }

    ImageFiltersDialog dlg(layers[idx].image, this, true, darkMode ? 1 : 0);

    connect(&dlg, &ImageFiltersDialog::paramsChanged, this, [this, idx, &dlg]() {
        paintArea->setLiveColorMaskPreview(idx, dlg.getParams());
    });

    if (dlg.exec() == QDialog::Accepted) {
        paintArea->addLayerColorMask(idx, dlg.getParams());
    } else {
        paintArea->clearLiveColorMaskPreview(idx);
    }
}


mainwind::mainwind() {
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowTitle(tr("Paint-ux"));

    darkMode = detectarTemaOscuroSistema();
    resize(1280, 720);
    animTimer = new QTimer(this);

    textFormatBar = new TextEngine::FormatBar(this);
    textFormatBar->hide();

    deformSettingsBar = new DeformSettingsBar(this);
    deformSettingsBar->hide();

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);

    // === SYS BAR ===
    sysBarWidget = new QWidget();
    sysBarWidget->setObjectName("sysBarContainer");
    sysBarWidget->setFixedHeight(34);
    QHBoxLayout *sysBarLayout = new QHBoxLayout(sysBarWidget);
    sysBarLayout->setContentsMargins(8, 0, 8, 0);
    sysBarLayout->setSpacing(4);

    btnArchivoMenu = new QPushButton(tr("Archivo"));
    btnArchivoMenu->setObjectName("btnArchivoMenu");
    menuArchivoDesplegable = new QMenu(this);
    actNuevo = menuArchivoDesplegable->addAction(tr("Nuevo lienzo..."));
    actInsertarImagen = menuArchivoDesplegable->addAction(tr("Insertar imagen (objeto seleccionable)"));
    actInsertarImagen->setToolTip(tr("Inserta la imagen como un objeto que puedes mover, escalar y rotar con los tiradores"));
    actInsertarComoCapa = menuArchivoDesplegable->addAction(tr("Insertar imagen como capa nueva"));
    actInsertarComoCapa->setToolTip(tr("Inserta la imagen en una capa nueva (solo modo Avanzado)"));
    actAbrirFondo = menuArchivoDesplegable->addAction(tr("Abrir imagen como fondo"));
    actAbrirFondo->setToolTip(tr("Reemplaza el lienzo actual con la imagen"));
    menuArchivoDesplegable->addSeparator();
    QMenu *menuGuardarComo = menuArchivoDesplegable->addMenu(tr("Guardar como..."));
    QAction *actGuardarPNG = menuGuardarComo->addAction(tr("PNG  (*.png)"));
    QAction *actGuardarJPG = menuGuardarComo->addAction(tr("JPEG  (*.jpg / *.jpeg)"));
    QAction *actGuardarSVG = menuGuardarComo->addAction(tr("SVG Vectorial  (*.svg)"));
    actExportGif = menuArchivoDesplegable->addAction(tr("Exportar animacion como GIF..."));
    actExportGif->setVisible(false);
    menuArchivoDesplegable->addSeparator();
    actSalir = menuArchivoDesplegable->addAction(tr("Salir de la aplicacion"));
    btnArchivoMenu->setMenu(menuArchivoDesplegable);

    btnViewMenu = new QPushButton(tr("Ver"));
    btnViewMenu->setObjectName("btnViewMenu");
    QMenu *menuViewDesplegable = new QMenu(this);
    menuViewDesplegable->addAction("720p HD (1280 x 720)");
    menuViewDesplegable->addAction("1080p FHD (1920 x 1080)");
    menuViewDesplegable->addAction("2K QHD (2560 x 1440)");
    menuViewDesplegable->addAction("4K UHD (3840 x 2160)");
    menuViewDesplegable->addSeparator();
    menuViewDesplegable->addAction(tr("Pantalla Completa"));
    menuViewDesplegable->addAction(tr("Maximizar Ventana"));
    menuViewDesplegable->addSeparator();
    connect(menuViewDesplegable, &QMenu::triggered, this, [this](QAction* a){
        QString txt = a->text();
        if(txt.contains("720p")) resize(1280,720);
        else if(txt.contains("1080p")) resize(1920,1080);
        else if(txt.contains("2K")) resize(2560,1440);
        else if(txt.contains("4K")) resize(3840,2160);
        else if(txt.contains(tr("Pantalla Completa"))) showFullScreen();
        else if(txt.contains(tr("Maximizar"))) showMaximized();
    });
    QMenu *menuGrid = menuViewDesplegable->addMenu(tr("Cuadricula"));
    QActionGroup *gridGroup = new QActionGroup(this);
    QAction *gOff = menuGrid->addAction(tr("Apagado")); gOff->setCheckable(true); gOff->setChecked(true);
    QAction *g8 = menuGrid->addAction("8x8"); g8->setCheckable(true);
    QAction *g16 = menuGrid->addAction("16x16"); g16->setCheckable(true);
    QAction *g32 = menuGrid->addAction("32x32"); g32->setCheckable(true);
    QAction *g64 = menuGrid->addAction("64x64"); g64->setCheckable(true);
    gridGroup->addAction(gOff); gridGroup->addAction(g8); gridGroup->addAction(g16); gridGroup->addAction(g32); gridGroup->addAction(g64);
    btnViewMenu->setMenu(menuViewDesplegable);

    btnConfiguraciones = new QPushButton();
    btnConfiguraciones->setObjectName("btnQuickIcon");
    btnConfiguraciones->setIcon(QIcon(resolveAssetPath("configuraciones.svg")));
    btnConfiguraciones->setIconSize(QSize(18, 18));
    menuConfiguracionesDesplegable = new QMenu(this);
    QAction *actNormal = menuConfiguracionesDesplegable->addAction(tr("Modo Normal"));
    QAction *actPixelArt = menuConfiguracionesDesplegable->addAction(tr("Modo Pixel Art"));
    QAction *actAvanzado = menuConfiguracionesDesplegable->addAction(tr("Modo Avanzado"));
    actNormal->setCheckable(true); actPixelArt->setCheckable(true); actAvanzado->setCheckable(true);
    QActionGroup *modeGroup = new QActionGroup(this);
    modeGroup->addAction(actNormal); modeGroup->addAction(actPixelArt); modeGroup->addAction(actAvanzado);
    actNormal->setChecked(true);
    menuConfiguracionesDesplegable->addSeparator();

    QMenu *menuSensibilidad = menuConfiguracionesDesplegable->addMenu(tr("Sensibilidad del Raton"));
    QActionGroup *sensGroup = new QActionGroup(this);
    QAction *actSensBaja = menuSensibilidad->addAction(tr("Baja")); actSensBaja->setCheckable(true);
    QAction *actSensNormal = menuSensibilidad->addAction(tr("Normal")); actSensNormal->setCheckable(true); actSensNormal->setChecked(true);
    QAction *actSensAlta = menuSensibilidad->addAction(tr("Alta")); actSensAlta->setCheckable(true);
    sensGroup->addAction(actSensBaja); sensGroup->addAction(actSensNormal); sensGroup->addAction(actSensAlta);
    connect(sensGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        double sens = 1.0;
        if (action->text() == tr("Baja")) sens = 0.5;
        else if (action->text() == tr("Alta")) sens = 2.0;
        paintArea->setMouseSensitivity(sens);
        statusBar()->showMessage(tr("Sensibilidad: %1").arg(action->text()), 2000);
    });

    menuConfiguracionesDesplegable->addSeparator();
    QMenu *menuTransparencia = menuConfiguracionesDesplegable->addMenu(tr("Transparencia del Fondo"));
    transparencyGroup = new QActionGroup(this);
    struct TransOption { QString name; int alpha; };
    QList<TransOption> opcionesTrans = {
        {tr("Opaco (100%)"), 255}, {tr("85% (Muy Ligero)"), 217}, {tr("75% (Ligero)"), 191},
        {tr("50% (Medio)"), 127}, {tr("25% (Fuerte)"), 64}, {tr("10% (Muy Fuerte)"), 26}, {tr("0% (Ver Wallpaper)"), 0}
    };
    for (const auto &opt : opcionesTrans) {
        QAction *act = menuTransparencia->addAction(opt.name);
        act->setCheckable(true); act->setData(opt.alpha);
        if (opt.alpha == 191) act->setChecked(true);
        transparencyGroup->addAction(act);
    }
    menuTransparencia->addSeparator();
    QAction *actPersonalizado = menuTransparencia->addAction(tr("Personalizado..."));
    connect(transparencyGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        currentWorkspaceAlpha = action->data().toInt();
        compilarHojasDeEstiloGlobales();
        statusBar()->showMessage(tr("Transparencia: %1").arg(action->text()), 2000);
    });
    connect(actPersonalizado, &QAction::triggered, this, [this]() {
        QDialog dlg(this); dlg.setWindowTitle(tr("Transparencia Personalizada"));
        QVBoxLayout *lay = new QVBoxLayout(&dlg);
        QSlider *slider = new QSlider(Qt::Horizontal); slider->setRange(1, 255); slider->setValue(currentWorkspaceAlpha);
        QLabel *valLbl = new QLabel(QString::number(currentWorkspaceAlpha));
        connect(slider, &QSlider::valueChanged, this, [valLbl](int v) { valLbl->setText(QString::number(v)); });
        lay->addWidget(new QLabel(tr("Selecciona el nivel de opacidad (1-255):")));
        QHBoxLayout *hl = new QHBoxLayout(); hl->addWidget(slider); hl->addWidget(valLbl); lay->addLayout(hl);
        QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel); lay->addWidget(bb);
        connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        if (dlg.exec() == QDialog::Accepted) {
            currentWorkspaceAlpha = slider->value();
            compilarHojasDeEstiloGlobales();
            statusBar()->showMessage(tr("Transparencia: %1").arg(currentWorkspaceAlpha), 2000);
        }
    });

    menuConfiguracionesDesplegable->addSeparator();
    QMenu *menuIdiomas = menuConfiguracionesDesplegable->addMenu(QString::fromUtf8("\xF0\x9F\x8C\x8D Idiomas / Languages"));
    QActionGroup *langGroup = new QActionGroup(this);
    struct LangEntry { const char *nativeName; const char *code; };
    const LangEntry langs[] = {
        {"Espa\xC3\xB1ol", "es"}, {"English", "en"}, {"Fran\xC3\xA7" "ais", "fr"},
        {"Deutsch", "de"}, {"Italiano", "it"}, {"Portugu\xC3\xAAs", "pt"},
        {"\xE4\xB8\xAD\xE6\x96\x87", "zh"}, {"\xD8\xA7\xD9\x84\xD8\xB9\xD8\xB1\xD8\xA8\xD9\x8A\xD8\xA9", "ar"},
        {"\xD0\xA0\xD1\x83\xD1\x81\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB9", "ru"}, {"\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E", "ja"},
        {"\xED\x95\x9C\xEA\xB5\xAD\xEC\x96\xB4", "ko"}, {"T\xC3\xBCrk\xC3\xA7" "e", "tr"},
        {"Polski", "pl"}, {"Nederlands", "nl"}, {"Svenska", "sv"},
        {"\xE0\xA4\xB9\xE0\xA4\xBF\xE0\xA4\xA8\xE0\xA5\x8D\xE0\xA4\xA6\xE0\xA5\x80", "hi"},
    };
    QSettings settings("Paintux", "PaintuxStudio");
    QString currentLang = settings.value("language", "es").toString();
    for (const auto &entry : langs) {
        QAction *act = menuIdiomas->addAction(QString::fromUtf8(entry.nativeName));
        act->setCheckable(true);
        act->setData(QString(entry.code));
        if (QString(entry.code) == currentLang) act->setChecked(true);
        langGroup->addAction(act);
    }
    connect(langGroup, &QActionGroup::triggered, this, [this](QAction *action) { cambiarIdioma(action->data().toString()); });

    menuConfiguracionesDesplegable->addSeparator();
    QAction *actAyuda = menuConfiguracionesDesplegable->addAction(tr("Ayuda / Informacion"));
    connect(actAyuda, &QAction::triggered, this, [this]() {
        QMessageBox::information(this, tr("Ayuda"), tr(
            "Atajos y consejos basicos:\n\n"
            "- Rueda del raton: Zoom centrado en el cursor.\n"
            "- Click izquierdo: Pinta con el Color activo.\n"
            "- Click derecho: Pinta con el Color secundario.\n"
            "- Seleccion Libre / Lazos: presiona V en el lienzo para modo vector.\n"
            "- Recorte unificado: un solo boton de lazo. Clic DERECHO sobre el\n"
            "  boton despliega 'Recortar hacia ADENTRO' o 'Recortar hacia AFUERA'.\n"
            "- Deformacion: empuja, gira, infla o desinfla la capa con el raton.\n"
            "  Radio y fuerza se ajustan en su ventana flotante.\n"
            "- O: convierte seleccion en objeto editable.\n"
            "- I: integra objetos al lienzo.\n"
            "- Mover: arrastra objetos, figuras, textos o capas.\n"
            "  Shift+clic para seleccionar varios. Rota/escala con las esquinas.\n"
            "- Capas: Arrastra para reordenar.\n"
            "- Mascaras: click en la miniatura B/N junto a la capa (negro oculta, blanco revela).\n"
            "- Mascaras de COLOR: click en la miniatura naranja. Se aplican EN TIEMPO REAL\n"
            "  y puedes pintar encima de la capa con el filtro activo."));
    });
    QAction *actAcercaDe = menuConfiguracionesDesplegable->addAction(tr("Acerca de Paintlux Studio"));
    connect(actAcercaDe, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, tr("Acerca de Paintlux Studio"), tr("<h2>Paintlux Studio - Alpha 2026</h2><p><b>Version:</b> Alpha 2026.1</p><p>Un program simple de dibujo y edicion.</p>"));
    });
    btnConfiguraciones->setMenu(menuConfiguracionesDesplegable);

    btnQuickSave = new QPushButton(); btnQuickSave->setObjectName("btnQuickIcon");
    btnQuickSave->setIcon(QIcon(resolveAssetPath("guardar.svg"))); btnQuickSave->setIconSize(QSize(18, 18));
    btnUndo = new QPushButton(); btnUndo->setObjectName("btnQuickIcon");
    btnUndo->setIcon(QIcon(resolveAssetPath("deshacer.svg"))); btnUndo->setIconSize(QSize(18, 18));
    btnRedo = new QPushButton(); btnRedo->setObjectName("btnQuickIcon");
    btnRedo->setIcon(QIcon(resolveAssetPath("rehacer.svg"))); btnRedo->setIconSize(QSize(18, 18));

    btnImageFilters = new QPushButton(tr("Filtros y Color")); btnImageFilters->setObjectName("btnQuickAction"); btnImageFilters->setVisible(false);
    btnCustomBrushesTop = new QPushButton(tr("Configurar Pincel Personal")); btnCustomBrushesTop->setObjectName("btnQuickAction"); btnCustomBrushesTop->setVisible(false);
    btnCanvasSize = new QPushButton(tr("Tamano / Recorte")); btnCanvasSize->setObjectName("btnQuickAction"); btnCanvasSize->setVisible(false);

    menuCanvasSizeStd = new QMenu(this);
    menuCanvasSizeStd->addAction("16:9 FHD (1920 x 1080)", this, [this](){ paintArea->cambiarDimensionesLienzo(1920, 1080); });
    menuCanvasSizeStd->addAction("16:9 HD (1280 x 720)", this, [this](){ paintArea->cambiarDimensionesLienzo(1280, 720); });
    menuCanvasSizeStd->addAction("4:3 (1024 x 768)", this, [this](){ paintArea->cambiarDimensionesLienzo(1024, 768); });
    menuCanvasSizeStd->addAction("1:1 Cuadrado (1080 x 1080)", this, [this](){ paintArea->cambiarDimensionesLienzo(1080, 1080); });
    menuCanvasSizeStd->addAction("9:16 Vertical (1080 x 1920)", this, [this](){ paintArea->cambiarDimensionesLienzo(1080, 1920); });
    menuCanvasSizeStd->addSeparator();
    menuCanvasSizeStd->addAction(tr("Personalizado..."), this, [this]() {
        QDialog dlg(this); dlg.setWindowTitle(tr("Tamano Personalizado"));
        QVBoxLayout *l = new QVBoxLayout(&dlg); QHBoxLayout *hl = new QHBoxLayout();
        QSpinBox *sbW = new QSpinBox(); sbW->setRange(50, 9999); sbW->setValue(paintArea->canvasSize().width());
        QSpinBox *sbH = new QSpinBox(); sbH->setRange(50, 9999); sbH->setValue(paintArea->canvasSize().height());
        hl->addWidget(new QLabel(tr("Ancho:"))); hl->addWidget(sbW);
        hl->addWidget(new QLabel(tr("Alto:"))); hl->addWidget(sbH);
        l->addLayout(hl);
        QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel); l->addWidget(bb);
        connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        if (dlg.exec() == QDialog::Accepted) paintArea->cambiarDimensionesLienzo(sbW->value(), sbH->value());
    });

    menuCanvasSizePixel = new QMenu(this);
    QWidget *pixelWidget = new QWidget();
    pixelWidget->setFixedWidth(180); pixelWidget->setFixedHeight(190);
    QVBoxLayout *pixelLayout = new QVBoxLayout(pixelWidget);
    pixelLayout->setContentsMargins(6, 6, 6, 6); pixelLayout->setSpacing(4); pixelLayout->setAlignment(Qt::AlignTop);
    struct PixelRes { const char *label; int res; };
    const PixelRes pixelResolutions[] = {
        {"Resoluci\xC3\xB3n 8x8", 8}, {"Resoluci\xC3\xB3n 16x16", 16},
        {"Resoluci\xC3\xB3n 32x32", 32}, {"Resoluci\xC3\xB3n 64x64", 64},
    };
    for (const auto &pr : pixelResolutions) {
        QPushButton *btnRes = new QPushButton(QString::fromUtf8(pr.label));
        btnRes->setCursor(Qt::PointingHandCursor); btnRes->setFixedHeight(30);
        btnRes->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btnRes->setStyleSheet("QPushButton { background-color: transparent; border: 1px solid #555; border-radius: 6px; padding: 4px 12px; font-size: 12px; text-align: left; color: #ddd; } QPushButton:hover { background-color: #1a4a7c; border-color: #0066cc; color: white; }");
        int res = pr.res;
        connect(btnRes, &QPushButton::clicked, this, [this, res]() { paintArea->setPixelArtResolution(res); menuCanvasSizePixel->close(); });
        pixelLayout->addWidget(btnRes);
    }
    QFrame *pixelSep = new QFrame(); pixelSep->setFrameShape(QFrame::HLine); pixelSep->setFixedHeight(1);
    pixelSep->setStyleSheet("background-color: #555; border: none;");
    pixelLayout->addWidget(pixelSep);
    QPushButton *btnPixelCustom = new QPushButton(tr("Personalizado..."));
    btnPixelCustom->setCursor(Qt::PointingHandCursor); btnPixelCustom->setFixedHeight(30);
    btnPixelCustom->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    btnPixelCustom->setStyleSheet("QPushButton { background-color: transparent; border: 1px solid #555; border-radius: 6px; padding: 4px 12px; font-size: 12px; text-align: left; color: #aaa; font-style: italic; } QPushButton:hover { background-color: #333; border-color: #0066cc; color: #ddd; }");
    connect(btnPixelCustom, &QPushButton::clicked, this, [this]() {
        bool ok;
        int res = QInputDialog::getInt(this, tr("Tamano Pixel Art"), tr("Resolucion cuadrada (px):"), paintArea->getPixelResolution(), 4, 512, 1, &ok);
        if (ok) paintArea->setPixelArtResolution(res);
        menuCanvasSizePixel->close();
    });
    pixelLayout->addWidget(btnPixelCustom);
    QWidgetAction *pixelWidgetAction = new QWidgetAction(this);
    pixelWidgetAction->setDefaultWidget(pixelWidget);
    menuCanvasSizePixel->addAction(pixelWidgetAction);

    btnCanvasSize->setMenu(menuCanvasSizeStd);

    btnThemeToggle = new QPushButton(darkMode ? tr("Modo Claro") : tr("Modo Oscuro"));
    btnThemeToggle->setObjectName("btnSysToggle");

    sysBarLayout->addWidget(btnArchivoMenu); sysBarLayout->addWidget(btnViewMenu); sysBarLayout->addWidget(btnConfiguraciones);
    sysBarLayout->addWidget(btnQuickSave); sysBarLayout->addWidget(btnUndo); sysBarLayout->addWidget(btnRedo);
    separadorBarra = new QFrame(); separadorBarra->setFrameShape(QFrame::VLine);
    sysBarLayout->addWidget(separadorBarra); sysBarLayout->addSpacing(12);
    sysBarLayout->addWidget(btnImageFilters); sysBarLayout->addWidget(btnCustomBrushesTop); sysBarLayout->addWidget(btnCanvasSize);
    sysBarLayout->addStretch(); sysBarLayout->addWidget(btnThemeToggle);
    centralLayout->addWidget(sysBarWidget);

    // === RIBBON ===
    ribbonWidget = new QWidget(); ribbonWidget->setObjectName("ribbonContainer"); ribbonWidget->setFixedHeight(155);
    QHBoxLayout *ribbonLayout = new QHBoxLayout(ribbonWidget);
    ribbonLayout->setContentsMargins(10, 4, 10, 2); ribbonLayout->setSpacing(8); ribbonLayout->setAlignment(Qt::AlignLeft);

    boxClipboard = new QGroupBox(tr("Portapapeles"));
    QHBoxLayout *layoutClip = new QHBoxLayout(boxClipboard); layoutClip->setContentsMargins(4, 2, 4, 2); layoutClip->setSpacing(4);
    QToolButton *btnPaste = new QToolButton(); btnPaste->setObjectName("btnBigPaste"); btnPaste->setText(tr("Pegar"));
    btnPaste->setIcon(QIcon(resolveAssetPath("pegar.svg"))); btnPaste->setIconSize(QSize(28, 28)); btnPaste->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    QPushButton *btnCopy = new QPushButton(tr(" Copiar")); btnCopy->setObjectName("btnSmallStacked");
    btnCopy->setIcon(QIcon(resolveAssetPath("copiar.svg"))); btnCopy->setIconSize(QSize(14, 14));
    QPushButton *btnCut = new QPushButton(tr(" Cortar")); btnCut->setObjectName("btnSmallStacked");
    btnCut->setIcon(QIcon(resolveAssetPath("cortar.svg"))); btnCut->setIconSize(QSize(14, 14));
    QVBoxLayout *layoutCutCopy = new QVBoxLayout(); layoutCutCopy->setSpacing(2);
    layoutCutCopy->addWidget(btnCopy); layoutCutCopy->addWidget(btnCut);
    layoutClip->addWidget(btnPaste); layoutClip->addLayout(layoutCutCopy);
    ribbonLayout->addWidget(boxClipboard);

    QGroupBox *boxImageGroup = new QGroupBox(tr("Imagen"));
    QVBoxLayout *layoutImg = new QVBoxLayout(boxImageGroup); layoutImg->setContentsMargins(4, 2, 4, 2); layoutImg->setSpacing(4);
    QPushButton *btnSelectBox = new QPushButton(tr("Seleccionar")); btnSelectBox->setObjectName("btnImgAction"); btnSelectBox->setCheckable(true);
    QPushButton *btnSelectFree = new QPushButton(tr("Seleccion Libre")); btnSelectFree->setObjectName("btnImgAction"); btnSelectFree->setCheckable(true);
    btnSelectFree->setToolTip(tr("Seleccion Libre"));
    layoutImg->addWidget(btnSelectBox); layoutImg->addWidget(btnSelectFree);
    ribbonLayout->addWidget(boxImageGroup);

    boxBrushesContainer = new QGroupBox(tr("Herramientas y Pinceles"));
    QHBoxLayout *layoutToolsMaster = new QHBoxLayout(boxBrushesContainer);
    layoutToolsMaster->setContentsMargins(4, 2, 4, 2); layoutToolsMaster->setSpacing(6);
    QGridLayout *layoutToolsGrid = new QGridLayout(); layoutToolsGrid->setSpacing(3);

    btnPencil = new QPushButton(); btnPencil->setObjectName("btnToolGrid");
    btnPencil->setIcon(QIcon(resolveAssetPath("lapiz.svg"))); btnPencil->setIconSize(QSize(18, 18)); btnPencil->setCheckable(true); btnPencil->setChecked(true);
    btnBucket = new QPushButton(); btnBucket->setObjectName("btnToolGrid");
    btnBucket->setIcon(QIcon(resolveAssetPath("cubeta-pintura.svg"))); btnBucket->setIconSize(QSize(18, 18)); btnBucket->setCheckable(true);
    btnText = new QPushButton("A"); btnText->setObjectName("btnToolGrid"); btnText->setFont(QFont("Adwaita Sans", 11, QFont::Bold)); btnText->setCheckable(true);
    btnEraser = new QPushButton(); btnEraser->setObjectName("btnToolGrid");
    btnEraser->setIcon(QIcon(resolveAssetPath("goma.svg"))); btnEraser->setIconSize(QSize(18, 18)); btnEraser->setCheckable(true);
    btnPicker = new QPushButton(); btnPicker->setObjectName("btnToolGrid");
    btnPicker->setIcon(QIcon(resolveAssetPath("gotero.svg"))); btnPicker->setIconSize(QSize(18, 18)); btnPicker->setCheckable(true);
    btnMoveTool = new QPushButton(); btnMoveTool->setObjectName("btnToolGrid");
    btnMoveTool->setIcon(QIcon(resolveAssetPath("move.svg"))); btnMoveTool->setIconSize(QSize(18, 18)); btnMoveTool->setCheckable(true);
    btnMoveTool->setToolTip(tr("Mover objetos / capas"));

    layoutToolsGrid->addWidget(btnPencil, 0, 0); layoutToolsGrid->addWidget(btnBucket, 0, 1); layoutToolsGrid->addWidget(btnText, 0, 2);
    layoutToolsGrid->addWidget(btnEraser, 1, 0); layoutToolsGrid->addWidget(btnPicker, 1, 1);
    layoutToolsGrid->addWidget(btnMoveTool, 1, 2);
    layoutToolsMaster->addLayout(layoutToolsGrid);

    QFrame *dividerTools = new QFrame(); dividerTools->setObjectName("dividerTools");
    dividerTools->setFrameShape(QFrame::VLine); dividerTools->setFrameShadow(QFrame::Plain);
    layoutToolsMaster->addWidget(dividerTools);

    btnBrushTool = new QToolButton(); btnBrushTool->setObjectName("btnBrushUnit");
    btnBrushTool->setIcon(QIcon(resolveAssetPath("pincel.svg"))); btnBrushTool->setIconSize(QSize(20, 20)); btnBrushTool->setCheckable(true);
    btnSprayTool = new QToolButton(); btnSprayTool->setObjectName("btnBrushUnit");
    btnSprayTool->setIcon(QIcon(resolveAssetPath("spray.svg"))); btnSprayTool->setIconSize(QSize(20, 20)); btnSprayTool->setCheckable(true);
    btnCrayonTool = new QToolButton(); btnCrayonTool->setObjectName("btnBrushUnit");
    btnCrayonTool->setIcon(QIcon(resolveAssetPath("crayon.svg"))); btnCrayonTool->setIconSize(QSize(20, 20)); btnCrayonTool->setCheckable(true);
    btnMarkerTool = new QToolButton(); btnMarkerTool->setObjectName("btnBrushUnit");
    btnMarkerTool->setIcon(QIcon(resolveAssetPath("marker.svg"))); btnMarkerTool->setIconSize(QSize(20, 20)); btnMarkerTool->setCheckable(true);
    btnWatercolor = new QToolButton(); btnWatercolor->setObjectName("btnBrushUnit");
    btnWatercolor->setIcon(QIcon(resolveAssetPath("acuarela.svg"))); btnWatercolor->setIconSize(QSize(20, 20)); btnWatercolor->setCheckable(true);
    btnOilBrush = new QToolButton(); btnOilBrush->setObjectName("btnBrushUnit");
    btnOilBrush->setIcon(QIcon(resolveAssetPath("oleo.svg"))); btnOilBrush->setIconSize(QSize(20, 20)); btnOilBrush->setCheckable(true);
    btnCalligraphy = new QToolButton(); btnCalligraphy->setObjectName("btnBrushUnit");
    btnCalligraphy->setIcon(QIcon(resolveAssetPath("caligrafia.svg"))); btnCalligraphy->setIconSize(QSize(20, 20)); btnCalligraphy->setCheckable(true);
    btnHighlighter = new QToolButton(); btnHighlighter->setObjectName("btnBrushUnit");
    btnHighlighter->setIcon(QIcon(resolveAssetPath("resaltador.svg"))); btnHighlighter->setIconSize(QSize(20, 20)); btnHighlighter->setCheckable(true);
    btnMirrorPen = new QToolButton(); btnMirrorPen->setObjectName("btnBrushUnit");
    btnMirrorPen->setIcon(QIcon(resolveAssetPath("mirror.svg"))); btnMirrorPen->setIconSize(QSize(20, 20)); btnMirrorPen->setVisible(false);
    btnLighten = new QToolButton(); btnLighten->setObjectName("btnBrushUnit");
    btnLighten->setIcon(QIcon(resolveAssetPath("lighten.svg"))); btnLighten->setIconSize(QSize(20, 20)); btnLighten->setVisible(false);
    btnPixelStroke = new QToolButton(); btnPixelStroke->setObjectName("btnBrushUnit");
    btnPixelStroke->setIcon(QIcon(resolveAssetPath("pixel_stroke.svg"))); btnPixelStroke->setIconSize(QSize(20, 20)); btnPixelStroke->setVisible(false);

    brushesGridFrame = new QFrame();
    brushesGridFrame->setObjectName("brushesGridFrame");
    QGridLayout *layoutBrushesGrid = new QGridLayout(brushesGridFrame);
    layoutBrushesGrid->setContentsMargins(0, 0, 0, 0); layoutBrushesGrid->setSpacing(3);
    layoutBrushesGrid->addWidget(btnBrushTool, 0, 0); layoutBrushesGrid->addWidget(btnSprayTool, 0, 1);
    layoutBrushesGrid->addWidget(btnCrayonTool, 0, 2); layoutBrushesGrid->addWidget(btnMarkerTool, 0, 3);
    layoutBrushesGrid->addWidget(btnWatercolor, 1, 0); layoutBrushesGrid->addWidget(btnOilBrush, 1, 1);
    layoutBrushesGrid->addWidget(btnCalligraphy, 1, 2); layoutBrushesGrid->addWidget(btnHighlighter, 1, 3);
    layoutToolsMaster->addWidget(brushesGridFrame);

    pixelToolsPanel = new QFrame();
    pixelToolsPanel->setObjectName("pixelToolsPanel");
    pixelToolsPanel->setFixedWidth(44);
    QVBoxLayout *pixelToolsLayout = new QVBoxLayout(pixelToolsPanel);
    pixelToolsLayout->setContentsMargins(0, 0, 0, 0); pixelToolsLayout->setSpacing(3); pixelToolsLayout->setAlignment(Qt::AlignTop);
    pixelToolsLayout->addWidget(btnMirrorPen);
    pixelToolsLayout->addWidget(btnPixelStroke);
    pixelToolsLayout->addWidget(btnLighten);
    pixelToolsPanel->setVisible(false);
    layoutToolsMaster->addWidget(pixelToolsPanel);

    btnCustomToolAction = new QToolButton(); btnCustomToolAction->setObjectName("btnCustomLong");
    btnCustomToolAction->setIcon(QIcon(resolveAssetPath("custom_brush.svg"))); btnCustomToolAction->setIconSize(QSize(32, 32));
    btnCustomToolAction->setToolTip(tr("Pincel Personalizado (1 o 2)")); btnCustomToolAction->setCheckable(true);
    btnCustomToolAction->setVisible(false); btnCustomToolAction->setFixedHeight(90); btnCustomToolAction->setFixedWidth(44);
    layoutToolsMaster->addWidget(btnCustomToolAction);
    ribbonLayout->addWidget(boxBrushesContainer);

    boxShapes = new QGroupBox(tr("Formas")); boxShapes->setFixedWidth(160);
    QVBoxLayout *layoutShapesOuter = new QVBoxLayout(boxShapes); layoutShapesOuter->setContentsMargins(4, 2, 4, 2); layoutShapesOuter->setSpacing(0);
    QFrame *shapesGridFrame = new QFrame(); shapesGridFrame->setObjectName("shapesGridFrame");
    QGridLayout *layoutShapesGrid = new QGridLayout(shapesGridFrame); layoutShapesGrid->setContentsMargins(4, 4, 4, 4); layoutShapesGrid->setSpacing(2); layoutShapesGrid->setAlignment(Qt::AlignCenter);

    QPushButton *btnLine = new QPushButton(); btnLine->setObjectName("btnShapeUnit");
    btnLine->setIcon(QIcon(resolveAssetPath("line.svg"))); btnLine->setIconSize(QSize(20, 20)); btnLine->setCheckable(true);
    QPushButton *btnRect = new QPushButton(); btnRect->setObjectName("btnShapeUnit");
    btnRect->setIcon(QIcon(resolveAssetPath("cuadrado.svg"))); btnRect->setIconSize(QSize(20, 20)); btnRect->setCheckable(true);
    QPushButton *btnEllipse = new QPushButton(); btnEllipse->setObjectName("btnShapeUnit");
    btnEllipse->setIcon(QIcon(resolveAssetPath("circle.svg"))); btnEllipse->setIconSize(QSize(20, 20)); btnEllipse->setCheckable(true);
    QPushButton *btnRoundRect = new QPushButton(); btnRoundRect->setObjectName("btnShapeUnit");
    btnRoundRect->setIcon(QIcon(resolveAssetPath("rectangulo_redondeado.svg"))); btnRoundRect->setIconSize(QSize(20, 20)); btnRoundRect->setCheckable(true);
    QPushButton *btnTriangle = new QPushButton(); btnTriangle->setObjectName("btnShapeUnit");
    btnTriangle->setIcon(QIcon(resolveAssetPath("triangulo.svg"))); btnTriangle->setIconSize(QSize(20, 20)); btnTriangle->setCheckable(true);
    QPushButton *btnRightTri = new QPushButton(); btnRightTri->setObjectName("btnShapeUnit");
    btnRightTri->setIcon(QIcon(resolveAssetPath("triangulo_rectangulo.svg"))); btnRightTri->setIconSize(QSize(20, 20)); btnRightTri->setCheckable(true);
    QPushButton *btnDiamond = new QPushButton(); btnDiamond->setObjectName("btnShapeUnit");
    btnDiamond->setIcon(QIcon(resolveAssetPath("rombo.svg"))); btnDiamond->setIconSize(QSize(20, 20)); btnDiamond->setCheckable(true);
    QPushButton *btnPentagon = new QPushButton(); btnPentagon->setObjectName("btnShapeUnit");
    btnPentagon->setIcon(QIcon(resolveAssetPath("pentagono.svg"))); btnPentagon->setIconSize(QSize(20, 20)); btnPentagon->setCheckable(true);
    QPushButton *btnHexagon = new QPushButton(); btnHexagon->setObjectName("btnShapeUnit");
    btnHexagon->setIcon(QIcon(resolveAssetPath("hexagono.svg"))); btnHexagon->setIconSize(QSize(20, 20)); btnHexagon->setCheckable(true);
    QPushButton *btnArrowRight = new QPushButton(); btnArrowRight->setObjectName("btnShapeUnit");
    btnArrowRight->setIcon(QIcon(resolveAssetPath("flecha_derecha.svg"))); btnArrowRight->setIconSize(QSize(20, 20)); btnArrowRight->setCheckable(true);
    QPushButton *btnArrowLeft = new QPushButton(); btnArrowLeft->setObjectName("btnShapeUnit");
    btnArrowLeft->setIcon(QIcon(resolveAssetPath("flecha_izquierda.svg"))); btnArrowLeft->setIconSize(QSize(20, 20)); btnArrowLeft->setCheckable(true);
    QPushButton *btnStar = new QPushButton(); btnStar->setObjectName("btnShapeUnit");
    btnStar->setIcon(QIcon(resolveAssetPath("estrella.svg"))); btnStar->setIconSize(QSize(20, 20)); btnStar->setCheckable(true);
    QPushButton *btnHeart = new QPushButton(); btnHeart->setObjectName("btnShapeUnit");
    btnHeart->setIcon(QIcon(resolveAssetPath("corazon.svg"))); btnHeart->setIconSize(QSize(20, 20)); btnHeart->setCheckable(true);
    QPushButton *btnCube = new QPushButton(); btnCube->setObjectName("btnShapeUnit");
    btnCube->setIcon(QIcon(resolveAssetPath("cubo2.svg"))); btnCube->setIconSize(QSize(20, 20)); btnCube->setCheckable(true);

    QList<QPushButton*> shapesList = {btnLine, btnRect, btnEllipse, btnRoundRect, btnTriangle, btnRightTri, btnDiamond, btnPentagon, btnHexagon, btnArrowRight, btnArrowLeft, btnStar, btnHeart, btnCube};
    int sr = 0, sc = 0;
    for(auto* b : shapesList) { layoutShapesGrid->addWidget(b, sr, sc); sc++; if(sc > 3) { sc = 0; sr++; } }
    layoutShapesOuter->addWidget(shapesGridFrame);
    ribbonLayout->addWidget(boxShapes);

    QGroupBox *boxSize = new QGroupBox(tr("Propiedades"));
    QVBoxLayout *layoutSize = new QVBoxLayout(boxSize); layoutSize->setContentsMargins(6, 2, 6, 2); layoutSize->setSpacing(2);
    QComboBox *comboOpacity = new QComboBox(); comboOpacity->setObjectName("comboPropiedades");
    comboOpacity->addItem("100%", 255); comboOpacity->addItem("75%", 191); comboOpacity->addItem("50%", 127); comboOpacity->addItem("25%", 64); comboOpacity->setCurrentIndex(0);
    QComboBox *comboSize = new QComboBox(); comboSize->setObjectName("comboPropiedades");
    comboSize->addItem("1 px", 1); comboSize->addItem("3 px", 3); comboSize->addItem("5 px", 5); comboSize->addItem("8 px", 8); comboSize->addItem("12 px", 12); comboSize->setCurrentIndex(1);
    layoutSize->addWidget(new QLabel(tr("Opacidad:"))); layoutSize->addWidget(comboOpacity);
    layoutSize->addWidget(new QLabel(tr("Tamano:"))); layoutSize->addWidget(comboSize);
    ribbonLayout->addWidget(boxSize);

    boxAnimation = new QGroupBox(tr("Animacion"));
    QVBoxLayout *layoutAnimMain = new QVBoxLayout(boxAnimation); layoutAnimMain->setContentsMargins(6, 4, 6, 4); layoutAnimMain->setSpacing(4);
    QHBoxLayout *layoutAnimNav = new QHBoxLayout(); layoutAnimNav->setSpacing(4);
    btnPrevFrame = new QPushButton("<"); btnPrevFrame->setObjectName("btnQuickAction"); btnPrevFrame->setFixedWidth(30);
    btnNextFrame = new QPushButton(">"); btnNextFrame->setObjectName("btnQuickAction"); btnNextFrame->setFixedWidth(30);
    lblFrameIndicator = new QLabel(tr("Frame 1/1")); lblFrameIndicator->setAlignment(Qt::AlignCenter); lblFrameIndicator->setObjectName("lblMini"); lblFrameIndicator->setMinimumWidth(70);
    animSpeedSlider = new QSlider(Qt::Horizontal); animSpeedSlider->setRange(50, 1000); animSpeedSlider->setValue(200); animSpeedSlider->setFixedWidth(80);
    layoutAnimNav->addWidget(btnPrevFrame); layoutAnimNav->addWidget(lblFrameIndicator); layoutAnimNav->addWidget(btnNextFrame);
    layoutAnimNav->addWidget(new QLabel(tr("Vel:"))); layoutAnimNav->addWidget(animSpeedSlider); layoutAnimNav->addStretch();
    QHBoxLayout *layoutFrameActions = new QHBoxLayout(); layoutFrameActions->setSpacing(4);
    btnAddFrame = new QPushButton(tr("+ Frame")); btnAddFrame->setObjectName("btnQuickAction");
    btnDupFrame = new QPushButton(tr("Duplicar")); btnDupFrame->setObjectName("btnQuickAction");
    btnDelFrame = new QPushButton(tr("Eliminar")); btnDelFrame->setObjectName("btnQuickAction");
    btnPlayAnimation = new QPushButton(tr("Play")); btnPlayAnimation->setObjectName("btnQuickAction"); btnPlayAnimation->setCheckable(true);
    btnPlayAnimation->setStyleSheet("QPushButton { background-color: #2563eb; color: white; font-weight: bold; border: none; border-radius: 6px; padding: 4px 12px; } QPushButton:hover { background-color: #1d4ed8; } QPushButton:checked { background-color: #dc2626; }");
    layoutFrameActions->addWidget(btnAddFrame); layoutFrameActions->addWidget(btnDupFrame); layoutFrameActions->addWidget(btnDelFrame);
    layoutFrameActions->addSpacing(12); layoutFrameActions->addWidget(btnPlayAnimation); layoutFrameActions->addStretch();
    framesScrollArea = new QScrollArea(); framesScrollArea->setObjectName("framesScrollArea"); framesScrollArea->setWidgetResizable(true); framesScrollArea->setFixedHeight(80);
    framesScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn); framesScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    framesContainer = new QWidget(); framesContainer->setObjectName("framesContainerWidget");
    framesLayout = new QHBoxLayout(framesContainer); framesLayout->setContentsMargins(2, 2, 2, 2); framesLayout->setSpacing(6); framesLayout->setAlignment(Qt::AlignLeft);
    framesScrollArea->setWidget(framesContainer);
    layoutAnimMain->addLayout(layoutAnimNav); layoutAnimMain->addLayout(layoutFrameActions); layoutAnimMain->addWidget(framesScrollArea);
    boxAnimation->setVisible(false);
    ribbonLayout->addWidget(boxAnimation);

    QGroupBox *boxColors = new QGroupBox(tr("Colores"));
    QHBoxLayout *layoutColorsMain = new QHBoxLayout(boxColors);
    layoutColorsMain->setContentsMargins(6, 4, 6, 4); layoutColorsMain->setSpacing(8); layoutColorsMain->setAlignment(Qt::AlignVCenter);

    QWidget *colorPanelWidget = new QWidget(); colorPanelWidget->setFixedWidth(110);
    QVBoxLayout *colorPanelLayout = new QVBoxLayout(colorPanelWidget);
    colorPanelLayout->setContentsMargins(0, 0, 0, 0); colorPanelLayout->setSpacing(4); colorPanelLayout->setAlignment(Qt::AlignHCenter);

    btnEditColors = new QToolButton(); btnEditColors->setObjectName("btnEditColorsWide");
    btnEditColors->setIcon(QIcon(resolveAssetPath("colors.svg"))); btnEditColors->setIconSize(QSize(18, 18));
    btnEditColors->setText(tr("Editar Colores")); btnEditColors->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    btnEditColors->setMinimumHeight(32); btnEditColors->setFixedWidth(100);
    btnEditColors->setStyleSheet("QToolButton#btnEditColorsWide { font-size: 10px; font-weight: normal; border-radius: 6px; padding: 4px 6px; }");
    connect(btnEditColors, &QToolButton::clicked, this, &mainwind::abrirPaletaAvanzada);
    colorPanelLayout->addWidget(btnEditColors, 0, Qt::AlignHCenter);

    frameSelectorsColor = new QFrame(); frameSelectorsColor->setObjectName("frameSelectorsColor"); frameSelectorsColor->setFixedWidth(100);
    QHBoxLayout *layoutSelectors = new QHBoxLayout(frameSelectorsColor);
    layoutSelectors->setContentsMargins(8, 8, 8, 8); layoutSelectors->setSpacing(12); layoutSelectors->setAlignment(Qt::AlignCenter);
    btnColor1 = new QPushButton(); btnColor1->setFixedSize(30, 50); btnColor1->setCheckable(true); btnColor1->setChecked(true);
    btnColor2 = new QPushButton(); btnColor2->setFixedSize(30, 50); btnColor2->setCheckable(true);
    QButtonGroup *groupTargets = new QButtonGroup(this); groupTargets->addButton(btnColor1, 1); groupTargets->addButton(btnColor2, 2);
    layoutSelectors->addWidget(btnColor1); layoutSelectors->addWidget(btnColor2);
    colorPanelLayout->addWidget(frameSelectorsColor, 0, Qt::AlignHCenter);
    layoutColorsMain->addWidget(colorPanelWidget);

    QWidget *gridWidget = new QWidget(); QGridLayout *gridLayout = new QGridLayout(gridWidget);
    gridLayout->setContentsMargins(0, 0, 0, 0); gridLayout->setSpacing(3);
    QList<QColor> paletaWin = {Qt::black, QColor(127,127,127), QColor(136,0,21), Qt::red, QColor(255,127,39), Qt::yellow, Qt::green, QColor(0,162,232), QColor(63,72,204), QColor(163,73,164), Qt::white, QColor(195,195,195), QColor(185,122,87), QColor(255,174,201), QColor(255,201,14), QColor(239,228,176), QColor(181,230,29), QColor(153,217,234), QColor(112,146,190), QColor(200,191,231)};
    for (int i = 0; i < paletaWin.size(); ++i) {
        QPushButton *btnC = new QPushButton(); btnC->setFixedSize(18, 18);
        btnC->setStyleSheet(QString("background-color: %1; border: 1px solid rgba(0,0,0,0.25); border-radius: 3px;").arg(paletaWin[i].name()));
        connect(btnC, &QPushButton::clicked, this, [this, paletaWin, i]() { inyectarColorAObjeto(paletaWin[i]); });
        gridLayout->addWidget(btnC, i / 10, i % 10);
    }
    for (int i = 0; i < 10; ++i) {
        QPushButton *btnRecienteSlot = new QPushButton(); btnRecienteSlot->setFixedSize(18, 18);
        btnRecienteSlot->setStyleSheet("background-color: #ffffff; border: 1px dashed rgba(0,0,0,0.4); border-radius: 3px;");
        btnRecienteSlot->setProperty("customColor", QColor(Qt::white));
        connect(btnRecienteSlot, &QPushButton::clicked, this, [this, btnRecienteSlot]() { inyectarColorAObjeto(btnRecienteSlot->property("customColor").value<QColor>()); });
        gridLayout->addWidget(btnRecienteSlot, 2, i);
        listaBotonesRecientes.append(btnRecienteSlot);
    }
    layoutColorsMain->addWidget(gridWidget);
    ribbonLayout->addWidget(boxColors);
    ribbonLayout->addStretch();
    centralLayout->addWidget(ribbonWidget);

    // === CONTENT AREA ===
    contentAreaWidget = new QWidget();
    QHBoxLayout *contentLayout = new QHBoxLayout(contentAreaWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0); contentLayout->setSpacing(0);

    leftSidebarWidget = new QWidget(); leftSidebarWidget->setObjectName("leftSidebarWidget"); leftSidebarWidget->setFixedWidth(220);
    QVBoxLayout *leftSidebarLayout = new QVBoxLayout(leftSidebarWidget);
    leftSidebarLayout->setContentsMargins(6, 6, 6, 6); leftSidebarLayout->setSpacing(6);

    boxAdvTools = new QGroupBox(tr("Seleccion y Retoque"));
    QGridLayout *gridAdv = new QGridLayout(boxAdvTools); gridAdv->setContentsMargins(4, 12, 4, 4); gridAdv->setSpacing(4);
    btnMagicWand = new QPushButton(); btnMagicWand->setIcon(QIcon(resolveAssetPath("magic_wand.svg"))); btnMagicWand->setCheckable(true); btnMagicWand->setObjectName("btnToolGrid");

    btnLassoTool = new QPushButton();
    btnLassoTool->setObjectName("btnToolGrid");
    btnLassoTool->setCheckable(true);
    btnLassoTool->setIcon(QIcon(resolveAssetPath("lasso_extract.svg")));
    btnLassoTool->setContextMenuPolicy(Qt::CustomContextMenu);
    menuLassoModos = new QMenu(this);
    actLassoAdentro = menuLassoModos->addAction(tr("Recortar hacia ADENTRO (extraer)"));
    actLassoAfuera  = menuLassoModos->addAction(tr("Recortar hacia AFUERA (hueco)"));
    actLassoAdentro->setCheckable(true);
    actLassoAfuera->setCheckable(true);
    actualizarBotonLazo();

    btnDeformTool = new QPushButton();
    btnDeformTool->setIcon(crearIconoDeformacion(darkMode));
    btnDeformTool->setIconSize(QSize(18, 18));
    btnDeformTool->setCheckable(true);
    btnDeformTool->setObjectName("btnToolGrid");
    btnDeformTool->setToolTip(tr("Pincel de Deformacion (empujar / girar / inflar)"));

    btnBlurTool = new QPushButton(); btnBlurTool->setIcon(QIcon(resolveAssetPath("blur.svg"))); btnBlurTool->setCheckable(true); btnBlurTool->setObjectName("btnToolGrid");
    btnHealTool = new QPushButton(); btnHealTool->setIcon(QIcon(resolveAssetPath("heal.svg"))); btnHealTool->setCheckable(true); btnHealTool->setObjectName("btnToolGrid");
    btnShadowTool = new QPushButton(); btnShadowTool->setIcon(QIcon(resolveAssetPath("burn.svg"))); btnShadowTool->setCheckable(true); btnShadowTool->setObjectName("btnToolGrid");
    btnGradientTool = new QPushButton(); btnGradientTool->setIcon(QIcon(resolveAssetPath("gradient.svg"))); btnGradientTool->setCheckable(true); btnGradientTool->setObjectName("btnToolGrid");
    btnGradientTool->setToolTip(tr("Gradiente"));
    btnCloneTool = new QPushButton(); btnCloneTool->setIcon(QIcon(resolveAssetPath("clone.svg"))); btnCloneTool->setCheckable(true); btnCloneTool->setObjectName("btnToolGrid");
    btnCloneTool->setToolTip(tr("Clonar"));
    btnZoomToolSidebar = new QPushButton(); btnZoomToolSidebar->setIcon(QIcon(resolveAssetPath("lupa.svg"))); btnZoomToolSidebar->setCheckable(true); btnZoomToolSidebar->setObjectName("btnToolGrid");
    btnZoomToolSidebar->setToolTip(tr("Lupa"));

    gridAdv->addWidget(btnMagicWand, 0, 0); gridAdv->addWidget(btnLassoTool, 0, 1); gridAdv->addWidget(btnDeformTool, 0, 2);
    gridAdv->addWidget(btnBlurTool, 1, 0); gridAdv->addWidget(btnHealTool, 1, 1); gridAdv->addWidget(btnShadowTool, 1, 2);
    gridAdv->addWidget(btnGradientTool, 2, 0); gridAdv->addWidget(btnCloneTool, 2, 1); gridAdv->addWidget(btnZoomToolSidebar, 2, 2);
    leftSidebarLayout->addWidget(boxAdvTools);

    // ------------------------------------------------------------
    // Vectores: se mantiene como sección propia, PERO ahora es
    // más compacto (botón horizontal) y se coloca JUSTO debajo
    // de "Seleccion y Retoque". Se quita el stretch intermedio
    // para que en pantallas 720p no se corte.
    // ------------------------------------------------------------
    boxVectorTools = new QGroupBox(tr("Vectores"));
    QVBoxLayout *layoutVecTools = new QVBoxLayout(boxVectorTools);
    layoutVecTools->setContentsMargins(4, 12, 4, 4);
    layoutVecTools->setSpacing(4);
    btnPenBezier = new QToolButton();
    btnPenBezier->setObjectName("btnVectorTool");
    btnPenBezier->setIcon(QIcon(resolveAssetPath("pen_bezier.svg")));
    btnPenBezier->setIconSize(QSize(22, 22));
    btnPenBezier->setCheckable(true);
    btnPenBezier->setText(tr("Pluma Bezier"));
    btnPenBezier->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    btnPenBezier->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    btnPenBezier->setMinimumHeight(34);
    layoutVecTools->addWidget(btnPenBezier);
    leftSidebarLayout->addWidget(boxVectorTools);

    boxLayersContainer = new QGroupBox(tr("Capas"));
    QVBoxLayout *layoutLayersContainer = new QVBoxLayout(boxLayersContainer);
    layoutLayersContainer->setContentsMargins(4, 12, 4, 4); layoutLayersContainer->setSpacing(4);
    QHBoxLayout *layoutLayerBtns = new QHBoxLayout(); layoutLayerBtns->setSpacing(2);
    btnAddLayer = new QPushButton("+"); btnAddLayer->setObjectName("btnQuickAction"); btnAddLayer->setFixedWidth(25);
    btnDupLayer = new QPushButton(QString::fromUtf8("\xE2\xA7\x89")); btnDupLayer->setObjectName("btnQuickAction"); btnDupLayer->setFixedWidth(25);
    btnDupLayer->setToolTip(tr("Duplicar capa"));
    btnDelLayer = new QPushButton(QString::fromUtf8("\xE2\x9C\x95")); btnDelLayer->setObjectName("btnQuickAction"); btnDelLayer->setFixedWidth(25);
    btnDelLayer->setFont(QFont("Adwaita Sans", 12, QFont::Bold));
    btnDelLayer->setToolTip(tr("Eliminar capa"));
    btnLayerMask = new QPushButton(); btnLayerMask->setObjectName("btnQuickAction"); btnLayerMask->setFixedWidth(25);
    btnLayerMask->setIcon(crearIconoMascara(darkMode));
    btnLayerMask->setIconSize(QSize(18, 18));
    btnLayerMask->setToolTip(tr("Máscaras de capa (B/N y COLOR)"));
    menuLayerMask = new QMenu(this);
    actAddMask = menuLayerMask->addAction(tr("Añadir máscara de capa"));
    actDelMask = menuLayerMask->addAction(tr("Eliminar máscara de capa"));
    actToggleMask = menuLayerMask->addAction(tr("Desactivar máscara"));
    menuLayerMask->addSeparator();
    actInvertMask = menuLayerMask->addAction(tr("Invertir máscara"));
    actApplyMask = menuLayerMask->addAction(tr("Aplicar máscara de capa"));
    menuLayerMask->addSeparator();
    actAddColorMask = menuLayerMask->addAction(tr("Añadir MÁSCARA DE COLOR (filtros)..."));
    actAddColorMask->setToolTip(tr("Abre la ventana flotante de filtros. Se aplica EN TIEMPO REAL mientras ajustas y puedes pintar encima."));
    actDelColorMask = menuLayerMask->addAction(tr("Eliminar máscara de color"));
    actToggleColorMask = menuLayerMask->addAction(tr("Desactivar máscara de color"));
    btnLayerMask->setMenu(menuLayerMask);
    layoutLayerBtns->addWidget(btnAddLayer); layoutLayerBtns->addWidget(btnDupLayer);
    layoutLayerBtns->addWidget(btnDelLayer); layoutLayerBtns->addWidget(btnLayerMask);
    layoutLayersContainer->addLayout(layoutLayerBtns);

    layersScrollArea = new QScrollArea(); layersScrollArea->setObjectName("layersScrollArea"); layersScrollArea->setWidgetResizable(true);
    layersScrollArea->setMinimumHeight(80); layersScrollArea->setMaximumHeight(250);
    layersContainer = new QWidget(); layersContainer->setObjectName("layersContainerWidget");
    layersLayout = new QVBoxLayout(layersContainer); layersLayout->setContentsMargins(2, 2, 2, 2); layersLayout->setSpacing(4); layersLayout->setAlignment(Qt::AlignTop);
    layersScrollArea->setWidget(layersContainer);
    layoutLayersContainer->addWidget(layersScrollArea);

    QHBoxLayout *layoutOrderBtns = new QHBoxLayout(); layoutOrderBtns->setSpacing(2);
    btnMoveLayerUp = new QPushButton(QString::fromUtf8("\xE2\x96\xB2")); btnMoveLayerUp->setObjectName("btnQuickAction"); btnMoveLayerUp->setFixedWidth(30);
    btnMoveLayerUp->setToolTip(tr("Mover capa arriba"));
    btnMoveLayerDown = new QPushButton(QString::fromUtf8("\xE2\x96\xBC")); btnMoveLayerDown->setObjectName("btnQuickAction"); btnMoveLayerDown->setFixedWidth(30);
    btnMoveLayerDown->setToolTip(tr("Mover capa abajo"));
    layoutOrderBtns->addWidget(btnMoveLayerUp); layoutOrderBtns->addWidget(btnMoveLayerDown); layoutOrderBtns->addStretch();
    layoutLayersContainer->addLayout(layoutOrderBtns);

    lblCurrentLayer = new QLabel(tr("Capa: 1")); lblCurrentLayer->setObjectName("lblMini"); layoutLayersContainer->addWidget(lblCurrentLayer);
    sliderLayerOpacity = new QSlider(Qt::Horizontal); sliderLayerOpacity->setRange(0, 100); sliderLayerOpacity->setValue(100);
    layoutLayersContainer->addWidget(new QLabel(tr("Opacidad:"))); layoutLayersContainer->addWidget(sliderLayerOpacity);
    comboBlendMode = new QComboBox();
    comboBlendMode->addItem(tr("Normal"), 0); comboBlendMode->addItem(tr("Multiplicar"), 1); comboBlendMode->addItem(tr("Trama"), 2);
    comboBlendMode->addItem(tr("Superponer"), 3); comboBlendMode->addItem(tr("Luz suave"), 4); comboBlendMode->addItem(tr("Diferencia"), 5);
    layoutLayersContainer->addWidget(new QLabel(tr("Fusion:"))); layoutLayersContainer->addWidget(comboBlendMode);
    chkLayerLocked = new QCheckBox(tr("Bloquear capa")); layoutLayersContainer->addWidget(chkLayerLocked);

    leftSidebarLayout->addWidget(boxLayersContainer); leftSidebarLayout->addStretch();
    leftSidebarWidget->setVisible(false);

    // === WORK AREA ===
    QWidget *workArea = new QWidget(); workArea->setObjectName("workAreaContainer"); workArea->setAttribute(Qt::WA_StyledBackground, true);
    QVBoxLayout *workLayout = new QVBoxLayout(workArea); workLayout->setContentsMargins(0, 0, 0, 0); workLayout->setSpacing(0);

    scrollArea = new QScrollArea();
    scrollArea->setAlignment(Qt::AlignCenter);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setObjectName("mainScrollArea");
    scrollArea->setStyleSheet("QScrollArea#mainScrollArea { background-color: transparent; border: none; }");
    scrollArea->viewport()->setAttribute(Qt::WA_TranslucentBackground, true);
    scrollArea->viewport()->setAutoFillBackground(false);
    scrollArea->viewport()->setStyleSheet("background-color: transparent;");

    paintArea = new PaintArea();
    paintArea->setDarkMode(darkMode);
    scrollArea->setWidget(paintArea);
    workLayout->addWidget(scrollArea);

    bottomBarWidget = new QWidget(); bottomBarWidget->setObjectName("bottomBarContainer"); bottomBarWidget->setFixedHeight(28);
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBarWidget);
    bottomLayout->setContentsMargins(8, 0, 8, 0); bottomLayout->setSpacing(6);

    lblResolutionIndicator = new QLabel(); lblResolutionIndicator->setObjectName("lblZoomIndicator");
    lblZoomIndicator = new QLabel(tr("Zoom: 50%")); lblZoomIndicator->setObjectName("lblZoomIndicator");

    btnRotateFlip = new QPushButton(); btnRotateFlip->setObjectName("btnQuickIcon");
    btnRotateFlip->setIcon(QIcon(resolveAssetPath("rotate_flip.svg"))); btnRotateFlip->setIconSize(QSize(16, 16)); btnRotateFlip->setFixedSize(22, 22);
    QMenu *menuRotate = new QMenu(this);
    menuRotate->addAction(tr("Rotar 90 Derecha"), this, [this](){ paintArea->rotateCurrentLayer(90); });
    menuRotate->addAction(tr("Rotar 90 Izquierda"), this, [this](){ paintArea->rotateCurrentLayer(-90); });
    menuRotate->addAction(tr("Rotar 180"), this, [this](){ paintArea->rotateCurrentLayer(180); });
    menuRotate->addSeparator();
    menuRotate->addAction(tr("Voltear Horizontal"), this, [this](){ paintArea->flipCurrentLayer(true, false); });
    menuRotate->addAction(tr("Voltear Vertical"), this, [this](){ paintArea->flipCurrentLayer(false, true); });
    btnRotateFlip->setMenu(menuRotate);

    btnZoomLess = new QPushButton(QString::fromUtf8("\xE2\x88\x92")); btnZoomLess->setObjectName("btnZoomAction"); btnZoomLess->setFixedSize(20, 20); btnZoomLess->setFont(QFont("Adwaita Sans", 8));
    comboZoom = new QComboBox(); comboZoom->setObjectName("comboZoomBottom"); comboZoom->setFixedWidth(62); comboZoom->setFixedHeight(20); comboZoom->setFont(QFont("Adwaita Sans", 8));
    comboZoom->addItem("12.5%", 0.125); comboZoom->addItem("25%", 0.25); comboZoom->addItem("50%", 0.50);
    comboZoom->addItem("100%", 1.0); comboZoom->addItem("200%", 2.0); comboZoom->addItem("400%", 4.0);
    comboZoom->addItem("800%", 8.0); comboZoom->addItem("1600%", 16.0); comboZoom->addItem("3200%", 32.0); comboZoom->setCurrentIndex(2);
    btnZoomMore = new QPushButton("+"); btnZoomMore->setObjectName("btnZoomAction"); btnZoomMore->setFixedSize(20, 20); btnZoomMore->setFont(QFont("Adwaita Sans", 8, QFont::Bold));

    bottomLayout->addWidget(lblResolutionIndicator); bottomLayout->addWidget(lblZoomIndicator); bottomLayout->addStretch();
    bottomLayout->addWidget(btnRotateFlip); bottomLayout->addSpacing(2);
    bottomLayout->addWidget(btnZoomLess); bottomLayout->addSpacing(1);
    bottomLayout->addWidget(comboZoom); bottomLayout->addSpacing(1); bottomLayout->addWidget(btnZoomMore);
    workLayout->addWidget(bottomBarWidget);

    contentLayout->addWidget(leftSidebarWidget); contentLayout->addWidget(workArea);
    centralLayout->addWidget(contentAreaWidget);
    setCentralWidget(centralWidget);

    // === LISTA DE BOTONES DE HERRAMIENTAS ===
    listaBotonesHerramientas << btnPencil << btnEraser << btnBucket << btnMirrorPen << btnPicker << btnText
                             << btnSelectBox << btnSelectFree << btnLine << btnRect << btnEllipse << btnRoundRect
                             << btnTriangle << btnRightTri << btnDiamond << btnPentagon << btnHexagon
                             << btnArrowRight << btnArrowLeft << btnStar << btnHeart << btnZoomToolSidebar
                             << btnBrushTool << btnCustomToolAction << btnSprayTool << btnCrayonTool << btnLighten
                             << btnMarkerTool << btnPixelStroke << btnCube << btnWatercolor << btnOilBrush
                             << btnCalligraphy << btnHighlighter << btnMagicWand << btnLassoTool << btnDeformTool
                             << btnPenBezier << btnBlurTool << btnHealTool << btnShadowTool
                             << btnGradientTool << btnCloneTool << btnMoveTool;

    auto resetToolButtons = [this](QAbstractButton* activo) {
        for (QAbstractButton* btn : listaBotonesHerramientas) btn->setChecked(false);
        activo->setChecked(true);
        if (deformSettingsBar) {
            if (activo == btnDeformTool) {
                QPoint g = mapToGlobal(QPoint(leftSidebarWidget->isVisible() ? leftSidebarWidget->width() + 12 : 236,
                                              sysBarWidget->height() + ribbonWidget->height() + 8));
                deformSettingsBar->move(g);
                deformSettingsBar->show();
                deformSettingsBar->raise();
            } else {
                deformSettingsBar->hide();
            }
        }
        paintArea->setFocus();
    };

    // === CONEXIONES DE HERRAMIENTAS ===
    connect(btnPencil, &QPushButton::clicked, this, [=, this]() { resetToolButtons(btnPencil); paintArea->setTool(ToolPencil); });
    connect(btnEraser, &QPushButton::clicked, this, [=, this]() { resetToolButtons(btnEraser); paintArea->setTool(ToolEraser); });
    connect(btnBucket, &QPushButton::clicked, this, [=, this]() { resetToolButtons(btnBucket); paintArea->setTool(ToolBucket); });
    connect(btnMirrorPen, &QPushButton::clicked, this, [=, this]() { resetToolButtons(btnMirrorPen); paintArea->setTool(ToolMirrorPen); });
    connect(btnPicker, &QPushButton::clicked, this, [=, this]() { resetToolButtons(btnPicker); paintArea->setTool(ToolPicker); });
    connect(btnText, &QPushButton::clicked, this, [=, this]() { resetToolButtons(btnText); paintArea->setTool(ToolText); });
    connect(btnSelectBox, &QPushButton::clicked, this, [=, this]() { resetToolButtons(btnSelectBox); paintArea->setTool(ToolSelect); });
    connect(btnSelectFree, &QPushButton::clicked, this, [=, this]() { resetToolButtons(btnSelectFree); paintArea->setTool(ToolSelectFree); statusBar()->showMessage(tr("Seleccion Libre"), 2000); });
    connect(btnZoomToolSidebar, &QPushButton::clicked, this, [=, this]() { resetToolButtons(btnZoomToolSidebar); paintArea->setTool(ToolZoom); statusBar()->showMessage(tr("Lupa"), 2000); });
    connect(btnBrushTool, &QToolButton::clicked, this, [=, this]() { resetToolButtons(btnBrushTool); paintArea->setTool(ToolBrush); });
    connect(btnSprayTool, &QToolButton::clicked, this, [=, this]() { resetToolButtons(btnSprayTool); paintArea->setTool(ToolSpray); });
    connect(btnCrayonTool, &QToolButton::clicked, this, [=, this]() { resetToolButtons(btnCrayonTool); paintArea->setTool(ToolCrayon); });
    connect(btnLighten, &QToolButton::clicked, this, [=, this]() { resetToolButtons(btnLighten); paintArea->setTool(ToolLighten); });
    connect(btnMarkerTool, &QToolButton::clicked, this, [=, this]() { resetToolButtons(btnMarkerTool); paintArea->setTool(ToolMarker); });
    connect(btnPixelStroke, &QToolButton::clicked, this, [=, this]() { resetToolButtons(btnPixelStroke); paintArea->setTool(ToolPixelStroke); });
    connect(btnCustomToolAction, &QToolButton::clicked, this, [=, this]() { resetToolButtons(btnCustomToolAction); paintArea->setTool(ToolCustomBrush); });
    connect(btnWatercolor, &QToolButton::clicked, this, [=, this]() { resetToolButtons(btnWatercolor); paintArea->setTool(ToolWatercolor); });
    connect(btnOilBrush, &QToolButton::clicked, this, [=, this]() { resetToolButtons(btnOilBrush); paintArea->setTool(ToolOilBrush); });
    connect(btnCalligraphy, &QToolButton::clicked, this, [=, this]() { resetToolButtons(btnCalligraphy); paintArea->setTool(ToolCalligraphy); });
    connect(btnHighlighter, &QToolButton::clicked, this, [=, this]() { resetToolButtons(btnHighlighter); paintArea->setTool(ToolHighlighter); });

    const std::vector<ToolType> shapeTools = {ToolLine, ToolRectangle, ToolEllipse, ToolRoundRect, ToolTriangle, ToolRightTriangle, ToolDiamond, ToolPentagon, ToolHexagon, ToolArrowRight, ToolArrowLeft, ToolStar, ToolHeart, ToolCube};
    for (int i = 0; i < shapesList.size(); ++i) {
        ToolType t = shapeTools[i]; QPushButton* btn = shapesList[i];
        connect(btn, &QPushButton::clicked, this, [=, this]() { resetToolButtons(btn); paintArea->setTool(t); });
    }

    connect(btnMagicWand, &QPushButton::clicked, this, [=, this]() { resetToolButtons(btnMagicWand); paintArea->setTool(ToolMagicWand); });
    connect(btnLassoTool, &QWidget::customContextMenuRequested, this, [this](const QPoint &p) { menuLassoModos->popup(btnLassoTool->mapToGlobal(p)); });
    connect(actLassoAdentro, &QAction::triggered, this, [=, this]() { lassoModoActual = 0; actualizarBotonLazo(); resetToolButtons(btnLassoTool); paintArea->setTool(ToolLassoExtract); statusBar()->showMessage(tr("Lazo: recortar hacia ADENTRO (extraer)"), 2500); });
    connect(actLassoAfuera, &QAction::triggered, this, [=, this]() { lassoModoActual = 1; actualizarBotonLazo(); resetToolButtons(btnLassoTool); paintArea->setTool(ToolLassoDelete); statusBar()->showMessage(tr("Lazo: recortar hacia AFUERA (hueco)"), 2500); });
    connect(btnLassoTool, &QPushButton::clicked, this, [=, this]() {
        resetToolButtons(btnLassoTool);
        if (lassoModoActual == 0) { paintArea->setTool(ToolLassoExtract); statusBar()->showMessage(tr("Lazo: recortar hacia ADENTRO (extraer)"), 2500); }
        else { paintArea->setTool(ToolLassoDelete); statusBar()->showMessage(tr("Lazo: recortar hacia AFUERA (hueco)"), 2500); }
    });
    connect(btnDeformTool, &QPushButton::clicked, this, [=, this]() { resetToolButtons(btnDeformTool); paintArea->setTool(ToolDeform); statusBar()->showMessage(tr("Pincel de Deformacion: arrastra para deformar"), 2500); });
    connect(btnPenBezier, &QToolButton::clicked, this, [=, this]() { resetToolButtons(btnPenBezier); paintArea->setTool(ToolPenBezier); });
    connect(btnBlurTool, &QPushButton::clicked, this, [=, this]() { resetToolButtons(btnBlurTool); paintArea->setTool(ToolBlur); });
    connect(btnHealTool, &QPushButton::clicked, this, [=, this]() { resetToolButtons(btnHealTool); paintArea->setTool(ToolHeal); });
    connect(btnShadowTool, &QPushButton::clicked, this, [=, this]() { resetToolButtons(btnShadowTool); paintArea->setTool(ToolShadowBurn); });
    connect(btnGradientTool, &QPushButton::clicked, this, [=, this]() { resetToolButtons(btnGradientTool); paintArea->setTool(ToolGradient); statusBar()->showMessage(tr("Gradiente"), 2000); });
    connect(btnCloneTool, &QPushButton::clicked, this, [=, this]() { resetToolButtons(btnCloneTool); paintArea->setTool(ToolClone); paintArea->resetCloneSource(); statusBar()->showMessage(tr("Clonar"), 2000); });
    connect(btnMoveTool, &QPushButton::clicked, this, [=, this]() { resetToolButtons(btnMoveTool); paintArea->setTool(ToolMove); statusBar()->showMessage(tr("Mover"), 2000); });

    // === ZOOM ===
    connect(comboZoom, &QComboBox::currentIndexChanged, this, [this](int idx) { if(idx>=0) paintArea->setZoomFactor(comboZoom->itemData(idx).toDouble()); });
    connect(btnZoomLess, &QPushButton::clicked, this, [this]() { QPoint center = scrollArea->viewport()->rect().center(); handleZoomRequest(paintArea->getZoomFactor() / 2.0, center); });
    connect(btnZoomMore, &QPushButton::clicked, this, [this]() { QPoint center = scrollArea->viewport()->rect().center(); handleZoomRequest(paintArea->getZoomFactor() * 2.0, center); });
    connect(paintArea, &PaintArea::zoomChanged, this, &mainwind::refrescarDatosZoomUI);
    connect(paintArea, &PaintArea::zoomRequested, this, &mainwind::handleZoomRequest);

    // === PROPIEDADES ===
    connect(comboSize, &QComboBox::currentIndexChanged, this, [this, comboSize](int idx) { paintArea->setPenWidth(comboSize->itemData(idx).toInt()); });
    connect(comboOpacity, &QComboBox::currentIndexChanged, this, [this, comboOpacity](int idx) { paintArea->setPenOpacity(comboOpacity->itemData(idx).toInt()); });

    // === GRID ===
    connect(gOff, &QAction::triggered, this, [this]() { paintArea->setGridActive(false); });
    connect(g8, &QAction::triggered, this, [this]() { paintArea->setGridSize(8); paintArea->setGridActive(true); });
    connect(g16, &QAction::triggered, this, [this]() { paintArea->setGridSize(16); paintArea->setGridActive(true); });
    connect(g32, &QAction::triggered, this, [this]() { paintArea->setGridSize(32); paintArea->setGridActive(true); });
    connect(g64, &QAction::triggered, this, [this]() { paintArea->setGridSize(64); paintArea->setGridActive(true); });

    // === ANIMACION ===
    connect(btnAddFrame, &QPushButton::clicked, this, [this]() { paintArea->addFrame(); });
    connect(btnDupFrame, &QPushButton::clicked, this, [this]() { paintArea->duplicateFrame(); });
    connect(btnDelFrame, &QPushButton::clicked, this, [this]() { paintArea->deleteFrame(); });
    connect(btnPrevFrame, &QPushButton::clicked, this, [this]() { paintArea->prevFrame(); });
    connect(btnNextFrame, &QPushButton::clicked, this, [this]() { paintArea->nextFrame(); });
    connect(btnPlayAnimation, &QPushButton::clicked, this, [this]() { if (isAnimating) detenerAnimacion(); else iniciarAnimacion(); });
    connect(animSpeedSlider, &QSlider::valueChanged, this, [this](int v) { animSpeed = v; if (isAnimating) animTimer->setInterval(animSpeed); });
    connect(animTimer, &QTimer::timeout, this, [this]() {
        const QList<QImage>& frames = paintArea->getFrames();
        if (frames.size() <= 1) { detenerAnimacion(); return; }
        paintArea->goToFrame((paintArea->getCurrentFrameIndex() + 1) % frames.size());
    });
    connect(paintArea, &PaintArea::framesChanged, this, &mainwind::actualizarMiniaturasFrames);
    connect(paintArea, &PaintArea::layersChanged, this, [this]() { solicitarRefreshCapas(); });

    // === CAPAS ===
    connect(btnAddLayer, &QPushButton::clicked, this, [this]() { paintArea->addLayer(); });
    connect(btnDupLayer, &QPushButton::clicked, this, [this]() { paintArea->duplicateLayer(); });
    connect(btnDelLayer, &QPushButton::clicked, this, [this]() { paintArea->deleteLayer(); });
    connect(btnMoveLayerUp, &QPushButton::clicked, this, [this]() { paintArea->moveLayerUp(); });
    connect(btnMoveLayerDown, &QPushButton::clicked, this, [this]() { paintArea->moveLayerDown(); });
    connect(sliderLayerOpacity, &QSlider::valueChanged, this, [this](int v) { paintArea->setLayerOpacity(paintArea->getCurrentLayerIndex(), v/100.0); });
    connect(comboBlendMode, &QComboBox::currentIndexChanged, this, [this](int idx) { paintArea->setLayerBlendMode(paintArea->getCurrentLayerIndex(), comboBlendMode->itemData(idx).toInt()); });
    connect(chkLayerLocked, &QCheckBox::toggled, this, [this](bool l) { paintArea->setLayerLocked(paintArea->getCurrentLayerIndex(), l); });

    // === MASCARAS ===
    connect(actAddMask, &QAction::triggered, this, [this]() { int idx = paintArea->getCurrentLayerIndex(); if (idx >= 0) paintArea->addLayerMask(idx); });
    connect(actDelMask, &QAction::triggered, this, [this]() { int idx = paintArea->getCurrentLayerIndex(); if (idx >= 0) paintArea->removeLayerMask(idx); });
    connect(actToggleMask, &QAction::triggered, this, [this]() { int idx = paintArea->getCurrentLayerIndex(); if (idx >= 0) paintArea->toggleLayerMaskEnabled(idx); });
    connect(actInvertMask, &QAction::triggered, this, [this]() { int idx = paintArea->getCurrentLayerIndex(); if (idx >= 0) paintArea->invertLayerMask(idx); });
    connect(actApplyMask, &QAction::triggered, this, [this]() { int idx = paintArea->getCurrentLayerIndex(); if (idx >= 0) paintArea->applyMaskToLayer(idx); });
    connect(actAddColorMask, &QAction::triggered, this, [this]() { int idx = paintArea->getCurrentLayerIndex(); if (idx >= 0) abrirEditorMascaraColor(idx); });
    connect(actDelColorMask, &QAction::triggered, this, [this]() { int idx = paintArea->getCurrentLayerIndex(); if (idx >= 0) paintArea->removeLayerColorMask(idx); });
    connect(actToggleColorMask, &QAction::triggered, this, [this]() { int idx = paintArea->getCurrentLayerIndex(); if (idx >= 0) paintArea->toggleLayerColorMaskEnabled(idx); });

    // === STATUS BAR / COLORES / CLIPBOARD ===
    connect(paintArea, &PaintArea::statusBarMessage, this, [this](const QString &m) { statusBar()->showMessage(m, 3000); });
    connect(groupTargets, &QButtonGroup::idClicked, this, [this](int id) { colorObjetivoActivo = id; paintArea->setActiveColorTarget(id); actualizarEstilosDePrevisualizacion(); });
    connect(paintArea, &PaintArea::colorPicked, this, &mainwind::sincronizarGoteroUI);
    connect(btnCopy, &QPushButton::clicked, this, [this]() { paintArea->copiarSeleccion(); });
    connect(btnCut, &QPushButton::clicked, this, [this]() { paintArea->cortarSeleccion(); });
    connect(btnPaste, &QToolButton::clicked, this, [=, this]() { resetToolButtons(btnSelectBox); paintArea->pegarClipboard(); });

    // === ARCHIVO ===
    connect(actNuevo, &QAction::triggered, this, [this]() {
        NewCanvasDialog dlg(darkMode, this);
        if (dlg.exec() == QDialog::Accepted) {
            paintArea->crearNuevoLienzo(dlg.getWidth(), dlg.getHeight(), dlg.isTransparent());
            statusBar()->showMessage(tr("Nuevo lienzo: %1 x %2 px").arg(dlg.getWidth()).arg(dlg.getHeight()), 2000);
        }
    });
    connect(actInsertarImagen, &QAction::triggered, this, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Insertar imagen como objeto"), QDir::currentPath(), tr("Imagenes (*.png *.jpg *.jpeg *.bmp *.gif)"));
        if (!fileName.isEmpty()) {
            QImage img = cargarImagenRespetandoExif(fileName);
            if (!img.isNull()) { paintArea->insertImageAsObject(img); statusBar()->showMessage(tr("Imagen insertada como objeto. Usa la herramienta Mover para escalar/rotar."), 4000); }
            else QMessageBox::warning(this, tr("Error"), tr("No se pudo cargar la imagen."));
        }
    });
    connect(actInsertarComoCapa, &QAction::triggered, this, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Insertar imagen como capa nueva"), QDir::currentPath(), tr("Imagenes (*.png *.jpg *.jpeg *.bmp *.gif)"));
        if (!fileName.isEmpty()) {
            QImage img = cargarImagenRespetandoExif(fileName);
            if (!img.isNull()) { paintArea->addImageLayer(img); statusBar()->showMessage(tr("Imagen insertada como capa nueva"), 2000); }
            else QMessageBox::warning(this, tr("Error"), tr("No se pudo cargar la imagen."));
        }
    });
    connect(actAbrirFondo, &QAction::triggered, this, &mainwind::subirImagenDisco);
    connect(actGuardarPNG, &QAction::triggered, this, [this]() { guardarComo("png"); });
    connect(actGuardarJPG, &QAction::triggered, this, [this]() { guardarComo("jpg"); });
    connect(actGuardarSVG, &QAction::triggered, this, [this]() { guardarComo("svg"); });
    connect(actExportGif, &QAction::triggered, this, [this]() { exportarGif(); });
    connect(actSalir, &QAction::triggered, this, [this]() { if (preguntarGuardarCambios()) close(); });

    // === PINCELES PERSONALIZADOS ===
    connect(btnCustomBrushesTop, &QPushButton::clicked, this, [=, this]() {
        CustomBrushesDialog dialog(darkMode, paintArea->getCustomBrush(0), paintArea->getCustomBrush(1), paintArea->getActiveCustomBrushIndex(), this);
        dialog.setPreviewColor(paintArea->getPenColor1());
        dialog.setPreviewSecondColor(paintArea->getPenColor2());
        if (dialog.exec() == QDialog::Accepted) {
            paintArea->setCustomBrushPresets(dialog.getPreset1(), dialog.getPreset2(), dialog.getActivePresetIndex());
            statusBar()->showMessage(tr("Pincel %1").arg(dialog.getActivePresetIndex() + 1), 2000);
            resetToolButtons(btnCustomToolAction); paintArea->setTool(ToolCustomBrush);
            btnCustomToolAction->setToolTip(tr("Pincel %1").arg(dialog.getActivePresetIndex() + 1));
        }
    });

    // === MODOS ===
    connect(actNormal, &QAction::triggered, this, [=, this]() {
        if (cambiarModo("Normal")) {
            QSize oldSize = this->size(); detenerAnimacion();
            leftSidebarWidget->setVisible(false);
            boxShapes->setVisible(true); boxAnimation->setVisible(false);
            btnImageFilters->setVisible(false); btnCustomBrushesTop->setVisible(false); btnCustomToolAction->setVisible(false); btnCanvasSize->setMenu(menuCanvasSizeStd); btnCanvasSize->setVisible(false);
            if (actExportGif) actExportGif->setVisible(false);
            actInsertarImagen->setVisible(true); actInsertarComoCapa->setVisible(false); actAbrirFondo->setVisible(true);
            paintArea->setPixelArtMode(false); paintArea->setZoomFactor(0.50); comboZoom->setCurrentIndex(2);
            brushesGridFrame->setVisible(true); pixelToolsPanel->setVisible(false);
            btnMirrorPen->setVisible(false); btnPixelStroke->setVisible(false); btnLighten->setVisible(false);
            btnSprayTool->setVisible(true); btnBrushTool->setVisible(true); btnCrayonTool->setVisible(true); btnMarkerTool->setVisible(true);
            btnWatercolor->setVisible(true); btnOilBrush->setVisible(true); btnCalligraphy->setVisible(true); btnHighlighter->setVisible(true);
            resetToolButtons(btnPencil); paintArea->setTool(ToolPencil);
            modoActual = "Normal"; actNormal->setChecked(true); actPixelArt->setChecked(false); actAvanzado->setChecked(false); this->resize(oldSize);
        }
    });
    connect(actPixelArt, &QAction::triggered, this, [=, this]() {
        if (cambiarModo("PixelArt")) {
            QSize oldSize = this->size(); detenerAnimacion();
            leftSidebarWidget->setVisible(false);
            boxShapes->setVisible(false); boxAnimation->setVisible(true);
            btnImageFilters->setVisible(false); btnCustomBrushesTop->setVisible(false); btnCustomToolAction->setVisible(false); btnCanvasSize->setMenu(menuCanvasSizePixel); btnCanvasSize->setVisible(true);
            if (actExportGif) actExportGif->setVisible(true);
            actInsertarImagen->setVisible(true); actInsertarComoCapa->setVisible(false); actAbrirFondo->setVisible(true);
            paintArea->setPixelArtMode(true, 32); comboZoom->setCurrentIndex(7);
            brushesGridFrame->setVisible(false); pixelToolsPanel->setVisible(true);
            btnSprayTool->setVisible(false); btnBrushTool->setVisible(false); btnCrayonTool->setVisible(false); btnMarkerTool->setVisible(false);
            btnWatercolor->setVisible(false); btnOilBrush->setVisible(false); btnCalligraphy->setVisible(false); btnHighlighter->setVisible(false);
            btnMirrorPen->setVisible(true); btnPixelStroke->setVisible(true); btnLighten->setVisible(true);
            resetToolButtons(btnPencil); paintArea->setTool(ToolPencil);
            modoActual = "PixelArt"; actNormal->setChecked(false); actPixelArt->setChecked(true); actAvanzado->setChecked(false); this->resize(oldSize);
        }
    });
    connect(actAvanzado, &QAction::triggered, this, [=, this]() {
        if (cambiarModo("Avanzado")) {
            QSize oldSize = this->size(); detenerAnimacion();
            leftSidebarWidget->setVisible(true);
            boxShapes->setVisible(true); boxAnimation->setVisible(false);
            btnImageFilters->setVisible(true); btnCustomBrushesTop->setVisible(true); btnCustomToolAction->setVisible(true); btnCanvasSize->setMenu(menuCanvasSizeStd); btnCanvasSize->setVisible(true);
            if (actExportGif) actExportGif->setVisible(false);
            actInsertarImagen->setVisible(true); actInsertarComoCapa->setVisible(true); actAbrirFondo->setVisible(true);
            paintArea->setPixelArtMode(false); paintArea->setZoomFactor(0.50); comboZoom->setCurrentIndex(2);
            brushesGridFrame->setVisible(true); pixelToolsPanel->setVisible(false);
            btnMirrorPen->setVisible(false); btnPixelStroke->setVisible(false); btnLighten->setVisible(false);
            btnSprayTool->setVisible(true); btnBrushTool->setVisible(true); btnCrayonTool->setVisible(true); btnMarkerTool->setVisible(true);
            btnWatercolor->setVisible(true); btnOilBrush->setVisible(true); btnCalligraphy->setVisible(true); btnHighlighter->setVisible(true);
            resetToolButtons(btnPencil); paintArea->setTool(ToolPencil);
            modoActual = "Avanzado"; actNormal->setChecked(false); actPixelArt->setChecked(false); actAvanzado->setChecked(true); this->resize(oldSize);
        }
    });


    connect(btnImageFilters, &QPushButton::clicked, this, [this]() {
        ImageFiltersDialog dialog(paintArea->getImage(), this, false, darkMode ? 1 : 0);
        if (dialog.exec() == QDialog::Accepted) { paintArea->applyImageFilters(dialog.getFilteredImage()); statusBar()->showMessage(tr("Filtros aplicados"), 2000); }
    });


    connect(btnThemeToggle, &QPushButton::clicked, this, [this]() {
        darkMode = !darkMode; paintArea->setDarkMode(darkMode);
        btnThemeToggle->setText(darkMode ? tr("Modo Claro") : tr("Modo Oscuro"));
        separadorBarra->setStyleSheet(darkMode ? "background-color: #383838; min-width: 1px; max-width: 1px; margin: 4px 8px;" : "background-color: #cbd5e1; min-width: 1px; max-width: 1px; margin: 4px 8px;");
        btnLayerMask->setIcon(crearIconoMascara(darkMode));
        btnDeformTool->setIcon(crearIconoDeformacion(darkMode));
        compilarHojasDeEstiloGlobales();
        actualizarPanelCapas();
    });

    connect(btnQuickSave, &QPushButton::clicked, this, [this]() { guardarComo(); });
    connect(btnUndo, &QPushButton::clicked, this, [this]() { paintArea->undo(); });
    connect(btnRedo, &QPushButton::clicked, this, [this]() { paintArea->redo(); });
    connect(paintArea, &PaintArea::resolutionChanged, this, [this](int w, int h) { lblResolutionIndicator->setText(tr("Resolucion: %1 x %2 px").arg(w).arg(h)); });

    // === TEXTO / DEFORM ===
    connect(paintArea, &PaintArea::textFrameClicked, this, [this](const QPoint &canvasPos) {
        Q_UNUSED(canvasPos);
        QPoint frameScreenPos = paintArea->mapToGlobal(QPoint(
            (paintArea->getTextFrameRect().left() + paintArea->getTextFrameRect().width()) * paintArea->getZoomFactor(),
            paintArea->getTextFrameRect().top() * paintArea->getZoomFactor()));
        QScreen *currentScreen = this->screen();
        QRect screenGeom = currentScreen->availableGeometry();
        int editorX = frameScreenPos.x() + 10;
        int editorY = frameScreenPos.y() - 10;
        if (editorX + textFormatBar ->width() > screenGeom.right()) editorX = frameScreenPos.x() - textFormatBar->width() - 10;
        if (editorY + textFormatBar ->height() > screenGeom.bottom()) editorY = screenGeom.bottom() - textFormatBar ->height() - 10;
        if (editorX < screenGeom.left()) editorX = screenGeom.left() + 10;
        if (editorY < screenGeom.top()) editorY = screenGeom.top() + 10;
        textFormatBar ->move(editorX, editorY);
        QColor activeColor = (colorObjetivoActivo == 1) ? paintArea->getPenColor1() : paintArea->getPenColor2();
        textFormatBar ->setColor(activeColor);
        textFormatBar->show(); textFormatBar->raise(); textFormatBar->activateWindow();
    });
        connect(textFormatBar, &TextEngine::FormatBar::formatChanged, this, [this]() {
        if (paintArea->isTextFrameActive()) paintArea->updateTextFrame(paintArea->getTextFrameRect(), textFormatBar ->getCurrentFont(), textFormatBar ->getColor());
    });
        connect(textFormatBar, &TextEngine::FormatBar::applyClicked, this, [this]() { paintArea->bakeTextFrame(); textFormatBar->hide(); });
        connect(textFormatBar, &TextEngine::FormatBar::cancelClicked, this, [this]() { paintArea->cancelTextFrame(); textFormatBar->hide(); });
        
    connect(deformSettingsBar, &DeformSettingsBar::optionsChanged, this, [this]() { paintArea->setDeformOptc(deformSettingsBar->getOptions()); });
    paintArea->setDeformOptc(deformSettingsBar->getOptions());

    // === INICIALIZACION FINAL ===
    paintArea->setZoomFactor(0.50);
    compilarHojasDeEstiloGlobales();
    actualizarEstilosDePrevisualizacion();
    actualizarPanelCapas();
    lblResolutionIndicator->setText(tr("Resolucion: %1 x %2 px").arg(paintArea->canvasSize().width()).arg(paintArea->canvasSize().height()));
    statusBar()->showMessage(tr("Paintlux Studio") + (darkMode ? " (Oscuro)" : " (Claro)"));
    actInsertarImagen->setVisible(true);
    actInsertarComoCapa->setVisible(false);
    actAbrirFondo->setVisible(true);
}


void mainwind::actualizarBotonLazo() {
    if (lassoModoActual == 0) {

        btnLassoTool->setIcon(QIcon(resolveAssetPath("lasso_extract.svg")));
        btnLassoTool->setToolTip(tr("Recorte hacia ADENTRO (extraer)\nClic derecho: cambiar a AFUERA"));
        actLassoAdentro->setChecked(true);
        actLassoAfuera->setChecked(false);
    } else {
        btnLassoTool->setIcon(QIcon(resolveAssetPath("lasso_delete.svg")));
        btnLassoTool->setToolTip(tr("Recorte hacia AFUERA (hueco)\nClic derecho: cambiar a ADENTRO"));
        actLassoAdentro->setChecked(false);
        actLassoAfuera->setChecked(true);

    }
}


void mainwind::handleZoomRequest(double newFactor, QPoint viewportPos) {
    if (newFactor < 0.125) newFactor = 0.125;
    if (newFactor > 32.0) newFactor = 32.0;

    int vpW = scrollArea->viewport()->width();
    int vpH = scrollArea->viewport()->height();
    int wW = paintArea->width();
    int wH = paintArea->height();

    int widgetTopLeftX = (wW <= vpW) ? (vpW - wW) / 2 : -scrollArea->horizontalScrollBar()->value();
    int widgetTopLeftY = (wH <= vpH) ? (vpH - wH) / 2 : -scrollArea->verticalScrollBar()->value();
    int widgetLocalX = viewportPos.x() - widgetTopLeftX;
    int widgetLocalY = viewportPos.y() - widgetTopLeftY;
    if (widgetLocalX < 0 || widgetLocalX >= wW || widgetLocalY < 0 || widgetLocalY >= wH) { widgetLocalX = wW / 2; widgetLocalY = wH / 2; }

    double oldFactor = paintArea->getZoomFactor();
    double canvasX = widgetLocalX / oldFactor;
    double canvasY = widgetLocalY / oldFactor;

    paintArea->setZoomFactor(newFactor);

    int newWidgetX = (int)(canvasX * newFactor);
    int newWidgetY = (int)(canvasY * newFactor);
    int newWW = paintArea->width();
    int newWH = paintArea->height();

    QScrollBar *hBar = scrollArea->horizontalScrollBar();
    QScrollBar *vBar = scrollArea->verticalScrollBar();
    int newHValue, newVValue;
    if (newWW > vpW) newHValue = newWidgetX - viewportPos.x(); else newHValue = 0;
    if (newWH > vpH) newVValue = newWidgetY - viewportPos.y(); else newVValue = 0;
    newHValue = qBound(hBar->minimum(), newHValue, hBar->maximum());
    newVValue = qBound(vBar->minimum(), newVValue, vBar->maximum());
    hBar->setValue(newHValue);
    vBar->setValue(newVValue);
}

void mainwind::solicitarRefreshCapas() {
    if (!layerRefreshTimer) {
        layerRefreshTimer = new QTimer(this);
        layerRefreshTimer->setSingleShot(true);
        layerRefreshTimer->setInterval(250);
        connect(layerRefreshTimer, &QTimer::timeout, this, [this]() { actualizarPanelCapas(); });
    }
    if (!layerRefreshTimer->isActive()) layerRefreshTimer->start();
}

void mainwind::cambiarIdioma(const QString &langCode) {
    if (!preguntarGuardarCambios()) return;
    QSettings settings("Paintux", "PaintuxStudio"); settings.setValue("language", langCode);
    QMessageBox::information(this, tr("Reinicio Requerido"), tr("La aplicacion se reiniciara para cambiar el idioma."));
    QProcess::startDetached(QApplication::applicationFilePath(), QApplication::arguments().mid(1));
    QApplication::quit();
}

void mainwind::actualizarPanelCapas() {
    if (actualizandoPanelCapas) return;
    actualizandoPanelCapas = true;
    bool blocked = blockSignals(true);

    const QList<Layer>& layers = paintArea->getLayers();
    int currentIdx = paintArea->getCurrentLayerIndex();
    int maskEditIdx = paintArea->getMaskEditLayer();

    if (draggableLayerItems.size() == layers.size() && !layers.isEmpty()) {
        for (int k = 0; k < draggableLayerItems.size(); ++k) {
            int layerIdx = layers.size() - 1 - k;
            DraggableLayerItem *item = draggableLayerItems[k];
            item->refreshFrom(layers[layerIdx].image, layers[layerIdx].name, layers[layerIdx].visible, darkMode);
            item->setSelected(layerIdx == currentIdx);
            bool hasM = paintArea->hasLayerMask(layerIdx);
            bool enM = paintArea->isLayerMaskEnabled(layerIdx);
            bool editM = (maskEditIdx == layerIdx);
            QImage prev = hasM ? paintArea->getMaskPreview(layerIdx) : QImage();
            item->updateMaskState(hasM, enM, editM, prev);
            bool hasCM = paintArea->hasLayerColorMask(layerIdx);
            bool enCM = paintArea->isLayerColorMaskEnabled(layerIdx);
            QImage prevCM = hasCM ? paintArea->getColorMaskPreview(layerIdx) : QImage();
            item->updateColorMaskState(hasCM, enCM, prevCM);
        }
    } else {
        for (DraggableLayerItem *item : draggableLayerItems) { layersLayout->removeWidget(item); item->deleteLater(); }
        draggableLayerItems.clear();
        for (int i = layers.size() - 1; i >= 0; --i) {
            DraggableLayerItem *layerItem = new DraggableLayerItem(i, layers[i].image, layers[i].name, layers[i].visible, darkMode, layersContainer);
            layerItem->setSelected(i == currentIdx);
            bool hasM = paintArea->hasLayerMask(i);
            bool enM = paintArea->isLayerMaskEnabled(i);
            bool editM = (maskEditIdx == i);
            QImage prev = hasM ? paintArea->getMaskPreview(i) : QImage();
            layerItem->updateMaskState(hasM, enM, editM, prev);
            bool hasCM = paintArea->hasLayerColorMask(i);
            bool enCM = paintArea->isLayerColorMaskEnabled(i);
            QImage prevCM = hasCM ? paintArea->getColorMaskPreview(i) : QImage();
            layerItem->updateColorMaskState(hasCM, enCM, prevCM);
            connect(layerItem, &DraggableLayerItem::clicked, this, [this](int idx) { paintArea->setCurrentLayer(idx); paintArea->selectLayerContentForEditing(); }, Qt::QueuedConnection);
            connect(layerItem, &DraggableLayerItem::layerMoved, this, [this](int fromIndex, int toIndex) { paintArea->reorderLayer(fromIndex, toIndex); }, Qt::QueuedConnection);
            connect(layerItem, &DraggableLayerItem::visibilityToggled, this, [this](int idx, bool visible) { paintArea->setLayerVisibility(idx, visible); }, Qt::QueuedConnection);
            connect(layerItem, &DraggableLayerItem::maskClicked, this, [this](int idx, bool shiftHeld) {
                if (shiftHeld) paintArea->toggleLayerMaskEnabled(idx);
                else { paintArea->setCurrentLayer(idx); paintArea->selectMaskForEditing(idx); }
            }, Qt::QueuedConnection);
            connect(layerItem, &DraggableLayerItem::colorMaskClicked, this, [this](int idx, bool shiftHeld) {
                if (shiftHeld) paintArea->toggleLayerColorMaskEnabled(idx);
                else { paintArea->setCurrentLayer(idx); abrirEditorMascaraColor(idx); }
            }, Qt::QueuedConnection);
            layersLayout->insertWidget(0, layerItem);
            draggableLayerItems.append(layerItem);
        }
    }

    if (currentIdx >= 0 && currentIdx < layers.size()) {
        const Layer &cl = layers[currentIdx];
        lblCurrentLayer->setText(tr("Capa: %1").arg(currentIdx + 1));
        sliderLayerOpacity->blockSignals(true); sliderLayerOpacity->setValue(cl.opacity * 100); sliderLayerOpacity->blockSignals(false);
        int bi = comboBlendMode->findData(cl.blendMode);
        if (bi >= 0) { comboBlendMode->blockSignals(true); comboBlendMode->setCurrentIndex(bi); comboBlendMode->blockSignals(false); }
        chkLayerLocked->blockSignals(true); chkLayerLocked->setChecked(cl.locked); chkLayerLocked->blockSignals(false);
    }

    bool hasMask = paintArea->hasLayerMask(currentIdx);
    bool maskOn = paintArea->isLayerMaskEnabled(currentIdx);
    actAddMask->setEnabled(!hasMask);
    actDelMask->setEnabled(hasMask);
    actToggleMask->setEnabled(hasMask);
    actInvertMask->setEnabled(hasMask);
    actApplyMask->setEnabled(hasMask);
    actToggleMask->setText(maskOn ? tr("Desactivar máscara") : tr("Activar máscara"));

    bool hasCM = paintArea->hasLayerColorMask(currentIdx);
    bool cmOn = paintArea->isLayerColorMaskEnabled(currentIdx);
    actAddColorMask->setEnabled(true);
    actAddColorMask->setText(hasCM ? tr("Re-editar MÁSCARA DE COLOR (filtros)...") : tr("Añadir MÁSCARA DE COLOR (filtros)..."));
    actDelColorMask->setEnabled(hasCM);
    actToggleColorMask->setEnabled(hasCM);
    actToggleColorMask->setText(cmOn ? tr("Desactivar máscara de color") : tr("Activar máscara de color"));

    blockSignals(blocked);
    actualizandoPanelCapas = false;
}

void mainwind::iniciarAnimacion() {
    const QList<QImage>& frames = paintArea->getFrames();
    if (frames.size() <= 1) { statusBar()->showMessage(tr("Se necesitan al menos 2 frames"), 2000); btnPlayAnimation->setChecked(false); return; }
    isAnimating = true; animTimer->start(animSpeed); btnPlayAnimation->setText(tr("Stop")); btnPlayAnimation->setChecked(true);
    btnAddFrame->setEnabled(false); btnDupFrame->setEnabled(false); btnDelFrame->setEnabled(false);
}

void mainwind::detenerAnimacion() { isAnimating = false; animTimer->stop(); btnPlayAnimation->setText(tr("Play")); btnPlayAnimation->setChecked(false); btnAddFrame->setEnabled(true); btnDupFrame->setEnabled(true); btnDelFrame->setEnabled(true); }

void mainwind::actualizarMiniaturasFrames(const QList<QImage> &frames, int currentIndex) {
    if (frameThumbnails.size() != frames.size()) {
        while (framesLayout->count() > 0) {
            QLayoutItem *item = framesLayout->takeAt(0);
            if (item->widget()) delete item->widget();
            delete item;
        }
        frameThumbnails.clear();
        for (int i = 0; i < frames.size(); ++i) {
            FrameThumbnail *thumbnail = new FrameThumbnail(frames[i], i, framesContainer);
            connect(thumbnail, &FrameThumbnail::clicked, this, [this](int fi) { if (!isAnimating) paintArea->goToFrame(fi); });
            framesLayout->addWidget(thumbnail);
            frameThumbnails.append(thumbnail);
        }
        framesLayout->addStretch();
    } else {
        for (int i = 0; i < frames.size(); ++i) frameThumbnails[i]->setFrameImage(frames[i]);
    }
    for (int i = 0; i < frameThumbnails.size(); ++i) frameThumbnails[i]->setSelected(i == currentIndex);
    lblFrameIndicator->setText(tr("Frame %1/%2").arg(currentIndex + 1).arg(frames.size()));
    btnDelFrame->setEnabled(frames.size() > 1 && !isAnimating);
    btnPlayAnimation->setEnabled(frames.size() > 1);
    if (isAnimating && frames.size() <= 1) detenerAnimacion();
}

bool mainwind::preguntarGuardarCambios() {
    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Guardar cambios"), tr("Deseas guardar el lienzo actual?"), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    if (reply == QMessageBox::Cancel) return false;
    if (reply == QMessageBox::Yes) return guardarComo();
    return true;
}

bool mainwind::cambiarModo(const QString &nuevoModo) { if (modoActual == nuevoModo) return true; if (!preguntarGuardarCambios()) return false; paintArea->clearHistory(); return true; }

void mainwind::refrescarDatosZoomUI(double factor) {
    lblZoomIndicator->setText(tr("Zoom: %1%").arg(qRound(factor * 100)));
    comboZoom->blockSignals(true); int idx = comboZoom->findData(factor); if (idx >= 0) comboZoom->setCurrentIndex(idx); comboZoom->blockSignals(false);
}

void mainwind::compilarHojasDeEstiloGlobales() {
    QString qss;
    QString workspaceBg = darkMode ? QColor(32, 32, 32, currentWorkspaceAlpha).name(QColor::HexArgb) : QColor(171, 179, 186, currentWorkspaceAlpha).name(QColor::HexArgb);

    if (darkMode) {
        qss = "QMainWindow { background-color: transparent; font-family: 'Adwaita Sans', 'Noto Sans', sans-serif; }"
              "QWidget#leftSidebarWidget { background-color: #1e1e1e; border-right: 1px solid #383838; }"
              "QWidget#sysBarContainer { background-color: #202020; border-bottom: 1px solid #181818; }"
              "QWidget#ribbonContainer { background-color: #262626; border-bottom: 1px solid #181818; }"
              "QWidget#bottomBarContainer { background-color: #202020; border-top: 1px solid #181818; color: #e0e0e0; }"
              "QScrollArea#framesScrollArea, QScrollArea#layersScrollArea { background-color: #1a1a1a; border: 1px solid #383838; border-radius: 4px; }"
              "QWidget#framesContainerWidget, QWidget#layersContainerWidget { background-color: #1a1a1a; }"
              "QFrame#layerItemFrame { background-color: #2a2a2a; border: 1px solid #383838; border-radius: 4px; }"
              "QFrame#layerItemFrame:hover { background-color: #333333; border: 1px solid #0066cc; }"
              "QFrame#maskThumbFrame { background-color: #1e1e1e; border: 1px solid #3a3a3a; border-radius: 5px; }"
              "QFrame#colorMaskThumbFrame { background-color: #1e1e1e; border: 1px solid #f59e0b; border-radius: 5px; }"
              "QGroupBox { font-size: 11px; color: #a0a0a0; border: none; border-right: 1px solid #383838; margin-top: 0px; padding: 2px; padding-bottom: 14px; }"
              "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: bottom center; padding: 2px; }"
              "QMenu { background-color: #252525; color: #fff; border: 1px solid #444; border-radius: 6px; }"
              "QMenu::item:selected { background-color: #0066cc; border-radius: 4px; } QMenu::separator { height: 1px; background-color: #444; margin: 4px 8px; }"
              "QPushButton, QToolButton { background-color: #333; color: #ddd; border: 1px solid #444; border-radius: 4px; padding: 3px; font-size: 11px; }"
              "QPushButton:hover, QToolButton:hover { background-color: #444; border: 1px solid #0066cc; }"
              "QPushButton:checked, QToolButton:checked { background-color: #1a4a7c; border: 1px solid #0066cc; color: #fff; }"
              "QPushButton#btnArchivoMenu { background-color: #1a6dd4; color: white; font-weight: bold; padding: 2px 14px; border: none; border-radius: 10px; }"
              "QPushButton#btnArchivoMenu:hover { background-color: #2280e8; }"
              "QPushButton#btnViewMenu { background-color: #1a6dd4; color: white; font-weight: bold; padding: 2px 14px; border: none; border-radius: 10px; }"
              "QPushButton#btnViewMenu:hover { background-color: #2280e8; }"
              "QPushButton#btnQuickIcon { background-color: transparent; border: none; padding: 2px; max-width: 26px; max-height: 26px; border-radius: 6px; }"
              "QPushButton#btnQuickIcon:hover { background-color: #333; border: 1px solid #444; }"
              "QPushButton#btnQuickAction { background-color: transparent; border: none; padding: 2px 8px; min-width: 60px; color: #ddd; border-radius: 6px; }"
              "QPushButton#btnQuickAction:hover { background-color: #444; border: 1px solid #0066cc; }"
              "QPushButton#btnSysToggle { background-color: transparent; border: 1px solid #0066cc; color: #0066cc; font-size: 10px; padding: 2px 8px; border-radius: 8px; }"
              "QToolButton#btnBigPaste { font-size: 11px; min-height: 64px; min-width: 52px; border-radius: 8px; }"
              "QToolButton#btnCustomLong { min-width: 44px; min-height: 90px; border: 1px solid #444; border-radius: 8px; background-color: #2a2a2a; }"
              "QToolButton#btnCustomLong:hover { background-color: #3a3a3a; border: 1px solid #0066cc; }"
              "QToolButton#btnCustomLong:checked { background-color: #1a4a7c; border: 1px solid #0066cc; }"
              "QToolButton#btnVectorTool { background-color: #333; color: #ddd; border: 1px solid #444; border-radius: 6px; padding: 4px 8px; font-size: 11px; text-align: left; }"
              "QToolButton#btnVectorTool:hover { background-color: #444; border: 1px solid #0066cc; }"
              "QToolButton#btnVectorTool:checked { background-color: #1a4a7c; border: 1px solid #0066cc; color: #fff; }"
              "QPushButton#btnSmallStacked { font-size: 10px; padding: 2px 4px; min-width: 75px; text-align: left; border-radius: 4px; }"
              "QPushButton#btnImgAction { font-size: 10px; padding: 3px; text-align: left; min-width: 85px; border-radius: 4px; }"
              "QPushButton#btnToolGrid { font-size: 12px; min-width: 25px; min-height: 25px; padding: 0px; border-radius: 4px; }"
              "QFrame#dividerTools { background-color: #383838; min-width: 1px; max-width: 1px; border: none; margin: 4px 2px; }"
              "QToolButton#btnBrushUnit { min-width: 28px; min-height: 28px; padding: 2px; font-size: 9px; border-radius: 6px; }"
              "QFrame#shapesGridFrame { background-color: #2d2d2d; border: 1px solid #444; border-radius: 6px; }"
              "QPushButton#btnShapeUnit { background-color: transparent; border: none; min-width: 24px; min-height: 24px; border-radius: 4px; padding: 2px; }"
              "QPushButton#btnShapeUnit:hover { background-color: #333; } QPushButton#btnShapeUnit:checked { background-color: #1a4a7c; border: 1px solid #0066cc; }"
              "QComboBox { background-color: #2a2a2a; color: #e0e0e0; border: 1px solid #555; border-radius: 6px; padding: 4px 8px; font-size: 10px; min-height: 22px; }"
              "QComboBox:hover { border: 1px solid #0066cc; background-color: #333; }"
              "QComboBox::drop-down { border: none; width: 18px; } QComboBox::down-arrow { image: none; }"
              "QComboBox QAbstractItemView { background-color: #2a2a2a; color: #e0e0e0; border: 1px solid #555; border-radius: 6px; selection-background-color: #0066cc; outline: none; }"
              "QComboBox#comboZoomBottom { font-size: 8px; min-height: 18px; padding: 1px 4px; }"
              "QToolButton#btnEditColorsWide { font-size: 10px; font-weight: normal; min-height: 32px; border-radius: 6px; padding: 4px 6px; }"
              "QLabel { color: #d0d0d0; }"
              "QLabel#lblMini { font-size: 10px; color: #a0a0a0; }"
              "QLabel#lblZoomIndicator { font-size: 10px; color: #e0e0e0; }"
              "QFrame#frameSelectorsColor { border: 1px solid #444; background-color: #1e1e1e; border-radius: 8px; }"
              "QSlider::groove:horizontal { height: 4px; background: #444; border-radius: 2px; }"
              "QSlider::handle:horizontal { background: #0066cc; width: 12px; height: 12px; margin: -4px 0; border-radius: 6px; }"
              "QCheckBox { color: #ddd; }"
              "QPushButton#btnZoomAction { background-color: #2a2a2a; color: #e0e0e0; border: 1px solid #555; border-radius: 3px; padding: 0px; font-weight: bold; }"
              "QPushButton#btnZoomAction:hover { background-color: #444; border: 1px solid #0066cc; }";
    } else {
        qss = "QMainWindow { background-color: transparent; font-family: 'Adwaita Sans', 'Noto Sans', sans-serif; }"
              "QWidget#leftSidebarWidget { background-color: #f8fafc; border-right: 1px solid #cbd5e1; }"
              "QWidget#sysBarContainer { background-color: #ffffff; border-bottom: 1px solid #cbd5e1; }"
              "QWidget#ribbonContainer { background-color: #ffffff; border-bottom: 1px solid #cbd5e1; }"
              "QWidget#bottomBarContainer { background-color: #ffffff; border-top: 1px solid #cbd5e1; color: #334155; }"
              "QScrollArea#framesScrollArea, QScrollArea#layersScrollArea { background-color: #ffffff; border: 1px solid #cbd5e1; border-radius: 4px; }"
              "QWidget#framesContainerWidget, QWidget#layersContainerWidget { background-color: #ffffff; }"
              "QFrame#layerItemFrame { background-color: #f8fafc; border: 1px solid #e2e8f0; border-radius: 4px; }"
              "QFrame#layerItemFrame:hover { background-color: #f1f5f9; border: 1px solid #3b82f6; }"
              "QFrame#maskThumbFrame { background-color: #ffffff; border: 1px solid #d1d5db; border-radius: 5px; }"
              "QFrame#colorMaskThumbFrame { background-color: #ffffff; border: 1px solid #d97706; border-radius: 5px; }"
              "QGroupBox { font-size: 11px; color: #334155; border: none; border-right: 1px solid #e2e8f0; margin-top: 0px; padding: 2px; padding-bottom: 14px; }"
              "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: bottom center; padding: 2px; }"
              "QMenu { background-color: #ffffff; color: #0f172a; border: 1px solid #cbd5e1; border-radius: 6px; }"
              "QMenu::item:selected { background-color: #eff6ff; color: #1d4ed8; border-radius: 4px; } QMenu::separator { height: 1px; background-color: #e2e8f0; margin: 4px 8px; }"
              "QPushButton, QToolButton { background-color: #f8fafc; color: #1e293b; border: 1px solid #cbd5e1; border-radius: 4px; padding: 3px; font-size: 11px; }"
              "QPushButton:hover, QToolButton:hover { background-color: #f1f5f9; border: 1px solid #93c5fd; }"
              "QPushButton:checked, QToolButton:checked { background-color: #eff6ff; border: 1px solid #3b82f6; color: #1d4ed8; }"
              "QPushButton#btnArchivoMenu { background-color: #2563eb; color: white; font-weight: bold; padding: 2px 14px; border: none; border-radius: 10px; }"
              "QPushButton#btnArchivoMenu:hover { background-color: #1d4ed8; }"
              "QPushButton#btnViewMenu { background-color: #2563eb; color: white; font-weight: bold; padding: 2px 14px; border: none; border-radius: 10px; }"
              "QPushButton#btnViewMenu:hover { background-color: #1d4ed8; }"
              "QPushButton#btnQuickIcon { background-color: transparent; border: none; padding: 2px; max-width: 26px; max-height: 26px; border-radius: 6px; }"
              "QPushButton#btnQuickIcon:hover { background-color: #f1f5f9; border: 1px solid #cbd5e1; }"
              "QPushButton#btnQuickAction { background-color: transparent; border: none; padding: 2px 8px; min-width: 60px; color: #1e293b; border-radius: 6px; }"
              "QPushButton#btnQuickAction:hover { background-color: #f1f5f9; border: 1px solid #cbd5e1; }"
              "QPushButton#btnSysToggle { background-color: transparent; border: 1px solid #3b82f6; color: #3b82f6; font-size: 10px; padding: 2px 8px; border-radius: 8px; }"
              "QToolButton#btnBigPaste { font-size: 11px; min-height: 64px; min-width: 52px; border-radius: 8px; }"
              "QToolButton#btnCustomLong { min-width: 44px; min-height: 90px; border: 1px solid #cbd5e1; border-radius: 8px; background-color: #f8fafc; }"
              "QToolButton#btnCustomLong:hover { background-color: #f1f5f9; border: 1px solid #93c5fd; }"
              "QToolButton#btnCustomLong:checked { background-color: #eff6ff; border: 1px solid #3b82f6; }"
              "QToolButton#btnVectorTool { background-color: #f8fafc; color: #1e293b; border: 1px solid #cbd5e1; border-radius: 6px; padding: 4px 8px; font-size: 11px; text-align: left; }"
              "QToolButton#btnVectorTool:hover { background-color: #f1f5f9; border: 1px solid #93c5fd; }"
              "QToolButton#btnVectorTool:checked { background-color: #eff6ff; border: 1px solid #3b82f6; color: #1d4ed8; }"
              "QPushButton#btnSmallStacked { font-size: 10px; padding: 2px 4px; min-width: 75px; text-align: left; border-radius: 4px; }"
              "QPushButton#btnImgAction { font-size: 10px; padding: 3px; text-align: left; min-width: 85px; border-radius: 4px; }"
              "QPushButton#btnToolGrid { font-size: 12px; min-width: 25px; min-height: 25px; padding: 0px; border-radius: 4px; }"
              "QFrame#dividerTools { background-color: #e2e8f0; min-width: 1px; max-width: 1px; border: none; margin: 4px 2px; }"
              "QToolButton#btnBrushUnit { min-width: 28px; min-height: 28px; padding: 2px; font-size: 9px; border-radius: 6px; }"
              "QFrame#shapesGridFrame { background-color: #f1f5f9; border: 1px solid #cbd5e1; border-radius: 6px; }"
              "QPushButton#btnShapeUnit { background-color: transparent; border: none; min-width: 24px; min-height: 24px; border-radius: 4px; padding: 2px; }"
              "QPushButton#btnShapeUnit:hover { background-color: #f1f5f9; } QPushButton#btnShapeUnit:checked { background-color: #eff6ff; border: 1px solid #3b82f6; }"
              "QComboBox { background-color: #ffffff; color: #1e293b; border: 1px solid #cbd5e1; border-radius: 6px; padding: 4px 8px; font-size: 11px; min-height: 22px; }"
              "QComboBox:hover { border: 1px solid #3b82f6; background-color: #f8fafc; }"
              "QComboBox::drop-down { border: none; width: 18px; } QComboBox::down-arrow { image: none; }"
              "QComboBox QAbstractItemView { background-color: #ffffff; color: #1e293b; border: 1px solid #cbd5e1; border-radius: 6px; selection-background-color: #eff6ff; outline: none; }"
              "QComboBox#comboZoomBottom { font-size: 8px; min-height: 18px; padding: 1px 4px; }"
              "QToolButton#btnEditColorsWide { font-size: 10px; font-weight: normal; min-height: 32px; border-radius: 6px; padding: 4px 6px; }"
              "QLabel { color: #475569; }"
              "QLabel#lblMini { font-size: 11px; color: #475569; }"
              "QLabel#lblZoomIndicator { font-size: 10px; color: #334155; }"
              "QFrame#frameSelectorsColor { border: 1px solid #cbd5e1; background-color: #ffffff; border-radius: 8px; }"
              "QSlider::groove:horizontal { height: 4px; background: #e2e8f0; border-radius: 2px; }"
              "QSlider::handle:horizontal { background: #3b82f6; width: 12px; height: 12px; margin: -4px 0; border-radius: 6px; }"
              "QCheckBox { color: #1e293b; }"
              "QPushButton#btnZoomAction { background-color: #f8fafc; color: #1e293b; border: 1px solid #cbd5e1; border-radius: 3px; padding: 0px; font-weight: bold; }"
              "QPushButton#btnZoomAction:hover { background-color: #f1f5f9; border: 1px solid #3b82f6; }";
    }
    qss += QString(" QWidget#workAreaContainer { background-color: %1; }").arg(workspaceBg);
    setStyleSheet(qss);
    ribbonWidget->setObjectName("ribbonContainer");
}

void mainwind::inyectarColorAObjeto(const QColor &color) {
    if (colorObjetivoActivo == 1) paintArea->setPenColor1(color); else paintArea->setPenColor2(color);
    actualizarEstilosDePrevisualizacion();
}

void mainwind::sincronizarGoteroUI(int target, const QColor &color) {
    if (target == 1) paintArea->setPenColor1(color); else paintArea->setPenColor2(color);
    actualizarEstilosDePrevisualizacion();
}

void mainwind::actualizarEstilosDePrevisualizacion() {
    QString active = darkMode ? "#0066cc" : "#3b82f6", inactive = darkMode ? "#555" : "#cbd5e1";
    btnColor1->setStyleSheet(QString("background-color: %1; border: 2px solid %2; border-radius: 8px;").arg(paintArea->getPenColor1().name(), colorObjetivoActivo == 1 ? active : inactive));
    btnColor2->setStyleSheet(QString("background-color: %1; border: 2px solid %2; border-radius: 8px;").arg(paintArea->getPenColor2().name(), colorObjetivoActivo == 2 ? active : inactive));
}

void mainwind::abrirPaletaAvanzada() {
    QColor nuevoColor = QColorDialog::getColor(Qt::black, this, tr("Editar Colores"));
    if (nuevoColor.isValid()) {
        inyectarColorAObjeto(nuevoColor);
        if (indiceRecienteActual < listaBotonesRecientes.size()) {
            QPushButton *btnR = listaBotonesRecientes[indiceRecienteActual];
            btnR->setStyleSheet(QString("background-color: %1; border: 1px solid rgba(0,0,0,0.4); border-radius: 3px;").arg(nuevoColor.name()));
            btnR->setProperty("customColor", nuevoColor);
            indiceRecienteActual = (indiceRecienteActual + 1) % 10;
        }
    }
}

void mainwind::subirImagenDisco() {
    if (!preguntarGuardarCambios()) return;
    QString fileName = QFileDialog::getOpenFileName(this, tr("Abrir imagen como fondo"), QDir::currentPath(), tr("Imagenes (*.png *.jpg *.jpeg *.bmp)"));
    if (!fileName.isEmpty()) { if (paintArea->abrirImagen(fileName)) statusBar()->showMessage(tr("Imagen cargada como fondo"), 2000); else QMessageBox::warning(this, tr("Error"), tr("No se pudo cargar la imagen.")); }
}

bool mainwind::guardarComo(const QString &extension) {
    QString filtro;
    QString nombreDefault = QDir::currentPath() + "/Sin titulo";
    if (extension == "png")       { filtro = "PNG (*.png)";                    nombreDefault += ".png"; }
    else if (extension == "jpg")  { filtro = "JPEG (*.jpg *.jpeg)";            nombreDefault += ".jpg"; }
    else if (extension == "svg")  { filtro = "SVG Vectorial (*.svg)";          nombreDefault += ".svg"; }
    else                          { filtro = "PNG (*.png);;JPEG (*.jpg *.jpeg);;SVG Vectorial (*.svg)"; nombreDefault += ".png"; }
    QString fileName = QFileDialog::getSaveFileName(this, tr("Guardar como"), nombreDefault, filtro);
    if (fileName.isEmpty()) return false;
    QString lowerName = fileName.toLower();

    QApplication::setOverrideCursor(Qt::WaitCursor);
    bool ok = false;
    if (lowerName.endsWith(".svg")) {
        ok = paintArea->guardarComoSvg(fileName);
        QApplication::restoreOverrideCursor();
        if (ok) { QSize size = paintArea->canvasSize(); statusBar()->showMessage(tr("SVG: %1 (%2x%3 px)").arg(fileName).arg(size.width()).arg(size.height()), 3000); }
        else QMessageBox::warning(this, tr("Error"), tr("No se pudo guardar el archivo SVG."));
    } else {
        const char *format = "PNG";
        if (lowerName.endsWith(".jpg") || lowerName.endsWith(".jpeg")) format = "JPEG";
        ok = paintArea->guardarImagen(fileName, format);
        QApplication::restoreOverrideCursor();
        if (ok) statusBar()->showMessage(tr("Guardado: %1").arg(fileName), 2000);
        else QMessageBox::warning(this, tr("Error"), tr("No se pudo guardar la imagen."));
    }
    return ok;
}

void mainwind::exportarGif() {
    if (!paintArea->getIsPixelArtMode()) { QMessageBox::warning(this, tr("Modo incorrecto"), tr("La exportacion GIF solo esta disponible en modo Pixel Art.")); return; }
    const QList<QImage> &frames = paintArea->getFrames();
    if (frames.size() < 2) { QMessageBox::warning(this, tr("Pocos frames"), tr("Necesitas al menos 2 frames para crear un GIF animado.")); return; }

    int resOriginal = paintArea->getPixelResolution();
    ThemeColors colors(darkMode);

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Exportar animacion GIF"));
    dlg.setFixedSize(480, 400);
    dlg.setStyleSheet(QString("QDialog { background-color: %1; }").arg(colors.bgDialog));

    QVBoxLayout *mainLayout = new QVBoxLayout(&dlg);
    mainLayout->setContentsMargins(24, 24, 24, 24); mainLayout->setSpacing(16);

    QGroupBox *boxPlayback = new QGroupBox(tr("Reproduccion"));
    boxPlayback->setStyleSheet(QString("QGroupBox { background-color: %1; border: 1px solid %2; border-radius: 10px; margin-top: 14px; padding: 18px 16px 16px 16px; font-weight: 600; color: %3; } QGroupBox::title { subcontrol-origin: margin; left: 18px; padding: 0 10px; font-size: 13px; }").arg(colors.bgPanel, colors.border, colors.textPrimary));
    QVBoxLayout *playbackLayout = new QVBoxLayout(boxPlayback); playbackLayout->setSpacing(14);

    QHBoxLayout *fpsRow = new QHBoxLayout(); fpsRow->setSpacing(12);
    QLabel *lblFps = new QLabel(tr("Velocidad (FPS):"));
    lblFps->setStyleSheet(QString("font-weight: 500; color: %1; min-width: 120px;").arg(colors.textPrimary));
    QSpinBox *fpsSpin = new QSpinBox(); fpsSpin->setRange(1, 60); fpsSpin->setValue(10); fpsSpin->setSuffix(" fps"); fpsSpin->setFixedWidth(90);
    fpsSpin->setStyleSheet(QString("QSpinBox { background-color: %1; border: 1px solid %2; border-radius: 6px; padding: 5px 8px; font-size: 12px; color: %3; } QSpinBox:focus { border: 1px solid %4; }").arg(colors.bgInput, colors.border, colors.textPrimary, colors.accent));
    QSlider *fpsSlider = new QSlider(Qt::Horizontal); fpsSlider->setRange(1, 60); fpsSlider->setValue(10);
    fpsSlider->setStyleSheet(QString("QSlider::groove:horizontal { height: 4px; background: %1; border-radius: 2px; } QSlider::handle:horizontal { background: %2; width: 14px; height: 14px; margin: -5px 0; border-radius: 7px; } QSlider::sub-page:horizontal { background: %2; border-radius: 2px; }").arg(colors.border, colors.accent));
    connect(fpsSlider, &QSlider::valueChanged, fpsSpin, [fpsSpin](int v) { fpsSpin->setValue(v); });
    connect(fpsSpin, QOverload<int>::of(&QSpinBox::valueChanged), fpsSlider, [fpsSlider](int v) { fpsSlider->setValue(v); });
    fpsRow->addWidget(lblFps); fpsRow->addWidget(fpsSlider, 1); fpsRow->addWidget(fpsSpin);
    playbackLayout->addLayout(fpsRow);

    QHBoxLayout *loopRow = new QHBoxLayout();
    QCheckBox *chkLoop = new QCheckBox(tr("Repetir en bucle")); chkLoop->setChecked(true);
    chkLoop->setStyleSheet(QString("color: %1; font-size: 12px;").arg(colors.textPrimary));
    loopRow->addWidget(chkLoop); loopRow->addStretch();
    playbackLayout->addLayout(loopRow);
    mainLayout->addWidget(boxPlayback);

    QGroupBox *boxScale = new QGroupBox(tr("Escala de salida"));
    boxScale->setStyleSheet(boxPlayback->styleSheet());
    QVBoxLayout *scaleLayout = new QVBoxLayout(boxScale); scaleLayout->setSpacing(14);

    QHBoxLayout *scaleToggleRow = new QHBoxLayout();
    QCheckBox *chkScale = new QCheckBox(tr("Reescalar para mejor visualizacion")); chkScale->setChecked(true);
    chkScale->setStyleSheet(QString("font-weight: 600; color: %1;").arg(colors.textPrimary));
    scaleToggleRow->addWidget(chkScale); scaleToggleRow->addStretch();
    scaleLayout->addLayout(scaleToggleRow);

    QWidget *scaleOptions = new QWidget();
    scaleOptions->setStyleSheet("background-color: transparent;");
    QVBoxLayout *scaleOptionsLayout = new QVBoxLayout(scaleOptions);
    scaleOptionsLayout->setContentsMargins(0, 0, 0, 0); scaleOptionsLayout->setSpacing(10);

    QHBoxLayout *factorRow = new QHBoxLayout(); factorRow->setSpacing(12);
    QLabel *lblFactor = new QLabel(tr("Factor:")); lblFactor->setStyleSheet(QString("font-weight: 500; color: %1; min-width: 120px;").arg(colors.textPrimary));
    QSpinBox *scaleSpin = new QSpinBox(); scaleSpin->setRange(1, 32); scaleSpin->setValue(8); scaleSpin->setSuffix("x"); scaleSpin->setFixedWidth(90); scaleSpin->setStyleSheet(fpsSpin->styleSheet());
    QSlider *scaleSlider = new QSlider(Qt::Horizontal); scaleSlider->setRange(1, 32); scaleSlider->setValue(8); scaleSlider->setStyleSheet(fpsSlider->styleSheet());
    connect(scaleSlider, &QSlider::valueChanged, scaleSpin, [scaleSpin](int v) { scaleSpin->setValue(v); });
    connect(scaleSpin, QOverload<int>::of(&QSpinBox::valueChanged), scaleSlider, [scaleSlider](int v) { scaleSlider->setValue(v); });
    factorRow->addWidget(lblFactor); factorRow->addWidget(scaleSlider, 1); factorRow->addWidget(scaleSpin);
    scaleOptionsLayout->addLayout(factorRow);

    QHBoxLayout *presetsRow = new QHBoxLayout(); presetsRow->setSpacing(6);
    QLabel *lblPresets = new QLabel(tr("Rapido:")); lblPresets->setStyleSheet(QString("font-weight: 500; color: %1; font-size: 11px; min-width: 120px;").arg(colors.textMuted));
    presetsRow->addWidget(lblPresets);
    auto makePresetBtn = [&colors](const QString &text, int value, QSpinBox *spin) {
        QPushButton *btn = new QPushButton(text); btn->setCursor(Qt::PointingHandCursor); btn->setFixedHeight(28);
        btn->setStyleSheet(QString("QPushButton { background-color: %1; border: 1px solid %2; border-radius: 5px; padding: 0 12px; font-size: 11px; font-weight: 500; color: %3; } QPushButton:hover { background-color: %4; border-color: %5; color: %6; }").arg(colors.bgInput, colors.border, colors.textPrimary, colors.bgHover, colors.accent, colors.textAccent));
        connect(btn, &QPushButton::clicked, [spin, value]() { spin->setValue(value); });
        return btn;
    };
    presetsRow->addWidget(makePresetBtn("2x", 2, scaleSpin)); presetsRow->addWidget(makePresetBtn("4x", 4, scaleSpin));
    presetsRow->addWidget(makePresetBtn("8x", 8, scaleSpin)); presetsRow->addWidget(makePresetBtn("16x", 16, scaleSpin)); presetsRow->addStretch();
    scaleOptionsLayout->addLayout(presetsRow);

    QLabel *previewLabel = new QLabel(); previewLabel->setAlignment(Qt::AlignCenter); previewLabel->setFixedHeight(40); previewLabel->setWordWrap(true);
    previewLabel->setStyleSheet(QString("QLabel { background-color: %1; border: 1px solid %2; border-radius: 6px; padding: 8px; font-size: 12px; color: %3; }").arg(colors.bgPreview, colors.border, colors.textPrimary));
    scaleOptionsLayout->addWidget(previewLabel);

    auto updatePreview = [previewLabel, scaleSpin, chkScale, resOriginal]() {
        int scale = chkScale->isChecked() ? scaleSpin->value() : 1;
        int finalSize = resOriginal * scale;
        previewLabel->setText(QObject::tr("Tamano final: <b>%1 x %1 px</b>").arg(finalSize));
    };
    connect(chkScale, &QCheckBox::toggled, scaleOptions, [scaleOptions, updatePreview](bool checked) { scaleOptions->setEnabled(checked); updatePreview(); });
    connect(scaleSpin, QOverload<int>::of(&QSpinBox::valueChanged), updatePreview);
    updatePreview();

    scaleLayout->addWidget(scaleOptions);
    mainLayout->addWidget(boxScale);

    QHBoxLayout *buttonsLayout = new QHBoxLayout(); buttonsLayout->setSpacing(12); buttonsLayout->addStretch();
    QPushButton *btnCancel = new QPushButton(tr("Cancelar")); btnCancel->setCursor(Qt::PointingHandCursor); btnCancel->setFixedHeight(40); btnCancel->setMinimumWidth(100);
    btnCancel->setStyleSheet(QString("QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: 8px; padding: 0 18px; font-size: 13px; font-weight: 500; } QPushButton:hover { background-color: %4; }").arg(colors.bgInput, colors.textPrimary, colors.border, colors.bgHover));
    QPushButton *btnExport = new QPushButton(tr("Exportar GIF")); btnExport->setCursor(Qt::PointingHandCursor); btnExport->setFixedHeight(40); btnExport->setMinimumWidth(130);
    btnExport->setStyleSheet(QString("QPushButton { background-color: %1; color: white; border: none; border-radius: 8px; padding: 0 22px; font-size: 13px; font-weight: 600; } QPushButton:hover { background-color: %2; } QPushButton:pressed { background-color: %3; }").arg(colors.accent, colors.accentHover, colors.accentPressed));
    buttonsLayout->addWidget(btnCancel); buttonsLayout->addWidget(btnExport);
    mainLayout->addLayout(buttonsLayout);

    connect(btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(btnExport, &QPushButton::clicked, &dlg, &QDialog::accept);

    if (dlg.exec() != QDialog::Accepted) return;

    int fpsValue = fpsSpin->value(), delayMs = 1000 / fpsValue;
    int scaleValue = chkScale->isChecked() ? scaleSpin->value() : 1;

    QString fileName = QFileDialog::getSaveFileName(this, tr("Guardar GIF como"), QDir::currentPath() + "/animacion.gif", tr("GIF animado (*.gif)"));
    if (fileName.isEmpty()) return;
    if (!fileName.toLower().endsWith(".gif")) fileName += ".gif";

    QApplication::setOverrideCursor(Qt::WaitCursor);
    bool ok = paintArea->guardarComoGif(fileName, delayMs, scaleValue);
    QApplication::restoreOverrideCursor();

    if (ok) {
        int finalW = resOriginal * scaleValue;
        QMessageBox::information(this, tr("Exportacion exitosa"), tr("Animacion guardada:\n%1\n%2 frames a %3 FPS\nEscala: %4x  ->  Tamano final: %5 x %5 px").arg(fileName).arg(frames.size()).arg(fpsValue).arg(scaleValue).arg(finalW));
        statusBar()->showMessage(tr("GIF: %1 (%2x%3 px)").arg(fileName).arg(finalW).arg(finalW), 3000);
    } else { QMessageBox::warning(this, tr("Error"), tr("No se pudo guardar el GIF.")); }
}