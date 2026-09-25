#ifndef SELECTTOOLS_H
#define SELECTTOOLS_H

#include <QPainter>
#include <QPainterPath>
#include <QImage>
#include <QRect>
#include <QRectF>
#include <QPoint>
#include <QPointF>
#include <QList>
#include <QMap>
#include <QApplication>
#include <QClipboard>
#include <QTextOption>
#include <QFont>
#include <QColor>
#include <QPen>
#include <QBrush>
#include <QLineF>
#include <cmath>
#include <algorithm>

#include "core/ShapeObjects.h"

// ============================================================
// SelectionManager
// Encapsula TODA la lógica de selección (rect, libre, path,
// varita) y de objetos (shapes, text, selection-image).
// Extraído de PaintArea para reducir su tamaño.
// ============================================================

class SelectionManager {
public:
    enum SelectionType { SelectionNone = 0, SelectionRect, SelectionPath, SelectionMagicWand };

private:
    // --- Estado de selección ---
    SelectionType selType = SelectionNone;
    bool active = false;
    bool dragging = false;
    bool resizing = false;
    bool rotating = false;

    ObjectHandle resizeHandle = ObjectHandle::None;
    QRectF resizeStartRect;
    QPointF resizeStartPos;

    QRect selRect;
    QImage selBuffer;
    QPainterPath freePath;
    QPoint dragOffset;
    double selRotation = 0.0;

    // --- Objetos ---
    QList<PaintObject> objects;
    int activeObjIndex = -1;

    bool objDragging = false;
    bool objRotating = false;
    bool objScaling = false;
    ObjectHandle objActiveHandle = ObjectHandle::None;
    QPointF objDragStart;
    double objRotationStart = 0.0;
    double objScaleStartX = 1.0;
    double objScaleStartY = 1.0;
    QRectF objBoundsStart;

    QMap<int, QImage> selObjBuffers;
    int nextSelBufferId = 0;

    // --- Clipboard interno ---
    QImage clipboardBuffer;

public:
    // ==================== GETTERS DE SELECCIÓN ====================
    bool isActive() const { return active; }
    SelectionType type() const { return selType; }
    QRect rect() const { return selRect; }
    QImage buffer() const { return selBuffer; }
    double rotation() const { return selRotation; }
    QPainterPath path() const { return freePath; }
    bool hasBuffer() const { return !selBuffer.isNull(); }

    // ==================== SETTERS DE SELECCIÓN ====================
    void setType(SelectionType t) { selType = t; }
    void setActive(bool a) { active = a; }
    void setRect(const QRect &r) { selRect = r; }
    void setBuffer(const QImage &b) { selBuffer = b; }
    void setRotation(double r) { selRotation = r; }
    void setPath(const QPainterPath &p) { freePath = p; }

    // ==================== INICIAR SELECCIÓN ====================

    void startRect(const QPoint &pos) {
        selRect = QRect(pos, pos);
        selRotation = 0.0;
        freePath = QPainterPath();
    }

    void startFree(const QPoint &pos) {
        selRect = QRect(pos, pos);
        selRotation = 0.0;
        freePath = QPainterPath();
        freePath.moveTo(pos);
    }

    // ==================== ACTUALIZAR SELECCIÓN ====================

    void updateRect(const QPoint &start, const QPoint &end, const QSize &canvasSize) {
        selRect = QRect(start, end).normalized().intersected(QRect(QPoint(0, 0), canvasSize));
    }

    void updateFree(const QPoint &pos, const QSize &canvasSize) {
        freePath.lineTo(pos);
        selRect = freePath.boundingRect().toRect().intersected(QRect(QPoint(0, 0), canvasSize));
    }

    void closeFreePath() { freePath.closeSubpath(); }

    // ==================== FINALIZAR SELECCIÓN ====================

    bool finalizeRect(QImage &layerImage) {
        if (selRect.width() <= 4 || selRect.height() <= 4) {
            active = false;
            selType = SelectionNone;
            return false;
        }
        selBuffer = QImage(selRect.size(), QImage::Format_ARGB32);
        selBuffer.fill(Qt::transparent);

        QPainter destPainter(&selBuffer);
        destPainter.drawImage(0, 0, layerImage.copy(selRect));
        destPainter.end();

        QPainter srcPainter(&layerImage);
        srcPainter.setCompositionMode(QPainter::CompositionMode_Clear);
        srcPainter.fillRect(selRect, Qt::transparent);
        srcPainter.end();

        selRotation = 0.0;
        selType = SelectionRect;
        active = true;
        return true;
    }

    bool finalizeFree(QImage &layerImage) {
        selRect = freePath.boundingRect().toRect().intersected(layerImage.rect());
        if (selRect.width() <= 4 || selRect.height() <= 4) {
            active = false;
            selType = SelectionNone;
            freePath = QPainterPath();
            return false;
        }
        selBuffer = QImage(selRect.size(), QImage::Format_ARGB32);
        selBuffer.fill(Qt::transparent);

        QPainter destPainter(&selBuffer);
        destPainter.setClipPath(freePath.translated(-selRect.topLeft()));
        destPainter.drawImage(0, 0, layerImage.copy(selRect));
        destPainter.end();

        QPainter srcPainter(&layerImage);
        srcPainter.setCompositionMode(QPainter::CompositionMode_Clear);
        srcPainter.setClipPath(freePath);
        srcPainter.fillRect(layerImage.rect(), Qt::transparent);
        srcPainter.end();

        selRotation = 0.0;
        selType = SelectionPath;
        active = true;
        return true;
    }

    bool finalizeVectorPath(const QPainterPath &path, QImage &layerImage) {
        QRect bounds = path.boundingRect().toRect().intersected(layerImage.rect());
        if (bounds.width() <= 4 || bounds.height() <= 4) return false;

        selRect = bounds;
        selBuffer = QImage(bounds.size(), QImage::Format_ARGB32);
        selBuffer.fill(Qt::transparent);

        QPainter destPainter(&selBuffer);
        destPainter.setClipPath(path.translated(-bounds.topLeft()));
        destPainter.drawImage(0, 0, layerImage.copy(bounds));
        destPainter.end();

        QPainter srcPainter(&layerImage);
        srcPainter.setCompositionMode(QPainter::CompositionMode_Clear);
        srcPainter.setClipPath(path);
        srcPainter.fillRect(layerImage.rect(), Qt::transparent);
        srcPainter.end();

        selRotation = 0.0;
        selType = SelectionPath;
        active = true;
        return true;
    }

    void applyMagicWand(const QRect &bbox, const QImage &extracted) {
        selRect = bbox;
        selBuffer = extracted;
        selType = SelectionMagicWand;
        active = true;
        selRotation = 0.0;
        freePath = QPainterPath();
    }

    void pasteAsSelection(const QImage &img, const QPoint &pos = QPoint(20, 20)) {
        selBuffer = img;
        selRect = QRect(pos.x(), pos.y(), img.width(), img.height());
        selRotation = 0.0;
        active = true;
        selType = SelectionRect;
    }

    // ==================== DESCARTAR / BAKE ====================

    void discard() {
        active = false;
        dragging = false;
        resizing = false;
        rotating = false;
        selRotation = 0.0;
        selBuffer = QImage();
        freePath = QPainterPath();
        resizeHandle = ObjectHandle::None;
        selType = SelectionNone;
    }

    bool bake(QImage &layerImage, bool antialias) {
        if (!active || selBuffer.isNull()) return false;
        QPainter painter(&layerImage);
        painter.setRenderHint(QPainter::Antialiasing, antialias);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, antialias);
        painter.translate(selRect.center());
        painter.rotate(selRotation);
        painter.drawImage(QRect(-selRect.width() / 2, -selRect.height() / 2,
                                selRect.width(), selRect.height()), selBuffer);
        painter.end();
        return true;
    }

    // ==================== DRAG / RESIZE / ROTATE ====================

    void setDragging(bool d) { dragging = d; }
    bool isDragging() const { return dragging; }
    void setDragOffset(const QPoint &off) { dragOffset = off; }
    QPoint dragOffsetValue() const { return dragOffset; }
    void moveTo(const QPoint &pos) { selRect.moveTo(pos - dragOffset); }

    void setResizing(bool r) { resizing = r; }
    bool isResizing() const { return resizing; }
    void setResizeHandle(ObjectHandle h) { resizeHandle = h; }
    ObjectHandle activeResizeHandle() const { return resizeHandle; }
    void setResizeStart(const QRectF &r, const QPointF &p) { resizeStartRect = r; resizeStartPos = p; }
    QRectF resizeStartRectValue() const { return resizeStartRect; }
    QPointF resizeStartPosValue() const { return resizeStartPos; }

    void applyResize(ObjectHandle handle, const QPointF &currentPos, bool keepAspect, double minSize = 5.0) {
        QRectF nr = resizeRotatedRect(handle, resizeStartRect, selRotation,
                                      resizeStartPos, currentPos, keepAspect, minSize);
        selRect = nr.toAlignedRect();
    }

    void setRotating(bool r) { rotating = r; }
    bool isRotating() const { return rotating; }

    void updateRotation(const QPoint &pos) {
        double dy = pos.y() - selRect.center().y();
        double dx = pos.x() - selRect.center().x();
        selRotation = atan2(dy, dx) * 180.0 / M_PI + 90.0;
    }

    void snapRotation(double step = 15.0) {
        selRotation = qRound(selRotation / step) * step;
    }

    // ==================== GIZMO DE SELECCIÓN ====================

    GizmoLayout gizmoLayout(double zoom) const {
        return computeGizmoLayout(QRectF(selRect), selRotation, zoom);
    }

    QPointF convertHandlePos(double zoom) const {
        GizmoLayout L = gizmoLayout(zoom);
        return L.midTop + L.upDir * (52.0 / zoom);
    }

    ObjectHandle hitTestGizmoAt(const QPointF &canvasPos, double zoom) const {
        GizmoLayout L = gizmoLayout(zoom);
        return hitTestGizmo(L, canvasPos, zoom);
    }

    bool isOverConvertHandle(const QPointF &canvasPos, double zoom) const {
        QPointF convPos = convertHandlePos(zoom);
        double threshold = 12.0 / zoom;
        return QLineF(canvasPos, convPos).length() <= threshold;
    }

    // ==================== OBJETOS: GETTERS ====================

    int objectCount() const { return objects.size(); }
    bool hasObjects() const { return !objects.isEmpty(); }
    PaintObject &objectAt(int idx) { return objects[idx]; }
    const PaintObject &objectAt(int idx) const { return objects[idx]; }
    int activeObjectIndex() const { return activeObjIndex; }
    void setActiveObjectIndex(int idx) { activeObjIndex = idx; }
    QList<PaintObject> &allObjects() { return objects; }
    const QList<PaintObject> &allObjects() const { return objects; }

    // ==================== OBJETOS: GESTIÓN ====================

    void addObject(const PaintObject &obj) { objects.append(obj); }

    void removeObjectAt(int idx) {
        if (idx < 0 || idx >= objects.size()) return;
        if (objects[idx].type == ObjectType::SelectionImage)
            selObjBuffers.remove(objects[idx].selectionBufferId);
        objects.removeAt(idx);
        if (activeObjIndex >= objects.size()) activeObjIndex = -1;
    }

    void clearObjects() {
        objects.clear();
        activeObjIndex = -1;
        selObjBuffers.clear();
    }

    int findObjectAt(const QPointF &canvasPos) const {
        for (int i = objects.size() - 1; i >= 0; --i) {
            if (objectContainsPoint(objects[i], canvasPos)) return i;
        }
        return -1;
    }

    void selectObject(int idx, bool addToSelection = false) {
        if (idx < 0 || idx >= objects.size()) return;
        if (!addToSelection) {
            for (int i = 0; i < objects.size(); ++i) objects[i].selected = (i == idx);
        } else {
            objects[idx].selected = !objects[idx].selected;
        }
        activeObjIndex = idx;
    }

    void deselectAllObjects() {
        for (PaintObject &obj : objects) obj.selected = false;
        activeObjIndex = -1;
    }

    QList<int> selectedObjectIndices() const {
        QList<int> result;
        for (int i = 0; i < objects.size(); ++i)
            if (objects[i].selected) result.append(i);
        return result;
    }

    void deleteSelectedObjects() {
        for (int i = objects.size() - 1; i >= 0; --i) {
            if (objects[i].selected) {
                if (objects[i].type == ObjectType::SelectionImage)
                    selObjBuffers.remove(objects[i].selectionBufferId);
                objects.removeAt(i);
            }
        }
        activeObjIndex = -1;
    }

    // ==================== OBJETOS: REGISTRO ====================

    int registerShapeObject(ToolType tool, const QPoint &p1, const QPoint &p2,
                            const QColor &fill, const QColor &stroke, int strokeW, int layerIndex) {
        PaintObject obj;
        obj.type = ObjectType::Shape;
        obj.shapeTool = tool;
        obj.bounds = QRectF(QRect(p1, p2).normalized());
        obj.fillColor = fill;
        obj.strokeColor = stroke;
        obj.strokeWidth = strokeW;
        obj.startPoint = p1;
        obj.endPoint = p2;
        obj.layerIndex = layerIndex;
        obj.selected = false;
        objects.append(obj);
        return objects.size() - 1;
    }

    int registerTextObject(const QRect &rect, const QString &text, const QFont &font,
                           const QColor &color, int layerIndex) {
        PaintObject obj;
        obj.type = ObjectType::Text;
        obj.bounds = QRectF(rect);
        obj.textContent = text;
        obj.textFont = font;
        obj.textColor = color;
        obj.layerIndex = layerIndex;
        obj.selected = false;
        objects.append(obj);
        return objects.size() - 1;
    }

    int registerSelectionImageObject(const QRect &rect, const QImage &buffer,
                                     double rotation, int layerIndex) {
        PaintObject obj;
        obj.type = ObjectType::SelectionImage;
        obj.bounds = QRectF(rect);
        obj.layerIndex = layerIndex;
        obj.selected = true;
        obj.rotation = rotation;
        obj.scaleX = 1.0;
        obj.scaleY = 1.0;
        obj.selectionBufferId = nextSelBufferId++;
        selObjBuffers.insert(obj.selectionBufferId, buffer);
        objects.append(obj);
        activeObjIndex = objects.size() - 1;
        return activeObjIndex;
    }

    // ==================== OBJETOS: BUFFERS ====================

    QImage selectionBufferFor(int bufferId) const { return selObjBuffers.value(bufferId); }
    bool hasSelectionBuffer(int bufferId) const { return selObjBuffers.contains(bufferId); }

    // ==================== OBJETOS: RENDER ====================

    QImage renderSelectedObjects() const {
        QList<int> selected = selectedObjectIndices();
        if (selected.isEmpty()) return QImage();

        QRectF totalBounds;
        for (int idx : selected) {
            QRectF tb = objects[idx].transformedBounds();
            totalBounds = totalBounds.isNull() ? tb : totalBounds.united(tb);
        }
        if (totalBounds.isEmpty()) return QImage();

        QRect r = totalBounds.toAlignedRect();
        QImage img(r.size(), QImage::Format_ARGB32);
        img.fill(Qt::transparent);

        QPainter p(&img);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p.setRenderHint(QPainter::TextAntialiasing, true);
        p.translate(-r.topLeft());
        for (int idx : selected) drawObjectInternal(p, objects[idx]);
        p.end();
        return img;
    }

    void drawObjectInternal(QPainter &painter, const PaintObject &obj) const {
        painter.save();
        if (obj.type == ObjectType::Shape) {
            renderShapeObject(painter, obj);
        } else if (obj.type == ObjectType::Text) {
            if (qAbs(obj.rotation) > 0.001 || qAbs(obj.scaleX - 1.0) > 0.001 || qAbs(obj.scaleY - 1.0) > 0.001) {
                painter.translate(obj.bounds.center());
                painter.rotate(obj.rotation);
                painter.scale(obj.scaleX, obj.scaleY);
                painter.translate(-obj.bounds.center());
            }
            painter.setFont(obj.textFont);
            painter.setPen(obj.textColor);
            QTextOption opt;
            opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
            opt.setAlignment(Qt::AlignLeft | Qt::AlignTop);
            painter.drawText(obj.bounds.toRect(), obj.textContent, opt);
        } else if (obj.type == ObjectType::SelectionImage) {
            if (selObjBuffers.contains(obj.selectionBufferId)) {
                const QImage &buf = selObjBuffers.value(obj.selectionBufferId);
                painter.translate(obj.bounds.center());
                painter.rotate(obj.rotation);
                painter.scale(obj.scaleX, obj.scaleY);
                painter.translate(-obj.bounds.center());
                painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
                painter.drawImage(obj.bounds.toRect(), buf);
            }
        }
        painter.restore();
    }

    void drawAllObjects(QPainter &painter) const {
        for (const PaintObject &obj : objects) drawObjectInternal(painter, obj);
    }

    // ==================== OBJETOS: BAKE ====================

    void bakeObjectInto(int idx, QImage &layerImage, bool antialias) {
        if (idx < 0 || idx >= objects.size()) return;
        const PaintObject &obj = objects[idx];
        QPainter painter(&layerImage);
        painter.setRenderHint(QPainter::Antialiasing, antialias);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        if (obj.type == ObjectType::Shape) {
            renderShapeObject(painter, obj);
        } else if (obj.type == ObjectType::Text) {
            if (qAbs(obj.rotation) > 0.001 || qAbs(obj.scaleX - 1.0) > 0.001 || qAbs(obj.scaleY - 1.0) > 0.001) {
                painter.translate(obj.bounds.center());
                painter.rotate(obj.rotation);
                painter.scale(obj.scaleX, obj.scaleY);
                painter.translate(-obj.bounds.center());
            }
            painter.setFont(obj.textFont);
            painter.setPen(obj.textColor);
            QTextOption opt;
            opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
            opt.setAlignment(Qt::AlignLeft | Qt::AlignTop);
            painter.drawText(obj.bounds.toRect(), obj.textContent, opt);
        } else if (obj.type == ObjectType::SelectionImage) {
            if (selObjBuffers.contains(obj.selectionBufferId)) {
                const QImage &buf = selObjBuffers.value(obj.selectionBufferId);
                painter.translate(obj.bounds.center());
                painter.rotate(obj.rotation);
                painter.scale(obj.scaleX, obj.scaleY);
                painter.translate(-obj.bounds.center());
                painter.drawImage(obj.bounds.toRect(), buf);
            }
        }
        painter.end();
    }

    // ==================== OBJETOS: MANIPULACIÓN ====================

    bool isObjectDragging() const { return objDragging; }
    bool isObjectRotating() const { return objRotating; }
    bool isObjectScaling() const { return objScaling; }
    ObjectHandle objectActiveHandle() const { return objActiveHandle; }

    void setObjectDragging(bool d) { objDragging = d; }
    void setObjectRotating(bool r) { objRotating = r; }
    void setObjectScaling(bool s) { objScaling = s; }
    void setObjectActiveHandle(ObjectHandle h) { objActiveHandle = h; }

    QPointF objectDragStartPos() const { return objDragStart; }
    void setObjectDragStart(const QPointF &p) { objDragStart = p; }

    double objectRotationStart() const { return objRotationStart; }
    void setObjectRotationStart(double r) { objRotationStart = r; }

    double objectScaleStartX() const { return objScaleStartX; }
    double objectScaleStartY() const { return objScaleStartY; }
    void setObjectScaleStart(double sx, double sy) { objScaleStartX = sx; objScaleStartY = sy; }

    QRectF objectBoundsStart() const { return objBoundsStart; }
    void setObjectBoundsStart(const QRectF &r) { objBoundsStart = r; }

    ObjectHandle findObjectGizmoHandleAt(int objIdx, const QPointF &canvasPos, double zoom) const {
        if (objIdx < 0 || objIdx >= objects.size()) return ObjectHandle::None;
        return ::findObjectGizmoHandle(objects[objIdx], canvasPos, zoom);
    }

    void moveActiveObject(const QPointF &canvasPos) {
        if (activeObjIndex < 0 || activeObjIndex >= objects.size()) return;
        PaintObject &obj = objects[activeObjIndex];
        QPointF delta = canvasPos - objDragStart;
        obj.bounds = objBoundsStart.translated(delta);
    }

    void rotateActiveObject(const QPointF &canvasPos, bool snap15) {
        if (activeObjIndex < 0 || activeObjIndex >= objects.size()) return;
        PaintObject &obj = objects[activeObjIndex];
        QPointF center = obj.bounds.center();
        double angle1 = atan2(objDragStart.y() - center.y(), objDragStart.x() - center.x());
        double angle2 = atan2(canvasPos.y() - center.y(), canvasPos.x() - center.x());
        obj.rotation = objRotationStart + (angle2 - angle1) * 180.0 / M_PI;
        if (snap15) obj.rotation = qRound(obj.rotation / 15.0) * 15.0;
    }

    void scaleActiveObject(const QPointF &canvasPos, bool keepAspect) {
        if (activeObjIndex < 0 || activeObjIndex >= objects.size()) return;
        PaintObject &obj = objects[activeObjIndex];
        double w0 = objBoundsStart.width() * objScaleStartX;
        double h0 = objBoundsStart.height() * objScaleStartY;
        QRectF visual0(objBoundsStart.center().x() - w0 / 2.0,
                       objBoundsStart.center().y() - h0 / 2.0, w0, h0);
        QRectF nv = resizeRotatedRect(objActiveHandle, visual0, obj.rotation,
                                      objDragStart, canvasPos, keepAspect, 5.0);
        obj.scaleX = qBound(0.05, nv.width() / qMax(0.001, objBoundsStart.width()), 50.0);
        obj.scaleY = qBound(0.05, nv.height() / qMax(0.001, objBoundsStart.height()), 50.0);
        QPointF nc = nv.center();
        obj.bounds = QRectF(nc.x() - objBoundsStart.width() / 2.0,
                            nc.y() - objBoundsStart.height() / 2.0,
                            objBoundsStart.width(), objBoundsStart.height());
    }

    void stopObjectManipulation() {
        objDragging = false;
        objRotating = false;
        objScaling = false;
        objActiveHandle = ObjectHandle::None;
    }

    // ==================== OBJETOS: INFO PARA EDICIÓN ====================

    PaintObject takeObjectAt(int idx) {
        PaintObject obj = objects[idx];
        objects.removeAt(idx);
        if (activeObjIndex >= objects.size()) activeObjIndex = -1;
        return obj;
    }

    // ==================== CLIPBOARD ====================

    void setClipboardBuffer(const QImage &img) { clipboardBuffer = img; }
    QImage clipboardBufferValue() const { return clipboardBuffer; }
    bool hasClipboardBuffer() const { return !clipboardBuffer.isNull(); }

    bool copySelectionToClipboard() {
        QList<int> selected = selectedObjectIndices();
        if (!selected.isEmpty()) {
            QImage rendered = renderSelectedObjects();
            if (!rendered.isNull()) {
                clipboardBuffer = rendered;
                QApplication::clipboard()->setImage(rendered);
                return true;
            }
        }
        if (active && !selBuffer.isNull()) {
            clipboardBuffer = selBuffer;
            QApplication::clipboard()->setImage(selBuffer);
            return true;
        }
        return false;
    }

    bool cutSelectionToClipboard() {
        QList<int> selected = selectedObjectIndices();
        if (!selected.isEmpty()) {
            QImage rendered = renderSelectedObjects();
            if (!rendered.isNull()) {
                clipboardBuffer = rendered;
                QApplication::clipboard()->setImage(rendered);
                deleteSelectedObjects();
                return true;
            }
        }
        if (active && !selBuffer.isNull()) {
            clipboardBuffer = selBuffer;
            QApplication::clipboard()->setImage(selBuffer);
            discard();
            return true;
        }
        return false;
    }

    QImage getImageToPaste() const {
        QImage sysImage = QApplication::clipboard()->image();
        if (!sysImage.isNull()) return sysImage.convertToFormat(QImage::Format_ARGB32);
        if (!clipboardBuffer.isNull()) return clipboardBuffer;
        return QImage();
    }

    // ==================== DIBUJO ====================

    void drawSelectionOverlay(QPainter &painter, double zoom) const {
        if (!active || selBuffer.isNull()) return;
        painter.save();
        painter.translate(selRect.center());
        painter.rotate(selRotation);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.drawImage(QRect(-selRect.width() / 2, -selRect.height() / 2,
                                selRect.width(), selRect.height()), selBuffer);
        painter.restore();
    }

    void drawSelectionGizmo(QPainter &painter, double zoom, bool darkMode,
                            const QColor &selBlue, const QColor &selBlueLight) const {
        if (!active || selBuffer.isNull()) return;
        GizmoLayout L = gizmoLayout(zoom);
        drawGizmoBase(painter, L, zoom, darkMode);

        QPointF convPos = convertHandlePos(zoom);
        painter.setPen(QPen(selBlue, 1.0 / zoom, Qt::SolidLine));
        painter.drawLine(L.rotatePos, convPos);

        double convSize = 20.0 / zoom;
        QRectF convRect(convPos.x() - convSize / 2, convPos.y() - convSize / 2, convSize, convSize);
        painter.setPen(QPen(selBlue, 1.0 / zoom));
        painter.setBrush(selBlueLight);
        painter.drawRoundedRect(convRect, 4.0 / zoom, 4.0 / zoom);

        painter.setPen(Qt::white);
        QFont fO;
        fO.setPixelSize(qMax(8, (int)(10.0 / zoom)));
        fO.setBold(true);
        painter.setFont(fO);
        painter.drawText(convRect, Qt::AlignCenter, "O");
    }

    void drawObjectGizmos(QPainter &painter, double zoom, bool darkMode) const {
        for (int i = 0; i < objects.size(); ++i)
            if (objects[i].selected)
                drawObjectGizmo(painter, objects[i], zoom, darkMode);
    }

    // ==================== LIMPIEZA TOTAL ====================

    void clearAll() {
        discard();
        clearObjects();
        clipboardBuffer = QImage();
    }
};

#endif // SELECTTOOLS_H
