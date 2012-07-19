/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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

#include "s60publishingwizardovi.h"
#include "s60publisherovi.h"
#include "s60publishingbuildsettingspageovi.h"
#include "s60publishingsissettingspageovi.h"
#include "s60publishingresultspageovi.h"

using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {

S60PublishingWizardOvi::S60PublishingWizardOvi(const Project *project, QWidget *parent) :
    Wizard(parent),
    m_publisher(new S60PublisherOvi(this))
{
    setWindowTitle(tr("Publishing to Nokia Store"));

    m_buildSettingsPage = new S60PublishingBuildSettingsPageOvi(m_publisher, project);
    m_buildSettingsPage->setTitle(tr("Build Configuration"));
    addPage(m_buildSettingsPage);

    m_sisSettingsPage = new S60PublishingSisSettingsPageOvi(m_publisher);
    m_sisSettingsPage->setTitle(tr("Project File Checks"));
    m_sisSettingsPage->setCommitPage(true);
    addPage(m_sisSettingsPage);

    m_resultsPage = new S60PublishingResultsPageOvi(m_publisher);
    m_resultsPage->setTitle(tr("Creating an Uploadable SIS File"));
    m_resultsPage->setFinalPage(true);
    addPage(m_resultsPage);

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

S60PublishingWizardOvi::~S60PublishingWizardOvi()
{
    delete m_publisher;
}

} // namespace Internal
} // namespace Qt4ProjectManager
