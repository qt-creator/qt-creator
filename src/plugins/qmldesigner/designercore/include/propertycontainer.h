/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef PROPERTYCONTAINER_H
#define PROPERTYCONTAINER_H

#include "qmldesignercorelib_global.h"

#include <QString>
#include <QDataStream>
#include <QVariant>

namespace QmlDesigner {

class PropertyContainer;

QMLDESIGNERCORE_EXPORT QDataStream &operator<<(QDataStream &stream, const PropertyContainer &propertyContainer);
QMLDESIGNERCORE_EXPORT QDataStream &operator>>(QDataStream &stream, PropertyContainer &propertyContainer);
QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const PropertyContainer &propertyContainer);

class QMLDESIGNERCORE_EXPORT PropertyContainer
{
    friend QMLDESIGNERCORE_EXPORT QDataStream &operator<<(QDataStream &stream, const PropertyContainer &propertyContainer);
    friend QMLDESIGNERCORE_EXPORT QDataStream &operator>>(QDataStream &stream, PropertyContainer &propertyContainer);
    friend QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const PropertyContainer &propertyContainer);

public:
    PropertyContainer();
    PropertyContainer(const PropertyName &name, const QString &type, const QVariant &value);

    bool isValid() const;

    PropertyName name() const;
    QVariant value() const;
    QString type() const;


    void setValue(const QVariant &value);
    void set(const PropertyName &name, const QString &type, const QVariant &value);


private:
    PropertyName m_name;
    QString m_type;
    mutable QVariant m_value;
};

QMLDESIGNERCORE_EXPORT QDataStream &operator<<(QDataStream &stream, const QList<PropertyContainer> &propertyContainerList);
QMLDESIGNERCORE_EXPORT QDataStream &operator>>(QDataStream &stream, QList<PropertyContainer> &propertyContainerList);
QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, QList<PropertyContainer> &propertyContainerList);

} //namespace QmlDesigner



#endif //PROPERTYCONTAINER_H
