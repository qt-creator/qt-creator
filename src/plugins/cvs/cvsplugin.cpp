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

#include "cvsplugin.h"
#include "settingspage.h"
#include "cvseditor.h"
#include "cvssubmiteditor.h"
#include "cvsconstants.h"
#include "cvscontrol.h"
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
#include <coreplugin/vcsmanager.h>
#include <utils/stringutils.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QDate>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTextCodec>
#include <QtCore/QtPlugin>
#include <QtCore/QTemporaryFile>
#include <QtGui/QAction>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>

namespace CVS {
    namespace Internal {

static inline QString msgCannotFindTopLevel(const QString &f)
{
    return CVSPlugin::tr("Cannot find repository for '%1'").arg(f);
}

static inline QString msgLogParsingFailed()
{
    return CVSPlugin::tr("Parsing of the log output failed");
}

static const char * const CMD_ID_CVS_MENU    = "CVS.Menu";
static const char * const CMD_ID_ADD                = "CVS.Add";
static const char * const CMD_ID_DELETE_FILE        = "CVS.Delete";
static const char * const CMD_ID_REVERT             = "CVS.Revert";
static const char * const CMD_ID_SEPARATOR0         = "CVS.Separator0";
static const char * const CMD_ID_DIFF_PROJECT       = "CVS.DiffAll";
static const char * const CMD_ID_DIFF_CURRENT       = "CVS.DiffCurrent";
static const char * const CMD_ID_SEPARATOR1         = "CVS.Separator1";
static const char * const CMD_ID_COMMIT_ALL         = "CVS.CommitAll";
static const char * const CMD_ID_COMMIT_CURRENT     = "CVS.CommitCurrent";
static const char * const CMD_ID_SEPARATOR2         = "CVS.Separator2";
static const char * const CMD_ID_FILELOG_CURRENT    = "CVS.FilelogCurrent";
static const char * const CMD_ID_ANNOTATE_CURRENT   = "CVS.AnnotateCurrent";
static const char * const CMD_ID_SEPARATOR3         = "CVS.Separator3";
static const char * const CMD_ID_STATUS             = "CVS.Status";
static const char * const CMD_ID_UPDATE             = "CVS.Update";

static const VCSBase::VCSBaseEditorParameters editorParameters[] = {
{
    VCSBase::RegularCommandOutput,
    "CVS Command Log Editor", // kind
    "CVS Command Log Editor", // context
    "application/vnd.nokia.text.scs_cvs_commandlog",
    "scslog"},
{   VCSBase::LogOutput,
    "CVS File Log Editor",   // kind
    "CVS File Log Editor",   // context
    "application/vnd.nokia.text.scs_cvs_filelog",
    "scsfilelog"},
{    VCSBase::AnnotateOutput,
    "CVS Annotation Editor",  // kind
    "CVS Annotation Editor",  // context
    "application/vnd.nokia.text.scs_cvs_annotation",
    "scsannotate"},
{   VCSBase::DiffOutput,
    "CVS Diff Editor",  // kind
    "CVS Diff Editor",  // context
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

// ------------- CVSPlugin
CVSPlugin *CVSPlugin::m_cvsPluginInstance = 0;

CVSPlugin::CVSPlugin() :
    VCSBase::VCSBasePlugin(QLatin1String(CVS::Constants::CVSCOMMITEDITOR_KIND)),
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
    m_submitCurrentLogAction(0),
    m_submitDiffAction(0),
    m_submitUndoAction(0),
    m_submitRedoAction(0),
    m_menuAction(0),
    m_submitActionTriggered(false)
{
}

CVSPlugin::~CVSPlugin()
{
    cleanCommitMessageFile();
}

void CVSPlugin::cleanCommitMessageFile()
{
    if (!m_commitMessageFileName.isEmpty()) {
        QFile::remove(m_commitMessageFileName);
        m_commitMessageFileName.clear();
        m_commitRepository.clear();
    }
}
bool CVSPlugin::isCommitEditorOpen() const
{
    return !m_commitMessageFileName.isEmpty();
}

static const VCSBase::VCSBaseSubmitEditorParameters submitParameters = {
    CVS::Constants::CVS_SUBMIT_MIMETYPE,
    CVS::Constants::CVSCOMMITEDITOR_KIND,
    CVS::Constants::CVSCOMMITEDITOR
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

bool CVSPlugin::initialize(const QStringList & /*arguments */, QString *errorMessage)
{
    typedef VCSBase::VCSSubmitEditorFactory<CVSSubmitEditor> CVSSubmitEditorFactory;
    typedef VCSBase::VCSEditorFactory<CVSEditor> CVSEditorFactory;
    using namespace Constants;

    using namespace Core::Constants;
    using namespace ExtensionSystem;

    VCSBase::VCSBasePlugin::initialize(new CVSControl(this));

    m_cvsPluginInstance = this;
    Core::ICore *core = Core::ICore::instance();

    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":/trolltech.cvs/CVS.mimetypes.xml"), errorMessage))
        return false;

    if (QSettings *settings = core->settings())
        m_settings.fromSettings(settings);

    addAutoReleasedObject(new SettingsPage);

    addAutoReleasedObject(new CVSSubmitEditorFactory(&submitParameters));

    static const char *describeSlotC = SLOT(slotDescribe(QString,QString));
    const int editorCount = sizeof(editorParameters)/sizeof(VCSBase::VCSBaseEditorParameters);
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new CVSEditorFactory(editorParameters + i, this, describeSlotC));

    addAutoReleasedObject(new CheckoutWizard);

    //register actions
    Core::ActionManager *ami = core->actionManager();
    Core::ActionContainer *toolsContainer = ami->actionContainer(M_TOOLS);

    Core::ActionContainer *cvsMenu =
        ami->createMenu(QLatin1String(CMD_ID_CVS_MENU));
    cvsMenu->menu()->setTitle(tr("&CVS"));
    toolsContainer->addMenu(cvsMenu);
    m_menuAction = cvsMenu->menu()->menuAction();

    QList<int> globalcontext;
    globalcontext << core->uniqueIDManager()->uniqueIdentifier(C_GLOBAL);

    Core::Command *command;
    m_addAction = new Utils::ParameterAction(tr("Add"), tr("Add \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_addAction, CMD_ID_ADD,
        globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+C,Alt+A")));
    connect(m_addAction, SIGNAL(triggered()), this, SLOT(addCurrentFile()));
    cvsMenu->addAction(command);

    m_deleteAction = new Utils::ParameterAction(tr("Delete"), tr("Delete \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_deleteAction, CMD_ID_DELETE_FILE,
        globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_deleteAction, SIGNAL(triggered()), this, SLOT(deleteCurrentFile()));
    cvsMenu->addAction(command);

    m_revertAction = new Utils::ParameterAction(tr("Revert"), tr("Revert \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_revertAction, CMD_ID_REVERT,
        globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_revertAction, SIGNAL(triggered()), this, SLOT(revertCurrentFile()));
    cvsMenu->addAction(command);

    cvsMenu->addAction(createSeparator(this, ami, CMD_ID_SEPARATOR0, globalcontext));

    m_diffProjectAction = new Utils::ParameterAction(tr("Diff Project"), tr("Diff Project \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_diffProjectAction, CMD_ID_DIFF_PROJECT,
        globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_diffProjectAction, SIGNAL(triggered()), this, SLOT(diffProject()));
    cvsMenu->addAction(command);

    m_diffCurrentAction = new Utils::ParameterAction(tr("Diff Current File"), tr("Diff \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_diffCurrentAction,
        CMD_ID_DIFF_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+C,Alt+D")));
    connect(m_diffCurrentAction, SIGNAL(triggered()), this, SLOT(diffCurrentFile()));
    cvsMenu->addAction(command);

    cvsMenu->addAction(createSeparator(this, ami, CMD_ID_SEPARATOR1, globalcontext));

    m_commitAllAction = new QAction(tr("Commit All Files"), this);
    command = ami->registerAction(m_commitAllAction, CMD_ID_COMMIT_ALL,
        globalcontext);
    connect(m_commitAllAction, SIGNAL(triggered()), this, SLOT(startCommitAll()));
    cvsMenu->addAction(command);

    m_commitCurrentAction = new Utils::ParameterAction(tr("Commit Current File"), tr("Commit \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_commitCurrentAction,
        CMD_ID_COMMIT_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+C,Alt+C")));
    connect(m_commitCurrentAction, SIGNAL(triggered()), this, SLOT(startCommitCurrentFile()));
    cvsMenu->addAction(command);

    cvsMenu->addAction(createSeparator(this, ami, CMD_ID_SEPARATOR2, globalcontext));

    m_filelogCurrentAction = new Utils::ParameterAction(tr("Filelog Current File"), tr("Filelog \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_filelogCurrentAction,
        CMD_ID_FILELOG_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_filelogCurrentAction, SIGNAL(triggered()), this,
        SLOT(filelogCurrentFile()));
    cvsMenu->addAction(command);

    m_annotateCurrentAction = new Utils::ParameterAction(tr("Annotate Current File"), tr("Annotate \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_annotateCurrentAction,
        CMD_ID_ANNOTATE_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_annotateCurrentAction, SIGNAL(triggered()), this,
        SLOT(annotateCurrentFile()));
    cvsMenu->addAction(command);

    cvsMenu->addAction(createSeparator(this, ami, CMD_ID_SEPARATOR3, globalcontext));

    m_statusProjectAction = new Utils::ParameterAction(tr("Project Status"), tr("Status of Project \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_statusProjectAction, CMD_ID_STATUS,
        globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_statusProjectAction, SIGNAL(triggered()), this, SLOT(projectStatus()));
    cvsMenu->addAction(command);

    m_updateProjectAction = new Utils::ParameterAction(tr("Update Project"), tr("Update Project \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_updateProjectAction, CMD_ID_UPDATE, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_updateProjectAction, SIGNAL(triggered()), this, SLOT(updateProject()));
    cvsMenu->addAction(command);

    // Actions of the submit editor
    QList<int> cvscommitcontext;
    cvscommitcontext << Core::UniqueIDManager::instance()->uniqueIdentifier(Constants::CVSCOMMITEDITOR);

    m_submitCurrentLogAction = new QAction(VCSBase::VCSBaseSubmitEditor::submitIcon(), tr("Commit"), this);
    command = ami->registerAction(m_submitCurrentLogAction, Constants::SUBMIT_CURRENT, cvscommitcontext);
    connect(m_submitCurrentLogAction, SIGNAL(triggered()), this, SLOT(submitCurrentLog()));

    m_submitDiffAction = new QAction(VCSBase::VCSBaseSubmitEditor::diffIcon(), tr("Diff Selected Files"), this);
    command = ami->registerAction(m_submitDiffAction , Constants::DIFF_SELECTED, cvscommitcontext);

    m_submitUndoAction = new QAction(tr("&Undo"), this);
    command = ami->registerAction(m_submitUndoAction, Core::Constants::UNDO, cvscommitcontext);

    m_submitRedoAction = new QAction(tr("&Redo"), this);
    command = ami->registerAction(m_submitRedoAction, Core::Constants::REDO, cvscommitcontext);
    return true;
}

void CVSPlugin::extensionsInitialized()
{
}

bool CVSPlugin::submitEditorAboutToClose(VCSBase::VCSBaseSubmitEditor *submitEditor)
{
    if (!isCommitEditorOpen())
        return true;

    Core::IFile *fileIFace = submitEditor->file();
    const CVSSubmitEditor *editor = qobject_cast<CVSSubmitEditor *>(submitEditor);
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
    CVSSettings newSettings = m_settings;
    const VCSBase::VCSBaseSubmitEditor::PromptSubmitResult answer =
            editor->promptSubmit(tr("Closing CVS Editor"),
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

void CVSPlugin::diffCommitFiles(const QStringList &files)
{
    cvsDiff(m_commitRepository, files);
}

static inline void setDiffBaseDirectory(Core::IEditor *editor, const QString &db)
{
    if (VCSBase::VCSBaseEditor *ve = qobject_cast<VCSBase::VCSBaseEditor*>(editor->widget()))
        ve->setDiffBaseDirectory(db);
}

void CVSPlugin::cvsDiff(const QString &workingDir, const QStringList &files)
{
    if (CVS::Constants::debug)
        qDebug() << Q_FUNC_INFO << files;
    const QString source = VCSBase::VCSBaseEditor::getSource(workingDir, files);
    QTextCodec *codec = VCSBase::VCSBaseEditor::getCodec(workingDir, files);
    const QString id = VCSBase::VCSBaseEditor::getTitleId(workingDir, files);

    QStringList args(QLatin1String("diff"));
    args << m_settings.cvsDiffOptions;
    args.append(files);

    // CVS returns the diff exit code (1 if files differ), which is
    // undistinguishable from a "file not found" error, unfortunately.
    const CVSResponse response = runCVS(workingDir, args, m_settings.timeOutMS(), false, codec);
    switch (response.result) {
    case CVSResponse::NonNullExitCode:
    case CVSResponse::Ok:
        break;
    case CVSResponse::OtherError:
        return;
    }

    QString output = fixDiffOutput(response.stdOut);
    if (output.isEmpty())
        output = tr("The files do not differ.");
    // diff of a single file? re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    if (files.count() == 1) {
        // Show in the same editor if diff has been executed before
        if (Core::IEditor *editor = locateEditor("originalFileName", id)) {
            editor->createNew(output);
            Core::EditorManager::instance()->activateEditor(editor);
            setDiffBaseDirectory(editor, workingDir);
            return;
        }
    }
    const QString title = QString::fromLatin1("cvs diff %1").arg(id);
    Core::IEditor *editor = showOutputInEditor(title, output, VCSBase::DiffOutput, source, codec);
    if (files.count() == 1)
        editor->setProperty("originalFileName", id);
    setDiffBaseDirectory(editor, workingDir);
}

CVSSubmitEditor *CVSPlugin::openCVSSubmitEditor(const QString &fileName)
{
    Core::IEditor *editor = Core::EditorManager::instance()->openEditor(fileName, QLatin1String(Constants::CVSCOMMITEDITOR_KIND));
    CVSSubmitEditor *submitEditor = qobject_cast<CVSSubmitEditor*>(editor);
    QTC_ASSERT(submitEditor, /**/);
    submitEditor->registerActions(m_submitUndoAction, m_submitRedoAction, m_submitCurrentLogAction, m_submitDiffAction);
    connect(submitEditor, SIGNAL(diffSelectedFiles(QStringList)), this, SLOT(diffCommitFiles(QStringList)));

    return submitEditor;
}

void CVSPlugin::updateActions(VCSBase::VCSBasePlugin::ActionState as)
{
    if (!VCSBase::VCSBasePlugin::enableMenuAction(as, m_menuAction))
        return;

    const QString currentFileName = currentState().currentFileName();
    m_addAction->setParameter(currentFileName);
    m_deleteAction->setParameter(currentFileName);
    m_revertAction->setParameter(currentFileName);
    m_diffCurrentAction->setParameter(currentFileName);
    m_commitCurrentAction->setParameter(currentFileName);
    m_filelogCurrentAction->setParameter(currentFileName);
    m_annotateCurrentAction->setParameter(currentFileName);

    const QString currentProjectName = currentState().currentProjectName();
    m_diffProjectAction->setParameter(currentProjectName);
    m_statusProjectAction->setParameter(currentProjectName);
    m_updateProjectAction->setParameter(currentProjectName);

    m_commitAllAction->setEnabled(currentState().hasTopLevel());
}

void CVSPlugin::addCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    vcsAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void CVSPlugin::deleteCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    if (!Core::ICore::instance()->vcsManager()->showDeleteDialog(state.currentFile()))
        QMessageBox::warning(0, QLatin1String("CVS remove"), tr("The file '%1' could not be deleted.").arg(state.currentFile()), QMessageBox::Ok);
}

void CVSPlugin::revertCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    QStringList args;
    args << QLatin1String("diff") << state.relativeCurrentFile();
    const CVSResponse diffResponse = runCVS(state.currentFileTopLevel(), args, m_settings.timeOutMS(), false);
    switch (diffResponse.result) {
    case CVSResponse::Ok:
        return; // Not modified, diff exit code 0
    case CVSResponse::NonNullExitCode: // Diff exit code != 0
        if (diffResponse.stdOut.isEmpty()) // Paranoia: Something else failed?
            return;
        break;
    case CVSResponse::OtherError:
        return;
    }

    if (QMessageBox::warning(0, QLatin1String("CVS revert"), tr("The file has been changed. Do you want to revert it?"),
                             QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
        return;

    Core::FileChangeBlocker fcb(state.currentFile());

    // revert
    args.clear();
    args << QLatin1String("update") << QLatin1String("-C") << state.relativeCurrentFile();
    const CVSResponse revertResponse = runCVS(state.currentFileTopLevel(), args, m_settings.timeOutMS(), true);
    if (revertResponse.result == CVSResponse::Ok) {
        fcb.setModifiedReload(true);
        cvsVersionControl()->emitFilesChanged(QStringList(state.currentFile()));
    }
}

void CVSPlugin::diffProject()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return)
    cvsDiff(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void CVSPlugin::diffCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    cvsDiff(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void CVSPlugin::startCommitCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    startCommit(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void CVSPlugin::startCommitAll()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    startCommit(state.topLevel());
}

/* Start commit of files of a single repository by displaying
 * template and files in a submit editor. On closing, the real
 * commit will start. */
void CVSPlugin::startCommit(const QString &workingDir, const QStringList &files)
{
    if (VCSBase::VCSBaseSubmitEditor::raiseSubmitEditor())
        return;
    if (isCommitEditorOpen()) {
        VCSBase::VCSBaseOutputWindow::instance()->appendWarning(tr("Another commit is currently being executed."));
        return;
    }

    // We need the "Examining <subdir>" stderr output to tell
    // where we are, so, have stdout/stderr channels merged.
    QStringList args = QStringList(QLatin1String("status"));
    const CVSResponse response = runCVS(workingDir, args, m_settings.timeOutMS(), false, 0, true);
    if (response.result != CVSResponse::Ok)
        return;
    // Get list of added/modified/deleted files and purge out undesired ones
    // (do not run status with relative arguments as it will omit the directories)
    StateList statusOutput = parseStatusOutput(QString(), response.stdOut);
    if (!files.isEmpty()) {
        for (StateList::iterator it = statusOutput.begin(); it != statusOutput.end() ; ) {
            if (files.contains(it->second)) {
                ++it;
            } else {
                it = statusOutput.erase(it);
            }
        }
    }
    if (statusOutput.empty()) {
        VCSBase::VCSBaseOutputWindow::instance()->append(tr("There are no modified files."));
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
    // TODO: Retrieve submit template from
    const QString submitTemplate;
    m_commitMessageFileName = changeTmpFile.fileName();
    // Create a submit
    changeTmpFile.write(submitTemplate.toUtf8());
    changeTmpFile.flush();
    changeTmpFile.close();
    // Create a submit editor and set file list
    CVSSubmitEditor *editor = openCVSSubmitEditor(m_commitMessageFileName);
    editor->setCheckScriptWorkingDirectory(m_commitRepository);
    editor->setStateList(statusOutput);
}

bool CVSPlugin::commit(const QString &messageFile,
                              const QStringList &fileList)
{
    if (CVS::Constants::debug)
        qDebug() << Q_FUNC_INFO << messageFile << fileList;
    QStringList args = QStringList(QLatin1String("commit"));
    args << QLatin1String("-F") << messageFile;
    args.append(fileList);
    const CVSResponse response = runCVS(m_commitRepository, args, m_settings.longTimeOutMS(), true);
    return response.result == CVSResponse::Ok ;
}

void CVSPlugin::filelogCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    filelog(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void CVSPlugin::filelog(const QString &workingDir, const QStringList &files)
{
    QTextCodec *codec = VCSBase::VCSBaseEditor::getCodec(workingDir, files);
    // no need for temp file
    const QString id = VCSBase::VCSBaseEditor::getTitleId(workingDir, files);
    const QString source = VCSBase::VCSBaseEditor::getSource(workingDir, files);
    QStringList args;
    args << QLatin1String("log");
    args.append(files);
    const CVSResponse response = runCVS(workingDir, args, m_settings.timeOutMS(), false, codec);
    if (response.result != CVSResponse::Ok)
        return;

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file

    if (Core::IEditor *editor = locateEditor("logFileName", id)) {
        editor->createNew(response.stdOut);
        Core::EditorManager::instance()->activateEditor(editor);
    } else {
        const QString title = QString::fromLatin1("cvs log %1").arg(id);
        Core::IEditor *newEditor = showOutputInEditor(title, response.stdOut, VCSBase::LogOutput, source, codec);
        newEditor->setProperty("logFileName", id);
    }
}

void CVSPlugin::updateProject()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return)

   QStringList args(QLatin1String("update"));
   args.push_back(QLatin1String("-dR"));
   args.append(state.relativeCurrentProject());
   const CVSResponse response = runCVS(state.currentProjectTopLevel(), args, m_settings.longTimeOutMS(), true);
   if (response.result == CVSResponse::Ok)
       cvsVersionControl()->emitRepositoryChanged(state.currentProjectTopLevel());
}

void CVSPlugin::annotateCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    annotate(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void CVSPlugin::annotate(const QString &workingDir, const QString &file)
{
    const QStringList files(file);
    QTextCodec *codec = VCSBase::VCSBaseEditor::getCodec(workingDir, files);
    const QString id = VCSBase::VCSBaseEditor::getTitleId(workingDir, files);
    const QString source = VCSBase::VCSBaseEditor::getSource(workingDir, file);
    QStringList args;
    args << QLatin1String("annotate") << file;
    const CVSResponse response = runCVS(workingDir, args, m_settings.timeOutMS(), false, codec);
    if (response.result != CVSResponse::Ok)
        return;

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    const int lineNumber = VCSBase::VCSBaseEditor::lineNumberOfCurrentEditor(file);

    if (Core::IEditor *editor = locateEditor("annotateFileName", id)) {
        editor->createNew(response.stdOut);
        VCSBase::VCSBaseEditor::gotoLineOfEditor(editor, lineNumber);
        Core::EditorManager::instance()->activateEditor(editor);
    } else {
        const QString title = QString::fromLatin1("cvs annotate %1").arg(id);
        Core::IEditor *newEditor = showOutputInEditor(title, response.stdOut, VCSBase::AnnotateOutput, source, codec);
        newEditor->setProperty("annotateFileName", id);
        VCSBase::VCSBaseEditor::gotoLineOfEditor(newEditor, lineNumber);
    }
}

void CVSPlugin::projectStatus()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return)
    QStringList args;
    args << QLatin1String("status") << state.relativeCurrentProject();
    const CVSResponse response = runCVS(state.currentProjectTopLevel(), args, m_settings.timeOutMS(), false);
    if (response.result == CVSResponse::Ok)
        showOutputInEditor(tr("Project status"), response.stdOut, VCSBase::RegularCommandOutput, state.currentProjectTopLevel(), 0);
}

// Decrement version number "1.2" -> "1.1"
static QString previousRevision(const QString &rev)
{
    const int dotPos = rev.lastIndexOf(QLatin1Char('.'));
    if (dotPos == -1)
        return rev;
    const int minor = rev.mid(dotPos + 1).toInt();
    return rev.left(dotPos + 1) + QString::number(minor - 1);
}

// Is "[1.2...].1"?
static inline bool isFirstRevision(const QString &r)
{
    return r.endsWith(QLatin1String(".1"));
}

void CVSPlugin::slotDescribe(const QString &source, const QString &changeNr)
{
    QString errorMessage;
    if (!describe(source, changeNr, &errorMessage))
        VCSBase::VCSBaseOutputWindow::instance()->appendError(errorMessage);
}

bool CVSPlugin::describe(const QString &file, const QString &changeNr, QString *errorMessage)
{
    const QString toplevel = findTopLevelForDirectory(QFileInfo(file).absolutePath());
    if (toplevel.isEmpty()) {
        *errorMessage = msgCannotFindTopLevel(file);
        return false;
    }
    return describe(toplevel, QDir(toplevel).relativeFilePath(file), changeNr, errorMessage);
}

bool CVSPlugin::describe(const QString &toplevel, const QString &file, const
                         QString &changeNr, QString *errorMessage)
{

    // In CVS, revisions of files are normally unrelated, there is
    // no global revision/change number. The only thing that groups
    // a commit is the "commit-id" (as shown in the log).
    // This function makes use of it to find all files related to
    // a commit in order to emulate a "describe global change" functionality
    // if desired.
    if (CVS::Constants::debug)
        qDebug() << Q_FUNC_INFO << file << changeNr;
    // Number must be > 1
    if (isFirstRevision(changeNr)) {
        *errorMessage = tr("The initial revision %1 cannot be described.").arg(changeNr);
        return false;
    }
    // Run log to obtain commit id and details
    QStringList args;
    args << QLatin1String("log") << (QLatin1String("-r") + changeNr) << file;
    const CVSResponse logResponse = runCVS(toplevel, args, m_settings.timeOutMS(), false);
    if (logResponse.result != CVSResponse::Ok) {
        *errorMessage = logResponse.message;
        return false;
    }
    const QList<CVS_LogEntry> fileLog = parseLogEntries(logResponse.stdOut);
    if (fileLog.empty() || fileLog.front().revisions.empty()) {
        *errorMessage = msgLogParsingFailed();
        return false;
    }
    if (m_settings.describeByCommitId) {
        // Run a log command over the repo, filtering by the commit date
        // and commit id, collecting all files touched by the commit.
        const QString commitId = fileLog.front().revisions.front().commitId;
        // Date range "D1<D2" in ISO format "YYYY-MM-DD"
        const QString dateS = fileLog.front().revisions.front().date;
        const QDate date = QDate::fromString(dateS, Qt::ISODate);
        const QString nextDayS = date.addDays(1).toString(Qt::ISODate);
        args.clear();
        args << QLatin1String("log") << QLatin1String("-d") << (dateS  + QLatin1Char('<') + nextDayS);

        const CVSResponse repoLogResponse = runCVS(toplevel, args, m_settings.longTimeOutMS(), false);
        if (repoLogResponse.result != CVSResponse::Ok) {
            *errorMessage = repoLogResponse.message;
            return false;
        }
        // Describe all files found, pass on dir to obtain correct absolute paths.
        const QList<CVS_LogEntry> repoEntries = parseLogEntries(repoLogResponse.stdOut, QString(), commitId);
        if (repoEntries.empty()) {
            *errorMessage = tr("Could not find commits of id '%1' on %2.").arg(commitId, dateS);
            return false;
        }
        return describe(toplevel, repoEntries, errorMessage);
    } else {
        // Just describe that single entry
        return describe(toplevel, fileLog, errorMessage);
    }
    return false;
}

// Describe a set of files and revisions by
// concatenating log and diffs to previous revisions
bool CVSPlugin::describe(const QString &repositoryPath,
                         QList<CVS_LogEntry> entries,
                         QString *errorMessage)
{
    // Collect logs
    QString output;
    QTextCodec *codec = 0;
    const QList<CVS_LogEntry>::iterator lend = entries.end();
    for (QList<CVS_LogEntry>::iterator it = entries.begin(); it != lend; ++it) {
        // Before fiddling file names, try to find codec
        if (!codec)
            codec = VCSBase::VCSBaseEditor::getCodec(repositoryPath, QStringList(it->file));
        // Run log
        QStringList args(QLatin1String("log"));
        args << (QLatin1String("-r") + it->revisions.front().revision) << it->file;
        const CVSResponse logResponse = runCVS(repositoryPath, args, m_settings.timeOutMS(), false);
        if (logResponse.result != CVSResponse::Ok) {
            *errorMessage =  logResponse.message;
            return false;
        }
        output += logResponse.stdOut;
    }
    // Collect diffs relative to repository
    for (QList<CVS_LogEntry>::iterator it = entries.begin(); it != lend; ++it) {
        const QString &revision = it->revisions.front().revision;
        if (!isFirstRevision(revision)) {
            const QString previousRev = previousRevision(revision);
            QStringList args(QLatin1String("diff"));
            args << m_settings.cvsDiffOptions << QLatin1String("-r") << previousRev
                    << QLatin1String("-r") << it->revisions.front().revision
                    << it->file;
            const CVSResponse diffResponse = runCVS(repositoryPath, args, m_settings.timeOutMS(), false, codec);
            switch (diffResponse.result) {
            case CVSResponse::Ok:
            case CVSResponse::NonNullExitCode: // Diff exit code != 0
                if (diffResponse.stdOut.isEmpty()) {
                    *errorMessage = diffResponse.message;
                    return false; // Something else failed.
                }
                break;
            case CVSResponse::OtherError:
                *errorMessage = diffResponse.message;
                return false;
            }
            output += fixDiffOutput(diffResponse.stdOut);
        }
    }

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    const QString commitId = entries.front().revisions.front().commitId;
    if (Core::IEditor *editor = locateEditor("describeChange", commitId)) {
        editor->createNew(output);
        Core::EditorManager::instance()->activateEditor(editor);
        setDiffBaseDirectory(editor, repositoryPath);
    } else {
        const QString title = QString::fromLatin1("cvs describe %1").arg(commitId);
        Core::IEditor *newEditor = showOutputInEditor(title, output, VCSBase::DiffOutput, entries.front().file, codec);
        newEditor->setProperty("describeChange", commitId);
        setDiffBaseDirectory(editor, repositoryPath);
    }
    return true;
}

void CVSPlugin::submitCurrentLog()
{
    m_submitActionTriggered = true;
    Core::EditorManager::instance()->closeEditors(QList<Core::IEditor*>()
        << Core::EditorManager::instance()->currentEditor());
}

static inline QString processStdErr(QProcess &proc)
{
    return QString::fromLocal8Bit(proc.readAllStandardError()).remove(QLatin1Char('\r'));
}

// Format log entry for command
static inline QString msgExecutionLogEntry(const QString &workingDir, const QString &executable, const QStringList &arguments)
{
    //: Executing: <executable> <arguments>
    const QString args = arguments.join(QString(QLatin1Char(' ')));
    if (workingDir.isEmpty())
        return CVSPlugin::tr("Executing: %1 %2\n").arg(executable, args);
    return CVSPlugin::tr("Executing in %1: %2 %3\n").arg(workingDir, executable, args);
}

// Run CVS. At this point, file arguments must be relative to
// the working directory (see above).
CVSResponse CVSPlugin::runCVS(const QString &workingDirectory,
                              const QStringList &arguments,
                              int timeOut,
                              bool showStdOutInOutputWindow, QTextCodec *outputCodec,
                              bool mergeStderr)
{
    const QString executable = m_settings.cvsCommand;
    CVSResponse response;
    if (executable.isEmpty()) {
        response.result = CVSResponse::OtherError;
        response.message =tr("No cvs executable specified!");
        return response;
    }
    // Fix files and compile complete arguments
    const QStringList allArgs = m_settings.addOptions(arguments);

    const QString outputText = msgExecutionLogEntry(workingDirectory, executable, allArgs);
    VCSBase::VCSBaseOutputWindow::instance()->appendCommand(outputText);

    if (CVS::Constants::debug)
        qDebug() << "runCVS" << timeOut << outputText;

    // Run, connect stderr to the output window
    Utils::SynchronousProcess process;
    if (!workingDirectory.isEmpty())
        process.setWorkingDirectory(workingDirectory);

    if (mergeStderr)
        process.setProcessChannelMode(QProcess::MergedChannels);

    process.setTimeout(timeOut);
    process.setStdOutCodec(outputCodec);

    process.setStdErrBufferedSignalsEnabled(true);
    VCSBase::VCSBaseOutputWindow *outputWindow = VCSBase::VCSBaseOutputWindow::instance();
    connect(&process, SIGNAL(stdErrBuffered(QString,bool)), outputWindow, SLOT(append(QString)));

    // connect stdout to the output window if desired
    if (showStdOutInOutputWindow) {
        process.setStdOutBufferedSignalsEnabled(true);
        connect(&process, SIGNAL(stdOutBuffered(QString,bool)), outputWindow, SLOT(append(QString)));
    }

    const Utils::SynchronousProcessResponse sp_resp = process.run(executable, allArgs);
    response.result = CVSResponse::OtherError;
    response.stdErr = sp_resp.stdErr;
    response.stdOut = sp_resp.stdOut;
    switch (sp_resp.result) {
    case Utils::SynchronousProcessResponse::Finished:
        response.result = CVSResponse::Ok;
        break;
    case Utils::SynchronousProcessResponse::FinishedError:
        response.result = CVSResponse::NonNullExitCode;
        response.message = tr("The process terminated with exit code %1.").arg(sp_resp.exitCode);
        break;
    case Utils::SynchronousProcessResponse::TerminatedAbnormally:
        response.message = tr("The process terminated abnormally.");
        break;
    case Utils::SynchronousProcessResponse::StartFailed:
        response.message = tr("Could not start cvs '%1'. Please check your settings in the preferences.").arg(executable);
        break;
    case Utils::SynchronousProcessResponse::Hang:
        response.message = tr("CVS did not respond within timeout limit (%1 ms).").arg(timeOut);
        break;
    }
    if (response.result != CVSResponse::Ok)
        VCSBase::VCSBaseOutputWindow::instance()->appendError(response.message);

    return response;
}

Core::IEditor * CVSPlugin::showOutputInEditor(const QString& title, const QString &output,
                                                     int editorType, const QString &source,
                                                     QTextCodec *codec)
{
    const VCSBase::VCSBaseEditorParameters *params = findType(editorType);
    QTC_ASSERT(params, return 0);
    const QString kind = QLatin1String(params->kind);
    if (CVS::Constants::debug)
        qDebug() << "CVSPlugin::showOutputInEditor" << title << kind <<  "source=" << source << "Size= " << output.size() <<  " Type=" << editorType << debugCodec(codec);
    QString s = title;
    Core::IEditor *editor = Core::EditorManager::instance()->openEditorWithContents(kind, &s, output.toLocal8Bit());
    CVSEditor *e = qobject_cast<CVSEditor*>(editor->widget());
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

CVSSettings CVSPlugin::settings() const
{
    return m_settings;
}

void CVSPlugin::setSettings(const CVSSettings &s)
{
    if (s != m_settings) {
        m_settings = s;
        if (QSettings *settings = Core::ICore::instance()->settings())
            m_settings.toSettings(settings);
    }
}

CVSPlugin *CVSPlugin::cvsPluginInstance()
{
    QTC_ASSERT(m_cvsPluginInstance, return m_cvsPluginInstance);
    return m_cvsPluginInstance;
}

bool CVSPlugin::vcsAdd(const QString &workingDir, const QString &rawFileName)
{
    QStringList args;
    args << QLatin1String("add") << rawFileName;
    const CVSResponse response = runCVS(workingDir, args, m_settings.timeOutMS(), true);
    return response.result == CVSResponse::Ok;
}

bool CVSPlugin::vcsDelete(const QString &workingDir, const QString &rawFileName)
{
    QStringList args;
    args << QLatin1String("remove") << QLatin1String("-f") << rawFileName;
    const CVSResponse response = runCVS(workingDir, args, m_settings.timeOutMS(), true);
    return response.result == CVSResponse::Ok;
}

/* CVS has a "CVS" directory in each directory it manages. The top level
 * is the first directory under the directory that does not have it. */
bool CVSPlugin::managesDirectory(const QString &directory) const
{
    const QDir dir(directory);
    const bool rc = dir.exists() && managesDirectory(dir);
    if (CVS::Constants::debug)
        qDebug() << "CVSPlugin::managesDirectory" << directory << rc;
    return rc;
}

bool CVSPlugin::managesDirectory(const QDir &directory) const
{
    const QString cvsDir = directory.absoluteFilePath(QLatin1String("CVS"));
    return QFileInfo(cvsDir).isDir();
}

QString CVSPlugin::findTopLevelForDirectory(const QString &directory) const
{
    // Debug wrapper
    const QString rc = findTopLevelForDirectoryI(directory);
    if (CVS::Constants::debug)
        qDebug() << "CVSPlugin::findTopLevelForDirectory" << directory << rc;
    return rc;
}

QString CVSPlugin::findTopLevelForDirectoryI(const QString &directory) const
{
    /* Recursing up, the top level is a child of the first directory that does
     * not have a  "CVS" directory. The starting directory must be a managed
     * one. Go up and try to find the first unmanaged parent dir. */
    QDir lastDirectory = QDir(directory);
    if (!lastDirectory.exists() || !managesDirectory(lastDirectory))
        return QString();
    for (QDir parentDir = lastDirectory; parentDir.cdUp() ; lastDirectory = parentDir) {
        if (!managesDirectory(parentDir))
            return lastDirectory.absolutePath();
    }
    return QString();
}

CVSControl *CVSPlugin::cvsVersionControl() const
{
    return static_cast<CVSControl *>(versionControl());
}

}
}
Q_EXPORT_PLUGIN(CVS::Internal::CVSPlugin)
