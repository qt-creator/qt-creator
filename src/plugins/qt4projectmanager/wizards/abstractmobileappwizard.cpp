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
    wizardProgress()->item(pageId)->setTitle(tr("Qt Versions"));
    m_optionsPage = new MobileAppWizardOptionsPage;
    pageId = addPage(m_optionsPage);
    wizardProgress()->item(pageId)->setTitle(tr("Application Options"));
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
    app()->setMaemoPngIcon(wdlg->m_optionsPage->maemoPngIcon());
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
