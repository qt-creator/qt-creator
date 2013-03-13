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

static const char kFilePathPostfix[] = ":FilePath";
static const char kPathPostfix[] = ":Path";
static const char kFileNamePostfix[] = ":FileName";
static const char kFileBaseNamePostfix[] = ":FileBaseName";

namespace Core {

class VMMapExpander : public Utils::AbstractQtcMacroExpander
{
public:
    virtual bool resolveMacro(const QString &name, QString *ret)
    {
        bool found;
        *ret = Core::VariableManager::value(name.toUtf8(), &found);
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
static VariableManagerPrivate *d;


VariableManager::VariableManager()
{
    d = new VariableManagerPrivate;
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
    variableManagerInstance->variableUpdateRequested(variable);
    if (found)
        *found = d->m_map.contains(variable);
    return d->m_map.value(variable);
}

QString VariableManager::expandedString(const QString &stringWithVariables)
{
    return Utils::expandMacros(stringWithVariables, macroExpander());
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

void VariableManager::registerFileVariables(const QByteArray &prefix, const QString &heading)
{
    registerVariable(prefix + kFilePathPostfix, tr("%1: Full path including file name.").arg(heading));
    registerVariable(prefix + kPathPostfix, tr("%1: Full path excluding file name.").arg(heading));
    registerVariable(prefix + kFileNamePostfix, tr("%1: File name without including path.").arg(heading));
    registerVariable(prefix + kFileBaseNamePostfix, tr("%1: File base name without path and suffix.").arg(heading));
}

bool VariableManager::isFileVariable(const QByteArray &variable, const QByteArray &prefix)
{
    return variable == prefix + kFilePathPostfix
            || variable == prefix + kPathPostfix
            || variable == prefix + kFileNamePostfix
            || variable == prefix + kFileBaseNamePostfix;
}

QString VariableManager::fileVariableValue(const QByteArray &variable, const QByteArray &prefix,
                                           const QString &fileName)
{
    return fileVariableValue(variable, prefix, QFileInfo(fileName));
}

QString VariableManager::fileVariableValue(const QByteArray &variable, const QByteArray &prefix,
                                           const QFileInfo &fileInfo)
{
    if (variable == prefix + kFilePathPostfix)
        return fileInfo.filePath();
    else if (variable == prefix + kPathPostfix)
        return fileInfo.path();
    else if (variable == prefix + kFileNamePostfix)
        return fileInfo.fileName();
    else if (variable == prefix + kFileBaseNamePostfix)
        return fileInfo.baseName();
    return QString();
}

QList<QByteArray> VariableManager::variables()
{
    return d->m_descriptions.keys();
}

QString VariableManager::variableDescription(const QByteArray &variable)
{
    return d->m_descriptions.value(variable);
}

} // namespace Core
