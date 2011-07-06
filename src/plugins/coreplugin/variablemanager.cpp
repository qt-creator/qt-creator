/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "variablemanager.h"
#include "ifile.h"
#include "editormanager/ieditor.h"
#include "editormanager/editormanager.h"

#include <utils/stringutils.h>
#include <utils/qtcassert.h>

#include <QtCore/QFileInfo>
#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QDebug>

namespace Core {

class VMMapExpander : public Utils::AbstractQtcMacroExpander
{
public:
    virtual bool resolveMacro(const QString &name, QString *ret)
    {
        bool found;
        *ret = Core::VariableManager::instance()->value(name, &found);
        return found;
    }
};

class VariableManagerPrivate
{
public:
    QHash<QString, QString> m_map;
    VMMapExpander m_macroExpander;
    QMap<QString, QString> m_descriptions;
};

static VariableManager *variableManagerInstance = 0;

VariableManager::VariableManager() : d(new VariableManagerPrivate)
{
    variableManagerInstance = this;
}

VariableManager::~VariableManager()
{
    variableManagerInstance = 0;
    delete d;
}

void VariableManager::insert(const QString &variable, const QString &value)
{
    d->m_map.insert(variable, value);
}

bool VariableManager::remove(const QString &variable)
{
    return d->m_map.remove(variable) > 0;
}

QString VariableManager::value(const QString &variable, bool *found)
{
    emit variableUpdateRequested(variable);
    if (found) {
        *found = d->m_map.contains(variable);
    }
    return d->m_map.value(variable);
}

QString VariableManager::value(const QString &variable, const QString &defaultValue)
{
    emit variableUpdateRequested(variable);
    return d->m_map.value(variable, defaultValue);
}

Utils::AbstractMacroExpander *VariableManager::macroExpander()
{
    return &d->m_macroExpander;
}

VariableManager *VariableManager::instance()
{
    return variableManagerInstance;
}

void VariableManager::registerVariable(const QString &variable, const QString &description)
{
    d->m_descriptions.insert(variable, description);
}

QList<QString> VariableManager::variables() const
{
    return d->m_descriptions.keys();
}

QString VariableManager::variableDescription(const QString &variable) const
{
    return d->m_descriptions.value(variable);
}

} // namespace Core
