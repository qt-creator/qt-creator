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

#include "qtwizard.h"

#include "qt4project.h"
#include "qt4projectmanager.h"
#include "qt4projectmanagerconstants.h"
#include "qt4target.h"
#include "modulespage.h"
#include "targetsetuppage.h"

#include <coreplugin/icore.h>
#include <cpptools/cpptoolsconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QVariant>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

static inline Core::BaseFileWizardParameters
    wizardParameters(const QString &id,
                     const QString &category,
                     const QString &categoryTranslationScope,
                     const QString &displayCategory,
                     const QString &name,
                     const QString &description,
                     const QIcon &icon)
{
    Core::BaseFileWizardParameters rc(Core::IWizard::ProjectWizard);
    rc.setCategory(category);
    rc.setDisplayCategory(QCoreApplication::translate(categoryTranslationScope.toLatin1(),
                                                      displayCategory.toLatin1()));
    rc.setIcon(icon);
    rc.setDisplayName(name);
    rc.setId(id);
    rc.setDescription(description);
    return rc;
}

// -------------------- QtWizard
QtWizard::QtWizard(const QString &id,
                   const QString &category,
                   const QString &categoryTranslationScope,
                   const QString &displayCategory,
                   const QString &name,
                   const QString &description, const QIcon &icon) :
    Core::BaseFileWizard(wizardParameters(id,
                                          category,
                                          categoryTranslationScope,
                                          displayCategory,
                                          name,
                                          description,
                                          icon))
{
}

QString QtWizard::sourceSuffix()
{
    return preferredSuffix(QLatin1String(Constants::CPP_SOURCE_MIMETYPE));
}

QString QtWizard::headerSuffix()
{
    return preferredSuffix(QLatin1String(Constants::CPP_HEADER_MIMETYPE));
}

QString QtWizard::formSuffix()
{
    return preferredSuffix(QLatin1String(Constants::FORM_MIMETYPE));
}

QString QtWizard::profileSuffix()
{
    return preferredSuffix(QLatin1String(Constants::PROFILE_MIMETYPE));
}

bool QtWizard::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage)
{
    return QtWizard::qt4ProjectPostGenerateFiles(w, l, errorMessage);
}

bool QtWizard::qt4ProjectPostGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage)
{
    const QString proFileName = l.back().path();
    const BaseQt4ProjectWizardDialog *dialog = qobject_cast<const BaseQt4ProjectWizardDialog *>(w);

    // Generate user settings:
    dialog->writeUserFile(proFileName);

    // Post-Generate: Open the project
    if (!ProjectExplorer::ProjectExplorerPlugin::instance()->openProject(proFileName)) {
        *errorMessage = tr("The project %1 could not be opened.").arg(proFileName);
        return false;
    }
    return true;
}

QString QtWizard::templateDir()
{
    QString rc = Core::ICore::instance()->resourcePath();
    rc += QLatin1String("/templates/qt4project");
    return rc;
}

bool QtWizard::lowerCaseFiles()
{
    QString lowerCaseSettingsKey = QLatin1String(CppTools::Constants::CPPTOOLS_SETTINGSGROUP);
    lowerCaseSettingsKey += QLatin1Char('/');
    lowerCaseSettingsKey += QLatin1String(CppTools::Constants::LOWERCASE_CPPFILES_KEY);
    const bool lowerCaseDefault = CppTools::Constants::lowerCaseFilesDefault;
    return Core::ICore::instance()->settings()->value(lowerCaseSettingsKey, QVariant(lowerCaseDefault)).toBool();
}

bool QtWizard::showModulesPageForApplications()
{
    return false;
}

bool QtWizard::showModulesPageForLibraries()
{
    return true;
}

// ------------ CustomQt4ProjectWizard
CustomQt4ProjectWizard::CustomQt4ProjectWizard(const Core::BaseFileWizardParameters& baseFileParameters,
                                               QObject *parent) :
    ProjectExplorer::CustomProjectWizard(baseFileParameters, parent)
{
}

QWizard *CustomQt4ProjectWizard::createWizardDialog(QWidget *parent,
                                                    const QString &defaultPath,
                                                    const WizardPageList &extensionPages) const
{
    BaseQt4ProjectWizardDialog *wizard =  new BaseQt4ProjectWizardDialog(false, parent);
    initProjectWizardDialog(wizard, defaultPath, extensionPages);
    if (wizard->pageIds().contains(targetPageId))
        qWarning("CustomQt4ProjectWizard: Unable to insert target page at %d", int(targetPageId));
    wizard->addTargetSetupPage(QSet<QString>(), targetPageId);
    return wizard;
}

bool CustomQt4ProjectWizard::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage)
{
    return QtWizard::qt4ProjectPostGenerateFiles(w, l, errorMessage);
}

void CustomQt4ProjectWizard::registerSelf()
{
    ProjectExplorer::CustomWizard::registerFactory<CustomQt4ProjectWizard>(QLatin1String("qt4project"));
}

// ----------------- BaseQt4ProjectWizardDialog
BaseQt4ProjectWizardDialog::BaseQt4ProjectWizardDialog(bool showModulesPage, QWidget *parent) :
    ProjectExplorer::BaseProjectWizardDialog(parent),
    m_modulesPage(0),
    m_targetSetupPage(0)
{
    init(showModulesPage);
}

BaseQt4ProjectWizardDialog::BaseQt4ProjectWizardDialog(bool showModulesPage,
                                                       Utils::ProjectIntroPage *introPage,
                                                       int introId, QWidget *parent) :
    ProjectExplorer::BaseProjectWizardDialog(introPage, introId, parent),
    m_modulesPage(0),
    m_targetSetupPage(0)
{
    init(showModulesPage);
}

BaseQt4ProjectWizardDialog::~BaseQt4ProjectWizardDialog()
{
    if (m_targetSetupPage && !m_targetSetupPage->parent())
        delete m_targetSetupPage;
    if (m_modulesPage && !m_modulesPage->parent())
        delete m_modulesPage;
}

void BaseQt4ProjectWizardDialog::init(bool showModulesPage)
{
    if (showModulesPage)
        m_modulesPage = new ModulesPage;
}

int BaseQt4ProjectWizardDialog::addModulesPage(int id)
{
    if (!m_modulesPage)
        return -1;
    if (id >= 0) {
        setPage(id, m_modulesPage);
        return id;
    }
    return addPage(m_modulesPage);
}

int BaseQt4ProjectWizardDialog::addTargetSetupPage(QSet<QString> targets, int id)
{
    m_targetSetupPage = new TargetSetupPage;
    QList<TargetSetupPage::ImportInfo> infos = TargetSetupPage::importInfosForKnownQtVersions(0);
    if (!targets.isEmpty())
        infos = TargetSetupPage::filterImportInfos(targets, infos);
    m_targetSetupPage->setImportDirectoryBrowsingEnabled(false);
    m_targetSetupPage->setShowLocationInformation(false);

    if (infos.count() <= 1)
        return -1;

    m_targetSetupPage->setImportInfos(infos);

    if (id >= 0)
        setPage(id, m_targetSetupPage);
    else
        id = addPage(m_targetSetupPage);

    return id;
}

QString BaseQt4ProjectWizardDialog::selectedModules() const
{
    return m_modulesPage ? m_modulesPage->selectedModules() : m_selectedModules;
}

void BaseQt4ProjectWizardDialog::setSelectedModules(const QString &modules, bool lock)
{
    if (m_modulesPage) {
        foreach(const QString &module, modules.split(QLatin1Char(' '))) {
            m_modulesPage->setModuleSelected(module, true);
            m_modulesPage->setModuleEnabled(module, !lock);
        }
    } else {
        m_selectedModules = modules;
    }
}

QString BaseQt4ProjectWizardDialog::deselectedModules() const
{
    return m_modulesPage ? m_modulesPage->deselectedModules() : m_deselectedModules;
}

void BaseQt4ProjectWizardDialog::setDeselectedModules(const QString &modules)
{
    if (m_modulesPage) {
        foreach(const QString &module, modules.split(QLatin1Char(' ')))
            m_modulesPage->setModuleSelected(module, false);
    } else {
        m_deselectedModules = modules;
    }
}

bool BaseQt4ProjectWizardDialog::writeUserFile(const QString &proFileName) const
{
    if (!m_targetSetupPage)
        return false;

    Qt4Manager *manager = ExtensionSystem::PluginManager::instance()->getObject<Qt4Manager>();
    Q_ASSERT(manager);

    Qt4Project *pro = new Qt4Project(manager, proFileName);
    bool success = m_targetSetupPage->setupProject(pro);
    if (success)
        pro->saveSettings();
    delete pro;
    return success;
}

bool BaseQt4ProjectWizardDialog::setupProject(Qt4Project *project) const
{
    return m_targetSetupPage->setupProject(project);
}

bool BaseQt4ProjectWizardDialog::isTargetSelected(const QString &targetid) const
{
    return m_targetSetupPage->isTargetSelected(targetid);
}

QSet<QString> BaseQt4ProjectWizardDialog::desktopTarget()
{
    QSet<QString> rc;
    rc.insert(QLatin1String(Constants::DESKTOP_TARGET_ID));
    return rc;
}
