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
#include <qt4projectmanager/qt4projectmanagerconstants.h>

#include <QtGui/QIcon>

namespace Qt4ProjectManager {
namespace Internal {

AbstractMobileAppWizardDialog::AbstractMobileAppWizardDialog(QWidget *parent)
    : ProjectExplorer::BaseProjectWizardDialog(parent)
{
    m_targetsPage = new TargetSetupPage;
    resize(900, 450);
    m_targetsPage->setImportDirectoryBrowsingEnabled(false);
    addPageWithTitle(m_targetsPage, tr("Qt Versions"));
    m_genericOptionsPage = new MobileAppWizardGenericOptionsPage;
    m_genericOptionsPageId = addPageWithTitle(m_genericOptionsPage,
        tr("Generic Mobile Application Options"));
    m_symbianOptionsPage = new MobileAppWizardSymbianOptionsPage;
    m_symbianOptionsPageId = addPageWithTitle(m_symbianOptionsPage,
        tr("Symbian-specific Options"));
    m_maemoOptionsPage = new MobileAppWizardMaemoOptionsPage;
    m_maemoOptionsPageId = addPageWithTitle(m_maemoOptionsPage,
        tr("Maemo-specific Options"));
}

int AbstractMobileAppWizardDialog::addPageWithTitle(QWizardPage *page, const QString &title)
{
    const int pageId = addPage(page);
    wizardProgress()->item(pageId)->setTitle(title);
    return pageId;
}

int AbstractMobileAppWizardDialog::nextId() const
{
    const bool symbianTargetSelected =
        m_targetsPage->isTargetSelected(QLatin1String(Constants::S60_EMULATOR_TARGET_ID))
        || m_targetsPage->isTargetSelected(QLatin1String(Constants::S60_DEVICE_TARGET_ID));
    const bool maemoTargetSelected =
        m_targetsPage->isTargetSelected(QLatin1String(Constants::MAEMO_DEVICE_TARGET_ID));

    if (currentPage() == m_targetsPage) {
        if (symbianTargetSelected || maemoTargetSelected)
            return m_genericOptionsPageId;
        else
            return idOfNextGenericPage();
    } else if (currentPage() == m_genericOptionsPage) {
        if (symbianTargetSelected)
            return m_symbianOptionsPageId;
        else
            return m_maemoOptionsPageId;
    } else if (currentPage() == m_symbianOptionsPage) {
        if (maemoTargetSelected)
            return m_maemoOptionsPageId;
        else
            return idOfNextGenericPage();
    } else {
        return BaseProjectWizardDialog::nextId();
    }
}

int AbstractMobileAppWizardDialog::idOfNextGenericPage() const
{
    return pageIds().at(pageIds().indexOf(m_maemoOptionsPageId) + 1);
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
    wdlg->m_genericOptionsPage->setOrientation(app()->orientation());
    wdlg->m_symbianOptionsPage->setSvgIcon(app()->symbianSvgIcon());
    wdlg->m_symbianOptionsPage->setNetworkEnabled(app()->networkEnabled());
    wdlg->m_maemoOptionsPage->setPngIcon(app()->maemoPngIcon());
    connect(wdlg, SIGNAL(projectParametersChanged(QString, QString)),
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
    app()->setOrientation(wdlg->m_genericOptionsPage->orientation());
    app()->setSymbianTargetUid(wdlg->m_symbianOptionsPage->symbianUid());
    app()->setSymbianSvgIcon(wdlg->m_symbianOptionsPage->svgIcon());
    app()->setNetworkEnabled(wdlg->m_symbianOptionsPage->networkEnabled());
    app()->setMaemoPngIcon(wdlg->m_maemoOptionsPage->pngIcon());
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
    wizardDialog()->m_symbianOptionsPage->setSymbianUid(app()->symbianUidForPath(projectPath + projectName));
    app()->setProjectName(projectName);
    app()->setProjectPath(projectPath);
    wizardDialog()->m_targetsPage->setProFilePath(app()->path(AbstractMobileApp::AppPro));
}

} // end of namespace Internal
} // end of namespace Qt4ProjectManager
