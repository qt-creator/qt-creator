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

#ifndef CURRENTPROJECTFILTER_H
#define CURRENTPROJECTFILTER_H

#include <locator/basefilefilter.h>

#include <QtCore/QFutureInterface>

namespace ProjectExplorer {

class ProjectExplorerPlugin;
class Project;

namespace Internal {

class CurrentProjectFilter : public Locator::BaseFileFilter
{
    Q_OBJECT

public:
    CurrentProjectFilter(ProjectExplorerPlugin *pe);
    QString displayName() const { return tr("Files in Current Project"); }
    QString id() const { return "Files in current project"; }
    Locator::ILocatorFilter::Priority priority() const { return Locator::ILocatorFilter::Low; }
    void refresh(QFutureInterface<void> &future);

protected:
    void updateFiles();

private slots:
    void currentProjectChanged(ProjectExplorer::Project *project);
    void markFilesAsOutOfDate();

private:

    ProjectExplorerPlugin *m_projectExplorer;
    Project *m_project;
    bool m_filesUpToDate;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // CURRENTPROJECTFILTER_H
