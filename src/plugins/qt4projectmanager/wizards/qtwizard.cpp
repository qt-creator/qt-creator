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

#include "qtwizard.h"

#include "qt4project.h"
#include "qt4projectmanager.h"
#include "qt4projectmanagerconstants.h"
#include "modulespage.h"
#include "targetsetuppage.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <cpptools/cpptoolsconstants.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/task.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

#include <extensionsystem/pluginmanager.h>

#include <QCoreApplication>
#include <QVariant>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

static Core::BaseFileWizardParameters
    wizardParameters(const QString &id,
                     const QString &category,
                     const QString &displayCategory,
                     const QString &name,
                     const QString &description,
                     const QIcon &icon)
{
    Core::BaseFileWizardParameters rc(Core::IWizard::ProjectWizard);
    rc.setCategory(category);
    rc.setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
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
                   const QString &displayCategory,
                   const QString &name,
                   const QString &description, const QIcon &icon) :
    Core::BaseFileWizard(wizardParameters(id,
                                          category,
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

bool QtWizard::qt4ProjectPostGenerateFiles(const QWizard *w,
                                           const Core::GeneratedFiles &generatedFiles,
                                           QString *errorMessage)
{
    const BaseQt4ProjectWizardDialog *dialog = qobject_cast<const BaseQt4ProjectWizardDialog *>(w);

    // Generate user settings
    foreach (const Core::GeneratedFile &file, generatedFiles)
        if (file.attributes() & Core::GeneratedFile::OpenProjectAttribute) {
            dialog->writeUserFile(file.path());
            break;
        }

    // Post-Generate: Open the projects/editors
    return ProjectExplorer::CustomProjectWizard::postGenerateOpen(generatedFiles ,errorMessage);
}

QString QtWizard::templateDir()
{
    QString rc = Core::ICore::resourcePath();
    rc += QLatin1String("/templates/qt4project");
    return rc;
}

bool QtWizard::lowerCaseFiles()
{
    QString lowerCaseSettingsKey = QLatin1String(CppTools::Constants::CPPTOOLS_SETTINGSGROUP);
    lowerCaseSettingsKey += QLatin1Char('/');
    lowerCaseSettingsKey += QLatin1String(CppTools::Constants::LOWERCASE_CPPFILES_KEY);
    const bool lowerCaseDefault = CppTools::Constants::lowerCaseFilesDefault;
    return Core::ICore::settings()->value(lowerCaseSettingsKey, QVariant(lowerCaseDefault)).toBool();
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
                                                        const Core::WizardDialogParameters &wizardDialogParameters) const
{
    BaseQt4ProjectWizardDialog *wizard = new BaseQt4ProjectWizardDialog(false, parent, wizardDialogParameters);

    if (!wizardDialogParameters.extraValues().contains(QLatin1String(ProjectExplorer::Constants::PROJECT_KIT_IDS)))
        wizard->addTargetSetupPage(false, targetPageId);

    initProjectWizardDialog(wizard, wizardDialogParameters.defaultPath(),
                            wizardDialogParameters.extensionPages());
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
BaseQt4ProjectWizardDialog::BaseQt4ProjectWizardDialog(bool showModulesPage, QWidget *parent,
                                                       const Core::WizardDialogParameters &parameters) :
    ProjectExplorer::BaseProjectWizardDialog(parent, parameters),
    m_modulesPage(0),
    m_targetSetupPage(0),
    m_profileIds(parameters.extraValues().value(QLatin1String(ProjectExplorer::Constants::PROJECT_KIT_IDS))
                 .value<QList<Core::Id> >())
{
    init(showModulesPage);
}

BaseQt4ProjectWizardDialog::BaseQt4ProjectWizardDialog(bool showModulesPage,
                                                       Utils::ProjectIntroPage *introPage,
                                                       int introId, QWidget *parent,
                                                       const Core::WizardDialogParameters &parameters) :
    ProjectExplorer::BaseProjectWizardDialog(introPage, introId, parent, parameters),
    m_modulesPage(0),
    m_targetSetupPage(0),
    m_profileIds(parameters.extraValues().value(QLatin1String(ProjectExplorer::Constants::PROJECT_KIT_IDS))
                 .value<QList<Core::Id> >())
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
    connect(this, SIGNAL(projectParametersChanged(QString,QString)),
            this, SLOT(generateProfileName(QString,QString)));
}

int BaseQt4ProjectWizardDialog::addModulesPage(int id)
{
    if (!m_modulesPage)
        return -1;
    if (id >= 0) {
        setPage(id, m_modulesPage);
        wizardProgress()->item(id)->setTitle(tr("Modules"));
        return id;
    }
    const int newId = addPage(m_modulesPage);
    wizardProgress()->item(newId)->setTitle(tr("Modules"));
    return newId;
}

int BaseQt4ProjectWizardDialog::addTargetSetupPage(bool mobile, int id)
{
    m_targetSetupPage = new TargetSetupPage;
    const QString platform = selectedPlatform();
    Core::FeatureSet features = mobile ? Core::FeatureSet(QtSupport::Constants::FEATURE_MOBILE)
                                       : Core::FeatureSet(QtSupport::Constants::FEATURE_DESKTOP);
    if (platform.isEmpty())
        m_targetSetupPage->setPreferredKitMatcher(new QtSupport::QtVersionKitMatcher(features));
    else
        m_targetSetupPage->setPreferredKitMatcher(new QtSupport::QtPlatformKitMatcher(platform));

    m_targetSetupPage->setRequiredKitMatcher(new QtSupport::QtVersionKitMatcher(requiredFeatures()));

    resize(900, 450);
    if (id >= 0)
        setPage(id, m_targetSetupPage);
    else
        id = addPage(m_targetSetupPage);
    wizardProgress()->item(id)->setTitle(tr("Kits"));

    return id;
}

QStringList BaseQt4ProjectWizardDialog::selectedModulesList() const
{
    return m_modulesPage ? m_modulesPage->selectedModulesList() : m_selectedModules;
}

void BaseQt4ProjectWizardDialog::setSelectedModules(const QString &modules, bool lock)
{
    const QStringList modulesList = modules.split(QLatin1Char(' '));
    if (m_modulesPage) {
        foreach (const QString &module, modulesList) {
            m_modulesPage->setModuleSelected(module, true);
            m_modulesPage->setModuleEnabled(module, !lock);
        }
    } else {
        m_selectedModules = modulesList;
    }
}

QStringList BaseQt4ProjectWizardDialog::deselectedModulesList() const
{
    return m_modulesPage ? m_modulesPage->deselectedModulesList() : m_deselectedModules;
}

void BaseQt4ProjectWizardDialog::setDeselectedModules(const QString &modules)
{
    const QStringList modulesList = modules.split(QLatin1Char(' '));
    if (m_modulesPage) {
        foreach (const QString &module, modulesList)
            m_modulesPage->setModuleSelected(module, false);
    } else {
        m_deselectedModules = modulesList;
    }
}

bool BaseQt4ProjectWizardDialog::writeUserFile(const QString &proFileName) const
{
    if (!m_targetSetupPage)
        return false;

    Qt4Manager *manager = ExtensionSystem::PluginManager::getObject<Qt4Manager>();
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
    if (!m_targetSetupPage)
        return true;
    return m_targetSetupPage->setupProject(project);
}

bool BaseQt4ProjectWizardDialog::isQtPlatformSelected(const QString &platform) const
{
    QList<Core::Id> selectedKitList = selectedKits();

    QtSupport::QtPlatformKitMatcher matcher(platform);
    QList<ProjectExplorer::Kit *> kitList
            = ProjectExplorer::KitManager::instance()->kits(&matcher);
    foreach (ProjectExplorer::Kit *k, kitList) {
        if (selectedKitList.contains(k->id()))
            return true;
    }
    return false;
}

QList<Core::Id> BaseQt4ProjectWizardDialog::selectedKits() const
{
    if (!m_targetSetupPage)
        return m_profileIds;
    return m_targetSetupPage->selectedKits();
}

void BaseQt4ProjectWizardDialog::addExtensionPages(const QList<QWizardPage *> &wizardPageList)
{
    foreach (QWizardPage *p,wizardPageList)
        Core::BaseFileWizard::applyExtensionPageShortTitle(this, addPage(p));
}

void BaseQt4ProjectWizardDialog::generateProfileName(const QString &name, const QString &path)
{
    if (!m_targetSetupPage)
        return;

    const QString proFile =
        QDir::cleanPath(path + QLatin1Char('/') + name + QLatin1Char('/')
                        + name + QLatin1String(".pro"));

    m_targetSetupPage->setProFilePath(proFile);
}
