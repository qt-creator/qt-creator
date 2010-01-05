/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "subversionplugin.h"

#include "settingspage.h"
#include "subversioneditor.h"

#include "subversionsubmiteditor.h"
#include "subversionconstants.h"
#include "subversioncontrol.h"
#include "checkoutwizard.h"

#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbaseoutputwindow.h>
#include <utils/synchronousprocess.h>
#include <utils/parameteraction.h>

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTextCodec>
#include <QtCore/QtPlugin>
#include <QtGui/QAction>
#include <QtGui/QFileDialog>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QInputDialog>

#include <limits.h>

using namespace Subversion::Internal;

static const char * const CMD_ID_SUBVERSION_MENU    = "Subversion.Menu";
static const char * const CMD_ID_ADD                = "Subversion.Add";
static const char * const CMD_ID_DELETE_FILE        = "Subversion.Delete";
static const char * const CMD_ID_REVERT             = "Subversion.Revert";
static const char * const CMD_ID_SEPARATOR0         = "Subversion.Separator0";
static const char * const CMD_ID_DIFF_PROJECT       = "Subversion.DiffAll";
static const char * const CMD_ID_DIFF_CURRENT       = "Subversion.DiffCurrent";
static const char * const CMD_ID_SEPARATOR1         = "Subversion.Separator1";
static const char * const CMD_ID_COMMIT_ALL         = "Subversion.CommitAll";
static const char * const CMD_ID_COMMIT_CURRENT     = "Subversion.CommitCurrent";
static const char * const CMD_ID_SEPARATOR2         = "Subversion.Separator2";
static const char * const CMD_ID_FILELOG_CURRENT    = "Subversion.FilelogCurrent";
static const char * const CMD_ID_ANNOTATE_CURRENT   = "Subversion.AnnotateCurrent";
static const char * const CMD_ID_SEPARATOR3         = "Subversion.Separator3";
static const char * const CMD_ID_STATUS             = "Subversion.Status";
static const char * const CMD_ID_UPDATE             = "Subversion.Update";
static const char * const CMD_ID_DESCRIBE           = "Subversion.Describe";

static const char *nonInteractiveOptionC = "--non-interactive";

static const VCSBase::VCSBaseEditorParameters editorParameters[] = {
{
    VCSBase::RegularCommandOutput,
    "Subversion Command Log Editor", // kind
    "Subversion Command Log Editor", // context
    "application/vnd.nokia.text.scs_svn_commandlog",
    "scslog"},
{   VCSBase::LogOutput,
    "Subversion File Log Editor",   // kind
    "Subversion File Log Editor",   // context
    "application/vnd.nokia.text.scs_svn_filelog",
    "scsfilelog"},
{    VCSBase::AnnotateOutput,
    "Subversion Annotation Editor",  // kind
    "Subversion Annotation Editor",  // context
    "application/vnd.nokia.text.scs_svn_annotation",
    "scsannotate"},
{   VCSBase::DiffOutput,
    "Subversion Diff Editor",  // kind
    "Subversion Diff Editor",  // context
    "text/x-patch","diff"}
};

// Utility to find a parameter set by type
static inline const VCSBase::VCSBaseEditorParameters *findType(int ie)
{
    const VCSBase::EditorContentType et = static_cast<VCSBase::EditorContentType>(ie);
    return  VCSBase::VCSBaseEditor::findType(editorParameters, sizeof(editorParameters)/sizeof(VCSBase::VCSBaseEditorParameters), et);
}

static inline QString debugCodec(const QTextCodec *c)
{
    return c ? QString::fromAscii(c->name()) : QString::fromAscii("Null codec");
}

Core::IEditor* locateEditor(const char *property, const QString &entry)
{
    foreach (Core::IEditor *ed, Core::EditorManager::instance()->openedEditors())
        if (ed->property(property).toString() == entry)
            return ed;
    return 0;
}

// Parse "svn status" output for added/modified/deleted files
// "M<7blanks>file"
typedef QList<SubversionSubmitEditor::StatusFilePair> StatusList;

StatusList parseStatusOutput(const QString &output)
{
    StatusList changeSet;
    const QString newLine = QString(QLatin1Char('\n'));
    const QStringList list = output.split(newLine, QString::SkipEmptyParts);
    foreach (const QString &l, list) {
        const QString line =l.trimmed();
        if (line.size() > 8) {
            const QChar state = line.at(0);
            if (state == QLatin1Char('A') || state == QLatin1Char('D') || state == QLatin1Char('M')) {
                const QString fileName = line.mid(7); // Column 8 starting from svn 1.6
                changeSet.push_back(SubversionSubmitEditor::StatusFilePair(QString(state), fileName.trimmed()));
            }

        }
    }
    return changeSet;
}

// Return a list of names for the internal svn directories
static inline QStringList svnDirectories()
{
    QStringList rc(QLatin1String(".svn"));
#ifdef Q_OS_WIN
    // Option on Windows systems to avoid hassle with some IDEs
    rc.push_back(QLatin1String("_svn"));
#endif
    return rc;
}

// ------------- SubversionPlugin
SubversionPlugin *SubversionPlugin::m_subversionPluginInstance = 0;

SubversionPlugin::SubversionPlugin() :
    VCSBase::VCSBasePlugin(QLatin1String(Subversion::Constants::SUBVERSIONCOMMITEDITOR_KIND)),
    m_svnDirectories(svnDirectories()),
    m_addAction(0),
    m_deleteAction(0),
    m_revertAction(0),
    m_diffProjectAction(0),
    m_diffCurrentAction(0),
    m_commitAllAction(0),
    m_commitCurrentAction(0),
    m_filelogCurrentAction(0),
    m_annotateCurrentAction(0),
    m_statusProjectAction(0),
    m_updateProjectAction(0),
    m_describeAction(0),
    m_submitCurrentLogAction(0),
    m_submitDiffAction(0),
    m_submitUndoAction(0),
    m_submitRedoAction(0),
    m_menuAction(0),
    m_submitActionTriggered(false)
{
}

SubversionPlugin::~SubversionPlugin()
{
    cleanCommitMessageFile();
}

void SubversionPlugin::cleanCommitMessageFile()
{
    if (!m_commitMessageFileName.isEmpty()) {
        QFile::remove(m_commitMessageFileName);
        m_commitMessageFileName.clear();
        m_commitRepository.clear();
    }
}

bool SubversionPlugin::isCommitEditorOpen() const
{
    return !m_commitMessageFileName.isEmpty();
}

static const VCSBase::VCSBaseSubmitEditorParameters submitParameters = {
    Subversion::Constants::SUBVERSION_SUBMIT_MIMETYPE,
    Subversion::Constants::SUBVERSIONCOMMITEDITOR_KIND,
    Subversion::Constants::SUBVERSIONCOMMITEDITOR
};

static inline Core::Command *createSeparator(QObject *parent,
                                             Core::ActionManager *ami,
                                             const char*id,
                                             const QList<int> &globalcontext)
{
    QAction *tmpaction = new QAction(parent);
    tmpaction->setSeparator(true);
    return ami->registerAction(tmpaction, id, globalcontext);
}

bool SubversionPlugin::initialize(const QStringList & /*arguments */, QString *errorMessage)
{
    typedef VCSBase::VCSSubmitEditorFactory<SubversionSubmitEditor> SubversionSubmitEditorFactory;
    typedef VCSBase::VCSEditorFactory<SubversionEditor> SubversionEditorFactory;
    using namespace Constants;

    using namespace Core::Constants;
    using namespace ExtensionSystem;

    VCSBase::VCSBasePlugin::initialize(new SubversionControl(this));

    m_subversionPluginInstance = this;
    Core::ICore *core = Core::ICore::instance();

    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":/trolltech.subversion/Subversion.mimetypes.xml"), errorMessage))
        return false;

    if (QSettings *settings = core->settings())
        m_settings.fromSettings(settings);

    addAutoReleasedObject(new SettingsPage);

    addAutoReleasedObject(new SubversionSubmitEditorFactory(&submitParameters));

    static const char *describeSlot = SLOT(describe(QString,QString));
    const int editorCount = sizeof(editorParameters)/sizeof(VCSBase::VCSBaseEditorParameters);
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new SubversionEditorFactory(editorParameters + i, this, describeSlot));

    addAutoReleasedObject(new CheckoutWizard);

    //register actions
    Core::ActionManager *ami = core->actionManager();
    Core::ActionContainer *toolsContainer = ami->actionContainer(M_TOOLS);

    Core::ActionContainer *subversionMenu =
        ami->createMenu(QLatin1String(CMD_ID_SUBVERSION_MENU));
    subversionMenu->menu()->setTitle(tr("&Subversion"));
    toolsContainer->addMenu(subversionMenu);
    m_menuAction = subversionMenu->menu()->menuAction();
    QList<int> globalcontext;
    globalcontext << core->uniqueIDManager()->uniqueIdentifier(C_GLOBAL);

    Core::Command *command;
    m_addAction = new Utils::ParameterAction(tr("Add"), tr("Add \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_addAction, CMD_ID_ADD,
        globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+S,Alt+A")));
    connect(m_addAction, SIGNAL(triggered()), this, SLOT(addCurrentFile()));
    subversionMenu->addAction(command);

    m_deleteAction = new Utils::ParameterAction(tr("Delete"), tr("Delete \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_deleteAction, CMD_ID_DELETE_FILE,
        globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_deleteAction, SIGNAL(triggered()), this, SLOT(deleteCurrentFile()));
    subversionMenu->addAction(command);

    m_revertAction = new Utils::ParameterAction(tr("Revert"), tr("Revert \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_revertAction, CMD_ID_REVERT,
        globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_revertAction, SIGNAL(triggered()), this, SLOT(revertCurrentFile()));
    subversionMenu->addAction(command);

    subversionMenu->addAction(createSeparator(this, ami, CMD_ID_SEPARATOR0, globalcontext));

    m_diffProjectAction = new Utils::ParameterAction(tr("Diff Project"), tr("Diff Project \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_diffProjectAction, CMD_ID_DIFF_PROJECT,
        globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_diffProjectAction, SIGNAL(triggered()), this, SLOT(diffProject()));
    subversionMenu->addAction(command);

    m_diffCurrentAction = new Utils::ParameterAction(tr("Diff Current File"), tr("Diff \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_diffCurrentAction,
        CMD_ID_DIFF_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+S,Alt+D")));
    connect(m_diffCurrentAction, SIGNAL(triggered()), this, SLOT(diffCurrentFile()));
    subversionMenu->addAction(command);

    subversionMenu->addAction(createSeparator(this, ami, CMD_ID_SEPARATOR1, globalcontext));

    m_commitAllAction = new QAction(tr("Commit All Files"), this);
    command = ami->registerAction(m_commitAllAction, CMD_ID_COMMIT_ALL,
        globalcontext);
    connect(m_commitAllAction, SIGNAL(triggered()), this, SLOT(startCommitAll()));
    subversionMenu->addAction(command);

    m_commitCurrentAction = new Utils::ParameterAction(tr("Commit Current File"), tr("Commit \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_commitCurrentAction,
        CMD_ID_COMMIT_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+S,Alt+C")));
    connect(m_commitCurrentAction, SIGNAL(triggered()), this, SLOT(startCommitCurrentFile()));
    subversionMenu->addAction(command);

    subversionMenu->addAction(createSeparator(this, ami, CMD_ID_SEPARATOR2, globalcontext));

    m_filelogCurrentAction = new Utils::ParameterAction(tr("Filelog Current File"), tr("Filelog \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_filelogCurrentAction,
        CMD_ID_FILELOG_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_filelogCurrentAction, SIGNAL(triggered()), this,
        SLOT(filelogCurrentFile()));
    subversionMenu->addAction(command);

    m_annotateCurrentAction = new Utils::ParameterAction(tr("Annotate Current File"), tr("Annotate \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_annotateCurrentAction,
        CMD_ID_ANNOTATE_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_annotateCurrentAction, SIGNAL(triggered()), this,
        SLOT(annotateCurrentFile()));
    subversionMenu->addAction(command);

    m_describeAction = new QAction(tr("Describe..."), this);
    command = ami->registerAction(m_describeAction, CMD_ID_DESCRIBE, globalcontext);
    connect(m_describeAction, SIGNAL(triggered()), this, SLOT(slotDescribe()));
    subversionMenu->addAction(command);

    subversionMenu->addAction(createSeparator(this, ami, CMD_ID_SEPARATOR3, globalcontext));

    m_statusProjectAction = new Utils::ParameterAction(tr("Project Status"), tr("Status of Project \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_statusProjectAction, CMD_ID_STATUS,
        globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_statusProjectAction, SIGNAL(triggered()), this, SLOT(projectStatus()));
    subversionMenu->addAction(command);

    m_updateProjectAction = new Utils::ParameterAction(tr("Update Project"), tr("Update Project \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_updateProjectAction, CMD_ID_UPDATE, globalcontext);
    connect(m_updateProjectAction, SIGNAL(triggered()), this, SLOT(updateProject()));
    command->setAttribute(Core::Command::CA_UpdateText);
    subversionMenu->addAction(command);

    // Actions of the submit editor
    QList<int> svncommitcontext;
    svncommitcontext << Core::UniqueIDManager::instance()->uniqueIdentifier(Constants::SUBVERSIONCOMMITEDITOR);

    m_submitCurrentLogAction = new QAction(VCSBase::VCSBaseSubmitEditor::submitIcon(), tr("Commit"), this);
    command = ami->registerAction(m_submitCurrentLogAction, Constants::SUBMIT_CURRENT, svncommitcontext);
    connect(m_submitCurrentLogAction, SIGNAL(triggered()), this, SLOT(submitCurrentLog()));

    m_submitDiffAction = new QAction(VCSBase::VCSBaseSubmitEditor::diffIcon(), tr("Diff Selected Files"), this);
    command = ami->registerAction(m_submitDiffAction , Constants::DIFF_SELECTED, svncommitcontext);

    m_submitUndoAction = new QAction(tr("&Undo"), this);
    command = ami->registerAction(m_submitUndoAction, Core::Constants::UNDO, svncommitcontext);

    m_submitRedoAction = new QAction(tr("&Redo"), this);
    command = ami->registerAction(m_submitRedoAction, Core::Constants::REDO, svncommitcontext);

    return true;
}

void SubversionPlugin::extensionsInitialized()
{
}

bool SubversionPlugin::submitEditorAboutToClose(VCSBase::VCSBaseSubmitEditor *submitEditor)
{
    if (!isCommitEditorOpen())
        return true;

    Core::IFile *fileIFace = submitEditor->file();
    const SubversionSubmitEditor *editor = qobject_cast<SubversionSubmitEditor *>(submitEditor);
    if (!fileIFace || !editor)
        return true;

    // Submit editor closing. Make it write out the commit message
    // and retrieve files
    const QFileInfo editorFile(fileIFace->fileName());
    const QFileInfo changeFile(m_commitMessageFileName);
    if (editorFile.absoluteFilePath() != changeFile.absoluteFilePath())
        return true; // Oops?!

    // Prompt user. Force a prompt unless submit was actually invoked (that
    // is, the editor was closed or shutdown).
    SubversionSettings newSettings = m_settings;
    const VCSBase::VCSBaseSubmitEditor::PromptSubmitResult answer =
            editor->promptSubmit(tr("Closing Subversion Editor"),
                                 tr("Do you want to commit the change?"),
                                 tr("The commit message check failed. Do you want to commit the change?"),
                                 &newSettings.promptToSubmit, !m_submitActionTriggered);
    m_submitActionTriggered = false;
    switch (answer) {
    case VCSBase::VCSBaseSubmitEditor::SubmitCanceled:
        return false; // Keep editing and change file
    case VCSBase::VCSBaseSubmitEditor::SubmitDiscarded:
        cleanCommitMessageFile();
        return true; // Cancel all
    default:
        break;
    }
    setSettings(newSettings); // in case someone turned prompting off
    const QStringList fileList = editor->checkedFiles();
    bool closeEditor = true;
    if (!fileList.empty()) {
        // get message & commit
        Core::ICore::instance()->fileManager()->blockFileChange(fileIFace);
        fileIFace->save();
        Core::ICore::instance()->fileManager()->unblockFileChange(fileIFace);
        closeEditor= commit(m_commitMessageFileName, fileList);
    }
    if (closeEditor)
        cleanCommitMessageFile();
    return closeEditor;
}

void SubversionPlugin::diffCommitFiles(const QStringList &files)
{
    svnDiff(m_commitRepository, files);
}

static inline void setDiffBaseDirectory(Core::IEditor *editor, const QString &db)
{
    if (VCSBase::VCSBaseEditor *ve = qobject_cast<VCSBase::VCSBaseEditor*>(editor->widget()))
        ve->setDiffBaseDirectory(db);
}

void SubversionPlugin::svnDiff(const QString &workingDir, const QStringList &files, QString diffname)
{
    if (Subversion::Constants::debug)
        qDebug() << Q_FUNC_INFO << files << diffname;
    const QString source = VCSBase::VCSBaseEditor::getSource(workingDir, files);
    QTextCodec *codec = source.isEmpty() ? static_cast<QTextCodec *>(0) : VCSBase::VCSBaseEditor::getCodec(source);

    if (files.count() == 1 && diffname.isEmpty())
        diffname = QFileInfo(files.front()).fileName();

    QStringList args(QLatin1String("diff"));
    args << files;

    const SubversionResponse response = runSvn(workingDir, args, m_settings.timeOutMS(), false, codec);
    if (response.error)
        return;

    // diff of a single file? re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    if (files.count() == 1) {
        // Show in the same editor if diff has been executed before
        if (Core::IEditor *editor = locateEditor("originalFileName", files.front())) {
            editor->createNew(response.stdOut);
            Core::EditorManager::instance()->activateEditor(editor);
            setDiffBaseDirectory(editor, workingDir);
            return;
        }
    }
    const QString title = QString::fromLatin1("svn diff %1").arg(diffname);
    Core::IEditor *editor = showOutputInEditor(title, response.stdOut, VCSBase::DiffOutput, source, codec);
    setDiffBaseDirectory(editor, workingDir);
    if (files.count() == 1)
        editor->setProperty("originalFileName", files.front());
}

SubversionSubmitEditor *SubversionPlugin::openSubversionSubmitEditor(const QString &fileName)
{
    Core::IEditor *editor = Core::EditorManager::instance()->openEditor(fileName, QLatin1String(Constants::SUBVERSIONCOMMITEDITOR_KIND));
    SubversionSubmitEditor *submitEditor = qobject_cast<SubversionSubmitEditor*>(editor);
    QTC_ASSERT(submitEditor, /**/);
    submitEditor->registerActions(m_submitUndoAction, m_submitRedoAction, m_submitCurrentLogAction, m_submitDiffAction);
    connect(submitEditor, SIGNAL(diffSelectedFiles(QStringList)), this, SLOT(diffCommitFiles(QStringList)));
    submitEditor->setCheckScriptWorkingDirectory(m_commitRepository);

    return submitEditor;
}

void SubversionPlugin::updateActions(VCSBase::VCSBasePlugin::ActionState as)
{
    if (!VCSBase::VCSBasePlugin::enableMenuAction(as, m_menuAction))
        return;

    const QString projectName = currentState().currentProjectName();
    m_diffProjectAction->setParameter(projectName);
    m_statusProjectAction->setParameter(projectName);
    m_updateProjectAction->setParameter(projectName);

    const bool repoEnabled = currentState().hasTopLevel();
    m_commitAllAction->setEnabled(repoEnabled);
    m_describeAction->setEnabled(repoEnabled);

    const QString fileName = currentState().currentFileName();

    m_addAction->setParameter(fileName);
    m_deleteAction->setParameter(fileName);
    m_revertAction->setParameter(fileName);
    m_diffCurrentAction->setParameter(fileName);
    m_commitCurrentAction->setParameter(fileName);
    m_filelogCurrentAction->setParameter(fileName);
    m_annotateCurrentAction->setParameter(fileName);
}

void SubversionPlugin::addCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    vcsAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void SubversionPlugin::deleteCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    vcsDelete(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void SubversionPlugin::revertCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)

    QStringList args(QLatin1String("diff"));
    args.push_back(state.relativeCurrentFile());

    const SubversionResponse diffResponse = runSvn(state.currentFileTopLevel(), args, m_settings.timeOutMS(), false);
    if (diffResponse.error)
        return;

    if (diffResponse.stdOut.isEmpty())
        return;
    if (QMessageBox::warning(0, QLatin1String("svn revert"), tr("The file has been changed. Do you want to revert it?"),
                             QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
        return;


    Core::FileChangeBlocker fcb(state.currentFile());

    // revert
    args.clear();
    args << QLatin1String("revert") << state.relativeCurrentFile();

    const SubversionResponse revertResponse = runSvn(state.currentFileTopLevel(), args, m_settings.timeOutMS(), true);
    if (!revertResponse.error) {
        fcb.setModifiedReload(true);
        subVersionControl()->emitFilesChanged(QStringList(state.currentFile()));
    }
}

void SubversionPlugin::diffProject()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return)
    svnDiff(state.currentProjectTopLevel(), state.relativeCurrentProject(), state.currentProjectName());
}

void SubversionPlugin::diffCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    svnDiff(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void SubversionPlugin::startCommitCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    startCommit(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void SubversionPlugin::startCommitAll()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    startCommit(state.topLevel());
}

/* Start commit of files of a single repository by displaying
 * template and files in a submit editor. On closing, the real
 * commit will start. */
void SubversionPlugin::startCommit(const QString &workingDir, const QStringList &files)
{
    if (VCSBase::VCSBaseSubmitEditor::raiseSubmitEditor())
        return;
    if (isCommitEditorOpen()) {
        VCSBase::VCSBaseOutputWindow::instance()->appendWarning(tr("Another commit is currently being executed."));
        return;
    }

    QStringList args(QLatin1String("status"));
    args += files;

    const SubversionResponse response = runSvn(workingDir, args, m_settings.timeOutMS(), false);
    if (response.error)
        return;

    // Get list of added/modified/deleted files
    const StatusList statusOutput = parseStatusOutput(response.stdOut);
    if (statusOutput.empty()) {
        VCSBase::VCSBaseOutputWindow::instance()->appendWarning(tr("There are no modified files."));
        return;
    }
    m_commitRepository = workingDir;
    // Create a new submit change file containing the submit template
    QTemporaryFile changeTmpFile;
    changeTmpFile.setAutoRemove(false);
    if (!changeTmpFile.open()) {
        VCSBase::VCSBaseOutputWindow::instance()->appendError(tr("Cannot create temporary file: %1").arg(changeTmpFile.errorString()));
        return;
    }
    m_commitMessageFileName = changeTmpFile.fileName();
    // TODO: Regitctrieve submit template from
    const QString submitTemplate;
    // Create a submit
    changeTmpFile.write(submitTemplate.toUtf8());
    changeTmpFile.flush();
    changeTmpFile.close();
    // Create a submit editor and set file list
    SubversionSubmitEditor *editor = openSubversionSubmitEditor(m_commitMessageFileName);
    editor->setStatusList(statusOutput);
}

bool SubversionPlugin::commit(const QString &messageFile,
                              const QStringList &subVersionFileList)
{
    if (Subversion::Constants::debug)
        qDebug() << Q_FUNC_INFO << messageFile << subVersionFileList;
    // Transform the status list which is sth
    // "[ADM]<blanks>file" into an args list. The files of the status log
    // can be relative or absolute depending on where the command was run.
    QStringList args = QStringList(QLatin1String("commit"));
    args << QLatin1String(nonInteractiveOptionC) << QLatin1String("--file") << messageFile;
    args.append(subVersionFileList);
    const SubversionResponse response = runSvn(m_commitRepository, args, m_settings.longTimeOutMS(), true);
    return !response.error ;
}

void SubversionPlugin::filelogCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
   filelog(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void SubversionPlugin::filelog(const QString &workingDir, const QStringList &files)
{
    QTextCodec *codec = VCSBase::VCSBaseEditor::getCodec(workingDir, files);
    // no need for temp file
    QStringList args(QLatin1String("log"));
    foreach(const QString &file, files)
        args.append(QDir::toNativeSeparators(file));

    const SubversionResponse response = runSvn(workingDir, args, m_settings.timeOutMS(), false, codec);
    if (response.error)
        return;

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file

    const QString id = VCSBase::VCSBaseEditor::getTitleId(workingDir, files);
    if (Core::IEditor *editor = locateEditor("logFileName", id)) {
        editor->createNew(response.stdOut);
        Core::EditorManager::instance()->activateEditor(editor);
    } else {
        const QString title = QString::fromLatin1("svn log %1").arg(id);
        const QString source = VCSBase::VCSBaseEditor::getSource(workingDir, files);
        Core::IEditor *newEditor = showOutputInEditor(title, response.stdOut, VCSBase::LogOutput, source, codec);
        newEditor->setProperty("logFileName", id);
    }
}

void SubversionPlugin::updateProject()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);

    QStringList args(QLatin1String("update"));
    args.push_back(QLatin1String(nonInteractiveOptionC));
    args.append(state.relativeCurrentProject());
    const SubversionResponse response = runSvn(state.currentProjectTopLevel(), args, m_settings.longTimeOutMS(), true);
    if (!response.error)
        subVersionControl()->emitRepositoryChanged(state.currentProjectTopLevel());
}

void SubversionPlugin::annotateCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    annotate(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void SubversionPlugin::annotate(const QString &workingDir, const QString &file)
{
    QTextCodec *codec = VCSBase::VCSBaseEditor::getCodec(file);

    QStringList args(QLatin1String("annotate"));
    if (m_settings.spaceIgnorantAnnotation)
        args << QLatin1String("-x") << QLatin1String("-uw");
    args.push_back(QLatin1String("-v"));
    args.append(QDir::toNativeSeparators(file));

    const SubversionResponse response = runSvn(workingDir, args, m_settings.timeOutMS(), false, codec);
    if (response.error)
        return;

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    const QString source = workingDir + QLatin1Char('/') + file;
    const int lineNumber = VCSBase::VCSBaseEditor::lineNumberOfCurrentEditor(source);
    const QString id = VCSBase::VCSBaseEditor::getTitleId(workingDir, QStringList(file));

    if (Core::IEditor *editor = locateEditor("annotateFileName", id)) {
        editor->createNew(response.stdOut);
        VCSBase::VCSBaseEditor::gotoLineOfEditor(editor, lineNumber);
        Core::EditorManager::instance()->activateEditor(editor);        
    } else {
        const QString title = QString::fromLatin1("svn annotate %1").arg(id);
        Core::IEditor *newEditor = showOutputInEditor(title, response.stdOut, VCSBase::AnnotateOutput, source, codec);
        newEditor->setProperty("annotateFileName", id);
        VCSBase::VCSBaseEditor::gotoLineOfEditor(newEditor, lineNumber);
    }
}

void SubversionPlugin::projectStatus()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    QStringList args(QLatin1String("status"));
    args += state.relativeCurrentProject();
    VCSBase::VCSBaseOutputWindow *outwin = VCSBase::VCSBaseOutputWindow::instance();
    outwin->setRepository(state.currentProjectTopLevel());
    runSvn(state.currentProjectTopLevel(), args, m_settings.timeOutMS(), true);
    outwin->clearRepository();
}

void SubversionPlugin::describe(const QString &source, const QString &changeNr)
{
    // To describe a complete change, find the top level and then do
    //svn diff -r 472958:472959 <top level>
    const QFileInfo fi(source);
    const QString topLevel = findTopLevelForDirectory(fi.isDir() ? source : fi.absolutePath());
    if (topLevel.isEmpty())
        return;
    if (Subversion::Constants::debug)
        qDebug() << Q_FUNC_INFO << source << topLevel << changeNr;
    // Number must be > 1
    bool ok;
    const int number = changeNr.toInt(&ok);
    if (!ok || number < 2)
        return;
    // Run log to obtain message (local utf8)
    QString description;
    QStringList args(QLatin1String("log"));
    args.push_back(QLatin1String("-r"));
    args.push_back(changeNr);
    const SubversionResponse logResponse = runSvn(topLevel, args, m_settings.timeOutMS(), false);
    if (logResponse.error)
        return;
    description = logResponse.stdOut;

    // Run diff (encoding via source codec)
    args.clear();
    args.push_back(QLatin1String("diff"));
    args.push_back(QLatin1String("-r"));
    QString diffArg;
    QTextStream(&diffArg) << (number - 1) << ':' << number;
    args.push_back(diffArg);

    QTextCodec *codec = VCSBase::VCSBaseEditor::getCodec(source);
    const SubversionResponse response = runSvn(topLevel, args, m_settings.timeOutMS(), false, codec);
    if (response.error)
        return;
    description += response.stdOut;

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    const QString id = diffArg + source;
    if (Core::IEditor *editor = locateEditor("describeChange", id)) {
        editor->createNew(description);
        Core::EditorManager::instance()->activateEditor(editor);
    } else {
        const QString title = QString::fromLatin1("svn describe %1#%2").arg(fi.fileName(), changeNr);
        Core::IEditor *newEditor = showOutputInEditor(title, description, VCSBase::DiffOutput, source, codec);
        newEditor->setProperty("describeChange", id);
    }
}

void SubversionPlugin::slotDescribe()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    QInputDialog inputDialog(Core::ICore::instance()->mainWindow());
    inputDialog.setWindowFlags(inputDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    inputDialog.setInputMode(QInputDialog::IntInput);
    inputDialog.setIntRange(2, INT_MAX);
    inputDialog.setWindowTitle(tr("Describe"));
    inputDialog.setLabelText(tr("Revision number:"));
    if (inputDialog.exec() != QDialog::Accepted)
        return;

    const int revision = inputDialog.intValue();
    describe(state.topLevel(), QString::number(revision));
}

void SubversionPlugin::submitCurrentLog()
{
    m_submitActionTriggered = true;
    Core::EditorManager::instance()->closeEditors(QList<Core::IEditor*>()
        << Core::EditorManager::instance()->currentEditor());
}

static inline QString processStdErr(QProcess &proc)
{
    return QString::fromLocal8Bit(proc.readAllStandardError()).remove(QLatin1Char('\r'));
}

static inline QString processStdOut(QProcess &proc, QTextCodec *outputCodec = 0)
{
    const QByteArray stdOutData = proc.readAllStandardOutput();
    QString stdOut = outputCodec ? outputCodec->toUnicode(stdOutData) : QString::fromLocal8Bit(stdOutData);
    return stdOut.remove(QLatin1Char('\r'));
}

SubversionResponse SubversionPlugin::runSvn(const QString &workingDir,
                                            const QStringList &arguments,
                                            int timeOut,
                                            bool showStdOutInOutputWindow,
                                            QTextCodec *outputCodec)
{
    const QString executable = m_settings.svnCommand;
    SubversionResponse response;
    if (executable.isEmpty()) {
        response.error = true;
        response.message =tr("No subversion executable specified!");
        return response;
    }
    const QStringList allArgs = m_settings.addOptions(arguments);

    VCSBase::VCSBaseOutputWindow *outputWindow = VCSBase::VCSBaseOutputWindow::instance();
    // Hide passwords, etc in the log window
    //: Executing: <executable> <arguments>
    const QString outputText = tr("Executing: %1 %2\n").arg(executable, SubversionSettings::formatArguments(allArgs));
    outputWindow->appendCommand(outputText);

    if (Subversion::Constants::debug)
        qDebug() << "runSvn" << timeOut << outputText;

    // Run, connect stderr to the output window
    Utils::SynchronousProcess process;
    if (!workingDir.isEmpty())
        process.setWorkingDirectory(workingDir);
    process.setTimeout(timeOut);
    process.setStdOutCodec(outputCodec);

    process.setStdErrBufferedSignalsEnabled(true);
    connect(&process, SIGNAL(stdErrBuffered(QString,bool)), outputWindow, SLOT(append(QString)));

    // connect stdout to the output window if desired
    if (showStdOutInOutputWindow) {
        process.setStdOutBufferedSignalsEnabled(true);
        connect(&process, SIGNAL(stdOutBuffered(QString,bool)), outputWindow, SLOT(append(QString)));
    }

    const Utils::SynchronousProcessResponse sp_resp = process.run(executable, allArgs);
    response.error = true;
    response.stdErr = sp_resp.stdErr;
    response.stdOut = sp_resp.stdOut;
    switch (sp_resp.result) {
    case Utils::SynchronousProcessResponse::Finished:
        response.error = false;
        break;
    case Utils::SynchronousProcessResponse::FinishedError:
        response.message = tr("The process terminated with exit code %1.").arg(sp_resp.exitCode);
        break;
    case Utils::SynchronousProcessResponse::TerminatedAbnormally:
        response.message = tr("The process terminated abnormally.");
        break;
    case Utils::SynchronousProcessResponse::StartFailed:
        response.message = tr("Could not start subversion '%1'. Please check your settings in the preferences.").arg(executable);
        break;
    case Utils::SynchronousProcessResponse::Hang:
        response.message = tr("Subversion did not respond within timeout limit (%1 ms).").arg(timeOut);
        break;
    }
    if (response.error)
        outputWindow->appendError(response.message);

    return response;
}

Core::IEditor * SubversionPlugin::showOutputInEditor(const QString& title, const QString &output,
                                                     int editorType, const QString &source,
                                                     QTextCodec *codec)
{
    const VCSBase::VCSBaseEditorParameters *params = findType(editorType);
    QTC_ASSERT(params, return 0);
    const QString kind = QLatin1String(params->kind);
    if (Subversion::Constants::debug)
        qDebug() << "SubversionPlugin::showOutputInEditor" << title << kind <<  "Size= " << output.size() <<  " Type=" << editorType << debugCodec(codec);
    QString s = title;
    Core::IEditor *editor = Core::EditorManager::instance()->openEditorWithContents(kind, &s, output);
    SubversionEditor *e = qobject_cast<SubversionEditor*>(editor->widget());
    if (!e)
        return 0;
    s.replace(QLatin1Char(' '), QLatin1Char('_'));
    e->setSuggestedFileName(s);
    if (!source.isEmpty())
        e->setSource(source);
    if (codec)
        e->setCodec(codec);
    Core::IEditor *ie = e->editableInterface();
    Core::EditorManager::instance()->activateEditor(ie);
    return ie;
}

SubversionSettings SubversionPlugin::settings() const
{
    return m_settings;
}

void SubversionPlugin::setSettings(const SubversionSettings &s)
{
    if (s != m_settings) {
        m_settings = s;
        if (QSettings *settings = Core::ICore::instance()->settings())
            m_settings.toSettings(settings);
    }
}

SubversionPlugin *SubversionPlugin::subversionPluginInstance()
{
    QTC_ASSERT(m_subversionPluginInstance, return m_subversionPluginInstance);
    return m_subversionPluginInstance;
}

bool SubversionPlugin::vcsAdd(const QString &workingDir, const QString &rawFileName)
{
    const QString file = QDir::toNativeSeparators(rawFileName);
    QStringList args(QLatin1String("add"));
    args.push_back(file);

    const SubversionResponse response = runSvn(workingDir, args, m_settings.timeOutMS(), true);
    return !response.error;
}

bool SubversionPlugin::vcsDelete(const QString &workingDir, const QString &rawFileName)
{
    const QString file = QDir::toNativeSeparators(rawFileName);

    QStringList args(QLatin1String("delete"));
    args.push_back(file);

    const SubversionResponse response = runSvn(workingDir, args, m_settings.timeOutMS(), true);
    return !response.error;
}

/* Subversion has ".svn" directory in each directory
 * it manages. The top level is the first directory
 * under the directory that does not have a  ".svn". */
bool SubversionPlugin::managesDirectory(const QString &directory) const
{
    const QDir dir(directory);
    const bool rc = dir.exists() && managesDirectory(dir);
    if (Subversion::Constants::debug)
        qDebug() << "SubversionPlugin::managesDirectory" << directory << rc;
    return rc;
}

bool SubversionPlugin::managesDirectory(const QDir &directory) const
{
    const int dirCount = m_svnDirectories.size();
    for (int i = 0; i < dirCount; i++) {
        const QString svnDir = directory.absoluteFilePath(m_svnDirectories.at(i));
        if (QFileInfo(svnDir).isDir())
            return true;
    }
    return false;
}

QString SubversionPlugin::findTopLevelForDirectory(const QString &directory) const
{
    // Debug wrapper
    const QString rc = findTopLevelForDirectoryI(directory);
    if (Subversion::Constants::debug)
        qDebug() << "SubversionPlugin::findTopLevelForDirectory" << directory << rc;
    return rc;
}

QString SubversionPlugin::findTopLevelForDirectoryI(const QString &directory) const
{
    /* Recursing up, the top level is a child of the first directory that does
     * not have a  ".svn" directory. The starting directory must be a managed
     * one. Go up and try to find the first unmanaged parent dir. */
    QDir lastDirectory = QDir(directory);
    if (!lastDirectory.exists() || !managesDirectory(lastDirectory))
        return QString();
    for (QDir parentDir = lastDirectory; parentDir.cdUp() ; lastDirectory = parentDir) {
        if (!managesDirectory(parentDir))
            return QDir::toNativeSeparators(lastDirectory.absolutePath());
    }
    return QString();
}

SubversionControl *SubversionPlugin::subVersionControl() const
{
    return static_cast<SubversionControl *>(versionControl());
}

Q_EXPORT_PLUGIN(SubversionPlugin)
