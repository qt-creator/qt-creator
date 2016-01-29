/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "unconfiguredprojectpanel.h"

#include "kit.h"
#include "kitmanager.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "session.h"
#include "targetsetuppage.h"

#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/coreconstants.h>

#include <QVBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>

namespace ProjectExplorer {
namespace Internal {

/////////
/// TargetSetupPageWrapper
////////

TargetSetupPageWrapper::TargetSetupPageWrapper(Project *project) :
    QWidget(), m_project(project)
{
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setMargin(0);
    setLayout(layout);

    m_targetSetupPage = new TargetSetupPage(this);
    m_targetSetupPage->setProjectImporter(project->createProjectImporter());
    m_targetSetupPage->setUseScrollArea(false);
    m_targetSetupPage->setProjectPath(project->projectFilePath().toString());
    m_targetSetupPage->setRequiredKitMatcher(project->requiredKitMatcher());
    m_targetSetupPage->setPreferredKitMatcher(project->preferredKitMatcher());
    m_targetSetupPage->initializePage();
    m_targetSetupPage->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    updateNoteText();

    layout->addWidget(m_targetSetupPage);

    // Apply row
    QHBoxLayout *hbox = new QHBoxLayout();
    layout->addLayout(hbox);
    layout->setMargin(0);
    hbox->addStretch();

    QDialogButtonBox *box = new QDialogButtonBox(this);

    m_configureButton = new QPushButton(this);
    m_configureButton->setText(tr("Configure Project"));
    box->addButton(m_configureButton, QDialogButtonBox::AcceptRole);

    m_cancelButton = new QPushButton(this);
    m_cancelButton->setText(tr("Cancel"));
    box->addButton(m_cancelButton, QDialogButtonBox::RejectRole);

    hbox->addWidget(box);

    layout->addStretch(10);

    completeChanged();

    connect(m_configureButton, &QAbstractButton::clicked,
            this, &TargetSetupPageWrapper::done);
    connect(m_cancelButton, &QAbstractButton::clicked,
            this, &TargetSetupPageWrapper::cancel);
    connect(m_targetSetupPage, &QWizardPage::completeChanged,
            this, &TargetSetupPageWrapper::completeChanged);
    connect(KitManager::instance(), &KitManager::defaultkitChanged,
            this, &TargetSetupPageWrapper::updateNoteText);
    connect(KitManager::instance(), &KitManager::kitUpdated,
            this, &TargetSetupPageWrapper::kitUpdated);
}

void TargetSetupPageWrapper::kitUpdated(Kit *k)
{
    if (k == KitManager::defaultKit())
        updateNoteText();
}

void TargetSetupPageWrapper::updateNoteText()
{
    Kit *k = KitManager::defaultKit();

    QString text;
    bool showHint = false;
    if (!k) {
        text = tr("The project <b>%1</b> is not yet configured.<br/>"
                  "Qt Creator cannot parse the project, because no kit "
                  "has been set up.")
                .arg(m_project->displayName());
        showHint = true;
    } else if (k->isValid()) {
        text = tr("The project <b>%1</b> is not yet configured.<br/>"
                  "Qt Creator uses the kit <b>%2</b> to parse the project.")
                .arg(m_project->displayName())
                .arg(k->displayName());
        showHint = false;
    } else {
        text = tr("The project <b>%1</b> is not yet configured.<br/>"
                  "Qt Creator uses the <b>invalid</b> kit <b>%2</b> to parse the project.")
                .arg(m_project->displayName())
                .arg(k->displayName());
        showHint = true;
    }

    m_targetSetupPage->setNoteText(text);
    m_targetSetupPage->showOptionsHint(showHint);
}

void TargetSetupPageWrapper::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        event->accept();
        done();
    }
}

void TargetSetupPageWrapper::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
        event->accept();
}

void TargetSetupPageWrapper::cancel()
{
    ProjectExplorerPlugin::instance()->unloadProject(m_project);
    if (!SessionManager::hasProjects())
        Core::ModeManager::activateMode(Core::Constants::MODE_WELCOME);
}

void TargetSetupPageWrapper::done()
{
    m_targetSetupPage->setupProject(m_project);
    ProjectExplorerPlugin::requestProjectModeUpdate(m_project);
    Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);
}

void TargetSetupPageWrapper::completeChanged()
{
    m_configureButton->setEnabled(m_targetSetupPage->isComplete());
}

} // namespace Internal
} // namespace ProjectExplorer
