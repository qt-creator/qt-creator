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

#include "variablemanager.h"
#include "idocument.h"
#include "editormanager/ieditor.h"
#include "editormanager/editormanager.h"

#include <utils/stringutils.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QObject>
#include <QMap>
#include <QDebug>

namespace Core {

class VMMapExpander : public Utils::AbstractQtcMacroExpander
{
public:
    virtual bool resolveMacro(const QString &name, QString *ret)
    {
        bool found;
        *ret = Core::VariableManager::instance()->value(name.toUtf8(), &found);
        return found;
    }
};

class VariableManagerPrivate
{
public:
    QHash<QByteArray, QString> m_map;
    VMMapExpander m_macroExpander;
    QMap<QByteArray, QString> m_descriptions;
};

/*!
    \class Core::VariableManager
    \brief The VaraiableManager class holds a generic map from variable names
    to string values.

    The names of the variables are stored as QByteArray. They are typically
    7-bit-clean. In cases where this is not possible, Latin-1 encoding is
    assumed.
*/

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

void VariableManager::insert(const QByteArray &variable, const QString &value)
{
    d->m_map.insert(variable, value);
}

bool VariableManager::remove(const QByteArray &variable)
{
    return d->m_map.remove(variable) > 0;
}

QString VariableManager::value(const QByteArray &variable, bool *found)
{
    emit variableUpdateRequested(variable);
    if (found)
        *found = d->m_map.contains(variable);
    return d->m_map.value(variable);
}

Utils::AbstractMacroExpander *VariableManager::macroExpander()
{
    return &d->m_macroExpander;
}

VariableManager *VariableManager::instance()
{
    return variableManagerInstance;
}

void VariableManager::registerVariable(const QByteArray &variable, const QString &description)
{
    d->m_descriptions.insert(variable, description);
}

QList<QByteArray> VariableManager::variables() const
{
    return d->m_descriptions.keys();
}

QString VariableManager::variableDescription(const QByteArray &variable) const
{
    return d->m_descriptions.value(variable);
}

} // namespace Core
