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

#include "core/CustomBrushes.h"

// ============================================================
// MaskEditController
//
// Encapsula COMPLETAMENTE la edición de máscaras B/N:
//   - Pincel (stamp + segmentos interpolados)
//   - Pluma Bezier para máscara (nodos + rasterizado)
//   - Repintado incremental vía callback
//
// No depende de PaintArea ni de LayerStack. Todo lo que
// necesita del exterior se inyecta vía Context (callbacks).
//
// Flujo típico desde PaintArea:
//   m_maskEdit.setContext({...});
//   m_maskEdit.onRepaint = [this](const QRect &r){ update(...); };
//   m_maskEdit.onMaskChanged = [this](){ emit layersChanged(); };
//
//   // mousePress:
//   if (stack.isEditingMask()) {
//       if (currentTool == ToolPenBezier)
//           m_maskEdit.beginBezierClick(pos, zoomFactor, rightBtn);
//       else if (herramientaDePintura())
//           m_maskEdit.beginStroke(pos);
//   }
//
//   // mouseMove:
//   m_maskEdit.continueStroke(lastPoint, pos);
//   m_maskEdit.moveBezierNode(pos);
//
//   // mouseRelease:
//   m_maskEdit.endStroke();
//   m_maskEdit.endBezierDrag();
//
//   // paintEvent:
//   m_maskEdit.paintBezierOverlay(painter, zoomFactor);
// ============================================================
class MaskEditController
{
public:
    // ------------------------------------------------------------
    // Contexto inyectado desde fuera (no posee nada)
    // ------------------------------------------------------------
    struct Context {
        // Devuelve referencia a la QImage de la máscara activa.
        // Si no hay máscara activa, debe devolver una imagen nula
        // (y isEditingMask() debe devolver false).
        std::function<QImage&()> maskRef;

        // ¿Estamos en modo edición de máscara?
        std::function<bool()> isEditingMask;

        // Índice de la capa cuya máscara se está editando (-1 si ninguna)
        std::function<int()> maskEditLayer;

        // Color de trabajo según el botón (izq/der)
        std::function<QColor(Qt::MouseButton)> brushColorFor;

        // Preset de pincel según la herramienta actual
        std::function<BrushSettings()> brushPresetFor;

        // Modo pixel art activo
        std::function<bool()> isPixelArtMode;
    };

    // ------------------------------------------------------------
    // Callbacks de salida
    // ------------------------------------------------------------
    // Notifica que hay que repintar el canvas en el rect indicado
    // (en coordenadas de canvas, NO de widget).
    std::function<void(const QRect&)> onRepaint;

    // Notifica que el contenido de la máscara cambió (para layersChanged)
    std::function<void()> onMaskChanged;

    // Mensaje para la status bar
    std::function<void(const QString&)> onStatusMessage;

    // ------------------------------------------------------------
    // Ciclo de vida
    // ------------------------------------------------------------
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

    // ------------------------------------------------------------
    // Estado
    // ------------------------------------------------------------
    bool isDrawing() const { return m_drawing; }
    bool isDraggingNode() const { return m_draggingNode; }
    bool hasBezierNodes() const { return !m_bezierNodes.isEmpty(); }
    int  selectedNode() const { return m_selectedNode; }
    int  nodeCount() const { return m_bezierNodes.size(); }

    // ------------------------------------------------------------
    // PINCEL DE MÁSCARA (stamp + segmentos)
    // ------------------------------------------------------------

    // Llamar desde mousePress cuando currentTool es de pintura y
    // estamos en modo edición de máscara.
    // Devuelve true si consumió el evento.
    bool beginStroke(const QPoint &pos)
    {
        if (!isReady()) return false;
        setupBrush();
        stampAt(pos);
        m_drawing = true;
        if (onMaskChanged) onMaskChanged();
        return true;
    }

    // Llamar desde mouseMove mientras m_drawing
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

    // ------------------------------------------------------------
    // PLUMA BEZIER PARA MÁSCARA
    // ------------------------------------------------------------

    // Click en canvas. Devuelve true si consumió el evento.
    // rightButton = true → finaliza (rasteriza si hay >= 3 nodos).
    // Si el click está cerca del primer nodo y hay >= 3 nodos → cierra y rasteriza.
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

    // Arrastre del nodo seleccionado (mouseMove con draggingNode)
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

    // Rasteriza el path actual dentro de la máscara.
    // Devuelve true si se rasterizó algo.
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

    // Borra el nodo seleccionado (Delete/Backspace)
    bool removeSelectedNode()
    {
        if (m_selectedNode < 0 || m_selectedNode >= m_bezierNodes.size()) return false;
        m_bezierNodes.removeAt(m_selectedNode);
        m_selectedNode = -1;
        return true;
    }

    // ------------------------------------------------------------
    // Render del overlay Bezier
    // ------------------------------------------------------------
    void paintBezierOverlay(QPainter &painter, double zoomFactor) const
    {
        if (m_bezierNodes.isEmpty()) return;

        const double z = qMax(0.0001, zoomFactor);
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);

        // Path principal (solo líneas)
        QPainterPath pp;
        pp.moveTo(m_bezierNodes.first());
        for (int i = 1; i < m_bezierNodes.size(); ++i) pp.lineTo(m_bezierNodes[i]);

        painter.setPen(QPen(QColor(255, 255, 255), 1.5 / z, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(pp);

        // Nodos
        painter.setBrush(QColor(255, 200, 0));
        painter.setPen(QPen(QColor(255, 255, 255), 1.0 / z));
        for (const QPointF &pt : m_bezierNodes) {
            painter.drawEllipse(pt, 4.0 / z, 4.0 / z);
        }

        // Cierre (si hay >= 3 nodos, marca el primero)
        if (m_bezierNodes.size() >= 3) {
            painter.setPen(QPen(QColor(0, 255, 0), 2.0 / z));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(m_bezierNodes.first(), 7.0 / z, 7.0 / z);
        }

        painter.restore();
    }

    // ------------------------------------------------------------
    // Bounds para repintar la zona del overlay
    // ------------------------------------------------------------
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
    // ------------------------------------------------------------
    // Estado interno
    // ------------------------------------------------------------
    Context m_ctx;
    bool m_drawing = false;
    Qt::MouseButton m_activeButton = Qt::LeftButton;
    int m_penWidth = 3;
    double m_sensitivity = 1.0;
    ToolType m_currentTool = ToolPencil;

    QImage m_brushStamp;
    BrushSettings m_brushConfig;
    QColor m_brushColor;

    QVector<QPointF> m_bezierNodes;
    int m_selectedNode = -1;
    bool m_draggingNode = false;

    // ------------------------------------------------------------
    // Helpers internos
    // ------------------------------------------------------------
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
        // La goma pinta siempre blanco (revela); el resto usa luminancia del color
        if (m_currentTool == ToolEraser) {
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

#endif // MASK_EDIT_CONTROLLER_H
