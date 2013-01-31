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

#include "qdeclarativecontext2d_p.h"

#include "qdeclarativecanvas_p.h"

#include <qdebug.h>
#include <math.h>

#include <qgraphicsitem.h>
#include <qapplication.h>
#include <qgraphicseffect.h>

#include <QImage>

QT_BEGIN_NAMESPACE

static const double Q_PI   = 3.14159265358979323846;   // pi

class CustomDropShadowEffect : public QGraphicsDropShadowEffect
{
public:
    void draw(QPainter *painter) { QGraphicsDropShadowEffect::draw(painter);}
    void drawSource(QPainter *painter) { QGraphicsDropShadowEffect::drawSource(painter);}
};

// Note, this is exported but in a private header as qtopengl depends on it.
// But it really should be considered private API
void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);
void qt_blurImage(QImage &blurImage, qreal radius, bool quality, int transposed = 0);

#define DEGREES(t) ((t) * 180.0 / Q_PI)

#define qClamp(val, min, max) qMin(qMax(val, min), max)
static QList<qreal> parseNumbersList(QString::const_iterator &itr)
{
    QList<qreal> points;
    QString temp;
    while ((*itr).isSpace())
        ++itr;
    while ((*itr).isNumber() ||
           (*itr) == QLatin1Char('-') || (*itr) == QLatin1Char('+') || (*itr) == QLatin1Char('.')) {
        temp.clear();

        if ((*itr) == QLatin1Char('-'))
            temp += *itr++;
        else if ((*itr) == QLatin1Char('+'))
            temp += *itr++;
        while ((*itr).isDigit())
            temp += *itr++;
        if ((*itr) == QLatin1Char('.'))
            temp += *itr++;
        while ((*itr).isDigit())
            temp += *itr++;
        while ((*itr).isSpace())
            ++itr;
        if ((*itr) == QLatin1Char(','))
            ++itr;
        points.append(temp.toDouble());
        //eat spaces
        while ((*itr).isSpace())
            ++itr;
    }

    return points;
}

QColor colorFromString(const QString &name)
{
    QString::const_iterator itr = name.constBegin();
    QList<qreal> compo;
    if (name.startsWith(QLatin1String("rgba("))) {
        ++itr; ++itr; ++itr; ++itr; ++itr;
        compo = parseNumbersList(itr);
        if (compo.size() != 4)
            return QColor();
        //alpha seems to be always between 0-1
        compo[3] *= 255;
        return QColor((int)compo[0], (int)compo[1],
                      (int)compo[2], (int)compo[3]);
    } else if (name.startsWith(QLatin1String("rgb("))) {
        ++itr; ++itr; ++itr; ++itr;
        compo = parseNumbersList(itr);
        if (compo.size() != 3)
            return QColor();
        return QColor((int)qClamp(compo[0], qreal(0), qreal(255)),
                      (int)qClamp(compo[1], qreal(0), qreal(255)),
                      (int)qClamp(compo[2], qreal(0), qreal(255)));
    } else if (name.startsWith(QLatin1String("hsla("))) {
        ++itr; ++itr; ++itr; ++itr; ++itr;
        compo = parseNumbersList(itr);
        if (compo.size() != 4)
            return QColor();
        return QColor::fromHslF(compo[0], compo[1],
                                compo[2], compo[3]);
    } else if (name.startsWith(QLatin1String("hsl("))) {
        ++itr; ++itr; ++itr; ++itr; ++itr;
        compo = parseNumbersList(itr);
        if (compo.size() != 3)
            return QColor();
        return QColor::fromHslF(compo[0], compo[1],
                                compo[2]);
    } else {
        //QRgb color;
        //CSSParser::parseColor(name, color);
        return QColor(name);
    }
}


static QPainter::CompositionMode compositeOperatorFromString(const QString &compositeOperator)
{
    if (compositeOperator == QLatin1String("source-over"))
        return QPainter::CompositionMode_SourceOver;
    else if (compositeOperator == QLatin1String("source-out"))
        return QPainter::CompositionMode_SourceOut;
    else if (compositeOperator == QLatin1String("source-in"))
        return QPainter::CompositionMode_SourceIn;
    else if (compositeOperator == QLatin1String("source-atop"))
        return QPainter::CompositionMode_SourceAtop;
    else if (compositeOperator == QLatin1String("destination-atop"))
        return QPainter::CompositionMode_DestinationAtop;
    else if (compositeOperator == QLatin1String("destination-in"))
        return QPainter::CompositionMode_DestinationIn;
    else if (compositeOperator == QLatin1String("destination-out"))
        return QPainter::CompositionMode_DestinationOut;
    else if (compositeOperator == QLatin1String("destination-over"))
        return QPainter::CompositionMode_DestinationOver;
    else if (compositeOperator == QLatin1String("darker"))
        return QPainter::CompositionMode_SourceOver;
    else if (compositeOperator == QLatin1String("lighter"))
        return QPainter::CompositionMode_SourceOver;
    else if (compositeOperator == QLatin1String("copy"))
        return QPainter::CompositionMode_Source;
    else if (compositeOperator == QLatin1String("xor"))
        return QPainter::CompositionMode_Xor;

    return QPainter::CompositionMode_SourceOver;
}

static QString compositeOperatorToString(QPainter::CompositionMode op)
{
    switch (op) {
    case QPainter::CompositionMode_SourceOver:
        return QLatin1String("source-over");
    case QPainter::CompositionMode_DestinationOver:
        return QLatin1String("destination-over");
    case QPainter::CompositionMode_Clear:
        return QLatin1String("clear");
    case QPainter::CompositionMode_Source:
        return QLatin1String("source");
    case QPainter::CompositionMode_Destination:
        return QLatin1String("destination");
    case QPainter::CompositionMode_SourceIn:
        return QLatin1String("source-in");
    case QPainter::CompositionMode_DestinationIn:
        return QLatin1String("destination-in");
    case QPainter::CompositionMode_SourceOut:
        return QLatin1String("source-out");
    case QPainter::CompositionMode_DestinationOut:
        return QLatin1String("destination-out");
    case QPainter::CompositionMode_SourceAtop:
        return QLatin1String("source-atop");
    case QPainter::CompositionMode_DestinationAtop:
        return QLatin1String("destination-atop");
    case QPainter::CompositionMode_Xor:
        return QLatin1String("xor");
    case QPainter::CompositionMode_Plus:
        return QLatin1String("plus");
    case QPainter::CompositionMode_Multiply:
        return QLatin1String("multiply");
    case QPainter::CompositionMode_Screen:
        return QLatin1String("screen");
    case QPainter::CompositionMode_Overlay:
        return QLatin1String("overlay");
    case QPainter::CompositionMode_Darken:
        return QLatin1String("darken");
    case QPainter::CompositionMode_Lighten:
        return QLatin1String("lighten");
    case QPainter::CompositionMode_ColorDodge:
        return QLatin1String("color-dodge");
    case QPainter::CompositionMode_ColorBurn:
        return QLatin1String("color-burn");
    case QPainter::CompositionMode_HardLight:
        return QLatin1String("hard-light");
    case QPainter::CompositionMode_SoftLight:
        return QLatin1String("soft-light");
    case QPainter::CompositionMode_Difference:
        return QLatin1String("difference");
    case QPainter::CompositionMode_Exclusion:
        return QLatin1String("exclusion");
    default:
        break;
    }
    return QString();
}

void Context2D::save()
{
    m_stateStack.push(m_state);
}


void Context2D::restore()
{
    if (!m_stateStack.isEmpty()) {
        m_state = m_stateStack.pop();
        m_state.flags = AllIsFullOfDirt;
    }
}


void Context2D::scale(qreal x, qreal y)
{
    m_state.matrix.scale(x, y);
    m_state.flags |= DirtyTransformationMatrix;
}


void Context2D::rotate(qreal angle)
{
    m_state.matrix.rotate(DEGREES(angle));
    m_state.flags |= DirtyTransformationMatrix;
}


void Context2D::translate(qreal x, qreal y)
{
    m_state.matrix.translate(x, y);
    m_state.flags |= DirtyTransformationMatrix;
}


void Context2D::transform(qreal m11, qreal m12, qreal m21, qreal m22,
                          qreal dx, qreal dy)
{
    QMatrix mat(m11, m12,
                m21, m22,
                dx, dy);
    m_state.matrix *= mat;
    m_state.flags |= DirtyTransformationMatrix;
}


void Context2D::setTransform(qreal m11, qreal m12, qreal m21, qreal m22,
                             qreal dx, qreal dy)
{
    QMatrix mat(m11, m12,
                m21, m22,
                dx, dy);
    m_state.matrix = mat;
    m_state.flags |= DirtyTransformationMatrix;
}


QString Context2D::globalCompositeOperation() const
{
    return compositeOperatorToString(m_state.globalCompositeOperation);
}

void Context2D::setGlobalCompositeOperation(const QString &op)
{
    QPainter::CompositionMode mode =
            compositeOperatorFromString(op);
    m_state.globalCompositeOperation = mode;
    m_state.flags |= DirtyGlobalCompositeOperation;
}

QVariant Context2D::strokeStyle() const
{
    return m_state.strokeStyle;
}

void Context2D::setStrokeStyle(const QVariant &style)
{
    CanvasGradient * gradient= qobject_cast<CanvasGradient*>(style.value<QObject*>());
    if (gradient) {
        m_state.strokeStyle = gradient->value();
    } else {
        QColor color = colorFromString(style.toString());
        m_state.strokeStyle = color;
    }
    m_state.flags |= DirtyStrokeStyle;
}

QVariant Context2D::fillStyle() const
{
    return m_state.fillStyle;
}

void Context2D::setFillStyle(const QVariant &style)
{
    CanvasGradient * gradient= qobject_cast<CanvasGradient*>(style.value<QObject*>());
    if (gradient) {
        m_state.fillStyle = gradient->value();
    } else {
        QColor color = colorFromString(style.toString());
        m_state.fillStyle = color;
    }
    m_state.flags |= DirtyFillStyle;
}

qreal Context2D::globalAlpha() const
{
    return m_state.globalAlpha;
}

void Context2D::setGlobalAlpha(qreal alpha)
{
    m_state.globalAlpha = alpha;
    m_state.flags |= DirtyGlobalAlpha;
}

CanvasImage *Context2D::createImage(const QString &url)
{
    return new CanvasImage(url);
}

CanvasGradient *Context2D::createLinearGradient(qreal x0, qreal y0,
                                                qreal x1, qreal y1)
{
    QLinearGradient g(x0, y0, x1, y1);
    return new CanvasGradient(g);
}


CanvasGradient *Context2D::createRadialGradient(qreal x0, qreal y0,
                                                qreal r0, qreal x1,
                                                qreal y1, qreal r1)
{
    QRadialGradient g(QPointF(x1, y1), r0+r1, QPointF(x0, y0));
    return new CanvasGradient(g);
}

qreal Context2D::lineWidth() const
{
    return m_state.lineWidth;
}

void Context2D::setLineWidth(qreal w)
{
    m_state.lineWidth = w;
    m_state.flags |= DirtyLineWidth;
}

QString Context2D::lineCap() const
{
    switch (m_state.lineCap) {
    case Qt::FlatCap:
        return QLatin1String("butt");
    case Qt::SquareCap:
        return QLatin1String("square");
    case Qt::RoundCap:
        return QLatin1String("round");
    default: ;
    }
    return QString();
}

void Context2D::setLineCap(const QString &capString)
{
    Qt::PenCapStyle style;
    if (capString == QLatin1String("round"))
        style = Qt::RoundCap;
    else if (capString == QLatin1String("square"))
        style = Qt::SquareCap;
    else //if (capString == QLatin1String("butt"))
        style = Qt::FlatCap;
    m_state.lineCap = style;
    m_state.flags |= DirtyLineCap;
}

QString Context2D::lineJoin() const
{
    switch (m_state.lineJoin) {
    case Qt::RoundJoin:
        return QLatin1String("round");
    case Qt::BevelJoin:
        return QLatin1String("bevel");
    case Qt::MiterJoin:
        return QLatin1String("miter");
    default: ;
    }
    return QString();
}

void Context2D::setLineJoin(const QString &joinString)
{
    Qt::PenJoinStyle style;
    if (joinString == QLatin1String("round"))
        style = Qt::RoundJoin;
    else if (joinString == QLatin1String("bevel"))
        style = Qt::BevelJoin;
    else //if (joinString == "miter")
        style = Qt::MiterJoin;
    m_state.lineJoin = style;
    m_state.flags |= DirtyLineJoin;
}

qreal Context2D::miterLimit() const
{
    return m_state.miterLimit;
}

void Context2D::setMiterLimit(qreal m)
{
    m_state.miterLimit = m;
    m_state.flags |= DirtyMiterLimit;
}

void Context2D::setShadowOffsetX(qreal x)
{
    if (m_state.shadowOffsetX == x)
        return;
    m_state.shadowOffsetX = x;
    updateShadowBuffer();
    if (m_painter.device() == &m_shadowbuffer && m_state.shadowBlur>0)
        endPainting();
    m_state.flags |= DirtyShadowOffsetX;
}

const QList<Context2D::MouseArea> &Context2D::mouseAreas() const
{
    return m_mouseAreas;
}

void Context2D::updateShadowBuffer() {
    if (m_shadowbuffer.isNull() || m_shadowbuffer.width() != m_width+m_state.shadowOffsetX ||
        m_shadowbuffer.height() != m_height+m_state.shadowOffsetY) {
        m_shadowbuffer = QImage(m_width+m_state.shadowOffsetX, m_height+m_state.shadowOffsetY, QImage::Format_ARGB32);
        m_shadowbuffer.fill(Qt::transparent);
    }
}

void Context2D::setShadowOffsetY(qreal y)
{
    if (m_state.shadowOffsetY == y)
        return;
    m_state.shadowOffsetY = y;
    updateShadowBuffer();
    if (m_painter.device() == &m_shadowbuffer && m_state.shadowBlur>0)
        endPainting();

    m_state.flags |= DirtyShadowOffsetY;
}

void Context2D::setShadowBlur(qreal b)
{
    if (m_state.shadowBlur == b)
        return;
    m_state.shadowBlur = b;
    updateShadowBuffer();
    if (m_painter.device() == &m_shadowbuffer && m_state.shadowBlur>0)
        endPainting();
    m_state.flags |= DirtyShadowBlur;
}

void Context2D::setShadowColor(const QString &str)
{
    m_state.shadowColor = colorFromString(str);
    if (m_painter.device() == &m_shadowbuffer && m_state.shadowBlur>0)
        endPainting();
    m_state.flags |= DirtyShadowColor;
}

QString Context2D::textBaseline()
{
    switch (m_state.textBaseline) {
    case Context2D::Alphabetic:
        return QLatin1String("alphabetic");
    case Context2D::Hanging:
        return QLatin1String("hanging");
    case Context2D::Bottom:
        return QLatin1String("bottom");
    case Context2D::Top:
        return QLatin1String("top");
    case Context2D::Middle:
        return QLatin1String("middle");
    default:
        Q_ASSERT("invalid value");
        return QLatin1String("start");
    }
}

void Context2D::setTextBaseline(const QString &baseline)
{
    if (baseline==QLatin1String("alphabetic"))
        m_state.textBaseline = Context2D::Alphabetic;
    else if (baseline == QLatin1String("hanging"))
        m_state.textBaseline = Context2D::Hanging;
    else if (baseline == QLatin1String("top"))
        m_state.textBaseline = Context2D::Top;
    else if (baseline == QLatin1String("bottom"))
        m_state.textBaseline = Context2D::Bottom;
    else if (baseline == QLatin1String("middle"))
        m_state.textBaseline = Context2D::Middle;
    else {
        m_state.textBaseline = Context2D::Alphabetic;
        qWarning() << (QLatin1String("Context2D: invalid baseline:") + baseline);
    }
    m_state.flags |= DirtyTextBaseline;
}

QString Context2D::textAlign()
{
    switch (m_state.textAlign) {
    case Context2D::Left:
        return QLatin1String("left");
    case Context2D::Right:
        return QLatin1String("right");
    case Context2D::Center:
        return QLatin1String("center");
    case Context2D::Start:
        return QLatin1String("start");
    case Context2D::End:
        return QLatin1String("end");
    default:
        Q_ASSERT("invalid value");
        qWarning() << ("Context2D::invalid textAlign");
        return QLatin1String("start");
    }
}

void Context2D::setTextAlign(const QString &baseline)
{
    if (baseline==QLatin1String("start"))
        m_state.textAlign = Context2D::Start;
    else if (baseline == QLatin1String("end"))
        m_state.textAlign = Context2D::End;
    else if (baseline == QLatin1String("left"))
        m_state.textAlign = Context2D::Left;
    else if (baseline == QLatin1String("right"))
        m_state.textAlign = Context2D::Right;
    else if (baseline == QLatin1String("center"))
        m_state.textAlign = Context2D::Center;
    else {
        m_state.textAlign= Context2D::Start;
        qWarning("Context2D: invalid text align");
    }
    // ### alphabetic, ideographic, hanging
    m_state.flags |= DirtyTextBaseline;
}

void Context2D::setFont(const QString &fontString)
{
    QFont font;
    // ### this is simplified and incomplete
    QStringList tokens = fontString.split(QLatin1Char(QLatin1Char(' ')));
    foreach (const QString &token, tokens) {
        if (token == QLatin1String("italic"))
            font.setItalic(true);
        else if (token == QLatin1String("bold"))
            font.setBold(true);
        else if (token.endsWith(QLatin1String("px"))) {
            QString number = token;
            number.remove(QLatin1String("px"));
#ifdef Q_OS_MACX
            // compensating the extra antialias space with bigger fonts
            // this class is only used by the QML Profiler
            // not much harm can be inflicted by this dirty hack
            font.setPointSizeF(number.trimmed().toFloat()*4.0f/3.0f);
#else
            font.setPointSizeF(number.trimmed().toFloat());
#endif
        } else
            font.setFamily(token);
    }
    m_state.font = font;
    m_state.flags |= DirtyFont;
}

QString Context2D::font()
{
    return m_state.font.toString();
}

qreal Context2D::shadowOffsetX() const
{
    return m_state.shadowOffsetX;
}

qreal Context2D::shadowOffsetY() const
{
    return m_state.shadowOffsetY;
}


qreal Context2D::shadowBlur() const
{
    return m_state.shadowBlur;
}


QString Context2D::shadowColor() const
{
    return m_state.shadowColor.name();
}


void Context2D::clearRect(qreal x, qreal y, qreal w, qreal h)
{
    beginPainting();
    m_painter.save();
    m_painter.setMatrix(worldMatrix(), false);
    m_painter.setCompositionMode(QPainter::CompositionMode_Source);
    QColor fillColor = parent()->property("color").value<QColor>();

    m_painter.fillRect(QRectF(x, y, w, h), fillColor);
    m_painter.restore();
    scheduleChange();
}

void Context2D::fillRect(qreal x, qreal y, qreal w, qreal h)
{
    beginPainting();
    m_painter.save();
    m_painter.setMatrix(worldMatrix(), false);
    m_painter.fillRect(QRectF(x, y, w, h), m_painter.brush());
    m_painter.restore();
    scheduleChange();
}

int Context2D::baseLineOffset(Context2D::TextBaseLine value, const QFontMetrics &metrics)
{
    int offset = 0;
    switch (value) {
    case Context2D::Top:
        break;
    case Context2D::Alphabetic:
    case Context2D::Middle:
    case Context2D::Hanging:
        offset = metrics.ascent();
        break;
    case Context2D::Bottom:
        offset = metrics.height();
       break;
    }
    return offset;
}

int Context2D::textAlignOffset(Context2D::TextAlign value, const QFontMetrics &metrics, const QString &text)
{
    int offset = 0;
    if (value == Context2D::Start)
        value = qApp->layoutDirection() == Qt::LeftToRight ? Context2D::Left : Context2D::Right;
    else if (value == Context2D::End)
        value = qApp->layoutDirection() == Qt::LeftToRight ? Context2D::Right: Context2D::Left;
    switch (value) {
    case Context2D::Center:
        offset = metrics.width(text)/2;
        break;
    case Context2D::Right:
        offset = metrics.width(text);
    case Context2D::Left:
    default:
        break;
    }
    return offset;
}

void Context2D::fillText(const QString &text, qreal x, qreal y)
{
    beginPainting();
    m_painter.save();
    m_painter.setPen(QPen(m_state.fillStyle, m_state.lineWidth));
    m_painter.setMatrix(worldMatrix(), false);
    QFont font;
    font.setBold(true);
    m_painter.setFont(m_state.font);
    int yoffset = baseLineOffset(m_state.textBaseline, m_painter.fontMetrics());
    int xoffset = textAlignOffset(m_state.textAlign, m_painter.fontMetrics(), text);
    QTextOption opt; // Adjust baseLine etc
    m_painter.drawText(QRectF(x-xoffset, y-yoffset, QWIDGETSIZE_MAX, m_painter.fontMetrics().height()), text, opt);
    m_painter.restore();
    endPainting();
    scheduleChange();
}

void Context2D::strokeText(const QString &text, qreal x, qreal y)
{
    beginPainting();
    m_painter.save();
    m_painter.setPen(QPen(m_state.fillStyle,0));
    m_painter.setMatrix(worldMatrix(), false);

    QPainterPath textPath;
    QFont font = m_state.font;
    font.setStyleStrategy(QFont::ForceOutline);
    m_painter.setFont(font);
    const QFontMetrics &metrics = m_painter.fontMetrics();
    int yoffset = baseLineOffset(m_state.textBaseline, metrics);
    int xoffset = textAlignOffset(m_state.textAlign, metrics, text);
    textPath.addText(x-xoffset, y-yoffset+metrics.ascent(), font, text);
    m_painter.strokePath(textPath, QPen(m_state.fillStyle, m_state.lineWidth));
    m_painter.restore();
    endPainting();
    scheduleChange();
}

void Context2D::strokeRect(qreal x, qreal y, qreal w, qreal h)
{
    QPainterPath path;
    path.addRect(x, y, w, h);
    beginPainting();
    m_painter.save();
    m_painter.setMatrix(worldMatrix(), false);
    m_painter.strokePath(path, m_painter.pen());
    m_painter.restore();
    scheduleChange();
}

void Context2D::mouseArea(qreal x, qreal y, qreal w, qreal h, const QScriptValue &callback,
                          const QScriptValue &data)
{
    MouseArea a = { callback, data, QRectF(x, y, w, h), m_state.matrix };
    m_mouseAreas << a;
}

void Context2D::beginPath()
{
    m_path = QPainterPath();
}


void Context2D::closePath()
{
    m_path.closeSubpath();
}


void Context2D::moveTo(qreal x, qreal y)
{
    QPointF pt = worldMatrix().map(QPointF(x, y));
    m_path.moveTo(pt);
}


void Context2D::lineTo(qreal x, qreal y)
{
    QPointF pt = worldMatrix().map(QPointF(x, y));
    m_path.lineTo(pt);
}


void Context2D::quadraticCurveTo(qreal cpx, qreal cpy, qreal x, qreal y)
{
    QPointF cp = worldMatrix().map(QPointF(cpx, cpy));
    QPointF xy = worldMatrix().map(QPointF(x, y));
    m_path.quadTo(cp, xy);
}


void Context2D::bezierCurveTo(qreal cp1x, qreal cp1y,
                              qreal cp2x, qreal cp2y, qreal x, qreal y)
{
    QPointF cp1 = worldMatrix().map(QPointF(cp1x, cp1y));
    QPointF cp2 = worldMatrix().map(QPointF(cp2x, cp2y));
    QPointF end = worldMatrix().map(QPointF(x, y));
    m_path.cubicTo(cp1, cp2, end);
}


void Context2D::arcTo(qreal x1, qreal y1, qreal x2, qreal y2, qreal radius)
{
    //FIXME: this is surely busted
    QPointF st  = worldMatrix().map(QPointF(x1, y1));
    QPointF end = worldMatrix().map(QPointF(x2, y2));
    m_path.arcTo(st.x(), st.y(),
                 end.x()-st.x(), end.y()-st.y(),
                 radius, 90);
}


void Context2D::rect(qreal x, qreal y, qreal w, qreal h)
{
    QPainterPath path; path.addRect(x, y, w, h);
    path = worldMatrix().map(path);
    m_path.addPath(path);
}

void Context2D::arc(qreal xc, qreal yc, qreal radius,
                    qreal sar, qreal ear,
                    bool anticlockwise)
{
    //### HACK
    // In Qt we don't switch the coordinate system for degrees
    // and still use the 0,0 as bottom left for degrees so we need
    // to switch
    sar = -sar;
    ear = -ear;
    anticlockwise = !anticlockwise;
    //end hack

    float sa = DEGREES(sar);
    float ea = DEGREES(ear);

    double span = 0;

    double xs     = xc - radius;
    double ys     = yc - radius;
    double width  = radius*2;
    double height = radius*2;

    if (!anticlockwise && (ea < sa))
        span += 360;
    else if (anticlockwise && (sa < ea))
        span -= 360;

    //### this is also due to switched coordinate system
    // we would end up with a 0 span instead of 360
    if (!(qFuzzyCompare(span + (ea - sa) + 1, 1) &&
          qFuzzyCompare(qAbs(span), 360))) {
        span   += ea - sa;
    }

    QPainterPath path;
    path.moveTo(QPointF(xc + radius  * cos(sar),
                        yc - radius  * sin(sar)));

    path.arcTo(xs, ys, width, height, sa, span);
    path = worldMatrix().map(path);
    m_path.addPath(path);
}


void Context2D::fill()
{
    beginPainting();
    m_painter.fillPath(m_path, m_painter.brush());
    scheduleChange();
}


void Context2D::stroke()
{
    beginPainting();
    m_painter.save();
    m_painter.setMatrix(worldMatrix(), false);
    QPainterPath tmp = worldMatrix().inverted().map(m_path);
    m_painter.strokePath(tmp, m_painter.pen());
    m_painter.restore();
    scheduleChange();
}


void Context2D::clip()
{
    m_state.clipPath = m_path;
    m_state.flags |= DirtyClippingRegion;
}


bool Context2D::isPointInPath(qreal x, qreal y) const
{
    return m_path.contains(QPointF(x, y));
}


ImageData Context2D::getImageData(qreal sx, qreal sy, qreal sw, qreal sh)
{
    Q_UNUSED(sx);
    Q_UNUSED(sy);
    Q_UNUSED(sw);
    Q_UNUSED(sh);
    return ImageData();
}


void Context2D::putImageData(ImageData image, qreal dx, qreal dy)
{
    Q_UNUSED(image);
    Q_UNUSED(dx);
    Q_UNUSED(dy);
}

Context2D::Context2D(QObject *parent)
    : QObject(parent), m_changeTimerId(-1), m_width(0), m_height(0), m_inPaint(false)
{
    reset();
}

void Context2D::setupPainter()
{
    m_painter.setRenderHint(QPainter::Antialiasing, true);
    if ((m_state.flags & DirtyClippingRegion) && !m_state.clipPath.isEmpty())
        m_painter.setClipPath(m_state.clipPath);
    if (m_state.flags & DirtyFillStyle)
        m_painter.setBrush(m_state.fillStyle);
    if (m_state.flags & DirtyGlobalAlpha)
        m_painter.setOpacity(m_state.globalAlpha);
    if (m_state.flags & DirtyGlobalCompositeOperation)
        m_painter.setCompositionMode(m_state.globalCompositeOperation);
    if (m_state.flags & MDirtyPen) {
        QPen pen = m_painter.pen();
        if (m_state.flags & DirtyStrokeStyle)
            pen.setBrush(m_state.strokeStyle);
        if (m_state.flags & DirtyLineWidth)
            pen.setWidthF(m_state.lineWidth);
        if (m_state.flags & DirtyLineCap)
            pen.setCapStyle(m_state.lineCap);
        if (m_state.flags & DirtyLineJoin)
            pen.setJoinStyle(m_state.lineJoin);
        if (m_state.flags & DirtyMiterLimit)
            pen.setMiterLimit(m_state.miterLimit);
        m_painter.setPen(pen);
    }
}

void Context2D::beginPainting()
{
    if (m_width <= 0 || m_height <=0)
        return;

    if (m_pixmap.width() != m_width || m_pixmap.height() != m_height) {
        if (m_painter.isActive())
            m_painter.end();
        m_pixmap = QPixmap(m_width, m_height);
        m_pixmap.fill(parent()->property("color").value<QColor>());
    }

    if (m_state.shadowBlur > 0 && m_painter.device() != &m_shadowbuffer) {
        if (m_painter.isActive())
            m_painter.end();
        updateShadowBuffer();
        m_painter.begin(&m_shadowbuffer);
        m_painter.setViewport(m_state.shadowOffsetX,
                              m_state.shadowOffsetY,
                              m_shadowbuffer.width(),
                              m_shadowbuffer.height());
        m_shadowbuffer.fill(Qt::transparent);
    }

    if (!m_painter.isActive()) {
        m_painter.begin(&m_pixmap);
        m_painter.setRenderHint(QPainter::Antialiasing);
        if (!m_state.clipPath.isEmpty())
            m_painter.setClipPath(m_state.clipPath);
        m_painter.setBrush(m_state.fillStyle);
        m_painter.setOpacity(m_state.globalAlpha);
        QPen pen;
        pen.setBrush(m_state.strokeStyle);
        if (pen.style() == Qt::NoPen)
            pen.setStyle(Qt::SolidLine);
        pen.setCapStyle(m_state.lineCap);
        pen.setJoinStyle(m_state.lineJoin);
        pen.setWidthF(m_state.lineWidth);
        pen.setMiterLimit(m_state.miterLimit);
        m_painter.setPen(pen);
    } else {
        setupPainter();
        m_state.flags = 0;
    }
}

void Context2D::endPainting()
{
    if (m_state.shadowBlur > 0) {
        QImage alphaChannel = m_shadowbuffer.alphaChannel();

        qt_blurImage(alphaChannel, m_state.shadowBlur, false, 1);

        QRect imageRect = m_shadowbuffer.rect();

        if (m_shadowColorIndexBuffer.isEmpty() || m_shadowColorBuffer != m_state.shadowColor) {
            m_shadowColorIndexBuffer.clear();
            m_shadowColorBuffer = m_state.shadowColor;

            for (int i = 0; i < 256; ++i) {
                m_shadowColorIndexBuffer << qRgba(qRound(255 * m_state.shadowColor.redF()),
                                                  qRound(255 * m_state.shadowColor.greenF()),
                                                  qRound(255 * m_state.shadowColor.blueF()),
                                                  i);
            }
        }
        alphaChannel.setColorTable(m_shadowColorIndexBuffer);

        if (m_painter.isActive())
            m_painter.end();

        m_painter.begin(&m_pixmap);

        // draw the blurred drop shadow...
        m_painter.save();
        QTransform tf = m_painter.transform();
        m_painter.translate(0, imageRect.height());
        m_painter.rotate(-90);
        m_painter.drawImage(0, 0, alphaChannel);
        m_painter.setTransform(tf);
        m_painter.restore();

        // draw source
        m_painter.drawImage(-m_state.shadowOffsetX, -m_state.shadowOffsetY, m_shadowbuffer.copy());
        m_painter.end();
    }
}

void Context2D::clear()
{
    m_painter.fillRect(QRect(QPoint(0,0), size()), Qt::white);
}

void Context2D::reset()
{
    m_stateStack.clear();
    m_state.matrix = QMatrix();
    m_state.clipPath = QPainterPath();
    m_state.strokeStyle = Qt::black;
    m_state.fillStyle = Qt::black;
    m_state.globalAlpha = 1.0;
    m_state.lineWidth = 1;
    m_state.lineCap = Qt::FlatCap;
    m_state.lineJoin = Qt::MiterJoin;
    m_state.miterLimit = 10;
    m_state.shadowOffsetX = 0;
    m_state.shadowOffsetY = 0;
    m_state.shadowBlur = 0;
    m_state.shadowColor = qRgba(0, 0, 0, 0);
    m_state.globalCompositeOperation = QPainter::CompositionMode_SourceOver;
    m_state.font = QFont();
    m_state.textAlign = Start;
    m_state.textBaseline = Alphabetic;
    m_state.flags = AllIsFullOfDirt;
    m_mouseAreas.clear();
    clear();
}

void Context2D::drawImage(const QVariant &var, qreal sx, qreal sy,
                          qreal sw = 0, qreal sh = 0)
{
    CanvasImage *image = qobject_cast<CanvasImage*>(var.value<QObject*>());
    if (!image) {
        Canvas *canvas = qobject_cast<Canvas*>(var.value<QObject*>());
        if (canvas)
            image = canvas->toImage();
    }
    if (image) {
        beginPainting();
        if (sw == sh && sh == 0)
            m_painter.drawPixmap(QPointF(sx, sy), image->value());
        else
            m_painter.drawPixmap(QRect(sx, sy, sw, sh), image->value());

        scheduleChange();
    }
}

void Context2D::setSize(int width, int height)
{
    endPainting();
    m_width = width;
    m_height = height;

    scheduleChange();
}

void Context2D::setSize(const QSize &size)
{
    setSize(size.width(), size.height());
}

QSize Context2D::size() const
{
    return m_pixmap.size();
}

QPoint Context2D::painterTranslate() const
{
    return m_painterTranslate;
}

void Context2D::setPainterTranslate(const QPoint &translate)
{
    m_painterTranslate = translate;
    m_state.flags |= DirtyTransformationMatrix;
}

void Context2D::scheduleChange()
{
    if (m_changeTimerId == -1 && !m_inPaint)
        m_changeTimerId = startTimer(0);
}

void Context2D::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == m_changeTimerId) {
        killTimer(m_changeTimerId);
        m_changeTimerId = -1;
        endPainting();
        emit changed();
    } else {
        QObject::timerEvent(e);
    }
}

QMatrix Context2D::worldMatrix() const
{
    QMatrix mat;
    mat.translate(m_painterTranslate.x(), m_painterTranslate.y());
    mat *= m_state.matrix;
    return mat;
}

QT_END_NAMESPACE
