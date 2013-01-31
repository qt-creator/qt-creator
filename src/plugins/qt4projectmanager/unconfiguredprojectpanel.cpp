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

#include "unconfiguredprojectpanel.h"
#include "wizards/targetsetuppage.h"
#include "qt4projectmanagerconstants.h"

#include "qt4project.h"
#include "qt4projectmanager.h"

#include <coreplugin/idocument.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/coreconstants.h>
#include <qtsupport/qtkitinformation.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;


UnconfiguredProjectPanel::UnconfiguredProjectPanel()
{
}

QString Qt4ProjectManager::Internal::UnconfiguredProjectPanel::id() const
{
    return QLatin1String(Constants::UNCONFIGURED_PANEL_PAGE_ID);
}

QString Qt4ProjectManager::Internal::UnconfiguredProjectPanel::displayName() const
{
    return tr("Configure Project");
}

int UnconfiguredProjectPanel::priority() const
{
    return -10;
}

bool Qt4ProjectManager::Internal::UnconfiguredProjectPanel::supports(ProjectExplorer::Project *project)
{
    if (qobject_cast<Qt4Project *>(project) && project->targets().isEmpty())
        return true;
    return false;
}

ProjectExplorer::PropertiesPanel *Qt4ProjectManager::Internal::UnconfiguredProjectPanel::createPanel(ProjectExplorer::Project *project)
{
    ProjectExplorer::PropertiesPanel *panel = new ProjectExplorer::PropertiesPanel;
    panel->setDisplayName(displayName());
    panel->setIcon(QIcon(QLatin1String(":/projectexplorer/images/unconfigured.png")));

    TargetSetupPageWrapper *w = new TargetSetupPageWrapper(project);
    panel->setWidget(w);
    return panel;
}

/////////
/// TargetSetupPageWrapper
////////

TargetSetupPageWrapper::TargetSetupPageWrapper(ProjectExplorer::Project *project)
    : QWidget(), m_project(qobject_cast<Qt4Project *>(project))
{
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setMargin(0);
    setLayout(layout);

    m_targetSetupPage = new TargetSetupPage(this);
    m_targetSetupPage->setRequiredKitMatcher(new QtSupport::QtVersionKitMatcher);
    m_targetSetupPage->setUseScrollArea(false);
    m_targetSetupPage->setImportSearch(true);
    m_targetSetupPage->setProFilePath(project->document()->fileName());
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

    connect(m_configureButton, SIGNAL(clicked()),
            this, SLOT(done()));
    connect(m_cancelButton, SIGNAL(clicked()),
            this, SLOT(cancel()));
    connect(m_targetSetupPage, SIGNAL(completeChanged()),
            this, SLOT(completeChanged()));
    connect(ProjectExplorer::KitManager::instance(), SIGNAL(defaultkitChanged()),
            this, SLOT(updateNoteText()));
    connect(ProjectExplorer::KitManager::instance(), SIGNAL(kitUpdated(ProjectExplorer::Kit*)),
            this, SLOT(kitUpdated(ProjectExplorer::Kit*)));
}

void TargetSetupPageWrapper::kitUpdated(ProjectExplorer::Kit *k)
{
    if (k == ProjectExplorer::KitManager::instance()->defaultKit())
        updateNoteText();
}

void TargetSetupPageWrapper::updateNoteText()
{
    ProjectExplorer::Kit *k = ProjectExplorer::KitManager::instance()->defaultKit();

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
    ProjectExplorer::ProjectExplorerPlugin::instance()->unloadProject(m_project);
    if (ProjectExplorer::ProjectExplorerPlugin::instance()->session()->projects().isEmpty())
        Core::ICore::instance()->modeManager()->activateMode(Core::Constants::MODE_WELCOME);
}

void TargetSetupPageWrapper::done()
{
    m_targetSetupPage->setupProject(m_project);
    ProjectExplorer::ProjectExplorerPlugin::instance()->requestProjectModeUpdate(m_project);
    Core::ICore::instance()->modeManager()->activateMode(Core::Constants::MODE_EDIT);
}

void TargetSetupPageWrapper::completeChanged()
{
    m_configureButton->setEnabled(m_targetSetupPage->isComplete());
}
