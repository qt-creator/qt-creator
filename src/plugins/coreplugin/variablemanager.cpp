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
