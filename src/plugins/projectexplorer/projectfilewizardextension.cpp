// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectfilewizardextension.h"

#include "editorconfiguration.h"
#include "project.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "projectmanager.h"
#include "projectnodes.h"
#include "projecttree.h"
#include "projectwizardpage.h"

#include <coreplugin/icore.h>

#include <texteditor/icodestylepreferences.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/storagesettings.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textindenter.h>

#include <utils/algorithm.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QDebug>
#include <QMessageBox>
#include <QPointer>
#include <QTextCursor>
#include <QTextDocument>

using namespace TextEditor;
using namespace Core;
using namespace Utils;

/*!
    \class ProjectExplorer::Internal::ProjectFileWizardExtension

    \brief The ProjectFileWizardExtension class implements the post-file
    generating steps of a project wizard.

    This class provides the following functions:
    \list
    \li Add to a project file (*.pri/ *.pro)
    \li Initialize a version control system repository (unless the path is already
        managed) and do 'add' if the VCS supports it.
    \endlist

    \sa ProjectExplorer::Internal::ProjectWizardPage
*/

enum { debugExtension = 0 };

namespace ProjectExplorer {
namespace Internal {

// --------- ProjectWizardContext

class ProjectWizardContext
{
public:
    void clear();

    QPointer<ProjectWizardPage> page = nullptr; // this is managed by the wizard!
    const IWizardFactory *wizard = nullptr;
};

void ProjectWizardContext::clear()
{
    page = nullptr;
    wizard = nullptr;
}

// ---- ProjectFileWizardExtension

ProjectFileWizardExtension::~ProjectFileWizardExtension()
{
    delete m_context;
}

static FilePath generatedProjectFilePath(const QList<GeneratedFile> &files)
{
    for (const GeneratedFile &file : files)
        if (file.attributes() & GeneratedFile::OpenProjectAttribute)
            return file.filePath();
    return {};
}

void ProjectFileWizardExtension::firstExtensionPageShown(
        const QList<GeneratedFile> &files,
        const QVariantMap &extraValues)
{
    if (debugExtension)
        qDebug() << Q_FUNC_INFO << files.size();

    const FilePaths fileNames = Utils::transform(files, &GeneratedFile::filePath);
    m_context->page->setFiles(fileNames);

    FilePaths filePaths;
    ProjectAction projectAction;
    const IWizardFactory::WizardKind kind = m_context->wizard->kind();
    if (kind == IWizardFactory::ProjectWizard) {
        projectAction = AddSubProject;
        filePaths << generatedProjectFilePath(files);
    } else {
        projectAction = AddNewFile;
        filePaths = Utils::transform(files, &GeneratedFile::filePath);
    }

    // Static cast from void * to avoid qobject_cast (which needs a valid object) in value().
    auto contextNode = static_cast<Node *>(extraValues.value(QLatin1String(Constants::PREFERRED_PROJECT_NODE)).value<void *>());
    auto project = static_cast<Project *>(extraValues.value(Constants::PROJECT_POINTER).value<void *>());
    const FilePath path = FilePath::fromVariant(extraValues.value(Constants::PREFERRED_PROJECT_NODE_PATH));

    m_context->page->initializeProjectTree(findWizardContextNode(contextNode, project, path),
                                           filePaths, m_context->wizard->kind(),
                                           projectAction);
    // Refresh combobox on project tree changes:
    connect(ProjectTree::instance(), &ProjectTree::treeChanged,
            m_context->page, [this, project, path, filePaths, kind, projectAction]() {
        m_context->page->initializeProjectTree(
                    findWizardContextNode(m_context->page->currentNode(), project, path), filePaths,
                    kind, projectAction);
    });

    m_context->page->initializeVersionControls();
}

Node *ProjectFileWizardExtension::findWizardContextNode(Node *contextNode, Project *project,
                                                        const FilePath &path)
{
    if (contextNode && !ProjectTree::hasNode(contextNode)) {
        if (ProjectManager::projects().contains(project) && project->rootProjectNode()) {
            contextNode = project->rootProjectNode()->findNode([path](const Node *n) {
                return path == n->filePath();
            });
        }
    }
    return contextNode;
}

QList<QWizardPage *> ProjectFileWizardExtension::extensionPages(const IWizardFactory *wizard)
{
    if (!m_context)
        m_context = new ProjectWizardContext;
    else
        m_context->clear();
    // Init context with page and projects
    m_context->page = new ProjectWizardPage;
    m_context->wizard = wizard;
    return {m_context->page};
}

bool ProjectFileWizardExtension::processFiles(
        const QList<GeneratedFile> &files,
        bool *removeOpenProjectAttribute, QString *errorMessage)
{
    if (!processProject(files, removeOpenProjectAttribute, errorMessage))
        return false;
    if (!m_context->page->runVersionControl(files, errorMessage)) {
        QString message;
        if (errorMessage) {
            message = *errorMessage;
            message.append(QLatin1String("\n\n"));
            errorMessage->clear();
        }
        message.append(Tr::tr("Open project anyway?"));
        if (QMessageBox::question(ICore::dialogParent(), Tr::tr("Version Control Failure"), message,
                                  QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
            return false;
    }
    return true;
}

// Add files to project && version control
bool ProjectFileWizardExtension::processProject(
        const QList<GeneratedFile> &files,
        bool *removeOpenProjectAttribute, QString *errorMessage)
{
    *removeOpenProjectAttribute = false;

    const FilePath generatedProject = generatedProjectFilePath(files);

    FolderNode *folder = m_context->page->currentNode();
    if (!folder)
        return true;
    if (m_context->wizard->kind() == IWizardFactory::ProjectWizard) {
        if (!static_cast<ProjectNode *>(folder)->addSubProject(generatedProject)) {
            *errorMessage = Tr::tr("Failed to add subproject \"%1\"\nto project \"%2\".")
                            .arg(generatedProject.toUserOutput()).arg(folder->filePath().toUserOutput());
            return false;
        }
        *removeOpenProjectAttribute = true;
    } else {
        FilePaths filePaths = Utils::transform(files, &GeneratedFile::filePath);
        if (!folder->addFiles(filePaths)) {
            *errorMessage = Tr::tr("Failed to add one or more files to project\n\"%1\" (%2).")
                    .arg(folder->filePath().toUserOutput())
                    .arg(FilePath::formatFilePaths(filePaths, ","));
            return false;
        }
    }
    return true;
}

static ICodeStylePreferences *codeStylePreferences(Project *project, Id languageId)
{
    if (!languageId.isValid())
        return nullptr;

    if (project)
        return project->editorConfiguration()->codeStyle(languageId);

    return TextEditorSettings::codeStyle(languageId);
}

void ProjectFileWizardExtension::applyCodeStyle(GeneratedFile *file) const
{
    if (file->isBinary() || file->contents().isEmpty())
        return; // nothing to do

    Id languageId = TextEditorSettings::languageId(Utils::mimeTypeForFile(file->filePath()).name());

    if (!languageId.isValid())
        return; // don't modify files like *.ui *.pro

    FolderNode *folder = m_context->page->currentNode();
    Project *baseProject = ProjectTree::projectForNode(folder);

    ICodeStylePreferencesFactory *factory = TextEditorSettings::codeStyleFactory(languageId);

    QTextDocument doc(file->contents());
    Indenter *indenter = nullptr;
    if (factory) {
        indenter = factory->createIndenter(&doc);
        indenter->setFileName(file->filePath());
    }
    if (!indenter)
        indenter = new TextIndenter(&doc);

    ICodeStylePreferences *codeStylePrefs = codeStylePreferences(baseProject, languageId);
    indenter->setCodeStylePreferences(codeStylePrefs);

    QTextCursor cursor(&doc);
    cursor.select(QTextCursor::Document);
    indenter->indent(cursor,
                     QChar::Null,
                     codeStylePrefs->currentTabSettings());
    delete indenter;
    if (TextEditorSettings::storageSettings().m_cleanWhitespace) {
        QTextBlock block = doc.firstBlock();
        while (block.isValid()) {
            TabSettings::removeTrailingWhitespace(cursor, block);
            block = block.next();
        }
    }
    file->setContents(doc.toPlainText());
}

} // namespace Internal
} // namespace ProjectExplorer
