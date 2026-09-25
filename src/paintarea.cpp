#include "paintarea.h"
#include "tools/BezierPathTool.h"
#include "tools/TextEngine.h"
#include "core/MaskEditController.h"

/// EXIF helpers
int leerOrientacionExif(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return 1;
    QByteArray data = file.read(65536);
    file.close();
    const uchar *d = (const uchar *)data.constData();
    int len = data.size();
    if (len < 4 || d[0] != 0xFF || d[1] != 0xD8) return 1;
    int pos = 2;
    while (pos + 4 < len) {
        if (d[pos] != 0xFF) { pos++; continue; }
        uchar marker = d[pos + 1];
        if (marker == 0xD8 || marker == 0x01 || (marker >= 0xD0 && marker <= 0xD9)) { pos += 2; continue; }
        int segLen = (d[pos + 2] << 8) | d[pos + 3];
        if (segLen < 2) break;
        if (marker == 0xE1 && pos + 2 + segLen <= len) {
            const uchar *seg = d + pos + 4;
            int segData = segLen - 2;
            if (segData >= 14 &&
                seg[0] == 'E' && seg[1] == 'x' && seg[2] == 'i' && seg[3] == 'f' &&
                seg[4] == 0 && seg[5] == 0) {
                const uchar *tiff = seg + 6;
                int tiffLen = segData - 6;
                if (tiffLen < 8) return 1;
                bool little;
                if (tiff[0] == 'I' && tiff[1] == 'I') little = true;
                else if (tiff[0] == 'M' && tiff[1] == 'M') little = false;
                else return 1;
                auto rd16 = [&](int off) -> int {
                    if (off < 0 || off + 1 >= tiffLen) return 0;
                    return little ? (tiff[off] | (tiff[off + 1] << 8))
                                  : ((tiff[off] << 8) | tiff[off + 1]);
                };
                auto rd32 = [&](int off) -> int {
                    if (off < 0 || off + 3 >= tiffLen) return 0;
                    return little ? (tiff[off] | (tiff[off+1] << 8) | (tiff[off+2] << 16) | (tiff[off+3] << 24))
                                  : ((tiff[off] << 24) | (tiff[off+1] << 16) | (tiff[off+2] << 8) | tiff[off+3]);
                };
                int ifd0 = rd32(4);
                if (ifd0 <= 0 || ifd0 + 2 > tiffLen) return 1;
                int entries = rd16(ifd0);
                for (int i = 0; i < entries; ++i) {
                    int entry = ifd0 + 2 + i * 12;
                    if (entry + 12 > tiffLen) break;
                    if (rd16(entry) == 0x0112) {
                        int val = rd16(entry + 8);
                        return (val >= 1 && val <= 8) ? val : 1;
                    }
                }
                return 1;
            }
        }
        pos += 2 + segLen;
    }
    return 1;
}

/// Aplica orientación EXIF a imagen
QImage aplicaOrientacionExif(const QImage &img, int orientation) {
    switch (orientation) {
        case 2: return img.mirrored(true, false);
        case 3: return img.transformed(QTransform().rotate(180), Qt::FastTransformation);
        case 4: return img.mirrored(false, true);
        case 5: return img.transformed(QTransform().rotate(90),  Qt::FastTransformation).mirrored(true, false);
        case 6: return img.transformed(QTransform().rotate(90),  Qt::FastTransformation);
        case 7: return img.transformed(QTransform().rotate(270), Qt::FastTransformation).mirrored(true, false);
        case 8: return img.transformed(QTransform().rotate(270), Qt::FastTransformation);
        default: return img;
    }
}

/// Carga imagen respetando EXIF
QImage cargarImagenRespetandoExif(const QString &filePath) {
    QImage img(filePath);
    if (img.isNull()) return img;
    int orient = leerOrientacionExif(filePath);
    if (orient > 1) img = aplicaOrientacionExif(img, orient);
    return img;
}

/// ========== FrameThumbnail ==========
FrameThumbnail::FrameThumbnail(const QImage &img, int index, QWidget *parent)
    : QFrame(parent), frameImage(img), frameIndex(index), isSelected(false) {
    setFixedSize(THUMB_SIZE + 8, THUMB_SIZE + 20);
    setCursor(Qt::PointingHandCursor);
    setToolTip(tr("Frame %1").arg(index + 1));
}
void FrameThumbnail::setSelected(bool selected) { isSelected = selected; update(); }
void FrameThumbnail::setFrameImage(const QImage &img) { frameImage = img; update(); }
int FrameThumbnail::getFrameIndex() const { return frameIndex; }

void FrameThumbnail::paintEvent(QPaintEvent *) {
    QPainter painter(this); painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.fillRect(rect(), isSelected ? QColor("#1a4a7c") : QColor("#2b2b2b"));
    painter.setPen(QPen(isSelected ? QColor("#0066cc") : QColor("#444444"), isSelected ? 2 : 1));
    painter.drawRect(1, 1, width() - 2, height() - 2);
    QPixmap checker(8, 8); checker.fill(Qt::white);
    QPainter p(&checker);
    p.fillRect(0, 0, 4, 4, QColor(210, 210, 210));
    p.fillRect(4, 4, 4, 4, QColor(210, 210, 210)); p.end();
    painter.fillRect(2, 5, width() - 4, THUMB_SIZE - 2, QBrush(checker));
    if (!frameImage.isNull()) {
        QImage scaled = frameImage.scaled(THUMB_SIZE, THUMB_SIZE, Qt::KeepAspectRatio, Qt::FastTransformation);
        painter.drawImage((width() - scaled.width()) / 2, 4, scaled);
    }
    painter.setPen(isSelected ? Qt::white : QColor("#aaa"));
    painter.setFont(QFont("Adwaita Sans", 8));
    painter.drawText(QRect(0, THUMB_SIZE + 6, width(), 14), Qt::AlignCenter, QString::number(frameIndex + 1));
}
void FrameThumbnail::mousePressEvent(QMouseEvent *) { emit clicked(frameIndex); }

/// ========== PaintArea: helpers ==========
QColor PaintArea::selBlue() const { return darkModeActive ? QColor("#60a5fa") : QColor("#2563eb"); }
QColor PaintArea::selBlueLight() const { return darkModeActive ? QColor("#93c5fd") : QColor("#3b82f6"); }

bool PaintArea::herramientaVectorizable() const {
    return ToolCategories::isVectorizableTool(currentTool);
}

QColor PaintArea::colorVectorActivo() const {
    if (currentTool == ToolLassoDelete)  return QColor(236, 72, 153);
    if (currentTool == ToolLassoExtract) return QColor(59, 130, 246);
    return QColor(16, 185, 129);
}

bool PaintArea::herramientaDePintura() const {
    return ToolCategories::isPaintingTool(currentTool);
}

/// ========== Helpers colapsadores ==========
const BrushSettings& PaintArea::activePreset() const {
    if (ArtisticPresets::isArtisticTool(currentTool)) return classicToolPreset;
    if (currentTool == ToolCustomBrush) return customBrushPresets[activeCustomBrushIndex];
    static BrushSettings empty;
    return empty;
}

const QImage& PaintArea::activeStamp() const {
    if (ArtisticPresets::isArtisticTool(currentTool)) {
        return (activeMouseButton == Qt::RightButton) ? classicToolStampRight : classicToolStamp;
    }
    if (currentTool == ToolCustomBrush) {
        return (activeMouseButton == Qt::RightButton) ? customBrushStampRight : customBrushStamp;
    }
    static QImage empty;
    return empty;
}

void PaintArea::refreshAndNotify(bool recompose) {
    if (recompose) recomponerImagen();
    emit layersChanged();
    update();
}

void PaintArea::bakeAllPending() {
    bakeSelection();
    bakeActivePath();
    bakeTextFrame();
    bakeAllObjects();
    cancelVectorMode();
}

void PaintArea::beginEdit() {
    bakeAllPending();
    saveHistoryState();
}

/// Mapea ToolType de retoque al código numérico que usa RetouchTools.
/// 101 = ShadowBurn, 201 = Blur, 202 = Heal (según RetouchTools.h).
static int retouchToolCode(ToolType t) {
    switch (t) {
        case ToolBlur:        return 201;
        case ToolHeal:        return 202;
        case ToolShadowBurn:  return 101;
        default:              return 0;
    }
}

/// Configura MaskEditController
void PaintArea::configureMaskEditController() {
    MaskEditController::Context ctx;
    ctx.maskRef = [this]() -> QImage& {
        return stack.maskRef(stack.maskEditLayer());
    };
    ctx.isEditingMask = [this]() { return stack.isEditingMask(); };
    ctx.maskEditLayer = [this]() { return stack.maskEditLayer(); };
    ctx.brushColorFor = [this](Qt::MouseButton b) { return obtenerColorDeTrabajo(b); };
    ctx.brushPresetFor = [this]() -> BrushSettings {
        if (ArtisticPresets::isArtisticTool(currentTool)) return classicToolPreset;
        if (currentTool == ToolCustomBrush) return customBrushPresets[activeCustomBrushIndex];
        BrushSettings s;
        s.shape = ShapeType::Circle;
        s.dragMode = DragMode::Continuous;
        s.rotationMode = RotationMode::Fixed;
        s.size = qMax(5, (int)(penWidth * mouseSensitivity * 2));
        return s;
    };
    ctx.isPixelArtMode = [this]() { return pixelOptions.getIsPixelArtMode(); };
    m_maskEdit.setContext(ctx);

    m_maskEdit.onRepaint = [this](const QRect &r) {
        QRect rr = r.intersected(stack.canvasRect());
        if (rr.isEmpty()) return;
        stack.syncMaskAlphaRegion(stack.maskEditLayer(), rr);
        stack.syncRubylithRegion(stack.maskEditLayer(), rr);
        stack.markDirty(rr);
        stack.clearPreviewCache();
        repintarZonaCanvas(rr);
    };
    m_maskEdit.onMaskChanged = [this]() { emit layersChanged(); };
    m_maskEdit.onStatusMessage = [this](const QString &msg) { emit statusBarMessage(msg); };
}

/// ========== Helpers de margen / invalidación ==========
int PaintArea::margenHerramienta() const {
    int sw = qMax(1, static_cast<int>(penWidth * mouseSensitivity));
    if (usaStampDePincel()) {
        const BrushSettings &c = activePreset();
        int base = qMax(c.size, sw);
        int compositeSpread = c.shapeElements.isEmpty() ? 0 : base * 2;
        return (int)((base + 12) * 1.25) + compositeSpread + c.scatter + 8;
    }
    switch (currentTool) {
        case ToolPencil:      return sw * 2 + 6;
        case ToolBlur: case ToolHeal: case ToolShadowBurn: return sw * 2 + 4;
        case ToolEraser:      return sw + 4;
        case ToolClone:       return sw * 2 + 6;
        case ToolDeform:      return m_deform.options().radius + 10;
        default: return sw + 6;
    }
}

QRect PaintArea::rectCanvasAWidget(const QRect &r) const {
    if (r.isEmpty()) return QRect();
    int x1 = (int)floor(r.left() * zoomFactor);
    int y1 = (int)floor(r.top() * zoomFactor);
    int x2 = (int)ceil(r.right() * zoomFactor);
    int y2 = (int)ceil(r.bottom() * zoomFactor);
    return QRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}

void PaintArea::repintarZonaCanvas(const QRect &canvasRect) {
    QRect c = canvasRect.intersected(stack.canvasRect());
    if (c.isEmpty()) return;
    update(rectCanvasAWidget(c).adjusted(-3, -3, 3, 3));
}

QRect PaintArea::rectSiluetaWidget(const QPoint &widgetPos) const {
    int m = (int)(margenHerramienta() * zoomFactor) + 10;
    return QRect(widgetPos.x() - m, widgetPos.y() - m, m * 2 + 1, m * 2 + 1);
}

void PaintArea::invalidarTrazo(const QPoint &a, const QPoint &b, const QRect &extraCanvas) {
    int m = margenHerramienta();
    QRect r = QRect(a, b).normalized().adjusted(-m, -m, m, m);
    r = r.intersected(stack.canvasRect());
    if (!r.isEmpty()) stack.markDirty(r);
    QRect wr = rectCanvasAWidget(r);
    if (!extraCanvas.isEmpty()) {
        QRect re = extraCanvas.intersected(stack.canvasRect());
        if (!re.isEmpty()) wr = wr.united(rectCanvasAWidget(re));
    }
    if (!wr.isEmpty()) update(wr.adjusted(-2, -2, 2, 2));
}

void PaintArea::invalidarPreviewClone(const QPoint &cursorPos) {
    if (!cloneSourceSet) return;
    int brushSize = qMax(1, static_cast<int>(penWidth * mouseSensitivity)) * 2;
    QRect sourceArea(cloneSource.x() - brushSize - 4, cloneSource.y() - brushSize - 4,
                     brushSize * 2 + 8, brushSize * 2 + 8);
    QRect cursorArea(cursorPos.x() - brushSize - 4, cursorPos.y() - brushSize - 4,
                     brushSize * 2 + 8, brushSize * 2 + 8);
    QRect lineArea = QRect(cloneSource, cursorPos).normalized().adjusted(-4, -4, 4, 4);
    QRect totalArea = sourceArea.united(cursorArea).united(lineArea);
    totalArea = totalArea.intersected(stack.canvasRect());
    if (!totalArea.isEmpty()) {
        QRect widgetArea = rectCanvasAWidget(totalArea).adjusted(-4, -4, 4, 4);
        update(widgetArea);
    }
}

/// ========== Tiles / sincronización con animación ==========
void PaintArea::renderTiles(QPainter &painter, const QRect &visibleWidgetRect) {
    if (!stack.tilesValid()) return;
    QRect canvasVisible(
        (int)floor(visibleWidgetRect.x() / zoomFactor),
        (int)floor(visibleWidgetRect.y() / zoomFactor),
        (int)ceil(visibleWidgetRect.width() / zoomFactor) + 2,
        (int)ceil(visibleWidgetRect.height() / zoomFactor) + 2);
    canvasVisible = canvasVisible.intersected(stack.canvasRect());
    if (canvasVisible.isEmpty()) return;
    QVector<int> tiles = stack.tilesIntersecting(canvasVisible);
    for (int idx : tiles) {
        if (stack.isTileDirty(idx)) stack.refreshTile(idx);
    }
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    for (int idx : tiles) {
        QRect tr = stack.tileRect(idx);
        int wx = (int)floor(tr.x() * zoomFactor);
        int wy = (int)floor(tr.y() * zoomFactor);
        int ww = (int)ceil((tr.x() + tr.width()) * zoomFactor) - wx;
        int wh = (int)ceil((tr.y() + tr.height()) * zoomFactor) - wy;
        painter.save();
        painter.resetTransform();
        painter.drawPixmap(wx, wy, ww, wh, stack.tilePixmap(idx));
        painter.restore();
    }
}

void PaintArea::sincronizarCapasConFrameActual() {
    if (!pixelOptions.getIsPixelArtMode()) return;
    QImage frameImg = animManager.getCurrentFrameImage();
    if (frameImg.isNull()) {
        frameImg = QImage(pixelOptions.getResolution(), pixelOptions.getResolution(), QImage::Format_ARGB32);
        frameImg.fill(Qt::transparent);
    }
    stack.resetWithSingleLayer(frameImg, tr("Capa 1"));
    update();
}

void PaintArea::guardarFrameActualEnAnimador() {
    if (!pixelOptions.getIsPixelArtMode()) return;
    animManager.setCurrentFrameImage(stack.compositedImage());
}

void PaintArea::procesarDibujoContinuo() {
    if (!drawing || !capaValida() || stack.currentLocked()) return;
    if (stack.isEditingMask()) return;
    if (!usaStampDePincel()) return;

    const BrushSettings &preset = activePreset();
    if (!preset.isAirbrush && preset.dragMode != DragMode::Scattered) return;

    QColor colorDeUso = obtenerColorDeTrabajo(activeMouseButton);
    QColor colorOpuesto = obtenerColorDeTrabajo(activeMouseButton == Qt::LeftButton ? Qt::RightButton : Qt::LeftButton);
    colorDeUso.setAlpha(penOpacity);

    PaintEngine::applyCustomBrushStroke(stack.currentImage(), currentMousePos,
        activeStamp(), preset, mouseSensitivity,
        0.0, 1.0, 1.0, colorDeUso, colorOpuesto, strokeCanvasFallback);
    invalidarTrazo(currentMousePos, currentMousePos);
}

QColor PaintArea::obtenerColorDeTrabajo(Qt::MouseButton button) {
    if (currentTool == ToolEraser) return Qt::transparent;
    if (activeColorTarget == 2) return (button == Qt::LeftButton) ? penColor2 : penColor1;
    return (button == Qt::LeftButton) ? penColor1 : penColor2;
}

void PaintArea::applyClonStamp(QImage &target, const QPoint &destPos) {
    if (!cloneSourceSet || cloneBuffer.isNull()) return;
    QPoint offset = cloneSource - cloneInitialDest;
    QPoint currentSource = destPos + offset;
    int brushSize = qMax(1, static_cast<int>(penWidth * mouseSensitivity)) * 2;
    for (int y = -brushSize; y <= brushSize; ++y) {
        for (int x = -brushSize; x <= brushSize; ++x) {
            if (x*x + y*y > brushSize*brushSize) continue;
            QPoint srcP = currentSource + QPoint(x, y);
            QPoint dstP = destPos + QPoint(x, y);
            if (srcP.x() < 0 || srcP.x() >= cloneBuffer.width() ||
                srcP.y() < 0 || srcP.y() >= cloneBuffer.height()) continue;
            if (dstP.x() < 0 || dstP.x() >= target.width() ||
                dstP.y() < 0 || dstP.y() >= target.height()) continue;
            QColor srcColor = cloneBuffer.pixelColor(srcP);
            if (srcColor.alpha() > 0) target.setPixelColor(dstP, srcColor);
        }
    }
}

void PaintArea::aplicarGradienteConfigurado(const QPoint &p1, const QPoint &p2) {
    if (!puedeEditarCapaActual()) return;
    GradientTools::applyGradient(
        stack.currentImage(), p1, p2,
        penColor1, penColor2,
        (int)gradientType, gradientOpacity, gradientAngle,
        gradientReverse, gradientDither, gradientUseSecondColor,
        gradientBlendMode);
}

void PaintArea::openGradientSettings() {
    GradientDialog dlg(darkModeActive, penColor1, penColor2,
                       (int)gradientType, gradientOpacity, gradientAngle,
                       gradientReverse, gradientDither, gradientBlendMode,
                       gradientUseSecondColor, this);
    if (dlg.exec() == QDialog::Accepted) {
        gradientType = (GradientType)dlg.getGradientType();
        gradientOpacity = dlg.getOpacity();
        gradientAngle = dlg.getAngle();
        gradientReverse = dlg.getReverse();
        gradientDither = dlg.getDither();
        gradientBlendMode = dlg.getBlendMode();
        gradientUseSecondColor = dlg.getUseSecondColor();
        QString typeStr = gradientType == GradientLinear ? tr("Lineal") :
                          gradientType == GradientRadial ? tr("Radial") : tr("Cónico");
        QString colorStr = gradientUseSecondColor ? tr("2 colores") : tr("Color → Transp.");
        emit statusBarMessage(tr("Gradiente: %1 | %2° | %3% | %4")
            .arg(typeStr).arg(gradientAngle)
            .arg(qRound(gradientOpacity / 255.0 * 100)).arg(colorStr));
    }
}

/// ========== Selección y objetos ==========
void PaintArea::bakeObjectIntoLayer(int idx) {
    if (idx < 0 || idx >= selMgr.objectCount()) return;
    const PaintObject &obj = selMgr.objectAt(idx);
    int layerIdx = obj.layerIndex;
    if (!capaValida(layerIdx)) layerIdx = stack.currentIndex();
    if (!capaValida(layerIdx)) return;
    selMgr.bakeObjectInto(idx, stack.layerAt(layerIdx).image, !pixelOptions.getIsPixelArtMode());
}

void PaintArea::bakeAllObjects() {
    if (!selMgr.hasObjects()) return;
    for (int i = 0; i < selMgr.objectCount(); ++i) bakeObjectIntoLayer(i);
    selMgr.clearObjects();
    recomponerImagen();
}

void PaintArea::convertSelectionToObject() {
    if (!selMgr.isActive() || !selMgr.hasBuffer()) { emit statusBarMessage(tr("Sin selección")); return; }
    if (!capaValida()) return;
    saveHistoryState();
    selMgr.registerSelectionImageObject(selMgr.rect(), selMgr.buffer(), selMgr.rotation(), stack.currentIndex());
    selMgr.discard();
    setTool(ToolMove);
    emit statusBarMessage(tr("Convertido en objeto"));
    refreshAndNotify();
}

void PaintArea::integrateSelectedObjects() {
    QList<int> selected = selMgr.selectedObjectIndices();
    if (selected.isEmpty()) {
        if (selMgr.hasObjects()) {
            saveHistoryState();
            bakeAllObjects();
            emit statusBarMessage(tr("Integrado al lienzo"));
            refreshAndNotify(false);
        } else { emit statusBarMessage(tr("Sin objetos")); }
        return;
    }
    if (!capaValida()) return;
    saveHistoryState();
    for (int idx : selected) bakeObjectIntoLayer(idx);
    std::sort(selected.begin(), selected.end(), std::greater<int>());
    for (int idx : selected) selMgr.removeObjectAt(idx);
    selMgr.setActiveObjectIndex(-1);
    emit statusBarMessage(tr("Integrado al lienzo"));
    refreshAndNotify();
}

void PaintArea::loadTextObjectForEditing(int idx) {
    if (idx < 0 || idx >= selMgr.objectCount()) return;
    if (selMgr.objectAt(idx).type != ObjectType::Text) return;
    PaintObject obj = selMgr.takeObjectAt(idx);
    textEdit.loadExisting(obj.bounds.toRect(), obj.textContent, obj.textFont, obj.textColor);
    lastUsedTextFont = obj.textFont;
    emit textFrameClicked(textEdit.rect.topLeft());
    update();
}

void PaintArea::loadShapeObjectForEditing(int idx) {
    if (idx < 0 || idx >= selMgr.objectCount()) return;
    if (selMgr.objectAt(idx).type != ObjectType::Shape) return;
    PaintObject &obj = selMgr.objectAt(idx);
    ShapePropertiesDialog dlg(darkModeActive, obj.shapeTool, obj.fillColor, obj.strokeColor,
                              obj.strokeWidth, obj.hollow, obj.isFrame, obj.frameImage,
                              obj.frameImageScale, obj.frameImageOffset, this);
    if (dlg.exec() == QDialog::Accepted) {
        saveHistoryState();
        obj.fillColor = dlg.getFillColor();
        obj.strokeColor = dlg.getStrokeColor();
        obj.strokeWidth = dlg.getStrokeWidth();
        obj.hollow = dlg.getHollow();
        obj.isFrame = dlg.getIsFrame();
        obj.frameImage = dlg.getFrameImage();
        obj.frameImageScale = dlg.getFrameImageScale();
        obj.frameImageOffset = dlg.getFrameImageOffset();
        emit statusBarMessage(tr("Figura actualizada"));
        refreshAndNotify();
    }
}

/// ========== Handles del canvas ==========
QRect PaintArea::getRightHandle() const { return QRect(stack.width() * zoomFactor, stack.height() * zoomFactor / 2 - HANDLE_SIZE/2, HANDLE_SIZE, HANDLE_SIZE); }
QRect PaintArea::getBottomHandle() const { return QRect(stack.width() * zoomFactor / 2 - HANDLE_SIZE/2, stack.height() * zoomFactor, HANDLE_SIZE, HANDLE_SIZE); }
QRect PaintArea::getBottomRightHandle() const { return QRect(stack.width() * zoomFactor, stack.height() * zoomFactor, HANDLE_SIZE, HANDLE_SIZE); }

/// ========== Vector helpers ==========
int PaintArea::findVectorPointAt(const QPointF &canvasPos) const {
    double threshold = 8.0 / zoomFactor;
    for (int i = 0; i < vectorPoints.size(); ++i) {
        double dx = vectorPoints[i].x() - canvasPos.x();
        double dy = vectorPoints[i].y() - canvasPos.y();
        if (sqrt(dx*dx + dy*dy) <= threshold) return i;
    }
    return -1;
}

bool PaintArea::isNearFirstPoint(const QPointF &canvasPos) const {
    if (vectorPoints.size() < 3) return false;
    double threshold = 12.0 / zoomFactor;
    double dx = vectorPoints.first().x() - canvasPos.x();
    double dy = vectorPoints.first().y() - canvasPos.y();
    return sqrt(dx*dx + dy*dy) <= threshold;
}

void PaintArea::finalizeVectorPath() {
    if (vectorPoints.size() < 3) { vectorPoints.clear(); vectorEditMode = false; update(); return; }
    QPainterPath path;
    path.moveTo(vectorPoints.first());
    for (int i = 1; i < vectorPoints.size(); ++i) path.lineTo(vectorPoints[i]);
    path.closeSubpath();
    if (currentTool == ToolLassoExtract || currentTool == ToolLassoDelete) {
        if (puedeEditarCapaActual()) {
            saveHistoryState();
            if (currentTool == ToolLassoExtract) {
                stack.currentImage() = LassoProcessor::applyLassoExtract(stack.currentImage(), path, pixelOptions.getIsPixelArtMode());
                emit statusBarMessage(tr("Fondo recortado"));
            } else {
                LassoProcessor::applyLassoDelete(stack.currentImage(), path, pixelOptions.getIsPixelArtMode());
                emit statusBarMessage(tr("Objeto borrado"));
            }
            refreshAndNotify();
        }
        vectorPoints.clear(); vectorEditMode = false; selectedVectorPoint = -1;
        update(); return;
    }
    saveHistoryState();
    if (selMgr.finalizeVectorPath(path, stack.currentImage())) {
        emit statusBarMessage(tr("Selección vectorial"));
        refreshAndNotify();
    }
    vectorPoints.clear(); vectorEditMode = false; selectedVectorPoint = -1;
    update();
}

void PaintArea::cancelVectorMode() {
    vectorPoints.clear(); vectorEditMode = false;
    selectedVectorPoint = -1; draggingVectorPoint = false;
    update();
}

void PaintArea::updateClassicToolStamp() {
    QColor leftBase  = obtenerColorDeTrabajo(Qt::LeftButton);
    QColor rightBase = obtenerColorDeTrabajo(Qt::RightButton);
    QColor secondL = classicToolPreset.mixSecondColor ? rightBase : QColor();
    QColor secondR = classicToolPreset.mixSecondColor ? leftBase  : QColor();
    classicToolStamp = PaintEngine::generateBrushStamp(classicToolPreset, leftBase, penOpacity, pixelOptions.getIsPixelArtMode(), secondL);
    classicToolStampRight = PaintEngine::generateBrushStamp(classicToolPreset, rightBase, penOpacity, pixelOptions.getIsPixelArtMode(), secondR);
}

/// ========== Constructor ==========
PaintArea::PaintArea(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_StaticContents);
    setMouseTracking(true);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAutoFillBackground(false);
    customBrushPresets[0] = BrushSettings();
    customBrushPresets[1] = BrushSettings();
    customBrushPresets[1].shape = ShapeType::Square;
    classicToolPreset = ArtisticPresets::softBrush();
    lastUsedTextFont = QFont("Adwaita Sans", 24);
    continuousDrawTimer = new QTimer(this);
    connect(continuousDrawTimer, &QTimer::timeout, this, &PaintArea::procesarDibujoContinuo);

    connect(&textEdit, &TextEngine::EditSession::needsRepaint, this, [this]() {
        update();
    });

    configureMaskEditController();

    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    int initW = qMin(1280, screenGeometry.width() - 200);
    int initH = qMin(720, screenGeometry.height() - 200);
    QImage defaultImg(initW, initH, QImage::Format_ARGB32);
    defaultImg.fill(Qt::white);
    stack.appendFirstLayer(defaultImg, tr("Fondo"));
    actualizarDimensionesFisicas();
    updateClassicToolStamp();
    updateCustomBrushStamp();
}

/// ========== Setters básicos ==========
void PaintArea::setActiveColorTarget(int target) {
    activeColorTarget = target;
    if (usaStampDePincel()) updateClassicToolStamp();
}
void PaintArea::setMouseSensitivity(double sens) {
    mouseSensitivity = sens;
    m_maskEdit.setMouseSensitivity(sens);
}
double PaintArea::getMouseSensitivity() const { return mouseSensitivity; }

/// ========== Bezier / pinceles custom ==========
void PaintArea::bakeActivePath() {
    if (bezierTool.isEmpty()) return;
    if (!capaValida()) { bezierTool.reset(); return; }
    QPainterPath path = bezierTool.hasCompletion()
                        ? bezierTool.takeCompletedPath()
                        : QPainterPath();
    if (path.isEmpty()) { bezierTool.reset(); return; }
    saveHistoryState();
    QPainter painter(&stack.currentImage());
    painter.setRenderHint(QPainter::Antialiasing, !pixelOptions.getIsPixelArtMode());
    QColor colorDeUso = obtenerColorDeTrabajo(Qt::LeftButton);
    colorDeUso.setAlpha(penOpacity);
    int scaledWidth = qMax(1, static_cast<int>(penWidth * mouseSensitivity));
    painter.setPen(QPen(colorDeUso, scaledWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(QBrush(colorDeUso));
    painter.drawPath(path);
    painter.end();
    bezierTool.reset();
    emit layersChanged();
    recomponerImagen();
    update();
}

void PaintArea::updateCustomBrushStamp() {
    const BrushSettings &config = customBrushPresets[activeCustomBrushIndex];
    QColor leftBase  = obtenerColorDeTrabajo(Qt::LeftButton);
    QColor rightBase = obtenerColorDeTrabajo(Qt::RightButton);
    QColor secondL = config.mixSecondColor ? rightBase : QColor();
    QColor secondR = config.mixSecondColor ? leftBase  : QColor();
    customBrushStamp = PaintEngine::generateBrushStamp(config, leftBase, penOpacity, pixelOptions.getIsPixelArtMode(), secondL);
    customBrushStampRight = PaintEngine::generateBrushStamp(config, rightBase, penOpacity, pixelOptions.getIsPixelArtMode(), secondR);
}

void PaintArea::setCustomBrushPresets(BrushSettings p1, BrushSettings p2, int activeIndex) {
    customBrushPresets[0] = p1; customBrushPresets[1] = p2;
    activeCustomBrushIndex = activeIndex;
    updateCustomBrushStamp();
}
BrushSettings PaintArea::getCustomBrush(int index) const { return customBrushPresets[index]; }
int PaintArea::getActiveCustomBrushIndex() const { return activeCustomBrushIndex; }

/// ========== Máscaras B/N ==========
bool PaintArea::hasLayerMask(int layerIndex) const { return stack.hasMask(layerIndex); }
bool PaintArea::isLayerMaskEnabled(int layerIndex) const { return stack.isMaskEnabled(layerIndex); }
int PaintArea::getMaskEditLayer() const { return stack.maskEditLayer(); }
bool PaintArea::isEditingMask() const { return stack.isEditingMask(); }
QImage PaintArea::getMaskPreview(int layerIndex) const { return stack.getMaskPreview(layerIndex); }

void PaintArea::addLayerMask(int layerIndex) {
    if (!stack.validIndex(layerIndex)) return;
    stack.addMask(layerIndex, stack.layerAt(layerIndex).image.size());
    emit statusBarMessage(tr("Máscara: NEGRO oculta · BLANCO revela · Goma revela · X intercambia"));
    refreshAndNotify();
}

void PaintArea::removeLayerMask(int layerIndex) {
    if (!stack.hasMask(layerIndex)) return;
    stack.removeMask(layerIndex);
    m_maskEdit.resetBezier();
    emit statusBarMessage(tr("Máscara eliminada"));
    refreshAndNotify();
}

void PaintArea::toggleLayerMaskEnabled(int layerIndex) {
    if (!stack.hasMask(layerIndex)) return;
    bool enabled = stack.toggleMaskEnabled(layerIndex);
    emit statusBarMessage(enabled ? tr("Máscara activada") : tr("Máscara desactivada"));
    refreshAndNotify();
}

void PaintArea::selectMaskForEditing(int layerIndex) {
    if (!stack.hasMask(layerIndex)) return;
    stack.setEditLayer(layerIndex);
    emit layersChanged();
    emit statusBarMessage(tr("Editando MÁSCARA: negro oculta / blanco revela"));
    update();
}

void PaintArea::selectLayerContentForEditing() {
    if (stack.maskEditLayer() < 0) return;
    stack.exitMaskEdit();
    m_maskEdit.resetBezier();
    emit layersChanged();
    update();
}

void PaintArea::applyMaskToLayer(int layerIndex) {
    if (!stack.hasMask(layerIndex)) return;
    if (!stack.validIndex(layerIndex)) return;
    saveHistoryState();
    stack.layerAt(layerIndex).image = stack.applyMaskAndRemove(layerIndex, stack.layerAt(layerIndex).image);
    emit statusBarMessage(tr("Máscara aplicada a la capa"));
    refreshAndNotify();
}

void PaintArea::invertLayerMask(int layerIndex) {
    if (!stack.hasMask(layerIndex)) return;
    stack.invertMask(layerIndex);
    emit statusBarMessage(tr("Máscara invertida"));
    refreshAndNotify();
}

/// ========== Máscaras de color ==========
bool PaintArea::hasLayerColorMask(int layerIndex) const { return stack.hasColorMask(layerIndex); }
bool PaintArea::isLayerColorMaskEnabled(int layerIndex) const { return stack.isColorMaskEnabled(layerIndex); }
FilterParams PaintArea::getLayerColorMaskParams(int layerIndex) const { return stack.colorMaskParams(layerIndex); }
QImage PaintArea::getColorMaskPreview(int layerIndex) const {
    if (!stack.validIndex(layerIndex)) return QImage();
    return stack.getColorMaskPreview(layerIndex, stack.layerAt(layerIndex).image);
}

void PaintArea::addLayerColorMask(int layerIndex, const FilterParams &fp) {
    if (!stack.validIndex(layerIndex)) return;
    saveHistoryState();
    stack.addColorMask(layerIndex, fp);
    emit statusBarMessage(tr("Máscara de COLOR añadida (puedes pintar encima, filtro en tiempo real)"));
    refreshAndNotify();
}

void PaintArea::removeLayerColorMask(int layerIndex) {
    if (!stack.hasColorMask(layerIndex)) return;
    saveHistoryState();
    stack.removeColorMask(layerIndex);
    emit statusBarMessage(tr("Máscara de color eliminada"));
    refreshAndNotify();
}

void PaintArea::toggleLayerColorMaskEnabled(int layerIndex) {
    if (!stack.hasColorMask(layerIndex)) return;
    bool enabled = stack.toggleColorMaskEnabled(layerIndex);
    emit statusBarMessage(enabled ? tr("Máscara de color ACTIVADA") : tr("Máscara de color DESACTIVADA"));
    refreshAndNotify();
}

void PaintArea::setLiveColorMaskPreview(int layerIndex, const FilterParams &fp) {
    if (!stack.validIndex(layerIndex)) return;
    stack.setLiveColorMaskPreview(layerIndex, fp);
    refreshAndNotify();
}

void PaintArea::clearLiveColorMaskPreview(int layerIndex) {
    if (!stack.hasLiveColorMaskPreview(layerIndex)) return;
    stack.clearLiveColorMaskPreview(layerIndex);
    refreshAndNotify();
}

/// ========== Capas ==========
void PaintArea::addLayer() {
    if (stack.isEmpty()) return;
    stack.addLayer(tr("Capa %1").arg(stack.count() + 1));
    refreshAndNotify(false);
}

void PaintArea::addImageLayer(const QImage& img) {
    beginEdit();
    if (stack.isEmpty()) return;
    stack.addImageLayer(img, tr("Capa Importada"));
    refreshAndNotify(false);
}

void PaintArea::insertImageAsObject(const QImage &img) {
    if (img.isNull()) return;
    bakeSelection(); bakeTextFrame(); bakeActivePath(); saveHistoryState();
    int canvasW = stack.width(), canvasH = stack.height();
    int imgW = img.width(), imgH = img.height();
    double maxScale = qMin((double)canvasW / imgW, (double)canvasH / imgH);
    if (maxScale < 1.0) {
        imgW = (int)(imgW * maxScale * 0.9);
        imgH = (int)(imgH * maxScale * 0.9);
    }
    int posX = (canvasW - imgW) / 2;
    int posY = (canvasH - imgH) / 2;
    QRect objRect(posX, posY, imgW, imgH);
    selMgr.registerSelectionImageObject(objRect, img, 0.0, stack.currentIndex());
    setTool(ToolMove);
    emit statusBarMessage(tr("Imagen insertada como objeto. Usa Mover para escalar/rotar."));
    refreshAndNotify();
}

void PaintArea::duplicateLayer() { if (!stack.duplicateCurrent()) return; refreshAndNotify(false); }
void PaintArea::deleteLayer()    { if (!stack.deleteCurrent())    return; refreshAndNotify(false); }
void PaintArea::mergeDown()      { if (!stack.mergeDown())        return; refreshAndNotify(false); }
void PaintArea::moveLayerUp()    { if (!stack.moveCurrentUp())    return; refreshAndNotify(false); }
void PaintArea::moveLayerDown()  { if (!stack.moveCurrentDown())  return; refreshAndNotify(false); }
void PaintArea::reorderLayer(int fromIndex, int toIndex) {
    if (!stack.reorder(fromIndex, toIndex)) return;
    refreshAndNotify(false);
}
void PaintArea::setCurrentLayer(int index) {
    if (!stack.validIndex(index)) return;
    stack.setCurrentIndex(index);
    refreshAndNotify(false);
}
void PaintArea::setLayerVisibility(int index, bool visible) {
    if (!stack.validIndex(index)) return;
    stack.setVisibility(index, visible);
    refreshAndNotify(false);
}
void PaintArea::setLayerOpacity(int index, double opacity) {
    if (!stack.validIndex(index)) return;
    stack.setLayerOpacity(index, opacity);
    refreshAndNotify(false);
}
void PaintArea::setLayerBlendMode(int index, int mode) {
    if (!stack.validIndex(index)) return;
    stack.setBlendMode(index, mode);
    refreshAndNotify(false);
}
void PaintArea::setLayerLocked(int index, bool locked) {
    if (!stack.validIndex(index)) return;
    stack.setLocked(index, locked);
    emit layersChanged();
}

const QList<Layer>& PaintArea::getLayers() const { return stack.layers(); }
int PaintArea::getCurrentLayerIndex() const { return stack.currentIndex(); }
QImage PaintArea::getImage() { return stack.compositedImage(); }

void PaintArea::applyImageFilters(const QImage &filteredImage) {
    if (!puedeEditarCapaActual()) {
        if (capaValida()) emit statusBarMessage(tr("Capa bloqueada"));
        return;
    }
    saveHistoryState();
    stack.currentImage() = filteredImage;
    refreshAndNotify();
}

void PaintArea::flipCurrentLayer(bool horizontal, bool vertical) {
    if (!puedeEditarCapaActual()) return;
    saveHistoryState();
    stack.flipCurrent(horizontal, vertical);
    refreshAndNotify(false);
}

void PaintArea::rotateCurrentLayer(int angle) {
    if (!puedeEditarCapaActual()) return;
    saveHistoryState();
    stack.rotateCurrent(angle);
    refreshAndNotify(false);
}

void PaintArea::magicWandSelect(const QPoint &pos, int tolerance) {
    if (!puedeEditarCapaActual()) {
        if (capaValida()) emit statusBarMessage(tr("Capa bloqueada"));
        return;
    }
    bakeSelection();
    QImage &layerImg = stack.currentImage();
    MagicWandResult result = MagicWandTools::applyMagicWand(layerImg, pos, tolerance, true, true);
    if (result.pixelsSelected == 0 || result.boundingBox.isEmpty()) {
        emit statusBarMessage(tr("Varita: sin píxeles similares"));
        return;
    }
    saveHistoryState();
    QImage extracted = MagicWandTools::extractMaskedRegion(layerImg, result.mask, result.boundingBox);
    MagicWandTools::clearMaskedRegion(layerImg, result.mask);
    selMgr.applyMagicWand(result.boundingBox, extracted);
    emit statusBarMessage(tr("Varita: %1 píxeles seleccionados — arrastra para mover").arg(result.pixelsSelected));
    refreshAndNotify();
}

/// ========== Pixel Art ==========
void PaintArea::setPixelArtMode(bool active, int resolution) {
    pixelOptions.setPixelArtMode(active, resolution);
    clearHistory();
    selMgr.clearObjects();
    stack.clearEverything();
    m_maskEdit.resetAll();
    bezierTool.reset();
    if (pixelOptions.getIsPixelArtMode()) {
        animManager.init(resolution);
        sincronizarCapasConFrameActual();
        emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
    } else {
        animManager.deinit();
        QRect screenGeometry = QApplication::primaryScreen()->geometry();
        int w = qMin(1280, screenGeometry.width() - 200);
        int h = qMin(720, screenGeometry.height() - 200);
        QImage nuevaImagen(w, h, QImage::Format_ARGB32);
        nuevaImagen.fill(Qt::white);
        stack.appendFirstLayer(nuevaImagen, tr("Fondo"));
    }
    actualizarDimensionesFisicas();
    emit layersChanged();
    emit resolutionChanged(stack.width(), stack.height());
    update();
}

void PaintArea::setGridSize(int size) { pixelOptions.setGridSize(size); update(); }
void PaintArea::setGridActive(bool active) { pixelOptions.setGridActive(active); update(); }
bool PaintArea::isGridActive() const { return pixelOptions.isGridActive(); }

void PaintArea::setPixelArtResolution(int resolution) {
    if (resolution != pixelOptions.getResolution() && pixelOptions.getIsPixelArtMode()) {
        pixelOptions.setPixelArtResolution(resolution);
        animManager.init(resolution);
        sincronizarCapasConFrameActual();
        emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
        actualizarDimensionesFisicas();
        emit layersChanged();
        emit resolutionChanged(stack.width(), stack.height());
        update();
    }
}

/// ========== Frames ==========
void PaintArea::addFrame() {
    if (!pixelOptions.getIsPixelArtMode()) return;
    saveHistoryState(); guardarFrameActualEnAnimador();
    animManager.addFrame(); sincronizarCapasConFrameActual();
    emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
    emit layersChanged(); actualizarDimensionesFisicas(); update();
}
void PaintArea::duplicateFrame() {
    if (!pixelOptions.getIsPixelArtMode() || animManager.getFrames().isEmpty()) return;
    saveHistoryState(); guardarFrameActualEnAnimador();
    animManager.duplicateFrame(); sincronizarCapasConFrameActual();
    emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
    emit layersChanged(); actualizarDimensionesFisicas(); update();
}
void PaintArea::deleteFrame() {
    if (!pixelOptions.getIsPixelArtMode() || animManager.getFrames().size() <= 1) return;
    saveHistoryState(); guardarFrameActualEnAnimador();
    animManager.deleteFrame(); sincronizarCapasConFrameActual();
    emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
    emit layersChanged(); actualizarDimensionesFisicas(); update();
}
void PaintArea::goToFrame(int index) {
    if (!pixelOptions.getIsPixelArtMode() || index < 0 || index >= animManager.getFrames().size()) return;
    if (index == animManager.getCurrentFrameIndex()) return;
    guardarFrameActualEnAnimador();
    animManager.goToFrame(index); sincronizarCapasConFrameActual();
    emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
    emit layersChanged(); actualizarDimensionesFisicas(); update();
}
void PaintArea::nextFrame() {
    if (pixelOptions.getIsPixelArtMode()) {
        int nextIdx = animManager.getCurrentFrameIndex() + 1;
        if (nextIdx < animManager.getFrames().size()) goToFrame(nextIdx);
    }
}
void PaintArea::prevFrame() {
    if (pixelOptions.getIsPixelArtMode()) {
        int prevIdx = animManager.getCurrentFrameIndex() - 1;
        if (prevIdx >= 0) goToFrame(prevIdx);
    }
}

/// ========== Historial ==========
void PaintArea::clearHistory() { undoStack.clear(); redoStack.clear(); }
void PaintArea::saveHistoryState() {
    if (capaValida()) {
        undoStack.append(stack.currentImage());
        if (undoStack.size() > MAX_HISTORY) undoStack.removeFirst();
    }
    redoStack.clear();
}
void PaintArea::undo() {
    if (undoStack.isEmpty()) return;
    bakeAllPending();
    redoStack.append(stack.currentImage());
    stack.currentImage() = undoStack.takeLast();
    recomponerImagen();
    actualizarDimensionesFisicas();
    emit layersChanged();
    emit resolutionChanged(stack.width(), stack.height());
    update();
}
void PaintArea::redo() {
    if (redoStack.isEmpty()) return;
    bakeAllPending();
    undoStack.append(stack.currentImage());
    stack.currentImage() = redoStack.takeLast();
    recomponerImagen();
    actualizarDimensionesFisicas();
    emit layersChanged();
    emit resolutionChanged(stack.width(), stack.height());
    update();
}

/// ========== Colores / pincel / zoom ==========
void PaintArea::setPenColor1(const QColor &c) { penColor1 = c; refreshBrushStamps(); }
void PaintArea::setPenColor2(const QColor &c) { penColor2 = c; refreshBrushStamps(); }
void PaintArea::refreshBrushStamps() {
    if (ArtisticPresets::isArtisticTool(currentTool)) updateClassicToolStamp();
    else if (currentTool == ToolCustomBrush) updateCustomBrushStamp();
}
QColor PaintArea::getPenColor1() const { return penColor1; }
QColor PaintArea::getPenColor2() const { return penColor2; }

void PaintArea::setPenWidth(int newWidth) {
    penWidth = newWidth;
    m_maskEdit.setPenWidth(newWidth);
    if (ArtisticPresets::isArtisticTool(currentTool)) {
        classicToolPreset.size = qMax(5, newWidth * 4);
        updateClassicToolStamp();
    } else if (currentTool == ToolCustomBrush) {
        customBrushPresets[activeCustomBrushIndex].size = qMax(5, newWidth * 4);
        updateCustomBrushStamp();
    }
}

void PaintArea::setPenOpacity(int opacity) {
    penOpacity = opacity;
    if (ArtisticPresets::isArtisticTool(currentTool)) updateClassicToolStamp();
    else if (currentTool == ToolCustomBrush) updateCustomBrushStamp();
}

double PaintArea::getZoomFactor() const { return zoomFactor; }
void PaintArea::setZoomFactor(double factor) {
    if (factor < 0.125) factor = 0.125;
    if (factor > 32.0) factor = 32.0;
    zoomFactor = factor;
    actualizarDimensionesFisicas();
    emit zoomChanged(zoomFactor);
    update();
}

void PaintArea::actualizarDimensionesFisicas() {
    setFixedSize((int)(stack.width() * zoomFactor) + HANDLE_SIZE,
                 (int)(stack.height() * zoomFactor) + HANDLE_SIZE);
}

QSize PaintArea::canvasSize() const { return stack.canvasSize(); }

void PaintArea::setDarkMode(bool enabled) { darkModeActive = enabled; update(); }
bool PaintArea::getDarkMode() const { return darkModeActive; }

/// ========== Gradiente ==========
void PaintArea::setGradientType(int t) { gradientType = (GradientType)qBound(0, t, 2); }
int PaintArea::getGradientType() const { return (int)gradientType; }
int PaintArea::getGradientOpacity() const { return gradientOpacity; }
void PaintArea::setGradientOpacity(int o) { gradientOpacity = qBound(0, o, 255); }
int PaintArea::getGradientAngle() const { return gradientAngle; }
void PaintArea::setGradientAngle(int a) { gradientAngle = qBound(0, a, 360); }
bool PaintArea::getGradientReverse() const { return gradientReverse; }
void PaintArea::setGradientReverse(bool r) { gradientReverse = r; }
bool PaintArea::getGradientDither() const { return gradientDither; }
void PaintArea::setGradientDither(bool d) { gradientDither = d; }
int PaintArea::getGradientBlendMode() const { return gradientBlendMode; }
void PaintArea::setGradientBlendMode(int m) { gradientBlendMode = m; }
bool PaintArea::getGradientUseSecondColor() const { return gradientUseSecondColor; }
void PaintArea::setGradientUseSecondColor(bool v) { gradientUseSecondColor = v; }

/// ========== Clonar ==========
bool PaintArea::isCloneSourceSet() const { return cloneSourceSet; }
void PaintArea::resetCloneSource() {
    cloneSourceSet = false;
    cloneBuffer = QImage();
    cloneIsStamping = false;
}

/// ========== setTool ==========
void PaintArea::setTool(ToolType tool) {
    if (tool != ToolPenBezier) {
        bakeActivePath();
        m_maskEdit.resetBezier();
    }
    if (tool != ToolSelect && tool != ToolSelectFree && tool != ToolMagicWand &&
        tool != ToolLassoExtract && tool != ToolLassoDelete) bakeSelection();
    if (tool != ToolSelectFree && tool != ToolLassoExtract && tool != ToolLassoDelete) cancelVectorMode();
    if (tool != ToolText) bakeTextFrame();
    if (tool != ToolGradient) { drawingGradient = false; }
    if (tool != ToolMove) { movingLayer = false; moveLayerOffset = QPoint(0,0); moveLayerIdx = -1; }
    if (tool != ToolClone) { cloneIsStamping = false; }
    if (tool != ToolDeform) { m_deform.reset(); }
    if (tool != ToolMove) { selMgr.deselectAllObjects(); }
    if (stack.isEditingMask() && !herramientaDePintura() && tool != ToolPenBezier) {
        stack.exitMaskEdit();
    }
    if (tool == ToolGradient && currentTool == ToolGradient) { openGradientSettings(); return; }
    currentTool = tool;
    m_maskEdit.setCurrentTool(tool);
    if (tool == ToolGradient) { openGradientSettings(); }
    if (ArtisticPresets::isArtisticTool(tool)) {
        classicToolPreset = ArtisticPresets::presetForTool(tool);
        classicToolPreset.size = qMax(5, penWidth * 4);
        updateClassicToolStamp();
    } else if (tool == ToolCustomBrush) { updateCustomBrushStamp(); }
    update();
}

/// ========== Limpiar / nuevo lienzo ==========
void PaintArea::clearImage() {
    saveHistoryState();
    bakeAllPending();
    QSize prevSize = stack.canvasSize();
    stack.clearEverything();
    m_maskEdit.resetAll();
    bezierTool.reset();
    if (pixelOptions.getIsPixelArtMode()) {
        animManager.init(pixelOptions.getResolution());
        sincronizarCapasConFrameActual();
        emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
    } else {
        QImage nuevaImagen(prevSize.width(), prevSize.height(), QImage::Format_ARGB32);
        nuevaImagen.fill(Qt::white);
        stack.appendFirstLayer(nuevaImagen, tr("Fondo"));
    }
    emit layersChanged();
    emit resolutionChanged(stack.width(), stack.height());
    update();
}

void PaintArea::crearNuevoLienzo(int w, int h, bool transparent) {
    if (w < 50) w = 50; if (h < 50) h = 50;
    saveHistoryState();
    bakeAllPending();
    stack.clearEverything();
    clearHistory();
    m_maskEdit.resetAll();
    bezierTool.reset();
    if (pixelOptions.getIsPixelArtMode()) {
        setPixelArtResolution(qMax(8, qMin(512, qMax(w, h))));
    } else {
        QImage nuevaImagen(w, h, QImage::Format_ARGB32);
        nuevaImagen.fill(transparent ? Qt::transparent : Qt::white);
        stack.appendFirstLayer(nuevaImagen, tr("Fondo"));
        actualizarDimensionesFisicas();
        emit layersChanged();
        emit resolutionChanged(stack.width(), stack.height());
        update();
    }
}

/// ========== Abrir / guardar ==========
bool PaintArea::abrirImagen(const QString &fileName) {
    bakeAllPending();
    QImage nuevaImagen = cargarImagenRespetandoExif(fileName);
    if (nuevaImagen.isNull()) return false;
    saveHistoryState();
    stack.clearEverything();
    m_maskEdit.resetAll();
    bezierTool.reset();
    if (pixelOptions.getIsPixelArtMode()) {
        QImage imagenEscalada = nuevaImagen.scaled(pixelOptions.getResolution(), pixelOptions.getResolution(),
                                                   Qt::KeepAspectRatio, Qt::SmoothTransformation).convertToFormat(QImage::Format_ARGB32);
        QImage imgLimpia(pixelOptions.getResolution(), pixelOptions.getResolution(), QImage::Format_ARGB32);
        imgLimpia.fill(Qt::transparent);
        QPainter p(&imgLimpia); p.drawImage(0, 0, imagenEscalada); p.end();
        animManager.init(pixelOptions.getResolution());
        animManager.setCurrentFrameImage(imgLimpia);
        sincronizarCapasConFrameActual();
        emit framesChanged(animManager.getFrames(), animManager.getCurrentFrameIndex());
    } else {
        stack.appendFirstLayer(nuevaImagen.convertToFormat(QImage::Format_ARGB32), tr("Fondo"));
    }
    actualizarDimensionesFisicas();
    emit layersChanged();
    emit resolutionChanged(stack.width(), stack.height());
    update();
    return true;
}

bool PaintArea::guardarImagen(const QString &fileName, const char *fileFormat) {
    bakeAllPending();
    if (pixelOptions.getIsPixelArtMode()) guardarFrameActualEnAnimador();
    QImage finalImg = stack.compositedImage();
    return finalImg.save(fileName, fileFormat);
}

bool PaintArea::guardarComoSvg(const QString &fileName) {
    bakeAllPending();
    if (pixelOptions.getIsPixelArtMode()) guardarFrameActualEnAnimador();
    QImage image = stack.compositedImage();
    if (image.isNull()) return false;
    QSvgGenerator generator;
    generator.setFileName(fileName);
    generator.setSize(image.size());
    generator.setViewBox(QRect(0, 0, image.width(), image.height()));
    generator.setTitle(tr("Dibujo Paint-UX"));
    generator.setDescription(tr("Exportado desde Paint-UX Studio"));
    QPainter painter(&generator);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.drawImage(0, 0, image);
    painter.end();
    return true;
}

bool PaintArea::guardarComoGif(const QString &fileName, int delayMs, int scale) {
    if (!pixelOptions.getIsPixelArtMode()) return false;
    guardarFrameActualEnAnimador();
    const QList<QImage> &frames = animManager.getFrames();
    if (frames.isEmpty()) return false;
    GifEncoder encoder;
    return encoder.save(fileName, frames, delayMs, true, scale);
}

/// ========== Selecciones / Texto ==========
void PaintArea::bakeSelection() {
    if (selMgr.isActive() && selMgr.hasBuffer()) {
        if (puedeEditarCapaActual()) {
            selMgr.bake(stack.currentImage(), !pixelOptions.getIsPixelArtMode());
            recomponerImagen();
        }
    }
    selMgr.discard();
    cancelVectorMode();
    emit layersChanged(); update();
}

void PaintArea::bakeTextFrame() {
    if (textEdit.active && !textEdit.isEmpty()) {
        lastUsedTextFont = textEdit.font;
        selMgr.registerTextObject(textEdit.rect, textEdit.text, textEdit.font, textEdit.color, stack.currentIndex());
    }
    textEdit.end();
    update();
}

void PaintArea::cancelTextFrame() {
    textEdit.end();
    update();
}

void PaintArea::updateTextFrame(const QRect &rect, const QFont &font, const QColor &color) {
    if (!textEdit.active) {
        textEdit.beginNew(rect, font, color);
    } else {
        textEdit.rect = rect;
        textEdit.applyFormat(font, color);
    }
    lastUsedTextFont = font;
    update();
}

bool PaintArea::isTextFrameActive() const { return textEdit.active; }
QRect PaintArea::getTextFrameRect() const { return textEdit.rect; }
QString PaintArea::getTextFrameContent() const { return textEdit.text; }
QFont PaintArea::getTextFrameFont() const { return textEdit.font; }
QColor PaintArea::getTextFrameColor() const { return textEdit.color; }
void PaintArea::insertTextChar(const QString &ch) { textEdit.insert(ch); }
void PaintArea::deleteTextChar() { textEdit.backspace(); }

/// ========== Portapapeles ==========
void PaintArea::copiarSeleccion() {
    QList<int> selected = selMgr.selectedObjectIndices();
    if (!selected.isEmpty()) {
        QImage rendered = selMgr.renderSelectedObjects();
        if (!rendered.isNull()) {
            selMgr.setClipboardBuffer(rendered);
            QApplication::clipboard()->setImage(rendered);
            emit statusBarMessage(tr("Objetos copiados (%1)").arg(selected.size()));
            return;
        }
    }
    if (selMgr.isActive() && selMgr.hasBuffer()) {
        selMgr.setClipboardBuffer(selMgr.buffer());
        QApplication::clipboard()->setImage(selMgr.buffer());
        emit statusBarMessage(tr("Copiado al portapapeles"));
    } else {
        emit statusBarMessage(tr("No hay selección ni objetos"));
    }
}

void PaintArea::cortarSeleccion() {
    QList<int> selected = selMgr.selectedObjectIndices();
    if (!selected.isEmpty()) {
        QImage rendered = selMgr.renderSelectedObjects();
        if (!rendered.isNull()) {
            selMgr.setClipboardBuffer(rendered);
            QApplication::clipboard()->setImage(rendered);
            saveHistoryState();
            selMgr.deleteSelectedObjects();
            emit statusBarMessage(tr("Objetos cortados (%1)").arg(selected.size()));
            return;
        }
    }
    if (selMgr.isActive() && selMgr.hasBuffer()) {
        selMgr.setClipboardBuffer(selMgr.buffer());
        QApplication::clipboard()->setImage(selMgr.buffer());
        selMgr.discard();
        emit statusBarMessage(tr("Cortado al portapapeles"));
        refreshAndNotify();
    } else {
        emit statusBarMessage(tr("No hay selección ni objetos"));
    }
}

void PaintArea::pegarClipboard() {
    bakeSelection(); bakeActivePath(); bakeTextFrame();
    QImage imgToPaste = selMgr.getImageToPaste();
    if (!imgToPaste.isNull()) {
        saveHistoryState();
        selMgr.pasteAsSelection(imgToPaste);
        setTool(ToolSelect);
        update();
        emit statusBarMessage(tr("Pegado desde portapapeles"));
    } else {
        emit statusBarMessage(tr("Portapapeles vacío"));
    }
}

void PaintArea::borrarSeleccion() {
    QList<int> selected = selMgr.selectedObjectIndices();
    if (!selected.isEmpty()) {
        saveHistoryState();
        selMgr.deleteSelectedObjects();
        emit statusBarMessage(tr("Objetos eliminados"));
        return;
    }
    if (selMgr.isActive() && selMgr.hasBuffer()) {
        selMgr.discard();
        emit statusBarMessage(tr("Selección borrada"));
        refreshAndNotify();
    } else {
        emit statusBarMessage(tr("No hay selección ni objetos"));
    }
}

/// ========== Drag & Drop ==========
void PaintArea::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls() || event->mimeData()->hasImage()) event->acceptProposedAction();
}

void PaintArea::dropEvent(QDropEvent *event) {
    QImage droppedImage;
    if (event->mimeData()->hasUrls()) {
        QString filePath = event->mimeData()->urls().first().toLocalFile();
        droppedImage = cargarImagenRespetandoExif(filePath);
    } else if (event->mimeData()->hasImage()) {
        droppedImage = qvariant_cast<QImage>(event->mimeData()->imageData());
    }
    if (!droppedImage.isNull()) {
        QPoint canvasPos((int)(event->position().x() / zoomFactor),
                         (int)(event->position().y() / zoomFactor));
        int hitObj = selMgr.findObjectAt(QPointF(canvasPos));
        if (hitObj >= 0 && selMgr.objectAt(hitObj).type == ObjectType::Shape) {
            bakeSelection(); bakeActivePath(); bakeTextFrame();
            saveHistoryState();
            PaintObject &obj = selMgr.objectAt(hitObj);
            obj.isFrame = true;
            obj.hollow = false;
            obj.frameImage = droppedImage;
            obj.frameImageScale = 1.0;
            obj.frameImageOffset = QPointF(0, 0);
            selMgr.selectObject(hitObj);
            emit statusBarMessage(tr("Imagen colocada dentro de la figura"));
            refreshAndNotify();
            return;
        }
        bakeSelection(); bakeActivePath(); bakeTextFrame();
        saveHistoryState();
        selMgr.pasteAsSelection(droppedImage, QPoint(50, 50));
        setTool(ToolSelect);
        emit statusBarMessage(tr("Imagen insertada"));
        update();
    }
}

/// ========== Lienzo ==========
void PaintArea::cambiarDimensionesLienzo(int nuevoW, int nuevoH) {
    if (pixelOptions.getIsPixelArtMode()) { setPixelArtResolution(qMax(8, qMin(64, nuevoW))); return; }
    if (nuevoW < 50) nuevoW = 50; if (nuevoH < 50) nuevoH = 50;
    bakeAllPending();
    saveHistoryState();
    stack.resizeCanvas(nuevoW, nuevoH);
    actualizarDimensionesFisicas();
    emit layersChanged();
    emit resolutionChanged(stack.width(), stack.height());
    update();
}

/// ========== Getters frames ==========
const QList<QImage>& PaintArea::getFrames() const { return animManager.getFrames(); }
int PaintArea::getCurrentFrameIndex() const { return animManager.getCurrentFrameIndex(); }
bool PaintArea::getIsPixelArtMode() const { return pixelOptions.getIsPixelArtMode(); }
int PaintArea::getPixelGridSize() const { return pixelOptions.getGridSize(); }
int PaintArea::getPixelResolution() const { return pixelOptions.getResolution(); }
bool PaintArea::getVectorEditMode() const { return vectorEditMode; }
void PaintArea::editTextObject(int idx) { loadTextObjectForEditing(idx); }

/// ============================================================
/// keyPressEvent — dispatcher delgado (Extract Method)
/// ============================================================
void PaintArea::keyPressEvent(QKeyEvent *event) {
    // Orden de prioridad: primero lo más específico, al final lo genérico.
    if (handleMaskBezierKeys(event))      return;
    if (handleClipboardShortcuts(event))  return;
    if (handleDeleteKey(event))           return;
    if (handleObjectShortcuts(event))     return;
    if (handleVectorModeKeys(event))      return;
    if (handleTextEditingKeys(event))     return;
    if (handleToolSpecificKeys(event))    return;
    if (handleBezierPenKeys(event))       return;

    // Fallback: Enter/Return global hornea pendientes.
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        bakeActivePath();
        bakeSelection();
        return;
    }
    QWidget::keyPressEvent(event);
}

/// ------------------------------------------------------------
/// 1) Enter/Esc/Delete en Bezier de máscara (prioridad alta)
/// ------------------------------------------------------------
bool PaintArea::handleMaskBezierKeys(QKeyEvent *event) {
    if (!stack.isEditingMask() || currentTool != ToolPenBezier) return false;
    if (!m_maskEdit.hasBezierNodes() || textEdit.active)         return false;

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        m_maskEdit.rasterizeBezier(zoomFactor);
        return true;
    }
    if (event->key() == Qt::Key_Escape) {
        m_maskEdit.cancelBezier();
        update();
        return true;
    }
    if ((event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
        && m_maskEdit.selectedNode() >= 0) {
        m_maskEdit.removeSelectedNode();
        update();
        return true;
    }
    return false;
}

/// ------------------------------------------------------------
/// 2) Ctrl+C / Ctrl+X / Ctrl+V (portapapeles)
/// ------------------------------------------------------------
bool PaintArea::handleClipboardShortcuts(QKeyEvent *event) {
    if (!(event->modifiers() & Qt::ControlModifier)) return false;
    if (textEdit.active) return false;

    if (event->key() == Qt::Key_C) { copiarSeleccion(); return true; }
    if (event->key() == Qt::Key_X) { cortarSeleccion(); return true; }
    if (event->key() == Qt::Key_V) { pegarClipboard();  return true; }
    return false;
}

/// ------------------------------------------------------------
/// 3) Delete / Backspace (borrar selección)
/// ------------------------------------------------------------
bool PaintArea::handleDeleteKey(QKeyEvent *event) {
    if (textEdit.active) return false;
    if (event->key() != Qt::Key_Delete && event->key() != Qt::Key_Backspace) return false;
    borrarSeleccion();
    return true;
}

/// ------------------------------------------------------------
/// 4) O / I (convertir selección en objeto / integrar objetos)
/// ------------------------------------------------------------
bool PaintArea::handleObjectShortcuts(QKeyEvent *event) {
    if (textEdit.active) return false;

    if (event->key() == Qt::Key_O) { convertSelectionToObject();  return true; }
    if (event->key() == Qt::Key_I) { integrateSelectedObjects();  return true; }
    return false;
}

/// ------------------------------------------------------------
/// 5) V (toggle modo vector) + Enter/Esc/Delete en modo vector
/// ------------------------------------------------------------
bool PaintArea::handleVectorModeKeys(QKeyEvent *event) {
    if (textEdit.active) return false;

    // Toggle con V
    if (event->key() == Qt::Key_V && herramientaVectorizable() && !vectorEditMode) {
        if (selMgr.isActive()) { emit statusBarMessage(tr("Deselecciona primero")); return true; }
        vectorEditMode = true;
        vectorPoints.clear();
        selectedVectorPoint = -1;
        selMgr.setPath(QPainterPath());
        drawing = false;
        emit statusBarMessage(tr("Modo vector"));
        emit vectorModeChanged(true);
        update();
        return true;
    }

    // Si ya estamos en modo vector y usamos una tool vectorizable, no consumimos nada.
    // El toggle "apagado" se hace desde la propia tool.
    if (!vectorEditMode || !herramientaVectorizable()) return false;

    if (event->key() == Qt::Key_V) {
        // Toggle off
        vectorEditMode = false;
        vectorPoints.clear();
        selectedVectorPoint = -1;
        emit statusBarMessage(tr("Modo libre"));
        emit vectorModeChanged(false);
        update();
        return true;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        finalizeVectorPath();
        return true;
    }
    if (event->key() == Qt::Key_Escape) {
        cancelVectorMode();
        emit statusBarMessage(tr("Cancelado"));
        return true;
    }
    if ((event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
        && selectedVectorPoint >= 0) {
        vectorPoints.removeAt(selectedVectorPoint);
        selectedVectorPoint = -1;
        update();
        return true;
    }
    return false;
}

/// ------------------------------------------------------------
/// 6) Teclas dentro del frame de texto activo
/// ------------------------------------------------------------
bool PaintArea::handleTextEditingKeys(QKeyEvent *event) {
    if (!textEdit.active) return false;

    switch (event->key()) {
        case Qt::Key_Escape:
            cancelTextFrame();
            emit textFrameCancelled();
            return true;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            textEdit.insertNewline();
            return true;
        case Qt::Key_Backspace:
            textEdit.backspace();
            return true;
        case Qt::Key_Delete:
            textEdit.deleteChar();
            return true;
        case Qt::Key_Left:
            textEdit.moveLeft();
            return true;
        case Qt::Key_Right:
            textEdit.moveRight();
            return true;
        case Qt::Key_Home:
            textEdit.moveHome();
            return true;
        case Qt::Key_End:
            textEdit.moveEnd();
            return true;
        default:
            break;
    }

    const QString txt = event->text();
    if (!txt.isEmpty() && txt.at(0).isPrint()) {
        textEdit.insert(txt);
        return true;
    }
    return false;
}

/// ------------------------------------------------------------
/// 7) Teclas específicas de la tool activa (Esc en Move, Ctrl+G en Gradient)
/// ------------------------------------------------------------
bool PaintArea::handleToolSpecificKeys(QKeyEvent *event) {
    if (currentTool == ToolMove && event->key() == Qt::Key_Escape) {
        selMgr.deselectAllObjects();
        movingLayer = false;
        update();
        return true;
    }
    if (currentTool == ToolGradient
        && event->key() == Qt::Key_G
        && (event->modifiers() & Qt::ControlModifier)) {
        openGradientSettings();
        return true;
    }
    return false;
}

/// ------------------------------------------------------------
/// 8) Enter/Esc en BezierPathTool (pluma Bézier normal)
/// ------------------------------------------------------------
bool PaintArea::handleBezierPenKeys(QKeyEvent *event) {
    if (currentTool != ToolPenBezier || bezierTool.isEmpty()) return false;

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (bezierTool.finalize()) bakeActivePath();
        return true;
    }
    if (event->key() == Qt::Key_Escape) {
        bezierTool.cancel();
        update();
        return true;
    }
    return false;
}

/// ========== mousePressEvent ==========
void PaintArea::mousePressEvent(QMouseEvent *event) {
    setFocus();
    QPoint rawPos = event->position().toPoint();
    QPoint pos(rawPos.x() / zoomFactor, rawPos.y() / zoomFactor);
    int scaledWidth = qMax(1, static_cast<int>(penWidth * mouseSensitivity));
    if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
        activeMouseButton = event->button();
        m_maskEdit.setActiveMouseButton(activeMouseButton);

        // Máscara + Bezier
        if (stack.isEditingMask() && currentTool == ToolPenBezier) {
            bool consumed = m_maskEdit.beginBezierClick(pos, zoomFactor, activeMouseButton == Qt::RightButton);
            if (consumed) { update(); return; }
        }
        // Máscara + Pintura
        if (stack.isEditingMask() && herramientaDePintura()) {
            if (m_maskEdit.beginStroke(pos)) {
                lastPoint = pos;
                drawing = true;
                emit layersChanged();
                return;
            }
        }
        // Modo vector
        if (vectorEditMode && herramientaVectorizable() && event->button() == Qt::LeftButton) {
            QPointF canvasPos(pos.x(), pos.y());
            if (isNearFirstPoint(canvasPos)) { finalizeVectorPath(); return; }
            int hitIdx = findVectorPointAt(canvasPos);
            if (hitIdx >= 0) { selectedVectorPoint = hitIdx; draggingVectorPoint = true; update(); return; }
            vectorPoints.append(canvasPos);
            selectedVectorPoint = vectorPoints.size() - 1;
            update(); return;
        }
        // Texto activo
        if (textEdit.active && activeMouseButton == Qt::LeftButton) {
            TextEngine::Handle h = textEdit.hitHandleAt(pos);
            if (h != TextEngine::Handle::None && h != TextEngine::Handle::Body) {
                textEdit.startResize(h);
                return;
            }
            if (h == TextEngine::Handle::Body) {
                textEdit.startDrag(pos);
                return;
            }
            bakeTextFrame();
        }
        // Handles de redimensión del lienzo
        if (activeMouseButton == Qt::LeftButton && !selMgr.isActive() && !textEdit.active) {
            if (getBottomRightHandle().contains(rawPos)) { resizingCanvas = true; resizeMode = 3; previewCanvasSize = QPoint(stack.width(), stack.height()); return; }
            if (getRightHandle().contains(rawPos)) { resizingCanvas = true; resizeMode = 1; previewCanvasSize = QPoint(stack.width(), stack.height()); return; }
            if (getBottomHandle().contains(rawPos)) { resizingCanvas = true; resizeMode = 2; previewCanvasSize = QPoint(stack.width(), stack.height()); return; }
        }
        // Gizmo de selección activa
        if (selMgr.isActive() && activeMouseButton == Qt::LeftButton && !vectorEditMode) {
            QPointF cp(pos.x(), pos.y());
            double threshold = 12.0 / zoomFactor;
            QPointF convPos = selMgr.convertHandlePos(zoomFactor);
            if (QLineF(cp, convPos).length() <= threshold) { convertSelectionToObject(); return; }
            ObjectHandle hSel = selMgr.hitTestGizmoAt(cp, zoomFactor);
            if (hSel == ObjectHandle::Rotate) {
                selMgr.setRotating(true);
                setCursor(Qt::CrossCursor);
                return;
            }
            if (isScaleHandle(hSel)) {
                selMgr.setResizing(true);
                selMgr.setResizeHandle(hSel);
                selMgr.setResizeStart(QRectF(selMgr.rect()), cp);
                setCursor(cursorForHandle(hSel));
                return;
            }
            if (hSel == ObjectHandle::Move) {
                selMgr.setDragging(true);
                selMgr.setDragOffset(pos - selMgr.rect().topLeft());
                setCursor(Qt::ClosedHandCursor);
                return;
            }
            bakeSelection();
        }
        // Zoom
        if (currentTool == ToolZoom) {
            QPoint viewportPos = mapToParent(rawPos);
            if (event->button() == Qt::LeftButton) emit zoomRequested(zoomFactor * 2.0, viewportPos);
            else emit zoomRequested(zoomFactor / 2.0, viewportPos);
            return;
        }
        // Dentro del canvas
        if (stack.canvasRect().contains(pos)) {
            if (capaValida() && stack.currentLocked()) {
                if (currentTool != ToolPicker && currentTool != ToolZoom) { emit statusBarMessage(tr("Capa bloqueada")); return; }
            }
            if (currentTool == ToolPenBezier) {
                bool completed = bezierTool.click(pos, zoomFactor, activeMouseButton == Qt::RightButton);
                if (completed) bakeActivePath();
                else update();
                return;
            }
            QColor colorDeUso = obtenerColorDeTrabajo(activeMouseButton);
            if (currentTool != ToolEraser) colorDeUso.setAlpha(penOpacity);
            if (currentTool == ToolSelect) {
                if (vectorEditMode) return;
                startPoint = pos;
                selMgr.startRect(pos);
                drawing = true;
            } else if (currentTool == ToolSelectFree ||
                       currentTool == ToolLassoExtract ||
                       currentTool == ToolLassoDelete) {
                if (vectorEditMode) return;
                startPoint = pos;
                selMgr.startFree(pos);
                drawing = true;
            } else if (currentTool == ToolMagicWand) {
                magicWandSelect(pos, static_cast<int>(32 * mouseSensitivity));
            } else if (currentTool == ToolBucket) {
                saveHistoryState();
                if (capaValida()) {
                    PaintEngine::floodFill(stack.currentImage(), pos, colorDeUso);
                    refreshAndNotify();
                }
            } else if (currentTool == ToolPicker) {
                QColor picked = stack.compositedImage().pixelColor(pos);
                emit colorPicked((activeMouseButton == Qt::LeftButton) ? 1 : 2, picked);
            } else if (currentTool == ToolText) {
                if (!textEdit.active) {
                    int defaultW = qMax(100, stack.width() / 4);
                    int defaultH = qMax(60, stack.height() / 8);
                    QRect r(pos.x(), pos.y(), defaultW, defaultH);
                    textEdit.beginNew(r, lastUsedTextFont, colorDeUso);
                    emit textFrameClicked(pos);
                    update();
                }
            } else if (currentTool == ToolGradient) {
                if (event->modifiers() & Qt::AltModifier) {
                    gradientType = (GradientType)(((int)gradientType + 1) % 3);
                    emit statusBarMessage(tr("Gradiente: %1").arg(
                        gradientType == GradientLinear ? tr("Lineal") :
                        gradientType == GradientRadial ? tr("Radial") : tr("Cónico")));
                    return;
                }
                if (event->modifiers() & Qt::ControlModifier) { openGradientSettings(); return; }
                gradientStart = pos; gradientEnd = pos; drawingGradient = true;
            } else if (currentTool == ToolClone) {
                if (event->modifiers() & Qt::AltModifier || !cloneSourceSet) {
                    cloneSource = pos; cloneSourceSet = true;
                    if (capaValida()) cloneBuffer = stack.currentImage().copy();
                    cloneInitialDest = pos;
                    emit statusBarMessage(tr("Fuente fijada"));
                    update(); return;
                }
                if (!cloneSourceSet) { emit statusBarMessage(tr("Alt+clic para fijar fuente")); return; }
                saveHistoryState();
                cloneInitialDest = pos; cloneIsStamping = true;
                if (capaValida()) { applyClonStamp(stack.currentImage(), pos); invalidarTrazo(pos, pos); emit layersChanged(); }
            } else if (currentTool == ToolMove) {
                QPointF canvasPos(pos.x(), pos.y());
                bool shiftHeld = (event->modifiers() & Qt::ShiftModifier);
                int activeIdx = selMgr.activeObjectIndex();
                if (!shiftHeld && activeIdx >= 0 && activeIdx < selMgr.objectCount() && selMgr.objectAt(activeIdx).selected) {
                    ObjectHandle h = selMgr.findObjectGizmoHandleAt(activeIdx, canvasPos, zoomFactor);
                    if (h == ObjectHandle::IntegrateToCanvas) { integrateSelectedObjects(); return; }
                    if (h == ObjectHandle::EditText) { loadTextObjectForEditing(activeIdx); return; }
                    if (h == ObjectHandle::EditShape) { loadShapeObjectForEditing(activeIdx); return; }
                    if (h != ObjectHandle::None) {
                        selMgr.setObjectActiveHandle(h);
                        selMgr.setObjectDragStart(canvasPos);
                        selMgr.setObjectRotationStart(selMgr.objectAt(activeIdx).rotation);
                        selMgr.setObjectScaleStart(selMgr.objectAt(activeIdx).scaleX, selMgr.objectAt(activeIdx).scaleY);
                        selMgr.setObjectBoundsStart(selMgr.objectAt(activeIdx).bounds);
                        if (h == ObjectHandle::Move) selMgr.setObjectDragging(true);
                        else if (h == ObjectHandle::Rotate) selMgr.setObjectRotating(true);
                        else selMgr.setObjectScaling(true);
                        setCursor(cursorForHandle(h));
                        return;
                    }
                }
                int hitObj = selMgr.findObjectAt(canvasPos);
                if (hitObj >= 0) {
                    if (shiftHeld) { selMgr.selectObject(hitObj, true); }
                    else {
                        if (!selMgr.objectAt(hitObj).selected) selMgr.selectObject(hitObj, false);
                        selMgr.setObjectActiveHandle(ObjectHandle::Move);
                        selMgr.setActiveObjectIndex(hitObj);
                        selMgr.setObjectDragStart(canvasPos);
                        selMgr.setObjectBoundsStart(selMgr.objectAt(hitObj).bounds);
                        selMgr.setObjectDragging(true);
                        setCursor(Qt::ClosedHandCursor);
                    }
                    return;
                }
                if (!selMgr.selectedObjectIndices().isEmpty()) { selMgr.deselectAllObjects(); return; }
                if (puedeEditarCapaActual()) {
                    saveHistoryState();
                    movingLayer = true;
                    moveStartPos = pos;
                    moveLayerIdx = stack.currentIndex();
                    moveLayerBackup = stack.currentImage().copy();
                }
            } else if (currentTool == ToolDeform) {
                if (!puedeEditarCapaActual()) return;
                saveHistoryState();
                m_deform.begin(pos, &stack.currentImage(),
                               activeMouseButton == Qt::RightButton);
            } else {
                saveHistoryState(); startPoint = pos; lastPoint = startPoint;
                drawing = true; currentMousePos = pos;
                strokeTotalLength = 0.0;
                strokeAccumulatedLength = 0.0;
                strokeInProgress = true;
                QColor colorOpuesto = obtenerColorDeTrabajo(activeMouseButton == Qt::LeftButton ? Qt::RightButton : Qt::LeftButton);
                strokeCanvasFallback = QColor();
                bool necesitaMezcla = false;
                if (usaStampDePincel()) necesitaMezcla = activePreset().wetMix;
                if (necesitaMezcla) {
                    strokeCanvasFallback = PaintEngine::sampleCanvasColor(stack.compositedImage(), pos, qMax(3, scaledWidth));
                }
                if (usaStampDePincel()) {
                    const BrushSettings &preset = activePreset();
                    lastClassicPoint = pos;
                    lastCustomPoint = pos;
                    if (preset.isAirbrush || preset.dragMode == DragMode::Scattered)
                        continuousDrawTimer->start(16);
                    if (capaValida()) {
                        PaintEngine::applyCustomBrushStroke(stack.currentImage(), pos, activeStamp(),
                            preset, mouseSensitivity, 0.0, 1.0, 1.0,
                            colorDeUso, colorOpuesto, strokeCanvasFallback);
                        invalidarTrazo(pos, pos); emit layersChanged();
                    }
                } else if ((currentTool == ToolPencil || currentTool == ToolEraser || currentTool == ToolMirrorPen || currentTool == ToolLighten) && pixelOptions.getIsPixelArtMode()) {
                    if (capaValida()) {
                        drawPixelArtPixel(stack.currentImage(), pos, colorDeUso, currentTool, true);
                        invalidarTrazo(pos, pos); emit layersChanged();
                    }
                } else if (currentTool == ToolPencil) {
                    if (capaValida()) {
                        PaintEngine::applyGraphitePencil(stack.currentImage(), pos, pos, colorDeUso, scaledWidth, penOpacity);
                        invalidarTrazo(pos, pos); emit layersChanged();
                    }
                } else if (currentTool == ToolEraser) {
                    if (capaValida()) {
                        PaintEngine::applyEraserLine(stack.currentImage(), pos, pos, scaledWidth, true);
                        invalidarTrazo(pos, pos); emit layersChanged();
                    }
                } else if (currentTool == ToolBlur || currentTool == ToolHeal || currentTool == ToolShadowBurn) {
                    RetouchTools::applyRetouchAlongLine(stack.currentImage(), pos, pos,
                                                        penWidth, mouseSensitivity, penOpacity,
                                                        retouchToolCode(currentTool));
                    invalidarTrazo(pos, pos); emit layersChanged();
                }
            }
        }
    }
}

/// ========== mouseMoveEvent ==========
void PaintArea::mouseMoveEvent(QMouseEvent *event) {
    QPoint rawPos = event->position().toPoint();
    QPoint pos(rawPos.x() / zoomFactor, rawPos.y() / zoomFactor);
    hoverPos = rawPos;
    previousMousePos = currentMousePos;
    currentMousePos = pos;
    int scaledWidth = qMax(1, static_cast<int>(penWidth * mouseSensitivity));

    if (stack.isEditingMask() && currentTool == ToolPenBezier && m_maskEdit.isDraggingNode()) {
        if (m_maskEdit.moveBezierNode(pos)) { update(); return; }
    }
    if (drawing && stack.isEditingMask() && herramientaDePintura()) {
        m_maskEdit.continueStroke(lastPoint, pos);
        lastPoint = pos;
        return;
    }
    if (vectorEditMode && herramientaVectorizable()) {
        vectorHoverPos = QPointF(pos.x(), pos.y());
        if (draggingVectorPoint && selectedVectorPoint >= 0 && selectedVectorPoint < vectorPoints.size()) {
            vectorPoints[selectedVectorPoint] = QPointF(pos.x(), pos.y());
            update(); return;
        }
        bool overPoint = (findVectorPointAt(QPointF(pos.x(), pos.y())) >= 0);
        bool overFirst = isNearFirstPoint(QPointF(pos.x(), pos.y()));
        if (overFirst) setCursor(Qt::ClosedHandCursor);
        else if (overPoint) setCursor(Qt::OpenHandCursor);
        else setCursor(Qt::CrossCursor);
        update(); return;
    }
    if (textEdit.active) {
        if (textEdit.dragging) { textEdit.dragTo(pos); return; }
        if (textEdit.resizing) { textEdit.resizeTo(pos, 20, 20); return; }
    }
    if (resizingCanvas) {
        if (resizeMode == 1 || resizeMode == 3) previewCanvasSize.setX(qMax(50, int(rawPos.x() / zoomFactor)));
        if (resizeMode == 2 || resizeMode == 3) previewCanvasSize.setY(qMax(50, int(rawPos.y() / zoomFactor)));
        update(); return;
    }
    if (selMgr.isRotating()) {
        selMgr.updateRotation(pos);
        if (event->modifiers() & Qt::ShiftModifier) selMgr.snapRotation();
        update(); return;
    }
    if (selMgr.isResizing()) {
        bool keepAspect = (event->modifiers() & Qt::ShiftModifier);
        selMgr.applyResize(selMgr.activeResizeHandle(), QPointF(pos.x(), pos.y()), keepAspect);
        setCursor(cursorForHandle(selMgr.activeResizeHandle()));
        update(); return;
    }
    if (selMgr.isDragging()) { selMgr.moveTo(pos); update(); return; }
    if (drawingGradient && currentTool == ToolGradient) { gradientEnd = pos; update(); return; }
    if (cloneIsStamping && currentTool == ToolClone && cloneSourceSet && (event->buttons() & Qt::LeftButton)) {
        if (capaValida()) { applyClonStamp(stack.currentImage(), pos); invalidarTrazo(pos, pos); emit layersChanged(); }
        invalidarPreviewClone(pos);
        return;
    }
    if (m_deform.isActive() && currentTool == ToolDeform &&
        (event->buttons() & (Qt::LeftButton | Qt::RightButton))) {
        const QPoint prev = previousMousePos;
        m_deform.continueStroke(pos);
        invalidarTrazo(prev, pos);
        return;
    }
    if (currentTool == ToolMove && (event->buttons() & Qt::LeftButton)) {
        QPointF canvasPos(pos.x(), pos.y());
        int activeIdx = selMgr.activeObjectIndex();
        if (activeIdx >= 0 && activeIdx < selMgr.objectCount()) {
            if (selMgr.isObjectDragging() && selMgr.objectActiveHandle() == ObjectHandle::Move) {
                selMgr.moveActiveObject(canvasPos);
                setCursor(Qt::ClosedHandCursor);
                update(); return;
            } else if (selMgr.isObjectRotating() && selMgr.objectActiveHandle() == ObjectHandle::Rotate) {
                selMgr.rotateActiveObject(canvasPos, event->modifiers() & Qt::ShiftModifier);
                setCursor(Qt::ClosedHandCursor);
                update(); return;
            } else if (selMgr.isObjectScaling()) {
                selMgr.scaleActiveObject(canvasPos, event->modifiers() & Qt::ShiftModifier);
                setCursor(cursorForHandle(selMgr.objectActiveHandle()));
                update(); return;
            }
        }
        if (movingLayer && moveLayerIdx >= 0 && moveLayerIdx < stack.count()) {
            QPoint delta = pos - moveStartPos;
            stack.layerAt(moveLayerIdx).image.fill(Qt::transparent);
            QPainter p(&stack.layerAt(moveLayerIdx).image);
            p.drawImage(delta.x(), delta.y(), moveLayerBackup);
            p.end();
            recomponerImagen();
            emit layersChanged();
            update(); return;
        }
    }
    if (currentTool == ToolMove && !(event->buttons() & Qt::LeftButton)) {
        QPointF canvasPos(pos.x(), pos.y());
        int activeIdx = selMgr.activeObjectIndex();
        if (activeIdx >= 0 && activeIdx < selMgr.objectCount() && selMgr.objectAt(activeIdx).selected) {
            ObjectHandle h = selMgr.findObjectGizmoHandleAt(activeIdx, canvasPos, zoomFactor);
            setCursor(cursorForHandle(h));
        } else {
            int hitObj = selMgr.findObjectAt(canvasPos);
            if (hitObj >= 0) setCursor(Qt::SizeAllCursor);
            else setCursor(Qt::ArrowCursor);
        }
        return;
    }
    if (selMgr.isActive() && selMgr.hasBuffer() && !(event->buttons() & Qt::LeftButton) && !drawing &&
        (currentTool == ToolSelect || currentTool == ToolSelectFree || currentTool == ToolMove)) {
        QPointF cp(pos.x(), pos.y());
        QPointF convPos = selMgr.convertHandlePos(zoomFactor);
        if (QLineF(cp, convPos).length() <= 12.0 / zoomFactor) {
            setCursor(Qt::PointingHandCursor);
        } else {
            setCursor(cursorForHandle(selMgr.hitTestGizmoAt(cp, zoomFactor)));
        }
        return;
    }
    if (drawing) {
        QRect selPrevia = selMgr.rect();
        QColor colorDeUso = obtenerColorDeTrabajo(activeMouseButton);
        if (currentTool != ToolEraser) colorDeUso.setAlpha(penOpacity);
        QColor colorOpuesto = obtenerColorDeTrabajo(activeMouseButton == Qt::LeftButton ? Qt::RightButton : Qt::LeftButton);
        QPoint puntoPrevio = lastPoint;
        if (usaStampDePincel()) {
            if (capaValida()) {
                double segLen = QLineF(lastClassicPoint, QPointF(pos)).length();
                strokeAccumulatedLength += segLen;
                double estimatedTotal = strokeAccumulatedLength + segLen * 10.0;
                if (estimatedTotal > strokeTotalLength) strokeTotalLength = estimatedTotal;
                PaintEngine::applyCustomBrushLine(stack.currentImage(), lastClassicPoint, pos,
                    activeStamp(), activePreset(),
                    mouseSensitivity, lastClassicPoint, colorDeUso, colorOpuesto, strokeCanvasFallback,
                    strokeTotalLength, strokeAccumulatedLength - segLen);
            }
            lastPoint = pos;
        } else if ((currentTool == ToolPencil || currentTool == ToolEraser || currentTool == ToolMirrorPen || currentTool == ToolLighten) && pixelOptions.getIsPixelArtMode()) {
            if (capaValida()) {
                drawPixelArtLine(stack.currentImage(), lastPoint, pos, colorDeUso, currentTool, true);
            }
            lastPoint = pos;
        } else if (currentTool == ToolSelect) {
            selMgr.updateRect(startPoint, pos, stack.canvasSize());
        } else if (currentTool == ToolSelectFree || currentTool == ToolLassoExtract || currentTool == ToolLassoDelete) {
            selMgr.updateFree(pos, stack.canvasSize());
        } else if (currentTool == ToolPencil) {
            if (capaValida()) {
                PaintEngine::applyGraphitePencil(stack.currentImage(), lastPoint, pos, colorDeUso, scaledWidth, penOpacity);
            }
            lastPoint = pos;
        } else if (currentTool == ToolEraser) {
            if (capaValida()) {
                PaintEngine::applyEraserLine(stack.currentImage(), lastPoint, pos, scaledWidth, true);
            }
            lastPoint = pos;
        } else if (currentTool == ToolBlur || currentTool == ToolHeal || currentTool == ToolShadowBurn) {
            RetouchTools::applyRetouchAlongLine(stack.currentImage(), lastPoint, pos,
                                                penWidth, mouseSensitivity, penOpacity,
                                                retouchToolCode(currentTool));
            lastPoint = pos;
        } else { lastPoint = pos; }
        if (currentTool == ToolSelect || currentTool == ToolSelectFree ||
            currentTool == ToolLassoExtract || currentTool == ToolLassoDelete) {
            repintarZonaCanvas(selPrevia.united(selMgr.rect()).adjusted(-6, -6, 6, 6));
        } else if (ToolCategories::isShapeTool(currentTool) || currentTool == ToolPixelStroke) {
            invalidarTrazo(puntoPrevio, pos, QRect(startPoint, puntoPrevio).normalized());
        } else { invalidarTrazo(puntoPrevio, pos); }
        return;
    }
    if (textEdit.active) {
        TextEngine::Handle h = textEdit.hitHandleAt(pos);
        setCursor(TextEngine::cursorForHandle(h));
        update();
        return;
    }
    setCursor(Qt::CrossCursor);
    if (currentTool == ToolZoom || currentTool == ToolPicker) setCursor(Qt::PointingHandCursor);
    else if (currentTool == ToolClone) setCursor(Qt::CrossCursor);
    else if (currentTool == ToolBlur || currentTool == ToolHeal || currentTool == ToolShadowBurn || currentTool == ToolDeform) setCursor(Qt::BlankCursor);
    if (currentTool == ToolClone && cloneSourceSet) invalidarPreviewClone(pos);
    if (currentTool == ToolBlur || currentTool == ToolHeal || currentTool == ToolShadowBurn || currentTool == ToolDeform ||
        usaStampDePincel()) {
        update(rectSiluetaWidget(hoverPos).adjusted(-2, -2, 2, 2));
    } else if (currentTool == ToolPenBezier) {
        bezierTool.move(pos, zoomFactor);
        update();
    }
}

/// ========== mouseReleaseEvent ==========
void PaintArea::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == activeMouseButton) {
        continuousDrawTimer->stop();
        int scaledWidth = qMax(1, static_cast<int>(penWidth * mouseSensitivity));
        m_maskEdit.endBezierDrag();

        if (drawing && stack.isEditingMask() && herramientaDePintura()) {
            m_maskEdit.endStroke();
            drawing = false;
            activeMouseButton = Qt::NoButton;
            emit layersChanged();
            update();
            return;
        }
        if (vectorEditMode && draggingVectorPoint) { draggingVectorPoint = false; return; }
        if (textEdit.dragging) { textEdit.endDrag(); return; }
        if (textEdit.resizing) { textEdit.endResize(); return; }
        if (resizingCanvas) { resizingCanvas = false; cambiarDimensionesLienzo(previewCanvasSize.x(), previewCanvasSize.y()); resizeMode = 0; return; }
        if (selMgr.isResizing()) { selMgr.setResizing(false); selMgr.setResizeHandle(ObjectHandle::None); return; }
        if (selMgr.isDragging()) { selMgr.setDragging(false); return; }
        if (selMgr.isRotating()) { selMgr.setRotating(false); return; }
        if (drawingGradient && currentTool == ToolGradient) {
            drawingGradient = false;
            if (gradientStart != gradientEnd) {
                saveHistoryState();
                aplicarGradienteConfigurado(gradientStart, gradientEnd);
                emit statusBarMessage(tr("Gradiente aplicado"));
                refreshAndNotify();
            } else {
                update();
            }
            return;
        }
        if (cloneIsStamping && currentTool == ToolClone) { cloneIsStamping = false; update(); return; }
        if (m_deform.isActive() && currentTool == ToolDeform) {
            m_deform.end();
            activeMouseButton = Qt::NoButton;
            emit layersChanged();
            update();
            return;
        }
        if (currentTool == ToolMove) {
            if (selMgr.isObjectDragging() || selMgr.isObjectRotating() || selMgr.isObjectScaling()) {
                selMgr.stopObjectManipulation();
                setCursor(Qt::ArrowCursor);
                update(); return;
            }
            if (movingLayer) { movingLayer = false; moveLayerBackup = QImage(); moveLayerIdx = -1; update(); return; }
        }
        if (drawing) {
            strokeTotalLength = strokeAccumulatedLength;
            strokeAccumulatedLength = 0.0;
            strokeInProgress = false;
            QPoint finalPoint(event->position().toPoint().x() / zoomFactor,
                              event->position().toPoint().y() / zoomFactor);

            // Lasso (extraer / borrar)
            if (currentTool == ToolLassoExtract || currentTool == ToolLassoDelete) {
                drawing = false;
                selMgr.closeFreePath();
                QPainterPath lassoPath = selMgr.path();
                if (puedeEditarCapaActual()) {
                    saveHistoryState();
                    if (currentTool == ToolLassoExtract) {
                        stack.currentImage() = LassoProcessor::applyLassoExtract(
                            stack.currentImage(), lassoPath, pixelOptions.getIsPixelArtMode());
                        emit statusBarMessage(tr("Fondo recortado"));
                    } else {
                        LassoProcessor::applyLassoDelete(
                            stack.currentImage(), lassoPath, pixelOptions.getIsPixelArtMode());
                        emit statusBarMessage(tr("Objeto borrado"));
                    }
                    refreshAndNotify();
                }
                selMgr.discard();
                update();
                activeMouseButton = Qt::NoButton;
                return;
            }

            // Selección (rect / free)
            if (currentTool == ToolSelect || currentTool == ToolSelectFree) {
                drawing = false;
                if (currentTool == ToolSelectFree) {
                    selMgr.closeFreePath();
                    QRect selBounds = selMgr.path().boundingRect().toRect().intersected(stack.canvasRect());
                    if (selBounds.width() > 4 && selBounds.height() > 4) {
                        saveHistoryState();
                        selMgr.finalizeFree(stack.currentImage());
                        emit statusBarMessage(tr("Selección libre"));
                        refreshAndNotify();
                    }
                } else {
                    selMgr.updateRect(startPoint, finalPoint, stack.canvasSize());
                    if (selMgr.rect().width() > 4 && selMgr.rect().height() > 4) {
                        saveHistoryState();
                        selMgr.finalizeRect(stack.currentImage());
                        emit statusBarMessage(tr("Selección"));
                        refreshAndNotify();
                    }
                }
            }

            // Figuras / PixelStroke
            else if (ToolCategories::isShapeTool(currentTool) || currentTool == ToolPixelStroke) {
                QColor colorDeUso = obtenerColorDeTrabajo(activeMouseButton);
                if (currentTool != ToolEraser) colorDeUso.setAlpha(penOpacity);
                if (pixelOptions.getIsPixelArtMode()) {
                    if (capaValida()) {
                        if ((currentTool == ToolPixelStroke || currentTool == ToolLine))
                            drawPixelArtShape(stack.currentImage(), startPoint, finalPoint, colorDeUso, currentTool);
                        else {
                            QPainter painter(&stack.currentImage());
                            painter.setRenderHint(QPainter::Antialiasing, false);
                            painter.setPen(QPen(colorDeUso, scaledWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
                            PaintEngine::drawGeometry(painter, startPoint, finalPoint, currentTool);
                            painter.end();
                        }
                        recomponerImagen();
                    }
                } else if (ToolCategories::isShapeTool(currentTool)) {
                    selMgr.registerShapeObject(currentTool, startPoint, finalPoint, colorDeUso, colorDeUso, scaledWidth, stack.currentIndex());
                } else {
                    if (capaValida()) {
                        QPainter painter(&stack.currentImage());
                        painter.setRenderHint(QPainter::Antialiasing, true);
                        painter.setPen(QPen(colorDeUso, scaledWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
                        PaintEngine::drawGeometry(painter, startPoint, finalPoint, currentTool);
                        painter.end(); recomponerImagen();
                    }
                }
            }

            emit layersChanged();
            drawing = false;
            activeMouseButton = Qt::NoButton;
            update();
        }
    }
}

/// ========== wheelEvent ==========
void PaintArea::wheelEvent(QWheelEvent *event) {
    QPoint localPos = event->position().toPoint();
    QPoint viewportPos = mapToParent(localPos);
    double factor = 1.15;
    double newZoom = zoomFactor;
    if (event->angleDelta().y() > 0) newZoom = zoomFactor * factor;
    else if (event->angleDelta().y() < 0) newZoom = zoomFactor / factor;
    else { event->accept(); return; }
    emit zoomRequested(newZoom, viewportPos);
    event->accept();
}

/// ========== paintEvent ==========
void PaintArea::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    int scaledWidth = qMax(1, static_cast<int>(penWidth * mouseSensitivity));
    QRect canvasRect(0, 0, (int)(stack.width() * zoomFactor), (int)(stack.height() * zoomFactor));
    static const QPixmap checker = []() {
        QPixmap c(16, 16); c.fill(QColor(255, 255, 255));
        QPainter pc(&c);
        pc.fillRect(0, 0, 8, 8, QColor(210, 210, 210));
        pc.fillRect(8, 8, 8, 8, QColor(210, 210, 210));
        pc.end();
        return c;
    }();
    painter.fillRect(canvasRect, QBrush(checker));
    renderTiles(painter, event->rect());
    painter.save();
    painter.scale(zoomFactor, zoomFactor);

    if (pixelOptions.isGridActive())
        drawPixelArtGrid(painter, stack.compositedImage(), pixelOptions.getGridSize(), zoomFactor);

    selMgr.drawAllObjects(painter);

    // Máscara
    if (stack.isEditingMask()) {
        const QImage &rubylithImg = stack.ensureRubylith(stack.maskEditLayer(), stack.canvasSize());
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter.drawImage(0, 0, rubylithImg);
        painter.setPen(QPen(QColor(220, 40, 60), 2.0 / zoomFactor, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(stack.canvasRect());
        painter.setPen(Qt::white);
        QFont labFont; labFont.setPixelSize(qMax(10, (int)(12 / zoomFactor))); labFont.setBold(true);
        painter.setFont(labFont);
        painter.fillRect(0, 0, 320 / zoomFactor, 20 / zoomFactor, QColor(220, 40, 60, 200));
        painter.drawText(4 / zoomFactor, 15 / zoomFactor, tr("MÁSCARA: negro oculta / blanco revela"));
        if (currentTool == ToolPenBezier && m_maskEdit.hasBezierNodes()) {
            m_maskEdit.paintBezierOverlay(painter, zoomFactor);
        }
    }

    // Bezier activo (no máscara)
    if (currentTool == ToolPenBezier && !stack.isEditingMask()) {
        bezierTool.paint(painter, zoomFactor);
    }

    // Preview de redimensión del lienzo
    if (resizingCanvas) {
        painter.setPen(QPen(darkModeActive ? Qt::white : Qt::black, 1.5 / zoomFactor, Qt::DashLine));
        painter.drawRect(0, 0, previewCanvasSize.x(), previewCanvasSize.y());
    }

    // Preview de figuras
    if (drawing && (ToolCategories::isShapeTool(currentTool) || currentTool == ToolPixelStroke)) {
        QColor colorDeUso = obtenerColorDeTrabajo(activeMouseButton);
        if (currentTool != ToolEraser) colorDeUso.setAlpha(penOpacity);
        if ((currentTool == ToolPixelStroke || currentTool == ToolLine) && pixelOptions.getIsPixelArtMode())
            drawPixelArtShape(stack.currentImage(), startPoint, lastPoint, colorDeUso, currentTool);
        else {
            painter.setPen(QPen(colorDeUso, scaledWidth, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));
            painter.setBrush(Qt::NoBrush);
            PaintEngine::drawGeometry(painter, startPoint, lastPoint, currentTool);
        }
    }

    // Selección activa
    if (selMgr.isActive() && selMgr.hasBuffer()) {
        selMgr.drawSelectionOverlay(painter, zoomFactor);
    }

    // Vector edit
    if (vectorEditMode && herramientaVectorizable() && !vectorPoints.isEmpty()) {
        QColor vectorCol = colorVectorActivo();
        painter.setPen(QPen(vectorCol, 2.0 / zoomFactor, Qt::SolidLine));
        painter.setBrush(Qt::NoBrush);
        QPainterPath vpath;
        vpath.moveTo(vectorPoints.first());
        for (int i = 1; i < vectorPoints.size(); ++i) vpath.lineTo(vectorPoints[i]);
        painter.drawPath(vpath);
        painter.setPen(QPen(vectorCol, 1.5 / zoomFactor, Qt::DashLine));
        painter.drawLine(vectorPoints.last(), vectorHoverPos);
        if (isNearFirstPoint(vectorHoverPos) && vectorPoints.size() >= 3) {
            painter.setPen(QPen(vectorCol, 2.5 / zoomFactor, Qt::SolidLine));
            painter.drawLine(vectorPoints.last(), vectorPoints.first());
        }
        double pointRadius = 5.0 / zoomFactor;
        for (int i = 0; i < vectorPoints.size(); ++i) {
            const QPointF &pt = vectorPoints[i];
            bool isFirst = (i == 0);
            bool isSelectedNode = (i == selectedVectorPoint);
            painter.setPen(QPen(Qt::white, 1.5 / zoomFactor));
            if (isFirst) {
                painter.setBrush(vectorCol);
                double r = pointRadius * 1.3;
                painter.drawRect(QRectF(pt.x() - r, pt.y() - r, r * 2, r * 2));
            } else {
                painter.setBrush(isSelectedNode ? QColor(239, 68, 68) : vectorCol);
                painter.drawEllipse(pt, pointRadius, pointRadius);
            }
        }
    }

    // Preview de la selección
    if (drawing && (currentTool == ToolSelect || currentTool == ToolSelectFree ||
                    currentTool == ToolLassoExtract || currentTool == ToolLassoDelete)) {
        painter.setPen(QPen((currentTool == ToolLassoExtract || currentTool == ToolLassoDelete)
                            ? QColor(255, 0, 150) : selBlue(),
                            1.5 / zoomFactor, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        if (currentTool == ToolSelect) painter.drawRect(selMgr.rect());
        else painter.drawPath(selMgr.path());
    }

    // Texto (via TextEngine)
    if (textEdit.active) {
        TextEngine::Style ts = currentTextStyle();
        ts.handleSize = textEdit.handleSize;
        textEdit.paint(painter, zoomFactor, ts);
    }

    // Gradiente
    if (drawingGradient && currentTool == ToolGradient) {
        QColor c1 = gradientReverse ? penColor2 : penColor1;
        QColor c2;
        if (gradientUseSecondColor) { c2 = gradientReverse ? penColor1 : penColor2; }
        else { c2 = c1; c2.setAlpha(0); }
        c1.setAlpha(gradientOpacity);
        c2.setAlpha(gradientUseSecondColor ? gradientOpacity : 0);
        QGradient *grad = nullptr;
        switch (gradientType) {
            case GradientLinear: grad = new QLinearGradient(gradientStart, gradientEnd); break;
            case GradientRadial: {
                int radius = qMax(1, (int)sqrt(pow(gradientEnd.x() - gradientStart.x(), 2) +
                                                pow(gradientEnd.y() - gradientStart.y(), 2)));
                grad = new QRadialGradient(gradientStart, radius); break;
            }
            case GradientConic: grad = new QConicalGradient(gradientStart, gradientAngle); break;
        }
        if (grad) {
            grad->setColorAt(0.0, c1);
            grad->setColorAt(1.0, c2);
            painter.setPen(Qt::NoPen);
            painter.setBrush(*grad);
            painter.setOpacity(0.85);
            painter.drawRect(stack.canvasRect());
            painter.setOpacity(1.0);
            delete grad;
        }
        painter.setPen(QPen(QColor(255, 80, 80), 2.0 / zoomFactor, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(gradientStart, gradientEnd);
        painter.setBrush(QColor(255, 80, 80, 220));
        painter.drawEllipse(QPointF(gradientStart), 6.0 / zoomFactor, 6.0 / zoomFactor);
        painter.drawEllipse(QPointF(gradientEnd), 6.0 / zoomFactor, 6.0 / zoomFactor);
        painter.setPen(Qt::white);
        QFont infoFont;
        infoFont.setPixelSize(qMax(10, (int)(12 / zoomFactor)));
        painter.setFont(infoFont);
        QString info = QString("%1 | %2° | %3% | %4")
            .arg(gradientType == GradientLinear ? "Lineal" :
                 gradientType == GradientRadial ? "Radial" : "Cónico")
            .arg(gradientAngle)
            .arg(qRound(gradientOpacity / 255.0 * 100))
            .arg(gradientUseSecondColor ? "2col" : "→ Transp");
        painter.drawText(gradientStart.x() + 10 / zoomFactor, gradientStart.y() - 10 / zoomFactor, info);
    }

    // Clonar
    if (currentTool == ToolClone && cloneSourceSet) {
        QPoint offset = cloneSource - cloneInitialDest;
        QPoint currentSource = currentMousePos + offset;
        int brushSize = qMax(1, static_cast<int>(penWidth * mouseSensitivity)) * 2;
        painter.setPen(QPen(QColor(100, 200, 255), 2.0 / zoomFactor, Qt::SolidLine));
        painter.setBrush(QColor(100, 200, 255, 40));
        painter.drawEllipse(cloneSource, brushSize, brushSize);
        painter.setPen(QPen(QColor(100, 200, 255), 1.0 / zoomFactor, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(cloneSource, QPoint(currentMousePos.x(), currentMousePos.y()));
        if (cloneIsStamping) {
            painter.setPen(QPen(QColor(255, 200, 100), 1.5 / zoomFactor, Qt::DotLine));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(currentSource, brushSize, brushSize);
        }
    }

    // Gizmos de Move
    if (currentTool == ToolMove) {
        selMgr.drawObjectGizmos(painter, zoomFactor, darkModeActive);
    }
    if (selMgr.isActive() && selMgr.hasBuffer()) {
        selMgr.drawSelectionGizmo(painter, zoomFactor, darkModeActive, selBlue(), selBlueLight());
    }

    painter.restore();

    // Handles del canvas
    painter.setPen(QPen(darkModeActive ? Qt::white : QColor("#404040"), 1));
    painter.setBrush(Qt::white);
    painter.drawRect(getRightHandle());
    painter.drawRect(getBottomHandle());
    painter.drawRect(getBottomRightHandle());

    // Siluetas de pincel bajo el cursor
    if (rect().contains(hoverPos) && !textEdit.active && !vectorEditMode) {
        if (stack.isEditingMask() && herramientaDePintura()) {
            painter.setPen(QPen(darkModeActive ? QColor(255, 255, 255, 140) : QColor(0, 0, 0, 120), 1, Qt::DashLine));
            painter.setBrush(Qt::NoBrush);
            BrushSettings cfg;
            if (usaStampDePincel()) cfg = activePreset();
            else {
                cfg.shape = ShapeType::Circle;
                cfg.dragMode = DragMode::Continuous;
                cfg.rotationMode = RotationMode::Fixed;
                cfg.size = qMax(5, (int)(penWidth * mouseSensitivity * 2));
            }
            double sSize = qMax(4, cfg.size) * zoomFactor;
            painter.save();
            painter.setRenderHint(QPainter::Antialiasing, true);
            PaintEngine::drawBrushSilhouette(painter, cfg, sSize, hoverPos);
            painter.restore();
        } else if (currentTool == ToolBlur || currentTool == ToolHeal || currentTool == ToolShadowBurn) {
            painter.setPen(QPen(darkModeActive ? QColor(255, 255, 255, 180) : QColor(0, 0, 0, 150), 1));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(QPointF(hoverPos),
                (double)((scaledWidth * 2 + 2) * zoomFactor),
                (double)((scaledWidth * 2 + 2) * zoomFactor));
        } else if (currentTool == ToolDeform) {
            m_deform.paintOverlay(painter, hoverPos, zoomFactor, darkModeActive);
        } else if (currentTool == ToolClone && cloneSourceSet) {
            painter.setPen(QPen(darkModeActive ? QColor(255, 255, 255, 200) : QColor(0, 0, 0, 180), 1.5));
            painter.setBrush(Qt::NoBrush);
            int brushSize = (scaledWidth * 2) * zoomFactor;
            painter.drawEllipse(QPointF(hoverPos), (double)brushSize, (double)brushSize);
            painter.drawLine(hoverPos.x() - 4, hoverPos.y(), hoverPos.x() + 4, hoverPos.y());
            painter.drawLine(hoverPos.x(), hoverPos.y() - 4, hoverPos.x(), hoverPos.y() + 4);
        } else if (usaStampDePincel()) {
            painter.setPen(QPen(darkModeActive ? QColor(255, 255, 255, 120) : QColor(0, 0, 0, 100), 1, Qt::DashLine));
            painter.setBrush(Qt::NoBrush);
            double sSize = activePreset().size * zoomFactor;
            painter.save();
            painter.setRenderHint(QPainter::Antialiasing, true);
            PaintEngine::drawBrushSilhouette(painter, activePreset(), sSize, hoverPos);
            painter.restore();
        }
    }
}

#include "moc_paintarea.cpp"
