/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef PROPERTYCONTAINER_H
#define PROPERTYCONTAINER_H

#include "corelib_global.h"

#include <QString>
#include <QExplicitlySharedDataPointer>
#include <QDataStream>
#include <QVariant>

#include <enumeratormetainfo.h>

namespace QmlDesigner {

class PropertyContainer;

CORESHARED_EXPORT QDataStream &operator<<(QDataStream &stream, const PropertyContainer &propertyContainer);
CORESHARED_EXPORT QDataStream &operator>>(QDataStream &stream, PropertyContainer &propertyContainer);

class CORESHARED_EXPORT PropertyContainer
{
    friend CORESHARED_EXPORT QDataStream &QmlDesigner::operator<<(QDataStream &stream, const PropertyContainer &propertyContainer);
    friend CORESHARED_EXPORT QDataStream &QmlDesigner::operator>>(QDataStream &stream, PropertyContainer &propertyContainer);

public:
    PropertyContainer();
    PropertyContainer(const QString &name, const QString &type, const QVariant &value);

    bool isValid() const;

    QString name() const;
    QVariant value() const;
    QString type() const;


    void setValue(const QVariant &value);
    void set(const QString &name, const QString &type, const QVariant &value);


private:
    QString m_name;
    QString m_type;
    mutable QVariant m_value;
};

CORESHARED_EXPORT QDataStream &operator<<(QDataStream &stream, const QList<PropertyContainer> &propertyContainerList);
CORESHARED_EXPORT QDataStream &operator>>(QDataStream &stream, QList<PropertyContainer> &propertyContainerList);

} //namespace QmlDesigner



#endif //PROPERTYCONTAINER_H
