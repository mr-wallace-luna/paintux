#ifndef MASK_EDIT_CONTROLLER_H
#define MASK_EDIT_CONTROLLER_H

#include <QImage>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QColor>
#include <QVector>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QBrush>
#include <QLineF>
#include <QString>
#include <QObject>
#include <functional>
#include <cmath>

#include "core/ToolManager.h"
#include "core/CustomBrushes.h"


class MaskEditController
{
public:
    struct Context {
        std::function<QImage&()> maskRef;
        std::function<bool()> isEditingMask;
        std::function<int()> maskEditLayer;

        std::function<QColor(Qt::MouseButton)> brushColorFor;

        std::function<BrushSettings()> brushPresetFor;

        std::function<bool()> isPixelArtMode;
    };

    std::function<void(const QRect&)> onRepaint;

    std::function<void()> onMaskChanged;

    std::function<void(const QString&)> onStatusMessage;

    MaskEditController() = default;

    void setContext(const Context &ctx) { m_ctx = ctx; }

    void setActiveMouseButton(Qt::MouseButton btn) { m_activeButton = btn; }
    void setPenWidth(int w) { m_penWidth = qMax(1, w); }
    void setMouseSensitivity(double s) { m_sensitivity = qMax(0.01, s); }
    void setCurrentTool(ToolType tool) { m_currentTool = tool; }

    void resetAll()
    {
        m_drawing = false;
        m_brushStamp = QImage();
        m_bezierNodes.clear();
        m_selectedNode = -1;
        m_draggingNode = false;
    }

    void resetBezier()
    {
        m_bezierNodes.clear();
        m_selectedNode = -1;
        m_draggingNode = false;
    }

    bool isDrawing() const { return m_drawing; }
    bool isDraggingNode() const { return m_draggingNode; }
    bool hasBezierNodes() const { return !m_bezierNodes.isEmpty(); }
    int  selectedNode() const { return m_selectedNode; }
    int  nodeCount() const { return m_bezierNodes.size(); }

    bool beginStroke(const QPoint &pos)
    {
        if (!isReady()) return false;
        setupBrush();
        stampAt(pos);
        m_drawing = true;
        if (onMaskChanged) onMaskChanged();
        return true;
    }

    void continueStroke(const QPoint &from, const QPoint &to)
    {
        if (!isReady() || !m_drawing) return;
        if (m_brushStamp.isNull()) setupBrush();
        drawSegment(from, to);
    }

    void endStroke()
    {
        if (!m_drawing) return;
        m_drawing = false;
    }

    bool beginBezierClick(const QPoint &pos, double zoomFactor, bool rightButton)
    {
        if (!isReady()) return false;

        if (rightButton) {
            rasterizeBezier(zoomFactor);
            return true;
        }

        QPointF cp(pos.x(), pos.y());
        if (m_bezierNodes.size() >= 3) {
            const double thr = 12.0 / qMax(0.0001, zoomFactor);
            if (QLineF(cp, m_bezierNodes.first()).length() <= thr) {
                rasterizeBezier(zoomFactor);
                return true;
            }
        }
        m_bezierNodes.append(cp);
        m_selectedNode = m_bezierNodes.size() - 1;
        m_draggingNode = true;
        if (onRepaint) onRepaint(QRect(pos, pos).adjusted(-20, -20, 20, 20));
        return true;
    }

    bool moveBezierNode(const QPoint &pos)
    {
        if (!m_draggingNode) return false;
        if (m_selectedNode < 0 || m_selectedNode >= m_bezierNodes.size()) return false;
        QPointF old = m_bezierNodes[m_selectedNode];
        m_bezierNodes[m_selectedNode] = QPointF(pos.x(), pos.y());
        if (onRepaint) {
            QRect r = QRect(old.toPoint(), pos).normalized();
            onRepaint(r.adjusted(-20, -20, 20, 20));
        }
        return true;
    }

    void endBezierDrag() { m_draggingNode = false; }

    bool rasterizeBezier(double zoomFactor)
    {
        Q_UNUSED(zoomFactor);
        if (!isReady() || m_bezierNodes.size() < 3) {
            resetBezier();
            return false;
        }

        QColor c = m_ctx.brushColorFor ? m_ctx.brushColorFor(Qt::LeftButton) : Qt::white;
        int lum = qGray(c.red(), c.green(), c.blue());

        QImage &mask = m_ctx.maskRef();
        QPainterPath path;
        path.moveTo(m_bezierNodes.first());
        for (int i = 1; i < m_bezierNodes.size(); ++i) path.lineTo(m_bezierNodes[i]);
        path.closeSubpath();

        QPainter p(&mask);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(lum, lum, lum, 255));
        p.drawPath(path);
        p.end();

        QRect r = path.boundingRect().toAlignedRect().adjusted(-4, -4, 4, 4);
        markDirty(r);

        resetBezier();
        if (onMaskChanged) onMaskChanged();
        if (onStatusMessage) {
            onStatusMessage(lum > 127
                ? QObject::tr("Máscara: área REVELADA (blanco)")
                : QObject::tr("Máscara: área OCULTADA (negro)"));
        }
        return true;
    }

    bool cancelBezier()
    {
        if (m_bezierNodes.isEmpty()) return false;
        resetBezier();
        return true;
    }

    bool removeSelectedNode()
    {
        if (m_selectedNode < 0 || m_selectedNode >= m_bezierNodes.size()) return false;
        m_bezierNodes.removeAt(m_selectedNode);
        m_selectedNode = -1;
        return true;
    }

    void paintBezierOverlay(QPainter &painter, double zoomFactor) const
    {
        if (m_bezierNodes.isEmpty()) return;

        const double z = qMax(0.0001, zoomFactor);
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);

        QPainterPath pp;
        pp.moveTo(m_bezierNodes.first());
        for (int i = 1; i < m_bezierNodes.size(); ++i) pp.lineTo(m_bezierNodes[i]);

        painter.setPen(QPen(QColor(255, 255, 255), 1.5 / z, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(pp);

        painter.setBrush(QColor(255, 200, 0));
        painter.setPen(QPen(QColor(255, 255, 255), 1.0 / z));
        for (const QPointF &pt : m_bezierNodes) {
            painter.drawEllipse(pt, 4.0 / z, 4.0 / z);
        }

        if (m_bezierNodes.size() >= 3) {
            painter.setPen(QPen(QColor(0, 255, 0), 2.0 / z));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(m_bezierNodes.first(), 7.0 / z, 7.0 / z);
        }

        painter.restore();
    }

    QRect bezierBoundsForRepaint() const
    {
        if (m_bezierNodes.isEmpty()) return QRect();
        QRect r(m_bezierNodes.first().toPoint(), m_bezierNodes.first().toPoint());
        for (const QPointF &p : m_bezierNodes) {
            r = r.united(QRect(p.toPoint(), p.toPoint()));
        }
        return r.adjusted(-20, -20, 20, 20);
    }

private:
    Context m_ctx;
    bool m_drawing = false;
    Qt::MouseButton m_activeButton = Qt::LeftButton;
    int m_penWidth = 3;
    double m_sensitivity = 1.0;
    ToolType m_currentTool = ToolType::Pencil;

    QImage m_brushStamp;
    BrushSettings m_brushConfig;
    QColor m_brushColor;

    QVector<QPointF> m_bezierNodes;
    int m_selectedNode = -1;
    bool m_draggingNode = false;

    bool isReady() const
    {
        if (!m_ctx.isEditingMask || !m_ctx.maskRef) return false;
        if (!m_ctx.isEditingMask()) return false;
        QImage &m = m_ctx.maskRef();
        return !m.isNull();
    }

    int brushMargin() const
    {
        int base = qMax(5, m_brushConfig.size);
        int compositeSpread = m_brushConfig.shapeElements.isEmpty() ? 0 : base * 2;
        return (int)((base + 12) * 1.25) + compositeSpread + 8;
    }

    void setupBrush()
    {
        int lum;
        if (m_currentTool == ToolType::Eraser) {
            lum = 255;
        } else {
            QColor base = m_ctx.brushColorFor ? m_ctx.brushColorFor(m_activeButton) : Qt::white;
            lum = qGray(base.red(), base.green(), base.blue());
        }
        m_brushColor = QColor(lum, lum, lum, 255);

        if (m_ctx.brushPresetFor) {
            m_brushConfig = m_ctx.brushPresetFor();
        } else {
            m_brushConfig = BrushSettings();
            m_brushConfig.shape = ShapeType::Circle;
            m_brushConfig.dragMode = DragMode::Continuous;
            m_brushConfig.rotationMode = RotationMode::Fixed;
            m_brushConfig.size = qMax(5, (int)(m_penWidth * m_sensitivity * 2));
        }
        m_brushConfig.opacity = 100;
        m_brushConfig.flow = 100;
        m_brushConfig.scatter = 0;
        m_brushConfig.sizeJitter = 0;
        m_brushConfig.angleJitter = 0;
        m_brushConfig.opacityJitter = 0;
        m_brushConfig.wetMix = false;
        m_brushConfig.mixSecondColor = false;
        m_brushConfig.isAirbrush = false;
        m_brushConfig.granulation = false;

        bool pixelArt = m_ctx.isPixelArtMode ? m_ctx.isPixelArtMode() : false;
        m_brushStamp = PaintEngine::generateBrushStamp(m_brushConfig, m_brushColor, 255, pixelArt);
    }

    void stampAt(const QPoint &pos)
    {
        if (!isReady()) return;
        if (m_brushStamp.isNull()) setupBrush();
        QImage &mask = m_ctx.maskRef();
        PaintEngine::applyCustomBrushStroke(mask, pos, m_brushStamp, m_brushConfig,
                                            1.0, 0.0, 1.0, 1.0,
                                            m_brushColor, QColor(), QColor(), 1.0);
        int m = brushMargin();
        markDirty(QRect(pos.x() - m, pos.y() - m, m * 2 + 1, m * 2 + 1));
    }

    void drawSegment(const QPoint &from, const QPoint &to)
    {
        if (!isReady()) return;
        QImage &mask = m_ctx.maskRef();

        const double dx = to.x() - from.x();
        const double dy = to.y() - from.y();
        const double dist = std::sqrt(dx * dx + dy * dy);
        const double spacing = qMax(1.0, m_brushConfig.size * 0.15);
        const int steps = qMax(1, (int)(dist / spacing));

        for (int s = 0; s <= steps; ++s) {
            const double t = (steps == 0) ? 0.0 : (double)s / steps;
            const int cx = (int)(from.x() + t * dx);
            const int cy = (int)(from.y() + t * dy);
            PaintEngine::applyCustomBrushStroke(mask, QPoint(cx, cy),
                                                m_brushStamp, m_brushConfig,
                                                1.0, 0.0, 1.0, 1.0,
                                                m_brushColor, QColor(), QColor(), 1.0);
        }

        int m = brushMargin();
        QRect r = QRect(from, to).normalized().adjusted(-m, -m, m, m);
        markDirty(r);
    }

    void markDirty(const QRect &r)
    {
        if (onRepaint) onRepaint(r);
    }
};

#endif