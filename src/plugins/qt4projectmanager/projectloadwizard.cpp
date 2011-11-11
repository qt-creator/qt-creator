/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "projectloadwizard.h"
#include "wizards/targetsetuppage.h"
#include "qt4project.h"

#include <coreplugin/ifile.h>

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

    setWindowTitle(tr("Project Setup"));

    m_targetSetupPage = new TargetSetupPage(this);
    m_targetSetupPage->setProFilePath(m_project->file()->fileName());
    m_targetSetupPage->setImportSearch(true);
    resize(900, 450);

    addPage(m_targetSetupPage);

    setOption(QWizard::NoCancelButton, false);
    setOption(QWizard::NoDefaultButton, false);
    setOption(QWizard::NoBackButtonOnLastPage, true);
#ifdef Q_OS_MAC
    setButtonLayout(QList<QWizard::WizardButton>()
                    << QWizard::CancelButton
                    << QWizard::Stretch
                    << QWizard::BackButton
                    << QWizard::NextButton
                    << QWizard::CommitButton
                    << QWizard::FinishButton);
#endif
}

ProjectLoadWizard::~ProjectLoadWizard()
{
}

void ProjectLoadWizard::done(int result)
{
    QWizard::done(result);

    if (result == Accepted)
        applySettings();
}

void ProjectLoadWizard::applySettings()
{
    m_targetSetupPage->setupProject(m_project);
}
