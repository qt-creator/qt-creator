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

#ifndef PROPERTYCONTAINER_H
#define PROPERTYCONTAINER_H

#include "corelib_global.h"

#include <QString>
#include <QExplicitlySharedDataPointer>
#include <QDataStream>
#include <QVariant>

namespace QmlDesigner {

class PropertyContainer;

CORESHARED_EXPORT QDataStream &operator<<(QDataStream &stream, const PropertyContainer &propertyContainer);
CORESHARED_EXPORT QDataStream &operator>>(QDataStream &stream, PropertyContainer &propertyContainer);

class CORESHARED_EXPORT PropertyContainer
{
    friend CORESHARED_EXPORT QDataStream &operator<<(QDataStream &stream, const PropertyContainer &propertyContainer);
    friend CORESHARED_EXPORT QDataStream &operator>>(QDataStream &stream, PropertyContainer &propertyContainer);

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
