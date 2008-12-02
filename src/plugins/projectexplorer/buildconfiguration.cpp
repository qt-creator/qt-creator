/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#include "buildconfiguration.h"

using namespace ProjectExplorer;

BuildConfiguration::BuildConfiguration(const QString &name)
    : m_name(name)
{
    setDisplayName(name);
}

BuildConfiguration::BuildConfiguration(const QString &name, BuildConfiguration *source)
    :m_values(source->m_values), m_name(name)
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
    if(it != m_values.constEnd())
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
    for(it = map.constBegin(); it != end; ++it)
        setValue(it.key(), it.value());
}

QMap<QString, QVariant> BuildConfiguration::toMap() const
{
    QMap<QString, QVariant> result;
    QHash<QString, QVariant>::const_iterator it, end;
    end = m_values.constEnd();
    for(it = m_values.constBegin(); it != end; ++it)
        result.insert(it.key(), it.value());
    return result;
}

