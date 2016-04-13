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

#include "jsonsummarypage.h"

#include "jsonwizard.h"
#include "../project.h"
#include "../projectexplorerconstants.h"
#include "../projectnodes.h"
#include "../session.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/iversioncontrol.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QMessageBox>

using namespace Core;

static char KEY_SELECTED_PROJECT[] = "SelectedProject";
static char KEY_SELECTED_NODE[] = "SelectedFolderNode";
static char KEY_IS_SUBPROJECT[] = "IsSubproject";
static char KEY_VERSIONCONTROL[] = "VersionControl";

namespace ProjectExplorer {

// --------------------------------------------------------------------
// Helper:
// --------------------------------------------------------------------

static QString generatedProjectFilePath(const QList<JsonWizard::GeneratorFile> &files)
{
    foreach (const JsonWizard::GeneratorFile &file, files)
        if (file.file.attributes() & GeneratedFile::OpenProjectAttribute)
            return file.file.path();
    return QString();
}

static IWizardFactory::WizardKind wizardKind(JsonWizard *wiz)
{
    IWizardFactory::WizardKind kind = IWizardFactory::ProjectWizard;
    const QString kindStr = wiz->stringValue(QLatin1String("kind"));
    if (kindStr == QLatin1String(Core::Constants::WIZARD_KIND_PROJECT))
        kind = IWizardFactory::ProjectWizard;
    else if (kindStr == QLatin1String(Core::Constants::WIZARD_KIND_FILE))
        kind = IWizardFactory::FileWizard;
    else
        QTC_CHECK(false);
    return kind;
}

// --------------------------------------------------------------------
// JsonSummaryPage:
// --------------------------------------------------------------------

JsonSummaryPage::JsonSummaryPage(QWidget *parent) :
    Internal::ProjectWizardPage(parent),
    m_wizard(nullptr)
{
    connect(this, &Internal::ProjectWizardPage::projectNodeChanged,
            this, &JsonSummaryPage::summarySettingsHaveChanged);
    connect(this, &Internal::ProjectWizardPage::versionControlChanged,
            this, &JsonSummaryPage::summarySettingsHaveChanged);
}

void JsonSummaryPage::setHideProjectUiValue(const QVariant &hideProjectUiValue)
{
    m_hideProjectUiValue = hideProjectUiValue;
}

void JsonSummaryPage::initializePage()
{
    m_wizard = qobject_cast<JsonWizard *>(wizard());
    QTC_ASSERT(m_wizard, return);

    m_wizard->setValue(QLatin1String(KEY_SELECTED_PROJECT), QVariant());
    m_wizard->setValue(QLatin1String(KEY_SELECTED_NODE), QVariant());
    m_wizard->setValue(QLatin1String(KEY_IS_SUBPROJECT), false);
    m_wizard->setValue(QLatin1String(KEY_VERSIONCONTROL), QString());

    connect(m_wizard, &JsonWizard::filesReady, this, &JsonSummaryPage::triggerCommit);
    connect(m_wizard, &JsonWizard::filesReady, this, &JsonSummaryPage::addToProject);

    updateFileList();

    IWizardFactory::WizardKind kind = wizardKind(m_wizard);
    bool isProject = (kind == IWizardFactory::ProjectWizard);

    QStringList files;
    if (isProject) {
        JsonWizard::GeneratorFile f
                = Utils::findOrDefault(m_fileList, [](const JsonWizard::GeneratorFile &f) {
            return f.file.attributes() & GeneratedFile::OpenProjectAttribute;
        });
        files << f.file.path();
    } else {
        files = Utils::transform(m_fileList,
                                 [](const JsonWizard::GeneratorFile &f) {
                                    return f.file.path();
                                 });
    }

    Node *contextNode = m_wizard->value(QLatin1String(Constants::PREFERRED_PROJECT_NODE))
            .value<Node *>();
    initializeProjectTree(contextNode, files, kind,
                          isProject ? AddSubProject : AddNewFile);

    bool hideProjectUi = JsonWizard::boolFromVariant(m_hideProjectUiValue, m_wizard->expander());
    setProjectUiVisible(!hideProjectUi);

    initializeVersionControls();

    // Do a new try at initialization, now that we have real values set up:
    summarySettingsHaveChanged();
}

bool JsonSummaryPage::validatePage()
{
    m_wizard->commitToFileList(m_fileList);
    m_fileList.clear();
    return true;
}

void JsonSummaryPage::cleanupPage()
{
    disconnect(m_wizard, &JsonWizard::filesReady, this, nullptr);
}

void JsonSummaryPage::triggerCommit(const JsonWizard::GeneratorFiles &files)
{
    GeneratedFiles coreFiles
            = Utils::transform(files, [](const JsonWizard::GeneratorFile &f) -> GeneratedFile
                                      { return f.file; });

    QString errorMessage;
    if (!runVersionControl(coreFiles, &errorMessage)) {
        QMessageBox::critical(wizard(), tr("Failed to Commit to Version Control"),
                              tr("Error message from Version Control System: \"%1\".")
                              .arg(errorMessage));
    }
}

void JsonSummaryPage::addToProject(const JsonWizard::GeneratorFiles &files)
{
    QTC_CHECK(m_fileList.isEmpty()); // Happens after this page is done
    QString generatedProject = generatedProjectFilePath(files);
    IWizardFactory::WizardKind kind = wizardKind(m_wizard);

    FolderNode *folder = currentNode();
    if (!folder)
        return;
    if (kind == IWizardFactory::ProjectWizard) {
        if (!static_cast<ProjectNode *>(folder)->addSubProjects(QStringList(generatedProject))) {
            QMessageBox::critical(m_wizard, tr("Failed to Add to Project"),
                                  tr("Failed to add subproject \"%1\"\nto project \"%2\".")
                                  .arg(QDir::toNativeSeparators(generatedProject))
                                  .arg(folder->filePath().toUserOutput()));
            return;
        }
        m_wizard->removeAttributeFromAllFiles(GeneratedFile::OpenProjectAttribute);
    } else {
        QStringList filePaths = Utils::transform(files, [](const JsonWizard::GeneratorFile &f) {
            return f.file.path();
        });
        if (!folder->addFiles(filePaths)) {
            QStringList nativeFilePaths = Utils::transform(filePaths, &QDir::toNativeSeparators);
            QMessageBox::critical(wizard(), tr("Failed to Add to Project"),
                                  tr("Failed to add one or more files to project\n\"%1\" (%2).")
                                  .arg(folder->filePath().toUserOutput(),
                                       nativeFilePaths.join(QLatin1String(", "))));
            return;
        }
    }
    return;
}

void JsonSummaryPage::summarySettingsHaveChanged()
{
    IVersionControl *vc = currentVersionControl();
    m_wizard->setValue(QLatin1String(KEY_VERSIONCONTROL), vc ? vc->id().toString() : QString());

    updateProjectData(currentNode());
}

void JsonSummaryPage::updateFileList()
{
    m_fileList = m_wizard->generateFileList();
    QStringList filePaths
            = Utils::transform(m_fileList, [](const JsonWizard::GeneratorFile &f) { return f.file.path(); });
    setFiles(filePaths);
}

void JsonSummaryPage::updateProjectData(FolderNode *node)
{
    Project *project = SessionManager::projectForNode(node);

    m_wizard->setValue(QLatin1String(KEY_SELECTED_PROJECT), QVariant::fromValue(project));
    m_wizard->setValue(QLatin1String(KEY_SELECTED_NODE), QVariant::fromValue(node));
    m_wizard->setValue(QLatin1String(KEY_IS_SUBPROJECT), node ? true : false);

    updateFileList();
}

} // namespace ProjectExplorer
