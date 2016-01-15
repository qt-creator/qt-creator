/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmlprojectmanager.h"
#include "qmlprojectconstants.h"
#include "qmlproject.h"

#include <coreplugin/idocument.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <QDebug>

namespace QmlProjectManager {
namespace Internal {

Manager::Manager()
{
}

QString Manager::mimeType() const
{ return QLatin1String(Constants::QMLPROJECT_MIMETYPE); }

ProjectExplorer::Project *Manager::openProject(const QString &fileName, QString *errorString)
{
    if (!QFileInfo(fileName).isFile()) {
        if (errorString)
            *errorString = tr("Failed opening project \"%1\": Project is not a file.")
                .arg(fileName);
        return 0;
    }

    return new QmlProject(this, Utils::FileName::fromString(fileName));
}

void Manager::registerProject(QmlProject *project)
{ m_projects.append(project); }

void Manager::unregisterProject(QmlProject *project)
{ m_projects.removeAll(project); }

void Manager::notifyChanged(const QString &fileName)
{
    const Utils::FileName file = Utils::FileName::fromString(fileName);
    foreach (QmlProject *project, m_projects) {
        if (file == project->filesFileName())
            project->refresh(QmlProject::Files);
    }
}

} // namespace Internal
} // namespace QmlProjectManager
