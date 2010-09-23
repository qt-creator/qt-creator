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

#include "abstractmobileappwizard.h"

#include "abstractmobileapp.h"
#include "mobileappwizardpages.h"
#include "targetsetuppage.h"

#include <extensionsystem/pluginmanager.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4projectmanager.h>

#include <QtGui/QIcon>

namespace Qt4ProjectManager {
namespace Internal {

AbstractMobileAppWizardDialog::AbstractMobileAppWizardDialog(QWidget *parent)
    : ProjectExplorer::BaseProjectWizardDialog(parent)
{
    m_targetsPage = new TargetSetupPage;
    resize(900, 450);
    m_targetsPage->setImportDirectoryBrowsingEnabled(false);
    int pageId = addPage(m_targetsPage);
    wizardProgress()->item(pageId)->setTitle(tr("Qt versions"));
    m_optionsPage = new MobileAppWizardOptionsPage;
    pageId = addPage(m_optionsPage);
    wizardProgress()->item(pageId)->setTitle(tr("Application options"));
}


AbstractMobileAppWizard::AbstractMobileAppWizard(const Core::BaseFileWizardParameters &params,
    QObject *parent) : Core::BaseFileWizard(params, parent)
{
}

QWizard *AbstractMobileAppWizard::createWizardDialog(QWidget *parent,
    const QString &defaultPath, const WizardPageList &extensionPages) const
{
    AbstractMobileAppWizardDialog * const wdlg
        = createWizardDialogInternal(parent);
    wdlg->setPath(defaultPath);
    wdlg->setProjectName(ProjectExplorer::BaseProjectWizardDialog::uniqueProjectName(defaultPath));
    wdlg->m_optionsPage->setSymbianSvgIcon(app()->symbianSvgIcon());
    wdlg->m_optionsPage->setMaemoPngIcon(app()->maemoPngIcon());
    wdlg->m_optionsPage->setOrientation(app()->orientation());
    wdlg->m_optionsPage->setNetworkEnabled(app()->networkEnabled());
    connect(wdlg, SIGNAL(introPageLeft(QString, QString)),
        SLOT(useProjectPath(QString, QString)));
    foreach (QWizardPage *p, extensionPages)
        BaseFileWizard::applyExtensionPageShortTitle(wdlg, wdlg->addPage(p));
    return wdlg;
}

Core::GeneratedFiles AbstractMobileAppWizard::generateFiles(const QWizard *wizard,
    QString *errorMessage) const
{
    prepareGenerateFiles(wizard, errorMessage);
    const AbstractMobileAppWizardDialog *wdlg
        = qobject_cast<const AbstractMobileAppWizardDialog*>(wizard);
    app()->setSymbianTargetUid(wdlg->m_optionsPage->symbianUid());
    app()->setSymbianSvgIcon(wdlg->m_optionsPage->symbianSvgIcon());
    app()->setOrientation(wdlg->m_optionsPage->orientation());
    app()->setNetworkEnabled(wdlg->m_optionsPage->networkEnabled());
    return app()->generateFiles(errorMessage);
}

bool AbstractMobileAppWizard::postGenerateFiles(const QWizard *w,
    const Core::GeneratedFiles &l, QString *errorMessage)
{
    Q_UNUSED(w);
    Qt4Manager * const manager
        = ExtensionSystem::PluginManager::instance()->getObject<Qt4Manager>();
    Q_ASSERT(manager);
    Qt4Project project(manager, app()->path(AbstractMobileApp::AppPro));
    bool success = wizardDialog()->m_targetsPage->setupProject(&project);
    if (success) {
        project.saveSettings();
        success = postGenerateFilesInternal(l, errorMessage);
    }
    return success;
}

void AbstractMobileAppWizard::useProjectPath(const QString &projectName,
    const QString &projectPath)
{
    wizardDialog()->m_optionsPage->setSymbianUid(app()->symbianUidForPath(projectPath + projectName));
    app()->setProjectName(projectName);
    app()->setProjectPath(projectPath);
    wizardDialog()->m_targetsPage->setProFilePath(app()->path(AbstractMobileApp::AppPro));
}

} // end of namespace Internal
} // end of namespace Qt4ProjectManager
