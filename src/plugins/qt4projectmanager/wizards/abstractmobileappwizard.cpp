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
    m_targetsPageId = addPageWithTitle(m_targetsPage, tr("Qt Versions"));
    m_genericOptionsPage = new MobileAppWizardGenericOptionsPage;
    m_genericOptionsPageId = addPageWithTitle(m_genericOptionsPage,
        tr("Mobile Options"));
    m_symbianOptionsPage = new MobileAppWizardSymbianOptionsPage;
    m_symbianOptionsPageId = addPageWithTitle(m_symbianOptionsPage,
        QLatin1String("    ") + tr("Symbian Specific"));
    m_maemoOptionsPage = new MobileAppWizardMaemoOptionsPage;
    m_maemoOptionsPageId = addPageWithTitle(m_maemoOptionsPage,
        QLatin1String("    ") + tr("Maemo Specific"));

    m_targetItem = wizardProgress()->item(m_targetsPageId);
    m_genericItem = wizardProgress()->item(m_genericOptionsPageId);
    m_symbianItem = wizardProgress()->item(m_symbianOptionsPageId);
    m_maemoItem = wizardProgress()->item(m_maemoOptionsPageId);

    m_targetItem->setNextShownItem(0);
    m_genericItem->setNextShownItem(0);
    m_symbianItem->setNextShownItem(0);
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
        m_targetsPage->isTargetSelected(QLatin1String(Constants::MAEMO5_DEVICE_TARGET_ID))
            || m_targetsPage->isTargetSelected(QLatin1String(Constants::HARMATTAN_DEVICE_TARGET_ID))
            || m_targetsPage->isTargetSelected(QLatin1String(Constants::MEEGO_DEVICE_TARGET_ID));

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

void AbstractMobileAppWizardDialog::initializePage(int id)
{
    if (id == startId()) {
        m_targetItem->setNextItems(QList<Utils::WizardProgressItem *>() << m_genericItem << itemOfNextGenericPage());
        m_genericItem->setNextItems(QList<Utils::WizardProgressItem *>() << m_symbianItem << m_maemoItem);
        m_symbianItem->setNextItems(QList<Utils::WizardProgressItem *>() << m_maemoItem << itemOfNextGenericPage());
    } else if (id == m_genericOptionsPageId) {
        const bool symbianTargetSelected =
            m_targetsPage->isTargetSelected(QLatin1String(Constants::S60_EMULATOR_TARGET_ID))
            || m_targetsPage->isTargetSelected(QLatin1String(Constants::S60_DEVICE_TARGET_ID));
        const bool maemoTargetSelected =
            m_targetsPage->isTargetSelected(QLatin1String(Constants::MAEMO5_DEVICE_TARGET_ID))
                || m_targetsPage->isTargetSelected(QLatin1String(Constants::HARMATTAN_DEVICE_TARGET_ID))
                || m_targetsPage->isTargetSelected(QLatin1String(Constants::MEEGO_DEVICE_TARGET_ID));

        QList<Utils::WizardProgressItem *> order;
        order << m_genericItem;
        if (symbianTargetSelected)
            order << m_symbianItem;
        if (maemoTargetSelected)
            order << m_maemoItem;
        order << itemOfNextGenericPage();

        for (int i = 0; i < order.count() - 1; i++)
            order.at(i)->setNextShownItem(order.at(i + 1));
    }
    BaseProjectWizardDialog::initializePage(id);
}

void AbstractMobileAppWizardDialog::cleanupPage(int id)
{
    if (id == m_genericOptionsPageId) {
        m_genericItem->setNextShownItem(0);
        m_symbianItem->setNextShownItem(0);
    }
    BaseProjectWizardDialog::cleanupPage(id);
}

int AbstractMobileAppWizardDialog::idOfNextGenericPage() const
{
    return pageIds().at(pageIds().indexOf(m_maemoOptionsPageId) + 1);
}

Utils::WizardProgressItem *AbstractMobileAppWizardDialog::itemOfNextGenericPage() const
{
    return wizardProgress()->item(idOfNextGenericPage());
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
    projectPathChanged(app()->path(AbstractMobileApp::AppPro));
}

} // namespace Internal
} // namespace Qt4ProjectManager
