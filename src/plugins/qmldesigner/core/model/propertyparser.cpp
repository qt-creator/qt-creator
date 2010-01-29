/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "propertyparser.h"
#include <QUrl>
#include <QtCore/QDebug>
#include <QtDeclarative/private/qmlstringconverters_p.h>
#include <modelnode.h>
#include <metainfo.h>

namespace QmlDesigner {
namespace Internal {
namespace PropertyParser {

static QVariant fromEnum(const QString &string, const QString &type, const MetaInfo &metaInfo)
{
    if (string.isEmpty())
        return QVariant();

    // TODO Use model metainfo
    EnumeratorMetaInfo enumerator = metaInfo.enumerator(type);
    int value = enumerator.elementValue(string);
    return QVariant(value);
}

QVariant read(const QString &typeStr, const QString &str, const MetaInfo &metaInfo)
{
    if (metaInfo.hasEnumerator(typeStr)) {
        return fromEnum(str, typeStr, metaInfo);
    }

    return read(typeStr, str);
}

QVariant read(const QString &typeStr, const QString &str)
{
    QMetaType::Type type = static_cast<QMetaType::Type>(QMetaType::type(typeStr.toAscii().constData()));
    if (type == 0)
        qWarning() << "Type " << typeStr
                << " is unknown to QMetaType system. Cannot create properly typed QVariant for value "
                << str;

    QVariant value;

    bool conversionOk = true;
    switch (type) {
    case QMetaType::QPoint:
        value = QmlStringConverters::pointFFromString(str, &conversionOk).toPoint();
        break;
    case QMetaType::QPointF:
        value = QmlStringConverters::pointFFromString(str, &conversionOk);
        break;
    case QMetaType::QSize:
        value = QmlStringConverters::sizeFFromString(str, &conversionOk).toSize();
        break;
    case QMetaType::QSizeF:
        value = QmlStringConverters::sizeFFromString(str, &conversionOk);
        break;
    case QMetaType::QRect:
        value = QmlStringConverters::rectFFromString(str, &conversionOk).toRect();
        break;
    case QMetaType::QRectF:
        value = QmlStringConverters::rectFFromString(str, &conversionOk);
        break;
    case QMetaType::QUrl:
        value = QVariant(QUrl(str));
        break;
    case QMetaType::QColor:
        value = QmlStringConverters::colorFromString(str);
        break;
    default: {
        value = QVariant(str);
        QVariant::Type varType = static_cast<QVariant::Type>(type);
        value.convert(varType);
        break;
        }
    }

    if (!conversionOk) {
        value = QVariant();
        qWarning() << "Could not convert" << str << "to" << QMetaType::typeName(type);
    }

    return value;
}

QString write(const QVariant &variant)
{
    if (!variant.isValid()) {
        qWarning() << "Trying to serialize invalid QVariant";
        return QString();
    }
    QString value;
    switch (variant.type()) {
    case QMetaType::QPoint:
    {
        QPoint p = variant.toPoint();
        value = QString("%1,%2").arg(QString::number(p.x()), QString::number(p.y()));
        break;
    }
    case QMetaType::QPointF:
    {
        QPointF p = variant.toPointF();
        value = QString("%1,%2").arg(QString::number(p.x(), 'f'), QString::number(p.y(), 'f'));
        break;
    }
    case QMetaType::QSize:
    {
        QSize s = variant.toSize();
        value = QString("%1x%2").arg(QString::number(s.width()), QString::number(s.height()));
        break;
    }
    case QMetaType::QSizeF:
    {
        QSizeF s = variant.toSizeF();
        value = QString("%1x%2").arg(QString::number(s.width(), 'f'), QString::number(s.height(), 'f'));
        break;
    }
    case QMetaType::QRect:
    {
        QRect r = variant.toRect();
        value = QString("%1,%2,%3x%4").arg(QString::number(r.x()), QString::number(r.y()),
                                           QString::number(r.width()), QString::number(r.height()));
        break;
    }
    case QMetaType::QRectF:
    {
        QRectF r = variant.toRectF();
        value = QString("%1,%2,%3x%4").arg(QString::number(r.x(), 'f'), QString::number(r.y(), 'f'),
                                           QString::number(r.width(), 'f'), QString::number(r.height(), 'f'));
        break;
    }
    default:
        QVariant strVariant = variant;
        strVariant.convert(QVariant::String);
        if (!strVariant.isValid())
            qWarning() << Q_FUNC_INFO << "cannot serialize type " << QMetaType::typeName(variant.type());
        value = strVariant.toString();
    }

    return value;
}

} // namespace PropertyParser
} // namespace Internal
} // namespace Designer

