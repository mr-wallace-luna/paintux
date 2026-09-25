#ifndef LASSO_TOOLS_H
#define LASSO_TOOLS_H

#include <QImage>
#include <QPainter>
#include <QPainterPath>

// PROCESAMIENTO DE LOS LAZOS 
namespace LassoProcessor {
    // Lazo Extraer: Mantiene lo de adentro del trazo y borra lo de afuera
    inline QImage applyLassoExtract(const QImage &source, const QPainterPath &path, bool isPixelArt) {
        QImage result(source.size(), QImage::Format_ARGB32);
        result.fill(Qt::transparent); // Fondo completamente transparente
        QPainter painter(&result);
        painter.setRenderHint(QPainter::Antialiasing, !isPixelArt);
        painter.setClipPath(path); // Limitar a la forma dibujada
        painter.drawImage(0, 0, source); // Pintar imagen original adentro del lazo
        painter.end();
        return result;
    }

    // Lazo Hueco: Borra lo de adentro del trazo y mantiene lo de afuera
    inline void applyLassoDelete(QImage &source, const QPainterPath &path, bool isPixelArt) {
        QPainter painter(&source);
        painter.setRenderHint(QPainter::Antialiasing, !isPixelArt);
        painter.setCompositionMode(QPainter::CompositionMode_Clear); // Modo borrado total
        painter.setClipPath(path); // Limitar borrado a la forma
        painter.fillRect(source.rect(), Qt::transparent); // Rellenar de vacío
        painter.end();
    }
}
//nota meter lazo inteligente opciones de lazo por vectores y alzo inteligente de selecion automatica a veriones futuras 

#endif // LASSO_TOOLS_H