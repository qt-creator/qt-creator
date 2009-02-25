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

#include "proiteminfo.h"

#include <QtCore/QFile>

using namespace Qt4ProjectManager::Internal;

ProItemInfo::ProItemInfo(ProItemInfoKind kind)
    : m_kind(kind)
{ }

ProItemInfo::ProItemInfoKind ProItemInfo::kind() const
{
    return m_kind;
}

void ProItemInfo::setId(const QString &id)
{
    m_id = id;
}

void ProItemInfo::setName(const QString &name)
{
    m_name = name;
}

void ProItemInfo::setDescription(const QString &desc)
{
    m_description = desc;
}

QString ProItemInfo::id() const
{
    return m_id;
}

QString ProItemInfo::name() const
{
    return m_name;
}

QString ProItemInfo::description() const
{
    return m_description;
}

ProScopeInfo::ProScopeInfo()
    : ProItemInfo(ProItemInfo::Scope)
{ }

ProValueInfo::ProValueInfo()
    : ProItemInfo(ProItemInfo::Value)
{ }

ProVariableInfo::ProVariableInfo()
    : ProItemInfo(ProItemInfo::Variable)
{
    m_operator = ProVariable::SetOperator;
}

ProVariableInfo::~ProVariableInfo()
{
    qDeleteAll(m_values.values());
}

void ProVariableInfo::addValue(ProValueInfo *value)
{
    m_values.insert(value->id(), value);    
}

void ProVariableInfo::setMultiple(bool multiple)
{
    m_multiple = multiple;
}

void ProVariableInfo::setDefaultOperator(ProVariable::VariableOperator op)
{
    m_operator = op;
}

ProValueInfo *ProVariableInfo::value(const QString &id) const
{
    return m_values.value(id, 0);
}

QList<ProValueInfo *> ProVariableInfo::values() const
{
    return m_values.values();
}

bool ProVariableInfo::multiple() const
{
    return m_multiple;
}

ProVariable::VariableOperator ProVariableInfo::defaultOperator() const
{
    return m_operator;
}

ProItemInfoManager::ProItemInfoManager(QObject *parent)
    : QObject(parent)
{
    load(QLatin1String(":/proparser/proiteminfo.xml"));
}

ProItemInfoManager::~ProItemInfoManager()
{
    qDeleteAll(m_variables.values());
    qDeleteAll(m_scopes.values());
}

void ProItemInfoManager::addVariable(ProVariableInfo *variable)
{
    m_variables.insert(variable->id(), variable);
}

void ProItemInfoManager::addScope(ProScopeInfo *scope)
{
    m_scopes.insert(scope->id(), scope);
}

ProVariableInfo *ProItemInfoManager::variable(const QString &id) const
{
    return m_variables.value(id, 0);
}

ProScopeInfo *ProItemInfoManager::scope(const QString &id) const
{
    return m_scopes.value(id, 0);
}

QList<ProScopeInfo *> ProItemInfoManager::scopes() const
{
    return m_scopes.values();
}

QList<ProVariableInfo *> ProItemInfoManager::variables() const
{
    return m_variables.values();
}

bool ProItemInfoManager::load(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QDomDocument doc;
    if (!doc.setContent(&file))
        return false;

    QDomElement root = doc.documentElement();
    if (root.nodeName() != QLatin1String("proiteminfo"))
        return false;

    QDomElement child = root.firstChildElement();
    for (; !child.isNull(); child = child.nextSiblingElement()) {
        if (child.nodeName() == QLatin1String("scope"))
            readScope(child);
        else if (child.nodeName() == QLatin1String("variable"))
            readVariable(child);
    }

    file.close();
    return true;
}

void ProItemInfoManager::readItem(ProItemInfo *item, const QDomElement &data)
{
    QDomElement child = data.firstChildElement();
    for (; !child.isNull(); child = child.nextSiblingElement()) {
        if (child.nodeName() == QLatin1String("id"))
            item->setId(child.text());
        else if (child.nodeName() == QLatin1String("name"))
            item->setName(child.text());
        else if (child.nodeName() == QLatin1String("description"))
            item->setDescription(child.text());
    }
}

void ProItemInfoManager::readScope(const QDomElement &data)
{
    ProScopeInfo *scope = new ProScopeInfo();
    readItem(scope, data);
    addScope(scope);
}

void ProItemInfoManager::readVariable(const QDomElement &data)
{
    ProVariableInfo *var = new ProVariableInfo();
    readItem(var, data);

    var->setMultiple(data.attribute(QLatin1String("multiple"), QLatin1String("false")) == QLatin1String("true"));
    var->setDefaultOperator((ProVariable::VariableOperator)data.attribute(QLatin1String("operator"),
        QLatin1String("3")).toInt());

    QDomElement child = data.firstChildElement();
    for (; !child.isNull(); child = child.nextSiblingElement()) {
        if (child.nodeName() == QLatin1String("value")) {
            ProValueInfo *val = new ProValueInfo();
            readItem(val, child);
            var->addValue(val);
        }
    }

    addVariable(var);
}

