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

#include "genericprojectmanager.h"
#include "genericprojectconstants.h"
#include "genericproject.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>

#include <QDebug>

namespace GenericProjectManager {
namespace Internal {

Manager::Manager()
{ }

QString Manager::mimeType() const
{
    return QLatin1String(Constants::GENERICMIMETYPE);
}

ProjectExplorer::Project *Manager::openProject(const QString &fileName, QString *errorString)
{
    if (!QFileInfo(fileName).isFile())
        return 0;

    ProjectExplorer::ProjectExplorerPlugin *projectExplorer = ProjectExplorer::ProjectExplorerPlugin::instance();
    foreach (ProjectExplorer::Project *pi, projectExplorer->session()->projects()) {
        if (fileName == pi->document()->fileName()) {
            if (errorString)
                *errorString = tr("Failed opening project '%1': Project already open")
                    .arg(QDir::toNativeSeparators(fileName));
            return 0;
        }
    }

    GenericProject *project = new GenericProject(this, fileName);
    return project;
}

void Manager::registerProject(GenericProject *project)
{
    m_projects.append(project);
}

void Manager::unregisterProject(GenericProject *project)
{
    m_projects.removeAll(project);
}

} // namespace Internal
} // namespace GenericProjectManager
