/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "buildconfiguration.h"

using namespace ProjectExplorer;

BuildConfiguration::BuildConfiguration(const QString &name)
    : m_name(name)
{
    setDisplayName(name);
}

BuildConfiguration::BuildConfiguration(const QString &name, BuildConfiguration *source)
    : m_values(source->m_values), m_name(name)
{
}

QString BuildConfiguration::name() const
{
    return m_name;
}

QString BuildConfiguration::displayName()
{
    QVariant v = getValue("ProjectExplorer.BuildConfiguration.DisplayName");
    if (v.isValid()) {
        return v.toString();
    } else {
        setDisplayName(m_name);
        return m_name;
    }
}

void BuildConfiguration::setDisplayName(const QString &name)
{
    setValue("ProjectExplorer.BuildConfiguration.DisplayName", name);
}

QVariant BuildConfiguration::getValue(const QString & key) const
{
    QHash<QString, QVariant>::const_iterator it = m_values.find(key);
    if (it != m_values.constEnd())
        return *it;
    else
        return QVariant();
}

void BuildConfiguration::setValue(const QString & key, QVariant value)
{
    m_values[key] = value;
}

void BuildConfiguration::setValuesFromMap(QMap<QString, QVariant> map)
{
    QMap<QString, QVariant>::const_iterator it, end;
    end = map.constEnd();
    for (it = map.constBegin(); it != end; ++it)
        setValue(it.key(), it.value());
}

QMap<QString, QVariant> BuildConfiguration::toMap() const
{
    QMap<QString, QVariant> result;
    QHash<QString, QVariant>::const_iterator it, end;
    end = m_values.constEnd();
    for (it = m_values.constBegin(); it != end; ++it)
        result.insert(it.key(), it.value());
    return result;
}

