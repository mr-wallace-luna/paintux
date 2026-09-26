#ifndef TILESYSTEM_H
#define TILESYSTEM_H

#include <QPixmap>
#include <QRect>
#include <QVector>

struct Tile {
    QPixmap pixmap;   
    bool dirty = true; 
};

class TilCach {
public:
static const int TILE_SIZE = 256;

    TilCach() = default;

    void init(int w, int h) {
        
        if (w == m_w && m_h == h && !m_tiles.isEmpty()) return;
        m_w = w; m_h = h;
        m_cols = (w + TILE_SIZE - 1) / TILE_SIZE;
        m_rows = (h + TILE_SIZE - 1) / TILE_SIZE;
        m_tiles.clear();
        m_tiles.resize(m_cols * m_rows);
        markAllDirty();

    }

    int width()  const { return m_w; }
    int height() const { return m_h; }
    int cols()   const { return m_cols; }
    int rows()   const { return m_rows; }
    bool isValid() const { return m_w > 0 && m_h > 0 && !m_tiles.isEmpty(); }

    void markAllDirty() {
        for (Tile &t : m_tiles) t.dirty = true;
    }

    
    void markDirty(const QRect &region) {

        if (!isValid() || region.isEmpty()) return;
        QRect r = region.intersected(QRect(0, 0, m_w, m_h));
        if (r.isEmpty()) return;
        int c0 = r.left()   / TILE_SIZE;
        int c1 = r.right()  / TILE_SIZE;
        int r0 = r.top()    / TILE_SIZE;
        int r1 = r.bottom() / TILE_SIZE;
        for (int ry = r0; ry <= r1; ++ry) {
            int base = ry * m_cols;
            for (int cx = c0; cx <= c1; ++cx) {
                int idx = base + cx;
                if (idx >= 0 && idx < m_tiles.size()) m_tiles[idx].dirty = true;
            }
        }
    }

    bool isDirty(int idx) const { return idx >= 0 && idx < m_tiles.size() && m_tiles[idx].dirty; }

    void setPixmap(int idx, const QPixmap &pm) {
        if (idx >= 0 && idx < m_tiles.size()) { m_tiles[idx].pixmap = pm; m_tiles[idx].dirty = false; }
    }

    QPixmap pixmap(int idx) const {
        return (idx >= 0 && idx < m_tiles.size()) ? m_tiles[idx].pixmap : QPixmap();
    }

    QRect tileRect(int idx) const {
        int cx = idx % m_cols;
        int ry = idx / m_cols;
        QRect r(cx * TILE_SIZE, ry * TILE_SIZE, TILE_SIZE, TILE_SIZE);
        return r.intersected(QRect(0, 0, m_w, m_h));
    }

    
    QVector<int> tilesIntersecting(const QRect &region) const {
        QVector<int> out;
        if (!isValid() || region.isEmpty()) return out;
        QRect r = region.intersected(QRect(0, 0, m_w, m_h));
        if (r.isEmpty()) return out;
        int c0 = r.left()   / TILE_SIZE;
        int c1 = r.right()  / TILE_SIZE;
        int r0 = r.top()    / TILE_SIZE;
        int r1 = r.bottom() / TILE_SIZE;
        out.reserve((c1 - c0 + 1) * (r1 - r0 + 1));
        for (int ry = r0; ry <= r1; ++ry) {
            int base = ry * m_cols;
            for (int cx = c0; cx <= c1; ++cx) out.append(base + cx);
        }
        return out;
    }

    
    int dirtyCount() const {
        int n = 0;
        for (const Tile &t : m_tiles) if (t.dirty) ++n;
        return n;
    }

   
    void releasePixmaps() {
        for (Tile &t : m_tiles) { t.pixmap = QPixmap(); t.dirty = true; }
    }

private:
    int m_w = 0, m_h = 0;
    int m_cols = 0, m_rows = 0;
    QVector<Tile> m_tiles;
};

#endif

