// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtwizard.h"

#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>
#include <qmakeprojectmanager/qmakeprojectmanagertr.h>

#include <coreplugin/icore.h>

#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cpptoolsreuse.h>

#include <projectexplorer/environmentkitaspect.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/targetsetuppage.h>
#include <projectexplorer/task.h>

#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/algorithm.h>
#include <utils/mimeconstants.h>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

namespace QmakeProjectManager {
namespace Internal {

// -------------------- QtWizard
QtWizard::QtWizard()
{
    setSupportedProjectTypes({Constants::QMAKEPROJECT_ID});
}

QString QtWizard::sourceSuffix()
{
    return CppEditor::preferredCxxSourceSuffix(ProjectTree::currentProject());
}

QString QtWizard::headerSuffix()
{
    return CppEditor::preferredCxxHeaderSuffix(ProjectTree::currentProject());
}

QString QtWizard::formSuffix()
{
    return preferredSuffix(QLatin1String(Utils::Constants::FORM_MIMETYPE));
}

QString QtWizard::profileSuffix()
{
    return preferredSuffix(QLatin1String(Utils::Constants::PROFILE_MIMETYPE));
}

Result<> QtWizard::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l) const
{
    return QtWizard::qt4ProjectPostGenerateFiles(w, l);
}

Result<> QtWizard::qt4ProjectPostGenerateFiles(const QWizard *w,
                                               const Core::GeneratedFiles &generatedFiles)
{
    const auto *dialog = qobject_cast<const BaseQmakeProjectWizardDialog *>(w);

    // Generate user settings
    for (const Core::GeneratedFile &file : generatedFiles)
        if (file.attributes() & Core::GeneratedFile::OpenProjectAttribute) {
            dialog->writeUserFile(file.filePath());
            break;
        }

    // Post-Generate: Open the projects/editors
    return ProjectExplorer::CustomProjectWizard::postGenerateOpen(generatedFiles);
}

QString QtWizard::templateDir()
{
    return Core::ICore::resourcePath("templates/qt4project").toUrlishString();
}

bool QtWizard::lowerCaseFiles()
{
    QByteArray lowerCaseSettingsKey = CppEditor::Constants::CPPEDITOR_SETTINGSGROUP;
    lowerCaseSettingsKey += '/';
    lowerCaseSettingsKey += CppEditor::Constants::LOWERCASE_CPPFILES_KEY;
    const bool lowerCaseDefault = CppEditor::Constants::LOWERCASE_CPPFILES_DEFAULT;
    return Core::ICore::settings()->value(lowerCaseSettingsKey, QVariant(lowerCaseDefault)).toBool();
}

// ------------ CustomQmakeProjectWizard
CustomQmakeProjectWizard::CustomQmakeProjectWizard() = default;

Core::BaseFileWizard *CustomQmakeProjectWizard::create(const Core::WizardDialogParameters &parameters) const
{
    auto *wizard = new BaseQmakeProjectWizardDialog(this, parameters);

    if (!parameters.extraValues().contains(QLatin1String(ProjectExplorer::Constants::PROJECT_KIT_IDS)))
        wizard->addTargetSetupPage(targetPageId);

    initProjectWizardDialog(wizard, parameters.defaultPath(), wizard->extensionPages());
    return wizard;
}

Result<> CustomQmakeProjectWizard::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l) const
{
    return QtWizard::qt4ProjectPostGenerateFiles(w, l);
}

// ----------------- BaseQmakeProjectWizardDialog
BaseQmakeProjectWizardDialog::BaseQmakeProjectWizardDialog(
    const Core::BaseFileWizardFactory *factory,
    const Core::WizardDialogParameters &parameters)
    : ProjectExplorer::BaseProjectWizardDialog(factory, parameters)
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
    const Core::WizardDialogParameters &parameters)
    : ProjectExplorer::BaseProjectWizardDialog(factory, introPage, introId, parameters)
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
                ProjectExplorer::CompileTask(Task::Error, Tr::tr("Required Qt features not present."))};

        const Utils::Id platform = selectedPlatform();
        if (platform.isValid() && !QtKitAspect::platformPredicate(platform)(k))
            return {ProjectExplorer::CompileTask(
                ProjectExplorer::Task::Warning,
                Tr::tr("Qt version does not target the expected platform."))};
        QSet<Utils::Id> features = {QtSupport::Constants::FEATURE_DESKTOP};
        if (!QtKitAspect::qtVersionPredicate(features)(k))
            return {ProjectExplorer::CompileTask(ProjectExplorer::Task::Unknown,
                                                 Tr::tr("Qt version does not provide all features."))};
        return {};
    });

    resize(900, 450);
    if (id >= 0)
        setPage(id, m_targetSetupPage);
    else
        id = addPage(m_targetSetupPage);

    return id;
}

bool BaseQmakeProjectWizardDialog::writeUserFile(const Utils::FilePath &proFile) const
{
    if (!m_targetSetupPage)
        return false;

    QmakeProject *pro = new QmakeProject(proFile);
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

void BaseQmakeProjectWizardDialog::generateProfileName(const QString &name,
                                                       const Utils::FilePath &path)
{
    if (!m_targetSetupPage)
        return;

    const Utils::FilePath proFile = path / name / (name + ".pro");

    m_targetSetupPage->setProjectPath(proFile);
}

} // Internal
} // QmakeProjectManager
