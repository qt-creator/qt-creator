/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "projectloadwizard.h"

#include "qt4project.h"
#include "qmakestep.h"
#include "qt4target.h"
#include "makestep.h"
#include "qt4buildconfiguration.h"
#include "qt4projectmanagerconstants.h"

#include "wizards/targetsetuppage.h"

#include <QtGui/QCheckBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWizardPage>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

ProjectLoadWizard::ProjectLoadWizard(Qt4Project *project, QWidget *parent, Qt::WindowFlags flags)
    : QWizard(parent, flags), m_project(project), m_targetSetupPage(0)
{
    Q_ASSERT(project);

    setWindowTitle(tr("Project setup"));

    m_importVersions = TargetSetupPage::recursivelyCheckDirectoryForBuild(project->projectDirectory());
    m_importVersions.append(TargetSetupPage::recursivelyCheckDirectoryForBuild(project->defaultTopLevelBuildDirectory()));

    m_importVersions.append(TargetSetupPage::importInfosForKnownQtVersions(project));

    if (m_importVersions.count() > 1)
        setupTargetPage();

    setOptions(options() | QWizard::NoCancelButton | QWizard::NoBackButtonOnLastPage);
}

// We don't want to actually show the dialog if we don't show the import page
// We used to simply call ::exec() on the dialog
void ProjectLoadWizard::execDialog()
{
    if (!pageIds().isEmpty())
        exec();
    else
        done(QDialog::Accepted);
}

ProjectLoadWizard::~ProjectLoadWizard()
{
}

void ProjectLoadWizard::done(int result)
{
    QWizard::done(result);
    // This normally happens on showing the final page, but since we
    // don't show it anymore, do it here

    if (result == Accepted)
        applySettings();
}

// This function used to do the commented stuff instead of having only one page
int ProjectLoadWizard::nextId() const
{
    return -1;
}

void ProjectLoadWizard::setupTargetPage()
{
    if (m_targetSetupPage)
        return;

    m_targetSetupPage = new TargetSetupPage(this);
    m_targetSetupPage->setImportInfos(m_importVersions);
    m_targetSetupPage->setImportDirectoryBrowsingEnabled(true);
    m_targetSetupPage->setImportDirectoryBrowsingLocation(m_project->projectDirectory());

    addPage(m_targetSetupPage);
}

void ProjectLoadWizard::applySettings()
{
    Q_ASSERT(m_targetSetupPage);
    m_targetSetupPage->setupProject(m_project);
}

