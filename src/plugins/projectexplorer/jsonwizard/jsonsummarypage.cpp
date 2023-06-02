// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jsonsummarypage.h"

#include "jsonwizard.h"
#include "../project.h"
#include "../projectexplorerconstants.h"
#include "../projectexplorertr.h"
#include "../projectnodes.h"
#include "../projectmanager.h"
#include "../projecttree.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/iversioncontrol.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QDir>
#include <QMessageBox>

using namespace Core;
using namespace Utils;

static char KEY_SELECTED_PROJECT[] = "SelectedProject";
static char KEY_SELECTED_NODE[] = "SelectedFolderNode";
static char KEY_IS_SUBPROJECT[] = "IsSubproject";
static char KEY_VERSIONCONTROL[] = "VersionControl";
static char KEY_QT_KEYWORDS_ENABLED[] = "QtKeywordsEnabled";

namespace ProjectExplorer {

// --------------------------------------------------------------------
// Helper:
// --------------------------------------------------------------------

static FilePath generatedProjectFilePath(const QList<JsonWizard::GeneratorFile> &files)
{
    for (const JsonWizard::GeneratorFile &file : files)
        if (file.file.attributes() & GeneratedFile::OpenProjectAttribute)
            return file.file.filePath();
    return {};
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
    m_wizard->setValue(QLatin1String(KEY_QT_KEYWORDS_ENABLED), false);

    connect(m_wizard, &JsonWizard::filesReady, this, &JsonSummaryPage::triggerCommit);
    connect(m_wizard, &JsonWizard::filesReady, this, &JsonSummaryPage::addToProject);

    updateFileList();

    IWizardFactory::WizardKind kind = wizardKind(m_wizard);
    bool isProject = (kind == IWizardFactory::ProjectWizard);

    FilePaths files;
    if (isProject) {
        JsonWizard::GeneratorFile f
                = Utils::findOrDefault(m_fileList, [](const JsonWizard::GeneratorFile &f) {
            return f.file.attributes() & GeneratedFile::OpenProjectAttribute;
        });
        files << f.file.filePath();
    } else {
        files = Utils::transform(m_fileList,
                                 [](const JsonWizard::GeneratorFile &f) {
                                    return f.file.filePath();
                                 });
    }

    // Use static cast from void * to avoid qobject_cast (which needs a valid object) in value()
    // in the following code:
    auto contextNode = findWizardContextNode(static_cast<Node *>(m_wizard->value(Constants::PREFERRED_PROJECT_NODE).value<void *>()));
    const ProjectAction currentAction = isProject ? AddSubProject : AddNewFile;

    initializeProjectTree(contextNode, files, kind, currentAction);

    // Refresh combobox on project tree changes:
    connect(ProjectTree::instance(), &ProjectTree::treeChanged,
            this, [this, files, kind, currentAction]() {
        initializeProjectTree(findWizardContextNode(currentNode()), files, kind, currentAction);
    });


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
            = Utils::transform(files, &JsonWizard::GeneratorFile::file);

    QString errorMessage;
    if (!runVersionControl(coreFiles, &errorMessage)) {
        QMessageBox::critical(wizard(), Tr::tr("Failed to Commit to Version Control"),
                              Tr::tr("Error message from Version Control System: \"%1\".")
                              .arg(errorMessage));
    }
}

void JsonSummaryPage::addToProject(const JsonWizard::GeneratorFiles &files)
{
    QTC_CHECK(m_fileList.isEmpty()); // Happens after this page is done
    const FilePath generatedProject = generatedProjectFilePath(files);
    IWizardFactory::WizardKind kind = wizardKind(m_wizard);

    FolderNode *folder = currentNode();
    if (!folder)
        return;
    if (kind == IWizardFactory::ProjectWizard) {
        if (!static_cast<ProjectNode *>(folder)->addSubProject(generatedProject)) {
            QMessageBox::critical(m_wizard, Tr::tr("Failed to Add to Project"),
                                  Tr::tr("Failed to add subproject \"%1\"\nto project \"%2\".")
                                  .arg(generatedProject.toUserOutput())
                                  .arg(folder->filePath().toUserOutput()));
            return;
        }
        m_wizard->removeAttributeFromAllFiles(GeneratedFile::OpenProjectAttribute);
    } else {
        FilePaths filePaths = Utils::transform(files, [](const JsonWizard::GeneratorFile &f) {
            return f.file.filePath();
        });
        if (!folder->addFiles(filePaths)) {
            QMessageBox::critical(wizard(), Tr::tr("Failed to Add to Project"),
                                  Tr::tr("Failed to add one or more files to project\n\"%1\" (%2).")
                                  .arg(folder->filePath().toUserOutput(),
                                       FilePath::formatFilePaths(filePaths, ", ")));
            return;
        }
        const QStringList dependencies = m_wizard->stringValue("Dependencies")
                .split(':', Qt::SkipEmptyParts);
        if (!dependencies.isEmpty())
            folder->addDependencies(dependencies);
    }
    return;
}

void JsonSummaryPage::summarySettingsHaveChanged()
{
    IVersionControl *vc = currentVersionControl();
    m_wizard->setValue(QLatin1String(KEY_VERSIONCONTROL), vc ? vc->id().toString() : QString());

    updateProjectData(currentNode());
}

Node *JsonSummaryPage::findWizardContextNode(Node *contextNode) const
{
    if (contextNode && !ProjectTree::hasNode(contextNode)) {
        contextNode = nullptr;

        // Static cast from void * to avoid qobject_cast (which needs a valid object) in value().
        auto project = static_cast<Project *>(m_wizard->value(Constants::PROJECT_POINTER).value<void *>());
        if (ProjectManager::projects().contains(project) && project->rootProjectNode()) {
            const FilePath path = FilePath::fromVariant(m_wizard->value(Constants::PREFERRED_PROJECT_NODE_PATH));
            contextNode = project->rootProjectNode()->findNode([path](const Node *n) {
                return path == n->filePath();
            });
        }
    }
    return contextNode;
}

void JsonSummaryPage::updateFileList()
{
    m_fileList = m_wizard->generateFileList();
    const FilePaths filePaths =
            Utils::transform(m_fileList,
                             [](const JsonWizard::GeneratorFile &f) { return f.file.filePath(); });
    setFiles(filePaths);
}

void JsonSummaryPage::updateProjectData(FolderNode *node)
{
    Project *project = ProjectTree::projectForNode(node);

    m_wizard->setValue(QLatin1String(KEY_SELECTED_PROJECT), QVariant::fromValue(project));
    m_wizard->setValue(QLatin1String(KEY_SELECTED_NODE), QVariant::fromValue(node));
    m_wizard->setValue(QLatin1String(KEY_IS_SUBPROJECT), node ? true : false);
    bool qtKeyWordsEnabled = true;
    if (ProjectTree::hasNode(node)) {
        const ProjectNode *projectNode = node->asProjectNode();
        if (!projectNode)
            projectNode = node->parentProjectNode();
        while (projectNode) {
            const QVariant keywordsEnabled = projectNode->data(Constants::QT_KEYWORDS_ENABLED);
            if (keywordsEnabled.isValid()) {
                qtKeyWordsEnabled = keywordsEnabled.toBool();
                break;
            }
            if (projectNode->isProduct())
                break;
            projectNode = projectNode->parentProjectNode();
        }
    }
    m_wizard->setValue(QLatin1String(KEY_QT_KEYWORDS_ENABLED), qtKeyWordsEnabled);

    updateFileList();
}

} // namespace ProjectExplorer
