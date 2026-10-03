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
#include <QVector>
#include <QQueue>
#include <QSet>
#include <QStack>
#include <QApplication>
#include <QClipboard>
#include <QTextOption>
#include <QFont>
#include <QColor>
#include <QPen>
#include <QBrush>
#include <QLineF>
#include <cmath>
#include <queue>
#include <vector>
#include <algorithm>

#include "core/ToolManager.h"
#include "core/ShapeObjects.h"

////////////////////////////////////////////////////////////////
// Resultado de selección 

struct ElementSelectResult {
    QImage mask;
    QRect  boundingBox;
    int    pixelsSelected = 0;
    int    pixelsSoft     = 0;
    bool isValid() const { return pixelsSelected > 0 && !boundingBox.isEmpty(); }
};

////////////////////////////////////////////////////////////
// Opciones de Selección de Elementos (pipeline completo)

struct ElementSelectOptions {
    int    baseTolerance        = 30;
    bool   usePerceptual        = false;
    bool   softEdges            = true;
    bool   fillHoles            = true;
    int    fillHolesRadius      = 3;
    int    closeRadius          = 2;
    bool   useEdgeRefine        = true;
    double edgeRefineStrength   = 0.35;
    int    feather              = 1;
    int    minArea              = 20;
    int    maxAreaPct           = 92;
    int    hoverDownsample      = 5;
    int    hoverTolerance       = 38;
};

////////////////////////////////////////////////////////////
// Configuración de la Varita Mágica

struct WandConfig {
    int    tolerance      = 32;
    bool   contiguous     = true;
    bool   includeAlpha   = true;
    bool   softEdges      = true;
    int    sampleRadius   = 0;
    int    feather        = 0;
    double edgeRefine     = 0.0;

    bool   useColorFamily = true;
    double hueTolerance   = 35.0;
    double satTolerance   = 0.40;
    double valTolerance   = 0.60;

    bool   usePerceptual  = false;
};


//////////////////////////////////////////////////////////

class SelectionManager {
public:
    enum SelectionType {
        SelectionNone = 0,
        SelectionRect,
        SelectionPath,
        SelectionMagicWand,
        SelectionElement
    };

    enum FreeSubMode {
        FreeLasso   = 0,
        FreeVector  = 1,
        FreeElement = 2
    };

private:
    // --- Estado de selección ---
    SelectionType selType    = SelectionNone;
    bool          active     = false;
    bool          dragging   = false;
    bool          resizing   = false;
    bool          rotating   = false;
    ObjectHandle  resizeHandle   = ObjectHandle::None;
    QRectF        resizeStartRect;
    QPointF       resizeStartPos;
    QRect         selRect;
    QImage        selBuffer;
    QPainterPath  freePath;
    QPoint        dragOffset;
    double        selRotation = 0.0;

    // --- Sub-modos libres ---
    FreeSubMode freeSubMode = FreeLasso;
    bool        freeVectorMode = false;
    QVector<QPointF> freeVectorPoints;
    int freeVectorHoverIndex = -1;
    int freeVectorDragIndex  = -1;

    // --- Hover de elementos ---
    bool elementHoverActive = false;
    QPoint elementHoverPos;
    ElementSelectResult elementHoverPreview;

    // --- Objetos ---
    QList<PaintObject> objects;
    int  activeObjIndex = -1;
    bool objDragging  = false;
    bool objRotating  = false;
    bool objScaling   = false;
    ObjectHandle objActiveHandle = ObjectHandle::None;
    QPointF objDragStart;
    double  objRotationStart = 0.0;
    double  objScaleStartX   = 1.0;
    double  objScaleStartY   = 1.0;
    QRectF  objBoundsStart;
    QMap<int, QImage> selObjBuffers;
    int nextSelBufferId = 0;
    QImage clipboardBuffer;


    ////////////////////////////////////////////////////////////
    //  MOTOR  — Espacios de color


    struct LabColor { double L, a, b; };

    static inline double srgbCompToLinear(int c) {
        double cs = c / 255.0;
        return (cs <= 0.04045) ? (cs / 12.92)
                               : std::pow((cs + 0.055) / 1.055, 2.4);
    }

    static LabColor rgbToLab(int r, int g, int b) {
        double rl = srgbCompToLinear(r);
        double gl = srgbCompToLinear(g);
        double bl = srgbCompToLinear(b);
        double X = (rl*0.4124564 + gl*0.3575761 + bl*0.1804375) * 100.0;
        double Y = (rl*0.2126729 + gl*0.7151522 + bl*0.0721750) * 100.0;
        double Z = (rl*0.0193339 + gl*0.1191920 + bl*0.9503041) * 100.0;
        const double Xn=95.047, Yn=100.0, Zn=108.883;
        const double eps=216.0/24389.0, k=24389.0/27.0;
        double fx = (X/Xn>eps) ? std::cbrt(X/Xn) : (k*(X/Xn)+16.0)/116.0;
        double fy = (Y/Yn>eps) ? std::cbrt(Y/Yn) : (k*(Y/Yn)+16.0)/116.0;
        double fz = (Z/Zn>eps) ? std::cbrt(Z/Zn) : (k*(Z/Zn)+16.0)/116.0;
        LabColor lab;
        lab.L = 116.0*fy - 16.0;
        lab.a = 500.0*(fx - fy);
        lab.b = 200.0*(fy - fz);
        return lab;
    }

    static void rgbToHsv(int r, int g, int b,
                         double &h, double &s, double &v) {
        double rd = r/255.0, gd = g/255.0, bd = b/255.0;
        double mx = std::max({rd, gd, bd});
        double mn = std::min({rd, gd, bd});
        double d  = mx - mn;
        v = mx;
        s = (mx > 0) ? d / mx : 0.0;
        if (d < 1e-9) {
            h = 0.0;
        } else if (mx == rd) {
            h = 60.0 * std::fmod((gd - bd) / d, 6.0);
        } else if (mx == gd) {
            h = 60.0 * ((bd - rd) / d + 2.0);
        } else {
            h = 60.0 * ((rd - gd) / d + 4.0);
        }
        if (h < 0) h += 360.0;
    }

    ////////////////////////////////////////////////////////////
    //  MOTOR I— Varita mágica


    struct WandNode { double dist; int x, y; };
    struct WandNodeCmp {
        bool operator()(const WandNode &a, const WandNode &b) const {
            return a.dist > b.dist;
        }
    };

    static void sampleSeedColor(const QImage &img, const QPoint &pos,
                                int radius, int &r, int &g, int &b, int &a) {
        int x0 = qMax(0, pos.x()-radius), x1 = qMin(img.width()-1,  pos.x()+radius);
        int y0 = qMax(0, pos.y()-radius), y1 = qMin(img.height()-1, pos.y()+radius);
        long long sr=0, sg=0, sb=0, sa=0; int count=0;
        for (int y=y0; y<=y1; ++y) {
            const QRgb *line = (const QRgb*)img.constScanLine(y);
            for (int x=x0; x<=x1; ++x) {
                QRgb p = line[x];
                sr += qRed(p); sg += qGreen(p);
                sb += qBlue(p); sa += qAlpha(p); count++;
            }
        }
        if (count==0) { r=g=b=a=0; return; }
        r=(int)(sr/count); g=(int)(sg/count);
        b=(int)(sb/count); a=(int)(sa/count);
    }

    static ElementSelectResult runMagicWand(const QImage &image,
                                            const QPoint &pos,
                                            const WandConfig &cfg) {
        ElementSelectResult result;
        if (image.isNull()) return result;
        if (pos.x()<0 || pos.x()>=image.width() ||
            pos.y()<0 || pos.y()>=image.height()) return result;

        QImage img = image;
        if (img.format()!=QImage::Format_ARGB32 &&
            img.format()!=QImage::Format_ARGB32_Premultiplied &&
            img.format()!=QImage::Format_RGB32)
            img = img.convertToFormat(QImage::Format_ARGB32);

        const int w = img.width(), h = img.height();

        int seedR, seedG, seedB, seedA;
        sampleSeedColor(img, pos, qMax(0, cfg.sampleRadius),
                        seedR, seedG, seedB, seedA);
        LabColor seedLab = rgbToLab(seedR, seedG, seedB);
        double seedH=0, seedS=0, seedV=0;
        if (cfg.useColorFamily) rgbToHsv(seedR, seedG, seedB, seedH, seedS, seedV);

        int numCh = cfg.includeAlpha ? 4 : 3;
        double tolDist = 0, softLimitDist = 0;
        const double softFactor = 1.35;
        if (cfg.usePerceptual) {
            tolDist = cfg.tolerance * 0.55;
        } else {
            tolDist = cfg.tolerance * std::sqrt((double)numCh);
        }
        softLimitDist = tolDist * softFactor;
        if (tolDist <= 0) { tolDist = 0.5; softLimitDist = cfg.softEdges ? 1.5 : 0.5; }

        auto pixelDist = [&](int r, int g, int b, int a) -> double {
            if (cfg.useColorFamily) {
                double ph, ps, pv;
                rgbToHsv(r, g, b, ph, ps, pv);

                double dh = std::abs(ph - seedH);
                dh = std::min(dh, 360.0 - dh);

                double ndh = (cfg.hueTolerance > 0) ? dh / cfg.hueTolerance : 0.0;
                double nds = (cfg.satTolerance > 0) ? std::abs(ps - seedS) / cfg.satTolerance : 0.0;
                double ndv = (cfg.valTolerance > 0) ? std::abs(pv - seedV) / cfg.valTolerance : 0.0;

                double d = ndh * 0.40 + ndv * 0.35 + nds * 0.25;

                if (cfg.includeAlpha) {
                    double da = std::abs(a - seedA) / 255.0;
                    d += da * 0.15;
                }
                return d;
            }

            if (cfg.usePerceptual) {
                LabColor lab = rgbToLab(r, g, b);
                double dL = lab.L - seedLab.L;
                double da = lab.a - seedLab.a;
                double db = lab.b - seedLab.b;
                double d  = std::sqrt(dL*dL + da*da + db*db);
                if (cfg.includeAlpha) {
                    double dA = (a - seedA) * (100.0/255.0);
                    d = std::sqrt(d*d + dA*dA);
                }
                return d;
            }

            double dr = r-seedR, dg = g-seedG, db = b-seedB;
            double dSq = dr*dr + dg*dg + db*db;
            if (cfg.includeAlpha) { double dA = a-seedA; dSq += dA*dA; }
            return std::sqrt(dSq);
        };

        auto alphaFromDist = [&](double d) -> int {
            if (cfg.useColorFamily) {
                if (d <= 1.0) return 255;
                if (cfg.softEdges && d <= 1.35) {
                    double t = (d - 1.0) / 0.35;
                    return qBound(0, (int)(255.0*(1.0-t)+0.5), 255);
                }
                return 0;
            }
            if (d <= tolDist) return 255;
            if (cfg.softEdges && d <= softLimitDist && softLimitDist > tolDist) {
                double t = (d - tolDist) / (softLimitDist - tolDist);
                return qBound(0, (int)(255.0*(1.0-t)+0.5), 255);
            }
            return 0;
        };

        result.mask = QImage(w, h, QImage::Format_ARGB32);
        result.mask.fill(Qt::transparent);
        uchar *maskBits = result.mask.bits();
        const int maskStride = result.mask.bytesPerLine();
        int minX=w, minY=h, maxX=0, maxY=0;

        auto markPixel = [&](int x, int y, int alpha) {
            uchar *mLine = maskBits + y * maskStride;
            uchar *mPx   = mLine + x * 4;
            mPx[0]=255; mPx[1]=255; mPx[2]=255; mPx[3]=(uchar)alpha;
            result.pixelsSelected++;
            if (alpha>0 && alpha<255) result.pixelsSoft++;
            if (x<minX) minX=x; if (x>maxX) maxX=x;
            if (y<minY) minY=y; if (y>maxY) maxY=y;
        };

        if (cfg.contiguous) {
            QVector<bool> visited(w*h, false);
            std::priority_queue<WandNode, std::vector<WandNode>, WandNodeCmp> pq;
            auto tryPush = [&](int x, int y) {
                if (x<0||x>=w||y<0||y>=h) return;
                int idx = y*w+x;
                if (visited[idx]) return;
                visited[idx] = true;
                QRgb px = ((const QRgb*)img.constScanLine(y))[x];
                double d = pixelDist(qRed(px), qGreen(px), qBlue(px), qAlpha(px));
                pq.push({d, x, y});
            };
            tryPush(pos.x(), pos.y());
            while (!pq.empty()) {
                WandNode n = pq.top(); pq.pop();
                int alpha = alphaFromDist(n.dist);
                if (alpha <= 0) continue;
                markPixel(n.x, n.y, alpha);
                tryPush(n.x-1, n.y);
                tryPush(n.x+1, n.y);
                tryPush(n.x, n.y-1);
                tryPush(n.x, n.y+1);
            }
        } else {
            for (int y=0; y<h; ++y) {
                const QRgb *line = (const QRgb*)img.constScanLine(y);
                for (int x=0; x<w; ++x) {
                    QRgb px = line[x];
                    double d = pixelDist(qRed(px), qGreen(px), qBlue(px), qAlpha(px));
                    int alpha = alphaFromDist(d);
                    if (alpha > 0) markPixel(x, y, alpha);
                }
            }
        }

        if (result.pixelsSelected > 0)
            result.boundingBox = QRect(minX, minY, maxX-minX+1, maxY-minY+1);
        return result;
    }

    ////////////////////////////////////////////////////////////
    //  MOTOR INTERNO — Post-procesos de máscara
    

    static void gaussianBlurAlpha(QImage &img, double sigma) {
        if (img.isNull() || sigma<=0) return;
        if (img.format()!=QImage::Format_ARGB32)
            img = img.convertToFormat(QImage::Format_ARGB32);
        int radius = (int)std::ceil(sigma*3.0);
        if (radius<1) radius=1;
        int ksize = radius*2+1;
        QVector<double> kernel(ksize);
        double ksum=0;
        for (int i=0; i<ksize; ++i) {
            double x = i-radius;
            kernel[i] = std::exp(-(x*x)/(2.0*sigma*sigma));
            ksum += kernel[i];
        }
        for (int i=0; i<ksize; ++i) kernel[i] /= ksum;
        const int w=img.width(), h=img.height();
        QVector<double> src(w*h), tmp(w*h);
        for (int y=0; y<h; ++y) {
            const uchar *line = img.constScanLine(y);
            for (int x=0; x<w; ++x) src[y*w+x] = line[x*4+3];
        }
        for (int y=0; y<h; ++y)
            for (int x=0; x<w; ++x) {
                double acc=0;
                for (int k=-radius; k<=radius; ++k) {
                    int xx = qBound(0, x+k, w-1);
                    acc += src[y*w+xx] * kernel[k+radius];
                }
                tmp[y*w+x] = acc;
            }
        for (int y=0; y<h; ++y) {
            uchar *line = img.bits() + y*img.bytesPerLine();
            for (int x=0; x<w; ++x) {
                double acc=0;
                for (int k=-radius; k<=radius; ++k) {
                    int yy = qBound(0, y+k, h-1);
                    acc += tmp[yy*w+x] * kernel[k+radius];
                }
                line[x*4+3] = (uchar)qBound(0, (int)(acc+0.5), 255);
            }
        }
    }

    static void featherMask(QImage &mask, int radius) {
        if (mask.isNull() || radius<=0) return;
        if (mask.format()!=QImage::Format_ARGB32)
            mask = mask.convertToFormat(QImage::Format_ARGB32);
        double sigma = radius/2.0;
        if (sigma<0.5) sigma=0.5;
        gaussianBlurAlpha(mask, sigma);
    }

    static void smoothMask(QImage &mask, double sigma) {
        if (mask.isNull() || sigma<=0) return;
        if (mask.format()!=QImage::Format_ARGB32)
            mask = mask.convertToFormat(QImage::Format_ARGB32);
        gaussianBlurAlpha(mask, sigma);
    }

    static QImage refineMaskWithEdges(const QImage &mask,
                                      const QImage &source,
                                      double strength) {
        if (mask.isNull()||source.isNull()) return mask;
        if (mask.size()!=source.size()) return mask;
        if (strength==0.0) return mask;
        QImage m = mask.copy();
        if (m.format()!=QImage::Format_ARGB32) m=m.convertToFormat(QImage::Format_ARGB32);
        QImage src = source;
        if (src.format()!=QImage::Format_ARGB32) src=src.convertToFormat(QImage::Format_ARGB32);
        const int w=m.width(), h=m.height();
        if (w<3||h<3) return m;
        QVector<double> gray(w*h);
        for (int y=0; y<h; ++y) {
            const QRgb *sl=(const QRgb*)src.constScanLine(y);
            for (int x=0; x<w; ++x) gray[y*w+x]=qGray(sl[x]);
        }
        QVector<double> mag(w*h, 0.0);
        for (int y=1; y<h-1; ++y)
            for (int x=1; x<w-1; ++x) {
                int i=y*w+x;
                double gx = -gray[i-w-1]+gray[i-w+1]
                            -2.0*gray[i-1]+2.0*gray[i+1]
                            -gray[i+w-1]+gray[i+w+1];
                double gy = -gray[i-w-1]-2.0*gray[i-w]-gray[i-w+1]
                            +gray[i+w-1]+2.0*gray[i+w]+gray[i+w+1];
                mag[i]=std::sqrt(gx*gx+gy*gy);
            }
        const double maxMag=1442.0;
        for (int y=1; y<h-1; ++y) {
            QRgb *mline=(QRgb*)m.scanLine(y);
            const uchar *maUp=m.constScanLine(y-1);
            const uchar *maDn=m.constScanLine(y+1);
            for (int x=1; x<w-1; ++x) {
                int a=qAlpha(mline[x]);
                if (a==0) continue;
                bool border=(qAlpha(mline[x-1])==0)||
                            (qAlpha(mline[x+1])==0)||
                            (maUp[x*4+3]==0)||
                            (maDn[x*4+3]==0);
                if (!border) continue;
                double e=mag[y*w+x]/maxMag;
                double factor=qBound(0.0, 1.0-strength*e, 2.0);
                mline[x]=qRgba(255,255,255,qBound(0,(int)(a*factor+0.5),255));
            }
        }
        return m;
    }

    static void recomputeElementStats(ElementSelectResult &r) {
        r.pixelsSelected=0; r.pixelsSoft=0; r.boundingBox=QRect();
        if (r.mask.isNull()) return;
        const int w=r.mask.width(), h=r.mask.height();
        int minX=w, minY=h, maxX=0, maxY=0;
        for (int y=0; y<h; ++y) {
            const QRgb *line=(const QRgb*)r.mask.constScanLine(y);
            for (int x=0; x<w; ++x) {
                int a=qAlpha(line[x]);
                if (a<=0) continue;
                r.pixelsSelected++;
                if (a<255) r.pixelsSoft++;
                if (x<minX) minX=x; if (x>maxX) maxX=x;
                if (y<minY) minY=y; if (y>maxY) maxY=y;
            }
        }
        if (r.pixelsSelected>0)
            r.boundingBox=QRect(minX,minY,maxX-minX+1,maxY-minY+1);
    }

    ////////////////////////////////////////////////////////////
    //  MOTOR INTERNO — Operaciones de máscara
  

    static QImage extractMaskedRegion(const QImage &source,
                                      const QImage &mask,
                                      const QRect &bbox) {
        if (source.isNull()||mask.isNull()) return QImage();
        QRect r = bbox.intersected(source.rect()).intersected(mask.rect());
        if (r.isEmpty()) return QImage();
        QImage src=source;
        if (src.format()!=QImage::Format_ARGB32) src=src.convertToFormat(QImage::Format_ARGB32);
        QImage msk=mask;
        if (msk.format()!=QImage::Format_ARGB32) msk=msk.convertToFormat(QImage::Format_ARGB32);
        QImage out(r.size(), QImage::Format_ARGB32);
        out.fill(Qt::transparent);
        for (int y=0; y<r.height(); ++y) {
            const QRgb *srcLine=(const QRgb*)src.constScanLine(r.y()+y);
            const QRgb *mskLine=(const QRgb*)msk.constScanLine(r.y()+y);
            QRgb *outLine=(QRgb*)out.scanLine(y);
            for (int x=0; x<r.width(); ++x) {
                int sx=r.x()+x;
                int ma=qAlpha(mskLine[sx]);
                if (ma<=0) continue;
                QRgb px=srcLine[sx];
                int sa=qAlpha(px);
                int na=(sa*ma)/255;
                if (na<=0) continue;
                outLine[x]=qRgba(qRed(px),qGreen(px),qBlue(px),na);
            }
        }
        return out;
    }

    static void clearMaskedRegion(QImage &target, const QImage &mask) {
        if (target.isNull()||mask.isNull()) return;
        if (target.size()!=mask.size()) return;
        QPainter p(&target);
        p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
        p.drawImage(0,0,mask);
        p.end();
    }

    static QVector<double> computeDistanceField(const QVector<bool> &zeroSet,
                                                int w, int h) {
        const double INF=1e30, D1=1.0, D2=1.41421356237;
        QVector<double> d(w*h, INF);
        for (int i=0; i<w*h; ++i) if (zeroSet[i]) d[i]=0.0;
        for (int y=0; y<h; ++y)
            for (int x=0; x<w; ++x) {
                int i=y*w+x;
                if (x>0) d[i]=std::min(d[i], d[i-1]+D1);
                if (y>0) {
                    d[i]=std::min(d[i], d[i-w]+D1);
                    if (x>0)     d[i]=std::min(d[i], d[i-w-1]+D2);
                    if (x<w-1)   d[i]=std::min(d[i], d[i-w+1]+D2);
                }
            }
        for (int y=h-1; y>=0; --y)
            for (int x=w-1; x>=0; --x) {
                int i=y*w+x;
                if (x<w-1) d[i]=std::min(d[i], d[i+1]+D1);
                if (y<h-1) {
                    d[i]=std::min(d[i], d[i+w]+D1);
                    if (x>0)     d[i]=std::min(d[i], d[i+w-1]+D2);
                    if (x<w-1)   d[i]=std::min(d[i], d[i+w+1]+D2);
                }
            }
        return d;
    }

    static QImage expandMask(const QImage &mask, int pixels) {
        if (mask.isNull()||pixels<=0) return mask;
        QImage m=mask.copy();
        if (m.format()!=QImage::Format_ARGB32) m=m.convertToFormat(QImage::Format_ARGB32);
        const int w=m.width(), h=m.height();
        QVector<bool> src(w*h);
        for (int y=0; y<h; ++y) {
            const QRgb *line=(const QRgb*)m.constScanLine(y);
            for (int x=0; x<w; ++x) src[y*w+x]=(qAlpha(line[x])>127);
        }
        QVector<double> d=computeDistanceField(src,w,h);
        double thr=(double)pixels;
        for (int y=0; y<h; ++y) {
            QRgb *line=(QRgb*)m.scanLine(y);
            for (int x=0; x<w; ++x) {
                int i=y*w+x, orig=qAlpha(line[x]), newA;
                if (d[i]<=thr-0.5) newA=255;
                else if (d[i]<=thr+0.5) newA=(int)(255.0*(thr+0.5-d[i])+0.5);
                else newA=0;
                line[x]=qRgba(255,255,255,qMax(orig,newA));
            }
        }
        return m;
    }

    static QImage contractMask(const QImage &mask, int pixels) {
        if (mask.isNull()||pixels<=0) return mask;
        QImage m=mask.copy();
        if (m.format()!=QImage::Format_ARGB32) m=m.convertToFormat(QImage::Format_ARGB32);
        const int w=m.width(), h=m.height();
        QVector<bool> src(w*h);
        for (int y=0; y<h; ++y) {
            const QRgb *line=(const QRgb*)m.constScanLine(y);
            for (int x=0; x<w; ++x) src[y*w+x]=(qAlpha(line[x])<=127);
        }
        QVector<double> d=computeDistanceField(src,w,h);
        double thr=(double)pixels;
        for (int y=0; y<h; ++y) {
            QRgb *line=(QRgb*)m.scanLine(y);
            for (int x=0; x<w; ++x) {
                int i=y*w+x, orig=qAlpha(line[x]);
                if (orig==0) continue;
                int newA;
                if (d[i]>=thr+0.5) newA=orig;
                else if (d[i]>=thr-0.5) newA=(int)(orig*(d[i]-(thr-0.5))+0.5);
                else newA=0;
                line[x]=qRgba(255,255,255,qBound(0,newA,255));
            }
        }
        return m;
    }

    static QImage sharpenMaskEdges(const QImage &mask, double amount) {
        if (mask.isNull()||amount<=0.0) return mask;
        QImage m=mask.copy();
        if (m.format()!=QImage::Format_ARGB32) m=m.convertToFormat(QImage::Format_ARGB32);
        QImage blurred=m.copy();
        gaussianBlurAlpha(blurred, 1.0);
        const int w=m.width(), h=m.height();
        for (int y=0; y<h; ++y) {
            QRgb *mline=(QRgb*)m.scanLine(y);
            const QRgb *bline=(const QRgb*)blurred.constScanLine(y);
            for (int x=0; x<w; ++x) {
                int a=qAlpha(mline[x]), ba=qAlpha(bline[x]);
                int sh=qBound(0,(int)(a+(a-ba)*amount+0.5),255);
                mline[x]=qRgba(255,255,255,sh);
            }
        }
        return m;
    }

    static int estimateAutoTolerance(const QImage &image,
                                     const QPoint &pos, int radius=5) {
        if (image.isNull()) return 32;
        if (pos.x()<0||pos.x()>=image.width()||
            pos.y()<0||pos.y()>=image.height()) return 32;
        QImage img=image;
        if (img.format()!=QImage::Format_ARGB32) img=img.convertToFormat(QImage::Format_ARGB32);
        int x0=qMax(0,pos.x()-radius), x1=qMin(img.width()-1,pos.x()+radius);
        int y0=qMax(0,pos.y()-radius), y1=qMin(img.height()-1,pos.y()+radius);
        double sr=0,sg=0,sb=0; int count=0;
        for (int y=y0; y<=y1; ++y) {
            const QRgb *line=(const QRgb*)img.constScanLine(y);
            for (int x=x0; x<=x1; ++x) {
                sr+=qRed(line[x]); sg+=qGreen(line[x]); sb+=qBlue(line[x]); count++;
            }
        }
        if (count==0) return 32;
        double mr=sr/count, mg=sg/count, mb=sb/count, var=0;
        for (int y=y0; y<=y1; ++y) {
            const QRgb *line=(const QRgb*)img.constScanLine(y);
            for (int x=x0; x<=x1; ++x) {
                double dr=qRed(line[x])-mr, dg=qGreen(line[x])-mg, db=qBlue(line[x])-mb;
                var+=(dr*dr+dg*dg+db*db)/3.0;
            }
        }
        var/=count;
        return qBound(8,(int)(2.0*std::sqrt(var)),128);
    }

    static QImage visualizeMask(const QImage &mask) {
        if (mask.isNull()) return QImage();
        QImage m=mask;
        if (m.format()!=QImage::Format_ARGB32) m=m.convertToFormat(QImage::Format_ARGB32);
        QImage out(m.size(), QImage::Format_ARGB32);
        for (int y=0; y<m.height(); ++y) {
            const QRgb *ml=(const QRgb*)m.constScanLine(y);
            QRgb *ol=(QRgb*)out.scanLine(y);
            for (int x=0; x<m.width(); ++x) {
                int a=qAlpha(ml[x]);
                ol[x]=qRgba(a,a,a,255);
            }
        }
        return out;
    }

    ////////////////////////////////////////////////////////////
    //  MOTOR INTERNO — Relleno de huecos / Cierre morfológico
    

    static void fillHolesInternal(QImage &mask, const QRect &bbox) {
        if (mask.isNull()||bbox.isEmpty()) return;
        const int w=mask.width(), h=mask.height();
        const QRect clip=bbox.intersected(QRect(0,0,w,h));
        if (clip.width()<3||clip.height()<3) return;
        QVector<bool> outside(w*h, false);
        QQueue<QPoint> q;
        for (int x=clip.left(); x<=clip.right(); ++x) {
            q.enqueue(QPoint(x,clip.top()));
            q.enqueue(QPoint(x,clip.bottom()));
        }
        for (int y=clip.top(); y<=clip.bottom(); ++y) {
            q.enqueue(QPoint(clip.left(),y));
            q.enqueue(QPoint(clip.right(),y));
        }
        while (!q.isEmpty()) {
            const QPoint p=q.dequeue();
            int x=p.x(), y=p.y();
            if (x<clip.left()||x>clip.right()) continue;
            if (y<clip.top()||y>clip.bottom()) continue;
            int i=y*w+x;
            if (outside[i]) continue;
            const QRgb *line=(const QRgb*)mask.constScanLine(y);
            if (qAlpha(line[x])>128) continue;
            outside[i]=true;
            q.enqueue(QPoint(x+1,y)); q.enqueue(QPoint(x-1,y));
            q.enqueue(QPoint(x,y+1)); q.enqueue(QPoint(x,y-1));
        }
        for (int y=clip.top(); y<=clip.bottom(); ++y) {
            QRgb *line=(QRgb*)mask.scanLine(y);
            for (int x=clip.left(); x<=clip.right(); ++x)
                if (!outside[y*w+x]) line[x]=qRgba(255,255,255,255);
        }
    }

    static void morphologicalClose(QImage &mask, int radius) {
        if (mask.isNull()||radius<=0) return;
        if (mask.format()!=QImage::Format_ARGB32)
            mask=mask.convertToFormat(QImage::Format_ARGB32);
        const int w=mask.width(), h=mask.height();
        QImage dilated(w,h,QImage::Format_ARGB32);
        dilated.fill(Qt::transparent);
        for (int y=0; y<h; ++y) {
            QRgb *dst=(QRgb*)dilated.scanLine(y);
            for (int x=0; x<w; ++x) {
                int best=0;
                for (int dy=-radius; dy<=radius; ++dy) {
                    int yy=y+dy; if (yy<0||yy>=h) continue;
                    const QRgb *src=(const QRgb*)mask.constScanLine(yy);
                    for (int dx=-radius; dx<=radius; ++dx) {
                        int xx=x+dx; if (xx<0||xx>=w) continue;
                        best=qMax(best, qAlpha(src[xx]));
                    }
                }
                dst[x]=qRgba(255,255,255,best);
            }
        }
        for (int y=0; y<h; ++y) {
            QRgb *dst=(QRgb*)mask.scanLine(y);
            for (int x=0; x<w; ++x) {
                int mn=255;
                for (int dy=-radius; dy<=radius; ++dy) {
                    int yy=y+dy; if (yy<0||yy>=h) continue;
                    const QRgb *src=(const QRgb*)dilated.constScanLine(yy);
                    for (int dx=-radius; dx<=radius; ++dx) {
                        int xx=x+dx; if (xx<0||xx>=w) continue;
                        mn=qMin(mn, qAlpha(src[xx]));
                    }
                }
                dst[x]=qRgba(255,255,255,mn);
            }
        }
    }

    ////////////////////////////////////////////////////////////
    //  NÚCLEO — Selección de Elementos (pipeline)
 

    static ElementSelectResult runSelect(const QImage &image,
                                         const QPoint &seed,
                                         const ElementSelectOptions &opts) {
        ElementSelectResult result;
        if (image.isNull()) return result;
        if (seed.x()<0||seed.x()>=image.width()||
            seed.y()<0||seed.y()>=image.height()) return result;

        WandConfig wc;
        wc.tolerance      = opts.baseTolerance;
        wc.usePerceptual  = opts.usePerceptual;
        wc.softEdges      = opts.softEdges;
        wc.includeAlpha   = true;
        wc.contiguous     = true;
        wc.useColorFamily = false;

        ElementSelectResult wand = runMagicWand(image, seed, wc);
        if (!wand.isValid()) return result;

        QImage mask = wand.mask;
        if (mask.format()!=QImage::Format_ARGB32)
            mask=mask.convertToFormat(QImage::Format_ARGB32);
        const int w=mask.width(), h=mask.height();

        if (opts.closeRadius>0) morphologicalClose(mask, opts.closeRadius);
        if (opts.fillHoles) {
            QRect bbox=wand.boundingBox.adjusted(-opts.fillHolesRadius,
                                                  -opts.fillHolesRadius,
                                                   opts.fillHolesRadius,
                                                   opts.fillHolesRadius);
            fillHolesInternal(mask, bbox.intersected(QRect(0,0,w,h)));
        }
        if (opts.useEdgeRefine && opts.edgeRefineStrength>0)
            mask=refineMaskWithEdges(mask, image, opts.edgeRefineStrength);
        if (opts.feather>0) featherMask(mask, opts.feather);

        result.mask=mask;
        recomputeElementStats(result);

        int totalPixels=w*h;
        int maxPixels=(totalPixels*opts.maxAreaPct)/100;
        if (result.pixelsSelected<opts.minArea || result.pixelsSelected>maxPixels) {
            result.mask=QImage(); result.boundingBox=QRect();
            result.pixelsSelected=0; result.pixelsSoft=0;
        }
        return result;
    }

    static ElementSelectResult runSelectDownsampled(const QImage &image,
                                                    const QPoint &seed,
                                                    const ElementSelectOptions &opts) {
        const int factor=qMax(1, opts.hoverDownsample);
        if (factor<=1) return runSelect(image, seed, opts);
        QSize smallSize(qMax(1,image.width()/factor), qMax(1,image.height()/factor));
        QImage small=image.scaled(smallSize, Qt::IgnoreAspectRatio, Qt::FastTransformation);
        QPoint smallSeed(seed.x()/factor, seed.y()/factor);
        ElementSelectOptions so=opts;
        so.feather=0; so.closeRadius=0; so.fillHoles=false;
        so.useEdgeRefine=false; so.minArea=8;
        so.baseTolerance=opts.hoverTolerance;
        ElementSelectResult sr=runSelect(small, smallSeed, so);
        if (!sr.isValid()) return sr;
        ElementSelectResult br;
        br.mask=sr.mask.scaled(image.size(), Qt::IgnoreAspectRatio, Qt::FastTransformation);
        br.boundingBox=QRect(sr.boundingBox.x()*factor, sr.boundingBox.y()*factor,
                             sr.boundingBox.width()*factor, sr.boundingBox.height()*factor);
        br.pixelsSelected=sr.pixelsSelected*factor*factor;
        br.pixelsSoft=sr.pixelsSoft*factor*factor;
        return br;
    }

public:
    ////////////////////////////////////////////////////////////
    //  Consultas generales
    
    bool          isActive()   const { return active; }
    SelectionType type()       const { return selType; }
    QRect         rect()       const { return selRect; }
    QImage        buffer()     const { return selBuffer; }
    double        rotation()   const { return selRotation; }
    QPainterPath  path()       const { return freePath; }
    bool          hasBuffer()  const { return !selBuffer.isNull(); }

    void setType(SelectionType t) { selType=t; }
    void setActive(bool a)        { active=a; }
    void setRect(const QRect &r)  { selRect=r; }
    void setBuffer(const QImage &b) { selBuffer=b; }
    void setRotation(double r)    { selRotation=r; }
    void setPath(const QPainterPath &p) { freePath=p; }

    void setFreeSubMode(FreeSubMode m) {
        freeSubMode=m;
        if (m!=FreeVector) cancelFreeVectorMode();
        if (m!=FreeElement) clearElementHover();
    }
    FreeSubMode getFreeSubMode() const { return freeSubMode; }

    ////////////////////////////////////////////////////////////
    //  Selección rectangular / libre
    
    void startRect(const QPoint &pos) {
        selRect=QRect(pos,pos); selRotation=0; freePath=QPainterPath();
    }
    void updateRect(const QPoint &start, const QPoint &end, const QSize &cs) {
        selRect=QRect(start,end).normalized().intersected(QRect(QPoint(0,0),cs));
    }
    void startFree(const QPoint &pos) {     //metmer slecio elimpitca 
        selRect=QRect(pos,pos); selRotation=0;
        freePath=QPainterPath(); freePath.moveTo(pos);
    }
    void updateFree(const QPoint &pos, const QSize &cs) {
        freePath.lineTo(pos);
        selRect=freePath.boundingRect().toRect().intersected(QRect(QPoint(0,0),cs));
    }
    void closeFreePath() { freePath.closeSubpath(); }

    ////////////////////////////////////////////////////////////
    //  Selección vectorial libre
   
    bool isFreeVectorMode() const { return freeVectorMode; }
    bool hasFreeVectorPoints() const { return !freeVectorPoints.isEmpty(); }
    void startFreeVectorMode() {
        freeVectorMode=true; freeVectorPoints.clear();
        freeVectorHoverIndex=-1; freeVectorDragIndex=-1;
        selRect=QRect(); freePath=QPainterPath();
    }
    void cancelFreeVectorMode() {
        freeVectorMode=false; freeVectorPoints.clear();
        freeVectorHoverIndex=-1; freeVectorDragIndex=-1;
    }
    void addFreeVectorPoint(const QPointF &p) { freeVectorPoints.append(p); }

    bool closeFreeVectorPath(const QSize &canvasSize) {
        if (freeVectorPoints.size()<3) { cancelFreeVectorMode(); return false; }
        QPainterPath p;
        p.moveTo(freeVectorPoints.first());
        for (int i=1; i<freeVectorPoints.size(); ++i) p.lineTo(freeVectorPoints[i]);
        p.closeSubpath();
        freePath=p;
        selRect=p.boundingRect().toRect().intersected(QRect(QPoint(0,0),canvasSize));
        freeVectorMode=false;
        return selRect.width()>4 && selRect.height()>4;
    }

    int findFreeVectorPointAt(const QPointF &cp, double zoom) const {
        double thr=10.0/qMax(0.0001,zoom);
        for (int i=0; i<freeVectorPoints.size(); ++i) {
            double dx=freeVectorPoints[i].x()-cp.x();
            double dy=freeVectorPoints[i].y()-cp.y();
            if (std::sqrt(dx*dx+dy*dy)<=thr) return i;
        }
        return -1;
    }
    bool isNearFirstFreeVectorPoint(const QPointF &cp, double zoom) const {
        if (freeVectorPoints.size()<3) return false;
        double thr=12.0/qMax(0.0001,zoom);
        double dx=freeVectorPoints.first().x()-cp.x();
        double dy=freeVectorPoints.first().y()-cp.y();
        return std::sqrt(dx*dx+dy*dy)<=thr;
    }
    void setFreeVectorDragIndex(int i) { freeVectorDragIndex=i; }
    int  freeVectorDragIndexValue() const { return freeVectorDragIndex; }
    void moveFreeVectorPoint(int i, const QPointF &p) {
        if (i>=0 && i<freeVectorPoints.size()) freeVectorPoints[i]=p;
    }
    void removeFreeVectorPoint(int i) {
        if (i>=0 && i<freeVectorPoints.size()) freeVectorPoints.removeAt(i);
    }
    const QVector<QPointF> &freeVectorNodes() const { return freeVectorPoints; }
    void setFreeVectorHover(int i) { freeVectorHoverIndex=i; }
    int  freeVectorHoverIndexValue() const { return freeVectorHoverIndex; }

    ////////////////////////////////////////////////////////////
    //  Elementos 
    
    static ElementSelectResult selectElement(const QImage &img, const QPoint &seed,
                                             const ElementSelectOptions &opts=ElementSelectOptions()) {
        return runSelect(img, seed, opts);
    }
    static ElementSelectResult selectElementHover(const QImage &img, const QPoint &seed,
                                                  const ElementSelectOptions &opts=ElementSelectOptions()) {
        return runSelectDownsampled(img, seed, opts);
    }

    bool isElementHoverActive() const { return elementHoverActive; }
    QPoint elementHoverPosValue() const { return elementHoverPos; }
    const ElementSelectResult &elementHoverPreviewValue() const { return elementHoverPreview; }

    void updateElementHover(const QImage &img, const QPoint &pos,
                            const ElementSelectOptions &opts=ElementSelectOptions()) {
        if (img.isNull()) { clearElementHover(); return; }
        elementHoverActive=true; elementHoverPos=pos;
        elementHoverPreview=selectElementHover(img, pos, opts);
        if (!elementHoverPreview.isValid()) elementHoverActive=false;
    }
    void clearElementHover() {
        elementHoverActive=false;
        elementHoverPreview=ElementSelectResult();
    }

    ////////////////////////////////////////////////////////////
    //  Finalizar selecciones
    
    bool finalizeRect(QImage &layerImage) {
        if (selRect.width()<=4||selRect.height()<=4) { active=false; selType=SelectionNone; return false; }
        selBuffer=QImage(selRect.size(), QImage::Format_ARGB32);
        selBuffer.fill(Qt::transparent);
        QPainter dp(&selBuffer); dp.drawImage(0,0,layerImage.copy(selRect)); dp.end();
        QPainter sp(&layerImage);
        sp.setCompositionMode(QPainter::CompositionMode_Clear);
        sp.fillRect(selRect, Qt::transparent); sp.end();
        selRotation=0; selType=SelectionRect; active=true;
        return true;
    }

    bool finalizeFree(QImage &layerImage) {
        selRect=freePath.boundingRect().toRect().intersected(layerImage.rect());
        if (selRect.width()<=4||selRect.height()<=4) {
            active=false; selType=SelectionNone; freePath=QPainterPath(); return false;
        }
        selBuffer=QImage(selRect.size(), QImage::Format_ARGB32);
        selBuffer.fill(Qt::transparent);
        QPainter dp(&selBuffer);
        dp.setClipPath(freePath.translated(-selRect.topLeft()));
        dp.drawImage(0,0,layerImage.copy(selRect)); dp.end();
        QPainter sp(&layerImage);
        sp.setCompositionMode(QPainter::CompositionMode_Clear);
        sp.setClipPath(freePath);
        sp.fillRect(layerImage.rect(), Qt::transparent); sp.end();
        selRotation=0; selType=SelectionPath; active=true;
        return true;
    }

    bool finalizeVectorPath(const QPainterPath &path, QImage &layerImage) {
        QRect bounds=path.boundingRect().toRect().intersected(layerImage.rect());
        if (bounds.width()<=4||bounds.height()<=4) return false;
        selRect=bounds;
        selBuffer=QImage(bounds.size(), QImage::Format_ARGB32);
        selBuffer.fill(Qt::transparent);
        QPainter dp(&selBuffer);
        dp.setClipPath(path.translated(-bounds.topLeft()));
        dp.drawImage(0,0,layerImage.copy(bounds)); dp.end();
        QPainter sp(&layerImage);
        sp.setCompositionMode(QPainter::CompositionMode_Clear);
        sp.setClipPath(path);
        sp.fillRect(layerImage.rect(), Qt::transparent); sp.end();
        selRotation=0; selType=SelectionPath; active=true;
        return true;
    }

    bool finalizeElement(const ElementSelectResult &r, QImage &layerImage) {
        if (!r.isValid()) return false;
        const QRect bbox=r.boundingBox.intersected(layerImage.rect());
        if (bbox.width()<=1||bbox.height()<=1) return false;
        selRect=bbox;
        selBuffer=QImage(bbox.size(), QImage::Format_ARGB32);
        selBuffer.fill(Qt::transparent);
        QImage src=layerImage;
        if (src.format()!=QImage::Format_ARGB32) src=src.convertToFormat(QImage::Format_ARGB32);
        QImage msk=r.mask;
        if (msk.format()!=QImage::Format_ARGB32) msk=msk.convertToFormat(QImage::Format_ARGB32);
        for (int y=0; y<bbox.height(); ++y) {
            const QRgb *sl=(const QRgb*)src.constScanLine(bbox.y()+y);
            const QRgb *ml=(const QRgb*)msk.constScanLine(bbox.y()+y);
            QRgb *dl=(QRgb*)selBuffer.scanLine(y);
            for (int x=0; x<bbox.width(); ++x) {
                int sx=bbox.x()+x, ma=qAlpha(ml[sx]);
                if (ma<=0) continue;
                QRgb px=sl[sx]; int sa=qAlpha(px), na=(sa*ma)/255;
                if (na<=0) continue;
                dl[x]=qRgba(qRed(px),qGreen(px),qBlue(px),na);
            }
        }
        QPainter sp(&layerImage);
        sp.setCompositionMode(QPainter::CompositionMode_DestinationOut);
        sp.drawImage(0,0,r.mask); sp.end();
        selRotation=0; selType=SelectionElement; active=true;
        clearElementHover();
        return true;
    }

    // //////////////////////////////////////////////////////////
    //  VARITA MÁGICA — API pública
    

    bool finalizeMagicWand(QImage &layerImage, const QPoint &pos,
                           const WandConfig &cfg = WandConfig()) {
        ElementSelectResult result = runMagicWand(layerImage, pos, cfg);
        if (!result.isValid()) return false;

        if (cfg.edgeRefine != 0.0) {
            result.mask = refineMaskWithEdges(result.mask, layerImage, cfg.edgeRefine);
            recomputeElementStats(result);
        }
        if (cfg.feather > 0) {
            featherMask(result.mask, cfg.feather);
            recomputeElementStats(result);
        }

        QImage extracted = extractMaskedRegion(layerImage, result.mask, result.boundingBox);
        clearMaskedRegion(layerImage, result.mask);

        selRect     = result.boundingBox;
        selBuffer   = extracted;
        selType     = SelectionMagicWand;
        active      = true;
        selRotation = 0.0;
        freePath    = QPainterPath();
        return true;
    }

    static ElementSelectResult applyMagicWandStatic(const QImage &image,
                                                    const QPoint &pos,
                                                    const WandConfig &cfg = WandConfig()) {
        return runMagicWand(image, pos, cfg);
    }

    void applyMagicWand(const QRect &bbox, const QImage &extracted) {
        selRect=bbox; selBuffer=extracted;
        selType=SelectionMagicWand; active=true;
        selRotation=0; freePath=QPainterPath();
    }

    static int autoTolerance(const QImage &img, const QPoint &pos, int radius=5) {
        return estimateAutoTolerance(img, pos, radius);
    }

    ////////////////////////////////////////////////////////////
    //  Pegar / Descartar / Hornear
    
    void pasteAsSelection(const QImage &img, const QPoint &pos=QPoint(20,20)) {
        selBuffer=img;
        selRect=QRect(pos.x(), pos.y(), img.width(), img.height());
        selRotation=0; active=true; selType=SelectionRect;
    }

    void discard() {
        active=false; dragging=false; resizing=false; rotating=false;
        selRotation=0; selBuffer=QImage(); freePath=QPainterPath();
        resizeHandle=ObjectHandle::None; selType=SelectionNone;
        cancelFreeVectorMode(); clearElementHover();
    }

    bool bake(QImage &layerImage, bool antialias) {
        if (!active||selBuffer.isNull()) return false;
        QPainter p(&layerImage);
        p.setRenderHint(QPainter::Antialiasing, antialias);
        p.setRenderHint(QPainter::SmoothPixmapTransform, antialias);
        p.translate(selRect.center());
        p.rotate(selRotation);
        p.drawImage(QRect(-selRect.width()/2, -selRect.height()/2,
                          selRect.width(), selRect.height()), selBuffer);
        p.end();
        return true;
    }

    ////////////////////////////////////////////////////////////
    //  Drag / Resize / Rotate
    
    void setDragging(bool d) { dragging=d; }
    bool isDragging() const { return dragging; }
    void setDragOffset(const QPoint &off) { dragOffset=off; }
    QPoint dragOffsetValue() const { return dragOffset; }
    void moveTo(const QPoint &pos) { selRect.moveTo(pos-dragOffset); }

    void setResizing(bool r) { resizing=r; }
    bool isResizing() const { return resizing; }
    void setResizeHandle(ObjectHandle h) { resizeHandle=h; }
    ObjectHandle activeResizeHandle() const { return resizeHandle; }
    void setResizeStart(const QRectF &r, const QPointF &p) { resizeStartRect=r; resizeStartPos=p; }
    QRectF resizeStartRectValue() const { return resizeStartRect; }
    QPointF resizeStartPosValue() const { return resizeStartPos; }
    void applyResize(ObjectHandle handle, const QPointF &cur, bool keepAspect, double minSize=5.0) {
        QRectF nr=resizeRotatedRect(handle, resizeStartRect, selRotation,
                                    resizeStartPos, cur, keepAspect, minSize);
        selRect=nr.toAlignedRect();
    }

    void setRotating(bool r) { rotating=r; }
    bool isRotating() const { return rotating; }
    void updateRotation(const QPoint &pos) {
        double dy=pos.y()-selRect.center().y();
        double dx=pos.x()-selRect.center().x();
        selRotation=atan2(dy,dx)*180.0/M_PI+90.0;
    }
    void snapRotation(double step=15.0) {
        selRotation=qRound(selRotation/step)*step;
    }

    GizmoLayout gizmoLayout(double zoom) const {
        return computeGizmoLayout(QRectF(selRect), selRotation, zoom);
    }
    QPointF convertHandlePos(double zoom) const {
        GizmoLayout L=gizmoLayout(zoom);
        return L.midTop + L.upDir*(52.0/zoom);
    }
    ObjectHandle hitTestGizmoAt(const QPointF &cp, double zoom) const {
        return hitTestGizmo(gizmoLayout(zoom), cp, zoom);
    }
    bool isOverConvertHandle(const QPointF &cp, double zoom) const {
        return QLineF(cp, convertHandlePos(zoom)).length() <= 12.0/zoom;
    }

    ////////////////////////////////////////////////////////////
    //  Objetos
    
    int  objectCount() const { return objects.size(); }
    bool hasObjects()  const { return !objects.isEmpty(); }
    PaintObject       &objectAt(int i)       { return objects[i]; }
    const PaintObject &objectAt(int i) const { return objects[i]; }
    int  activeObjectIndex() const { return activeObjIndex; }
    void setActiveObjectIndex(int i) { activeObjIndex=i; }
    QList<PaintObject>       &allObjects()       { return objects; }
    const QList<PaintObject> &allObjects() const { return objects; }
    void addObject(const PaintObject &o) { objects.append(o); }

    void removeObjectAt(int i) {
        if (i<0||i>=objects.size()) return;
        if (objects[i].type==ObjectType::SelectionImage)
            selObjBuffers.remove(objects[i].selectionBufferId);
        objects.removeAt(i);
        if (activeObjIndex>=objects.size()) activeObjIndex=-1;
    }
    void clearObjects() {
        objects.clear(); activeObjIndex=-1; selObjBuffers.clear();
    }

    int findObjectAt(const QPointF &cp) const {
        for (int i=objects.size()-1; i>=0; --i)
            if (objectContainsPoint(objects[i], cp)) return i;
        return -1;
    }

    void selectObject(int i, bool add=false) {
        if (i<0||i>=objects.size()) return;
        if (!add) { for (int j=0; j<objects.size(); ++j) objects[j].selected=(j==i); }
        else objects[i].selected=!objects[i].selected;
        activeObjIndex=i;
    }
    void deselectAllObjects() {
        for (PaintObject &o : objects) o.selected=false;
        activeObjIndex=-1;
    }
    QList<int> selectedObjectIndices() const {
        QList<int> r;
        for (int i=0; i<objects.size(); ++i) if (objects[i].selected) r.append(i);
        return r;
    }
    void deleteSelectedObjects() {
        for (int i=objects.size()-1; i>=0; --i) {
            if (objects[i].selected) {
                if (objects[i].type==ObjectType::SelectionImage)
                    selObjBuffers.remove(objects[i].selectionBufferId);
                objects.removeAt(i);
            }
        }
        activeObjIndex=-1;
    }

    int registerShapeObject(ToolType tool, const QPoint &p1, const QPoint &p2,
                            const QColor &fill, const QColor &stroke,
                            int strokeW, int layerIndex) {
        PaintObject o;
        o.type=ObjectType::Shape; o.shapeTool=tool;
        o.bounds=QRectF(QRect(p1,p2).normalized());
        o.fillColor=fill; o.strokeColor=stroke; o.strokeWidth=strokeW;
        o.startPoint=p1; o.endPoint=p2; o.layerIndex=layerIndex; o.selected=false;
        objects.append(o); return objects.size()-1;
    }

    int registerTextObject(const QRect &rect, const QString &text,
                           const QFont &font, const QColor &color, int layerIndex) {
        PaintObject o;
        o.type=ObjectType::Text; o.bounds=QRectF(rect);
        o.textContent=text; o.textFont=font; o.textColor=color;
        o.layerIndex=layerIndex; o.selected=false;
        objects.append(o); return objects.size()-1;
    }

    int registerSelectionImageObject(const QRect &rect, const QImage &buffer,
                                     double rotation, int layerIndex) {
        PaintObject o;
        o.type=ObjectType::SelectionImage; o.bounds=QRectF(rect);
        o.layerIndex=layerIndex; o.selected=true;
        o.rotation=rotation; o.scaleX=1; o.scaleY=1;
        o.selectionBufferId=nextSelBufferId++;
        selObjBuffers.insert(o.selectionBufferId, buffer);
        objects.append(o); activeObjIndex=objects.size()-1;
        return activeObjIndex;
    }

       
    int registerBufferForLoad(const QImage &buf) {
        int id = nextSelBufferId++;
        selObjBuffers.insert(id, buf);
        return id;
    }

    QImage selectionBufferFor(int id) const { return selObjBuffers.value(id); }
    bool   hasSelectionBuffer(int id) const { return selObjBuffers.contains(id); }

    // //////////////////////////////////////////////////////////
    //  Render / Bake objetos
 
    QImage renderSelectedObjects() const {
        QList<int> sel=selectedObjectIndices();
        if (sel.isEmpty()) return QImage();
        QRectF tb;
        for (int i : sel) {
            QRectF b=objects[i].transformedBounds();
            tb=tb.isNull()?b:tb.united(b);
        }
        if (tb.isEmpty()) return QImage();
        QRect r=tb.toAlignedRect();
        QImage img(r.size(), QImage::Format_ARGB32);
        img.fill(Qt::transparent);
        QPainter p(&img);
        p.setRenderHint(QPainter::Antialiasing,true);
        p.setRenderHint(QPainter::SmoothPixmapTransform,true);
        p.setRenderHint(QPainter::TextAntialiasing,true);
        p.translate(-r.topLeft());
        for (int i : sel) drawObjectInternal(p, objects[i]);
        p.end();
        return img;
    }

    void drawObjectInternal(QPainter &painter, const PaintObject &obj) const {
        painter.save();
        if (obj.type==ObjectType::Shape) {
            renderShapeObject(painter, obj);
        } else if (obj.type==ObjectType::Text) {
            if (qAbs(obj.rotation)>0.001||qAbs(obj.scaleX-1)>0.001||qAbs(obj.scaleY-1)>0.001) {
                painter.translate(obj.bounds.center());
                painter.rotate(obj.rotation);
                painter.scale(obj.scaleX,obj.scaleY);
                painter.translate(-obj.bounds.center());
            }
            painter.setFont(obj.textFont); painter.setPen(obj.textColor);
            QTextOption opt;
            opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
            opt.setAlignment(Qt::AlignLeft|Qt::AlignTop);
            painter.drawText(obj.bounds.toRect(), obj.textContent, opt);
        } else if (obj.type==ObjectType::SelectionImage) {
            if (selObjBuffers.contains(obj.selectionBufferId)) {
                const QImage &buf=selObjBuffers.value(obj.selectionBufferId);
                painter.translate(obj.bounds.center());
                painter.rotate(obj.rotation);
                painter.scale(obj.scaleX,obj.scaleY);
                painter.translate(-obj.bounds.center());
                painter.setRenderHint(QPainter::SmoothPixmapTransform,true);
                painter.drawImage(obj.bounds.toRect(), buf);
            }
        }
        painter.restore();
    }

    void drawAllObjects(QPainter &painter) const {
        for (const PaintObject &o : objects) drawObjectInternal(painter, o);
    }

    void bakeObjectInto(int idx, QImage &layerImage, bool antialias) {
        if (idx<0||idx>=objects.size()) return;
        const PaintObject &obj=objects[idx];
        QPainter p(&layerImage);
        p.setRenderHint(QPainter::Antialiasing, antialias);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p.setRenderHint(QPainter::TextAntialiasing, true);
        if (obj.type==ObjectType::Shape) {
            renderShapeObject(p, obj);
        } else if (obj.type==ObjectType::Text) {
            if (qAbs(obj.rotation)>0.001||qAbs(obj.scaleX-1)>0.001||qAbs(obj.scaleY-1)>0.001) {
                p.translate(obj.bounds.center()); p.rotate(obj.rotation);
                p.scale(obj.scaleX,obj.scaleY); p.translate(-obj.bounds.center());
            }
            p.setFont(obj.textFont); p.setPen(obj.textColor);
            QTextOption opt;
            opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
            opt.setAlignment(Qt::AlignLeft|Qt::AlignTop);
            p.drawText(obj.bounds.toRect(), obj.textContent, opt);
        } else if (obj.type==ObjectType::SelectionImage) {
            if (selObjBuffers.contains(obj.selectionBufferId)) {
                const QImage &buf=selObjBuffers.value(obj.selectionBufferId);
                p.translate(obj.bounds.center()); p.rotate(obj.rotation);
                p.scale(obj.scaleX,obj.scaleY); p.translate(-obj.bounds.center());
                p.drawImage(obj.bounds.toRect(), buf);
            }
        }
        p.end();
    }

    ////////////////////////////////////////////////////////////
    //  Manipulación de objetos
 
    bool isObjectDragging()  const { return objDragging; }
    bool isObjectRotating()  const { return objRotating; }
    bool isObjectScaling()   const { return objScaling; }
    ObjectHandle objectActiveHandle() const { return objActiveHandle; }
    void setObjectDragging(bool d)  { objDragging=d; }
    void setObjectRotating(bool r)  { objRotating=r; }
    void setObjectScaling(bool s)   { objScaling=s; }
    void setObjectActiveHandle(ObjectHandle h) { objActiveHandle=h; }
    QPointF objectDragStartPos() const { return objDragStart; }
    void setObjectDragStart(const QPointF &p) { objDragStart=p; }
    double objectRotationStart() const { return objRotationStart; }
    void setObjectRotationStart(double r) { objRotationStart=r; }
    double objectScaleStartX() const { return objScaleStartX; }
    double objectScaleStartY() const { return objScaleStartY; }
    void setObjectScaleStart(double sx, double sy) { objScaleStartX=sx; objScaleStartY=sy; }
    QRectF objectBoundsStart() const { return objBoundsStart; }
    void setObjectBoundsStart(const QRectF &r) { objBoundsStart=r; }

    ObjectHandle findObjectGizmoHandleAt(int idx, const QPointF &cp, double zoom) const {
        if (idx<0||idx>=objects.size()) return ObjectHandle::None;
        return ::findObjectGizmoHandle(objects[idx], cp, zoom);
    }

    void moveActiveObject(const QPointF &cp) {
        if (activeObjIndex<0||activeObjIndex>=objects.size()) return;
        objects[activeObjIndex].bounds = objBoundsStart.translated(cp-objDragStart);
    }

    void rotateActiveObject(const QPointF &cp, bool snap15) {
        if (activeObjIndex<0||activeObjIndex>=objects.size()) return;
        PaintObject &o=objects[activeObjIndex];
        QPointF c=o.bounds.center();
        double a1=atan2(objDragStart.y()-c.y(), objDragStart.x()-c.x());
        double a2=atan2(cp.y()-c.y(), cp.x()-c.x());
        o.rotation=objRotationStart+(a2-a1)*180.0/M_PI;
        if (snap15) o.rotation=qRound(o.rotation/15.0)*15.0;
    }

    void scaleActiveObject(const QPointF &cp, bool keepAspect) {
        if (activeObjIndex<0||activeObjIndex>=objects.size()) return;
        PaintObject &o=objects[activeObjIndex];
        double w0=objBoundsStart.width()*objScaleStartX;
        double h0=objBoundsStart.height()*objScaleStartY;
        QRectF v0(objBoundsStart.center().x()-w0/2, objBoundsStart.center().y()-h0/2, w0, h0);
        QRectF nv=resizeRotatedRect(objActiveHandle, v0, o.rotation,
                                    objDragStart, cp, keepAspect, 5.0);
        o.scaleX=qBound(0.05, nv.width()/qMax(0.001,objBoundsStart.width()), 50.0);
        o.scaleY=qBound(0.05, nv.height()/qMax(0.001,objBoundsStart.height()), 50.0);
        QPointF nc=nv.center();
        o.bounds=QRectF(nc.x()-objBoundsStart.width()/2,
                        nc.y()-objBoundsStart.height()/2,
                        objBoundsStart.width(), objBoundsStart.height());
    }

    void stopObjectManipulation() {
        objDragging=false; objRotating=false; objScaling=false;
        objActiveHandle=ObjectHandle::None;
    }

    PaintObject takeObjectAt(int idx) {
        PaintObject o=objects[idx];
        objects.removeAt(idx);
        if (activeObjIndex>=objects.size()) activeObjIndex=-1;
        return o;
    }

    ////////////////////////////////////////////////////////////
    //  Portapapeles

    void   setClipboardBuffer(const QImage &img) { clipboardBuffer=img; }
    QImage clipboardBufferValue() const { return clipboardBuffer; }
    bool   hasClipboardBuffer() const { return !clipboardBuffer.isNull(); }

    bool copySelectionToClipboard() {
        QList<int> sel=selectedObjectIndices();
        if (!sel.isEmpty()) {
            QImage rendered=renderSelectedObjects();
            if (!rendered.isNull()) {
                clipboardBuffer=rendered;
                QApplication::clipboard()->setImage(rendered);
                return true;
            }
        }
        if (active&&!selBuffer.isNull()) {
            clipboardBuffer=selBuffer;
            QApplication::clipboard()->setImage(selBuffer);
            return true;
        }
        return false;
    }

    bool cutSelectionToClipboard() {
        QList<int> sel=selectedObjectIndices();
        if (!sel.isEmpty()) {
            QImage rendered=renderSelectedObjects();
            if (!rendered.isNull()) {
                clipboardBuffer=rendered;
                QApplication::clipboard()->setImage(rendered);
                deleteSelectedObjects();
                return true;
            }
        }
        if (active&&!selBuffer.isNull()) {
            clipboardBuffer=selBuffer;
            QApplication::clipboard()->setImage(selBuffer);
            discard();
            return true;
        }
        return false;
    }

    QImage getImageToPaste() const {
        QImage sys=QApplication::clipboard()->image();
        if (!sys.isNull()) return sys.convertToFormat(QImage::Format_ARGB32);
        if (!clipboardBuffer.isNull()) return clipboardBuffer;
        return QImage();
    }

    ////////////////////////////////////////////////////////////
    //  Overlays
   
    void drawSelectionOverlay(QPainter &p, double zoom) const {
        if (!active||selBuffer.isNull()) return;
        p.save();
        p.translate(selRect.center());
        p.rotate(selRotation);
        p.setRenderHint(QPainter::SmoothPixmapTransform,true);
        p.drawImage(QRect(-selRect.width()/2,-selRect.height()/2,
                          selRect.width(),selRect.height()), selBuffer);
        p.restore();
    }

    void drawSelectionGizmo(QPainter &p, double zoom, bool darkMode,
                            const QColor &selBlue, const QColor &selBlueLight) const {
        if (!active||selBuffer.isNull()) return;
        GizmoLayout L=gizmoLayout(zoom);
        drawGizmoBase(p, L, zoom, darkMode);
        QPointF convPos=convertHandlePos(zoom);
        p.setPen(QPen(selBlue, 1.0/zoom, Qt::SolidLine));
        p.drawLine(L.rotatePos, convPos);
        double cs=20.0/zoom;
        QRectF cr(convPos.x()-cs/2, convPos.y()-cs/2, cs, cs);
        p.setPen(QPen(selBlue, 1.0/zoom));
        p.setBrush(selBlueLight);
        p.drawRoundedRect(cr, 4.0/zoom, 4.0/zoom);
        p.setPen(Qt::white);
        QFont fO; fO.setPixelSize(qMax(8,(int)(10.0/zoom))); fO.setBold(true);
        p.setFont(fO);
        p.drawText(cr, Qt::AlignCenter, "O");
    }

    void drawObjectGizmos(QPainter &p, double zoom, bool darkMode) const {
        for (int i=0; i<objects.size(); ++i)
            if (objects[i].selected)
                drawObjectGizmo(p, objects[i], zoom, darkMode);
    }

    void paintFreeVectorOverlay(QPainter &p, const QPointF &hoverPos,
                                double zoom, const QColor &accent) const {
        if (!freeVectorMode||freeVectorPoints.isEmpty()) return;
        double z=qMax(0.0001,zoom);
        p.save();
        p.setRenderHint(QPainter::Antialiasing,true);
        QPainterPath path;
        path.moveTo(freeVectorPoints.first());
        for (int i=1; i<freeVectorPoints.size(); ++i) path.lineTo(freeVectorPoints[i]);
        p.setPen(QPen(accent, 2.0/z, Qt::SolidLine));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);
        p.setPen(QPen(accent, 1.5/z, Qt::DashLine));
        p.drawLine(freeVectorPoints.last(), hoverPos);
        if (isNearFirstFreeVectorPoint(hoverPos,zoom) && freeVectorPoints.size()>=3) {
            p.setPen(QPen(QColor(0,200,100), 2.5/z));
            p.drawEllipse(freeVectorPoints.first(), 7.0/z, 7.0/z);
        }
        for (int i=0; i<freeVectorPoints.size(); ++i) {
            bool sel=(i==freeVectorDragIndex);
            p.setPen(QPen(Qt::white, 1.5/z));
            p.setBrush(sel?QColor(239,68,68):accent);
            double r=(i==0)?5.0/z:4.0/z;
            p.drawEllipse(freeVectorPoints[i], r, r);
        }
        p.restore();
    }

    void paintElementHoverOverlay(QPainter &p, double zoom,
                                  const QColor &accent) const {
        if (!elementHoverActive||!elementHoverPreview.isValid()) return;
        if (elementHoverPreview.mask.isNull()) return;
        double z=qMax(0.0001,zoom);
        p.save();
        p.setRenderHint(QPainter::Antialiasing,false);
        QImage tinted=elementHoverPreview.mask;
        QPainter tp(&tinted);
        tp.setCompositionMode(QPainter::CompositionMode_SourceIn);
        QColor tint=accent; tint.setAlpha(100);
        tp.fillRect(tinted.rect(), tint);
        tp.end();
        p.drawImage(0,0,tinted);
        p.setPen(QPen(accent, 1.5/z, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawRect(elementHoverPreview.boundingBox);
        p.restore();
    }

    void clearAll() {
        discard(); clearObjects(); clipboardBuffer=QImage();
    }
};

#endif // SELECTTOOLS_H