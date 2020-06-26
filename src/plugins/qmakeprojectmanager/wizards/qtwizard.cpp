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

#include "qtwizard.h"

#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <coreplugin/icore.h>

#include <cpptools/cpptoolsconstants.h>

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/targetsetuppage.h>
#include <projectexplorer/task.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/algorithm.h>

#include <QCoreApplication>
#include <QVariant>

using namespace ProjectExplorer;
using namespace QtSupport;

namespace QmakeProjectManager {
namespace Internal {

// -------------------- QtWizard
QtWizard::QtWizard()
{
    setSupportedProjectTypes({Constants::QMAKEPROJECT_ID});
}

QString QtWizard::sourceSuffix()
{
    return preferredSuffix(QLatin1String(ProjectExplorer::Constants::CPP_SOURCE_MIMETYPE));
}

QString QtWizard::headerSuffix()
{
    return preferredSuffix(QLatin1String(ProjectExplorer::Constants::CPP_HEADER_MIMETYPE));
}

QString QtWizard::formSuffix()
{
    return preferredSuffix(QLatin1String(ProjectExplorer::Constants::FORM_MIMETYPE));
}

QString QtWizard::profileSuffix()
{
    return preferredSuffix(QLatin1String(Constants::PROFILE_MIMETYPE));
}

bool QtWizard::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage) const
{
    return QtWizard::qt4ProjectPostGenerateFiles(w, l, errorMessage);
}

bool QtWizard::qt4ProjectPostGenerateFiles(const QWizard *w,
                                           const Core::GeneratedFiles &generatedFiles,
                                           QString *errorMessage)
{
    const auto *dialog = qobject_cast<const BaseQmakeProjectWizardDialog *>(w);

    // Generate user settings
    for (const Core::GeneratedFile &file : generatedFiles)
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

// ------------ CustomQmakeProjectWizard
CustomQmakeProjectWizard::CustomQmakeProjectWizard() = default;

Core::BaseFileWizard *CustomQmakeProjectWizard::create(QWidget *parent,
                                          const Core::WizardDialogParameters &parameters) const
{
    auto *wizard = new BaseQmakeProjectWizardDialog(this, parent, parameters);

    if (!parameters.extraValues().contains(QLatin1String(ProjectExplorer::Constants::PROJECT_KIT_IDS)))
        wizard->addTargetSetupPage(targetPageId);

    initProjectWizardDialog(wizard, parameters.defaultPath(), wizard->extensionPages());
    return wizard;
}

bool CustomQmakeProjectWizard::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l,
                                                 QString *errorMessage) const
{
    return QtWizard::qt4ProjectPostGenerateFiles(w, l, errorMessage);
}

// ----------------- BaseQmakeProjectWizardDialog
BaseQmakeProjectWizardDialog::BaseQmakeProjectWizardDialog(
    const Core::BaseFileWizardFactory *factory,
    QWidget *parent,
    const Core::WizardDialogParameters &parameters)
    : ProjectExplorer::BaseProjectWizardDialog(factory, parent, parameters)
{
    m_profileIds = Utils::transform(parameters.extraValues()
                                        .value(ProjectExplorer::Constants::PROJECT_KIT_IDS)
                                        .toStringList(),
                                    &Utils::Id::fromString);

    connect(this, &BaseProjectWizardDialog::projectParametersChanged,
            this, &BaseQmakeProjectWizardDialog::generateProfileName);
}

BaseQmakeProjectWizardDialog::BaseQmakeProjectWizardDialog(
    const Core::BaseFileWizardFactory *factory,
    Utils::ProjectIntroPage *introPage,
    int introId,
    QWidget *parent,
    const Core::WizardDialogParameters &parameters)
    : ProjectExplorer::BaseProjectWizardDialog(factory, introPage, introId, parent, parameters)
{
    m_profileIds = Utils::transform(parameters.extraValues()
                                        .value(ProjectExplorer::Constants::PROJECT_KIT_IDS)
                                        .toStringList(),
                                    &Utils::Id::fromString);
    connect(this, &BaseProjectWizardDialog::projectParametersChanged,
            this, &BaseQmakeProjectWizardDialog::generateProfileName);
}

BaseQmakeProjectWizardDialog::~BaseQmakeProjectWizardDialog()
{
    if (m_targetSetupPage && !m_targetSetupPage->parent())
        delete m_targetSetupPage;
}

int BaseQmakeProjectWizardDialog::addTargetSetupPage(int id)
{
    m_targetSetupPage = new ProjectExplorer::TargetSetupPage;

    m_targetSetupPage->setTasksGenerator([this](const Kit *k) -> Tasks {
        if (!QtKitAspect::qtVersionPredicate(requiredFeatures())(k))
            return {
                ProjectExplorer::CompileTask(Task::Error, tr("Required Qt features not present."))};

        const Utils::Id platform = selectedPlatform();
        if (platform.isValid() && !QtKitAspect::platformPredicate(platform)(k))
            return {ProjectExplorer::CompileTask(
                ProjectExplorer::Task::Warning,
                tr("Qt version does not target the expected platform."))};
        QSet<Utils::Id> features = {QtSupport::Constants::FEATURE_DESKTOP};
        if (!QtKitAspect::qtVersionPredicate(features)(k))
            return {ProjectExplorer::CompileTask(ProjectExplorer::Task::Unknown,
                                                 tr("Qt version does not provide all features."))};
        return {};
    });

    resize(900, 450);
    if (id >= 0)
        setPage(id, m_targetSetupPage);
    else
        id = addPage(m_targetSetupPage);

    return id;
}

bool BaseQmakeProjectWizardDialog::writeUserFile(const QString &proFileName) const
{
    if (!m_targetSetupPage)
        return false;

    QmakeProject *pro = new QmakeProject(Utils::FilePath::fromString(proFileName));
    bool success = m_targetSetupPage->setupProject(pro);
    if (success)
        pro->saveSettings();
    delete pro;
    return success;
}

QList<Utils::Id> BaseQmakeProjectWizardDialog::selectedKits() const
{
    if (!m_targetSetupPage)
        return m_profileIds;
    return m_targetSetupPage->selectedKits();
}

void BaseQmakeProjectWizardDialog::generateProfileName(const QString &name, const QString &path)
{
    if (!m_targetSetupPage)
        return;

    const QString proFile = QDir::cleanPath(path + '/' + name + '/' + name + ".pro");

    m_targetSetupPage->setProjectPath(Utils::FilePath::fromString(proFile));
}

} // Internal
} // QmakeProjectManager
