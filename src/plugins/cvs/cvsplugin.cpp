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
#include <vcsbase/vcsbaseeditorparameterwidget.h>
#include <locator/commandlocator.h>
#include <utils/synchronousprocess.h>
#include <utils/parameteraction.h>
#include <utils/qtcassert.h>

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/vcsmanager.h>
#include <utils/stringutils.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QDate>
#include <QDir>
#include <QFileInfo>
#include <QTextCodec>
#include <QtPlugin>
#include <QAction>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>

#ifdef WITH_TESTS
#include <QTest>
#endif

using namespace VcsBase;
using namespace Core;

namespace Cvs {
namespace Internal {

static inline QString msgCannotFindTopLevel(const QString &f)
{
    return CvsPlugin::tr("Cannot find repository for '%1'").
            arg(QDir::toNativeSeparators(f));
}

static inline QString msgLogParsingFailed()
{
    return CvsPlugin::tr("Parsing of the log output failed");
}

static const char CMD_ID_CVS_MENU[]           = "CVS.Menu";
static const char CMD_ID_ADD[]                = "CVS.Add";
static const char CMD_ID_DELETE_FILE[]        = "CVS.Delete";
static const char CMD_ID_EDIT_FILE[]          = "CVS.EditFile";
static const char CMD_ID_UNEDIT_FILE[]        = "CVS.UneditFile";
static const char CMD_ID_UNEDIT_REPOSITORY[]  = "CVS.UneditRepository";
static const char CMD_ID_REVERT[]             = "CVS.Revert";
static const char CMD_ID_DIFF_PROJECT[]       = "CVS.DiffAll";
static const char CMD_ID_DIFF_CURRENT[]       = "CVS.DiffCurrent";
static const char CMD_ID_COMMIT_ALL[]         = "CVS.CommitAll";
static const char CMD_ID_REVERT_ALL[]         = "CVS.RevertAll";
static const char CMD_ID_COMMIT_CURRENT[]     = "CVS.CommitCurrent";
static const char CMD_ID_FILELOG_CURRENT[]    = "CVS.FilelogCurrent";
static const char CMD_ID_ANNOTATE_CURRENT[]   = "CVS.AnnotateCurrent";
static const char CMD_ID_STATUS[]             = "CVS.Status";
static const char CMD_ID_UPDATE[]             = "CVS.Update";
static const char CMD_ID_PROJECTLOG[]         = "CVS.ProjectLog";
static const char CMD_ID_PROJECTCOMMIT[]      = "CVS.ProjectCommit";
static const char CMD_ID_REPOSITORYLOG[]      = "CVS.RepositoryLog";
static const char CMD_ID_REPOSITORYDIFF[]     = "CVS.RepositoryDiff";
static const char CMD_ID_REPOSITORYSTATUS[]   = "CVS.RepositoryStatus";
static const char CMD_ID_REPOSITORYUPDATE[]   = "CVS.RepositoryUpdate";

static const VcsBaseEditorParameters editorParameters[] = {
{
    RegularCommandOutput,
    "CVS Command Log Editor", // id
    QT_TRANSLATE_NOOP("VCS", "CVS Command Log Editor"), // display name
    "CVS Command Log Editor", // context
    "application/vnd.nokia.text.scs_cvs_commandlog",
    "scslog"},
{   LogOutput,
    "CVS File Log Editor",   // id
    QT_TRANSLATE_NOOP("VCS", "CVS File Log Editor"),   // display name
    "CVS File Log Editor",   // context
    "application/vnd.nokia.text.scs_cvs_filelog",
    "scsfilelog"},
{    AnnotateOutput,
    "CVS Annotation Editor",  // id
    QT_TRANSLATE_NOOP("VCS", "CVS Annotation Editor"),  // display name
    "CVS Annotation Editor",  // context
    "application/vnd.nokia.text.scs_cvs_annotation",
    "scsannotate"},
{   DiffOutput,
    "CVS Diff Editor",  // id
    QT_TRANSLATE_NOOP("VCS", "CVS Diff Editor"),  // display name
    "CVS Diff Editor",  // context
    "text/x-patch","diff"}
};

// Utility to find a parameter set by type
static inline const VcsBaseEditorParameters *findType(int ie)
{
    const EditorContentType et = static_cast<EditorContentType>(ie);
    return  VcsBaseEditorWidget::findType(editorParameters, sizeof(editorParameters)/sizeof(VcsBaseEditorParameters), et);
}

static inline QString debugCodec(const QTextCodec *c)
{
    return c ? QString::fromLatin1(c->name()) : QString::fromLatin1("Null codec");
}

static inline bool messageBoxQuestion(const QString &title, const QString &question, QWidget *parent = 0)
{
    return QMessageBox::question(parent, title, question, QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes;
}

// ------------- CVSPlugin
CvsPlugin *CvsPlugin::m_cvsPluginInstance = 0;

CvsPlugin::CvsPlugin() :
    VcsBasePlugin(QLatin1String(Constants::CVSCOMMITEDITOR_ID)),
    m_commandLocator(0),
    m_addAction(0),
    m_deleteAction(0),
    m_revertAction(0),
    m_editCurrentAction(0),
    m_uneditCurrentAction(0),
    m_uneditRepositoryAction(0),
    m_diffProjectAction(0),
    m_diffCurrentAction(0),
    m_logProjectAction(0),
    m_logRepositoryAction(0),
    m_commitAllAction(0),
    m_revertRepositoryAction(0),
    m_commitCurrentAction(0),
    m_filelogCurrentAction(0),
    m_annotateCurrentAction(0),
    m_statusProjectAction(0),
    m_updateProjectAction(0),
    m_commitProjectAction(0),
    m_diffRepositoryAction(0),
    m_updateRepositoryAction(0),
    m_statusRepositoryAction(0),
    m_submitCurrentLogAction(0),
    m_submitDiffAction(0),
    m_submitUndoAction(0),
    m_submitRedoAction(0),
    m_menuAction(0),
    m_submitActionTriggered(false)
{
}

CvsPlugin::~CvsPlugin()
{
    cleanCommitMessageFile();
}

void CvsPlugin::cleanCommitMessageFile()
{
    if (!m_commitMessageFileName.isEmpty()) {
        QFile::remove(m_commitMessageFileName);
        m_commitMessageFileName.clear();
        m_commitRepository.clear();
    }
}
bool CvsPlugin::isCommitEditorOpen() const
{
    return !m_commitMessageFileName.isEmpty();
}

static const VcsBaseSubmitEditorParameters submitParameters = {
    Constants::CVS_SUBMIT_MIMETYPE,
    Constants::CVSCOMMITEDITOR_ID,
    Constants::CVSCOMMITEDITOR_DISPLAY_NAME,
    Constants::CVSCOMMITEDITOR,
    VcsBase::VcsBaseSubmitEditorParameters::DiffFiles
};

bool CvsPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    typedef VcsSubmitEditorFactory<CvsSubmitEditor> CVSSubmitEditorFactory;
    typedef VcsEditorFactory<CvsEditor> CVSEditorFactory;
    using namespace Constants;

    using namespace Core::Constants;
    using namespace ExtensionSystem;

    initializeVcs(new CvsControl(this));

    m_cvsPluginInstance = this;

    if (!ICore::mimeDatabase()->addMimeTypes(QLatin1String(":/trolltech.cvs/CVS.mimetypes.xml"), errorMessage))
        return false;

    if (QSettings *settings = ICore::settings())
        m_settings.fromSettings(settings);

    addAutoReleasedObject(new SettingsPage);

    addAutoReleasedObject(new CVSSubmitEditorFactory(&submitParameters));

    static const char *describeSlotC = SLOT(slotDescribe(QString,QString));
    const int editorCount = sizeof(editorParameters) / sizeof(editorParameters[0]);
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new CVSEditorFactory(editorParameters + i, this, describeSlotC));

    addAutoReleasedObject(new CheckoutWizard);

    const QString prefix = QLatin1String("cvs");
    m_commandLocator = new Locator::CommandLocator("CVS", prefix, prefix);
    addAutoReleasedObject(m_commandLocator);

    // Register actions
    ActionContainer *toolsContainer = Core::ActionManager::actionContainer(M_TOOLS);

    ActionContainer *cvsMenu = Core::ActionManager::createMenu(Id(CMD_ID_CVS_MENU));
    cvsMenu->menu()->setTitle(tr("&CVS"));
    toolsContainer->addMenu(cvsMenu);
    m_menuAction = cvsMenu->menu()->menuAction();

    Context globalcontext(C_GLOBAL);

    Command *command;

    m_diffCurrentAction = new Utils::ParameterAction(tr("Diff Current File"), tr("Diff \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_diffCurrentAction,
        CMD_ID_DIFF_CURRENT, globalcontext);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+C,Meta+D") : tr("Alt+C,Alt+D")));
    connect(m_diffCurrentAction, SIGNAL(triggered()), this, SLOT(diffCurrentFile()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_filelogCurrentAction = new Utils::ParameterAction(tr("Filelog Current File"), tr("Filelog \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_filelogCurrentAction,
        CMD_ID_FILELOG_CURRENT, globalcontext);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_filelogCurrentAction, SIGNAL(triggered()), this,
        SLOT(filelogCurrentFile()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_annotateCurrentAction = new Utils::ParameterAction(tr("Annotate Current File"), tr("Annotate \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_annotateCurrentAction,
        CMD_ID_ANNOTATE_CURRENT, globalcontext);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_annotateCurrentAction, SIGNAL(triggered()), this,
        SLOT(annotateCurrentFile()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    cvsMenu->addSeparator(globalcontext);

    m_addAction = new Utils::ParameterAction(tr("Add"), tr("Add \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_addAction, CMD_ID_ADD,
        globalcontext);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+C,Meta+A") : tr("Alt+C,Alt+A")));
    connect(m_addAction, SIGNAL(triggered()), this, SLOT(addCurrentFile()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_commitCurrentAction = new Utils::ParameterAction(tr("Commit Current File"), tr("Commit \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_commitCurrentAction,
        CMD_ID_COMMIT_CURRENT, globalcontext);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+C,Meta+C") : tr("Alt+C,Alt+C")));
    connect(m_commitCurrentAction, SIGNAL(triggered()), this, SLOT(startCommitCurrentFile()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_deleteAction = new Utils::ParameterAction(tr("Delete..."), tr("Delete \"%1\"..."), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_deleteAction, CMD_ID_DELETE_FILE,
        globalcontext);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_deleteAction, SIGNAL(triggered()), this, SLOT(promptToDeleteCurrentFile()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertAction = new Utils::ParameterAction(tr("Revert..."), tr("Revert \"%1\"..."), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_revertAction, CMD_ID_REVERT,
        globalcontext);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_revertAction, SIGNAL(triggered()), this, SLOT(revertCurrentFile()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    cvsMenu->addSeparator(globalcontext);

    m_editCurrentAction = new Utils::ParameterAction(tr("Edit"), tr("Edit \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_editCurrentAction, CMD_ID_EDIT_FILE, globalcontext);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_editCurrentAction, SIGNAL(triggered()), this, SLOT(editCurrentFile()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_uneditCurrentAction = new Utils::ParameterAction(tr("Unedit"), tr("Unedit \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_uneditCurrentAction, CMD_ID_UNEDIT_FILE, globalcontext);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_uneditCurrentAction, SIGNAL(triggered()), this, SLOT(uneditCurrentFile()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_uneditRepositoryAction = new QAction(tr("Unedit Repository"), this);
    command = Core::ActionManager::registerAction(m_uneditRepositoryAction, CMD_ID_UNEDIT_REPOSITORY, globalcontext);
    connect(m_uneditRepositoryAction, SIGNAL(triggered()), this, SLOT(uneditCurrentRepository()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    cvsMenu->addSeparator(globalcontext);

    m_diffProjectAction = new Utils::ParameterAction(tr("Diff Project"), tr("Diff Project \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_diffProjectAction, CMD_ID_DIFF_PROJECT,
        globalcontext);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_diffProjectAction, SIGNAL(triggered()), this, SLOT(diffProject()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_statusProjectAction = new Utils::ParameterAction(tr("Project Status"), tr("Status of Project \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_statusProjectAction, CMD_ID_STATUS,
        globalcontext);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_statusProjectAction, SIGNAL(triggered()), this, SLOT(projectStatus()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logProjectAction = new Utils::ParameterAction(tr("Log Project"), tr("Log Project \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_logProjectAction, CMD_ID_PROJECTLOG, globalcontext);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_logProjectAction, SIGNAL(triggered()), this, SLOT(logProject()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_updateProjectAction = new Utils::ParameterAction(tr("Update Project"), tr("Update Project \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_updateProjectAction, CMD_ID_UPDATE, globalcontext);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_updateProjectAction, SIGNAL(triggered()), this, SLOT(updateProject()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_commitProjectAction = new Utils::ParameterAction(tr("Commit Project"), tr("Commit Project \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_commitProjectAction, CMD_ID_PROJECTCOMMIT, globalcontext);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_commitProjectAction, SIGNAL(triggered()), this, SLOT(commitProject()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    cvsMenu->addSeparator(globalcontext);

    m_diffRepositoryAction = new QAction(tr("Diff Repository"), this);
    command = Core::ActionManager::registerAction(m_diffRepositoryAction, CMD_ID_REPOSITORYDIFF, globalcontext);
    connect(m_diffRepositoryAction, SIGNAL(triggered()), this, SLOT(diffRepository()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_statusRepositoryAction = new QAction(tr("Repository Status"), this);
    command = Core::ActionManager::registerAction(m_statusRepositoryAction, CMD_ID_REPOSITORYSTATUS, globalcontext);
    connect(m_statusRepositoryAction, SIGNAL(triggered()), this, SLOT(statusRepository()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logRepositoryAction = new QAction(tr("Repository Log"), this);
    command = Core::ActionManager::registerAction(m_logRepositoryAction, CMD_ID_REPOSITORYLOG, globalcontext);
    connect(m_logRepositoryAction, SIGNAL(triggered()), this, SLOT(logRepository()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_updateRepositoryAction = new QAction(tr("Update Repository"), this);
    command = Core::ActionManager::registerAction(m_updateRepositoryAction, CMD_ID_REPOSITORYUPDATE, globalcontext);
    connect(m_updateRepositoryAction, SIGNAL(triggered()), this, SLOT(updateRepository()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_commitAllAction = new QAction(tr("Commit All Files"), this);
    command = Core::ActionManager::registerAction(m_commitAllAction, CMD_ID_COMMIT_ALL,
        globalcontext);
    connect(m_commitAllAction, SIGNAL(triggered()), this, SLOT(startCommitAll()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertRepositoryAction = new QAction(tr("Revert Repository..."), this);
    command = Core::ActionManager::registerAction(m_revertRepositoryAction, CMD_ID_REVERT_ALL,
                                  globalcontext);
    connect(m_revertRepositoryAction, SIGNAL(triggered()), this, SLOT(revertAll()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    // Actions of the submit editor
    Context cvscommitcontext(Constants::CVSCOMMITEDITOR);

    m_submitCurrentLogAction = new QAction(VcsBaseSubmitEditor::submitIcon(), tr("Commit"), this);
    command = Core::ActionManager::registerAction(m_submitCurrentLogAction, Constants::SUBMIT_CURRENT, cvscommitcontext);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_submitCurrentLogAction, SIGNAL(triggered()), this, SLOT(submitCurrentLog()));

    m_submitDiffAction = new QAction(VcsBaseSubmitEditor::diffIcon(), tr("Diff &Selected Files"), this);
    command = Core::ActionManager::registerAction(m_submitDiffAction , Constants::DIFF_SELECTED, cvscommitcontext);

    m_submitUndoAction = new QAction(tr("&Undo"), this);
    command = Core::ActionManager::registerAction(m_submitUndoAction, Core::Constants::UNDO, cvscommitcontext);

    m_submitRedoAction = new QAction(tr("&Redo"), this);
    command = Core::ActionManager::registerAction(m_submitRedoAction, Core::Constants::REDO, cvscommitcontext);
    return true;
}

bool CvsPlugin::submitEditorAboutToClose(VcsBaseSubmitEditor *submitEditor)
{
    if (!isCommitEditorOpen())
        return true;

    IDocument *editorDocument = submitEditor->document();
    const CvsSubmitEditor *editor = qobject_cast<CvsSubmitEditor *>(submitEditor);
    if (!editorDocument || !editor)
        return true;

    // Submit editor closing. Make it write out the commit message
    // and retrieve files
    const QFileInfo editorFile(editorDocument->fileName());
    const QFileInfo changeFile(m_commitMessageFileName);
    if (editorFile.absoluteFilePath() != changeFile.absoluteFilePath())
        return true; // Oops?!

    // Prompt user. Force a prompt unless submit was actually invoked (that
    // is, the editor was closed or shutdown).
    CvsSettings newSettings = m_settings;
    const VcsBaseSubmitEditor::PromptSubmitResult answer =
            editor->promptSubmit(tr("Closing CVS Editor"),
                                 tr("Do you want to commit the change?"),
                                 tr("The commit message check failed. Do you want to commit the change?"),
                                 &newSettings.promptToSubmit, !m_submitActionTriggered);
    m_submitActionTriggered = false;
    switch (answer) {
    case VcsBaseSubmitEditor::SubmitCanceled:
        return false; // Keep editing and change file
    case VcsBaseSubmitEditor::SubmitDiscarded:
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
        closeEditor = DocumentManager::saveDocument(editorDocument);
        if (closeEditor)
            closeEditor = commit(m_commitMessageFileName, fileList);
    }
    if (closeEditor)
        cleanCommitMessageFile();
    return closeEditor;
}

void CvsPlugin::diffCommitFiles(const QStringList &files)
{
    cvsDiff(m_commitRepository, files);
}

static void setDiffBaseDirectory(IEditor *editor, const QString &db)
{
    if (VcsBaseEditorWidget *ve = qobject_cast<VcsBaseEditorWidget*>(editor->widget()))
        ve->setDiffBaseDirectory(db);
}

// Collect all parameters required for a diff to be able to associate them
// with a diff editor and re-run the diff with parameters.
struct CvsDiffParameters
{
    QString workingDir;
    QStringList arguments;
    QStringList files;
};

// Parameter widget controlling whitespace diff mode, associated with a parameter
// struct.
class CvsDiffParameterWidget : public VcsBaseEditorParameterWidget
{
    Q_OBJECT

public:
    explicit CvsDiffParameterWidget(const CvsDiffParameters &p, QWidget *parent = 0);

signals:
    void reRunDiff(const Cvs::Internal::CvsDiffParameters &);

public slots:
    void triggerReRun();

private:
    const CvsDiffParameters m_parameters;
};

CvsDiffParameterWidget::CvsDiffParameterWidget(const CvsDiffParameters &p, QWidget *parent) :
    VcsBaseEditorParameterWidget(parent), m_parameters(p)
{
    setBaseArguments(p.arguments);
    addToggleButton(QLatin1String("-w"), tr("Ignore whitespace"));
    addToggleButton(QLatin1String("-B"), tr("Ignore blank lines"));
    connect(this, SIGNAL(argumentsChanged()),
            this, SLOT(triggerReRun()));
}

void CvsDiffParameterWidget::triggerReRun()
{
    CvsDiffParameters effectiveParameters = m_parameters;
    effectiveParameters.arguments = arguments();
    emit reRunDiff(effectiveParameters);
}

void CvsPlugin::cvsDiff(const QString &workingDir, const QStringList &files)
{
    CvsDiffParameters p;
    p.workingDir = workingDir;
    p.files = files;
    p.arguments = m_settings.cvsDiffOptions.split(QLatin1Char(' '), QString::SkipEmptyParts);
    cvsDiff(p);
}

void CvsPlugin::cvsDiff(const CvsDiffParameters &p)
{
    if (Cvs::Constants::debug)
        qDebug() << Q_FUNC_INFO << p.files;
    const QString source = VcsBaseEditorWidget::getSource(p.workingDir, p.files);
    QTextCodec *codec = VcsBaseEditorWidget::getCodec(p.workingDir, p.files);
    const QString id = VcsBaseEditorWidget::getTitleId(p.workingDir, p.files);

    QStringList args(QLatin1String("diff"));
    args.append(p.arguments);
    args.append(p.files);

    // CVS returns the diff exit code (1 if files differ), which is
    // undistinguishable from a "file not found" error, unfortunately.
    const CvsResponse response =
            runCvs(p.workingDir, args, m_settings.timeOutMS(), 0, codec);
    switch (response.result) {
    case CvsResponse::NonNullExitCode:
    case CvsResponse::Ok:
        break;
    case CvsResponse::OtherError:
        return;
    }

    QString output = fixDiffOutput(response.stdOut);
    if (output.isEmpty())
        output = tr("The files do not differ.");
    // diff of a single file? re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    // Show in the same editor if diff has been executed before
    const QString tag = VcsBaseEditorWidget::editorTag(DiffOutput, p.workingDir, p.files);
    if (IEditor *existingEditor = VcsBaseEditorWidget::locateEditorByTag(tag)) {
        existingEditor->createNew(output);
        EditorManager::activateEditor(existingEditor, EditorManager::ModeSwitch);
        setDiffBaseDirectory(existingEditor, p.workingDir);
        return;
    }
    const QString title = QString::fromLatin1("cvs diff %1").arg(id);
    IEditor *editor = showOutputInEditor(title, output, DiffOutput, source, codec);
    VcsBaseEditorWidget::tagEditor(editor, tag);
    setDiffBaseDirectory(editor, p.workingDir);
    CvsEditor *diffEditorWidget = qobject_cast<CvsEditor*>(editor->widget());
    QTC_ASSERT(diffEditorWidget, return);

    // Wire up the parameter widget to trigger a re-run on
    // parameter change and 'revert' from inside the diff editor.
    CvsDiffParameterWidget *pw = new CvsDiffParameterWidget(p);
    connect(pw, SIGNAL(reRunDiff(Cvs::Internal::CvsDiffParameters)),
            this, SLOT(cvsDiff(Cvs::Internal::CvsDiffParameters)));
    connect(diffEditorWidget, SIGNAL(diffChunkReverted(VcsBase::DiffChunk)),
            pw, SLOT(triggerReRun()));
    diffEditorWidget->setConfigurationWidget(pw);
}

CvsSubmitEditor *CvsPlugin::openCVSSubmitEditor(const QString &fileName)
{
    IEditor *editor = EditorManager::openEditor(fileName, Constants::CVSCOMMITEDITOR_ID,
                                                EditorManager::ModeSwitch);
    CvsSubmitEditor *submitEditor = qobject_cast<CvsSubmitEditor*>(editor);
    QTC_CHECK(submitEditor);
    submitEditor->registerActions(m_submitUndoAction, m_submitRedoAction, m_submitCurrentLogAction, m_submitDiffAction);
    connect(submitEditor, SIGNAL(diffSelectedFiles(QStringList)), this, SLOT(diffCommitFiles(QStringList)));

    return submitEditor;
}

void CvsPlugin::updateActions(VcsBasePlugin::ActionState as)
{
    if (!enableMenuAction(as, m_menuAction)) {
        m_commandLocator->setEnabled(false);
        return;
    }

    const bool hasTopLevel = currentState().hasTopLevel();
    m_commandLocator->setEnabled(hasTopLevel);

    const QString currentFileName = currentState().currentFileName();
    m_addAction->setParameter(currentFileName);
    m_deleteAction->setParameter(currentFileName);
    m_revertAction->setParameter(currentFileName);
    m_diffCurrentAction->setParameter(currentFileName);
    m_commitCurrentAction->setParameter(currentFileName);
    m_filelogCurrentAction->setParameter(currentFileName);
    m_annotateCurrentAction->setParameter(currentFileName);
    m_editCurrentAction->setParameter(currentFileName);
    m_uneditCurrentAction->setParameter(currentFileName);

    const QString currentProjectName = currentState().currentProjectName();
    m_diffProjectAction->setParameter(currentProjectName);
    m_statusProjectAction->setParameter(currentProjectName);
    m_updateProjectAction->setParameter(currentProjectName);
    m_logProjectAction->setParameter(currentProjectName);
    m_commitProjectAction->setParameter(currentProjectName);

    m_diffRepositoryAction->setEnabled(hasTopLevel);
    m_statusRepositoryAction->setEnabled(hasTopLevel);
    m_updateRepositoryAction->setEnabled(hasTopLevel);
    m_commitAllAction->setEnabled(hasTopLevel);
    m_logRepositoryAction->setEnabled(hasTopLevel);
    m_uneditRepositoryAction->setEnabled(hasTopLevel);
}

void CvsPlugin::addCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    vcsAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void CvsPlugin::revertAll()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    const QString title = tr("Revert repository");
    if (!messageBoxQuestion(title, tr("Revert all pending changes to the repository?")))
        return;
    QStringList args;
    args << QLatin1String("update") << QLatin1String("-C") << state.topLevel();
    const CvsResponse revertResponse =
            runCvs(state.topLevel(), args, m_settings.timeOutMS(),
                   SshPasswordPrompt|ShowStdOutInLogWindow);
    if (revertResponse.result == CvsResponse::Ok)
        cvsVersionControl()->emitRepositoryChanged(state.topLevel());
    else
        QMessageBox::warning(0, title, tr("Revert failed: %1").arg(revertResponse.message), QMessageBox::Ok);
}

void CvsPlugin::revertCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    QStringList args;
    args << QLatin1String("diff") << state.relativeCurrentFile();
    const CvsResponse diffResponse =
            runCvs(state.currentFileTopLevel(), args, m_settings.timeOutMS(), 0);
    switch (diffResponse.result) {
    case CvsResponse::Ok:
        return; // Not modified, diff exit code 0
    case CvsResponse::NonNullExitCode: // Diff exit code != 0
        if (diffResponse.stdOut.isEmpty()) // Paranoia: Something else failed?
            return;
        break;
    case CvsResponse::OtherError:
        return;
    }

    if (!messageBoxQuestion(QLatin1String("CVS Revert"),
                            tr("The file has been changed. Do you want to revert it?")))
        return;

    FileChangeBlocker fcb(state.currentFile());

    // revert
    args.clear();
    args << QLatin1String("update") << QLatin1String("-C") << state.relativeCurrentFile();
    const CvsResponse revertResponse =
            runCvs(state.currentFileTopLevel(), args, m_settings.timeOutMS(),
                   SshPasswordPrompt|ShowStdOutInLogWindow);
    if (revertResponse.result == CvsResponse::Ok)
        cvsVersionControl()->emitFilesChanged(QStringList(state.currentFile()));
}

void CvsPlugin::diffProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    cvsDiff(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void CvsPlugin::diffCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    cvsDiff(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void CvsPlugin::startCommitCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    startCommit(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void CvsPlugin::startCommitAll()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    startCommit(state.topLevel());
}

/* Start commit of files of a single repository by displaying
 * template and files in a submit editor. On closing, the real
 * commit will start. */
void CvsPlugin::startCommit(const QString &workingDir, const QStringList &files)
{
    if (VcsBaseSubmitEditor::raiseSubmitEditor())
        return;
    if (isCommitEditorOpen()) {
        VcsBaseOutputWindow::instance()->appendWarning(tr("Another commit is currently being executed."));
        return;
    }

    // We need the "Examining <subdir>" stderr output to tell
    // where we are, so, have stdout/stderr channels merged.
    QStringList args = QStringList(QLatin1String("status"));
    const CvsResponse response =
            runCvs(workingDir, args, m_settings.timeOutMS(), MergeOutputChannels);
    if (response.result != CvsResponse::Ok)
        return;
    // Get list of added/modified/deleted files and purge out undesired ones
    // (do not run status with relative arguments as it will omit the directories)
    StateList statusOutput = parseStatusOutput(QString(), response.stdOut);
    if (!files.isEmpty()) {
        for (StateList::iterator it = statusOutput.begin(); it != statusOutput.end() ; ) {
            if (files.contains(it->second))
                ++it;
            else
                it = statusOutput.erase(it);
        }
    }
    if (statusOutput.empty()) {
        VcsBaseOutputWindow::instance()->append(tr("There are no modified files."));
        return;
    }
    m_commitRepository = workingDir;

    // Create a new submit change file containing the submit template
    Utils::TempFileSaver saver;
    saver.setAutoRemove(false);
    // TODO: Retrieve submit template from
    const QString submitTemplate;
    // Create a submit
    saver.write(submitTemplate.toUtf8());
    if (!saver.finalize()) {
        VcsBaseOutputWindow::instance()->appendError(saver.errorString());
        return;
    }
    m_commitMessageFileName = saver.fileName();
    // Create a submit editor and set file list
    CvsSubmitEditor *editor = openCVSSubmitEditor(m_commitMessageFileName);
    editor->setCheckScriptWorkingDirectory(m_commitRepository);
    editor->setStateList(statusOutput);
}

bool CvsPlugin::commit(const QString &messageFile,
                              const QStringList &fileList)
{
    if (Cvs::Constants::debug)
        qDebug() << Q_FUNC_INFO << messageFile << fileList;
    QStringList args = QStringList(QLatin1String("commit"));
    args << QLatin1String("-F") << messageFile;
    args.append(fileList);
    const CvsResponse response =
            runCvs(m_commitRepository, args, m_settings.longTimeOutMS(),
                   SshPasswordPrompt|ShowStdOutInLogWindow);
    return response.result == CvsResponse::Ok ;
}

void CvsPlugin::filelogCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    filelog(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()), true);
}

void CvsPlugin::logProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    filelog(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void CvsPlugin::logRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    filelog(state.topLevel());
}

void CvsPlugin::filelog(const QString &workingDir,
                        const QStringList &files,
                        bool enableAnnotationContextMenu)
{
    QTextCodec *codec = VcsBaseEditorWidget::getCodec(workingDir, files);
    // no need for temp file
    const QString id = VcsBaseEditorWidget::getTitleId(workingDir, files);
    const QString source = VcsBaseEditorWidget::getSource(workingDir, files);
    QStringList args;
    args << QLatin1String("log");
    args.append(files);
    const CvsResponse response =
            runCvs(workingDir, args, m_settings.timeOutMS(),
                   SshPasswordPrompt, codec);
    if (response.result != CvsResponse::Ok)
        return;

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    const QString tag = VcsBaseEditorWidget::editorTag(LogOutput, workingDir, files);
    if (Core::IEditor *editor = VcsBaseEditorWidget::locateEditorByTag(tag)) {
        editor->createNew(response.stdOut);
        Core::EditorManager::activateEditor(editor, Core::EditorManager::ModeSwitch);
    } else {
        const QString title = QString::fromLatin1("cvs log %1").arg(id);
        Core::IEditor *newEditor = showOutputInEditor(title, response.stdOut, LogOutput, source, codec);
        VcsBaseEditorWidget::tagEditor(newEditor, tag);
        if (enableAnnotationContextMenu)
            VcsBaseEditorWidget::getVcsBaseEditor(newEditor)->setFileLogAnnotateEnabled(true);
    }
}

void CvsPlugin::updateProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    update(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

bool CvsPlugin::update(const QString &topLevel, const QStringList &files)
{
    QStringList args(QLatin1String("update"));
    args.push_back(QLatin1String("-dR"));
    args.append(files);
    const CvsResponse response =
            runCvs(topLevel, args, m_settings.longTimeOutMS(),
                   SshPasswordPrompt|ShowStdOutInLogWindow);
    const bool ok = response.result == CvsResponse::Ok;
    if (ok)
        cvsVersionControl()->emitRepositoryChanged(topLevel);
    return ok;
}

void CvsPlugin::editCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    edit(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void CvsPlugin::uneditCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    unedit(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void CvsPlugin::uneditCurrentRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    unedit(state.topLevel(), QStringList());
}

void CvsPlugin::annotateCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    annotate(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void CvsPlugin::vcsAnnotate(const QString &file, const QString &revision, int lineNumber)
{
    const QFileInfo fi(file);
    annotate(fi.absolutePath(), fi.fileName(), revision, lineNumber);
}

bool CvsPlugin::edit(const QString &topLevel, const QStringList &files)
{
    QStringList args(QLatin1String("edit"));
    args.append(files);
    const CvsResponse response =
            runCvs(topLevel, args, m_settings.timeOutMS(),
                   ShowStdOutInLogWindow|SshPasswordPrompt);
    return response.result == CvsResponse::Ok;
}

bool CvsPlugin::diffCheckModified(const QString &topLevel, const QStringList &files, bool *modified)
{
    // Quick check for modified files using diff
    *modified = false;
    QStringList args(QLatin1String("-q"));
    args << QLatin1String("diff");
    args.append(files);
    const CvsResponse response = runCvs(topLevel, args, m_settings.timeOutMS(), 0);
    if (response.result == CvsResponse::OtherError)
        return false;
    *modified = response.result == CvsResponse::NonNullExitCode;
    return true;
}

bool CvsPlugin::unedit(const QString &topLevel, const QStringList &files)
{
    bool modified;
    // Prompt and use force flag if modified
    if (!diffCheckModified(topLevel, files, &modified))
        return false;
    if (modified) {
        const QString question = files.isEmpty() ?
                      tr("Would you like to discard your changes to the repository '%1'?").arg(topLevel) :
                      tr("Would you like to discard your changes to the file '%1'?").arg(files.front());
        if (!messageBoxQuestion(tr("Unedit"), question))
            return false;
    }

    QStringList args(QLatin1String("unedit"));
    // Note: Option '-y' to force 'yes'-answer to CVS' 'undo change' prompt,
    // exists in CVSNT only as of 6.8.2010. Standard CVS will otherwise prompt
    if (modified)
        args.append(QLatin1String("-y"));
    args.append(files);
    const CvsResponse response =
            runCvs(topLevel, args, m_settings.timeOutMS(),
                   ShowStdOutInLogWindow|SshPasswordPrompt);
    return response.result == CvsResponse::Ok;
}

void CvsPlugin::annotate(const QString &workingDir, const QString &file,
                         const QString &revision /* = QString() */,
                         int lineNumber /* = -1 */)
{
    const QStringList files(file);
    QTextCodec *codec = VcsBaseEditorWidget::getCodec(workingDir, files);
    const QString id = VcsBaseEditorWidget::getTitleId(workingDir, files, revision);
    const QString source = VcsBaseEditorWidget::getSource(workingDir, file);
    QStringList args;
    args << QLatin1String("annotate");
    if (!revision.isEmpty())
        args << QLatin1String("-r") << revision;
    args << file;
    const CvsResponse response =
            runCvs(workingDir, args, m_settings.timeOutMS(),
                   SshPasswordPrompt, codec);
    if (response.result != CvsResponse::Ok)
        return;

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    if (lineNumber < 1)
        lineNumber = VcsBaseEditorWidget::lineNumberOfCurrentEditor(file);

    const QString tag = VcsBaseEditorWidget::editorTag(AnnotateOutput, workingDir, QStringList(file), revision);
    if (IEditor *editor = VcsBaseEditorWidget::locateEditorByTag(tag)) {
        editor->createNew(response.stdOut);
        VcsBaseEditorWidget::gotoLineOfEditor(editor, lineNumber);
        EditorManager::activateEditor(editor, EditorManager::ModeSwitch);
    } else {
        const QString title = QString::fromLatin1("cvs annotate %1").arg(id);
        IEditor *newEditor = showOutputInEditor(title, response.stdOut, AnnotateOutput, source, codec);
        VcsBaseEditorWidget::tagEditor(newEditor, tag);
        VcsBaseEditorWidget::gotoLineOfEditor(newEditor, lineNumber);
    }
}

bool CvsPlugin::status(const QString &topLevel, const QStringList &files, const QString &title)
{
    QStringList args(QLatin1String("status"));
    args.append(files);
    const CvsResponse response =
            runCvs(topLevel, args, m_settings.timeOutMS(), 0);
    const bool ok = response.result == CvsResponse::Ok;
    if (ok)
        showOutputInEditor(title, response.stdOut, RegularCommandOutput, topLevel, 0);
    return ok;
}

void CvsPlugin::projectStatus()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    status(state.currentProjectTopLevel(), state.relativeCurrentProject(), tr("Project status"));
}

void CvsPlugin::commitProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    startCommit(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void CvsPlugin::diffRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    cvsDiff(state.topLevel(), QStringList());
}

void CvsPlugin::statusRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    status(state.topLevel(), QStringList(), tr("Repository status"));
}

void CvsPlugin::updateRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    update(state.topLevel(), QStringList());

}

void CvsPlugin::slotDescribe(const QString &source, const QString &changeNr)
{
    QString errorMessage;
    if (!describe(source, changeNr, &errorMessage))
        VcsBaseOutputWindow::instance()->appendError(errorMessage);
}

bool CvsPlugin::describe(const QString &file, const QString &changeNr, QString *errorMessage)
{

    QString toplevel;
    const bool manages = managesDirectory(QFileInfo(file).absolutePath(), &toplevel);
    if (!manages || toplevel.isEmpty()) {
        *errorMessage = msgCannotFindTopLevel(file);
        return false;
    }
    return describe(toplevel, QDir(toplevel).relativeFilePath(file), changeNr, errorMessage);
}

bool CvsPlugin::describe(const QString &toplevel, const QString &file, const
                         QString &changeNr, QString *errorMessage)
{

    // In CVS, revisions of files are normally unrelated, there is
    // no global revision/change number. The only thing that groups
    // a commit is the "commit-id" (as shown in the log).
    // This function makes use of it to find all files related to
    // a commit in order to emulate a "describe global change" functionality
    // if desired.
    if (Cvs::Constants::debug)
        qDebug() << Q_FUNC_INFO << file << changeNr;
    // Number must be > 1
    if (isFirstRevision(changeNr)) {
        *errorMessage = tr("The initial revision %1 cannot be described.").arg(changeNr);
        return false;
    }
    // Run log to obtain commit id and details
    QStringList args;
    args << QLatin1String("log") << (QLatin1String("-r") + changeNr) << file;
    const CvsResponse logResponse =
            runCvs(toplevel, args, m_settings.timeOutMS(), SshPasswordPrompt);
    if (logResponse.result != CvsResponse::Ok) {
        *errorMessage = logResponse.message;
        return false;
    }
    const QList<CvsLogEntry> fileLog = parseLogEntries(logResponse.stdOut);
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

        const CvsResponse repoLogResponse =
                runCvs(toplevel, args, m_settings.longTimeOutMS(), SshPasswordPrompt);
        if (repoLogResponse.result != CvsResponse::Ok) {
            *errorMessage = repoLogResponse.message;
            return false;
        }
        // Describe all files found, pass on dir to obtain correct absolute paths.
        const QList<CvsLogEntry> repoEntries = parseLogEntries(repoLogResponse.stdOut, QString(), commitId);
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
bool CvsPlugin::describe(const QString &repositoryPath,
                         QList<CvsLogEntry> entries,
                         QString *errorMessage)
{
    // Collect logs
    QString output;
    QTextCodec *codec = 0;
    const QList<CvsLogEntry>::iterator lend = entries.end();
    for (QList<CvsLogEntry>::iterator it = entries.begin(); it != lend; ++it) {
        // Before fiddling file names, try to find codec
        if (!codec)
            codec = VcsBaseEditorWidget::getCodec(repositoryPath, QStringList(it->file));
        // Run log
        QStringList args(QLatin1String("log"));
        args << (QLatin1String("-r") + it->revisions.front().revision) << it->file;
        const CvsResponse logResponse =
                runCvs(repositoryPath, args, m_settings.timeOutMS(), SshPasswordPrompt);
        if (logResponse.result != CvsResponse::Ok) {
            *errorMessage =  logResponse.message;
            return false;
        }
        output += logResponse.stdOut;
    }
    // Collect diffs relative to repository
    for (QList<CvsLogEntry>::iterator it = entries.begin(); it != lend; ++it) {
        const QString &revision = it->revisions.front().revision;
        if (!isFirstRevision(revision)) {
            const QString previousRev = previousRevision(revision);
            QStringList args(QLatin1String("diff"));
            args << m_settings.cvsDiffOptions << QLatin1String("-r") << previousRev
                    << QLatin1String("-r") << it->revisions.front().revision
                    << it->file;
            const CvsResponse diffResponse =
                    runCvs(repositoryPath, args, m_settings.timeOutMS(), 0, codec);
            switch (diffResponse.result) {
            case CvsResponse::Ok:
            case CvsResponse::NonNullExitCode: // Diff exit code != 0
                if (diffResponse.stdOut.isEmpty()) {
                    *errorMessage = diffResponse.message;
                    return false; // Something else failed.
                }
                break;
            case CvsResponse::OtherError:
                *errorMessage = diffResponse.message;
                return false;
            }
            output += fixDiffOutput(diffResponse.stdOut);
        }
    }

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    const QString commitId = entries.front().revisions.front().commitId;
    if (IEditor *editor = VcsBaseEditorWidget::locateEditorByTag(commitId)) {
        editor->createNew(output);
        EditorManager::activateEditor(editor, EditorManager::ModeSwitch);
        setDiffBaseDirectory(editor, repositoryPath);
    } else {
        const QString title = QString::fromLatin1("cvs describe %1").arg(commitId);
        IEditor *newEditor = showOutputInEditor(title, output, DiffOutput, entries.front().file, codec);
        VcsBaseEditorWidget::tagEditor(newEditor, commitId);
        setDiffBaseDirectory(newEditor, repositoryPath);
    }
    return true;
}

void CvsPlugin::submitCurrentLog()
{
    m_submitActionTriggered = true;
    EditorManager::instance()->closeEditors(QList<IEditor*>()
        << EditorManager::currentEditor());
}

// Run CVS. At this point, file arguments must be relative to
// the working directory (see above).
CvsResponse CvsPlugin::runCvs(const QString &workingDirectory,
                              const QStringList &arguments,
                              int timeOut,
                              unsigned flags,
                              QTextCodec *outputCodec)
{
    const QString executable = m_settings.cvsBinaryPath;
    CvsResponse response;
    if (executable.isEmpty()) {
        response.result = CvsResponse::OtherError;
        response.message =tr("No cvs executable specified!");
        return response;
    }
    // Run, connect stderr to the output window
    const Utils::SynchronousProcessResponse sp_resp =
            runVcs(workingDirectory, executable,
                   m_settings.addOptions(arguments),
                   timeOut, flags, outputCodec);

    response.result = CvsResponse::OtherError;
    response.stdErr = sp_resp.stdErr;
    response.stdOut = sp_resp.stdOut;
    switch (sp_resp.result) {
    case Utils::SynchronousProcessResponse::Finished:
        response.result = CvsResponse::Ok;
        break;
    case Utils::SynchronousProcessResponse::FinishedError:
        response.result = CvsResponse::NonNullExitCode;
        break;
    case Utils::SynchronousProcessResponse::TerminatedAbnormally:
    case Utils::SynchronousProcessResponse::StartFailed:
    case Utils::SynchronousProcessResponse::Hang:
        break;
    }

    if (response.result != CvsResponse::Ok)
        response.message = sp_resp.exitMessage(executable, timeOut);

    return response;
}

IEditor *CvsPlugin::showOutputInEditor(const QString& title, const QString &output,
                                       int editorType, const QString &source,
                                       QTextCodec *codec)
{
    const VcsBaseEditorParameters *params = findType(editorType);
    QTC_ASSERT(params, return 0);
    const Id id = Core::Id(QByteArray(params->id));
    if (Cvs::Constants::debug)
        qDebug() << "CVSPlugin::showOutputInEditor" << title << id.name()
                 <<  "source=" << source << "Size= " << output.size() <<  " Type=" << editorType << debugCodec(codec);
    QString s = title;
    IEditor *editor = EditorManager::openEditorWithContents(id, &s, output);
    connect(editor, SIGNAL(annotateRevisionRequested(QString,QString,int)),
            this, SLOT(vcsAnnotate(QString,QString,int)));
    CvsEditor *e = qobject_cast<CvsEditor*>(editor->widget());
    if (!e)
        return 0;
    s.replace(QLatin1Char(' '), QLatin1Char('_'));
    e->setSuggestedFileName(s);
    e->setForceReadOnly(true);
    if (!source.isEmpty())
        e->setSource(source);
    if (codec)
        e->setCodec(codec);
    IEditor *ie = e->editor();
    EditorManager::activateEditor(ie, EditorManager::ModeSwitch);
    return ie;
}

CvsSettings CvsPlugin::settings() const
{
    return m_settings;
}

void CvsPlugin::setSettings(const CvsSettings &s)
{
    if (s != m_settings) {
        m_settings = s;
        if (QSettings *settings = ICore::settings())
            m_settings.toSettings(settings);
        cvsVersionControl()->emitConfigurationChanged();
    }
}

CvsPlugin *CvsPlugin::instance()
{
    QTC_ASSERT(m_cvsPluginInstance, return m_cvsPluginInstance);
    return m_cvsPluginInstance;
}

bool CvsPlugin::vcsAdd(const QString &workingDir, const QString &rawFileName)
{
    QStringList args;
    args << QLatin1String("add") << rawFileName;
    const CvsResponse response =
            runCvs(workingDir, args, m_settings.timeOutMS(),
                   SshPasswordPrompt|ShowStdOutInLogWindow);
    return response.result == CvsResponse::Ok;
}

bool CvsPlugin::vcsDelete(const QString &workingDir, const QString &rawFileName)
{
    QStringList args;
    args << QLatin1String("remove") << QLatin1String("-f") << rawFileName;
    const CvsResponse response =
            runCvs(workingDir, args, m_settings.timeOutMS(),
                   SshPasswordPrompt|ShowStdOutInLogWindow);
    return response.result == CvsResponse::Ok;
}

/* CVS has a "CVS" directory in each directory it manages. The top level
 * is the first directory under the directory that does not have it. */
bool CvsPlugin::managesDirectory(const QString &directory, QString *topLevel /* = 0 */) const
{
    if (topLevel)
        topLevel->clear();
    bool manages = false;
    const QDir dir(directory);
    do {
        if (!dir.exists() || !checkCVSDirectory(dir))
            break;
        manages = true;
        if (!topLevel)
            break;
        /* Recursing up, the top level is a child of the first directory that does
         * not have a  "CVS" directory. The starting directory must be a managed
         * one. Go up and try to find the first unmanaged parent dir. */
        QDir lastDirectory = dir;
        for (QDir parentDir = lastDirectory; parentDir.cdUp() ; lastDirectory = parentDir) {
            if (!checkCVSDirectory(parentDir)) {
                *topLevel = lastDirectory.absolutePath();
                break;
            }
        }
    } while (false);
    if (Cvs::Constants::debug) {
        QDebug nsp = qDebug().nospace();
        nsp << "CVSPlugin::managesDirectory" << directory << manages;
        if (topLevel)
            nsp << *topLevel;
    }
    return manages;
}

bool CvsPlugin::checkCVSDirectory(const QDir &directory) const
{
    const QString cvsDir = directory.absoluteFilePath(QLatin1String("CVS"));
    return QFileInfo(cvsDir).isDir();
}

CvsControl *CvsPlugin::cvsVersionControl() const
{
    return static_cast<CvsControl *>(versionControl());
}

#ifdef WITH_TESTS
void CvsPlugin::testDiffFileResolving_data()
{
    QTest::addColumn<QByteArray>("header");
    QTest::addColumn<QByteArray>("fileName");

    QTest::newRow("Modified") << QByteArray(
            "Index: src/plugins/cvs/cvseditor.cpp\n"
            "===================================================================\n"
            "--- src/plugins/cvs/cvseditor.cpp\t21 Jan 2013 20:34:20 -0000\t1.1\n"
            "+++ src/plugins/cvs/cvseditor.cpp\t21 Jan 2013 20:34:28 -0000\n"
            "@@ -120,7 +120,7 @@\n\n")
        << QByteArray("src/plugins/cvs/cvseditor.cpp");
}

void CvsPlugin::testDiffFileResolving()
{
    CvsEditor editor(editorParameters + 3, 0);
    editor.testDiffFileResolving();
}

void CvsPlugin::testLogResolving()
{
    QByteArray data(
                "RCS file: /sources/cvs/ccvs/Attic/FIXED-BUGS,v\n"
                "Working file: FIXED-BUGS\n"
                "head: 1.3\n"
                "branch:\n"
                "locks: strict\n"
                "access list:\n"
                "symbolic names:\n"
                "keyword substitution: kv\n"
                "total revisions: 3;     selected revisions: 3\n"
                "description:\n"
                "----------------------------\n"
                "revision 1.3\n"
                "date: 1995-04-29 06:22:41 +0300;  author: jimb;  state: dead;  lines: +0 -0;\n"
                "*** empty log message ***\n"
                "----------------------------\n"
                "revision 1.2\n"
                "date: 1995-04-28 18:52:24 +0300;  author: noel;  state: Exp;  lines: +6 -0;\n"
                "added latest commentary\n"
                "----------------------------\n"
                );
    CvsEditor editor(editorParameters + 1, 0);
    editor.testLogResolving(data, "1.3", "1.2");
}
#endif

} // namespace Internal
} // namespace Cvs

Q_EXPORT_PLUGIN(Cvs::Internal::CvsPlugin)

#include "cvsplugin.moc"
