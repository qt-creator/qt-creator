/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#include "gitplugin.h"
#include "gitclient.h"
#include "giteditor.h"
#include "gitconstants.h"
#include "changeselectiondialog.h"
#include "gitsubmiteditor.h"
#include "commitdata.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanagerinterface.h>
#include <coreplugin/editormanager/editormanager.h>
#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/basevcssubmiteditorfactory.h>

#include <QtCore/qplugin.h>
#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryFile>
#include <QtCore/QDir>
#include <QtGui/QAction>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QMainWindow>
#include <QtGui/QFileDialog>

static const VCSBase::VCSBaseEditorParameters editorParameters[] = {
{
    VCSBase::RegularCommandOutput,
    Git::Constants::GIT_COMMAND_LOG_EDITOR_KIND,
    Core::Constants::C_GLOBAL,
    "application/vnd.nokia.text.scs_git_commandlog",
    "gitlog"},
{   VCSBase::LogOutput,
    Git::Constants::GIT_LOG_EDITOR_KIND,
    Core::Constants::C_GLOBAL,
    "application/vnd.nokia.text.scs_git_filelog",
    "gitfilelog"},
{   VCSBase::AnnotateOutput,
    Git::Constants::GIT_BLAME_EDITOR_KIND,
    Core::Constants::C_GLOBAL,
    "application/vnd.nokia.text.scs_git_annotation",
    "gitsannotate"},
{   VCSBase::DiffOutput,
    Git::Constants::GIT_DIFF_EDITOR_KIND,
    Core::Constants::C_GLOBAL,
    "text/x-patch","diff"}
};

// Utility to find a parameter set by type
static inline const VCSBase::VCSBaseEditorParameters *findType(int ie)
{
    const VCSBase::EditorContentType et = static_cast<VCSBase::EditorContentType>(ie);
    return  VCSBase::VCSBaseEditor::findType(editorParameters, sizeof(editorParameters)/sizeof(VCSBase::VCSBaseEditorParameters), et);
}

using namespace Git;
using namespace Git::Internal;

// CoreListener

bool CoreListener::editorAboutToClose(Core::IEditor *editor)
{
    return m_plugin->editorAboutToClose(editor);
}

// GitPlugin

GitPlugin *GitPlugin::m_instance = 0;

GitPlugin::GitPlugin() :
    m_core(0),
    m_diffAction(0),
    m_diffProjectAction(0),
    m_statusAction(0),
    m_statusProjectAction(0),
    m_logAction(0),
    m_blameAction(0),
    m_logProjectAction(0),
    m_undoFileAction(0),
    m_undoProjectAction(0),
    m_showAction(0),
    m_addAction(0),
    m_commitAction(0),
    m_pullAction(0),
    m_pushAction(0),
    m_submitCurrentAction(0),
    m_diffSelectedFilesAction(0),
    m_undoAction(0),
    m_redoAction(0),
    m_projectExplorer(0),
    m_gitClient(0),
    m_outputWindow(0),
    m_changeSelectionDialog(0),
    m_settingsPage(0),
    m_coreListener(0),
    m_submitEditorFactory(0),
    m_changeTmpFile(0)
{
    Q_ASSERT(m_instance == 0);
    m_instance = this;
}

GitPlugin::~GitPlugin()
{
    if (m_outputWindow) {
        removeObject(m_outputWindow);
        delete m_outputWindow;
        m_outputWindow = 0;
    }

    if (m_settingsPage) {
        removeObject(m_settingsPage);
        delete m_settingsPage;
        m_settingsPage = 0;
    }

    if (!m_editorFactories.empty()) {
        foreach(Core::IEditorFactory* pf, m_editorFactories)
            removeObject(pf);
        qDeleteAll(m_editorFactories);
    }

    if (m_coreListener) {
        removeObject(m_coreListener);
        delete m_coreListener;
        m_coreListener = 0;
    }

    if (m_submitEditorFactory) {
        removeObject(m_submitEditorFactory);
        m_submitEditorFactory = 0;
    }

    cleanChangeTmpFile();
    delete m_gitClient;
    m_instance = 0;
}

void GitPlugin::cleanChangeTmpFile()
{
    if (m_changeTmpFile) {
        delete m_changeTmpFile;
        m_changeTmpFile = 0;
    }
}

GitPlugin *GitPlugin::instance()
{
    return m_instance;
}

static const VCSBase::VCSBaseSubmitEditorParameters submitParameters = {
    Git::Constants::SUBMIT_MIMETYPE,
    Git::Constants::GITSUBMITEDITOR_KIND,
    Git::Constants::C_GITSUBMITEDITOR,
    Core::Constants::UNDO,
    Core::Constants::REDO,
    Git::Constants::SUBMIT_CURRENT,
    Git::Constants::DIFF_SELECTED
};


bool GitPlugin::initialize(const QStringList &arguments, QString *error_message)
{
    typedef VCSBase::VCSEditorFactory<GitEditor> GitEditorFactory;
    typedef VCSBase::VCSSubmitEditorFactory<GitSubmitEditor> GitSubmitEditorFactory;

    Q_UNUSED(arguments);
    Q_UNUSED(error_message);

    m_core = ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>();
    m_gitClient = new GitClient(this, m_core);
    // Create the globalcontext list to register actions accordingly
    QList<int> globalcontext;
    globalcontext << m_core->uniqueIDManager()->
        uniqueIdentifier(Core::Constants::C_GLOBAL);

    // Create the output Window
    m_outputWindow = new GitOutputWindow();
    addObject(m_outputWindow);

    // Create the settings Page
    m_settingsPage = new SettingsPage();
    addObject(m_settingsPage);

    static const char *describeSlot = SLOT(show(QString,QString));
    const int editorCount = sizeof(editorParameters)/sizeof(VCSBase::VCSBaseEditorParameters);
    for (int i = 0; i < editorCount; i++) {
        m_editorFactories.push_back(new GitEditorFactory(editorParameters + i, m_core, m_gitClient, describeSlot));
        addObject(m_editorFactories.back());
    }

    m_coreListener = new CoreListener(this);
    addObject(m_coreListener);

    m_submitEditorFactory = new GitSubmitEditorFactory(&submitParameters);
    addObject(m_submitEditorFactory);

    //register actions
    Core::ActionManagerInterface *actionManager = m_core->actionManager();

    Core::IActionContainer *toolsContainer =
        actionManager->actionContainer(Core::Constants::M_TOOLS);

    Core::IActionContainer *gitContainer =
        actionManager->createMenu(QLatin1String("Git"));
    gitContainer->menu()->setTitle(tr("&Git"));
    toolsContainer->addMenu(gitContainer);

    Core::ICommand *command;
    QAction *tmpaction;

    m_diffAction = new QAction(tr("Diff current file"), this);
    command = actionManager->registerAction(m_diffAction, "Git.Diff", globalcontext);
    command->setAttribute(Core::ICommand::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+G,Alt+D")));
    connect(m_diffAction, SIGNAL(triggered()), this, SLOT(diffCurrentFile()));
    gitContainer->addAction(command);

    m_statusAction = new QAction(tr("File Status"), this);
    command = actionManager->registerAction(m_statusAction, "Git.Status", globalcontext);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+G,Alt+S")));
    command->setAttribute(Core::ICommand::CA_UpdateText);
    connect(m_statusAction, SIGNAL(triggered()), this, SLOT(statusFile()));
    gitContainer->addAction(command);

    m_logAction = new QAction(tr("Log File"), this);
    command = actionManager->registerAction(m_logAction, "Git.Log", globalcontext);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+G,Alt+L")));
    command->setAttribute(Core::ICommand::CA_UpdateText);
    connect(m_logAction, SIGNAL(triggered()), this, SLOT(logFile()));
    gitContainer->addAction(command);

    m_blameAction = new QAction(tr("Blame"), this);
    command = actionManager->registerAction(m_blameAction, "Git.Blame", globalcontext);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+G,Alt+B")));
    command->setAttribute(Core::ICommand::CA_UpdateText);
    connect(m_blameAction, SIGNAL(triggered()), this, SLOT(blameFile()));
    gitContainer->addAction(command);

    m_undoFileAction = new QAction(tr("Undo Changes"), this);
    command = actionManager->registerAction(m_undoFileAction, "Git.Undo", globalcontext);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+G,Alt+U")));
    command->setAttribute(Core::ICommand::CA_UpdateText);
    connect(m_undoFileAction, SIGNAL(triggered()), this, SLOT(undoFileChanges()));
    gitContainer->addAction(command);

    m_addAction = new QAction(tr("Add File"), this);
    command = actionManager->registerAction(m_addAction, "Git.Add", globalcontext);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+G,Alt+A")));
    command->setAttribute(Core::ICommand::CA_UpdateText);
    connect(m_addAction, SIGNAL(triggered()), this, SLOT(addFile()));
    gitContainer->addAction(command);

    tmpaction = new QAction(this);
    tmpaction->setSeparator(true);
    command = actionManager->registerAction(tmpaction, QLatin1String("Git.Sep.Project"), globalcontext);
    gitContainer->addAction(command);

    m_diffProjectAction = new QAction(tr("Diff current project"), this);
    command = actionManager->registerAction(m_diffProjectAction, "Git.DiffProject", globalcontext);
    command->setDefaultKeySequence(QKeySequence("Alt+G,Alt+Shift+D"));
    command->setAttribute(Core::ICommand::CA_UpdateText);
    connect(m_diffProjectAction, SIGNAL(triggered()), this, SLOT(diffCurrentProject()));
    gitContainer->addAction(command);

    m_statusProjectAction = new QAction(tr("Project status"), this);
    command = actionManager->registerAction(m_statusProjectAction, "Git.StatusProject", globalcontext);
    command->setAttribute(Core::ICommand::CA_UpdateText);
    connect(m_statusProjectAction, SIGNAL(triggered()), this, SLOT(statusProject()));
    gitContainer->addAction(command);

    m_logProjectAction = new QAction(tr("Log project"), this);
    command = actionManager->registerAction(m_logProjectAction, "Git.LogProject", globalcontext);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+G,Alt+K")));
    command->setAttribute(Core::ICommand::CA_UpdateText);
    connect(m_logProjectAction, SIGNAL(triggered()), this, SLOT(logProject()));
    gitContainer->addAction(command);

    m_undoProjectAction = new QAction(tr("Undo Project Changes"), this);
    command = actionManager->registerAction(m_undoProjectAction, "Git.UndoProject", globalcontext);
    command->setAttribute(Core::ICommand::CA_UpdateText);
    connect(m_undoProjectAction, SIGNAL(triggered()), this, SLOT(undoProjectChanges()));
    gitContainer->addAction(command);

    tmpaction = new QAction(this);
    tmpaction->setSeparator(true);
    command = actionManager->registerAction(tmpaction, QLatin1String("Git.Sep.Global"), globalcontext);
    gitContainer->addAction(command);

    m_showAction = new QAction(tr("Show commit..."), this);
    command = actionManager->registerAction(m_showAction, "Git.ShowCommit", globalcontext);
    command->setAttribute(Core::ICommand::CA_UpdateText);
    connect(m_showAction, SIGNAL(triggered()), this, SLOT(showCommit()));
    gitContainer->addAction(command);

    m_commitAction = new QAction(tr("Commit..."), this);
    command = actionManager->registerAction(m_commitAction, "Git.Commit", globalcontext);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+G,Alt+C")));
    command->setAttribute(Core::ICommand::CA_UpdateText);
    connect(m_commitAction, SIGNAL(triggered()), this, SLOT(startCommit()));
    gitContainer->addAction(command);

    m_pullAction = new QAction(tr("Pull"), this);
    command = actionManager->registerAction(m_pullAction, "Git.Pull", globalcontext);
    command->setAttribute(Core::ICommand::CA_UpdateText);
    connect(m_pullAction, SIGNAL(triggered()), this, SLOT(pull()));
    gitContainer->addAction(command);

    m_pushAction = new QAction(tr("Push"), this);
    command = actionManager->registerAction(m_pushAction, "Git.Push", globalcontext);
    command->setAttribute(Core::ICommand::CA_UpdateText);
    connect(m_pushAction, SIGNAL(triggered()), this, SLOT(push()));
    gitContainer->addAction(command);

    // Submit editor
    QList<int> submitContext;
    submitContext.push_back(m_core->uniqueIDManager()->uniqueIdentifier(QLatin1String(Constants::C_GITSUBMITEDITOR)));
    m_submitCurrentAction = new QAction(QIcon(Constants::ICON_SUBMIT), tr("Commit"), this);
    command = actionManager->registerAction(m_submitCurrentAction, Constants::SUBMIT_CURRENT, submitContext);
    // TODO
    connect(m_submitCurrentAction, SIGNAL(triggered()), this, SLOT(submitCurrentLog()));

    m_diffSelectedFilesAction = new QAction(QIcon(Constants::ICON_DIFF), tr("Diff Selected Files"), this);
    command = actionManager->registerAction(m_diffSelectedFilesAction, Constants::DIFF_SELECTED, submitContext);

    m_undoAction = new QAction(tr("&Undo"), this);
    command = actionManager->registerAction(m_undoAction, Core::Constants::UNDO, submitContext);

    m_redoAction = new QAction(tr("&Redo"), this);
    command = actionManager->registerAction(m_redoAction, Core::Constants::REDO, submitContext);

    // Ask for updates of our actions, in case context switches
    connect(m_core, SIGNAL(contextChanged(Core::IContext *)),
        this, SLOT(updateActions()));
    connect(m_core->fileManager(), SIGNAL(currentFileChanged(const QString &)),
        this, SLOT(updateActions()));

    return true;
}

void GitPlugin::extensionsInitialized()
{
    m_projectExplorer = ExtensionSystem::PluginManager::instance()->getObject<ProjectExplorer::ProjectExplorerPlugin>();
}

bool GitPlugin::vcsOpen(const QString &fileName)
{
    Q_UNUSED(fileName);
    return false;
}

void GitPlugin::submitEditorDiff(const QStringList &files)
{
    if (files.empty())
        return;
    m_gitClient->diff(m_submitRepository, files);
}

void GitPlugin::diffCurrentFile()
{
    QFileInfo fileInfo = currentFile();
    QString fileName = fileInfo.fileName();
    QString workingDirectory = fileInfo.absolutePath();
    m_gitClient->diff(workingDirectory, fileName);
}

void GitPlugin::diffCurrentProject()
{
    QString workingDirectory = getWorkingDirectory();
    if (workingDirectory.isEmpty())
        return;
    m_gitClient->diff(workingDirectory, QString());
}

QFileInfo GitPlugin::currentFile()
{
    QString fileName = m_core->fileManager()->currentFile();
    QFileInfo fileInfo(fileName);
    return fileInfo;
}

QString GitPlugin::getWorkingDirectory()
{
    QString workingDirectory;
    if (m_projectExplorer && m_projectExplorer->currentNode()) {
        workingDirectory = QFileInfo(m_projectExplorer->currentNode()->path()).absolutePath();
    }
    if (Git::Constants::debug > 1)
        qDebug() << Q_FUNC_INFO << "Project" << workingDirectory;

    if (workingDirectory.isEmpty())
        workingDirectory = QFileInfo(m_core->fileManager()->currentFile()).absolutePath();
    if (Git::Constants::debug > 1)
        qDebug() << Q_FUNC_INFO << "file" << workingDirectory;

    if (workingDirectory.isEmpty()) {
        m_outputWindow->clearContents();
        m_outputWindow->append(tr("Could not find working directory"));
        m_outputWindow->popup();
        return QString();
    }
    return workingDirectory;
}

void GitPlugin::statusProject()
{
    QString workingDirectory = getWorkingDirectory();
    if (workingDirectory.isEmpty())
        return;
    m_gitClient->status(workingDirectory);
}

void GitPlugin::statusFile()
{
    m_gitClient->status(currentFile().absolutePath());
}

void GitPlugin::logFile()
{
    const QFileInfo fileInfo = currentFile();
    const QString fileName = fileInfo.fileName();
    const QString workingDirectory = fileInfo.absolutePath();
    m_gitClient->log(workingDirectory, fileName);
}

void GitPlugin::blameFile()
{
    const QFileInfo fileInfo = currentFile();
    const QString fileName = fileInfo.fileName();
    const QString workingDirectory = fileInfo.absolutePath();
    m_gitClient->blame(workingDirectory, fileName);
}

void GitPlugin::logProject()
{
    QString workingDirectory = getWorkingDirectory();
    if (workingDirectory.isEmpty())
        return;
    m_gitClient->log(workingDirectory, QString());
}

void GitPlugin::undoFileChanges()
{
    QFileInfo fileInfo = currentFile();
    QString fileName = fileInfo.fileName();
    QString workingDirectory = fileInfo.absolutePath();
    m_gitClient->checkout(workingDirectory, fileName);
}

void GitPlugin::undoProjectChanges()
{
    QString workingDirectory = getWorkingDirectory();
    if (workingDirectory.isEmpty())
        return;
    m_gitClient->hardReset(workingDirectory, QString());
}

void GitPlugin::addFile()
{
    QFileInfo fileInfo = currentFile();
    QString fileName = fileInfo.fileName();
    QString workingDirectory = fileInfo.absolutePath();
    m_gitClient->addFile(workingDirectory, fileName);
}

void GitPlugin::startCommit()
{
    if (m_changeTmpFile) {
        m_outputWindow->append(tr("Another submit is currently beeing executed."));
        m_outputWindow->popup(false);
        return;
    }

    // Find repository and get commit data
    const QFileInfo currentFileInfo = currentFile();
    if (!currentFileInfo.exists())
        return;

    const QString workingDirectory = currentFileInfo.absolutePath();
    QString errorMessage, commitTemplate;
    CommitData data;
    if (!m_gitClient->getCommitData(workingDirectory, &commitTemplate, &data, &errorMessage)) {
        m_outputWindow->append(errorMessage);
        m_outputWindow->popup(false);
        return;
    }

    m_submitRepository = data.panelInfo.repository;

    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << data << commitTemplate;

    // Start new temp file with message template
    QTemporaryFile *changeTmpFile = new QTemporaryFile(this);
    changeTmpFile->setAutoRemove(true);
    if (!changeTmpFile->open()) {
        m_outputWindow->append(tr("Cannot create temporary file: %1").arg(changeTmpFile->errorString()));
        delete changeTmpFile;
        return;
    }
    m_changeTmpFile = changeTmpFile;
    m_changeTmpFile->write(commitTemplate.toLocal8Bit());
    m_changeTmpFile->flush();
    // Keep the file alive, else it removes self and forgets
    // its name
    m_changeTmpFile->seek(0);
    openSubmitEditor(m_changeTmpFile->fileName(), data);
}

Core::IEditor *GitPlugin::openSubmitEditor(const QString &fileName, const CommitData &cd)
{
    Core::IEditor *editor =  m_core->editorManager()->openEditor(fileName, QLatin1String(Constants::GITSUBMITEDITOR_KIND));
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << fileName << editor;
    m_core->editorManager()->ensureEditorManagerVisible();
    GitSubmitEditor *submitEditor = qobject_cast<GitSubmitEditor*>(editor);
    Q_ASSERT(submitEditor);
    // The actions are for some reason enabled by the context switching
    // mechanism. Disable them correctly.
    m_submitCurrentAction->setEnabled(!cd.commitFiles.empty());
    m_diffSelectedFilesAction->setEnabled(false);
    m_undoAction->setEnabled(false);
    m_redoAction->setEnabled(false);
    submitEditor->setCommitData(cd);
    connect(submitEditor, SIGNAL(diffSelectedFiles(QStringList)), this, SLOT(submitEditorDiff(QStringList)));
    return editor;
}

void GitPlugin::submitCurrentLog()
{
    // Close the submit editor
    QList<Core::IEditor*> editors;
    editors.push_back(m_core->editorManager()->currentEditor());
    m_core->editorManager()->closeEditors(editors);
}

bool GitPlugin::editorAboutToClose(Core::IEditor *iEditor)
{
    // Closing a submit editor?
    if (!m_changeTmpFile || !iEditor || qstrcmp(iEditor->kind(), Constants::GITSUBMITEDITOR_KIND))
        return true;
    Core::IFile *fileIFace = iEditor->file();
    const GitSubmitEditor *editor = qobject_cast<GitSubmitEditor *>(iEditor);
    if (!fileIFace || !editor)
        return true;
    // Submit editor closing. Make it write out the commit message
    // and retrieve files
    const QFileInfo editorFile(fileIFace->fileName());
    const QFileInfo changeFile(m_changeTmpFile->fileName());
    // Paranoia!
    if (editorFile.absoluteFilePath() != changeFile.absoluteFilePath())
        return true;
    // Prompt user.
    const QMessageBox::StandardButton answer = QMessageBox::question(m_core->mainWindow(), tr("Closing git editor"),  tr("Do you want to commit the change?"),
                                                                     QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel, QMessageBox::Yes);
    switch (answer) {
    case QMessageBox::Cancel:
        return false; // Keep editing and change file
    case QMessageBox::No:
        cleanChangeTmpFile();
        return true; // Cancel all
    default:
        break;
    }
    // Go ahead!
    const QStringList fileList = editor->checkedFiles();
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << fileList;
    if (!fileList.empty()) {
        // get message & commit
        m_core->fileManager()->blockFileChange(fileIFace);
        fileIFace->save();
        m_core->fileManager()->unblockFileChange(fileIFace);

        m_gitClient->addAndCommit(m_submitRepository,
                                  editor->panelData(),
                                  m_changeTmpFile->fileName(),
                                  fileList);
    }
    cleanChangeTmpFile();
    return true;
}

void GitPlugin::pull()
{
    QString workingDirectory = getWorkingDirectory();
    if (workingDirectory.isEmpty())
        return;
    m_gitClient->pull(workingDirectory);
}

void GitPlugin::push()
{
    QString workingDirectory = getWorkingDirectory();
    if (workingDirectory.isEmpty())
        return;
    m_gitClient->push(workingDirectory);
}

void GitPlugin::updateActions()
{
    QFileInfo current = currentFile();
    const QString fileName = current.fileName();
    const QString currentDirectory = getWorkingDirectory();
    QString repository = m_gitClient->findRepositoryForFile(current.absoluteFilePath());
    // First check for file commands and if the current file is inside
    // a Git-repository
    m_diffAction->setText(tr("Diff %1").arg(fileName));
    m_statusAction->setText(tr("Status related to %1").arg(fileName));
    m_logAction->setText(tr("Log %1").arg(fileName));
    m_blameAction->setText(tr("Blame %1").arg(fileName));
    m_undoFileAction->setText(tr("Undo changes for %1").arg(fileName));
    m_addAction->setText(tr("Add %1").arg(fileName));
    if (repository.isEmpty()) {
        // If the file is not in a repository, the corresponding project will
        // be neither and we can disable everything and return
        m_diffAction->setEnabled(false);
        m_statusAction->setEnabled(false);
        m_logAction->setEnabled(false);
        m_blameAction->setEnabled(false);
        m_undoFileAction->setEnabled(false);
        m_addAction->setEnabled(false);
        m_diffProjectAction->setEnabled(false);
        m_diffProjectAction->setText(tr("Diff Project"));
        m_statusProjectAction->setText(tr("Status Project"));
        m_statusProjectAction->setEnabled(false);
        m_logProjectAction->setText(tr("Log Project"));
        m_logProjectAction->setEnabled(false);
        return;
    } else {
        // We only know the file is in some repository, we do not know
        // anything about any project so far.
        m_diffAction->setEnabled(true);
        m_statusAction->setEnabled(true);
        m_logAction->setEnabled(true);
        m_blameAction->setEnabled(true);
        m_undoFileAction->setEnabled(true);
        m_addAction->setEnabled(true);
    }

    if (m_projectExplorer && m_projectExplorer->currentNode()
        && m_projectExplorer->currentNode()->projectNode()) {
        QString name = QFileInfo(m_projectExplorer->currentNode()->projectNode()->path()).baseName();
        m_diffProjectAction->setEnabled(true);
        m_diffProjectAction->setText(tr("Diff Project %1").arg(name));
        m_statusProjectAction->setEnabled(true);
        m_statusProjectAction->setText(tr("Status Project %1").arg(name));
        m_logProjectAction->setEnabled(true);
        m_logProjectAction->setText(tr("Log Project %1").arg(name));
    } else {
        m_diffProjectAction->setEnabled(false);
        m_diffProjectAction->setText(tr("Diff Project"));
        m_statusProjectAction->setEnabled(false);
        m_statusProjectAction->setText(tr("Status Project"));
        m_logProjectAction->setEnabled(false);
        m_logProjectAction->setText(tr("Log Project"));
    }
}

void GitPlugin::showCommit()
{
    if (!m_changeSelectionDialog)
        m_changeSelectionDialog = new ChangeSelectionDialog();

    const QFileInfo currentInfo = currentFile();
    QString repositoryLocation = m_gitClient->findRepositoryForFile(currentInfo.absoluteFilePath());
    if (!repositoryLocation.isEmpty())
        m_changeSelectionDialog->m_ui.repositoryEdit->setText(repositoryLocation);

    if (m_changeSelectionDialog->exec() != QDialog::Accepted)
        return;
    const QString change = m_changeSelectionDialog->m_ui.changeNumberEdit->text();
    if (change .isEmpty())
        return;

    m_gitClient->show(m_changeSelectionDialog->m_ui.repositoryEdit->text(), change);
}

Q_EXPORT_PLUGIN(GitPlugin)
