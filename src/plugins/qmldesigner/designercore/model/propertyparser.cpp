/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "propertyparser.h"
#include "enumeration.h"
#include <modelnode.h>
#include <metainfo.h>

#include <QUrl>
#include <QVector3D>
#include <QDebug>

namespace {

static uchar fromHex(const uchar c, const uchar c2)
{
    uchar rv = 0;
    if (c >= '0' && c <= '9')
        rv += (c - '0') * 16;
    else if (c >= 'A' && c <= 'F')
        rv += (c - 'A' + 10) * 16;
    else if (c >= 'a' && c <= 'f')
        rv += (c - 'a' + 10) * 16;

    if (c2 >= '0' && c2 <= '9')
        rv += (c2 - '0');
    else if (c2 >= 'A' && c2 <= 'F')
        rv += (c2 - 'A' + 10);
    else if (c2 >= 'a' && c2 <= 'f')
        rv += (c2 - 'a' + 10);

    return rv;
}

static uchar fromHex(const QString &s, int idx)
{
    uchar c = s.at(idx).toLatin1();
    uchar c2 = s.at(idx + 1).toLatin1();
    return fromHex(c, c2);
}

QColor colorFromString(const QString &s, bool *ok)
{
    if (s.length() == 9 && s.startsWith(QLatin1Char('#'))) {
        uchar a = fromHex(s, 1);
        uchar r = fromHex(s, 3);
        uchar g = fromHex(s, 5);
        uchar b = fromHex(s, 7);
        if (ok) *ok = true;
        return QColor(r, g, b, a);
    } else {
        QColor rv(s);
        if (ok) *ok = rv.isValid();
        return rv;
    }
}

QPointF pointFFromString(const QString &s, bool *ok)
{
    if (s.count(QLatin1Char(',')) != 1) {
        if (ok)
            *ok = false;
        return QPointF();
    }

    bool xGood, yGood;
    int index = s.indexOf(QLatin1Char(','));
    qreal xCoord = s.left(index).toDouble(&xGood);
    qreal yCoord = s.mid(index+1).toDouble(&yGood);
    if (!xGood || !yGood) {
        if (ok)
            *ok = false;
        return QPointF();
    }

    if (ok)
        *ok = true;
    return QPointF(xCoord, yCoord);
}

QRectF rectFFromString(const QString &s, bool *ok)
{
    if (s.count(QLatin1Char(',')) != 2 || s.count(QLatin1Char('x')) != 1) {
        if (ok)
            *ok = false;
        return QRectF();
    }

    bool xGood, yGood, wGood, hGood;
    int index = s.indexOf(QLatin1Char(','));
    qreal x = s.left(index).toDouble(&xGood);
    int index2 = s.indexOf(QLatin1Char(','), index+1);
    qreal y = s.mid(index+1, index2-index-1).toDouble(&yGood);
    index = s.indexOf(QLatin1Char('x'), index2+1);
    qreal width = s.mid(index2+1, index-index2-1).toDouble(&wGood);
    qreal height = s.mid(index+1).toDouble(&hGood);
    if (!xGood || !yGood || !wGood || !hGood) {
        if (ok)
            *ok = false;
        return QRectF();
    }

    if (ok)
        *ok = true;
    return QRectF(x, y, width, height);
}

QSizeF sizeFFromString(const QString &s, bool *ok)
{
    if (s.count(QLatin1Char('x')) != 1) {
        if (ok)
            *ok = false;
        return QSizeF();
    }

    bool wGood, hGood;
    int index = s.indexOf(QLatin1Char('x'));
    qreal width = s.left(index).toDouble(&wGood);
    qreal height = s.mid(index+1).toDouble(&hGood);
    if (!wGood || !hGood) {
        if (ok)
            *ok = false;
        return QSizeF();
    }

    if (ok)
        *ok = true;
    return QSizeF(width, height);
}

QVector3D vector3DFromString(const QString &s, bool *ok)
{
    if (s.count(QLatin1Char(',')) != 2) {
        if (ok)
            *ok = false;
        return QVector3D();
    }

    bool xGood, yGood, zGood;
    int index = s.indexOf(QLatin1Char(','));
    int index2 = s.indexOf(QLatin1Char(','), index+1);
    qreal xCoord = s.left(index).toDouble(&xGood);
    qreal yCoord = s.mid(index+1, index2-index-1).toDouble(&yGood);
    qreal zCoord = s.mid(index2+1).toDouble(&zGood);
    if (!xGood || !yGood || !zGood) {
        if (ok)
            *ok = false;
        return QVector3D();
    }

    if (ok)
        *ok = true;
    return QVector3D(xCoord, yCoord, zCoord);
}

QmlDesigner::Enumeration enumerationFromString(const QString &string, bool *ok)
{
    QmlDesigner::Enumeration tEnumeration(string);
    if (ok)
        *ok = !tEnumeration.scope().isEmpty() && !tEnumeration.name().isEmpty();
    return tEnumeration;
}


} //namespace

namespace QmlDesigner {
namespace Internal {
namespace PropertyParser {

QVariant read(const QString &typeStr, const QString &str, const MetaInfo &)
{
    return read(typeStr, str);
}

QVariant read(const QString &typeStr, const QString &str)
{
    int type = QMetaType::type(typeStr.toUtf8().constData());
    if (type == 0) {
        if (typeStr != QStringLiteral("binding")) {
            qWarning() << "Type " << typeStr
                    << " is unknown to QMetaType system. Cannot create properly typed QVariant for value "
                    << str;
        }
        // Fall back to a QVariant of type String
        return QVariant(str);
    }
    return read(type, str);
}

QVariant read(int variantType, const QString &str)
{
    QVariant value;

    bool conversionOk = true;
    switch (variantType) {
    case QMetaType::QPoint:
        value = pointFFromString(str, &conversionOk).toPoint();
        break;
    case QMetaType::QPointF:
        value = pointFFromString(str, &conversionOk);
        break;
    case QMetaType::QSize:
        value = sizeFFromString(str, &conversionOk).toSize();
        break;
    case QMetaType::QSizeF:
        value = sizeFFromString(str, &conversionOk);
        break;
    case QMetaType::QRect:
        value = rectFFromString(str, &conversionOk).toRect();
        break;
    case QMetaType::QRectF:
        value = rectFFromString(str, &conversionOk);
        break;
    case QMetaType::QUrl:
        value = QVariant(QUrl(str));
        break;
    case QMetaType::QColor:
        value = colorFromString(str, &conversionOk);
        break;
    case QMetaType::QVector3D:
        value = vector3DFromString(str, &conversionOk);
        break;
    default: {
        if (variantType == QMetaType::type("Enumeration")) {
            value = QVariant::fromValue<Enumeration>(enumerationFromString(str, &conversionOk));
        } else {
            value = QVariant(str);
            value.convert(static_cast<QVariant::Type>(variantType));
        }
        break;
        }
    }

    if (!conversionOk) {
        qWarning() << "Could not convert" << str
                   << "to" << QMetaType::typeName(variantType);
        value = QVariant(str);
    }

    return value;
}

QVariant variantFromString(const QString &s)
{
    if (s.isEmpty())
        return QVariant(s);
    bool ok = false;
    QRectF r = rectFFromString(s, &ok);
    if (ok) return QVariant(r);
    QColor c = colorFromString(s, &ok);
    if (ok) return QVariant(c);
    QPointF p = pointFFromString(s, &ok);
    if (ok) return QVariant(p);
    QSizeF sz = sizeFFromString(s, &ok);
    if (ok) return QVariant(sz);
    QVector3D v = vector3DFromString(s, &ok);
    if (ok) return QVariant::fromValue(v);

    return QVariant(s);
}

} // namespace PropertyParser
} // namespace Internal
} // namespace Designer

