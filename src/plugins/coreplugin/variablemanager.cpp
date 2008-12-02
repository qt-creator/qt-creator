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
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
#include "variablemanager.h"

using namespace Core;

VariableManager *VariableManager::m_instance = 0;

VariableManager::VariableManager(QObject *parent) : QObject(parent)
{
    m_instance = this;
}

VariableManager::~VariableManager()
{
    m_instance = 0;
}

void VariableManager::insert(const QString &variable, const QString &value)
{
    m_map.insert(variable, value);
}

QString VariableManager::value(const QString &variable)
{
    return m_map.value(variable);
}

QString VariableManager::value(const QString &variable, const QString &defaultValue)
{
    return m_map.value(variable, defaultValue);
}

void VariableManager::remove(const QString &variable)
{
    m_map.remove(variable);
}

QString VariableManager::resolve(const QString &stringWithVariables)
{
    QString result = stringWithVariables;
    QMapIterator<QString, QString> i(m_map);
    while (i.hasNext()) {
        i.next();
        result.replace(QString("${%1}").arg(i.key()), i.value());
    }
    return result;
}
