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

#include "variantparser.h"
#include <private/qmlvaluetype_p.h>

#include <QPoint>
#include <QPointF>
#include <QFont>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QVector3D>
#include <QMetaProperty>

namespace QmlDesigner {
namespace Internal {

VariantParser::VariantParser(const QVariant &value) : m_valueType(QmlValueTypeFactory::valueType(value.type()))
{
    if (m_valueType) {
        m_valueType->setValue(value);
        if (!m_valueType->value().isValid())
            qWarning("VariantParser::VariantParser: value not valid");
    }
}

VariantParser::~VariantParser()
{
    if (m_valueType)
        delete m_valueType;
}

QVariant VariantParser::value() const
{
    return m_valueType->value();
}

QVariant VariantParser::property(QString name) const
{
    if (!m_valueType)
        return QVariant();
    return m_valueType->property(name.toLatin1());
}

bool VariantParser::setProperty(const QString &name, const QVariant &value)
{
    if (!m_valueType)
        return false;
    if (name == "pixelSize" && value.toInt() < 1)
        return false; //this check we have to harcode
    return m_valueType->setProperty(name.toLatin1(), value);
}

bool VariantParser::isValueType(const QString &type)
{
    return VariantParser::create(type).isValid();
}

QStringList VariantParser::properties()
{
    if (!m_valueType)
        return QStringList();
    QStringList propertyList;
    for (int i=1; i < m_valueType->metaObject()->propertyCount(); i++) {
        QMetaProperty metaProperty = m_valueType->metaObject()->property(i);
        propertyList.append(metaProperty.name());
    }
    return propertyList;
}

VariantParser VariantParser::create(const QString &type)
{
    if (type == "QFont")
        return VariantParser(QVariant(QFont()));
    if (type == "QPoint")
        return VariantParser(QVariant(QPoint()));
    if (type == "QPointF")
        return VariantParser(QVariant(QPointF()));
    if (type == "QSize")
        return VariantParser(QVariant(QSize()));
    if (type == "QSizeF")
        return VariantParser(QVariant(QSizeF()));
    if (type == "QRect")
        return VariantParser(QVariant(QRect()));
    if (type == "QRectF")
        return VariantParser(QVariant(QRectF()));
    if (type == "QVector3D")
        return VariantParser(QVariant(QVector3D()));

    return VariantParser(QVariant());
}

void VariantParser::init(const QString &type)
{
    if (type == "QFont")
        m_valueType  = QmlValueTypeFactory::valueType(QVariant::Font);
    if (type == "QPoint")
        m_valueType  = QmlValueTypeFactory::valueType(QVariant::Point);
    if (type == "QPointF")
        m_valueType  = QmlValueTypeFactory::valueType(QVariant::PointF);
    if (type == "QSize")
        m_valueType  = QmlValueTypeFactory::valueType(QVariant::Size);
    if (type == "QSizeF")
        m_valueType  = QmlValueTypeFactory::valueType(QVariant::SizeF);
    if (type == "QRect")
        m_valueType  = QmlValueTypeFactory::valueType(QVariant::Rect);
    if (type == "QRectF")
        m_valueType  = QmlValueTypeFactory::valueType(QVariant::RectF);
    if (type == "QVector3D")
        m_valueType  = QmlValueTypeFactory::valueType(QVariant::Vector3D);
}

bool VariantParser::isValid()
{
    return m_valueType && m_valueType->value().isValid();
}

} // namespace Internal
} // namespace QmlDesigner


