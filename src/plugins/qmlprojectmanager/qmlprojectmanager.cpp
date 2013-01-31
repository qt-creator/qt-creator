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

#include "qmlprojectmanager.h"
#include "qmlprojectconstants.h"
#include "qmlproject.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <projectexplorer/projectexplorerconstants.h>
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
    QFileInfo fileInfo(fileName);
    ProjectExplorer::ProjectExplorerPlugin *projectExplorer = ProjectExplorer::ProjectExplorerPlugin::instance();

    foreach (ProjectExplorer::Project *pi, projectExplorer->session()->projects()) {
        if (fileName == pi->document()->fileName()) {
            if (errorString)
                *errorString = tr("Failed opening project '%1': Project already open") .arg(QDir::toNativeSeparators(fileName));
            return 0;
        }
    }

    if (fileInfo.isFile())
        return new QmlProject(this, fileName);

    *errorString = tr("Failed opening project '%1': Project file is not a file").arg(QDir::toNativeSeparators(fileName));
    return 0;
}

void Manager::registerProject(QmlProject *project)
{ m_projects.append(project); }

void Manager::unregisterProject(QmlProject *project)
{ m_projects.removeAll(project); }

void Manager::notifyChanged(const QString &fileName)
{
    foreach (QmlProject *project, m_projects) {
        if (fileName == project->filesFileName())
            project->refresh(QmlProject::Files);
    }
}

} // namespace Internal
} // namespace QmlProjectManager
