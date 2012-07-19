/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "allprojectsfilter.h"
#include "projectexplorer.h"
#include "session.h"
#include "project.h"

#include <QVariant>

using namespace Core;
using namespace Locator;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

AllProjectsFilter::AllProjectsFilter(ProjectExplorerPlugin *pe)
    : m_projectExplorer(pe), m_filesUpToDate(false)
{
    connect(m_projectExplorer, SIGNAL(fileListChanged()),
            this, SLOT(markFilesAsOutOfDate()));
    setShortcutString(QString(QLatin1Char('a')));
    setIncludedByDefault(true);
}

void AllProjectsFilter::markFilesAsOutOfDate()
{
    m_filesUpToDate = false;
}

void AllProjectsFilter::updateFiles()
{
    if (m_filesUpToDate)
        return;
    m_filesUpToDate = true;
    files().clear();
    foreach (Project *project, m_projectExplorer->session()->projects())
        files().append(project->files(Project::AllFiles));
    qSort(files());
    generateFileNames();
}

void AllProjectsFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    QMetaObject::invokeMethod(this, "markFilesAsOutOfDate", Qt::BlockingQueuedConnection);
}
