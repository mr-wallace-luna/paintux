#ifndef TEXT_ENGINE_H
#define TEXT_ENGINE_H

#include <QtGlobal>
#include <QRect>
#include <QPoint>
#include <QPointF>
#include <QFont>
#include <QFontMetrics>
#include <QColor>
#include <QString>
#include <QTextLayout>
#include <QTextLine>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QCursor>
#include <QTimer>
#include <QObject>
#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QColorDialog>
#include <QFontDatabase>
#include <QStandardItemModel>
#include <QAbstractItemView>
#include <algorithm>

namespace TextEngine {

// ------------------------------------------------------------
// Handles del marco de texto
// ------------------------------------------------------------
enum class Handle {
    None = 0,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    Top,
    Bottom,
    Left,
    Right,
    Body
};

// ------------------------------------------------------------
// Estilo visual adaptativo (dark/light)
// ------------------------------------------------------------
struct Style {
    QColor accent;
    QColor accentLight;
    QColor fill;
    QColor textColor;
    QColor cursorColor;
    QColor handleBorder;
    QColor handleFill;
    int handleSize = 8;
    bool darkMode = false;

    static Style forDarkMode() {
        Style s;
        s.darkMode = true;
        s.accent = QColor("#60a5fa");
        s.accentLight = QColor("#93c5fd");
        s.fill = QColor(255, 255, 255, 18);
        s.textColor = QColor("#e5e5e5");
        s.cursorColor = QColor("#e5e5e5");
        s.handleBorder = QColor("#60a5fa");
        s.handleFill = QColor("#93c5fd");
        return s;
    }

    static Style forLightMode() {
        Style s;
        s.darkMode = false;
        s.accent = QColor("#2563eb");
        s.accentLight = QColor("#3b82f6");
        s.fill = QColor(0, 0, 0, 12);
        s.textColor = QColor("#111827");
        s.cursorColor = QColor("#111827");
        s.handleBorder = QColor("#2563eb");
        s.handleFill = QColor("#3b82f6");
        return s;
    }
};

inline QRect handleRect(const QRect &r, Handle h, int size)
{
    if (size <= 0) return QRect();
    const int half = size / 2;
    switch (h) {
    case Handle::TopLeft:     return QRect(r.left() - half,  r.top() - half,    size, size);
    case Handle::TopRight:    return QRect(r.right() - half, r.top() - half,    size, size);
    case Handle::BottomLeft:  return QRect(r.left() - half,  r.bottom() - half, size, size);
    case Handle::BottomRight: return QRect(r.right() - half, r.bottom() - half, size, size);
    case Handle::Top:         return QRect(r.left() + r.width()/2 - half, r.top() - half,    size, size);
    case Handle::Bottom:      return QRect(r.left() + r.width()/2 - half, r.bottom() - half, size, size);
    case Handle::Left:        return QRect(r.left() - half, r.top() + r.height()/2 - half, size, size);
    case Handle::Right:       return QRect(r.right() - half, r.top() + r.height()/2 - half, size, size);
    default: return QRect();
    }
}

inline Handle hitHandle(const QRect &r, const QPoint &pos, int size)
{
    static const Handle handles[] = {
        Handle::TopLeft, Handle::TopRight, Handle::BottomLeft, Handle::BottomRight,
        Handle::Top, Handle::Bottom, Handle::Left, Handle::Right
    };
    for (Handle h : handles) {
        if (handleRect(r, h, size).contains(pos)) return h;
    }
    if (r.contains(pos)) return Handle::Body;
    return Handle::None;
}

inline Qt::CursorShape cursorForHandle(Handle h)
{
    switch (h) {
    case Handle::TopLeft:
    case Handle::BottomRight: return Qt::SizeFDiagCursor;
    case Handle::TopRight:
    case Handle::BottomLeft:  return Qt::SizeBDiagCursor;
    case Handle::Top:
    case Handle::Bottom:      return Qt::SizeVerCursor;
    case Handle::Left:
    case Handle::Right:       return Qt::SizeHorCursor;
    case Handle::Body:        return Qt::IBeamCursor;
    default:                  return Qt::CrossCursor;
    }
}

struct TextData {
    QRect bounds;
    QString text;
    QFont font;
    QColor color;
};

// ------------------------------------------------------------
// EditSession: encapsula TODO el ciclo de vida del marco de
// texto (datos + cursor + drag + resize + pintado).
// ------------------------------------------------------------
class EditSession : public QObject
{
    Q_OBJECT

public:
    explicit EditSession(QObject *parent = nullptr) : QObject(parent) {
        cursorTimer = new QTimer(this);
        cursorTimer->setInterval(500);
        connect(cursorTimer, &QTimer::timeout, this, [this]() {
            if (active) { cursorVisible = !cursorVisible; emit needsRepaint(); }
        });
    }

    // --- Estado público ---
    bool active = false;
    QRect rect;
    QString text;
    int cursor = 0;
    QFont font;
    QColor color;
    bool cursorVisible = true;
    int handleSize = 8;

    // --- Interacción ---
    bool dragging = false;
    bool resizing = false;
    Handle resizeHandle = Handle::None;
    QPoint dragOffset;

    // ------------------------------------------------------------
    // Ciclo de vida
    // ------------------------------------------------------------
    void beginNew(const QRect &r, const QFont &f, const QColor &c) {
        active = true;
        rect = r;
        text.clear();
        cursor = 0;
        font = f;
        color = c;
        cursorVisible = true;
        dragging = false;
        resizing = false;
        resizeHandle = Handle::None;
        dragOffset = QPoint(0, 0);
        cursorTimer->start();
    }

    void loadExisting(const QRect &r, const QString &t, const QFont &f, const QColor &c) {
        active = true;
        rect = r;
        text = t;
        cursor = t.length();
        font = f;
        color = c;
        cursorVisible = true;
        dragging = false;
        resizing = false;
        resizeHandle = Handle::None;
        dragOffset = QPoint(0, 0);
        cursorTimer->start();
    }

    void end() {
        active = false;
        text.clear();
        cursor = 0;
        cursorVisible = true;
        dragging = false;
        resizing = false;
        resizeHandle = Handle::None;
        dragOffset = QPoint(0, 0);
        cursorTimer->stop();
    }

    void cancel() { end(); }
    bool isEmpty() const { return text.isEmpty(); }

    TextData data() const {
        TextData d;
        d.bounds = rect;
        d.text = text;
        d.font = font;
        d.color = color;
        return d;
    }

    void applyFormat(const QFont &f, const QColor &c) {
        font = f;
        color = c;
        emit needsRepaint();
    }

    // ------------------------------------------------------------
    // Edición
    // ------------------------------------------------------------
    void insert(const QString &s) {
        if (!active || s.isEmpty()) return;
        text.insert(cursor, s);
        cursor += s.length();
        cursorVisible = true;
        emit needsRepaint();
    }
    void insertNewline() { insert(QStringLiteral("\n")); }
    void backspace() {
        if (!active || cursor <= 0) return;
        --cursor;
        text.remove(cursor, 1);
        cursorVisible = true;
        emit needsRepaint();
    }
    void deleteChar() {
        if (!active || cursor >= text.length()) return;
        text.remove(cursor, 1);
        cursorVisible = true;
        emit needsRepaint();
    }
    void moveLeft()    { if (active && cursor > 0) { --cursor; cursorVisible = true; emit needsRepaint(); } }
    void moveRight()   { if (active && cursor < text.length()) { ++cursor; cursorVisible = true; emit needsRepaint(); } }
    void moveHome()    { if (active) { cursor = 0; cursorVisible = true; emit needsRepaint(); } }
    void moveEnd()     { if (active) { cursor = text.length(); cursorVisible = true; emit needsRepaint(); } }
    void toggleCursor(){ cursorVisible = !cursorVisible; }

    /// Hit testing
    Handle hitHandleAt(const QPoint &canvasPos) const {
        if (!active) return Handle::None;
        return hitHandle(rect, canvasPos, handleSize);
    }
    bool contains(const QPoint &canvasPos) const { return active && rect.contains(canvasPos); }

    // ------------------------------------------------------------
    // Drag
    void startDrag(const QPoint &canvasPos) {
        if (!active) return;
        dragging = true;
        resizing = false;
        resizeHandle = Handle::None;
        dragOffset = canvasPos - rect.topLeft();
    }
    void dragTo(const QPoint &canvasPos) {
        if (!active || !dragging) return;
        rect.moveTo(canvasPos - dragOffset);
        emit needsRepaint();
    }
    void endDrag() { dragging = false; }

    // ------------------------------------------------------------
    // Resize
    // ------------------------------------------------------------
    void startResize(Handle h) {
        if (!active || h == Handle::None || h == Handle::Body) return;
        resizing = true;
        resizeHandle = h;
        dragging = false;
    }
    void resizeTo(const QPoint &canvasPos, int minW = 20, int minH = 20) {
        if (!active || !resizing) return;
        QRect r = rect;
        switch (resizeHandle) {
        case Handle::TopLeft:     r.setTopLeft(canvasPos); break;
        case Handle::TopRight:    r.setTopRight(canvasPos); break;
        case Handle::BottomLeft:  r.setBottomLeft(canvasPos); break;
        case Handle::BottomRight: r.setBottomRight(canvasPos); break;
        case Handle::Top:         r.setTop(canvasPos.y()); break;
        case Handle::Bottom:      r.setBottom(canvasPos.y()); break;
        case Handle::Left:        r.setLeft(canvasPos.x()); break;
        case Handle::Right:       r.setRight(canvasPos.x()); break;
        default: return;
        }
        if (r.width() >= minW && r.height() >= minH) {
            rect = r.normalized();
            emit needsRepaint();
        }
    }
    void endResize() { resizing = false; resizeHandle = Handle::None; }

    // ------------------------------------------------------------
    // Pintado COMPLETO del marco (autocontenido)
    // ------------------------------------------------------------
    void paint(QPainter &painter, double zoomFactor, const Style &style) const {
        if (!active) return;

        const double z = qMax(0.0001, zoomFactor);
        painter.save();
        painter.setRenderHint(QPainter::TextAntialiasing, true);

        // Fondo suave
        painter.fillRect(rect, style.fill);

        // Borde punteado
        painter.setPen(QPen(style.accent, 1.5 / z, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(rect);

        // Texto
        if (!text.isEmpty()) {
            QTextLayout layout(text, font);
            prepareLayout(layout);
            painter.save();
            painter.translate(rect.topLeft());
            painter.setPen(color);
            layout.draw(&painter, QPointF(0, 0));
            painter.restore();

            if (cursorVisible) {
                QRectF cr = cursorRectFromLayout(layout);
                painter.setPen(QPen(color, 1.5 / z));
                painter.drawLine(cr.topLeft(), cr.bottomLeft());
            }
        } else if (cursorVisible) {
            QFontMetrics fm(font);
            QPointF p(rect.left() + 2, rect.top() + 2);
            painter.setPen(QPen(color, 1.5 / z));
            painter.drawLine(p, QPointF(p.x(), p.y() + fm.height()));
        }

        // Handles
        painter.setPen(QPen(style.handleBorder, 1.0 / z));
        painter.setBrush(style.handleFill);
        static const Handle handles[] = {
            Handle::TopLeft, Handle::TopRight, Handle::BottomLeft, Handle::BottomRight,
            Handle::Top, Handle::Bottom, Handle::Left, Handle::Right
        };
        for (Handle h : handles)
            painter.drawRect(handleRect(rect, h, handleSize));

        painter.restore();
    }

signals:
    void needsRepaint();

private:
    QTimer *cursorTimer = nullptr;

    void prepareLayout(QTextLayout &layout) const {
        layout.beginLayout();
        qreal y = 0;
        const qreal lineWidth = qMax<qreal>(1.0, qreal(rect.width()) - 4.0);
        while (true) {
            QTextLine line = layout.createLine();
            if (!line.isValid()) break;
            line.setLineWidth(lineWidth);
            line.setPosition(QPointF(2, 2 + y));
            y += line.height();
        }
        layout.endLayout();
    }

    QRectF cursorRectFromLayout(QTextLayout &layout) const {
        QTextLine line = layout.lineForTextPosition(cursor);
        if (line.isValid()) {
            QPointF p = line.position();
            qreal x = rect.left() + p.x() + line.cursorToX(cursor);
            qreal y = rect.top() + p.y();
            return QRectF(x, y, 1.0, line.height());
        }
        QFontMetrics fm(font);
        return QRectF(rect.left() + 2, rect.top() + 2, 1.0, fm.height());
    }
};

// ============================================================
// FormatBar: barra flotante para editar el formato del texto.
// Antes vivía en paintarea.cpp como "TextFmtBar".
// Se mueve aquí para que el módulo TextEngine sea autocontenido.
// ============================================================
class FormatBar : public QWidget
{
    Q_OBJECT

public:
    explicit FormatBar(QWidget *parent = nullptr)
        : QWidget(parent, Qt::Window | Qt::WindowStaysOnTopHint | Qt::Tool)
    {
        setWindowTitle(QObject::tr("Formato de Texto"));
        setFixedSize(340, 120);
        currentColor = Qt::black;

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(10, 8, 10, 8);
        mainLayout->setSpacing(6);

        // --- Fila de fuente ---
        QHBoxLayout *fontLayout = new QHBoxLayout();
        fontLayout->addWidget(new QLabel(QObject::tr("Fuente:")));
        fontCombo = new QComboBox();

        QStringList families = QFontDatabase::families();
        families.removeDuplicates();
        families.sort();

        QStandardItemModel *model = new QStandardItemModel(this);
        for (const QString &f : families) {
            QStandardItem *item = new QStandardItem(f);
            QFont itemFont(f);
            itemFont.setPointSize(12);
            item->setFont(itemFont);
            model->appendRow(item);
        }
        fontCombo->setModel(model);

        int defaultIndex = families.indexOf("Adwaita Sans");
        if (defaultIndex == -1) defaultIndex = families.indexOf("Arial");
        if (defaultIndex == -1) defaultIndex = families.indexOf("Sans Serif");
        if (defaultIndex == -1) defaultIndex = 0;
        fontCombo->setCurrentIndex(defaultIndex);

        fontCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        fontCombo->setMinimumHeight(28);
        fontCombo->view()->setMinimumWidth(280);
        fontLayout->addWidget(fontCombo);
        mainLayout->addLayout(fontLayout);

        // --- Fila de estilo (tamaño, B, I) ---
        QHBoxLayout *styleLayout = new QHBoxLayout();
        styleLayout->addWidget(new QLabel(QObject::tr("Tamano:")));
        sizeSpin = new QSpinBox();
        sizeSpin->setRange(6, 400);
        sizeSpin->setValue(24);
        sizeSpin->setFixedWidth(70);
        styleLayout->addWidget(sizeSpin);

        boldCheck = new QCheckBox(QObject::tr("B"));
        boldCheck->setFont(QFont(fontCombo->currentText(), 10, QFont::Bold));
        boldCheck->setFixedWidth(32);
        italicCheck = new QCheckBox(QObject::tr("I"));
        QFont italicFont = italicCheck->font();
        italicFont.setItalic(true);
        italicCheck->setFont(italicFont);
        italicCheck->setFixedWidth(32);
        styleLayout->addWidget(boldCheck);
        styleLayout->addWidget(italicCheck);
        styleLayout->addStretch();
        mainLayout->addLayout(styleLayout);

        // --- Fila de color + aplicar/cancelar ---
        QHBoxLayout *colorLayout = new QHBoxLayout();
        colorLayout->addWidget(new QLabel(QObject::tr("Color:")));
        btnColor = new QPushButton();
        btnColor->setFixedSize(60, 26);
        btnColor->setStyleSheet("background-color: black; border: 1px solid #555; border-radius: 4px;");
        colorLayout->addWidget(btnColor);
        colorLayout->addStretch();

        btnApply = new QPushButton(QObject::tr("Aplicar"));
        btnApply->setStyleSheet("background-color: #2563eb; color: white; font-weight: bold; padding: 4px 10px; border-radius: 4px;");
        btnCancel = new QPushButton(QObject::tr("Cancelar"));
        colorLayout->addWidget(btnApply);
        colorLayout->addWidget(btnCancel);
        mainLayout->addLayout(colorLayout);

        // --- Conexiones ---
        connect(btnColor, &QPushButton::clicked, this, [this]() {
            QColor c = QColorDialog::getColor(currentColor, this, QObject::tr("Color del texto"));
            if (c.isValid()) {
                currentColor = c;
                btnColor->setStyleSheet(QString("background-color: %1; border: 1px solid #555; border-radius: 4px;").arg(c.name()));
                emit formatChanged();
            }
        });
        connect(fontCombo, &QComboBox::currentTextChanged, this, [this]() {
            boldCheck->setFont(QFont(fontCombo->currentText(), 10, QFont::Bold));
            QFont ifont = italicCheck->font();
            ifont.setFamily(fontCombo->currentText());
            italicCheck->setFont(ifont);
            emit formatChanged();
        });
        connect(sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() { emit formatChanged(); });
        connect(boldCheck, &QCheckBox::toggled, this, [this]() { emit formatChanged(); });
        connect(italicCheck, &QCheckBox::toggled, this, [this]() { emit formatChanged(); });
        connect(btnApply, &QPushButton::clicked, this, [this]() { emit applyClicked(); });
        connect(btnCancel, &QPushButton::clicked, this, [this]() { emit cancelClicked(); });
    }

    QColor getColor() const { return currentColor; }

    void setColor(const QColor &c) {
        currentColor = c;
        btnColor->setStyleSheet(QString("background-color: %1; border: 1px solid #555; border-radius: 4px;").arg(c.name()));
    }

    void setFont(const QFont &f) {
        fontCombo->setCurrentText(f.family());
        sizeSpin->setValue(f.pointSize() > 0 ? f.pointSize() : 24);
        boldCheck->setChecked(f.bold());
        italicCheck->setChecked(f.italic());
    }

    QFont getCurrentFont() const {
        QFont f(fontCombo->currentText());
        f.setPointSize(sizeSpin->value());
        f.setBold(boldCheck->isChecked());
        f.setItalic(italicCheck->isChecked());
        return f;
    }

signals:
    void applyClicked();
    void cancelClicked();
    void formatChanged();

private:
    QComboBox *fontCombo = nullptr;
    QSpinBox *sizeSpin = nullptr;
    QCheckBox *boldCheck = nullptr;
    QCheckBox *italicCheck = nullptr;
    QPushButton *btnColor = nullptr;
    QPushButton *btnApply = nullptr;
    QPushButton *btnCancel = nullptr;
    QColor currentColor = Qt::black;
};

} // namespace TextEngine

#endif // TEXT_ENGINE_H