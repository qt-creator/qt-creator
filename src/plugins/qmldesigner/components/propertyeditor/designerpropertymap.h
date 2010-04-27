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

#ifndef DESIGNERPROPERTYMAP_H
#define DESIGNERPROPERTYMAP_H

#include <QDeclarativePropertyMap>
#include <qdeclarative.h>

namespace QmlDesigner {

template <class DefaultType>
class DesignerPropertyMap : public QDeclarativePropertyMap
{

public:
    DesignerPropertyMap(QObject *parent = 0);

    QVariant value(const QString &key) const;

    static void registerDeclarativeType(const QString &name);
private:
    DefaultType m_defaultValue;
};

template <class DefaultType>
DesignerPropertyMap<DefaultType>::DesignerPropertyMap(QObject *parent) : QDeclarativePropertyMap(parent), m_defaultValue(this)
{
}

template <class DefaultType>
QVariant DesignerPropertyMap<DefaultType>::value(const QString &key) const
{
    if (contains(key))
        return QDeclarativePropertyMap::value(key);
    return QVariant(&m_defaultValue);
}


template <class DefaultType>
void DesignerPropertyMap<DefaultType>::registerDeclarativeType(const QString &name)
{
    typedef DesignerPropertyMap<DefaultType> myType;
    qmlRegisterType<myType>("Bauhaus",1,0,name);
}

} //QmlDesigner

#endif // DESIGNERPROPERTYMAP_H
