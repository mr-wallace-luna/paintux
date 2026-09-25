#ifndef BEZIER_PATH_TOOL_H
#define BEZIER_PATH_TOOL_H

#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QPainterPath>
#include <QVector>
#include <QLineF>
#include <cmath>
#include <functional>

// ============================================================
// BezierPathTool
//
// Encapsula COMPLETAMENTE la lógica de la "Pluma Bezier":
//   - Nodos del path activo (puntos)
//   - Estado de cierre
//   - Hover y snap al primer punto
//   - Construcción del QPainterPath
//   - Render del preview (líneas guía, nodos, cierre)
//
// No depende de PaintArea ni de LayerStack.
// El consumidor (PaintArea) le pasa eventos y consulta el path
// resultante cuando termina.
//
// Uso:
//   bezierTool.click(pos, zoomFactor, rightButton) → bool (completed?)
//   bezierTool.move(pos, zoomFactor)
//   bezierTool.paint(painter, zoomFactor)
//   QPainterPath p = bezierTool.takeCompletedPath();
// ============================================================
class BezierPathTool
{
public:
    BezierPathTool() = default;

    // ------------------------------------------------------------
    // Estado
    // ------------------------------------------------------------
    bool isEmpty() const { return m_nodes.isEmpty(); }
    bool isClosed() const { return m_closed; }
    int  nodeCount() const { return m_nodes.size(); }
    const QVector<QPoint> &nodes() const { return m_nodes; }
    const QRect &lastBounds() const { return m_lastBounds; }

    // ------------------------------------------------------------
    // Ciclo de vida
    // ------------------------------------------------------------
    void reset() {
        m_nodes.clear();
        m_closed = false;
        m_hoverPos = QPoint();
        m_lastBounds = QRect();
        m_hasCompletion = false;
        m_completedPath = QPainterPath();
    }

    // ------------------------------------------------------------
    // Interacción
    // Devuelve true si el path se completó y hay un QPainterPath
    // disponible en takeCompletedPath().
    // ------------------------------------------------------------
    bool click(const QPoint &canvasPos, double zoomFactor, bool rightButton)
    {
        if (rightButton) {
            return finalizeIfValid();
        }

        // Cerrar si clic cerca del primer nodo (y hay al menos 3)
        if (m_nodes.size() >= 3) {
            const double threshold = 12.0 / qMax(0.0001, zoomFactor);
            const QPointF a(canvasPos);
            const QPointF b(m_nodes.first());
            if (QLineF(a, b).length() <= threshold) {
                m_closed = true;
                return finalizeIfValid();
            }
        }

        m_nodes.append(canvasPos);
        return false;
    }

    void move(const QPoint &canvasPos, double zoomFactor)
    {
        Q_UNUSED(zoomFactor);
        m_hoverPos = canvasPos;
    }

    // Fuerza el cierre y finaliza (Return / Enter)
    bool finalize()
    {
        if (m_nodes.size() < 3) { reset(); return false; }
        m_closed = true;
        return finalizeIfValid();
    }

    // Cancela sin producir path
    void cancel() { reset(); }

    // ------------------------------------------------------------
    // Entrega del path completado (consume)
    // ------------------------------------------------------------
    QPainterPath takeCompletedPath()
    {
        QPainterPath p = m_completedPath;
        m_completedPath = QPainterPath();
        m_hasCompletion = false;
        m_nodes.clear();
        m_closed = false;
        return p;
    }

    bool hasCompletion() const { return m_hasCompletion; }

    // ------------------------------------------------------------
    // Render del preview
    // ------------------------------------------------------------
    void paint(QPainter &painter, double zoomFactor) const
    {
        if (m_nodes.isEmpty()) return;

        const double z = qMax(0.0001, zoomFactor);
        const double nodeR = 4.0 / z;
        const double closeR = 7.0 / z;
        const double lineW = 2.0 / z;
        const double dashW = 1.5 / z;

        // Path principal
        QPainterPath path;
        path.moveTo(m_nodes.first());
        for (int i = 1; i < m_nodes.size(); ++i) path.lineTo(m_nodes[i]);

        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);

        // Línea principal
        painter.setPen(QPen(QColor(0, 150, 255), lineW));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);

        // Línea "fantasma" hacia el cursor
        if (!m_closed && !m_nodes.isEmpty()) {
            painter.setPen(QPen(QColor(0, 150, 255, 120), lineW, Qt::DashLine));
            painter.drawLine(m_nodes.last(), m_hoverPos);
        }

        // Snap al primer punto: resaltar cierre
        if (!m_closed && m_nodes.size() >= 3) {
            const double threshold = 12.0 / z;
            QLineF lf(QPointF(m_hoverPos), QPointF(m_nodes.first()));
            if (lf.length() <= threshold) {
                painter.setPen(QPen(QColor(0, 200, 100), lineW * 1.25));
                painter.setBrush(Qt::NoBrush);
                painter.drawEllipse(m_nodes.first(), closeR, closeR);
            }
        }

        // Nodos
        painter.setPen(QPen(QColor(0, 150, 255), dashW));
        painter.setBrush(Qt::white);
        for (const QPoint &p : m_nodes) {
            painter.drawEllipse(QPointF(p), nodeR, nodeR);
        }

        painter.restore();
    }

    // ------------------------------------------------------------
    // Bounds para invalidar zona de repintado
    // ------------------------------------------------------------
    QRect boundsForRepaint() const
    {
        if (m_nodes.isEmpty()) return QRect();
        QRect r(m_nodes.first(), m_nodes.first());
        for (const QPoint &p : m_nodes) {
            r = r.united(QRect(p, p));
        }
        r = r.united(QRect(m_hoverPos, m_hoverPos));
        return r.adjusted(-20, -20, 20, 20);
    }

private:
    QVector<QPoint> m_nodes;
    bool m_closed = false;
    QPoint m_hoverPos;

    QPainterPath m_completedPath;
    bool m_hasCompletion = false;
    QRect m_lastBounds;

    bool finalizeIfValid()
    {
        if (m_nodes.size() < 3) { reset(); return false; }

        QPainterPath path;
        path.moveTo(m_nodes.first());
        for (int i = 1; i < m_nodes.size(); ++i) path.lineTo(m_nodes[i]);
        if (m_closed) path.closeSubpath();

        m_completedPath = path;
        m_hasCompletion = true;
        m_lastBounds = path.boundingRect().toAlignedRect();
        return true;
    }
};

#endif // BEZIER_PATH_TOOL_H
