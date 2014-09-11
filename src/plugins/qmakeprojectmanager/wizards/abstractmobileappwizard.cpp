/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "abstractmobileappwizard.h"
#include "abstractmobileapp.h"

#include "../qmakeprojectimporter.h"

#include <extensionsystem/pluginmanager.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakeprojectmanager.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtkitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/targetsetuppage.h>
#include <projectexplorer/customwizard/customwizard.h>
#include <projectexplorer/session.h>
#include <coreplugin/editormanager/editormanager.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace QtSupport;

namespace QmakeProjectManager {

AbstractMobileAppWizardDialog::AbstractMobileAppWizardDialog(QWidget *parent,
                                                             const QtSupport::QtVersionNumber &minimumQtVersionNumber,
                                                             const QtSupport::QtVersionNumber &maximumQtVersionNumber,
                                                             const Core::WizardDialogParameters &parameters)
    : ProjectExplorer::BaseProjectWizardDialog(parent, parameters)
    , m_kitsPage(0)
    , m_minimumQtVersionNumber(minimumQtVersionNumber)
    , m_maximumQtVersionNumber(maximumQtVersionNumber)
{
    if (!parameters.extraValues().contains(QLatin1String(ProjectExplorer::Constants::PROJECT_KIT_IDS))) {
        m_kitsPage = new ProjectExplorer::TargetSetupPage;
        updateKitsPage();
        resize(900, 450);
    }
}

void AbstractMobileAppWizardDialog::addKitsPage()
{
    if (m_kitsPage)
        addPage(m_kitsPage);
}

void AbstractMobileAppWizardDialog::updateKitsPage()
{
    if (m_kitsPage) {
        QString platform = selectedPlatform();
        if (platform.isEmpty()) {
            m_kitsPage->setPreferredKitMatcher(
                QtKitInformation::qtVersionMatcher(FeatureSet(QtSupport::Constants::FEATURE_MOBILE)));
        } else {
            m_kitsPage->setPreferredKitMatcher(QtKitInformation::platformMatcher(platform));
        }
        m_kitsPage->setRequiredKitMatcher(QtKitInformation::qtVersionMatcher(requiredFeatures(),
                                                                           m_minimumQtVersionNumber,
                                                                           m_maximumQtVersionNumber));
    }
}

ProjectExplorer::TargetSetupPage *AbstractMobileAppWizardDialog::kitsPage() const
{
    return m_kitsPage;
}

Core::BaseFileWizard *AbstractMobileAppWizard::create(QWidget *parent, const Core::WizardDialogParameters &parameters) const
{
    AbstractMobileAppWizardDialog * const wdlg = createInternal(parent, parameters);
    wdlg->setProjectName(ProjectExplorer::BaseProjectWizardDialog::uniqueProjectName(parameters.defaultPath()));
    connect(wdlg, SIGNAL(projectParametersChanged(QString,QString)),
        SLOT(useProjectPath(QString,QString)));
    wdlg->addExtensionPages(parameters.extensionPages());

    return wdlg;
}

Core::GeneratedFiles AbstractMobileAppWizard::generateFiles(const QWizard *wizard,
    QString *errorMessage) const
{
    prepareGenerateFiles(wizard, errorMessage);
    return app()->generateFiles(errorMessage);
}

bool AbstractMobileAppWizard::postGenerateFiles(const QWizard *w,
    const Core::GeneratedFiles &l, QString *errorMessage)
{
    Q_UNUSED(w)
    Q_UNUSED(l)
    Q_UNUSED(errorMessage)
    QmakeManager * const manager
        = ExtensionSystem::PluginManager::getObject<QmakeManager>();
    Q_ASSERT(manager);
    QmakeProject project(manager, app()->path(AbstractMobileApp::AppPro));
    bool success = true;
    if (wizardDialog()->kitsPage()) {
        success = wizardDialog()->kitsPage()->setupProject(&project);
        if (success) {
            project.saveSettings();
            success = ProjectExplorer::CustomProjectWizard::postGenerateOpen(l, errorMessage);
        }
    }
    if (success) {
        const QString fileToOpen = fileToOpenPostGeneration();
        if (!fileToOpen.isEmpty()) {
            EditorManager::openEditor(fileToOpen);
            Project *project = SessionManager::projectForFile(fileToOpen);
            ProjectExplorerPlugin::setCurrentFile(project, fileToOpen);
        }
    }
    return success;
}

void AbstractMobileAppWizard::useProjectPath(const QString &projectName,
    const QString &projectPath)
{
    app()->setProjectName(projectName);
    app()->setProjectPath(projectPath);
    if (wizardDialog()->kitsPage())
        wizardDialog()->kitsPage()->setProjectPath(app()->path(AbstractMobileApp::AppPro));
    projectPathChanged(app()->path(AbstractMobileApp::AppPro));
}

} // namespace QmakeProjectManager
