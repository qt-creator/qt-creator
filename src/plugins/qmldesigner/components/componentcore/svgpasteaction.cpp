// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "svgpasteaction.h"

#include <itemlibraryinfo.h>
#include <variantproperty.h>
#include <nodeabstractproperty.h>

#include <private/qlocale_tools_p.h>

#include <QSvgGenerator>
#include <QBuffer>
#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QtMath>

#include <array>

namespace QmlDesigner {

namespace {

/* Copied from qquicksvgparser.cpp 3e783b26a8fb41e3f5a53b883735f5d10fbbd98a */

// '0' is 0x30 and '9' is 0x39
inline static bool isDigit(ushort ch)
{
    static quint16 magic = 0x3ff;
    return ((ch >> 4) == 3) && (magic >> (ch & 15));
}

static qreal toDouble(const QChar *&str)
{
    const int maxLen = 255;//technically doubles can go til 308+ but whatever
    char temp[maxLen+1];
    int pos = 0;

    if (*str == QLatin1Char('-')) {
        temp[pos++] = '-';
        ++str;
    } else if (*str == QLatin1Char('+')) {
        ++str;
    }
    while (isDigit(str->unicode()) && pos < maxLen) {
        temp[pos++] = str->toLatin1();
        ++str;
    }
    if (*str == QLatin1Char('.') && pos < maxLen) {
        temp[pos++] = '.';
        ++str;
    }
    while (isDigit(str->unicode()) && pos < maxLen) {
        temp[pos++] = str->toLatin1();
        ++str;
    }
    bool exponent = false;
    if ((*str == QLatin1Char('e') || *str == QLatin1Char('E')) && pos < maxLen) {
        exponent = true;
        temp[pos++] = 'e';
        ++str;
        if ((*str == QLatin1Char('-') || *str == QLatin1Char('+')) && pos < maxLen) {
            temp[pos++] = str->toLatin1();
            ++str;
        }
        while (isDigit(str->unicode()) && pos < maxLen) {
            temp[pos++] = str->toLatin1();
            ++str;
        }
    }

    temp[pos] = '\0';

    qreal val;
    if (!exponent && pos < 10) {
        int ival = 0;
        const char *t = temp;
        bool neg = false;
        if (*t == '-') {
            neg = true;
            ++t;
        }
        while (*t && *t != '.') {
            ival *= 10;
            ival += (*t) - '0';
            ++t;
        }
        if (*t == '.') {
            ++t;
            int div = 1;
            while (*t) {
                ival *= 10;
                ival += (*t) - '0';
                div *= 10;
                ++t;
            }
            val = ((qreal)ival)/((qreal)div);
        } else {
            val = ival;
        }
        if (neg)
            val = -val;
    } else {
        bool ok = false;
        val = qstrtod(temp, nullptr, &ok);
    }
    return val;
}

inline static void parseNumbersArray(const QChar *&str, QVarLengthArray<qreal, 8> &points)
{
    while (str->isSpace())
        ++str;
    while (isDigit(str->unicode()) ||
           *str == QLatin1Char('-') || *str == QLatin1Char('+') ||
           *str == QLatin1Char('.')) {

        points.append(toDouble(str));

        while (str->isSpace())
            ++str;
        if (*str == QLatin1Char(','))
            ++str;

        //eat the rest of space
        while (str->isSpace())
            ++str;
    }
}

static void pathArcSegment(QPainterPath &path,
                           qreal xc, qreal yc,
                           qreal th0, qreal th1,
                           qreal rx, qreal ry, qreal xAxisRotation)
{
    qreal sinTh, cosTh;
    qreal a00, a01, a10, a11;
    qreal x1, y1, x2, y2, x3, y3;
    qreal t;
    qreal thHalf;

    sinTh = qSin(qDegreesToRadians(xAxisRotation));
    cosTh = qCos(qDegreesToRadians(xAxisRotation));

    a00 =  cosTh * rx;
    a01 = -sinTh * ry;
    a10 =  sinTh * rx;
    a11 =  cosTh * ry;

    thHalf = 0.5 * (th1 - th0);
    t = (8.0 / 3.0) * qSin(thHalf * 0.5) * qSin(thHalf * 0.5) / qSin(thHalf);
    x1 = xc + qCos(th0) - t * qSin(th0);
    y1 = yc + qSin(th0) + t * qCos(th0);
    x3 = xc + qCos(th1);
    y3 = yc + qSin(th1);
    x2 = x3 + t * qSin(th1);
    y2 = y3 - t * qCos(th1);

    path.cubicTo(a00 * x1 + a01 * y1, a10 * x1 + a11 * y1,
                 a00 * x2 + a01 * y2, a10 * x2 + a11 * y2,
                 a00 * x3 + a01 * y3, a10 * x3 + a11 * y3);
}

void pathArc(QPainterPath &path,
             qreal rx, qreal ry,
             qreal x_axis_rotation,
             int large_arc_flag,
             int sweep_flag,
             qreal x, qreal y,
             qreal curx, qreal cury)
{
    qreal sin_th, cos_th;
    qreal a00, a01, a10, a11;
    qreal x0, y0, x1, y1, xc, yc;
    qreal d, sfactor, sfactor_sq;
    qreal th0, th1, th_arc;
    int i, n_segs;
    qreal dx, dy, dx1, dy1, Pr1, Pr2, Px, Py, check;

    rx = qAbs(rx);
    ry = qAbs(ry);

    sin_th = qSin(qDegreesToRadians(x_axis_rotation));
    cos_th = qCos(qDegreesToRadians(x_axis_rotation));

    dx = (curx - x) / 2.0;
    dy = (cury - y) / 2.0;
    dx1 =  cos_th * dx + sin_th * dy;
    dy1 = -sin_th * dx + cos_th * dy;
    Pr1 = rx * rx;
    Pr2 = ry * ry;
    Px = dx1 * dx1;
    Py = dy1 * dy1;
    /* Spec : check if radii are large enough */
    check = Px / Pr1 + Py / Pr2;
    if (check > 1) {
        rx = rx * qSqrt(check);
        ry = ry * qSqrt(check);
    }

    a00 =  cos_th / rx;
    a01 =  sin_th / rx;
    a10 = -sin_th / ry;
    a11 =  cos_th / ry;
    x0 = a00 * curx + a01 * cury;
    y0 = a10 * curx + a11 * cury;
    x1 = a00 * x + a01 * y;
    y1 = a10 * x + a11 * y;
    /* (x0, y0) is current point in transformed coordinate space.
       (x1, y1) is new point in transformed coordinate space.
       The arc fits a unit-radius circle in this space.
    */
    d = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
    sfactor_sq = 1.0 / d - 0.25;
    if (sfactor_sq < 0) sfactor_sq = 0;
    sfactor = qSqrt(sfactor_sq);
    if (sweep_flag == large_arc_flag) sfactor = -sfactor;
    xc = 0.5 * (x0 + x1) - sfactor * (y1 - y0);
    yc = 0.5 * (y0 + y1) + sfactor * (x1 - x0);
    /* (xc, yc) is center of the circle. */

    th0 = qAtan2(y0 - yc, x0 - xc);
    th1 = qAtan2(y1 - yc, x1 - xc);

    th_arc = th1 - th0;
    if (th_arc < 0 && sweep_flag)
        th_arc += 2 * M_PI;
    else if (th_arc > 0 && !sweep_flag)
        th_arc -= 2 * M_PI;

    n_segs = qCeil(qAbs(th_arc / (M_PI * 0.5 + 0.001)));

    for (i = 0; i < n_segs; i++) {
        pathArcSegment(path, xc, yc,
                       th0 + i * th_arc / n_segs,
                       th0 + (i + 1) * th_arc / n_segs,
                       rx, ry, x_axis_rotation);
    }
}


bool parsePathDataFast(const QString &dataStr, QPainterPath &path)
{
    qreal x0 = 0, y0 = 0;              // starting point
    qreal x = 0, y = 0;                // current point
    char lastMode = 0;
    QPointF ctrlPt;
    const QChar *str = dataStr.constData();
    const QChar *end = str + dataStr.size();

    while (str != end) {
        while (str->isSpace())
            ++str;
        QChar pathElem = *str;
        ++str;
        QVarLengthArray<qreal, 8> arg;
        parseNumbersArray(str, arg);
        if (pathElem == QLatin1Char('z') || pathElem == QLatin1Char('Z'))
            arg.append(0);//dummy
        const qreal *num = arg.constData();
        int count = arg.size();
        while (count > 0) {
            qreal offsetX = x;        // correction offsets
            qreal offsetY = y;        // for relative commands
            switch (pathElem.unicode()) {
            case 'm': {
                if (count < 2) {
                    num++;
                    count--;
                    break;
                }
                x = x0 = num[0] + offsetX;
                y = y0 = num[1] + offsetY;
                num += 2;
                count -= 2;
                path.moveTo(x0, y0);

                 // As per 1.2  spec 8.3.2 The "moveto" commands
                 // If a 'moveto' is followed by multiple pairs of coordinates without explicit commands,
                 // the subsequent pairs shall be treated as implicit 'lineto' commands.
                 pathElem = QLatin1Char('l');
            }
                break;
            case 'M': {
                if (count < 2) {
                    num++;
                    count--;
                    break;
                }
                x = x0 = num[0];
                y = y0 = num[1];
                num += 2;
                count -= 2;
                path.moveTo(x0, y0);

                // As per 1.2  spec 8.3.2 The "moveto" commands
                // If a 'moveto' is followed by multiple pairs of coordinates without explicit commands,
                // the subsequent pairs shall be treated as implicit 'lineto' commands.
                pathElem = QLatin1Char('L');
            }
                break;
            case 'z':
            case 'Z': {
                x = x0;
                y = y0;
                count--; // skip dummy
                num++;
                path.closeSubpath();
            }
                break;
            case 'l': {
                if (count < 2) {
                    num++;
                    count--;
                    break;
                }
                x = num[0] + offsetX;
                y = num[1] + offsetY;
                num += 2;
                count -= 2;
                path.lineTo(x, y);

            }
                break;
            case 'L': {
                if (count < 2) {
                    num++;
                    count--;
                    break;
                }
                x = num[0];
                y = num[1];
                num += 2;
                count -= 2;
                path.lineTo(x, y);
            }
                break;
            case 'h': {
                x = num[0] + offsetX;
                num++;
                count--;
                path.lineTo(x, y);
            }
                break;
            case 'H': {
                x = num[0];
                num++;
                count--;
                path.lineTo(x, y);
            }
                break;
            case 'v': {
                y = num[0] + offsetY;
                num++;
                count--;
                path.lineTo(x, y);
            }
                break;
            case 'V': {
                y = num[0];
                num++;
                count--;
                path.lineTo(x, y);
            }
                break;
            case 'c': {
                if (count < 6) {
                    num += count;
                    count = 0;
                    break;
                }
                QPointF c1(num[0] + offsetX, num[1] + offsetY);
                QPointF c2(num[2] + offsetX, num[3] + offsetY);
                QPointF e(num[4] + offsetX, num[5] + offsetY);
                num += 6;
                count -= 6;
                path.cubicTo(c1, c2, e);
                ctrlPt = c2;
                x = e.x();
                y = e.y();
                break;
            }
            case 'C': {
                if (count < 6) {
                    num += count;
                    count = 0;
                    break;
                }
                QPointF c1(num[0], num[1]);
                QPointF c2(num[2], num[3]);
                QPointF e(num[4], num[5]);
                num += 6;
                count -= 6;
                path.cubicTo(c1, c2, e);
                ctrlPt = c2;
                x = e.x();
                y = e.y();
                break;
            }
            case 's': {
                if (count < 4) {
                    num += count;
                    count = 0;
                    break;
                }
                QPointF c1;
                if (lastMode == 'c' || lastMode == 'C' ||
                    lastMode == 's' || lastMode == 'S')
                    c1 = QPointF(2*x-ctrlPt.x(), 2*y-ctrlPt.y());
                else
                    c1 = QPointF(x, y);
                QPointF c2(num[0] + offsetX, num[1] + offsetY);
                QPointF e(num[2] + offsetX, num[3] + offsetY);
                num += 4;
                count -= 4;
                path.cubicTo(c1, c2, e);
                ctrlPt = c2;
                x = e.x();
                y = e.y();
                break;
            }
            case 'S': {
                if (count < 4) {
                    num += count;
                    count = 0;
                    break;
                }
                QPointF c1;
                if (lastMode == 'c' || lastMode == 'C' ||
                    lastMode == 's' || lastMode == 'S')
                    c1 = QPointF(2*x-ctrlPt.x(), 2*y-ctrlPt.y());
                else
                    c1 = QPointF(x, y);
                QPointF c2(num[0], num[1]);
                QPointF e(num[2], num[3]);
                num += 4;
                count -= 4;
                path.cubicTo(c1, c2, e);
                ctrlPt = c2;
                x = e.x();
                y = e.y();
                break;
            }
            case 'q': {
                if (count < 4) {
                    num += count;
                    count = 0;
                    break;
                }
                QPointF c(num[0] + offsetX, num[1] + offsetY);
                QPointF e(num[2] + offsetX, num[3] + offsetY);
                num += 4;
                count -= 4;
                path.quadTo(c, e);
                ctrlPt = c;
                x = e.x();
                y = e.y();
                break;
            }
            case 'Q': {
                if (count < 4) {
                    num += count;
                    count = 0;
                    break;
                }
                QPointF c(num[0], num[1]);
                QPointF e(num[2], num[3]);
                num += 4;
                count -= 4;
                path.quadTo(c, e);
                ctrlPt = c;
                x = e.x();
                y = e.y();
                break;
            }
            case 't': {
                if (count < 2) {
                    num += count;
                    count = 0;
                    break;
                }
                QPointF e(num[0] + offsetX, num[1] + offsetY);
                num += 2;
                count -= 2;
                QPointF c;
                if (lastMode == 'q' || lastMode == 'Q' ||
                    lastMode == 't' || lastMode == 'T')
                    c = QPointF(2*x-ctrlPt.x(), 2*y-ctrlPt.y());
                else
                    c = QPointF(x, y);
                path.quadTo(c, e);
                ctrlPt = c;
                x = e.x();
                y = e.y();
                break;
            }
            case 'T': {
                if (count < 2) {
                    num += count;
                    count = 0;
                    break;
                }
                QPointF e(num[0], num[1]);
                num += 2;
                count -= 2;
                QPointF c;
                if (lastMode == 'q' || lastMode == 'Q' ||
                    lastMode == 't' || lastMode == 'T')
                    c = QPointF(2*x-ctrlPt.x(), 2*y-ctrlPt.y());
                else
                    c = QPointF(x, y);
                path.quadTo(c, e);
                ctrlPt = c;
                x = e.x();
                y = e.y();
                break;
            }
            case 'a': {
                if (count < 7) {
                    num += count;
                    count = 0;
                    break;
                }
                qreal rx = (*num++);
                qreal ry = (*num++);
                qreal xAxisRotation = (*num++);
                qreal largeArcFlag  = (*num++);
                qreal sweepFlag = (*num++);
                qreal ex = (*num++) + offsetX;
                qreal ey = (*num++) + offsetY;
                count -= 7;
                qreal curx = x;
                qreal cury = y;
                pathArc(path, rx, ry, xAxisRotation, int(largeArcFlag),
                        int(sweepFlag), ex, ey, curx, cury);

                x = ex;
                y = ey;
            }
                break;
            case 'A': {
                if (count < 7) {
                    num += count;
                    count = 0;
                    break;
                }
                qreal rx = (*num++);
                qreal ry = (*num++);
                qreal xAxisRotation = (*num++);
                qreal largeArcFlag  = (*num++);
                qreal sweepFlag = (*num++);
                qreal ex = (*num++);
                qreal ey = (*num++);
                count -= 7;
                qreal curx = x;
                qreal cury = y;
                pathArc(path, rx, ry, xAxisRotation, int(largeArcFlag),
                        int(sweepFlag), ex, ey, curx, cury);

                x = ex;
                y = ey;
            }
                break;
            default:
                return false;
            }
            lastMode = pathElem.toLatin1();
        }
    }
    return true;
}

/* Copied from qquicksvgparser.cpp 3e783b26a8fb41e3f5a53b883735f5d10fbbd98a */

double round(double value, int decimal_places) {
    const double multiplier = std::pow(10.0, decimal_places);
    return std::round(value * multiplier) / multiplier;
}

const std::initializer_list<QStringView> tagAllowList{
    u"path", u"rect", u"line", u"polygon", u"polyline", u"circle", u"ellipse"};

// fillOpacity and strokeOpacity aren't actual QML properties, but get mapped anyways
// for completeness.
const std::initializer_list<std::pair<QStringView, QString>> mapping{{u"fill", "fillColor"},
                                                                     {u"stroke", "strokeColor"},
                                                                     {u"stroke-width", "strokeWidth"},
                                                                     {u"opacity", "opacity"},
                                                                     {u"fill-opacity", "fillOpacity"},
                                                                     {u"stroke-opacity",
                                                                      "strokeOpacity"}};

template <typename Container>
bool contains(const Container &c, const QStringView &stringView) {
    return std::find(std::begin(c), std::end(c), stringView) != std::end(c);
}

template <typename Container>
auto findKey(const Container &c, const QStringView &key) {
    return std::find_if(std::begin(c), std::end(c), [&](const auto &pair){
        return pair.first == key;
    });
}

template<typename Callable>
void depthFirstTraversal(const QDomNode &node,
                         const Callable &action)
{
    QDomNode currentNode = node;

    while (!currentNode.isNull()) {
        action(currentNode);
        depthFirstTraversal(currentNode.firstChild(), action);
        currentNode = currentNode.nextSibling();
    }
}

template<typename Callable>
void topToBottomTraversal(const QDomNode &node,
                          const Callable &action)
{
    if (node.isNull())
        return;

    topToBottomTraversal(node.parentNode(), action);
    action(node);
}

QTransform parseMatrix(const QString &values)
{
    // matrix(<a> <b> <c> <d> <e> <f>)
    //     [a c e]        [m11 m21 m31]
    // SVG [b d f]     Qt [m12 m22 m32]
    //     [0 0 1]        [m13 m23 m33]
    static const QRegularExpression re("([0-9-.]+)");
    QRegularExpressionMatchIterator iter = re.globalMatch(values.simplified());

    std::array<float, 6> arr = {1, 0, 0, 1, 0, 0};
    int i = 0;
    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        arr[i] = match.captured(1).toFloat();
        ++i;
    }

    if (i != 6)
        return QTransform();

    return QTransform(arr[0], arr[1], 0, arr[2], arr[3], 0, arr[4], arr[5], 1);
}

QTransform parseTranslate(const QString &values)
{
    // translate(<x> [<y>]) translate(<x>[,<y>])
    static const QRegularExpression re(R"(^([\d.-]+)(?:(?:\s*,\s*|\s+)([\d.-]+))?$)");
    QRegularExpressionMatch m = re.match(values.simplified());

    if (!m.hasMatch())
        return QTransform();

    return QTransform().translate(m.captured(1).toFloat(), m.captured(2).toFloat());
}

QTransform parseScale(const QString &values)
{
    // scale(<x> [<y>]) scale(<x>[,<y>])
    static const QRegularExpression re(R"(^([\d.-]+)(?:(?:\s*,\s*|\s+)([\d.-]+))?$)");
    QRegularExpressionMatch m = re.match(values.simplified());

    if (!m.hasMatch())
        return QTransform();

    float x = m.captured(1).toFloat();
    // If y is not provided, it is assumed to be equal to x.
    float y = (m.captured(2).isEmpty()) ? x : m.captured(2).toFloat();

    return QTransform().scale(x, y);
}

QTransform parseRotate(const QString &values)
{
    // rotate(<a> [<x> <y>]) rotate(<a>[,<x>,<y>])
    static const QRegularExpression re(R"(^([\d.-]+)(?:(?:\s*,\s*|\s+)([\d.-]+)(?:\s*,\s*|\s+)([\d.-]+))?$)");
    QRegularExpressionMatch m = re.match(values.simplified());

    if (!m.hasMatch())
        return QTransform();

    float a = m.captured(1).toFloat();

    QTransform transform;

    if (m.captured(2).isEmpty() || m.captured(3).isEmpty()) {
        transform.rotate(a);
    } else {
        float x = m.captured(2).toFloat();
        float y = m.captured(3).toFloat();
        transform.translate(x, y);
        transform.rotate(a);
        transform.translate(-x, -y);
    }

    return transform;
}

QTransform parseSkewX(const QString &values)
{
    // skewX(<a>)
    static const QRegularExpression re(R"(^([\d.-]+)$)");
    QRegularExpressionMatch m = re.match(values.simplified());

    if (!m.hasMatch())
        return QTransform();

    float a = m.captured(1).toFloat();

    return QTransform(1, 0, 0, std::tan(a * M_PI / 180.0), 1, 0, 0, 0, 1);
}

QTransform parseSkewY(const QString &values)
{
    // skewY(<a>)
    static const QRegularExpression re(R"(^([\d.-]+)$)");
    QRegularExpressionMatch m = re.match(values.simplified());

    if (!m.hasMatch())
        return QTransform();

    float a = m.captured(1).toFloat();

    return QTransform(1, std::tan(a * M_PI / 180.0), 0, 0, 1, 0, 0, 0, 1);
}

QTransform parseTransform(const QString &transformStr)
{
    if (transformStr.isEmpty())
        return QTransform();

    std::vector<QTransform> transforms;

    static const QRegularExpression reTransform(R"(([\w]+)\(([\s\S]*?)\))");
    QRegularExpressionMatchIterator i = reTransform.globalMatch(transformStr.simplified());

    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();

        const QString function = match.captured(1).simplified();
        const QString values = match.captured(2).simplified();

        if (function == "matrix")
            transforms.push_back(parseMatrix(values));
        else if (function == "translate")
            transforms.push_back(parseTranslate(values));
        else if (function == "scale")
            transforms.push_back(parseScale(values));
        else if (function == "rotate")
            transforms.push_back(parseRotate(values));
        else if (function == "skewX")
            transforms.push_back(parseSkewX(values));
        else if (function == "skewY")
            transforms.push_back(parseSkewY(values));
    }

    QTransform transform;
    std::for_each(transforms.rbegin(), transforms.rend(), [&](const QTransform &t)
    {
        transform *= t;
    });

    return transform;
}

QString convertQPainterPathtoSVGPath(const QPainterPath &path)
{
    QByteArray byteArray;
    QBuffer buffer(&byteArray);

    QSvgGenerator generator;
    generator.setOutputDevice(&buffer);

    QPainter p;
    p.begin(&generator);
    p.drawPath(path);
    p.end();

    QDomDocument tmpDomDocument;
    if (!tmpDomDocument.setContent(byteArray))
        return QString();

    QDomElement pathElement;

    auto extractPathElement = [&pathElement](const QDomNode &node) {
        QDomElement element = node.toElement();
        if (!element.isNull()) {
            if (element.tagName() == "path")
                pathElement = element;
        }
    };

    depthFirstTraversal(tmpDomDocument.firstChild(), extractPathElement);

    return pathElement.attribute("d");
}

QVariant convertValue(const QByteArray &key, const QString &value)
{
    if (key == "fillOpacity" || key == "strokeOpacity") {
        if (value.contains("%"))
            return QString(value).replace("%", "").toFloat() / 100.0f;

        return value.toFloat();
    } else if (key == "strokeWidth") {
        return value.toInt();
    } else if (key == "opacity") {
        return value.toFloat();
    } else if ((key == "fillColor" || key == "strokeColor") && value == "none") {
        return "transparent";
    }

    return value;
}

CSSRule parseCSSRule(const QString &ruleStr)
{
    static const QRegularExpression reRules(R"(([\s\S]*?):([\s\S]*?)(?:;|;?$))");

    CSSRule rule;
    QRegularExpressionMatchIterator i = reRules.globalMatch(ruleStr);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();

        if (match.lastCapturedIndex() != 2)
            continue;

        CSSProperty property;
        property.directive = match.captured(1).trimmed();
        property.value = match.captured(2).trimmed();

        rule.push_back(property);
    }

    return rule;
}

CSSRules parseCSS(const QDomElement &styleElement)
{
    static const QRegularExpression reCSS(R"(([\s\S]*?){([\s\S]*?)})");

    CSSRules cssRules;
    QRegularExpressionMatchIterator i = reCSS.globalMatch(styleElement.text().simplified());
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();

        if (match.lastCapturedIndex() != 2)
            continue;

        cssRules.insert(match.captured(1).trimmed(),
                        parseCSSRule(match.captured(2).trimmed()));
    }

    return cssRules;
}

void applyCSSRules(const CSSRule &cssRule, PropertyMap &properties)
{
    for (const CSSProperty &property : cssRule) {
        const QString directive = property.directive;
        if (auto iter = findKey(mapping, directive); iter != mapping.end()) {
            const QByteArray directive = iter->second.toUtf8();
            properties.insert(directive, convertValue(directive, property.value));
        }
    }
}

// This merges potential fill and/or stroke opacity with fill and/or stroke color
void mergeOpacity(PropertyMap &properties)
{
    auto merge = [&](const QByteArray &opacityKey, const QByteArray &colorKey) {

        if (!properties.contains(opacityKey))
            return;

        QColor color; // By default black same as SVG

        if (properties.contains(colorKey))
            color = QColor(properties[colorKey].toString());

        color.setAlphaF(properties[opacityKey].toFloat());
        // Insert/replace merged color and remove opacity
        properties.insert(colorKey, color.name(QColor::HexArgb));
        properties.remove(opacityKey);
    };

    merge("fillOpacity", "fillColor");
    merge("strokeOpacity", "strokeColor");
}

void flattenTransformsAndStyles(const QDomElement &element,
                                const CSSRules &cssRules,
                                QTransform &transform,
                                PropertyMap &properties)
{
    properties.insert("fillColor", "black"); // overwrite default fillColor
    properties.insert("strokeWidth", -1); // overwrite default strokeWidth of 4

    auto collectTransformAndStyle = [&](const QDomNode &node) {
        QDomElement e = node.toElement();
        transform *= parseTransform(e.attribute("transform"));

        // Parse and assign presentation attributes contained in mapping
        for (const auto &p : mapping) {
            const QString attributeValue = e.attribute(p.first.toString()).trimmed();
            if (attributeValue.isEmpty())
                continue;

            const QByteArray directive = p.second.toUtf8();
            properties.insert(directive, convertValue(directive, attributeValue));
        }

        // Parse and assign css styles
        if (e.hasAttribute("class")) {
            // Replace all commas with whitespaces, if there are commas contained
            const QString classStr = e.attribute("class").replace(",", " ").simplified();
            const QStringList classes = classStr.split(" ", Qt::SkipEmptyParts);

            for (const auto &c : classes)
                applyCSSRules(cssRules["." + c], properties);
        }

        if (e.hasAttribute("id")) {
            const QString id = e.attribute("id").simplified();
            applyCSSRules(cssRules["#" + id], properties);
        }

        // Parse and assign inline style
        if (e.hasAttribute("style")) {
            const QString rule = e.attribute("style").simplified();
            applyCSSRules(parseCSSRule(rule), properties);
        }
    };

    topToBottomTraversal(element, collectTransformAndStyle);

    mergeOpacity(properties);
}

bool applyMinimumBoundingBox(QPainterPath &path, PropertyMap &properties)
{
    const QRectF boundingRect = path.boundingRect();

    path.translate(-boundingRect.topLeft());

    properties.insert("x", round(boundingRect.x(), 2));
    properties.insert("y", round(boundingRect.y(), 2));
    properties.insert("width", round(boundingRect.width(), 2));
    properties.insert("height", round(boundingRect.height(), 2));

    const QString svgPath = convertQPainterPathtoSVGPath(path);

    if (svgPath.isEmpty())
        return false;

    properties.insert("path", svgPath);

    return true;
}

PropertyMap generateRectProperties(const QDomElement &e, const CSSRules &cssRules)
{
    QRectF rect(e.attribute("x").toFloat(),
                e.attribute("y").toFloat(),
                e.attribute("width").toFloat(),
                e.attribute("height").toFloat());

    if (!rect.isValid())
        return {};

    QPainterPath path;
    path.addRect(rect);

    PropertyMap properties;
    QTransform transform;
    flattenTransformsAndStyles(e, cssRules, transform, properties);

    path = transform.map(path);

    if (!applyMinimumBoundingBox(path, properties))
        return {};

    return properties;
}

PropertyMap generateLineProperties(const QDomElement &e, const CSSRules &cssRules)
{
    QLineF line(e.attribute("x1").toFloat(),
                e.attribute("y1").toFloat(),
                e.attribute("x2").toFloat(),
                e.attribute("y2").toFloat());

    QPainterPath path(line.p1());
    path.lineTo(line.p2());

    PropertyMap properties;
    QTransform transform;
    flattenTransformsAndStyles(e, cssRules, transform, properties);

    path = transform.map(path);

    if (!applyMinimumBoundingBox(path, properties))
        return {};

    return properties;
}

PropertyMap generateEllipseProperties(const QDomElement &e, const CSSRules &cssRules)
{
    const QPointF center(e.attribute("cx").toFloat(), e.attribute("cy").toFloat());
    qreal radiusX = 0;
    qreal radiusY = 0;

    if (e.tagName() == "circle")
        radiusX = radiusY = e.attribute("r").toFloat();
    else if (e.tagName() == "ellipse") {
        radiusX = e.attribute("rx").toFloat();
        radiusY = e.attribute("ry").toFloat();
    }

    if (radiusX <= 0 || radiusY <= 0)
        return {};

    QPainterPath path;
    path.addEllipse(center, radiusX, radiusY);

    PropertyMap properties;
    QTransform transform;
    flattenTransformsAndStyles(e, cssRules, transform, properties);

    path = transform.map(path);

    if (!applyMinimumBoundingBox(path, properties))
        return {};

    return properties;
}

PropertyMap generatePathProperties(const QDomElement &e, const CSSRules &cssRules)
{
    if (!e.hasAttribute("d"))
        return {};

    QPainterPath path;

    if (!parsePathDataFast(e.attribute("d"), path))
        return {};

    PropertyMap properties;
    QTransform transform;
    flattenTransformsAndStyles(e, cssRules, transform, properties);

    path = transform.map(path);

    if (!applyMinimumBoundingBox(path, properties))
        return {};

    return properties;
}

PropertyMap generatePolygonProperties(const QDomElement &e, const CSSRules &cssRules)
{
    if (!e.hasAttribute("points"))
        return {};

    // Replace all commas with whitespaces, if there are commas contained
    const QString pointsStr = e.attribute("points").replace(",", " ").simplified();
    const QStringList pointList = pointsStr.split(" ", Qt::SkipEmptyParts);

    if (pointList.isEmpty() || pointList.length() < 4)
        return {};

    QPolygonF polygon;

    for (int i = 0; i < pointList.length(); i += 2)
        polygon.push_back({pointList[i].toFloat(), pointList[i + 1].toFloat()});

    if (e.tagName() != "polyline" && !polygon.isClosed() && polygon.size())
        polygon.push_back(polygon.front());

    QPainterPath path;
    path.addPolygon(polygon);

    PropertyMap properties;
    QTransform transform;
    flattenTransformsAndStyles(e, cssRules, transform, properties);

    path = transform.map(path);

    if (!applyMinimumBoundingBox(path, properties))
        return {};

    return properties;
}

ModelNode createPathNode(ModelNode parent, const PropertyMap &properties)
{
    ItemLibraryEntry itemLibraryEntry;
    itemLibraryEntry.setName("SVG Path Item");
    itemLibraryEntry.setType("QtQuick.Studio.Components.SvgPathItem", 1, 0);

    ModelNode node = QmlItemNode::createQmlObjectNode(
                parent.view(), itemLibraryEntry, {}, parent.defaultNodeAbstractProperty(), false);

    PropertyMap::const_iterator i = properties.constBegin();
    while (i != properties.constEnd()) {
        node.variantProperty(i.key()).setValue(i.value());
        ++i;
    }

    return node;
}

ModelNode createGroupNode(ModelNode parent, const PropertyMap &properties)
{
    ItemLibraryEntry itemLibraryEntry;
    itemLibraryEntry.setName("Group");
    itemLibraryEntry.setType("QtQuick.Studio.Components.GroupItem", 1, 0);

    ModelNode node = QmlItemNode::createQmlObjectNode(
                parent.view(), itemLibraryEntry, {}, parent.defaultNodeAbstractProperty(), false);

    PropertyMap::const_iterator i = properties.constBegin();
    while (i != properties.constEnd()) {
        node.variantProperty(i.key()).setValue(i.value());
        ++i;
    }

    return node;
}

} // namespace

SVGPasteAction::SVGPasteAction()
    : m_domDocument()
{}

bool SVGPasteAction::containsSVG(const QString &str)
{
    if (!m_domDocument.setContent(str, true))
        return false; // TODO error reporting

    if (m_domDocument.documentElement().namespaceURI() == "http://www.w3.org/2000/svg")
        return true;

    return false;
}

QmlObjectNode SVGPasteAction::createQmlObjectNode(QmlDesigner::ModelNode &targetNode)
{
    const QDomElement rootElement = m_domDocument.documentElement();
    if (rootElement.isNull()) {
        qWarning() << Q_FUNC_INFO << "Couldn't create a root element.";
        return {};
    }

    PropertyMap viewBoxProperties;
    QRectF viewBox(0, 0, 100, 100);

    if (rootElement.hasAttribute("viewBox")) {
        // Replace all commas with whitespaces, if there are commas contained
        const QString viewBoxStr = rootElement.attribute("viewBox").replace(",", " ").simplified();
        const QStringList tmp = viewBoxStr.split(" ", Qt::SkipEmptyParts);

        if (tmp.size() == 4) {
            viewBox = QRectF(round(tmp[0].toFloat(), 2),
                             round(tmp[1].toFloat(), 2),
                             round(tmp[2].toFloat(), 2),
                             round(tmp[3].toFloat(), 2));
        }

        viewBoxProperties.insert("clip", true);
    } else {
        viewBox.setWidth(round(rootElement.attribute("width").toFloat(), 2));
        viewBox.setHeight(round(rootElement.attribute("height").toFloat(), 2));
    }

    viewBoxProperties.insert("x", viewBox.x());
    viewBoxProperties.insert("y", viewBox.y());
    viewBoxProperties.insert("width", viewBox.width());
    viewBoxProperties.insert("height", viewBox.height());

    const QDomNode node = rootElement.firstChild();
    std::vector<QDomElement> shapeElements;
    CSSRules cssRules;

    auto processStyleAndCollectShapes = [&](const QDomNode &node) {
        QDomElement element = node.toElement();
        if (!element.isNull()) {
            if (element.tagName() == "style"/* && element.attribute("type") == "text/css"*/)
                cssRules = parseCSS(element);

            if (contains(tagAllowList, element.tagName()))
                shapeElements.push_back(element);
        }
    };

    depthFirstTraversal(node, processStyleAndCollectShapes);

    ModelNode groupNode = createGroupNode(targetNode, viewBoxProperties);

    for (const QDomElement &e : shapeElements) {
        PropertyMap pathProperties;

        if (e.tagName() == "path")
            pathProperties = generatePathProperties(e, cssRules);
        else if (e.tagName() == "rect")
            pathProperties = generateRectProperties(e, cssRules);
        else if (e.tagName() == "line")
            pathProperties = generateLineProperties(e, cssRules);
        else if (e.tagName() == "polygon" || e.tagName() == "polyline")
            pathProperties = generatePolygonProperties(e, cssRules);
        else if (e.tagName() == "circle" || e.tagName() == "ellipse")
            pathProperties = generateEllipseProperties(e, cssRules);

        if (pathProperties.empty())
            continue;

        const QPointF topLeft = -viewBox.topLeft();

        pathProperties["x"] = pathProperties["x"].toDouble() + topLeft.x();
        pathProperties["y"] = pathProperties["y"].toDouble() + topLeft.y();

        createPathNode(groupNode, pathProperties);
    }

    return groupNode;
}

} // namespace QmlDesigner
