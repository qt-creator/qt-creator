/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "projectfilewizardextension.h"
#include "projectexplorer.h"
#include "session.h"
#include "projectnodes.h"
#include "nodesvisitor.h"
#include "projectwizardpage.h"
#include "addnewmodel.h"

#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <coreplugin/basefilewizardfactory.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/mimedatabase.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/icodestylepreferences.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/normalindenter.h>
#include <texteditor/tabsettings.h>
#include <texteditor/storagesettings.h>
#include <projectexplorer/project.h>
#include <projectexplorer/editorconfiguration.h>

#include <QPointer>
#include <QDebug>
#include <QFileInfo>
#include <QMultiMap>
#include <QDir>
#include <QTextDocument>
#include <QTextCursor>
#include <QMessageBox>

using namespace TextEditor;
using namespace Core;

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

class BestNodeSelector
{
public:
    BestNodeSelector(const QString &commonDirectory, const QStringList &files);
    void inspect(AddNewTree *tree);
    AddNewTree *bestChoice() const;
    QString deployingProjects() const;
private:
    QString m_commonDirectory;
    QStringList m_files;
    bool m_deploys;
    QString m_deployText;
    AddNewTree *m_bestChoice;
    int m_bestMatchLength;
    int m_bestMatchPriority;
};

BestNodeSelector::BestNodeSelector(const QString &commonDirectory, const QStringList &files)
    : m_commonDirectory(commonDirectory),
      m_files(files),
      m_deploys(false),
      m_deployText(QCoreApplication::translate("ProjectWizard", "The files are implicitly added to the projects:") + QLatin1Char('\n')),
      m_bestChoice(0),
      m_bestMatchLength(-1),
      m_bestMatchPriority(-1)
{

}

// Find the project the new files should be added
// If any node deploys the files, then we don't want to add the files.
// Otherwise consider their common path. Either a direct match on the directory
// or the directory with the longest matching path (list containing"/project/subproject1"
// matching common path "/project/subproject1/newuserpath").
void BestNodeSelector::inspect(AddNewTree *tree)
{
    FolderNode *node = tree->node();
    if (node->nodeType() == ProjectNodeType) {
        if (static_cast<ProjectNode *>(node)->deploysFolder(m_commonDirectory)) {
            m_deploys = true;
            m_deployText += tree->displayName() + QLatin1Char('\n');
        }
    }
    if (m_deploys)
        return;
    const QString projectDirectory = ProjectExplorerPlugin::directoryFor(node);
    const int projectDirectorySize = projectDirectory.size();
    if (!m_commonDirectory.startsWith(projectDirectory))
        return;
    bool betterMatch = projectDirectorySize > m_bestMatchLength
            || (projectDirectorySize == m_bestMatchLength && tree->priority() > m_bestMatchPriority);
    if (betterMatch) {
        m_bestMatchPriority = tree->priority();
        m_bestMatchLength = projectDirectorySize;
        m_bestChoice = tree;
    }
}

AddNewTree *BestNodeSelector::bestChoice() const
{
    if (m_deploys)
        return 0;
    return m_bestChoice;
}

QString BestNodeSelector::deployingProjects() const
{
    if (m_deploys)
        return m_deployText;
    return QString();
}

static inline AddNewTree *createNoneNode(BestNodeSelector *selector)
{
    QString displayName = QCoreApplication::translate("ProjectWizard", "<Implicitly Add>");
    if (selector->bestChoice())
        displayName = QCoreApplication::translate("ProjectWizard", "<None>");
    return new AddNewTree(displayName);
}

static inline AddNewTree *buildAddProjectTree(ProjectNode *root, const QString &projectPath, Node *contextNode, BestNodeSelector *selector)
{
    QList<AddNewTree *> children;
    foreach (ProjectNode *pn, root->subProjectNodes()) {
        AddNewTree *child = buildAddProjectTree(pn, projectPath, contextNode, selector);
        if (child)
            children.append(child);
    }

    const QList<ProjectExplorer::ProjectAction> &list = root->supportedActions(root);
    if (list.contains(ProjectExplorer::AddSubProject) && !list.contains(ProjectExplorer::InheritedFromParent)) {
        if (projectPath.isEmpty() || root->canAddSubProject(projectPath)) {
            FolderNode::AddNewInformation info = root->addNewInformation(QStringList() << projectPath, contextNode);
            AddNewTree *item = new AddNewTree(root, children, info);
            selector->inspect(item);
            return item;
        }
    }

    if (children.isEmpty())
        return 0;
    return new AddNewTree(root, children, root->displayName());
}

static inline AddNewTree *buildAddProjectTree(SessionNode *root, const QString &projectPath, Node *contextNode, BestNodeSelector *selector)
{
    QList<AddNewTree *> children;
    foreach (ProjectNode *pn, root->projectNodes()) {
        AddNewTree *child = buildAddProjectTree(pn, projectPath, contextNode, selector);
        if (child)
            children.append(child);
    }
    children.prepend(createNoneNode(selector));
    return new AddNewTree(root, children, root->displayName());
}

static inline AddNewTree *buildAddFilesTree(FolderNode *root, const QStringList &files, Node *contextNode, BestNodeSelector *selector)
{
    QList<AddNewTree *> children;
    foreach (FolderNode *fn, root->subFolderNodes()) {
        AddNewTree *child = buildAddFilesTree(fn, files, contextNode, selector);
        if (child)
            children.append(child);
    }

    const QList<ProjectExplorer::ProjectAction> &list = root->supportedActions(root);
    if (list.contains(ProjectExplorer::AddNewFile) && !list.contains(ProjectExplorer::InheritedFromParent)) {
        FolderNode::AddNewInformation info = root->addNewInformation(files, contextNode);
        AddNewTree *item = new AddNewTree(root, children, info);
        selector->inspect(item);
        return item;
    }
    if (children.isEmpty())
        return 0;
    return new AddNewTree(root, children, root->displayName());
}

static inline AddNewTree *buildAddFilesTree(SessionNode *root, const QStringList &files, Node *contextNode, BestNodeSelector *selector)
{
    QList<AddNewTree *> children;
    foreach (ProjectNode *pn, root->projectNodes()) {
        AddNewTree *child = buildAddFilesTree(pn, files, contextNode, selector);
        if (child)
            children.append(child);
    }
    children.prepend(createNoneNode(selector));
    return new AddNewTree(root, children, root->displayName());
}

static inline AddNewTree *getChoices(const QStringList &generatedFiles,
                                     IWizardFactory::WizardKind wizardKind,
                                     Node *contextNode,
                                     BestNodeSelector *selector)
{
    if (wizardKind == IWizardFactory::ProjectWizard)
        return buildAddProjectTree(SessionManager::sessionNode(), generatedFiles.first(), contextNode, selector);
    else
        return buildAddFilesTree(SessionManager::sessionNode(), generatedFiles, contextNode, selector);
}

// --------- ProjectWizardContext
class ProjectWizardContext
{
public:
    ProjectWizardContext();
    void clear();

    QPointer<ProjectWizardPage> page; // this is managed by the wizard!
    const IWizardFactory *wizard;
};

ProjectWizardContext::ProjectWizardContext() :
    page(0),
    wizard(0)
{
}

void ProjectWizardContext::clear()
{
    page = 0;
    wizard = 0;
}

// ---- ProjectFileWizardExtension
ProjectFileWizardExtension::ProjectFileWizardExtension()
  : m_context(0)
{
}

ProjectFileWizardExtension::~ProjectFileWizardExtension()
{
    delete m_context;
}

static QString generatedProjectFilePath(const QList<GeneratedFile> &files)
{
    foreach (const GeneratedFile &file, files)
        if (file.attributes() & GeneratedFile::OpenProjectAttribute)
            return file.path();
    return QString();
}

void ProjectFileWizardExtension::firstExtensionPageShown(
        const QList<GeneratedFile> &files,
        const QVariantMap &extraValues)
{
    if (debugExtension)
        qDebug() << Q_FUNC_INFO << files.size();

    QStringList fileNames;
    foreach (const GeneratedFile &f, files)
        fileNames.push_back(f.path());
    QString commonDirectory = Utils::commonPath(fileNames);
    m_context->page->setFiles(fileNames);

    QStringList filePaths;
    ProjectExplorer::ProjectAction projectAction;
    if (m_context->wizard->kind()== IWizardFactory::ProjectWizard) {
        projectAction = ProjectExplorer::AddSubProject;
        filePaths << generatedProjectFilePath(files);
    } else {
        projectAction = ProjectExplorer::AddNewFile;
        foreach (const GeneratedFile &gf, files)
            filePaths << gf.path();
    }


    Node *contextNode = extraValues.value(QLatin1String(Constants::PREFERRED_PROJECT_NODE)).value<Node *>();
    BestNodeSelector selector(commonDirectory, filePaths);
    AddNewTree *tree = getChoices(filePaths, m_context->wizard->kind(), contextNode, &selector);

    m_context->page->setAdditionalInfo(selector.deployingProjects());

    AddNewModel *model = new AddNewModel(tree);
    m_context->page->setModel(model);
    m_context->page->setBestNode(selector.bestChoice());
    m_context->page->setAddingSubProject(projectAction == ProjectExplorer::AddSubProject);

    m_context->page->initializeVersionControls();
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
    return QList<QWizardPage *>() << m_context->page;
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
        message.append(tr("Open project anyway?"));
        if (QMessageBox::question(ICore::mainWindow(), tr("Version Control Failure"), message,
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

    QString generatedProject = generatedProjectFilePath(files);

    FolderNode *folder = m_context->page->currentNode();
    if (!folder)
        return true;
    if (m_context->wizard->kind() == IWizardFactory::ProjectWizard) {
        if (!static_cast<ProjectNode *>(folder)->addSubProjects(QStringList(generatedProject))) {
            *errorMessage = tr("Failed to add subproject \"%1\"\nto project \"%2\".")
                            .arg(generatedProject).arg(folder->path());
            return false;
        }
        *removeOpenProjectAttribute = true;
    } else {
        QStringList filePaths;
        foreach (const GeneratedFile &generatedFile, files)
            filePaths << generatedFile.path();
        if (!folder->addFiles(filePaths)) {
            *errorMessage = tr("Failed to add one or more files to project\n\"%1\" (%2).").
                    arg(folder->path(), filePaths.join(QString(QLatin1Char(','))));
            return false;
        }
    }
    return true;
}

static ICodeStylePreferences *codeStylePreferences(Project *project, Id languageId)
{
    if (!languageId.isValid())
        return 0;

    if (project)
        return project->editorConfiguration()->codeStyle(languageId);

    return TextEditorSettings::codeStyle(languageId);
}

void ProjectFileWizardExtension::applyCodeStyle(GeneratedFile *file) const
{
    if (file->isBinary() || file->contents().isEmpty())
        return; // nothing to do

    MimeType mt = MimeDatabase::findByFile(QFileInfo(file->path()));
    Id languageId = TextEditorSettings::languageId(mt.type());

    if (!languageId.isValid())
        return; // don't modify files like *.ui *.pro

    FolderNode *folder = m_context->page->currentNode();
    Project *baseProject = SessionManager::projectForNode(folder);

    ICodeStylePreferencesFactory *factory = TextEditorSettings::codeStyleFactory(languageId);

    Indenter *indenter = 0;
    if (factory)
        indenter = factory->createIndenter();
    if (!indenter)
        indenter = new NormalIndenter();

    ICodeStylePreferences *codeStylePrefs = codeStylePreferences(baseProject, languageId);
    indenter->setCodeStylePreferences(codeStylePrefs);
    QTextDocument doc(file->contents());
    QTextCursor cursor(&doc);
    cursor.select(QTextCursor::Document);
    indenter->indent(&doc, cursor, QChar::Null, codeStylePrefs->currentTabSettings());
    delete indenter;
    if (TextEditorSettings::storageSettings().m_cleanWhitespace) {
        QTextBlock block = doc.firstBlock();
        while (block.isValid()) {
            codeStylePrefs->currentTabSettings().removeTrailingWhitespace(cursor, block);
            block = block.next();
        }
    }
    file->setContents(doc.toPlainText());
}

} // namespace Internal
} // namespace ProjectExplorer
