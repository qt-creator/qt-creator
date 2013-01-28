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

#include "currentprojectfind.h"

#include "projectexplorer.h"
#include "project.h"
#include "session.h"

#include <coreplugin/idocument.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QSettings>
#include <QLabel>
#include <QHBoxLayout>

using namespace Find;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
using namespace TextEditor;

CurrentProjectFind::CurrentProjectFind(ProjectExplorerPlugin *plugin)
  : AllProjectsFind(plugin),
    m_plugin(plugin)
{
    connect(m_plugin, SIGNAL(currentProjectChanged(ProjectExplorer::Project*)),
            this, SLOT(handleProjectChanged()));
}

QString CurrentProjectFind::id() const
{
    return QLatin1String("Current Project");
}

QString CurrentProjectFind::displayName() const
{
    return tr("Current Project");
}

bool CurrentProjectFind::isEnabled() const
{
    return ProjectExplorerPlugin::currentProject() != 0 && BaseFileFind::isEnabled();
}

QVariant CurrentProjectFind::additionalParameters() const
{
    Project *project = ProjectExplorerPlugin::currentProject();
    if (project && project->document())
        return qVariantFromValue(project->document()->fileName());
    return QVariant();
}

Utils::FileIterator *CurrentProjectFind::files(const QStringList &nameFilters,
                           const QVariant &additionalParameters) const
{
    QTC_ASSERT(additionalParameters.isValid(), return new Utils::FileIterator());
    QList<Project *> allProjects = m_plugin->session()->projects();
    QString projectFile = additionalParameters.toString();
    foreach (Project *project, allProjects) {
        if (project->document() && projectFile == project->document()->fileName())
            return filesForProjects(nameFilters, QList<Project *>() << project);
    }
    return new Utils::FileIterator();
}

QString CurrentProjectFind::label() const
{
    QTC_ASSERT(ProjectExplorerPlugin::currentProject(), return QString());
    return tr("Project '%1':").arg(ProjectExplorerPlugin::currentProject()->displayName());
}

void CurrentProjectFind::handleProjectChanged()
{
    emit enabledChanged(isEnabled());
}

void CurrentProjectFind::writeSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("CurrentProjectFind"));
    writeCommonSettings(settings);
    settings->endGroup();
}

void CurrentProjectFind::readSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("CurrentProjectFind"));
    readCommonSettings(settings, QString(QLatin1Char('*')));
    settings->endGroup();
}
