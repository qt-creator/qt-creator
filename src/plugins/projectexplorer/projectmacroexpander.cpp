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

#include "projectmacroexpander.h"
#include "kit.h"
#include "projectexplorerconstants.h"

#include <coreplugin/variablemanager.h>

using namespace ProjectExplorer;

ProjectExpander::ProjectExpander(const QString &projectFilePath, const QString &projectName,
                                 const Kit *k, const QString &bcName)
    : m_projectFile(projectFilePath), m_projectName(projectName), m_kit(k), m_bcName(bcName)
{ }

bool ProjectExpander::resolveMacro(const QString &name, QString *ret)
{
    QString result;
    bool found = false;
    if (name == QLatin1String(ProjectExplorer::Constants::VAR_CURRENTPROJECT_NAME)) {
        result = m_projectName;
        found = true;
    } else if (name == QLatin1String(ProjectExplorer::Constants::VAR_CURRENTPROJECT_PATH)) {
        result = m_projectFile.absolutePath();
        found = true;
    } else if (name == QLatin1String(ProjectExplorer::Constants::VAR_CURRENTPROJECT_FILEPATH)) {
        result = m_projectFile.absoluteFilePath();
        found = true;
    } else if (m_kit && name == QLatin1String(ProjectExplorer::Constants::VAR_CURRENTKIT_NAME)) {
        result = m_kit->displayName();
        found = true;
    } else if (m_kit && name == QLatin1String(ProjectExplorer::Constants::VAR_CURRENTKIT_FILESYSTEMNAME)) {
        result = m_kit->fileSystemFriendlyName();
        found = true;
    } else if (m_kit && name == QLatin1String(ProjectExplorer::Constants::VAR_CURRENTKIT_ID)) {
        result = m_kit->id().toString();
        found = true;
    } else if (name == QLatin1String(ProjectExplorer::Constants::VAR_CURRENTBUILD_NAME)) {
        result = m_bcName;
        found = true;
    } else {
        result = Core::VariableManager::instance()->value(name.toUtf8(), &found);
    }
    if (ret)
        *ret = result;
    return found;
}
