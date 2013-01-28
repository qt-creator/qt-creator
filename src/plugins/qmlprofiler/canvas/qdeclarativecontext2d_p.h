/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QDECLARATIVECONTEXT2D_P_H
#define QDECLARATIVECONTEXT2D_P_H

#include <qpainter.h>
#include <qpainterpath.h>
#include <qpixmap.h>
#include <qstring.h>
#include <qstack.h>
#include <qmetatype.h>
#include <qcoreevent.h>
#include <qvariant.h>
#include <qscriptvalue.h>

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

QColor colorFromString(const QString &name);

class CanvasGradient : public QObject
{
    Q_OBJECT
public:
    CanvasGradient(const QGradient &gradient) : m_gradient(gradient) {}

public slots:
    QGradient value() { return m_gradient; }
    void addColorStop(float pos, const QString &color) { m_gradient.setColorAt(pos, colorFromString(color));}

public:
    QGradient m_gradient;
};

class CanvasImage: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString src READ src WRITE setSrc NOTIFY sourceChanged)
    Q_PROPERTY(int width READ width)
    Q_PROPERTY(int height READ height)

public:
    CanvasImage() {}
    CanvasImage(const QString &url) : m_image(url), m_src(url) {}
    CanvasImage(const QPixmap &pixmap) {m_image = pixmap;}

public slots:
    int width() { return m_image.width(); }
    int height() { return m_image.height(); }
    QPixmap &value() { return m_image; }
    QString src() { return m_src; }
    void setSrc(const QString &src) { m_src = src; m_image.load(src); emit sourceChanged();}
signals:
    void sourceChanged();

private:
    QPixmap m_image;
    QString m_src;
};


class ImageData {
};

class Context2D : public QObject
{
    Q_OBJECT
    // compositing
    Q_PROPERTY(qreal globalAlpha READ globalAlpha WRITE setGlobalAlpha)
    Q_PROPERTY(QString globalCompositeOperation READ globalCompositeOperation WRITE setGlobalCompositeOperation)
    Q_PROPERTY(QVariant strokeStyle READ strokeStyle WRITE setStrokeStyle)
    Q_PROPERTY(QVariant fillStyle READ fillStyle WRITE setFillStyle)
    // line caps/joins
    Q_PROPERTY(qreal lineWidth READ lineWidth WRITE setLineWidth)
    Q_PROPERTY(QString lineCap READ lineCap WRITE setLineCap)
    Q_PROPERTY(QString lineJoin READ lineJoin WRITE setLineJoin)
    Q_PROPERTY(qreal miterLimit READ miterLimit WRITE setMiterLimit)
    // shadows
    Q_PROPERTY(qreal shadowOffsetX READ shadowOffsetX WRITE setShadowOffsetX)
    Q_PROPERTY(qreal shadowOffsetY READ shadowOffsetY WRITE setShadowOffsetY)
    Q_PROPERTY(qreal shadowBlur READ shadowBlur WRITE setShadowBlur)
    Q_PROPERTY(QString shadowColor READ shadowColor WRITE setShadowColor)
    // fonts
    Q_PROPERTY(QString font READ font WRITE setFont)
    Q_PROPERTY(QString textBaseline READ textBaseline WRITE setTextBaseline)
    Q_PROPERTY(QString textAlign READ textAlign WRITE setTextAlign)

    enum TextBaseLine { Alphabetic=0, Top, Middle, Bottom, Hanging};
    enum TextAlign { Start=0, End, Left, Right, Center};

public:
    Context2D(QObject *parent = 0);
    void setSize(int width, int height);
    void setSize(const QSize &size);
    QSize size() const;

    QPoint painterTranslate() const;
    void setPainterTranslate(const QPoint &);

    void scheduleChange();
    void timerEvent(QTimerEvent *e);

    void clear();
    void reset();

    QPixmap pixmap() { return m_pixmap; }

    // compositing
    qreal globalAlpha() const; // (default 1.0)
    QString globalCompositeOperation() const; // (default over)
    QVariant strokeStyle() const; // (default black)
    QVariant fillStyle() const; // (default black)

    void setGlobalAlpha(qreal alpha);
    void setGlobalCompositeOperation(const QString &op);
    void setStrokeStyle(const QVariant &style);
    void setFillStyle(const QVariant &style);

    // line caps/joins
    qreal lineWidth() const; // (default 1)
    QString lineCap() const; // "butt", "round", "square" (default "butt")
    QString lineJoin() const; // "round", "bevel", "miter" (default "miter")
    qreal miterLimit() const; // (default 10)

    void setLineWidth(qreal w);
    void setLineCap(const QString &s);
    void setLineJoin(const QString &s);
    void setMiterLimit(qreal m);

    void setFont(const QString &font);
    QString font();
    void setTextBaseline(const QString &font);
    QString textBaseline();
    void setTextAlign(const QString &font);
    QString textAlign();

    // shadows
    qreal shadowOffsetX() const; // (default 0)
    qreal shadowOffsetY() const; // (default 0)
    qreal shadowBlur() const; // (default 0)
    QString shadowColor() const; // (default black)

    void setShadowOffsetX(qreal x);
    void setShadowOffsetY(qreal y);
    void setShadowBlur(qreal b);
    void setShadowColor(const QString &str);

    struct MouseArea {
        QScriptValue callback;
        QScriptValue data;
        QRectF rect;
        QMatrix matrix;
    };
    const QList<MouseArea> &mouseAreas() const;

public slots:
    void save(); // push state on state stack
    void restore(); // pop state stack and restore state

    void fillText(const QString &text, qreal x, qreal y);
    void strokeText(const QString &text, qreal x, qreal y);

    void setInPaint(bool val){m_inPaint = val;}
    void scale(qreal x, qreal y);
    void rotate(qreal angle);
    void translate(qreal x, qreal y);
    void transform(qreal m11, qreal m12, qreal m21, qreal m22,
                   qreal dx, qreal dy);
    void setTransform(qreal m11, qreal m12, qreal m21, qreal m22,
                      qreal dx, qreal dy);

    CanvasGradient *createLinearGradient(qreal x0, qreal y0,
                                         qreal x1, qreal y1);
    CanvasGradient *createRadialGradient(qreal x0, qreal y0,
                                         qreal r0, qreal x1,
                                         qreal y1, qreal r1);

    // rects
    void clearRect(qreal x, qreal y, qreal w, qreal h);
    void fillRect(qreal x, qreal y, qreal w, qreal h);
    void strokeRect(qreal x, qreal y, qreal w, qreal h);

    // mouse
    void mouseArea(qreal x, qreal y, qreal w, qreal h, const QScriptValue &, const QScriptValue & = QScriptValue());

    // path API
    void beginPath();
    void closePath();
    void moveTo(qreal x, qreal y);
    void lineTo(qreal x, qreal y);
    void quadraticCurveTo(qreal cpx, qreal cpy, qreal x, qreal y);
    void bezierCurveTo(qreal cp1x, qreal cp1y,
                       qreal cp2x, qreal cp2y, qreal x, qreal y);
    void arcTo(qreal x1, qreal y1, qreal x2, qreal y2, qreal radius);
    void rect(qreal x, qreal y, qreal w, qreal h);
    void arc(qreal x, qreal y, qreal radius,
             qreal startAngle, qreal endAngle,
             bool anticlockwise);
    void fill();
    void stroke();
    void clip();
    bool isPointInPath(qreal x, qreal y) const;

    CanvasImage *createImage(const QString &url);

    // drawing images (no overloads due to QTBUG-11604)
    void drawImage(const QVariant &var, qreal dx, qreal dy, qreal dw, qreal dh);

    // pixel manipulation
    ImageData getImageData(qreal sx, qreal sy, qreal sw, qreal sh);
    void putImageData(ImageData image, qreal dx, qreal dy);
    void endPainting();

signals:
    void changed();

private:
    void setupPainter();
    void beginPainting();
    void updateShadowBuffer();

    int m_changeTimerId;
    QPainterPath m_path;

    enum DirtyFlag {
        DirtyTransformationMatrix = 0x00001,
        DirtyClippingRegion       = 0x00002,
        DirtyStrokeStyle          = 0x00004,
        DirtyFillStyle            = 0x00008,
        DirtyGlobalAlpha          = 0x00010,
        DirtyLineWidth            = 0x00020,
        DirtyLineCap              = 0x00040,
        DirtyLineJoin             = 0x00080,
        DirtyMiterLimit           = 0x00100,
        MDirtyPen                 = DirtyStrokeStyle
                                    | DirtyLineWidth
                                    | DirtyLineCap
                                    | DirtyLineJoin
                                    | DirtyMiterLimit,
                                    DirtyShadowOffsetX        = 0x00200,
                                    DirtyShadowOffsetY        = 0x00400,
                                    DirtyShadowBlur           = 0x00800,
                                    DirtyShadowColor          = 0x01000,
                                    DirtyGlobalCompositeOperation = 0x2000,
                                    DirtyFont                 = 0x04000,
                                    DirtyTextAlign            = 0x08000,
                                    DirtyTextBaseline         = 0x10000,
                                    AllIsFullOfDirt           = 0xfffff
                                                            };

    struct State {
        State() : flags(0) {}
        QMatrix matrix;
        QPainterPath clipPath;
        QBrush strokeStyle;
        QBrush fillStyle;
        qreal globalAlpha;
        qreal lineWidth;
        Qt::PenCapStyle lineCap;
        Qt::PenJoinStyle lineJoin;
        qreal miterLimit;
        qreal shadowOffsetX;
        qreal shadowOffsetY;
        qreal shadowBlur;
        QColor shadowColor;
        QPainter::CompositionMode globalCompositeOperation;
        QFont font;
        Context2D::TextAlign textAlign;
        Context2D::TextBaseLine textBaseline;
        int flags;
    };

    int baseLineOffset(Context2D::TextBaseLine value, const QFontMetrics &metrics);
    int textAlignOffset(Context2D::TextAlign value, const QFontMetrics &metrics, const QString &string);

    QMatrix worldMatrix() const;

    QPoint m_painterTranslate;
    State m_state;
    QStack<State> m_stateStack;
    QPixmap m_pixmap;
    QList<MouseArea> m_mouseAreas;
    QImage m_shadowbuffer;
    QVector<QRgb> m_shadowColorIndexBuffer;
    QColor m_shadowColorBuffer;
    QPainter m_painter;
    int m_width, m_height;
    bool m_inPaint;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(CanvasImage*)
Q_DECLARE_METATYPE(CanvasGradient*)

#endif // QDECLARATIVECONTEXT2D_P_H
