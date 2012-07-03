/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "abstractmobileappwizard.h"

#include "abstractmobileapp.h"
#include "mobileappwizardpages.h"
#include "targetsetuppage.h"

#include <extensionsystem/pluginmanager.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4projectmanager.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtprofileinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/customwizard/customwizard.h>
#include <coreplugin/editormanager/editormanager.h>
#include <utils/qtcassert.h>

#include <QIcon>

namespace Qt4ProjectManager {

AbstractMobileAppWizardDialog::AbstractMobileAppWizardDialog(QWidget *parent,
                                                             const QtSupport::QtVersionNumber &minimumQtVersionNumber,
                                                             const QtSupport::QtVersionNumber &maximumQtVersionNumber,
                                                             const Core::WizardDialogParameters &parameters)
    : ProjectExplorer::BaseProjectWizardDialog(parent, parameters)
    , m_targetsPage(0)
    , m_genericOptionsPageId(-1)
    , m_symbianOptionsPageId(-1)
    , m_maemoOptionsPageId(-1)
    , m_harmattanOptionsPageId(-1)
    , m_targetsPageId(-1)
    , m_ignoreGeneralOptions(false)
    , m_targetItem(0)
    , m_genericItem(0)
    , m_symbianItem(0)
    , m_maemoItem(0)
    , m_harmattanItem(0)
    , m_profileIds(parameters.extraValues().value(ProjectExplorer::Constants::PROJECT_PROFILE_IDS).value<QList<Core::Id> >())
{
    if (!parameters.extraValues().contains(ProjectExplorer::Constants::PROJECT_PROFILE_IDS)) {
        m_targetsPage = new TargetSetupPage;
        m_targetsPage->setPreferredProfileMatcher(new QtSupport::QtPlatformProfileMatcher(selectedPlatform()));
        m_targetsPage->setRequiredProfileMatcher(new QtSupport::QtVersionProfileMatcher(requiredFeatures(),
                                                                                        minimumQtVersionNumber,
                                                                                        maximumQtVersionNumber));
        resize(900, 450);
    }

    m_genericOptionsPage = new Internal::MobileAppWizardGenericOptionsPage;
    m_symbianOptionsPage = new Internal::MobileAppWizardSymbianOptionsPage;
    m_maemoOptionsPage = new Internal::MobileAppWizardMaemoOptionsPage;
    m_harmattanOptionsPage = new Internal::MobileAppWizardHarmattanOptionsPage;
}

void AbstractMobileAppWizardDialog::addMobilePages()
{
    if (m_targetsPage) {
        m_targetsPageId = addPageWithTitle(m_targetsPage, tr("Targets"));
        m_targetItem = wizardProgress()->item(m_targetsPageId);
    }

    const bool shouldAddGenericPage = m_targetsPage
            || (isQtPlatformSelected(QtSupport::Constants::SYMBIAN_PLATFORM) && !m_ignoreGeneralOptions)
            || isQtPlatformSelected(QtSupport::Constants::MAEMO_FREMANTLE_PLATFORM);
    const bool shouldAddSymbianPage = m_targetsPage
            || isQtPlatformSelected(QtSupport::Constants::SYMBIAN_PLATFORM);
    const bool shouldAddMaemoPage = m_targetsPage
            || isQtPlatformSelected(QtSupport::Constants::MAEMO_FREMANTLE_PLATFORM)
            || isQtPlatformSelected(QtSupport::Constants::MEEGO_PLATFORM);
    const bool shouldAddHarmattanPage = m_targetsPage
            || isQtPlatformSelected(QtSupport::Constants::MEEGO_HARMATTAN_PLATFORM);

    if (shouldAddGenericPage) {
        m_genericOptionsPageId = addPageWithTitle(m_genericOptionsPage,
                                                  tr("Mobile Options"));
        m_genericItem = wizardProgress()->item(m_genericOptionsPageId);
    }

    if (shouldAddSymbianPage) {
        m_symbianOptionsPageId = addPageWithTitle(m_symbianOptionsPage,
                                                  QLatin1String("    ") + tr("Symbian Specific"));
        m_symbianItem = wizardProgress()->item(m_symbianOptionsPageId);
    }

    if (shouldAddMaemoPage) {
        m_maemoOptionsPageId = addPageWithTitle(m_maemoOptionsPage,
                                                QLatin1String("    ") + tr("Maemo5 And MeeGo Specific"));
        m_maemoItem = wizardProgress()->item(m_maemoOptionsPageId);
    }

    if (shouldAddHarmattanPage) {
        m_harmattanOptionsPageId = addPageWithTitle(m_harmattanOptionsPage,
                                                    QLatin1String("    ") + tr("Harmattan Specific"));
        m_harmattanItem = wizardProgress()->item(m_harmattanOptionsPageId);
    }

    if (m_targetItem)
        m_targetItem->setNextShownItem(0);
}

TargetSetupPage *AbstractMobileAppWizardDialog::targetsPage() const
{
    return m_targetsPage;
}

int AbstractMobileAppWizardDialog::addPageWithTitle(QWizardPage *page, const QString &title)
{
    const int pageId = addPage(page);
    wizardProgress()->item(pageId)->setTitle(title);
    return pageId;
}

int AbstractMobileAppWizardDialog::nextId() const
{
    if (m_targetsPage) {
        if (currentPage() == m_targetsPage) {
            if ((isQtPlatformSelected(QtSupport::Constants::SYMBIAN_PLATFORM) && !m_ignoreGeneralOptions) ||
                    isQtPlatformSelected(QtSupport::Constants::MAEMO_FREMANTLE_PLATFORM))
                return m_genericOptionsPageId;
            // If Symbian target and Qt Quick components for Symbian, skip the mobile options page.
            else if (isQtPlatformSelected(QtSupport::Constants::SYMBIAN_PLATFORM) && m_ignoreGeneralOptions)
                return m_symbianOptionsPageId;
            else if (isQtPlatformSelected(QtSupport::Constants::MEEGO_PLATFORM))
                return m_maemoOptionsPageId;
            else if (isQtPlatformSelected(QtSupport::Constants::MEEGO_HARMATTAN_PLATFORM))
                return m_harmattanOptionsPageId;
            else
                return idOfNextGenericPage();
        } else if (currentPage() == m_genericOptionsPage) {
            if (isQtPlatformSelected(QtSupport::Constants::SYMBIAN_PLATFORM))
                return m_symbianOptionsPageId;
            else if (isQtPlatformSelected(QtSupport::Constants::MAEMO_FREMANTLE_PLATFORM)
                     || isQtPlatformSelected(QtSupport::Constants::MEEGO_PLATFORM))
                return m_maemoOptionsPageId;
            else if (isQtPlatformSelected(QtSupport::Constants::MEEGO_HARMATTAN_PLATFORM))
                return m_harmattanOptionsPageId;
            else
                return idOfNextGenericPage();
        } else if (currentPage() == m_symbianOptionsPage) {
            if (isQtPlatformSelected(QtSupport::Constants::MAEMO_FREMANTLE_PLATFORM)
                    || isQtPlatformSelected(QtSupport::Constants::MEEGO_PLATFORM))
                return m_maemoOptionsPageId;
            else if (isQtPlatformSelected(QtSupport::Constants::MEEGO_HARMATTAN_PLATFORM))
                return m_harmattanOptionsPageId;
            else
                return idOfNextGenericPage();
        } else if (currentPage() == m_maemoOptionsPage) {
            if (isQtPlatformSelected(QtSupport::Constants::MEEGO_HARMATTAN_PLATFORM))
                return m_harmattanOptionsPageId;
            else
                return idOfNextGenericPage();
        }
    }
    return BaseProjectWizardDialog::nextId();
}

void AbstractMobileAppWizardDialog::initializePage(int id)
{
    if (m_targetItem) {
        if (id == startId()) {
            m_targetItem->setNextItems(QList<Utils::WizardProgressItem *>()
                    << m_genericItem << m_symbianItem << m_maemoItem << m_harmattanItem << itemOfNextGenericPage());
            m_genericItem->setNextItems(QList<Utils::WizardProgressItem *>()
                    << m_symbianItem << m_maemoItem);
            m_symbianItem->setNextItems(QList<Utils::WizardProgressItem *>()
                    << m_maemoItem << m_harmattanItem << itemOfNextGenericPage());
            m_maemoItem->setNextItems(QList<Utils::WizardProgressItem *>()
                    << m_harmattanItem << itemOfNextGenericPage());
        } else if (id == m_genericOptionsPageId
                   || id == m_symbianOptionsPageId
                   || id == m_maemoOptionsPageId) {
            QList<Utils::WizardProgressItem *> order;
            order << m_genericItem;
            if (isQtPlatformSelected(QtSupport::Constants::SYMBIAN_PLATFORM))
                order << m_symbianItem;
            if (isQtPlatformSelected(QtSupport::Constants::MAEMO_FREMANTLE_PLATFORM)
                   || isQtPlatformSelected(QtSupport::Constants::MEEGO_PLATFORM))
                order << m_maemoItem;
            if (isQtPlatformSelected(QtSupport::Constants::MEEGO_HARMATTAN_PLATFORM))
                order << m_harmattanItem;
            order << itemOfNextGenericPage();

            for (int i = 0; i < order.count() - 1; i++)
                order.at(i)->setNextShownItem(order.at(i + 1));
        }
    }
    BaseProjectWizardDialog::initializePage(id);
}

void AbstractMobileAppWizardDialog::setIgnoreGenericOptionsPage(bool ignore)
{
    m_ignoreGeneralOptions = ignore;
}

Utils::WizardProgressItem *AbstractMobileAppWizardDialog::targetsPageItem() const
{
    return m_targetItem;
}

int AbstractMobileAppWizardDialog::idOfNextGenericPage() const
{
    return pageIds().at(pageIds().indexOf(m_harmattanOptionsPageId) + 1);
}

Utils::WizardProgressItem *AbstractMobileAppWizardDialog::itemOfNextGenericPage() const
{
    return wizardProgress()->item(idOfNextGenericPage());
}

bool AbstractMobileAppWizardDialog::isQtPlatformSelected(const QString &platform) const
{
    QList<Core::Id> selectedProfileList = selectedProfiles();

    QtSupport::QtPlatformProfileMatcher matcher(platform);
    QList<ProjectExplorer::Profile *> allProfileList
            = ProjectExplorer::ProfileManager::instance()->profiles(&matcher);
    foreach (ProjectExplorer::Profile *p, allProfileList) {
        if (selectedProfileList.contains(p->id()))
            return true;
    }
    return false;
}

QList<Core::Id> AbstractMobileAppWizardDialog::selectedProfiles() const
{
    if (m_targetsPage)
        return m_targetsPage->selectedProfiles();
    return m_profileIds;
}



AbstractMobileAppWizard::AbstractMobileAppWizard(const Core::BaseFileWizardParameters &params,
    QObject *parent) : Core::BaseFileWizard(params, parent)
{ }

QWizard *AbstractMobileAppWizard::createWizardDialog(QWidget *parent,
                                                     const Core::WizardDialogParameters &wizardDialogParameters) const
{
    AbstractMobileAppWizardDialog * const wdlg
        = createWizardDialogInternal(parent, wizardDialogParameters);
    wdlg->setProjectName(ProjectExplorer::BaseProjectWizardDialog::uniqueProjectName(wizardDialogParameters.defaultPath()));
    wdlg->m_genericOptionsPage->setOrientation(app()->orientation());
    wdlg->m_symbianOptionsPage->setSvgIcon(app()->symbianSvgIcon());
    wdlg->m_symbianOptionsPage->setNetworkEnabled(app()->networkEnabled());
    wdlg->m_maemoOptionsPage->setPngIcon(app()->pngIcon64());
    wdlg->m_harmattanOptionsPage->setPngIcon(app()->pngIcon80());
    wdlg->m_harmattanOptionsPage->setBoosterOptionEnabled(app()->canSupportMeegoBooster());
    connect(wdlg, SIGNAL(projectParametersChanged(QString,QString)),
        SLOT(useProjectPath(QString,QString)));
    wdlg->addExtensionPages(wizardDialogParameters.extensionPages());

    return wdlg;
}

Core::GeneratedFiles AbstractMobileAppWizard::generateFiles(const QWizard *wizard,
    QString *errorMessage) const
{
    const AbstractMobileAppWizardDialog *wdlg
        = qobject_cast<const AbstractMobileAppWizardDialog*>(wizard);
    app()->setOrientation(wdlg->m_genericOptionsPage->orientation());
    app()->setSymbianTargetUid(wdlg->m_symbianOptionsPage->symbianUid());
    app()->setSymbianSvgIcon(wdlg->m_symbianOptionsPage->svgIcon());
    app()->setNetworkEnabled(wdlg->m_symbianOptionsPage->networkEnabled());
    app()->setPngIcon64(wdlg->m_maemoOptionsPage->pngIcon());
    app()->setPngIcon80(wdlg->m_harmattanOptionsPage->pngIcon());
    if (wdlg->isQtPlatformSelected(QtSupport::Constants::MEEGO_HARMATTAN_PLATFORM))
        app()->setSupportsMeegoBooster(wdlg->m_harmattanOptionsPage->supportsBooster());
    prepareGenerateFiles(wizard, errorMessage);
    return app()->generateFiles(errorMessage);
}

bool AbstractMobileAppWizard::postGenerateFiles(const QWizard *w,
    const Core::GeneratedFiles &l, QString *errorMessage)
{
    Q_UNUSED(w)
    Q_UNUSED(l)
    Q_UNUSED(errorMessage)
    Qt4Manager * const manager
        = ExtensionSystem::PluginManager::getObject<Qt4Manager>();
    Q_ASSERT(manager);
    Qt4Project project(manager, app()->path(AbstractMobileApp::AppPro));
    bool success = true;
    if (wizardDialog()->m_targetsPage) {
        success = wizardDialog()->m_targetsPage->setupProject(&project);
        if (success) {
            project.saveSettings();
            success = ProjectExplorer::CustomProjectWizard::postGenerateOpen(l, errorMessage);
        }
    }
    if (success) {
        const QString fileToOpen = fileToOpenPostGeneration();
        if (!fileToOpen.isEmpty()) {
            Core::EditorManager::openEditor(fileToOpen, Core::Id(), Core::EditorManager::ModeSwitch);
            ProjectExplorer::ProjectExplorerPlugin::instance()->setCurrentFile(0, fileToOpen);
        }
    }
    return success;
}

void AbstractMobileAppWizard::useProjectPath(const QString &projectName,
    const QString &projectPath)
{
    wizardDialog()->m_symbianOptionsPage->setSymbianUid(app()->symbianUidForPath(projectPath + projectName));
    app()->setProjectName(projectName);
    app()->setProjectPath(projectPath);
    if (wizardDialog()->m_targetsPage)
        wizardDialog()->m_targetsPage->setProFilePath(app()->path(AbstractMobileApp::AppPro));
    projectPathChanged(app()->path(AbstractMobileApp::AppPro));
}

} // namespace Qt4ProjectManager
