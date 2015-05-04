/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "currentprojectfind.h"

#include "project.h"
#include "projecttree.h"
#include "session.h"

#include <utils/qtcassert.h>
#include <utils/filesearch.h>

#include <QDebug>
#include <QSettings>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
using namespace TextEditor;

CurrentProjectFind::CurrentProjectFind()
{
    connect(ProjectTree::instance(), &ProjectTree::currentProjectChanged,
            this, &CurrentProjectFind::handleProjectChanged);
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
    return ProjectTree::currentProject() != 0 && BaseFileFind::isEnabled();
}

QVariant CurrentProjectFind::additionalParameters() const
{
    Project *project = ProjectTree::currentProject();
    if (project && project->document())
        return qVariantFromValue(project->projectFilePath().toString());
    return QVariant();
}

Utils::FileIterator *CurrentProjectFind::files(const QStringList &nameFilters,
                           const QVariant &additionalParameters) const
{
    QTC_ASSERT(additionalParameters.isValid(),
               return new Utils::FileListIterator(QStringList(), QList<QTextCodec *>()));
    QString projectFile = additionalParameters.toString();
    foreach (Project *project, SessionManager::projects()) {
        if (project->document() && projectFile == project->projectFilePath().toString())
            return filesForProjects(nameFilters, QList<Project *>() << project);
    }
    return new Utils::FileListIterator(QStringList(), QList<QTextCodec *>());
}

QString CurrentProjectFind::label() const
{
    Project *p = ProjectTree::currentProject();
    QTC_ASSERT(p, return QString());
    return tr("Project \"%1\":").arg(p->displayName());
}

void CurrentProjectFind::handleProjectChanged()
{
    emit enabledChanged(isEnabled());
}

void CurrentProjectFind::recheckEnabled()
{
    Core::SearchResult *search = qobject_cast<Core::SearchResult *>(sender());
    if (!search)
        return;
    QString projectFile = getAdditionalParameters(search).toString();
    foreach (Project *project, SessionManager::projects()) {
        if (projectFile == project->projectFilePath().toString()) {
            search->setSearchAgainEnabled(true);
            return;
        }
    }
    search->setSearchAgainEnabled(false);
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
