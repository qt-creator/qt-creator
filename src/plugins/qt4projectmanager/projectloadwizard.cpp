/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "projectloadwizard.h"
#include "wizards/targetsetuppage.h"
#include "qt4project.h"

#include <QtGui/QCheckBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWizardPage>
#include <QtGui/QApplication>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

ProjectLoadWizard::ProjectLoadWizard(Qt4Project *project, QWidget *parent, Qt::WindowFlags flags)
    : QWizard(parent, flags), m_project(project), m_targetSetupPage(0)
{
    Q_ASSERT(project);

    setWindowTitle(tr("Project setup"));

    setupTargetPage();

    setOptions(options() | QWizard::NoBackButtonOnLastPage);
}

// We don't want to actually show the dialog if we don't show the import page
// We used to simply call ::exec() on the dialog
void ProjectLoadWizard::execDialog()
{
    if (!pageIds().isEmpty()) {
        QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
        exec();
        QApplication::restoreOverrideCursor();
    } else {
        done(QDialog::Accepted);
    }
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

void ProjectLoadWizard::setupTargetPage()
{
    if (m_targetSetupPage)
        return;

    QList<TargetSetupPage::ImportInfo> importVersions = TargetSetupPage::scanDefaultProjectDirectories(m_project);
    importVersions.append(TargetSetupPage::importInfosForKnownQtVersions(m_project->file()->fileName()));

    m_targetSetupPage = new TargetSetupPage(this);
    m_targetSetupPage->setProFilePath(m_project->file()->fileName());
    m_targetSetupPage->setImportInfos(importVersions);
    m_targetSetupPage->setImportDirectoryBrowsingEnabled(true);
    m_targetSetupPage->setImportDirectoryBrowsingLocation(m_project->projectDirectory());
    resize(900, 450);

    addPage(m_targetSetupPage);
}

void ProjectLoadWizard::applySettings()
{
    Q_ASSERT(m_targetSetupPage);
    m_targetSetupPage->setupProject(m_project);
}
