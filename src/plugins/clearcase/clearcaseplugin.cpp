/**************************************************************************
**
** Copyright (c) 2013 AudioCodes Ltd.
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
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

#include "activityselector.h"
#include "checkoutdialog.h"
#include "clearcaseconstants.h"
#include "clearcasecontrol.h"
#include "clearcaseeditor.h"
#include "clearcaseplugin.h"
#include "clearcasesubmiteditor.h"
#include "clearcasesubmiteditorwidget.h"
#include "clearcasesync.h"
#include "settingspage.h"
#include "versionselector.h"
#include "ui_undocheckout.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <locator/commandlocator.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/iprojectmanager.h>
#include <utils/synchronousprocess.h>
#include <utils/parameteraction.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseeditorparameterwidget.h>
#include <vcsbase/vcsbaseoutputwindow.h>
#include <vcsbase/vcsbasesubmiteditor.h>


#include <QAction>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFuture>
#include <QFutureInterface>
#include <QInputDialog>
#include <QList>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QMetaObject>
#include <QMutex>
#include <QProcess>
#include <QRegExp>
#include <QSharedPointer>
#include <QtConcurrentRun>
#include <QTemporaryFile>
#include <QTextCodec>
#include <QtPlugin>
#include <QUrl>
#include <QUuid>
#include <QVariant>
#include <QVBoxLayout>
#include <QXmlStreamReader>
#ifdef WITH_TESTS
#include <QTest>
#endif

namespace ClearCase {
namespace Internal {

static const char CMD_ID_CLEARCASE_MENU[]     = "ClearCase.Menu";
static const char CMD_ID_CHECKOUT[]           = "ClearCase.CheckOut";
static const char CMD_ID_CHECKIN[]            = "ClearCase.CheckInCurrent";
static const char CMD_ID_UNDOCHECKOUT[]       = "ClearCase.UndoCheckOut";
static const char CMD_ID_UNDOHIJACK[]         = "ClearCase.UndoHijack";
static const char CMD_ID_DIFF_CURRENT[]       = "ClearCase.DiffCurrent";
static const char CMD_ID_HISTORY_CURRENT[]    = "ClearCase.HistoryCurrent";
static const char CMD_ID_ANNOTATE[]           = "ClearCase.Annotate";
static const char CMD_ID_ADD_FILE[]           = "ClearCase.AddFile";
static const char CMD_ID_DIFF_ACTIVITY[]      = "ClearCase.DiffActivity";
static const char CMD_ID_CHECKIN_ACTIVITY[]   = "ClearCase.CheckInActivity";
static const char CMD_ID_UPDATEINDEX[]        = "ClearCase.UpdateIndex";
static const char CMD_ID_UPDATE_VIEW[]        = "ClearCase.UpdateView";
static const char CMD_ID_CHECKIN_ALL[]        = "ClearCase.CheckInAll";
static const char CMD_ID_STATUS[]             = "ClearCase.Status";

static const VcsBase::VcsBaseEditorParameters editorParameters[] = {
{
    VcsBase::RegularCommandOutput,
    "ClearCase Command Log Editor", // id
    QT_TRANSLATE_NOOP("VCS", "ClearCase Command Log Editor"), // display name
    "ClearCase Command Log Editor", // context
    "application/vnd.audc.text.scs_cc_commandlog",
    "scslog"},
{   VcsBase::LogOutput,
    "ClearCase File Log Editor",   // id
    QT_TRANSLATE_NOOP("VCS", "ClearCase File Log Editor"),   // display_name
    "ClearCase File Log Editor",   // context
    "application/vnd.audc.text.scs_cc_filelog",
    "scsfilelog"},
{    VcsBase::AnnotateOutput,
    "ClearCase Annotation Editor",  // id
    QT_TRANSLATE_NOOP("VCS", "ClearCase Annotation Editor"),   // display_name
    "ClearCase Annotation Editor",  // context
    "application/vnd.audc.text.scs_cc_annotation",
    "scsannotate"},
{   VcsBase::DiffOutput,
    "ClearCase Diff Editor",  // id
    QT_TRANSLATE_NOOP("VCS", "ClearCase Diff Editor"),   // display_name
    "ClearCase Diff Editor",  // context
    "text/x-patch","diff"}
};

// Utility to find a parameter set by type
static inline const VcsBase::VcsBaseEditorParameters *findType(int ie)
{
    const VcsBase::EditorContentType et = static_cast<VcsBase::EditorContentType>(ie);
    return VcsBase::VcsBaseEditorWidget::findType(editorParameters, sizeof(editorParameters)/sizeof(VcsBase::VcsBaseEditorParameters), et);
}

static inline QString debugCodec(const QTextCodec *c)
{
    return c ? QString::fromLatin1(c->name()) : QString::fromLatin1("Null codec");
}

// ------------- ClearCasePlugin
ClearCasePlugin *ClearCasePlugin::m_clearcasePluginInstance = 0;

ViewData::ViewData() :
    isDynamic(false),
    isUcm(false)
{
}

ClearCasePlugin::ClearCasePlugin() :
    VcsBase::VcsBasePlugin(QLatin1String(ClearCase::Constants::CLEARCASECHECKINEDITOR_ID)),
    m_commandLocator(0),
    m_checkOutAction(0),
    m_checkInCurrentAction(0),
    m_undoCheckOutAction(0),
    m_undoHijackAction(0),
    m_diffCurrentAction(0),
    m_historyCurrentAction(0),
    m_annotateCurrentAction(0),
    m_addFileAction(0),
    m_diffActivityAction(0),
    m_updateIndexAction(0),
    m_updateViewAction(0),
    m_checkInActivityAction(0),
    m_checkInAllAction(0),
    m_statusAction(0),
    m_checkInSelectedAction(0),
    m_checkInDiffAction(0),
    m_submitUndoAction(0),
    m_submitRedoAction(0),
    m_menuAction(0),
    m_submitActionTriggered(false),
    m_activityMutex(new QMutex),
    m_statusMap(new StatusMap)
{
    qRegisterMetaType<ClearCase::Internal::FileStatus::Status>("ClearCase::Internal::FileStatus::Status");
}

ClearCasePlugin::~ClearCasePlugin()
{
    cleanCheckInMessageFile();
    // wait for sync thread to finish reading activities
    m_activityMutex->lock();
    m_activityMutex->unlock();
    delete m_activityMutex;
}

void ClearCasePlugin::cleanCheckInMessageFile()
{
    if (!m_checkInMessageFileName.isEmpty()) {
        QFile::remove(m_checkInMessageFileName);
        m_checkInMessageFileName.clear();
        m_checkInView.clear();
    }
}

bool ClearCasePlugin::isCheckInEditorOpen() const
{
    return !m_checkInMessageFileName.isEmpty();
}

/*! Find top level for view that contains \a directory
 *
 * - Snapshot Views will have the CLEARCASE_ROOT_FILE (view.dat) in its top dir
 * - Dynamic views can either be
 *      - M:/view_name,
 *      - or mapped to a drive letter, like Z:/
 *    (drive letters are just examples)
 */
QString ClearCasePlugin::findTopLevel(const QString &directory) const
{
    if ((directory == m_topLevel) ||
            Utils::FileName::fromString(directory).isChildOf(Utils::FileName::fromString(m_topLevel)))
        return m_topLevel;

    // Snapshot view
    QString topLevel =
            findRepositoryForDirectory(directory, QLatin1String(ClearCase::Constants::CLEARCASE_ROOT_FILE));
    if (!topLevel.isEmpty() || !clearCaseControl()->isConfigured())
        return topLevel;

    // Dynamic view
    if (ccGetView(directory).isDynamic) {
        QDir dir(directory);
        // Go up to one level before root
        QDir outer = dir;
        outer.cdUp();
        while (outer.cdUp())
            dir.cdUp();
        topLevel = dir.path(); // M:/View_Name
        dir.cdUp(); // Z:/ (dynamic view with assigned letter)
        if (!ccGetView(dir.path()).name.isEmpty())
            topLevel = dir.path();
    }

    return topLevel;
}

static const VcsBase::VcsBaseSubmitEditorParameters submitParameters = {
    ClearCase::Constants::CLEARCASE_SUBMIT_MIMETYPE,
    ClearCase::Constants::CLEARCASECHECKINEDITOR_ID,
    ClearCase::Constants::CLEARCASECHECKINEDITOR_DISPLAY_NAME,
    ClearCase::Constants::CLEARCASECHECKINEDITOR,
    VcsBase::VcsBaseSubmitEditorParameters::DiffFiles
};

bool ClearCasePlugin::initialize(const QStringList & /*arguments */, QString *errorMessage)
{
    typedef VcsBase::VcsSubmitEditorFactory<ClearCaseSubmitEditor> ClearCaseSubmitEditorFactory;
    typedef VcsBase::VcsEditorFactory<ClearCaseEditor> ClearCaseEditorFactory;
    using namespace Constants;

    using namespace Core::Constants;
    using namespace ExtensionSystem;

    initializeVcs(new ClearCaseControl(this));

    m_clearcasePluginInstance = this;
    connect(Core::ICore::instance(), SIGNAL(coreAboutToClose()), this, SLOT(closing()));
    connect(Core::ICore::progressManager(), SIGNAL(allTasksFinished(QString)),
            this, SLOT(tasksFinished(QString)));

    if (!Core::ICore::mimeDatabase()->addMimeTypes(QLatin1String(":/clearcase/ClearCase.mimetypes.xml"), errorMessage))
        return false;

    m_settings.fromSettings(Core::ICore::settings());

    // update view name when changing active project
    if (ProjectExplorer::ProjectExplorerPlugin *pe = ProjectExplorer::ProjectExplorerPlugin::instance())
        connect(pe, SIGNAL(currentProjectChanged(ProjectExplorer::Project*)),
                this, SLOT(projectChanged(ProjectExplorer::Project*)));

    addAutoReleasedObject(new SettingsPage);

    addAutoReleasedObject(new ClearCaseSubmitEditorFactory(&submitParameters));

    // any editor responds to describe (when clicking a version)
    static const char *describeSlot = SLOT(describe(QString,QString));
    const int editorCount = sizeof(editorParameters)/sizeof(VcsBase::VcsBaseEditorParameters);
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new ClearCaseEditorFactory(editorParameters + i, this, describeSlot));

    const QString description = QLatin1String("ClearCase");
    const QString prefix = QLatin1String("cc");
    // register cc prefix in Locator
    m_commandLocator = new Locator::CommandLocator("cc", description, prefix);
    addAutoReleasedObject(m_commandLocator);

    //register actions
    Core::ActionContainer *toolsContainer = Core::ActionManager::actionContainer(M_TOOLS);

    Core::ActionContainer *clearcaseMenu = Core::ActionManager::createMenu(Core::Id(CMD_ID_CLEARCASE_MENU));
    clearcaseMenu->menu()->setTitle(tr("C&learCase"));
    toolsContainer->addMenu(clearcaseMenu);
    m_menuAction = clearcaseMenu->menu()->menuAction();
    Core::Context globalcontext(C_GLOBAL);
    Core::Command *command;

    m_checkOutAction = new Utils::ParameterAction(tr("Check Out..."), tr("Check &Out \"%1\"..."), Utils::ParameterAction::AlwaysEnabled, this);
    command = Core::ActionManager::registerAction(m_checkOutAction, CMD_ID_CHECKOUT,
        globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+L,Meta+O") : tr("Alt+L,Alt+O")));
    connect(m_checkOutAction, SIGNAL(triggered()), this, SLOT(checkOutCurrentFile()));
    clearcaseMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_checkInCurrentAction = new Utils::ParameterAction(tr("Check &In..."), tr("Check &In \"%1\"..."), Utils::ParameterAction::AlwaysEnabled, this);
    command = Core::ActionManager::registerAction(m_checkInCurrentAction, CMD_ID_CHECKIN, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+L,Meta+I") : tr("Alt+L,Alt+I")));
    connect(m_checkInCurrentAction, SIGNAL(triggered()), this, SLOT(startCheckInCurrentFile()));
    clearcaseMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_undoCheckOutAction = new Utils::ParameterAction(tr("Undo Check Out"), tr("&Undo Check Out \"%1\""), Utils::ParameterAction::AlwaysEnabled, this);
    command = Core::ActionManager::registerAction(m_undoCheckOutAction, CMD_ID_UNDOCHECKOUT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+L,Meta+U") : tr("Alt+L,Alt+U")));
    connect(m_undoCheckOutAction, SIGNAL(triggered()), this, SLOT(undoCheckOutCurrent()));
    clearcaseMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_undoHijackAction = new Utils::ParameterAction(tr("Undo Hijack"), tr("Undo Hi&jack \"%1\""), Utils::ParameterAction::AlwaysEnabled, this);
    command = Core::ActionManager::registerAction(m_undoHijackAction, CMD_ID_UNDOHIJACK, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+L,Meta+R") : tr("Alt+L,Alt+R")));
    connect(m_undoHijackAction, SIGNAL(triggered()), this, SLOT(undoHijackCurrent()));
    clearcaseMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    clearcaseMenu->addSeparator(globalcontext);

    m_diffCurrentAction = new Utils::ParameterAction(tr("Diff Current File"), tr("&Diff \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_diffCurrentAction,
        CMD_ID_DIFF_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+L,Meta+D") : tr("Alt+L,Alt+D")));
    connect(m_diffCurrentAction, SIGNAL(triggered()), this, SLOT(diffCurrentFile()));
    clearcaseMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_historyCurrentAction = new Utils::ParameterAction(tr("History Current File"), tr("&History \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_historyCurrentAction,
        CMD_ID_HISTORY_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+L,Meta+H") : tr("Alt+L,Alt+H")));
    connect(m_historyCurrentAction, SIGNAL(triggered()), this,
        SLOT(historyCurrentFile()));
    clearcaseMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_annotateCurrentAction = new Utils::ParameterAction(tr("Annotate Current File"), tr("&Annotate \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_annotateCurrentAction,
        CMD_ID_ANNOTATE, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+L,Meta+A") : tr("Alt+L,Alt+A")));
    connect(m_annotateCurrentAction, SIGNAL(triggered()), this,
        SLOT(annotateCurrentFile()));
    clearcaseMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_addFileAction = new Utils::ParameterAction(tr("Add File..."), tr("Add File \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_addFileAction, CMD_ID_ADD_FILE, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_addFileAction, SIGNAL(triggered()), this, SLOT(addCurrentFile()));
    clearcaseMenu->addAction(command);

    clearcaseMenu->addSeparator(globalcontext);

    m_diffActivityAction = new QAction(tr("Diff A&ctivity..."), this);
    command = Core::ActionManager::registerAction(m_diffActivityAction, CMD_ID_DIFF_ACTIVITY, globalcontext);
    connect(m_diffActivityAction, SIGNAL(triggered()), this, SLOT(diffActivity()));
    clearcaseMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_checkInActivityAction = new Utils::ParameterAction(tr("Ch&eck In Activity"), tr("Chec&k In Activity \"%1\"..."), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_checkInActivityAction, CMD_ID_CHECKIN_ACTIVITY, globalcontext);
    connect(m_checkInActivityAction, SIGNAL(triggered()), this, SLOT(startCheckInActivity()));
    command->setAttribute(Core::Command::CA_UpdateText);
    clearcaseMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    clearcaseMenu->addSeparator(globalcontext);

    m_updateIndexAction = new QAction(tr("Update Index"), this);
    command = Core::ActionManager::registerAction(m_updateIndexAction, CMD_ID_UPDATEINDEX, globalcontext);
    connect(m_updateIndexAction, SIGNAL(triggered()), this, SLOT(updateIndex()));
    clearcaseMenu->addAction(command);

    m_updateViewAction = new Utils::ParameterAction(tr("Update View"), tr("U&pdate View \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = Core::ActionManager::registerAction(m_updateViewAction, CMD_ID_UPDATE_VIEW, globalcontext);
    connect(m_updateViewAction, SIGNAL(triggered()), this, SLOT(updateView()));
    command->setAttribute(Core::Command::CA_UpdateText);
    clearcaseMenu->addAction(command);

    clearcaseMenu->addSeparator(globalcontext);

    m_checkInAllAction = new QAction(tr("Check In All &Files..."), this);
    command = Core::ActionManager::registerAction(m_checkInAllAction, CMD_ID_CHECKIN_ALL, globalcontext);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+L,Meta+F") : tr("Alt+L,Alt+F")));
    connect(m_checkInAllAction, SIGNAL(triggered()), this, SLOT(startCheckInAll()));
    clearcaseMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_statusAction = new QAction(tr("View &Status"), this);
    command = Core::ActionManager::registerAction(m_statusAction, CMD_ID_STATUS, globalcontext);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+L,Meta+S") : tr("Alt+L,Alt+S")));
    connect(m_statusAction, SIGNAL(triggered()), this, SLOT(viewStatus()));
    clearcaseMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    // Actions of the submit editor
    Core::Context clearcasecheckincontext(Constants::CLEARCASECHECKINEDITOR);

    m_checkInSelectedAction = new QAction(VcsBase::VcsBaseSubmitEditor::submitIcon(), tr("Check In"), this);
    command = Core::ActionManager::registerAction(m_checkInSelectedAction, Constants::CHECKIN_SELECTED, clearcasecheckincontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_checkInSelectedAction, SIGNAL(triggered()), this, SLOT(checkInSelected()));

    m_checkInDiffAction = new QAction(VcsBase::VcsBaseSubmitEditor::diffIcon(), tr("Diff Selected Files"), this);
    command = Core::ActionManager::registerAction(m_checkInDiffAction , Constants::DIFF_SELECTED, clearcasecheckincontext);

    m_submitUndoAction = new QAction(tr("&Undo"), this);
    command = Core::ActionManager::registerAction(m_submitUndoAction, Core::Constants::UNDO, clearcasecheckincontext);

    m_submitRedoAction = new QAction(tr("&Redo"), this);
    command = Core::ActionManager::registerAction(m_submitRedoAction, Core::Constants::REDO, clearcasecheckincontext);

    return true;
}

// called before closing the submit editor
bool ClearCasePlugin::submitEditorAboutToClose(VcsBase::VcsBaseSubmitEditor *submitEditor)
{
    if (!isCheckInEditorOpen())
        return true;

    Core::IDocument *editorDocument = submitEditor->document();
    ClearCaseSubmitEditor *editor = qobject_cast<ClearCaseSubmitEditor *>(submitEditor);
    if (!editorDocument || !editor)
        return true;

    // Submit editor closing. Make it write out the check in message
    // and retrieve files
    const QFileInfo editorFile(editorDocument->fileName());
    const QFileInfo changeFile(m_checkInMessageFileName);
    if (editorFile.absoluteFilePath() != changeFile.absoluteFilePath())
        return true; // Oops?!

    // Prompt user. Force a prompt unless submit was actually invoked (that
    // is, the editor was closed or shutdown).
    bool prompt = m_settings.promptToCheckIn;
    const VcsBase::VcsBaseSubmitEditor::PromptSubmitResult answer =
            editor->promptSubmit(tr("Closing ClearCase Editor"),
                                 tr("Do you want to check in the files?"),
                                 tr("The comment check failed. Do you want to check in the files?"),
                                 &prompt, !m_submitActionTriggered);
    m_submitActionTriggered = false;
    switch (answer) {
    case VcsBase::VcsBaseSubmitEditor::SubmitCanceled:
        return false; // Keep editing and change file
    case VcsBase::VcsBaseSubmitEditor::SubmitDiscarded:
        cleanCheckInMessageFile();
        return true; // Cancel all
    default:
        break;
    }
    // If user changed
    if (prompt != m_settings.promptToCheckIn) {
        m_settings.promptToCheckIn = prompt;
        m_settings.toSettings(Core::ICore::settings());
    }

    const QStringList fileList = editor->checkedFiles();
    bool closeEditor = true;
    if (!fileList.empty()) {
        // get message & check in
        closeEditor = Core::DocumentManager::saveDocument(editorDocument);
        if (closeEditor) {
            ClearCaseSubmitEditorWidget *widget = editor->submitEditorWidget();
            closeEditor = vcsCheckIn(m_checkInMessageFileName, fileList, widget->activity(),
                                   widget->isIdentical(), widget->isPreserve(),
                                   widget->activityChanged());
        }
    }
    // vcsCheckIn might fail if some of the files failed to check-in (though it does check-in
    // those who didn't fail). Therefore, if more than one file was sent, consider it as success
    // anyway (sync will be called from vcsCheckIn for next attempt)
    closeEditor |= (fileList.count() > 1);
    if (closeEditor)
        cleanCheckInMessageFile();
    return closeEditor;
}

void ClearCasePlugin::diffCheckInFiles(const QStringList &files)
{
    ccDiffWithPred(m_checkInView, files);
}

static inline void setDiffBaseDirectory(Core::IEditor *editor, const QString &db)
{
    if (VcsBase::VcsBaseEditorWidget *ve = qobject_cast<VcsBase::VcsBaseEditorWidget*>(editor->widget()))
        ve->setDiffBaseDirectory(db);
}

//! retrieve full location of predecessor of \a version
QString ClearCasePlugin::ccGetPredecessor(const QString &version) const
{
    QStringList args(QLatin1String("describe"));
    args << QLatin1String("-fmt") << QLatin1String("%En@@%PSn") << version;
    const ClearCaseResponse response =
            runCleartool(currentState().topLevel(), args, m_settings.timeOutMS(), SilentRun);
    if (response.error || response.stdOut.endsWith(QLatin1Char('@'))) // <name-unknown>@@
        return QString();
    else
        return response.stdOut;
}

QStringList ClearCasePlugin::ccGetActiveVobs() const
{
    QStringList res;
    QStringList args(QLatin1String("lsvob"));
    args << QLatin1String("-short");
    QString topLevel = currentState().topLevel();
    const ClearCaseResponse response =
            runCleartool(topLevel, args, m_settings.timeOutMS(), SilentRun);
    if (response.error)
        return res;
    foreach (QString dir, response.stdOut.split(QLatin1Char('\n'), QString::SkipEmptyParts)) {
        dir = dir.mid(1); // omit first slash
        QFileInfo fi(topLevel, dir);
        if (fi.exists())
            res.append(dir);
    }
    return res;
}

// file must be relative to topLevel, and using '/' path separator
FileStatus ClearCasePlugin::vcsStatus(const QString &file) const
{
    return m_statusMap->value(file, FileStatus(FileStatus::Unknown));
}

QString ClearCasePlugin::ccGetFileActivity(const QString &workingDir, const QString &file)
{
    QStringList args(QLatin1String("lscheckout"));
    args << QLatin1String("-fmt") << QLatin1String("%[activity]p");
    args << file;
    const ClearCaseResponse response =
            runCleartool(workingDir, args, m_settings.timeOutMS(), SilentRun);
    return response.stdOut;
}

ClearCaseSubmitEditor *ClearCasePlugin::openClearCaseSubmitEditor(const QString &fileName, bool isUcm)
{
    Core::IEditor *editor =
            Core::EditorManager::openEditor(fileName,
                                                        Constants::CLEARCASECHECKINEDITOR_ID,
                                                        Core::EditorManager::ModeSwitch);
    ClearCaseSubmitEditor *submitEditor = qobject_cast<ClearCaseSubmitEditor*>(editor);
    QTC_CHECK(submitEditor);
    submitEditor->registerActions(m_submitUndoAction, m_submitRedoAction, m_checkInSelectedAction, m_checkInDiffAction);
    connect(submitEditor, SIGNAL(diffSelectedFiles(QStringList)), this, SLOT(diffCheckInFiles(QStringList)));
    submitEditor->setCheckScriptWorkingDirectory(m_checkInView);
    submitEditor->setIsUcm(isUcm);
    return submitEditor;
}

void ClearCasePlugin::updateStatusActions()
{
    bool hasFile = currentState().hasFile();
    QString fileName = currentState().relativeCurrentFile();

    FileStatus fileStatus = m_statusMap->value(fileName, FileStatus(FileStatus::Unknown));

    if (ClearCase::Constants::debug)
        qDebug() << Q_FUNC_INFO << fileName << ", status = " << fileStatus.status;

    m_checkOutAction->setEnabled(hasFile && (fileStatus.status & (FileStatus::CheckedIn | FileStatus::Hijacked)));
    m_undoCheckOutAction->setEnabled(hasFile && (fileStatus.status & FileStatus::CheckedOut));
    m_undoHijackAction->setEnabled(!m_viewData.isDynamic && hasFile && (fileStatus.status & FileStatus::Hijacked));
    m_checkInCurrentAction->setEnabled(hasFile && (fileStatus.status & FileStatus::CheckedOut));
    m_addFileAction->setEnabled(hasFile && (fileStatus.status & FileStatus::NotManaged));

    m_checkInActivityAction->setEnabled(m_viewData.isUcm);
    m_diffActivityAction->setEnabled(m_viewData.isUcm);
}

void ClearCasePlugin::updateActions(VcsBase::VcsBasePlugin::ActionState as)
{
    if (!enableMenuAction(as, m_menuAction)) {
        m_commandLocator->setEnabled(false);
        return;
    }
    const VcsBase::VcsBasePluginState state = currentState();
    const bool hasTopLevel = state.hasTopLevel();
    m_commandLocator->setEnabled(hasTopLevel);
    if (hasTopLevel)
        m_topLevel = state.topLevel();

    m_updateViewAction->setParameter(m_viewData.isDynamic ? QString() : m_viewData.name);

    const QString fileName = state.currentFileName();
    m_checkOutAction->setParameter(fileName);
    m_undoCheckOutAction->setParameter(fileName);
    m_undoHijackAction->setParameter(fileName);
    m_diffCurrentAction->setParameter(fileName);
    m_checkInCurrentAction->setParameter(fileName);
    m_historyCurrentAction->setParameter(fileName);
    m_annotateCurrentAction->setParameter(fileName);
    m_addFileAction->setParameter(fileName);
    m_updateIndexAction->setEnabled(!m_settings.disableIndexer);
    updateStatusActions();
}

void ClearCasePlugin::checkOutCurrentFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    vcsOpen(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void ClearCasePlugin::addCurrentFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    vcsAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void ClearCasePlugin::setStatus(const QString &file, FileStatus::Status status, bool update)
{
    m_statusMap->insert(file, FileStatus(status, QFileInfo(currentState().topLevel(), file).permissions()));
    if (update && (currentState().relativeCurrentFile() == file))
        QMetaObject::invokeMethod(this, "updateStatusActions");
}

void ClearCasePlugin::undoCheckOutCurrent()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    QString file = state.relativeCurrentFile();
    const QString fileName = QDir::toNativeSeparators(file);

    QStringList args(QLatin1String("diff"));
    args << QLatin1String("-diff_format") << QLatin1String("-predecessor");
    args << fileName;

    const ClearCaseResponse diffResponse =
            runCleartool(state.currentFileTopLevel(), args, m_settings.timeOutMS(), 0);

    bool different = diffResponse.error; // return value is 1 if there is any difference
    bool keep = false;
    if (different) {
        Ui::UndoCheckOut uncoUi;
        QDialog uncoDlg;
        uncoUi.setupUi(&uncoDlg);
        uncoUi.lblMessage->setText(tr("Do you want to undo the check out of '%1'?").arg(fileName));
        if (uncoDlg.exec() != QDialog::Accepted)
            return;
        keep = uncoUi.chkKeep->isChecked();
    }
    vcsUndoCheckOut(state.topLevel(), file, keep);
}

bool ClearCasePlugin::vcsUndoCheckOut(const QString &workingDir, const QString &fileName, bool keep)
{
    if (ClearCase::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDir << fileName << keep;

    Core::FileChangeBlocker fcb(fileName);

    // revert
    QStringList args(QLatin1String("uncheckout"));
    args << QLatin1String(keep ? "-keep" : "-rm");
    args << QDir::toNativeSeparators(fileName);

    const ClearCaseResponse response =
            runCleartool(workingDir, args, m_settings.timeOutMS(),
                         ShowStdOutInLogWindow | FullySynchronously);

    if (!response.error) {
        if (!m_settings.disableIndexer)
            setStatus(fileName, FileStatus::CheckedIn);
        clearCaseControl()->emitFilesChanged(QStringList(fileName));
    }
    return !response.error;
}


/*! Undo a hijacked file in a snapshot view
 *
 * Runs cleartool update -overwrite \a fileName in \a workingDir
 * if \a keep is true, renames hijacked files to <filename>.keep. Otherwise it is overwritten
 */
bool ClearCasePlugin::vcsUndoHijack(const QString &workingDir, const QString &fileName, bool keep)
{
    if (ClearCase::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDir << fileName << keep;
    QStringList args(QLatin1String("update"));
    args << QLatin1String(keep ? "-rename" : "-overwrite");
    args << QLatin1String("-log");
    if (Utils::HostOsInfo::isWindowsHost())
        args << QLatin1String("NUL");
    else
    args << QLatin1String("/dev/null");
    args << QDir::toNativeSeparators(fileName);

    const ClearCaseResponse response =
            runCleartool(workingDir, args, m_settings.timeOutMS(),
                   ShowStdOutInLogWindow | FullySynchronously);
    if (!response.error && !m_settings.disableIndexer)
        setStatus(fileName, FileStatus::CheckedIn);
    return !response.error;
}

void ClearCasePlugin::undoHijackCurrent()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    const QString fileName = state.relativeCurrentFile();

    bool keep = false;
    bool askKeep = true;
    if (m_settings.extDiffAvailable) {
        QString diffres = diffExternal(ccGetFileVersion(state.topLevel(), fileName), fileName);
        if (diffres.at(0) == QLatin1Char('F')) // Files are identical
            askKeep = false;
    }
    if (askKeep) {
        Ui::UndoCheckOut unhijackUi;
        QDialog unhijackDlg;
        unhijackUi.setupUi(&unhijackDlg);
        unhijackDlg.setWindowTitle(tr("Undo Hijack File"));
        unhijackUi.lblMessage->setText(tr("Do you want to undo hijack of '%1'?")
                                       .arg(QDir::toNativeSeparators(fileName)));
        if (unhijackDlg.exec() != QDialog::Accepted)
            return;
        keep = unhijackUi.chkKeep->isChecked();
    }

    Core::FileChangeBlocker fcb(state.currentFile());

    // revert
    if (vcsUndoHijack(state.currentFileTopLevel(), fileName, keep))
        clearCaseControl()->emitFilesChanged(QStringList(state.currentFile()));
}

QString ClearCasePlugin::ccGetFileVersion(const QString &workingDir, const QString &file) const
{
    QStringList args(QLatin1String("ls"));
    args << QLatin1String("-short") << file;
    return runCleartoolSync(workingDir, args).trimmed();
}

void ClearCasePlugin::ccDiffWithPred(const QString &workingDir, const QStringList &files)
{
    if (ClearCase::Constants::debug)
        qDebug() << Q_FUNC_INFO << files;
    const QString source = VcsBase::VcsBaseEditorWidget::getSource(workingDir, files);
    QTextCodec *codec = source.isEmpty() ? static_cast<QTextCodec *>(0) : VcsBase::VcsBaseEditorWidget::getCodec(source);

    if ((m_settings.diffType == GraphicalDiff) && (files.count() == 1)) {
        QString file = files.first();
        if (m_statusMap->value(file).status == FileStatus::Hijacked)
            diffGraphical(ccGetFileVersion(workingDir, file), file);
        else
            diffGraphical(file);
        return; // done here, diff is opened in a new window
    }
    if (!m_settings.extDiffAvailable) {
        VcsBase::VcsBaseOutputWindow::instance()->appendError(
                    tr("External diff is required to compare multiple files."));
        return;
    }
    QString result;
    foreach (const QString &file, files) {
        if (m_statusMap->value(QDir::fromNativeSeparators(file)).status == FileStatus::Hijacked)
            result += diffExternal(ccGetFileVersion(workingDir, file), file);
        else
            result += diffExternal(file);
    }

    QString diffname;

    // diff of a single file? re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    const QString tag = VcsBase::VcsBaseEditorWidget::editorTag(VcsBase::DiffOutput, workingDir, files);
    if (files.count() == 1) {
        // Show in the same editor if diff has been executed before
        if (Core::IEditor *existingEditor = VcsBase::VcsBaseEditorWidget::locateEditorByTag(tag)) {
            existingEditor->createNew(result);
            Core::EditorManager::activateEditor(existingEditor, Core::EditorManager::ModeSwitch);
            setDiffBaseDirectory(existingEditor, workingDir);
            return;
        }
        diffname = QDir::toNativeSeparators(files.first());
    }
    const QString title = QString::fromLatin1("cc diff %1").arg(diffname);
    Core::IEditor *editor = showOutputInEditor(title, result, VcsBase::DiffOutput, source, codec);
    setDiffBaseDirectory(editor, workingDir);
    VcsBase::VcsBaseEditorWidget::tagEditor(editor, tag);
    ClearCaseEditor *diffEditorWidget = qobject_cast<ClearCaseEditor *>(editor->widget());
    QTC_ASSERT(diffEditorWidget, return);
    if (files.count() == 1)
        editor->setProperty("originalFileName", diffname);
}

QStringList ClearCasePlugin::ccGetActivityVersions(const QString &workingDir, const QString &activity)
{
    QStringList args(QLatin1String("lsactivity"));
    args << QLatin1String("-fmt") << QLatin1String("%[versions]Cp") << activity;
    const ClearCaseResponse response =
        runCleartool(workingDir, args, m_settings.timeOutMS(), SilentRun);
    if (response.error)
        return QStringList();
    QStringList versions = response.stdOut.split(QLatin1String(", "));
    versions.sort();
    return versions;
}

void ClearCasePlugin::rmdir(const QString &path)
{
    QDir dir(path);
    foreach (QFileInfo fi, dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
        if (fi.isDir()) {
            rmdir(fi.canonicalFilePath());
            dir.rmdir(fi.baseName());
        }
        else
            QFile::remove(fi.canonicalFilePath());
    }
}

void ClearCasePlugin::diffActivity()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    if (ClearCase::Constants::debug)
        qDebug() << Q_FUNC_INFO;
    if (!m_settings.extDiffAvailable) {
        VcsBase::VcsBaseOutputWindow::instance()->appendError(
                    tr("External diff is required to compare multiple files."));
        return;
    }
    QString topLevel = state.topLevel();
    QString activity = QInputDialog::getText(0, tr("Enter Activity"), tr("Activity Name"), QLineEdit::Normal, m_activity);
    if (activity.isEmpty())
        return;
    QStringList versions = ccGetActivityVersions(topLevel, activity);

    QString result;
    // map from fileName to (first, latest) pair
    QMap<QString, QStringPair> filever;
    int topLevelLen = topLevel.length();
    foreach (const QString &version, versions) {
        QString shortver = version.mid(topLevelLen + 1);
        int atatpos = shortver.indexOf(QLatin1String("@@"));
        if (atatpos != -1) {
            QString file = shortver.left(atatpos);
            // latest version - updated each line
            filever[file].second = shortver;

            // pre-first version. only for the first occurence
            if (filever[file].first.isEmpty()) {
                int verpos = shortver.lastIndexOf(QRegExp(QLatin1String("[^0-9]"))) + 1;
                int vernum = shortver.mid(verpos).toInt();
                if (vernum)
                    --vernum;
                shortver.replace(verpos, shortver.length() - verpos, QString::number(vernum));
                // first version
                filever[file].first = shortver;
            }
        }
    }

    if ((m_settings.diffType == GraphicalDiff) && (filever.count() == 1)) {
        QStringPair pair(filever.values().at(0));
        diffGraphical(pair.first, pair.second);
        return;
    }
    rmdir(QDir::tempPath() + QLatin1String("/ccdiff/") + activity);
    QDir(QDir::tempPath()).rmpath(QLatin1String("ccdiff/") + activity);
    m_diffPrefix = activity;
    foreach (const QString &file, filever.keys()) {
        QStringPair pair(filever.value(file));
        if (pair.first.contains(QLatin1String("CHECKEDOUT")))
            pair.first = ccGetPredecessor(pair.first.left(pair.first.indexOf(QLatin1String("@@"))));
        result += diffExternal(pair.first, pair.second, true);
    }
    m_diffPrefix.clear();
    const QString title = QString::fromLatin1("%1.patch").arg(activity);
    Core::IEditor *editor = showOutputInEditor(title, result, VcsBase::DiffOutput, activity, 0);
    setDiffBaseDirectory(editor, topLevel);
}

void ClearCasePlugin::diffCurrentFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    ccDiffWithPred(state.topLevel(), QStringList(state.relativeCurrentFile()));
}

void ClearCasePlugin::startCheckInCurrentFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    QString nativeFile = QDir::toNativeSeparators(state.relativeCurrentFile());
    startCheckIn(state.currentFileTopLevel(), QStringList(nativeFile));
}

void ClearCasePlugin::startCheckInAll()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    QString topLevel = state.topLevel();
    QStringList files;
    for (StatusMap::ConstIterator iterator = m_statusMap->constBegin();
         iterator != m_statusMap->constEnd();
         ++iterator)
    {
        if (iterator.value().status == FileStatus::CheckedOut)
            files.append(QDir::toNativeSeparators(iterator.key()));
    }
    files.sort();
    startCheckIn(topLevel, files);
}

void ClearCasePlugin::startCheckInActivity()
{
    QTC_ASSERT(isUcm(), return);

    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);

    QDialog dlg;
    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    ActivitySelector *actSelector = new ActivitySelector(&dlg);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dlg);
    connect(buttonBox, SIGNAL(accepted()), &dlg, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), &dlg, SLOT(reject()));
    layout->addWidget(actSelector);
    layout->addWidget(buttonBox);
    dlg.setWindowTitle(tr("Check In Activity"));
    if (!dlg.exec())
        return;

    QString topLevel = state.topLevel();
    int topLevelLen = topLevel.length();
    QStringList versions = ccGetActivityVersions(topLevel, actSelector->activity());
    QStringList files;
    QString last;
    foreach (const QString &version, versions) {
        int atatpos = version.indexOf(QLatin1String("@@"));
        if ((atatpos != -1) && (version.indexOf(QLatin1String("CHECKEDOUT"), atatpos) != -1)) {
            QString file = version.left(atatpos);
            if (file != last)
                files.append(file.mid(topLevelLen+1));
            last = file;
        }
    }
    files.sort();
    startCheckIn(topLevel, files);
}

/* Start check in of files of a single repository by displaying
 * template and files in a submit editor. On closing, the real
 * check in will start. */
void ClearCasePlugin::startCheckIn(const QString &workingDir, const QStringList &files)
{
    if (VcsBase::VcsBaseSubmitEditor::raiseSubmitEditor())
        return;
    VcsBase::VcsBaseOutputWindow *outputwindow = VcsBase::VcsBaseOutputWindow::instance();

    if (isCheckInEditorOpen()) {
        outputwindow->appendWarning(tr("Another check in is currently being executed."));
        return;
    }

    // Get list of added/modified/deleted files
    if (files.empty()) {
        outputwindow->appendWarning(tr("There are no modified files."));
        return;
    }
    // Create a new submit change file containing the submit template
    Utils::TempFileSaver saver;
    saver.setAutoRemove(false);
    // TODO: Retrieve submit template from
    const QString submitTemplate;
    // Create a submit
    saver.write(submitTemplate.toUtf8());
    if (!saver.finalize()) {
        VcsBase::VcsBaseOutputWindow::instance()->appendError(saver.errorString());
        return;
    }
    m_checkInMessageFileName = saver.fileName();
    m_checkInView = workingDir;
    // Create a submit editor and set file list
    ClearCaseSubmitEditor *editor = openClearCaseSubmitEditor(m_checkInMessageFileName, m_viewData.isUcm);
    editor->setStatusList(files);

    if (m_viewData.isUcm && (files.size() == 1)) {
        QString activity = ccGetFileActivity(workingDir, files.first());
        editor->submitEditorWidget()->setActivity(activity);
    }
}

void ClearCasePlugin::historyCurrentFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    history(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()), true);
}

void ClearCasePlugin::updateView()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    ccUpdate(state.topLevel());
}

void ClearCasePlugin::history(const QString &workingDir,
                               const QStringList &files,
                               bool enableAnnotationContextMenu)
{
    QTextCodec *codec = VcsBase::VcsBaseEditorWidget::getCodec(workingDir, files);
    // no need for temp file
    QStringList args(QLatin1String("lshistory"));
    if (m_settings.historyCount > 0)
        args << QLatin1String("-last") << QString::number(m_settings.historyCount);
    if (!m_intStream.isEmpty())
        args << QLatin1String("-branch") << m_intStream;
    foreach (const QString &file, files)
        args.append(QDir::toNativeSeparators(file));

    const ClearCaseResponse response =
            runCleartool(workingDir, args, m_settings.timeOutMS(),
                   0, codec);
    if (response.error)
        return;

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file

    const QString id = VcsBase::VcsBaseEditorWidget::getTitleId(workingDir, files);
    const QString tag = VcsBase::VcsBaseEditorWidget::editorTag(VcsBase::LogOutput, workingDir, files);
    if (Core::IEditor *editor = VcsBase::VcsBaseEditorWidget::locateEditorByTag(tag)) {
        editor->createNew(response.stdOut);
        Core::EditorManager::activateEditor(editor, Core::EditorManager::ModeSwitch);
    } else {
        const QString title = QString::fromLatin1("cc history %1").arg(id);
        const QString source = VcsBase::VcsBaseEditorWidget::getSource(workingDir, files);
        Core::IEditor *newEditor = showOutputInEditor(title, response.stdOut, VcsBase::LogOutput, source, codec);
        VcsBase::VcsBaseEditorWidget::tagEditor(newEditor, tag);
        if (enableAnnotationContextMenu)
            VcsBase::VcsBaseEditorWidget::getVcsBaseEditor(newEditor)->setFileLogAnnotateEnabled(true);
    }
}

void ClearCasePlugin::viewStatus()
{
    if (m_viewData.name.isEmpty())
        m_viewData = ccGetView(m_topLevel);
    QTC_ASSERT(!m_viewData.name.isEmpty() && !m_settings.disableIndexer, return);
    VcsBase::VcsBaseOutputWindow *outputwindow = VcsBase::VcsBaseOutputWindow::instance();
    outputwindow->appendCommand(QLatin1String("Indexed files status (C=Checked Out, H=Hijacked, ?=Missing)"));
    bool anymod = false;
    for (StatusMap::ConstIterator it = m_statusMap->constBegin();
         it != m_statusMap->constEnd();
         ++it)
    {
        char cstat = 0;
        switch (it.value().status) {
            case FileStatus::CheckedOut: cstat = 'C'; break;
            case FileStatus::Hijacked:   cstat = 'H'; break;
            case FileStatus::Missing:    cstat = '?'; break;
            default: break;
        }
        if (cstat) {
            outputwindow->append(QString::fromLatin1("%1    %2\n")
                           .arg(cstat)
                           .arg(QDir::toNativeSeparators(it.key())));
            anymod = true;
        }
    }
    if (!anymod)
        outputwindow->appendWarning(QLatin1String("No modified files found!"));
}

void ClearCasePlugin::ccUpdate(const QString &workingDir, const QStringList &relativePaths)
{
    QStringList args(QLatin1String("update"));
    args << QLatin1String("-noverwrite");
    if (!relativePaths.isEmpty())
        args.append(relativePaths);
        const ClearCaseResponse response =
                runCleartool(workingDir, args, m_settings.longTimeOutMS(), ShowStdOutInLogWindow);
    if (!response.error)
        clearCaseControl()->emitRepositoryChanged(workingDir);
}

void ClearCasePlugin::annotateCurrentFile()
{
    const VcsBase::VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    vcsAnnotate(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void ClearCasePlugin::annotateVersion(const QString &file,
                                       const QString &revision,
                                       int lineNr)
{
    const QFileInfo fi(file);
    vcsAnnotate(fi.absolutePath(), fi.fileName(), revision, lineNr);
}

void ClearCasePlugin::vcsAnnotate(const QString &workingDir, const QString &file,
                                const QString &revision /* = QString() */,
                                int lineNumber /* = -1 */) const
{
    if (ClearCase::Constants::debug)
        qDebug() << Q_FUNC_INFO << file;

    QTextCodec *codec = VcsBase::VcsBaseEditorWidget::getCodec(file);

    // Determine id
    QString id = file;
    if (!revision.isEmpty())
        id += QLatin1String("@@") + revision;

    QStringList args(QLatin1String("annotate"));
    args << QLatin1String("-nco") << QLatin1String("-f");
    args << QLatin1String("-fmt") << QLatin1String("%-14.14Sd %-8.8u | ");
    args << QLatin1String("-out") << QLatin1String("-");
    args.append(QDir::toNativeSeparators(id));

    const ClearCaseResponse response =
            runCleartool(workingDir, args, m_settings.timeOutMS(), 0, codec);
    if (response.error)
        return;

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    const QString source = workingDir + QLatin1Char('/') + file;
    if (lineNumber <= 0)
        lineNumber = VcsBase::VcsBaseEditorWidget::lineNumberOfCurrentEditor(source);

    QString headerSep(QLatin1String("-------------------------------------------------"));
    int pos = qMax(0, response.stdOut.indexOf(headerSep));
    // there are 2 identical headerSep lines - skip them
    int dataStart = response.stdOut.indexOf(QLatin1Char('\n'), pos) + 1;
    dataStart = response.stdOut.indexOf(QLatin1Char('\n'), dataStart) + 1;
    QString res;
    QTextStream stream(&res, QIODevice::WriteOnly | QIODevice::Text);
    stream << response.stdOut.mid(dataStart) << headerSep << QLatin1Char('\n')
           << headerSep << QLatin1Char('\n') << response.stdOut.left(pos);
    const QStringList files = QStringList(file);
    const QString tag = VcsBase::VcsBaseEditorWidget::editorTag(VcsBase::AnnotateOutput, workingDir, files);
    if (Core::IEditor *editor = VcsBase::VcsBaseEditorWidget::locateEditorByTag(tag)) {
        editor->createNew(res);
        VcsBase::VcsBaseEditorWidget::gotoLineOfEditor(editor, lineNumber);
        Core::EditorManager::activateEditor(editor, Core::EditorManager::ModeSwitch);
    } else {
        const QString title = QString::fromLatin1("cc annotate %1").arg(id);
        Core::IEditor *newEditor = showOutputInEditor(title, res, VcsBase::AnnotateOutput, source, codec);
        VcsBase::VcsBaseEditorWidget::tagEditor(newEditor, tag);
        VcsBase::VcsBaseEditorWidget::gotoLineOfEditor(newEditor, lineNumber);
    }
}

void ClearCasePlugin::describe(const QString &source, const QString &changeNr)
{
    const QFileInfo fi(source);
    QString topLevel;
    const bool manages = managesDirectory(fi.isDir() ? source : fi.absolutePath(), &topLevel);
    if (!manages || topLevel.isEmpty())
        return;
    if (ClearCase::Constants::debug)
        qDebug() << Q_FUNC_INFO << source << topLevel << changeNr;
    QString description;
    QString relPath = QDir::toNativeSeparators(QDir(topLevel).relativeFilePath(source));
    QString id = QString::fromLatin1("%1@@%2").arg(relPath).arg(changeNr);

    QStringList args(QLatin1String("describe"));
    args.push_back(id);
    QTextCodec *codec = VcsBase::VcsBaseEditorWidget::getCodec(source);
    const ClearCaseResponse response =
            runCleartool(topLevel, args, m_settings.timeOutMS(), 0, codec);
    description = response.stdOut;
    if (m_settings.extDiffAvailable)
        description += diffExternal(id);

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    const QString tag = VcsBase::VcsBaseEditorWidget::editorTag(VcsBase::DiffOutput, source, QStringList(), changeNr);
    if (Core::IEditor *editor = VcsBase::VcsBaseEditorWidget::locateEditorByTag(tag)) {
        editor->createNew(description);
        Core::EditorManager::activateEditor(editor, Core::EditorManager::ModeSwitch);
    } else {
        const QString title = QString::fromLatin1("cc describe %1").arg(id);
        Core::IEditor *newEditor = showOutputInEditor(title, description, VcsBase::DiffOutput, source, codec);
        VcsBase::VcsBaseEditorWidget::tagEditor(newEditor, tag);
    }
}

void ClearCasePlugin::checkInSelected()
{
    m_submitActionTriggered = true;
    Core::EditorManager::instance()->closeEditor();
}

QString ClearCasePlugin::runCleartoolSync(const QString &workingDir,
                                          const QStringList &arguments) const
{
    return runCleartool(workingDir, arguments, m_settings.timeOutMS(), SilentRun).stdOut;
}

ClearCaseResponse
        ClearCasePlugin::runCleartool(const QString &workingDir,
                                 const QStringList &arguments,
                                 int timeOut,
                                 unsigned flags,
                                 QTextCodec *outputCodec) const
{
    const QString executable = m_settings.ccBinaryPath;
    ClearCaseResponse response;
    if (executable.isEmpty()) {
        response.error = true;
        response.message = tr("No ClearCase executable specified.");
        return response;
    }

    const Utils::SynchronousProcessResponse sp_resp =
            VcsBase::VcsBasePlugin::runVcs(workingDir, executable,
                                           arguments, timeOut, flags, outputCodec);

    response.error = sp_resp.result != Utils::SynchronousProcessResponse::Finished;
    if (response.error)
        response.message = sp_resp.exitMessage(executable, timeOut);
    response.stdErr = sp_resp.stdErr;
    response.stdOut = sp_resp.stdOut;
    return response;
}

Core::IEditor *ClearCasePlugin::showOutputInEditor(const QString& title, const QString &output,
                                                   int editorType, const QString &source,
                                                   QTextCodec *codec) const
{
    const VcsBase::VcsBaseEditorParameters *params = findType(editorType);
    QTC_ASSERT(params, return 0);
    const Core::Id id = Core::Id(QByteArray(params->id));
    if (ClearCase::Constants::debug)
        qDebug() << "ClearCasePlugin::showOutputInEditor" << title << id.name()
                 <<  "Size= " << output.size() <<  " Type=" << editorType << debugCodec(codec);
    QString s = title;
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(id, &s, output);
    connect(editor, SIGNAL(annotateRevisionRequested(QString,QString,int)),
            this, SLOT(annotateVersion(QString,QString,int)));
    ClearCaseEditor *e = qobject_cast<ClearCaseEditor*>(editor->widget());
    if (!e)
        return 0;
    e->setForceReadOnly(true);
    s.replace(QLatin1Char(' '), QLatin1Char('_'));
    e->setSuggestedFileName(s);
    if (!source.isEmpty())
        e->setSource(source);
    if (codec)
        e->setCodec(codec);
    Core::IEditor *ie = e->editor();
    Core::EditorManager::activateEditor(ie, Core::EditorManager::ModeSwitch);
    return ie;
}

const ClearCaseSettings &ClearCasePlugin::settings() const
{
    return m_settings;
}

void ClearCasePlugin::setSettings(const ClearCaseSettings &s)
{
    if (s != m_settings) {
        m_settings = s;
        m_settings.toSettings(Core::ICore::settings());
        clearCaseControl()->emitConfigurationChanged();
    }
}

ClearCasePlugin *ClearCasePlugin::instance()
{
    QTC_ASSERT(m_clearcasePluginInstance, return m_clearcasePluginInstance);
    return m_clearcasePluginInstance;
}

bool ClearCasePlugin::vcsOpen(const QString &workingDir, const QString &fileName)
{
    QTC_ASSERT(currentState().hasTopLevel(), return false);

    if (ClearCase::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDir << fileName;

    QFileInfo fi(workingDir, fileName);
    QString topLevel = currentState().topLevel();
    QString absPath = fi.absoluteFilePath();
    const QString relFile = QDir(topLevel).relativeFilePath(absPath);

    const QString file = QDir::toNativeSeparators(relFile);
    const QString title = QString::fromLatin1("Checkout %1").arg(file);
    CheckOutDialog coDialog(title, m_viewData.isUcm);

    if (!m_settings.disableIndexer &&
            (fi.isWritable() || m_statusMap->value(relFile).status == FileStatus::Unknown))
        QtConcurrent::run(&sync, topLevel, QStringList(relFile)).waitForFinished();
    if (m_statusMap->value(relFile).status == FileStatus::CheckedOut) {
        QMessageBox::information(0, tr("ClearCase Checkout"), tr("File is already checked out."));
        return true;
    }
    // Only snapshot views can have hijacked files
    bool isHijacked = (!m_viewData.isDynamic && (m_statusMap->value(relFile).status & FileStatus::Hijacked));
    if (!isHijacked)
        coDialog.hideHijack();
    if (coDialog.exec() == QDialog::Accepted) {
        if (m_viewData.isUcm && !vcsSetActivity(topLevel, title, coDialog.activity()))
            return false;

        Core::FileChangeBlocker fcb(absPath);
        QStringList args(QLatin1String("checkout"));
        QString comment = coDialog.comment();
        if (comment.isEmpty())
            args << QLatin1String("-nc");
        else
            args << QLatin1String("-c") << comment;
        args << QLatin1String("-query");
        if (coDialog.isReserved())
            args << QLatin1String("-reserved");
        if (coDialog.isUnreserved())
            args << QLatin1String("-unreserved");
        if (coDialog.isPreserveTime())
            args << QLatin1String("-ptime");
        if (isHijacked) {
            if (ClearCase::Constants::debug)
                qDebug() << Q_FUNC_INFO << file << " seems to be hijacked";

            // A hijacked files means that the file is modified but was
            // not checked out. By checking it out now changes will
            // be lost, unless handled. This can be done by renaming
            // the hijacked file, undoing the hijack and updating the file

            // -usehijack not supported in old cleartool versions...
            // args << QLatin1String("-usehijack");
            if (coDialog.isUseHijacked())
                QFile::rename(absPath, absPath + QLatin1String(".hijack"));
            vcsUndoHijack(topLevel, relFile, false); // don't keep, we've already kept a copy
        }
        args << file;
        ClearCaseResponse response =
                runCleartool(topLevel, args, m_settings.timeOutMS(), ShowStdOutInLogWindow |
                             SuppressStdErrInLogWindow | FullySynchronously);
        if (response.error) {
            if (response.stdErr.contains(QLatin1String("Versions other than the selected version"))) {
                VersionSelector selector(file, response.stdErr);
                if (selector.exec() == QDialog::Accepted) {
                    if (selector.isUpdate())
                        ccUpdate(workingDir, QStringList() << file);
                    else
                        args.removeOne(QLatin1String("-query"));
                    response = runCleartool(topLevel, args, m_settings.timeOutMS(),
                                            ShowStdOutInLogWindow | FullySynchronously);
                }
            } else {
                VcsBase::VcsBaseOutputWindow *outputWindow = VcsBase::VcsBaseOutputWindow::instance();
                outputWindow->append(response.stdOut);
                outputWindow->append(response.stdErr);
            }
        }

        if (!response.error && isHijacked && coDialog.isUseHijacked()) { // rename back
            QFile::remove(absPath);
            QFile::rename(absPath + QLatin1String(".hijack"), absPath);
        }

        if ((!response.error || response.stdOut.contains(QLatin1String("already checked out"))) && !m_settings.disableIndexer)
            setStatus(relFile, FileStatus::CheckedOut);
        return !response.error;
    }
    return true;
}

bool ClearCasePlugin::vcsSetActivity(const QString &workingDir, const QString &title, const QString &activity)
{
    QStringList args;
    args << QLatin1String("setactivity") << activity;
    const ClearCaseResponse actResponse =
            runCleartool(workingDir, args, m_settings.timeOutMS(), ShowStdOutInLogWindow);
    if (actResponse.error) {
        QMessageBox::warning(0, title, tr("Set current activity failed: %1").arg(actResponse.message), QMessageBox::Ok);
        return false;
    }
    m_activity = activity;
    return true;
}

// files are received using native separators
bool ClearCasePlugin::vcsCheckIn(const QString &messageFile, const QStringList &files, const QString &activity,
                                 bool isIdentical, bool isPreserve, bool replaceActivity)
{
    if (ClearCase::Constants::debug)
        qDebug() << Q_FUNC_INFO << messageFile << files << activity;
    if (files.isEmpty())
        return true;
    const QString title = QString::fromLatin1("Checkin %1").arg(files.join(QLatin1String("; ")));
    typedef QSharedPointer<Core::FileChangeBlocker> FCBPointer;
    replaceActivity &= (activity != QLatin1String(Constants::KEEP_ACTIVITY));
    if (replaceActivity && !vcsSetActivity(m_checkInView, title, activity))
        return false;
    QFile msgFile(messageFile);
    msgFile.open(QFile::ReadOnly | QFile::Text);
    QString message = QString::fromLocal8Bit(msgFile.readAll().trimmed().constData());
    msgFile.close();
    QStringList args;
    args << QLatin1String("checkin");
    if (message.isEmpty())
        args << QLatin1String("-nc");
    else
        args << QLatin1String("-cfile") << messageFile;
    if (isIdentical)
        args << QLatin1String("-identical");
    if (isPreserve)
        args << QLatin1String("-ptime");
    args << files;
    QList<FCBPointer> blockers;
    foreach (QString fileName, files) {
        FCBPointer fcb(new Core::FileChangeBlocker(QFileInfo(m_checkInView, fileName).canonicalFilePath()));
        blockers.append(fcb);
    }
    const ClearCaseResponse response =
            runCleartool(m_checkInView, args, m_settings.longTimeOutMS(), ShowStdOutInLogWindow);
    QRegExp checkedIn(QLatin1String("Checked in \\\"([^\"]*)\\\""));
    bool anySucceeded = false;
    int offset = checkedIn.indexIn(response.stdOut);
    while (offset != -1) {
        QString file = checkedIn.cap(1);
        if (!m_settings.disableIndexer)
            setStatus(QDir::fromNativeSeparators(file), FileStatus::CheckedIn);
        clearCaseControl()->emitFilesChanged(files);
        anySucceeded = true;
        offset = checkedIn.indexIn(response.stdOut, offset + 12);
    }
    return anySucceeded;
}

bool ClearCasePlugin::ccFileOp(const QString &workingDir, const QString &title, const QStringList &opArgs,
                               const QString &fileName, const QString &file2)
{
    const QString file = QDir::toNativeSeparators(fileName);
    bool noCheckout = false;
    QVBoxLayout *verticalLayout;
    ActivitySelector *actSelector = 0;
    QLabel *commentLabel;
    QTextEdit *commentEdit;
    QDialogButtonBox *buttonBox;
    QDialog fileOpDlg;
    fileOpDlg.setWindowTitle(title);

    verticalLayout = new QVBoxLayout(&fileOpDlg);
    if (m_viewData.isUcm) {
        actSelector = new ActivitySelector;
        verticalLayout->addWidget(actSelector);
    }

    commentLabel = new QLabel(tr("Enter &comment:"));
    verticalLayout->addWidget(commentLabel);

    commentEdit = new QTextEdit;
    verticalLayout->addWidget(commentEdit);

    buttonBox = new QDialogButtonBox;
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    verticalLayout->addWidget(buttonBox);

#ifndef QT_NO_SHORTCUT
    commentLabel->setBuddy(commentEdit);
#endif // QT_NO_SHORTCUT

    connect(buttonBox, SIGNAL(accepted()), &fileOpDlg, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), &fileOpDlg, SLOT(reject()));

    if (!fileOpDlg.exec())
        return true;

    QString comment = commentEdit->toPlainText();
    if (m_viewData.isUcm && actSelector->changed())
        vcsSetActivity(workingDir, fileOpDlg.windowTitle(), actSelector->activity());

    QString dirName = QDir::toNativeSeparators(QFileInfo(workingDir, fileName).absolutePath());
    QStringList commentArg;
    if (comment.isEmpty())
        commentArg << QLatin1String("-nc");
    else
        commentArg << QLatin1String("-c") << comment;

    // check out directory
    QStringList args;
    args << QLatin1String("checkout") << commentArg << dirName;
    const ClearCaseResponse coResponse =
        runCleartool(workingDir, args, m_settings.timeOutMS(),
                     ShowStdOutInLogWindow | FullySynchronously);
    if (coResponse.error) {
        if (coResponse.stdOut.contains(QLatin1String("already checked out")))
            noCheckout = true;
        else
            return false;
    }

    // do the file operation
    args.clear();
    args << opArgs << commentArg << file;
    if (!file2.isEmpty())
        args << QDir::toNativeSeparators(file2);
    const ClearCaseResponse opResponse =
            runCleartool(workingDir, args, m_settings.timeOutMS(),
                         ShowStdOutInLogWindow | FullySynchronously);
    if (opResponse.error) {
        // on failure - undo checkout for the directory
        if (!noCheckout)
            vcsUndoCheckOut(workingDir, dirName, false);
        return false;
    }

    if (!noCheckout) {
        // check in the directory
        args.clear();
        args << QLatin1String("checkin") << commentArg << dirName;
        const ClearCaseResponse ciResponse =
            runCleartool(workingDir, args, m_settings.timeOutMS(),
                         ShowStdOutInLogWindow | FullySynchronously);
        return !ciResponse.error;
    }
    return true;
}

static QString baseName(const QString &fileName)
{
    return fileName.mid(fileName.lastIndexOf(QLatin1Char('/')) + 1);
}

bool ClearCasePlugin::vcsAdd(const QString &workingDir, const QString &fileName)
{
    return ccFileOp(workingDir, tr("ClearCase Add File %1").arg(baseName(fileName)),
                    QStringList() << QLatin1String("mkelem") << QLatin1String("-ci"), fileName);
}

bool ClearCasePlugin::vcsDelete(const QString &workingDir, const QString &fileName)
{
    const QString title(tr("ClearCase Remove Element %1").arg(baseName(fileName)));
    if (QMessageBox::warning(0, title, tr("This operation is irreversible. Are you sure?"),
                         QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
        return true;

    return ccFileOp(workingDir, tr("ClearCase Remove File %1").arg(baseName(fileName)),
                    QStringList() << QLatin1String("rmname") << QLatin1String("-force"), fileName);
}

bool ClearCasePlugin::vcsMove(const QString &workingDir, const QString &from, const QString &to)
{
    return ccFileOp(workingDir, tr("ClearCase Rename File %1 -> %2")
                    .arg(baseName(from)).arg(baseName(to)),
                    QStringList() << QLatin1String("move"), from, to);
}

bool ClearCasePlugin::vcsCheckout(const QString & /*directory*/, const QByteArray & /*url*/)
{
    return false;
}

QString ClearCasePlugin::vcsGetRepositoryURL(const QString & /*directory*/)
{
    return currentState().topLevel();
}

// ClearCase has "view.dat" file in the root directory it manages for snapshot views.
bool ClearCasePlugin::managesDirectory(const QString &directory, QString *topLevel /* = 0 */) const
{
    QString topLevelFound = findTopLevel(directory);
    if (topLevel)
        *topLevel = topLevelFound;
    return !topLevelFound.isEmpty();
}

ClearCaseControl *ClearCasePlugin::clearCaseControl() const
{
    return static_cast<ClearCaseControl *>(versionControl());
}

QString ClearCasePlugin::ccGetCurrentActivity() const
{
    QStringList args(QLatin1String("lsactivity"));
    args << QLatin1String("-cact");
    args << QLatin1String("-fmt") << QLatin1String("%n");
    return runCleartoolSync(currentState().topLevel(), args);
}

QList<QStringPair> ClearCasePlugin::ccGetActivities() const
{
    QList<QStringPair> result;
    // Maintain latest deliver and rebase activities only
    QStringPair rebaseAct;
    QStringPair deliverAct;
    // Retrieve all activities
    QStringList args(QLatin1String("lsactivity"));
    args << QLatin1String("-fmt") << QLatin1String("%n\\t%[headline]p\\n");
    const QString response = runCleartoolSync(currentState().topLevel(), args);
    QStringList acts = response.split(QLatin1Char('\n'), QString::SkipEmptyParts);
    foreach (QString activity, acts) {
        QStringList act = activity.split(QLatin1Char('\t'));
        if (act.size() >= 2)
        {
            QString actName = act.at(0);
            // include only latest deliver/rebase activities. Activities are sorted
            // by creation time
            if (actName.startsWith(QLatin1String("rebase.")))
                rebaseAct = QStringPair(actName, act.at(1));
            else if (actName.startsWith(QLatin1String("deliver.")))
                deliverAct = QStringPair(actName, act.at(1));
            else
                result.append(QStringPair(actName, act.at(1).trimmed()));
        }
    }
    qSort(result);
    if (!rebaseAct.first.isEmpty())
        result.append(rebaseAct);
    if (!deliverAct.first.isEmpty())
        result.append(deliverAct);
    return result;
}

void ClearCasePlugin::refreshActivities()
{
    QMutexLocker locker(m_activityMutex);
    m_activity = ccGetCurrentActivity();
    m_activities = ccGetActivities();
}

QList<QStringPair> ClearCasePlugin::activities(int *current) const
{
    QList<QStringPair> activitiesList;
    QString curActivity;
    const VcsBase::VcsBasePluginState state = currentState();
    if (state.topLevel() == state.currentProjectTopLevel()) {
        QMutexLocker locker(m_activityMutex);
        activitiesList = m_activities;
        curActivity = m_activity;
    } else {
        activitiesList = ccGetActivities();
        curActivity = ccGetCurrentActivity();
    }
    if (current) {
        int nActivities = activitiesList.size();
        *current = -1;
        for (int i = 0; i < nActivities && (*current == -1); ++i) {
            if (activitiesList[i].first == curActivity)
                *current = i;
        }
    }
    return activitiesList;
}

bool ClearCasePlugin::newActivity()
{
    QString workingDir = currentState().topLevel();
    QStringList args;
    args << QLatin1String("mkactivity") << QLatin1String("-f");
    if (!m_settings.autoAssignActivityName) {
        QString headline = QInputDialog::getText(0, tr("Activity Headline"), tr("Enter activity headline"));
        if (headline.isEmpty())
            return false;
        args << QLatin1String("-headline") << headline;
    }

    const ClearCaseResponse response =
            runCleartool(workingDir, args, m_settings.timeOutMS(), 0);

    if (!response.error)
        refreshActivities();
    return (!response.error);
}

// check if the view is UCM
bool ClearCasePlugin::ccCheckUcm(const QString &viewname, const QString &workingDir) const
{
    QStringList catcsArgs(QLatin1String("catcs"));
    catcsArgs << QLatin1String("-tag") << viewname;
    QString catcsData = runCleartoolSync(workingDir, catcsArgs);

    // check output for the word "ucm"
    return QRegExp(QLatin1String("(^|\\n)ucm\\n")).indexIn(catcsData) != -1;
}

ViewData ClearCasePlugin::ccGetView(const QString &workingDir) const
{
    static QHash<QString, ViewData> viewCache;

    bool inCache = viewCache.contains(workingDir);
    ViewData &res = viewCache[workingDir];
    if (!inCache) {
        QStringList args(QLatin1String("lsview"));
        args << QLatin1String("-cview");
        QString data = runCleartoolSync(workingDir, args);
        res.isDynamic = !data.isEmpty() && (data.at(0) == QLatin1Char('*'));
        res.name = data.mid(2, data.indexOf(QLatin1Char(' '), 2) - 2);
        res.isUcm = ccCheckUcm(res.name, workingDir);
    }

    return res;
}

void ClearCasePlugin::updateStreamAndView()
{
    QStringList args(QLatin1String("lsstream"));
    args << QLatin1String("-fmt") << QLatin1String("%n\\t%[def_deliver_tgt]Xp");
    const QString sresponse = runCleartoolSync(m_topLevel, args);
    int tabPos = sresponse.indexOf(QLatin1Char('\t'));
    m_stream = sresponse.left(tabPos);
    QRegExp intStreamExp(QLatin1String("stream:([^@]*)"));
    if (intStreamExp.indexIn(sresponse.mid(tabPos + 1)) != -1)
        m_intStream = intStreamExp.cap(1);
    m_viewData = ccGetView(m_topLevel);
    m_updateViewAction->setParameter(m_viewData.isDynamic ? QString() : m_viewData.name);
}

void ClearCasePlugin::projectChanged(ProjectExplorer::Project *project)
{
    if (m_viewData.name == ccGetView(m_topLevel).name) // New project on same view as old project
        return;
    m_viewData = ViewData();
    m_stream.clear();
    m_intStream.clear();
    disconnect(Core::ICore::mainWindow(), SIGNAL(windowActivated()), this, SLOT(syncSlot()));
    Core::ICore::progressManager()->cancelTasks(QLatin1String(ClearCase::Constants::TASK_INDEX));
    if (project) {
        QString projDir = project->projectDirectory();
        QString topLevel = findTopLevel(projDir);
        m_topLevel = topLevel;
        if (topLevel.isEmpty())
            return;
        connect(Core::ICore::mainWindow(), SIGNAL(windowActivated()), this, SLOT(syncSlot()));
        updateStreamAndView();
        if (m_viewData.name.isEmpty())
            return;
        updateIndex();
    }
    if ( ClearCase::Constants::debug )
        qDebug() << "stream: " << m_stream << "; intStream: " << m_intStream << "view: " << m_viewData.name;
}

void ClearCasePlugin::tasksFinished(const QString &type)
{
    if (type == QLatin1String(ClearCase::Constants::TASK_INDEX))
        m_checkInAllAction->setEnabled(true);
}

void ClearCasePlugin::updateIndex()
{
    QTC_ASSERT(currentState().hasTopLevel(), return);
    Core::ICore::progressManager()->cancelTasks(QLatin1String(ClearCase::Constants::TASK_INDEX));
    ProjectExplorer::Project *project = ProjectExplorer::ProjectExplorerPlugin::currentProject();
    if (!project)
        return;
    m_checkInAllAction->setEnabled(false);
    m_statusMap->clear();
    QFuture<void> result = QtConcurrent::run(&sync, currentState().topLevel(),
               project->files(ProjectExplorer::Project::ExcludeGeneratedFiles));
    if (!m_settings.disableIndexer)
        Core::ICore::progressManager()->addTask(result, tr("CC Indexing"),
                            QLatin1String(ClearCase::Constants::TASK_INDEX));
}

/*! retrieve a \a file (usually of the form path\to\filename.cpp@@\main\ver)
 *  from cc and save it to a temporary location which is returned
 */
QString ClearCasePlugin::getFile(const QString &nativeFile, const QString &prefix)
{
    QString tempFile;
    QDir tempDir = QDir::temp();
    tempDir.mkdir(QLatin1String("ccdiff"));
    tempDir.cd(QLatin1String("ccdiff"));
    int atatpos = nativeFile.indexOf(QLatin1String("@@"));
    QString file = QDir::fromNativeSeparators(nativeFile.left(atatpos));
    if (prefix.isEmpty()) {
        tempFile = tempDir.absoluteFilePath(QString::number(QUuid::createUuid().data1, 16));
    } else {
        tempDir.mkpath(prefix);
        tempDir.cd(prefix);
        int slash = file.lastIndexOf(QLatin1Char('/'));
        if (slash != -1)
            tempDir.mkpath(file.left(slash));
        tempFile = tempDir.absoluteFilePath(file);
    }
    if (ClearCase::Constants::debug)
        qDebug() << Q_FUNC_INFO << nativeFile;
    if ((atatpos != -1) && (nativeFile.indexOf(QLatin1String("CHECKEDOUT"), atatpos) != -1)) {
        bool res = QFile::copy(QDir(m_topLevel).absoluteFilePath(file), tempFile);
        return res ? tempFile : QString();
    }
    QStringList args(QLatin1String("get"));
    args << QLatin1String("-to") << tempFile << nativeFile;
    const ClearCaseResponse response =
            runCleartool(m_topLevel, args, m_settings.timeOutMS(), SilentRun);
    if (response.error)
        return QString();
    QFile::setPermissions(tempFile, QFile::ReadOwner | QFile::ReadUser |
                          QFile::WriteOwner | QFile::WriteUser);
    return tempFile;
}

// runs external (GNU) diff, and returns the stdout result
QString ClearCasePlugin::diffExternal(QString file1, QString file2, bool keep)
{
    QTextCodec *codec = VcsBase::VcsBaseEditorWidget::getCodec(file1);

    // if file2 is empty, we should compare to predecessor
    if (file2.isEmpty()) {
        QString predVer = ccGetPredecessor(file1);
        return (predVer.isEmpty() ? QString() : diffExternal(predVer, file1, keep));
    }

    file1 = QDir::toNativeSeparators(file1);
    file2 = QDir::toNativeSeparators(file2);
    QString tempFile1, tempFile2;
    QString prefix = m_diffPrefix;
    if (!prefix.isEmpty())
        prefix.append(QLatin1Char('/'));

    if (file1.contains(QLatin1String("@@")))
        tempFile1 = getFile(file1, prefix + QLatin1String("old"));
    if (file2.contains(QLatin1String("@@")))
        tempFile2 = getFile(file2, prefix + QLatin1String("new"));
    QStringList args;
    if (!tempFile1.isEmpty()) {
        args << QLatin1String("-L") << file1;
        args << tempFile1;
    } else {
        args << file1;
    }
    if (!tempFile2.isEmpty()) {
        args << QLatin1String("-L") << file2;
        args << tempFile2;
    } else {
        args << file2;
    }
    const QString diffResponse =
            runExtDiff(m_topLevel, args, m_settings.timeOutMS(), codec);
    if (!keep && !tempFile1.isEmpty()) {
        QFile::remove(tempFile1);
        QFileInfo(tempFile1).dir().rmpath(QLatin1String("."));
    }
    if (!keep && !tempFile2.isEmpty()) {
        QFile::remove(tempFile2);
        QFileInfo(tempFile2).dir().rmpath(QLatin1String("."));
    }
    if (diffResponse.isEmpty())
        return QLatin1String("Files are identical");
    QString header = QString::fromLatin1("diff %1 old/%2 new/%2\n")
            .arg(m_settings.diffArgs)
            .arg(QDir::fromNativeSeparators(file2.left(file2.indexOf(QLatin1String("@@")))));
    return header + diffResponse;
}

// runs builtin diff (either graphical or diff_format)
void ClearCasePlugin::diffGraphical(const QString &file1, const QString &file2)
{
    QStringList args;
    bool pred = file2.isEmpty();
    args.push_back(QLatin1String("diff"));
    if (pred)
        args.push_back(QLatin1String("-predecessor"));
    args.push_back(QLatin1String("-graphical"));
    args << file1;
    if (!pred)
        args << file2;
    QProcess::startDetached(m_settings.ccBinaryPath, args, m_topLevel);
}

QString ClearCasePlugin::runExtDiff(const QString &workingDir,
                                  const QStringList &arguments,
                                  int timeOut,
                                  QTextCodec *outputCodec)
{
    const QString executable(QLatin1String("diff"));
    QStringList args(m_settings.diffArgs.split(QLatin1Char(' '), QString::SkipEmptyParts));
    args << arguments;

    QProcess process;
    process.setWorkingDirectory(workingDir);
    process.start(executable, args);
    if (!process.waitForFinished(timeOut))
        return QString();
    QByteArray ba = process.readAll();
    return outputCodec ? outputCodec->toUnicode(ba) :
                         QString::fromLocal8Bit(ba.constData(), ba.size());
}

void ClearCasePlugin::syncSlot()
{
    VcsBase::VcsBasePluginState state = currentState();
    if (!state.hasProject() || !state.hasTopLevel())
        return;
    QString topLevel = state.topLevel();
    if (topLevel != state.currentProjectTopLevel())
        return;
    QtConcurrent::run(&sync, topLevel, QStringList());
}

void ClearCasePlugin::closing()
{
    // prevent syncSlot from being called on shutdown
    Core::ICore::progressManager()->cancelTasks(QLatin1String(ClearCase::Constants::TASK_INDEX));
    disconnect(Core::ICore::mainWindow(), SIGNAL(windowActivated()), this, SLOT(syncSlot()));
}

void ClearCasePlugin::sync(QFutureInterface<void> &future, QString topLevel, QStringList files)
{
    ClearCasePlugin *plugin = ClearCasePlugin::instance();
    ClearCaseSync ccSync(plugin, plugin->m_statusMap);
    connect(&ccSync, SIGNAL(updateStreamAndView()), plugin, SLOT(updateStreamAndView()));
    ccSync.run(future, topLevel, files);
}

#ifdef WITH_TESTS
void ClearCasePlugin::testDiffFileResolving_data()
{
    QTest::addColumn<QByteArray>("header");
    QTest::addColumn<QByteArray>("fileName");

    QTest::newRow("Modified") << QByteArray(
            "--- src/plugins/clearcase/clearcaseeditor.cpp@@/main/1\t2013-01-20 23:45:48.549615210 +0200\n"
            "+++ src/plugins/clearcase/clearcaseeditor.cpp@@/main/2\t2013-01-20 23:45:53.217604679 +0200\n"
            "@@ -58,6 +58,10 @@\n\n")
        << QByteArray("src/plugins/clearcase/clearcaseeditor.cpp");
}

void ClearCasePlugin::testDiffFileResolving()
{
    ClearCaseEditor editor(editorParameters + 3, 0);
    editor.testDiffFileResolving();
}

void ClearCasePlugin::testLogResolving()
{
    QByteArray data(
                "13-Sep.17:41   user1      create version \"src/plugins/clearcase/clearcaseeditor.h@@/main/branch1/branch2/9\" (baseline1, baseline2, ...)\n"
                "22-Aug.14:13   user2      create version \"src/plugins/clearcase/clearcaseeditor.h@@/main/branch1/branch2/8\" (baseline3, baseline4, ...)\n"
                );
    ClearCaseEditor editor(editorParameters + 1, 0);
    editor.testLogResolving(data,
                            "src/plugins/clearcase/clearcaseeditor.h@@/main/branch1/branch2/9",
                            "src/plugins/clearcase/clearcaseeditor.h@@/main/branch1/branch2/8");
}
#endif

} // namespace Internal
} // namespace ClearCase

Q_EXPORT_PLUGIN(ClearCase::Internal::ClearCasePlugin)
