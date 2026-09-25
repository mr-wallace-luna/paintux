#ifndef RETOUCHTOOLS_H
#define RETOUCHTOOLS_H

#include <QImage>
#include <QPoint>
#include <QColor>
#include <QtGlobal>
#include <cstring>
#include <climits>
#include <vector>
#include <cmath>

class RetouchTools {
public:

static void applyBlur(QImage &image, const QPoint &pos, int radius) {
    if (image.isNull() || radius <= 0) return;
    if (image.format() != QImage::Format_ARGB32 &&
        image.format() != QImage::Format_ARGB32_Premultiplied &&
        image.format() != QImage::Format_RGB32) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }
    const int w = image.width(), h = image.height();
    const int stride = image.bytesPerLine();
    const int r2 = radius * radius;
    const int x0 = qMax(0, pos.x() - radius), x1 = qMin(w - 1, pos.x() + radius);
    const int y0 = qMax(0, pos.y() - radius), y1 = qMin(h - 1, pos.y() + radius);
    if (x0 > x1 || y0 > y1) return;
    const int rw = x1 - x0 + 1, rh = y1 - y0 + 1;
    std::vector<uchar> roi(rw * rh * 4);
    for (int y = 0; y < rh; ++y)
        std::memcpy(&roi[y * rw * 4], image.constScanLine(y0 + y) + x0 * 4, rw * 4);
    uchar *bits = image.bits();
    for (int y = y0; y <= y1; ++y) {
        const int dy = y - pos.y(), dy2 = dy * dy, ly = y - y0;
        uchar *dstRow = bits + y * stride;
        for (int x = x0; x <= x1; ++x) {
            const int dx = x - pos.x();
            if (dx * dx + dy2 > r2) continue;
            int sumB = 0, sumG = 0, sumR = 0, sumA = 0;
            const int lx = x - x0;
            for (int ky = -1; ky <= 1; ++ky) {
                int sy = ly + ky;
                if (sy < 0) sy = 0; else if (sy >= rh) sy = rh - 1;
                const uchar *kRow = &roi[sy * rw * 4];
                for (int kx = -1; kx <= 1; ++kx) {
                    int sx = lx + kx;
                    if (sx < 0) sx = 0; else if (sx >= rw) sx = rw - 1;
                    const uchar *px = kRow + sx * 4;
                    sumB += px[0]; sumG += px[1]; sumR += px[2]; sumA += px[3];
                }
            }
            uchar *dst = dstRow + x * 4;
            dst[0] = (uchar)(sumB / 9);
            dst[1] = (uchar)(sumG / 9);
            dst[2] = (uchar)(sumR / 9);
            dst[3] = (uchar)(sumA / 9);
        }
    }
}

static void applyHeal(QImage &image, const QPoint &pos, int radius) {
    if (image.isNull() || radius <= 0) return;
    if (image.format() != QImage::Format_ARGB32 &&
        image.format() != QImage::Format_ARGB32_Premultiplied &&
        image.format() != QImage::Format_RGB32) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }

    const int w = image.width(), h = image.height();
    const int stride = image.bytesPerLine();
    const int r2 = qMax(1, radius * radius);

    const int ph = qBound(2, radius / 4, 3);
    const int margin = ph * 2 + 8;

    const int ex0 = qMax(0, pos.x() - radius - margin);
    const int ex1 = qMin(w - 1, pos.x() + radius + margin);
    const int ey0 = qMax(0, pos.y() - radius - margin);
    const int ey1 = qMin(h - 1, pos.y() + radius + margin);
    if (ex0 > ex1 || ey0 > ey1) return;

    const int rw = ex1 - ex0 + 1, rh = ey1 - ey0 + 1;
    std::vector<uchar> roi((size_t)rw * rh * 4);
    for (int y = 0; y < rh; ++y)
        std::memcpy(&roi[(size_t)y * rw * 4], image.constScanLine(ey0 + y) + ex0 * 4, (size_t)rw * 4);

    std::vector<uchar> state((size_t)rw * rh, 0);
    bool anyHole = false;

    for (int y = 0; y < rh; ++y) {
        const int cy = ey0 + y, dy = cy - pos.y();
        for (int x = 0; x < rw; ++x) {
            const int cx = ex0 + x, dx = cx - pos.x();
            const bool inBrush = (dx * dx + dy * dy <= r2);
            const uchar a = roi[((size_t)y * rw + x) * 4 + 3];
            if (a > 50) state[(size_t)y * rw + x] = 0;
            else if (inBrush) { state[(size_t)y * rw + x] = 1; anyHole = true; }
            else state[(size_t)y * rw + x] = 3;
        }
    }

    if (anyHole) {
        std::vector<int> srcXs, srcYs;
        for (int cy = ph; cy < rh - ph; cy += 2) {
            for (int cx = ph; cx < rw - ph; cx += 2) {
                bool ok = true;
                for (int oy = -ph; oy <= ph && ok; ++oy)
                    for (int ox = -ph; ox <= ph; ++ox)
                        if (state[(size_t)(cy + oy) * rw + (cx + ox)] != 0) { ok = false; break; }
                if (ok) { srcXs.push_back(cx); srcYs.push_back(cy); }
            }
        }

        std::vector<uchar> snap(roi.size());
        std::vector<uchar> stateSnap(state.size());
        int dOff[49];

        const int maxIter = qMin(radius / ph + 3, 20);
        for (int iter = 0; iter < maxIter; ++iter) {
            bool changed = false;
            std::memcpy(&snap[0], &roi[0], roi.size());
            std::memcpy(&stateSnap[0], &state[0], state.size());

            for (int y = 0; y < rh; ++y) {
                for (int x = 0; x < rw; ++x) {
                    if (state[(size_t)y * rw + x] != 1) continue;
                    if (x < ph || x >= rw - ph || y < ph || y >= rh - ph) continue;

                    int nOff = 0;
                    for (int oy = -ph; oy <= ph; ++oy) {
                        for (int ox = -ph; ox <= ph; ++ox) {
                            const uchar st = stateSnap[(size_t)(y + oy) * rw + (x + ox)];
                            if (st == 0 || st == 2) dOff[nOff++] = (oy * rw + ox) * 4;
                        }
                    }
                    if (nOff < 4) continue;

                    const size_t tBase = ((size_t)y * rw + x) * 4;
                    int bestSSD = INT_MAX, bestIdx = -1;

                    for (size_t si = 0; si < srcXs.size(); ++si) {
                        const size_t sBase = ((size_t)srcYs[si] * rw + srcXs[si]) * 4;
                        int ssd = 0;
                        for (int k = 0; k < nOff; ++k) {
                            const uchar *p1 = &snap[tBase + dOff[k]];
                            const uchar *p2 = &snap[sBase + dOff[k]];
                            const int db = p1[0] - p2[0], dg = p1[1] - p2[1], dr = p1[2] - p2[2];
                            ssd += db * db + dg * dg + dr * dr;
                            if (ssd >= bestSSD) break;
                        }
                        if (ssd < bestSSD) {
                            bestSSD = ssd; bestIdx = (int)si;
                            if (ssd == 0) break;
                        }
                    }

                    if (bestIdx >= 0) {
                        const size_t sBase = ((size_t)srcYs[bestIdx] * rw + srcXs[bestIdx]) * 4;
                        const uchar *src = &snap[sBase];
                        uchar *dstp = &roi[tBase];
                        dstp[0] = src[0]; dstp[1] = src[1]; dstp[2] = src[2]; dstp[3] = src[3];
                        state[(size_t)y * rw + x] = 2;
                        changed = true;
                    }
                }
            }
            if (!changed) break;
        }

        const int fs = qMin(radius + ph, 24);
        for (int y = 0; y < rh; ++y) {
            for (int x = 0; x < rw; ++x) {
                if (state[(size_t)y * rw + x] != 1) continue;
                double sumB = 0, sumG = 0, sumR = 0, sumA = 0, wSum = 0;
                for (int oy = -fs; oy <= fs; ++oy) {
                    const int ny = y + oy;
                    if (ny < 0 || ny >= rh) continue;
                    for (int ox = -fs; ox <= fs; ++ox) {
                        const int nx = x + ox;
                        if (nx < 0 || nx >= rw) continue;
                        const uchar *p = &roi[((size_t)ny * rw + nx) * 4];
                        if (p[3] <= 50) continue;
                        const double wt = 1.0 / (1.0 + (double)(ox * ox + oy * oy));
                        sumB += p[0] * wt; sumG += p[1] * wt; sumR += p[2] * wt; sumA += p[3] * wt;
                        wSum += wt;
                    }
                }
                if (wSum > 1e-9) {
                    uchar *dstp = &roi[((size_t)y * rw + x) * 4];
                    dstp[0] = (uchar)qBound(0, (int)(sumB / wSum + 0.5), 255);
                    dstp[1] = (uchar)qBound(0, (int)(sumG / wSum + 0.5), 255);
                    dstp[2] = (uchar)qBound(0, (int)(sumR / wSum + 0.5), 255);
                    dstp[3] = (uchar)qBound(0, (int)(sumA / wSum + 0.5), 255);
                    state[(size_t)y * rw + x] = 2;
                }
            }
        }
    }

    std::vector<uchar> snap2(roi.size());
    std::memcpy(&snap2[0], &roi[0], roi.size());

    static const double SW[3][3] = {
        { 0.3679, 0.6065, 0.3679 },
        { 0.6065, 1.0000, 0.6065 },
        { 0.3679, 0.6065, 0.3679 }
    };
    const double sigmaR = 32.0;
    const double inv2s2 = 1.0 / (2.0 * sigmaR * sigmaR);

    for (int y = 0; y < rh; ++y) {
        const int cy = ey0 + y, dy = cy - pos.y();
        for (int x = 0; x < rw; ++x) {
            const int cx = ex0 + x, dx = cx - pos.x();
            const int dist2 = dx * dx + dy * dy;
            if (dist2 > r2) continue;
            if (state[(size_t)y * rw + x] != 0) continue;

            bool nearFill = false;
            for (int oy = -1; oy <= 1 && !nearFill; ++oy) {
                const int ny = y + oy;
                if (ny < 0 || ny >= rh) continue;
                for (int ox = -1; ox <= 1; ++ox) {
                    const int nx = x + ox;
                    if (nx < 0 || nx >= rw) continue;
                    if (state[(size_t)ny * rw + nx] == 2) { nearFill = true; break; }
                }
            }
            if (nearFill) continue;

            const uchar *cpx = &snap2[((size_t)y * rw + x) * 4];
            const int cB = cpx[0], cG = cpx[1], cR = cpx[2];
            double sumB = 0, sumG = 0, sumR = 0, wSum = 0;

            for (int ky = -1; ky <= 1; ++ky) {
                int ny = y + ky;
                if (ny < 0) ny = 0; else if (ny >= rh) ny = rh - 1;
                for (int kx = -1; kx <= 1; ++kx) {
                    int nx = x + kx;
                    if (nx < 0) nx = 0; else if (nx >= rw) nx = rw - 1;
                    const uchar *p = &snap2[((size_t)ny * rw + nx) * 4];
                    if (p[3] <= 50) continue;
                    const int dB = p[0] - cB, dG = p[1] - cG, dR = p[2] - cR;
                    const double cd2 = (dB * dB + dG * dG + dR * dR) / 3.0;
                    const double wt = SW[ky + 1][kx + 1] * std::exp(-cd2 * inv2s2);
                    sumB += p[0] * wt; sumG += p[1] * wt; sumR += p[2] * wt;
                    wSum += wt;
                }
            }

            if (wSum > 1e-9) {
                const double fall = 1.0 - (double)dist2 / (double)r2;
                const double strength = fall * fall * (3.0 - 2.0 * fall);
                const int hB = (int)(sumB / wSum + 0.5);
                const int hG = (int)(sumG / wSum + 0.5);
                const int hR = (int)(sumR / wSum + 0.5);
                uchar *dstp = &roi[((size_t)y * rw + x) * 4];
                dstp[0] = (uchar)qBound(0, (int)(cB + (hB - cB) * strength + 0.5), 255);
                dstp[1] = (uchar)qBound(0, (int)(cG + (hG - cG) * strength + 0.5), 255);
                dstp[2] = (uchar)qBound(0, (int)(cR + (hR - cR) * strength + 0.5), 255);
            }
        }
    }

    uchar *bits = image.bits();
    for (int y = 0; y < rh; ++y) {
        const int cy = ey0 + y, dy = cy - pos.y();
        uchar *dstRow = bits + (size_t)cy * stride;
        for (int x = 0; x < rw; ++x) {
            const int cx = ex0 + x, dx = cx - pos.x();
            if (dx * dx + dy * dy > r2) continue;
            const uchar *src = &roi[((size_t)y * rw + x) * 4];
            uchar *dstp = dstRow + (size_t)cx * 4;
            dstp[0] = src[0]; dstp[1] = src[1]; dstp[2] = src[2]; dstp[3] = src[3];
        }
    }
}

static void applyShadowBurn(QImage &image, const QPoint &pos, int radius, double sensitivity, int penOpacity) {
    if (image.isNull() || radius <= 0) return;
    if (image.format() != QImage::Format_ARGB32 &&
        image.format() != QImage::Format_ARGB32_Premultiplied &&
        image.format() != QImage::Format_RGB32) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }
    const int w = image.width(), h = image.height();
    const int stride = image.bytesPerLine();
    const int r2 = qMax(1, radius * radius);
    const int x0 = qMax(0, pos.x() - radius), x1 = qMin(w - 1, pos.x() + radius);
    const int y0 = qMax(0, pos.y() - radius), y1 = qMin(h - 1, pos.y() + radius);
    if (x0 > x1 || y0 > y1) return;

    double baseDarken = 0.03 * sensitivity * (penOpacity / 255.0);
    if (baseDarken > 0.5) baseDarken = 0.5;

    uchar *bits = image.bits();
    for (int y = y0; y <= y1; ++y) {
        const int dy = y - pos.y(), dy2 = dy * dy;
        uchar *row = bits + y * stride;
        for (int x = x0; x <= x1; ++x) {
            const int dx = x - pos.x();
            const int dist2 = dx * dx + dy2;
            if (dist2 > r2) continue;

            uchar *px = row + x * 4;
            if (px[3] == 0) continue;

            const double fall = 1.0 - (double)dist2 / (double)r2;
            int mul = 256 - (int)(baseDarken * fall * 256.0);
            if (mul < 0) mul = 0; else if (mul > 256) mul = 256;

            px[0] = (uchar)((px[0] * mul) >> 8);
            px[1] = (uchar)((px[1] * mul) >> 8);
            px[2] = (uchar)((px[2] * mul) >> 8);
        }
    }
}

static void applyRetouchAlongLine(QImage &image, const QPoint &start, const QPoint &end,
                                  int penWidth, double mouseSensitivity, int penOpacity,
                                  int toolType) {
    int scaledWidth = qMax(1, static_cast<int>(penWidth * mouseSensitivity));
    int radius = scaledWidth * 2 + 2;
    int dist = qMax(qAbs(end.x() - start.x()), qAbs(end.y() - start.y()));
    int spacing = qMax(2, radius / 2);
    int steps = dist / spacing;
    if (dist > 0 && steps == 0) steps = 1;

    for (int s = 0; s <= steps; ++s) {
        double t = (steps == 0) ? 0.0 : static_cast<double>(s) / steps;
        int cx = start.x() + static_cast<int>(t * (end.x() - start.x()));
        int cy = start.y() + static_cast<int>(t * (end.y() - start.y()));

        switch (toolType) {
            case 101: applyShadowBurn(image, QPoint(cx, cy), radius, mouseSensitivity, penOpacity); break;
            case 201: applyBlur(image, QPoint(cx, cy), radius); break;
            case 202: applyHeal(image, QPoint(cx, cy), radius); break;
            default: break;
        }
    }
}

};

#endif // RETOUCHTOOLS_H