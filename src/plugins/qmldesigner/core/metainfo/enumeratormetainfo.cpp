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

#include "enumeratormetainfo.h"

#include <QSharedData>
#include <QString>
#include <QMap>
#include <QtDebug>

namespace QmlDesigner {

namespace Internal {

class EnumeratorMetaInfoData : public QSharedData
{
public:
    QString name;
    QString scope;
    bool isFlagType;
    bool isValid;
    QMap<QString, int> elements;
};

}

/*!
\class QmlDesigner::EnumeratorMetaInfo
\ingroup CoreModel
\brief The EnumeratorMetaInfo class provides meta information about an enumerator type.

TODO

\see QmlDesigner::MetaInfo, QmlDesigner::NodeMetaInfo, QmlDesigner::PropertyMetaInfo
*/

EnumeratorMetaInfo::EnumeratorMetaInfo()
    : m_data(new Internal::EnumeratorMetaInfoData)
{
    m_data->isFlagType = false;
    m_data->isValid = false;
}

EnumeratorMetaInfo::~EnumeratorMetaInfo()
{}

EnumeratorMetaInfo::EnumeratorMetaInfo(const EnumeratorMetaInfo &other)
    : m_data(other.m_data)
{
}

EnumeratorMetaInfo& EnumeratorMetaInfo::operator=(const EnumeratorMetaInfo &other)
{
    if (this !=&other)
        m_data = other.m_data;

    return *this;
}

QString EnumeratorMetaInfo::name() const
{
    return m_data->name;
}

QString EnumeratorMetaInfo::scope() const
{
    return m_data->scope;
}

bool EnumeratorMetaInfo::isValid() const
{
    return m_data->isValid;
}

QString EnumeratorMetaInfo::scopeAndName(const QString &combiner) const
{
    return m_data->scope + combiner + m_data->name;
}

QList<QString> EnumeratorMetaInfo::elementNames() const
{
    return m_data->elements.keys();
}

int EnumeratorMetaInfo::elementValue(const QString &enumeratorName) const
{
    QString possibleScope = scope();
    if (!possibleScope.isEmpty())
      possibleScope.append("::");
    QString unscoped = enumeratorName;
    unscoped.remove(possibleScope);
    return m_data->elements.value(unscoped, -1);
}

void EnumeratorMetaInfo::setScope(const QString &scope)
{
    Q_ASSERT(!scope.isEmpty());
    m_data->scope = scope;
}

void EnumeratorMetaInfo::setName(const QString &name)
{
    Q_ASSERT(!name.isEmpty());
    m_data->name = name;
}

void EnumeratorMetaInfo::addElement(const QString &enumeratorName, int enumeratorValue)
{
    m_data->elements.insert(enumeratorName, enumeratorValue);
}

bool EnumeratorMetaInfo::isFlagType() const
{
    return m_data->isFlagType;
}

void EnumeratorMetaInfo::setIsFlagType(bool isFlagType)
{
    m_data->isFlagType = isFlagType;
}

void EnumeratorMetaInfo::setValid(bool valid)
{
    m_data->isValid = valid;
}

}
