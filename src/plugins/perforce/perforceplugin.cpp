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

#include "perforceplugin.h"

#include "changenumberdialog.h"
#include "pendingchangesdialog.h"
#include "perforcechecker.h"
#include "perforceeditor.h"
#include "perforcesettings.h"
#include "perforcesubmiteditor.h"
#include "settingspage.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/locator/commandlocator.h>
#include <texteditor/textdocument.h>
#include <utils/fileutils.h>
#include <utils/parameteraction.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>
#include <utils/temporarydirectory.h>

#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsbaseeditorconfig.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseplugin.h>
#include <vcsbase/vcsoutputwindow.h>

#include <QAction>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSettings>
#include <QTextCodec>

using namespace Core;
using namespace Utils;
using namespace VcsBase;
using namespace std::placeholders;

namespace Perforce {
namespace Internal {

const char SUBMIT_MIMETYPE[] = "text/vnd.qtcreator.p4.submit";

const char PERFORCE_CONTEXT[] = "Perforce Context";
const char PERFORCE_SUBMIT_EDITOR_ID[] = "Perforce.SubmitEditor";
const char PERFORCE_SUBMIT_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Perforce.SubmitEditor");

const char PERFORCE_LOG_EDITOR_ID[] = "Perforce.LogEditor";
const char PERFORCE_LOG_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Perforce Log Editor");

const char PERFORCE_DIFF_EDITOR_ID[] = "Perforce.DiffEditor";
const char PERFORCE_DIFF_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Perforce Diff Editor");

const char PERFORCE_ANNOTATION_EDITOR_ID[] = "Perforce.AnnotationEditor";
const char PERFORCE_ANNOTATION_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Perforce Annotation Editor");

// Ensure adding "..." to relative paths which is p4's convention
// for the current directory
static inline QString perforceRelativeFileArguments(const QString &args)
{
    if (args.isEmpty())
        return QLatin1String("...");
    return args + QLatin1String("/...");
}

static inline QStringList perforceRelativeProjectDirectory(const VcsBasePluginState &s)
{
    return QStringList(perforceRelativeFileArguments(s.relativeCurrentProject()));
}

// Clean user setting off diff-binary for 'p4 resolve' and 'p4 diff'.
static inline QProcessEnvironment overrideDiffEnvironmentVariable()
{
    QProcessEnvironment rc = QProcessEnvironment::systemEnvironment();
    rc.remove(QLatin1String("P4DIFF"));
    return rc;
}

const char CMD_ID_PERFORCE_MENU[] = "Perforce.Menu";
const char CMD_ID_EDIT[] = "Perforce.Edit";
const char CMD_ID_ADD[] = "Perforce.Add";
const char CMD_ID_DELETE_FILE[] = "Perforce.Delete";
const char CMD_ID_OPENED[] = "Perforce.Opened";
const char CMD_ID_PROJECTLOG[] = "Perforce.ProjectLog";
const char CMD_ID_REPOSITORYLOG[] = "Perforce.RepositoryLog";
const char CMD_ID_REVERT[] = "Perforce.Revert";
const char CMD_ID_DIFF_CURRENT[] = "Perforce.DiffCurrent";
const char CMD_ID_DIFF_PROJECT[] = "Perforce.DiffProject";
const char CMD_ID_UPDATE_PROJECT[] = "Perforce.UpdateProject";
const char CMD_ID_REVERT_PROJECT[] = "Perforce.RevertProject";
const char CMD_ID_REVERT_UNCHANGED_PROJECT[] = "Perforce.RevertUnchangedProject";
const char CMD_ID_DIFF_ALL[] = "Perforce.DiffAll";
const char CMD_ID_SUBMIT[] = "Perforce.Submit";
const char CMD_ID_PENDING_CHANGES[] = "Perforce.PendingChanges";
const char CMD_ID_DESCRIBE[] = "Perforce.Describe";
const char CMD_ID_ANNOTATE_CURRENT[] = "Perforce.AnnotateCurrent";
const char CMD_ID_ANNOTATE[] = "Perforce.Annotate";
const char CMD_ID_FILELOG_CURRENT[] = "Perforce.FilelogCurrent";
const char CMD_ID_FILELOG[] = "Perforce.Filelog";
const char CMD_ID_UPDATEALL[] = "Perforce.UpdateAll";

// Helpers

struct PerforceResponse
{
    bool error = true;
    int exitCode = -1;
    QString stdOut;
    QString stdErr;
    QString message;
};

const VcsBaseSubmitEditorParameters submitEditorParameters {
    SUBMIT_MIMETYPE,
    PERFORCE_SUBMIT_EDITOR_ID,
    PERFORCE_SUBMIT_EDITOR_DISPLAY_NAME,
    VcsBaseSubmitEditorParameters::DiffFiles
};

const VcsBaseEditorParameters logEditorParameters {
    LogOutput,
    PERFORCE_LOG_EDITOR_ID,
    PERFORCE_LOG_EDITOR_DISPLAY_NAME,
    "text/vnd.qtcreator.p4.log"
};

const VcsBaseEditorParameters annotateEditorParameters {
    AnnotateOutput,
    PERFORCE_ANNOTATION_EDITOR_ID,
    PERFORCE_ANNOTATION_EDITOR_DISPLAY_NAME,
    "text/vnd.qtcreator.p4.annotation"
};

const VcsBaseEditorParameters diffEditorParameters {
    DiffOutput,
    PERFORCE_DIFF_EDITOR_ID,
    PERFORCE_DIFF_EDITOR_DISPLAY_NAME,
    "text/x-patch"
};

// Flags for runP4Cmd.
enum RunFlags
{
    CommandToWindow = 0x1,
    StdOutToWindow = 0x2,
    StdErrToWindow = 0x4,
    ErrorToWindow = 0x8,
    OverrideDiffEnvironment = 0x10,
    // Run completely synchronously, no signals emitted
    RunFullySynchronous = 0x20,
    IgnoreExitCode = 0x40,
    ShowBusyCursor = 0x80,
    LongTimeOut = 0x100,
    SilentStdOut = 0x200,
};

struct PerforceDiffParameters
{
    QString workingDir;
    QStringList diffArguments;
    QStringList files;
};

class PerforcePluginPrivate final : public VcsBasePluginPrivate
{
    Q_DECLARE_TR_FUNCTIONS(Perforce::Internal::PerforcePlugin)

public:
    PerforcePluginPrivate();

    // IVersionControl
    QString displayName() const final { return {"perforce"}; }
    Id id() const final { return VcsBase::Constants::VCS_ID_PERFORCE; }

    bool isVcsFileOrDirectory(const FilePath &fileName) const final;
    bool managesDirectory(const QString &directory, QString *topLevel = nullptr) const final;
    bool managesFile(const QString &workingDirectory, const QString &fileName) const final;

    bool isConfigured() const final;
    bool supportsOperation(Operation operation) const final;
    OpenSupportMode openSupportMode(const QString &fileName) const final;
    bool vcsOpen(const QString &fileName) final;
    SettingsFlags settingsFlags() const final;
    bool vcsAdd(const QString &fileName) final;
    bool vcsDelete(const QString &filename) final;
    bool vcsMove(const QString &from, const QString &to) final;
    bool vcsCreateRepository(const QString &directory) final;
    void vcsAnnotate(const QString &file, int line) final;
    void vcsDescribe(const QString &source, const QString &n) final;
    QString vcsOpenText() const final;
    QString vcsMakeWritableText() const final;

    ///
    bool vcsOpen(const QString &workingDir, const QString &fileName, bool silently = false);
    bool vcsAdd(const QString &workingDir, const QString &fileName);
    bool vcsDelete(const QString &workingDir, const QString &filename);
    bool vcsMove(const QString &workingDir, const QString &from, const QString &to);

    void p4Diff(const QString &workingDir, const QStringList &files);

    IEditor *openPerforceSubmitEditor(const QString &fileName, const QStringList &depotFileNames);

    void getTopLevel(const QString &workingDirectory = QString(), bool isSync = false);

    void updateActions(ActionState) override;
    bool submitEditorAboutToClose() override;

    QString commitDisplayName() const final;
    void p4Diff(const PerforceDiffParameters &p);

    void openCurrentFile();
    void addCurrentFile();
    void revertCurrentFile();
    void printOpenedFileList();
    void diffCurrentFile();
    void diffCurrentProject();
    void updateCurrentProject();
    void revertCurrentProject();
    void revertUnchangedCurrentProject();
    void updateAll();
    void diffAllOpened();
    void startSubmitProject();
    void describeChange();
    void annotateCurrentFile();
    void annotateFile();
    void filelogCurrentFile();
    void filelogFile();
    void logProject();
    void logRepository();

    void commitFromEditor() override;
    void printPendingChanges();
    void slotSubmitDiff(const QStringList &files);
    void setTopLevel(const QString &);
    void slotTopLevelFailed(const QString &);

    class DirectoryCacheEntry
    {
    public:
        DirectoryCacheEntry(bool isManaged, const QString &topLevel):
            m_isManaged(isManaged), m_topLevel(topLevel)
        { }

        bool m_isManaged;
        QString m_topLevel;
    };

    typedef QHash<QString, DirectoryCacheEntry> ManagedDirectoryCache;

    IEditor *showOutputInEditor(const QString &title, const QString &output,
                                Id id, const QString &source,
                                QTextCodec *codec = nullptr);

    // args are passed as command line arguments
    // extra args via a tempfile and the option -x "temp-filename"
    PerforceResponse runP4Cmd(const QString &workingDir,
                              const QStringList &args,
                              unsigned flags = CommandToWindow|StdErrToWindow|ErrorToWindow,
                              const QStringList &extraArgs = {},
                              const QByteArray &stdInput = {},
                              QTextCodec *outputCodec = nullptr) const;

    PerforceResponse synchronousProcess(const QString &workingDir,
                                        const QStringList &args,
                                        unsigned flags,
                                        const QByteArray &stdInput,
                                        QTextCodec *outputCodec) const;

    PerforceResponse fullySynchronousProcess(const QString &workingDir,
                                             const QStringList &args,
                                             unsigned flags,
                                             const QByteArray &stdInput,
                                             QTextCodec *outputCodec) const;

    QString clientFilePath(const QString &serverFilePath);
    void annotate(const QString &workingDir, const QString &fileName,
                  const QString &changeList = QString(), int lineNumber = -1);
    void filelog(const QString &workingDir, const QString &fileName = QString(),
                 bool enableAnnotationContextMenu = false);
    void changelists(const QString &workingDir, const QString &fileName = QString());
    void cleanCommitMessageFile();
    bool isCommitEditorOpen() const;
    static QSharedPointer<TempFileSaver> createTemporaryArgumentFile(const QStringList &extraArgs,
                                                                            QString *errorString);

    QString pendingChangesData();

    void updateCheckout(const QString &workingDir = QString(),
                        const QStringList &dirs = QStringList());
    bool revertProject(const QString &workingDir, const QStringList &args, bool unchangedOnly);
    bool managesDirectoryFstat(const QString &directory);

    void applySettings();

    CommandLocator *m_commandLocator = nullptr;
    ParameterAction *m_editAction = nullptr;
    ParameterAction *m_addAction = nullptr;
    ParameterAction *m_deleteAction = nullptr;
    QAction *m_openedAction = nullptr;
    ParameterAction *m_revertFileAction = nullptr;
    ParameterAction *m_diffFileAction = nullptr;
    ParameterAction *m_diffProjectAction = nullptr;
    ParameterAction *m_updateProjectAction = nullptr;
    ParameterAction *m_revertProjectAction = nullptr;
    ParameterAction *m_revertUnchangedAction = nullptr;
    QAction *m_diffAllAction = nullptr;
    ParameterAction *m_submitProjectAction = nullptr;
    QAction *m_pendingAction = nullptr;
    QAction *m_describeAction = nullptr;
    ParameterAction *m_annotateCurrentAction = nullptr;
    QAction *m_annotateAction = nullptr;
    ParameterAction *m_filelogCurrentAction = nullptr;
    QAction *m_filelogAction = nullptr;
    ParameterAction *m_logProjectAction = nullptr;
    QAction *m_logRepositoryAction = nullptr;
    QAction *m_updateAllAction = nullptr;
    bool m_submitActionTriggered = false;
    QString m_commitMessageFileName;
    mutable QString m_tempFilePattern;
    QAction *m_menuAction = nullptr;

    PerforceSettings m_settings;
    SettingsPage m_settingsPage{&m_settings, [this] { applySettings(); }};

    ManagedDirectoryCache m_managedDirectoryCache;

    VcsSubmitEditorFactory submitEditorFactory {
        submitEditorParameters,
        [] { return new PerforceSubmitEditor; },
        this
    };

    VcsEditorFactory logEditorFactory {
        &logEditorParameters,
        [] { return new PerforceEditorWidget; },
        std::bind(&PerforcePluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsEditorFactory annotateEditorFactory {
        &annotateEditorParameters,
        [] { return new PerforceEditorWidget; },
        std::bind(&PerforcePluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsEditorFactory diffEditorFactory {
        &diffEditorParameters,
        [] { return new PerforceEditorWidget; },
        std::bind(&PerforcePluginPrivate::vcsDescribe, this, _1, _2)
    };
};

static PerforcePluginPrivate *dd = nullptr;

PerforcePluginPrivate::PerforcePluginPrivate()
    : VcsBasePluginPrivate(Context(PERFORCE_CONTEXT))
{
    Context context(PERFORCE_CONTEXT);

    dd = this;

    m_settings.fromSettings(ICore::settings());

    const QString prefix = QLatin1String("p4");
    m_commandLocator = new CommandLocator("Perforce", prefix, prefix, this);

    ActionContainer *mtools = ActionManager::actionContainer(Core::Constants::M_TOOLS);

    ActionContainer *perforceContainer = ActionManager::createMenu(CMD_ID_PERFORCE_MENU);
    perforceContainer->menu()->setTitle(tr("&Perforce"));
    mtools->addMenu(perforceContainer);
    m_menuAction = perforceContainer->menu()->menuAction();

    Command *command;

    m_diffFileAction = new ParameterAction(tr("Diff Current File"), tr("Diff \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_diffFileAction, CMD_ID_DIFF_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(tr("Diff Current File"));
    connect(m_diffFileAction, &QAction::triggered, this, &PerforcePluginPrivate::diffCurrentFile);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_annotateCurrentAction = new ParameterAction(tr("Annotate Current File"), tr("Annotate \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_annotateCurrentAction, CMD_ID_ANNOTATE_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(tr("Annotate Current File"));
    connect(m_annotateCurrentAction, &QAction::triggered, this, &PerforcePluginPrivate::annotateCurrentFile);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_filelogCurrentAction = new ParameterAction(tr("Filelog Current File"), tr("Filelog \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_filelogCurrentAction, CMD_ID_FILELOG_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+P,Meta+F") : tr("Alt+P,Alt+F")));
    command->setDescription(tr("Filelog Current File"));
    connect(m_filelogCurrentAction, &QAction::triggered, this, &PerforcePluginPrivate::filelogCurrentFile);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    perforceContainer->addSeparator(context);

    m_editAction = new ParameterAction(tr("Edit"), tr("Edit \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_editAction, CMD_ID_EDIT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+P,Meta+E") : tr("Alt+P,Alt+E")));
    command->setDescription(tr("Edit File"));
    connect(m_editAction, &QAction::triggered, this, &PerforcePluginPrivate::openCurrentFile);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_addAction = new ParameterAction(tr("Add"), tr("Add \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_addAction, CMD_ID_ADD, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+P,Meta+A") : tr("Alt+P,Alt+A")));
    command->setDescription(tr("Add File"));
    connect(m_addAction, &QAction::triggered, this, &PerforcePluginPrivate::addCurrentFile);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_deleteAction = new ParameterAction(tr("Delete..."), tr("Delete \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_deleteAction, CMD_ID_DELETE_FILE, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(tr("Delete File"));
    connect(m_deleteAction, &QAction::triggered, this, &PerforcePluginPrivate::promptToDeleteCurrentFile);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertFileAction = new ParameterAction(tr("Revert"), tr("Revert \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_revertFileAction, CMD_ID_REVERT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+P,Meta+R") : tr("Alt+P,Alt+R")));
    command->setDescription(tr("Revert File"));
    connect(m_revertFileAction, &QAction::triggered, this, &PerforcePluginPrivate::revertCurrentFile);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    perforceContainer->addSeparator(context);

    const QString diffProjectDefaultText = tr("Diff Current Project/Session");
    m_diffProjectAction = new ParameterAction(diffProjectDefaultText, tr("Diff Project \"%1\""), ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(m_diffProjectAction, CMD_ID_DIFF_PROJECT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+P,Meta+D") : tr("Alt+P,Alt+D")));
    command->setDescription(diffProjectDefaultText);
    connect(m_diffProjectAction, &QAction::triggered, this, &PerforcePluginPrivate::diffCurrentProject);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logProjectAction = new ParameterAction(tr("Log Project"), tr("Log Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_logProjectAction, CMD_ID_PROJECTLOG, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_logProjectAction, &QAction::triggered, this, &PerforcePluginPrivate::logProject);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_submitProjectAction = new ParameterAction(tr("Submit Project"), tr("Submit Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_submitProjectAction, CMD_ID_SUBMIT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+P,Meta+S") : tr("Alt+P,Alt+S")));
    connect(m_submitProjectAction, &QAction::triggered, this, &PerforcePluginPrivate::startSubmitProject);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    const QString updateProjectDefaultText = tr("Update Current Project");
    m_updateProjectAction = new ParameterAction(updateProjectDefaultText, tr("Update Project \"%1\""), ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(m_updateProjectAction, CMD_ID_UPDATE_PROJECT, context);
    command->setDescription(updateProjectDefaultText);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_updateProjectAction, &QAction::triggered, this, &PerforcePluginPrivate::updateCurrentProject);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertUnchangedAction = new ParameterAction(tr("Revert Unchanged"), tr("Revert Unchanged Files of Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_revertUnchangedAction, CMD_ID_REVERT_UNCHANGED_PROJECT, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_revertUnchangedAction, &QAction::triggered, this, &PerforcePluginPrivate::revertUnchangedCurrentProject);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertProjectAction = new ParameterAction(tr("Revert Project"), tr("Revert Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_revertProjectAction, CMD_ID_REVERT_PROJECT, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_revertProjectAction, &QAction::triggered, this, &PerforcePluginPrivate::revertCurrentProject);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    perforceContainer->addSeparator(context);

    m_diffAllAction = new QAction(tr("Diff Opened Files"), this);
    command = ActionManager::registerAction(m_diffAllAction, CMD_ID_DIFF_ALL, context);
    connect(m_diffAllAction, &QAction::triggered, this, &PerforcePluginPrivate::diffAllOpened);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_openedAction = new QAction(tr("Opened"), this);
    command = ActionManager::registerAction(m_openedAction, CMD_ID_OPENED, context);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+P,Meta+O") : tr("Alt+P,Alt+O")));
    connect(m_openedAction, &QAction::triggered, this, &PerforcePluginPrivate::printOpenedFileList);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logRepositoryAction = new QAction(tr("Repository Log"), this);
    command = ActionManager::registerAction(m_logRepositoryAction, CMD_ID_REPOSITORYLOG, context);
    connect(m_logRepositoryAction, &QAction::triggered, this, &PerforcePluginPrivate::logRepository);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_pendingAction = new QAction(tr("Pending Changes..."), this);
    command = ActionManager::registerAction(m_pendingAction, CMD_ID_PENDING_CHANGES, context);
    connect(m_pendingAction, &QAction::triggered, this, &PerforcePluginPrivate::printPendingChanges);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_updateAllAction = new QAction(tr("Update All"), this);
    command = ActionManager::registerAction(m_updateAllAction, CMD_ID_UPDATEALL, context);
    connect(m_updateAllAction, &QAction::triggered, this, &PerforcePluginPrivate::updateAll);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    perforceContainer->addSeparator(context);

    m_describeAction = new QAction(tr("Describe..."), this);
    command = ActionManager::registerAction(m_describeAction, CMD_ID_DESCRIBE, context);
    connect(m_describeAction, &QAction::triggered, this, &PerforcePluginPrivate::describeChange);
    perforceContainer->addAction(command);

    m_annotateAction = new QAction(tr("Annotate..."), this);
    command = ActionManager::registerAction(m_annotateAction, CMD_ID_ANNOTATE, context);
    connect(m_annotateAction, &QAction::triggered, this, &PerforcePluginPrivate::annotateFile);
    perforceContainer->addAction(command);

    m_filelogAction = new QAction(tr("Filelog..."), this);
    command = ActionManager::registerAction(m_filelogAction, CMD_ID_FILELOG, context);
    connect(m_filelogAction, &QAction::triggered, this, &PerforcePluginPrivate::filelogFile);
    perforceContainer->addAction(command);
}

void PerforcePlugin::extensionsInitialized()
{
    dd->extensionsInitialized();
    dd->getTopLevel();
}

void PerforcePluginPrivate::openCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    vcsOpen(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void PerforcePluginPrivate::addCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    vcsAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void PerforcePluginPrivate::revertCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);

    QTextCodec *codec = VcsBaseEditor::getCodec(state.currentFile());
    QStringList args;
    args << QLatin1String("diff") << QLatin1String("-sa") << state.relativeCurrentFile();
    PerforceResponse result = runP4Cmd(state.currentFileTopLevel(), args,
                                       RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow,
                                       QStringList(), QByteArray(), codec);
    if (result.error)
        return;
    // "foo.cpp - file(s) not opened on this client."
    // also revert when the output is empty: The file is unchanged but open then.
    if (result.stdOut.contains(QLatin1String(" - ")) || result.stdErr.contains(QLatin1String(" - ")))
        return;

    bool doNotRevert = false;
    if (!result.stdOut.isEmpty())
        doNotRevert = (QMessageBox::warning(ICore::dialogParent(), tr("p4 revert"),
                                            tr("The file has been changed. Do you want to revert it?"),
                                            QMessageBox::Yes, QMessageBox::No) == QMessageBox::No);
    if (doNotRevert)
        return;

    FileChangeBlocker fcb(state.currentFile());
    args.clear();
    args << QLatin1String("revert") << state.relativeCurrentFile();
    PerforceResponse result2 = runP4Cmd(state.currentFileTopLevel(), args,
                                        CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    if (!result2.error)
        emit filesChanged(QStringList(state.currentFile()));
}

void PerforcePluginPrivate::diffCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    p4Diff(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void PerforcePluginPrivate::diffCurrentProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    p4Diff(state.currentProjectTopLevel(), perforceRelativeProjectDirectory(state));
}

void PerforcePluginPrivate::diffAllOpened()
{
    p4Diff(m_settings.topLevel(), QStringList());
}

void PerforcePluginPrivate::updateCurrentProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    updateCheckout(state.currentProjectTopLevel(), perforceRelativeProjectDirectory(state));
}

void PerforcePluginPrivate::updateAll()
{
    updateCheckout(m_settings.topLevel());
}

void PerforcePluginPrivate::revertCurrentProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);

    const QString msg = tr("Do you want to revert all changes to the project \"%1\"?").arg(state.currentProjectName());
    if (QMessageBox::warning(ICore::dialogParent(), tr("p4 revert"), msg, QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
        return;
    revertProject(state.currentProjectTopLevel(), perforceRelativeProjectDirectory(state), false);
}

void PerforcePluginPrivate::revertUnchangedCurrentProject()
{
    // revert -a.
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    revertProject(state.currentProjectTopLevel(), perforceRelativeProjectDirectory(state), true);
}

bool PerforcePluginPrivate::revertProject(const QString &workingDir, const QStringList &pathArgs, bool unchangedOnly)
{
    QStringList args(QLatin1String("revert"));
    if (unchangedOnly)
        args.push_back(QLatin1String("-a"));
    args.append(pathArgs);
    const PerforceResponse resp = runP4Cmd(workingDir, args,
                                           RunFullySynchronous|CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    return !resp.error;
}

void PerforcePluginPrivate::updateCheckout(const QString &workingDir, const QStringList &dirs)
{
    QStringList args(QLatin1String("sync"));
    args.append(dirs);
    const PerforceResponse resp = runP4Cmd(workingDir, args,
                                           CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    if (dirs.empty()) {
        if (!workingDir.isEmpty())
            emit repositoryChanged(workingDir);
    } else {
        const QChar slash = QLatin1Char('/');
        foreach (const QString &dir, dirs)
            emit repositoryChanged(workingDir + slash + dir);
    }
}

void PerforcePluginPrivate::printOpenedFileList()
{
    const PerforceResponse perforceResponse
            = runP4Cmd(m_settings.topLevel(), QStringList(QLatin1String("opened")),
                       CommandToWindow|StdErrToWindow|ErrorToWindow);
    if (perforceResponse.error || perforceResponse.stdOut.isEmpty())
        return;
    // reformat "//depot/file.cpp#1 - description" into "file.cpp # - description"
    // for context menu opening to work. This produces absolute paths, then.
    QString errorMessage;
    QString mapped;
    const QChar delimiter = QLatin1Char('#');
    foreach (const QString &line, perforceResponse.stdOut.split(QLatin1Char('\n'))) {
        mapped.clear();
        const int delimiterPos = line.indexOf(delimiter);
        if (delimiterPos > 0)
            mapped = PerforcePlugin::fileNameFromPerforceName(line.left(delimiterPos), true, &errorMessage);
        if (mapped.isEmpty())
            VcsOutputWindow::appendSilently(line);
        else
            VcsOutputWindow::appendSilently(mapped + QLatin1Char(' ') + line.mid(delimiterPos));
    }
    VcsOutputWindow::instance()->popup(IOutputPane::ModeSwitch | IOutputPane::WithFocus);
}

void PerforcePluginPrivate::startSubmitProject()
{
    if (!promptBeforeCommit())
        return;

    if (raiseSubmitEditor())
        return;

    if (isCommitEditorOpen()) {
        VcsOutputWindow::appendWarning(tr("Another submit is currently executed."));
        return;
    }

    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);

    // Revert all unchanged files.
    if (!revertProject(state.currentProjectTopLevel(), perforceRelativeProjectDirectory(state), true))
        return;
    // Start a change
    QStringList args;

    args << QLatin1String("change") << QLatin1String("-o");
    PerforceResponse result = runP4Cmd(state.currentProjectTopLevel(), args,
                                       RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow);
    if (result.error) {
        cleanCommitMessageFile();
        return;
    }

    TempFileSaver saver;
    saver.setAutoRemove(false);
    saver.write(result.stdOut.toLatin1());
    if (!saver.finalize()) {
        VcsOutputWindow::appendError(saver.errorString());
        cleanCommitMessageFile();
        return;
    }
    m_commitMessageFileName = saver.fileName();

    args.clear();
    args << QLatin1String("files");
    args.append(perforceRelativeProjectDirectory(state));
    PerforceResponse filesResult = runP4Cmd(state.currentProjectTopLevel(), args,
                                            RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow);
    if (filesResult.error) {
        cleanCommitMessageFile();
        return;
    }

    QStringList filesLines = filesResult.stdOut.split(QLatin1Char('\n'));
    QStringList depotFileNames;
    foreach (const QString &line, filesLines) {
        depotFileNames.append(line.left(line.lastIndexOf(QRegularExpression("#[0-9]+\\s-\\s"))));
    }
    if (depotFileNames.isEmpty()) {
        VcsOutputWindow::appendWarning(tr("Project has no files"));
        cleanCommitMessageFile();
        return;
    }

    openPerforceSubmitEditor(m_commitMessageFileName, depotFileNames);
}

IEditor *PerforcePluginPrivate::openPerforceSubmitEditor(const QString &fileName, const QStringList &depotFileNames)
{
    IEditor *editor = EditorManager::openEditor(fileName, PERFORCE_SUBMIT_EDITOR_ID);
    auto submitEditor = static_cast<PerforceSubmitEditor*>(editor);
    setSubmitEditor(submitEditor);
    submitEditor->restrictToProjectFiles(depotFileNames);
    connect(submitEditor, &VcsBaseSubmitEditor::diffSelectedFiles,
            this, &PerforcePluginPrivate::slotSubmitDiff);
    submitEditor->setCheckScriptWorkingDirectory(m_settings.topLevel());
    return editor;
}

void PerforcePluginPrivate::printPendingChanges()
{
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    PendingChangesDialog dia(pendingChangesData(), ICore::dialogParent());
    QGuiApplication::restoreOverrideCursor();
    if (dia.exec() == QDialog::Accepted) {
        const int i = dia.changeNumber();
        QStringList args(QLatin1String("submit"));
        args << QLatin1String("-c") << QString::number(i);
        runP4Cmd(m_settings.topLevel(), args,
                 CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    }
}

void PerforcePluginPrivate::describeChange()
{
    ChangeNumberDialog dia;
    if (dia.exec() == QDialog::Accepted && dia.number() > 0)
        vcsDescribe(QString(), QString::number(dia.number()));
}

void PerforcePluginPrivate::annotateCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    annotate(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void PerforcePluginPrivate::annotateFile()
{
    const QString file = QFileDialog::getOpenFileName(ICore::dialogParent(), tr("p4 annotate"));
    if (!file.isEmpty()) {
        const QFileInfo fi(file);
        annotate(fi.absolutePath(), fi.fileName());
    }
}

void PerforcePluginPrivate::annotate(const QString &workingDir,
                              const QString &fileName,
                              const QString &changeList /* = QString() */,
                              int lineNumber /* = -1 */)
{
    const QStringList files = QStringList(fileName);
    QTextCodec *codec = VcsBaseEditor::getCodec(workingDir, files);
    const QString id = VcsBaseEditor::getTitleId(workingDir, files, changeList);
    const QString source = VcsBaseEditor::getSource(workingDir, files);
    QStringList args;
    args << QLatin1String("annotate") << QLatin1String("-cqi");
    if (changeList.isEmpty())
        args << fileName;
    else
        args << (fileName + QLatin1Char('@') + changeList);
    const PerforceResponse result = runP4Cmd(workingDir, args,
                                             CommandToWindow|StdErrToWindow|ErrorToWindow,
                                             QStringList(), QByteArray(), codec);
    if (!result.error) {
        if (lineNumber < 1)
            lineNumber = VcsBaseEditor::lineNumberOfCurrentEditor();
        IEditor *ed = showOutputInEditor(tr("p4 annotate %1").arg(id),
                                         result.stdOut, annotateEditorParameters.id,
                                         source, codec);
        VcsBaseEditor::gotoLineOfEditor(ed, lineNumber);
    }
}

void PerforcePluginPrivate::filelogCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    filelog(state.currentFileTopLevel(), state.relativeCurrentFile(), true);
}

void PerforcePluginPrivate::filelogFile()
{
    const QString file = QFileDialog::getOpenFileName(ICore::dialogParent(), tr("p4 filelog"));
    if (!file.isEmpty()) {
        const QFileInfo fi(file);
        filelog(fi.absolutePath(), fi.fileName());
    }
}

void PerforcePluginPrivate::logProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    changelists(state.currentProjectTopLevel(), perforceRelativeFileArguments(state.relativeCurrentProject()));
}

void PerforcePluginPrivate::logRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    changelists(state.topLevel(), perforceRelativeFileArguments(QString()));
}

void PerforcePluginPrivate::filelog(const QString &workingDir, const QString &fileName,
                             bool enableAnnotationContextMenu)
{
    const QString id = VcsBaseEditor::getTitleId(workingDir, QStringList(fileName));
    QTextCodec *codec = VcsBaseEditor::getCodec(workingDir, QStringList(fileName));
    QStringList args;
    args << QLatin1String("filelog") << QLatin1String("-li");
    if (m_settings.logCount() > 0)
        args << QLatin1String("-m") << QString::number(m_settings.logCount());
    if (!fileName.isEmpty())
        args.append(fileName);
    const PerforceResponse result = runP4Cmd(workingDir, args,
                                             CommandToWindow|StdErrToWindow|ErrorToWindow,
                                             QStringList(), QByteArray(), codec);
    if (!result.error) {
        const QString source = VcsBaseEditor::getSource(workingDir, fileName);
        IEditor *editor = showOutputInEditor(tr("p4 filelog %1").arg(id), result.stdOut,
                                             logEditorParameters.id, source, codec);
        if (enableAnnotationContextMenu)
            VcsBaseEditor::getVcsBaseEditor(editor)->setFileLogAnnotateEnabled(true);
    }
}

void PerforcePluginPrivate::changelists(const QString &workingDir, const QString &fileName)
{
    const QString id = VcsBaseEditor::getTitleId(workingDir, QStringList(fileName));
    QTextCodec *codec = VcsBaseEditor::getCodec(workingDir, QStringList(fileName));
    QStringList args;
    args << QLatin1String("changelists") << QLatin1String("-lit");
    if (m_settings.logCount() > 0)
        args << QLatin1String("-m") << QString::number(m_settings.logCount());
    if (!fileName.isEmpty())
        args.append(fileName);
    const PerforceResponse result = runP4Cmd(workingDir, args,
                                             CommandToWindow|StdErrToWindow|ErrorToWindow,
                                             QStringList(), QByteArray(), codec);
    if (!result.error) {
        const QString source = VcsBaseEditor::getSource(workingDir, fileName);
        IEditor *editor = showOutputInEditor(tr("p4 changelists %1").arg(id), result.stdOut,
                                             logEditorParameters.id, source, codec);
        VcsBaseEditor::gotoLineOfEditor(editor, 1);
    }
}

void PerforcePluginPrivate::updateActions(VcsBasePluginPrivate::ActionState as)
{
    const bool menuActionEnabled = enableMenuAction(as, m_menuAction);
    const bool enableActions = currentState().hasTopLevel() && menuActionEnabled;
    m_commandLocator->setEnabled(enableActions);
    if (!menuActionEnabled)
        return;

    const QString fileName = currentState().currentFileName();
    m_editAction->setParameter(fileName);
    m_addAction->setParameter(fileName);
    m_deleteAction->setParameter(fileName);
    m_revertFileAction->setParameter(fileName);
    m_diffFileAction->setParameter(fileName);
    m_annotateCurrentAction->setParameter(fileName);
    m_filelogCurrentAction->setParameter(fileName);

    const QString projectName = currentState().currentProjectName();
    m_logProjectAction->setParameter(projectName);
    m_updateProjectAction->setParameter(projectName);
    m_diffProjectAction->setParameter(projectName);
    m_submitProjectAction->setParameter(projectName);
    m_revertProjectAction->setParameter(projectName);
    m_revertUnchangedAction->setParameter(projectName);
}

bool PerforcePluginPrivate::managesDirectory(const QString &directory, QString *topLevel /* = 0 */) const
{
    const bool rc = const_cast<PerforcePluginPrivate *>(this)->managesDirectoryFstat(directory);
    if (topLevel) {
        if (rc)
            *topLevel = m_settings.topLevelSymLinkTarget();
        else
            topLevel->clear();
    }
    return rc;
}

bool PerforcePluginPrivate::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    QStringList args;
    args << QLatin1String("fstat") << QLatin1String("-m1") << fileName;
    const PerforceResponse result = runP4Cmd(workingDirectory, args, RunFullySynchronous);
    return result.stdOut.contains(QLatin1String("depotFile"));
}

bool PerforcePluginPrivate::managesDirectoryFstat(const QString &directory)
{
    // Cached?
    const ManagedDirectoryCache::const_iterator cit = m_managedDirectoryCache.constFind(directory);
    if (cit != m_managedDirectoryCache.constEnd()) {
        const DirectoryCacheEntry &entry = cit.value();
        setTopLevel(entry.m_topLevel);
        return entry.m_isManaged;
    }
    if (!m_settings.isValid()) {
        if (m_settings.topLevel().isEmpty())
            getTopLevel(directory, true);

        if (!m_settings.isValid())
            return false;
    }
    // Determine value and insert into cache
    bool managed = false;
    do {
        // Quick check: Must be at or below top level and not "../../other_path"
        const QString relativeDirArgs = m_settings.relativeToTopLevelArguments(directory);
        if (!relativeDirArgs.isEmpty() && relativeDirArgs.startsWith(QLatin1String(".."))) {
            if (!m_settings.defaultEnv())
                break;
            else
                getTopLevel(directory, true);
        }
        // Is it actually managed by perforce?
        QStringList args;
        args << QLatin1String("fstat") << QLatin1String("-m1") << perforceRelativeFileArguments(relativeDirArgs);
        const PerforceResponse result = runP4Cmd(m_settings.topLevel(), args,
                                                 RunFullySynchronous);

        managed = result.stdOut.contains(QLatin1String("depotFile"))
                  || result.stdErr.contains(QLatin1String("... - no such file(s)"));
    } while (false);

    m_managedDirectoryCache.insert(directory, DirectoryCacheEntry(managed, m_settings.topLevel()));
    return managed;
}

bool PerforcePluginPrivate::vcsOpen(const QString &workingDir, const QString &fileName, bool silently)
{
    QStringList args;
    args << QLatin1String("edit") << QDir::toNativeSeparators(fileName);

    uint flags = CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow;
    if (silently) {
        flags |= SilentStdOut;
    }
    const PerforceResponse result = runP4Cmd(workingDir, args, flags);
    return !result.error;
}

bool PerforcePluginPrivate::vcsAdd(const QString &workingDir, const QString &fileName)
{
    QStringList args;
    args << QLatin1String("add") << fileName;
    const PerforceResponse result = runP4Cmd(workingDir, args,
                                       CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    return !result.error;
}

bool PerforcePluginPrivate::vcsDelete(const QString &workingDir, const QString &fileName)
{

    QStringList args;
    args << QLatin1String("revert") << fileName;
    const PerforceResponse revertResult = runP4Cmd(workingDir, args,
                                       CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    if (revertResult.error)
        return false;
    args.clear();
    args << QLatin1String("delete") << fileName;
    const PerforceResponse deleteResult = runP4Cmd(workingDir, args,
                                             CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    // TODO need to carefully parse the actual messages from perforce
    // or do a fstat before to decide what to do

    // Different states are:
    // File is in depot and unopened => p4 delete %
    // File is in depot and opened => p4 revert %, p4 delete %
    // File is not in depot => p4 revert %
    return !deleteResult.error;
}

bool PerforcePluginPrivate::vcsMove(const QString &workingDir, const QString &from, const QString &to)
{
    // TODO verify this works
    QStringList args;
    args << QLatin1String("edit") << from;
    const PerforceResponse editResult = runP4Cmd(workingDir, args,
                                                 RunFullySynchronous|CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    if (editResult.error)
        return false;
    args.clear();
    args << QLatin1String("move") << from << to;
    const PerforceResponse moveResult = runP4Cmd(workingDir, args,
                                                 RunFullySynchronous|CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    return !moveResult.error;
}

// Write extra args to temporary file
QSharedPointer<TempFileSaver>
PerforcePluginPrivate::createTemporaryArgumentFile(const QStringList &extraArgs,
                                            QString *errorString)
{
    if (extraArgs.isEmpty())
        return QSharedPointer<TempFileSaver>();
    // create pattern
    QString pattern = dd->m_tempFilePattern;
    if (pattern.isEmpty()) {
        pattern = TemporaryDirectory::masterDirectoryPath() + "/qtc_p4_XXXXXX.args";
        dd->m_tempFilePattern = pattern;
    }
    QSharedPointer<TempFileSaver> rc(new TempFileSaver(pattern));
    rc->setAutoRemove(true);
    const int last = extraArgs.size() - 1;
    for (int i = 0; i <= last; i++) {
        rc->write(extraArgs.at(i).toLocal8Bit());
        if (i != last)
            rc->write("\n", 1);
    }
    if (!rc->finalize(errorString))
        return QSharedPointer<TempFileSaver>();
    return rc;
}

bool PerforcePluginPrivate::isVcsFileOrDirectory(const FilePath &fileName) const
{
    Q_UNUSED(fileName)
    return false; // Perforce does not seem to litter its files into the source tree.
}

bool PerforcePluginPrivate::isConfigured() const
{
    const QString binary = m_settings.p4BinaryPath();
    if (binary.isEmpty())
        return false;
    QFileInfo fi(binary);
    return fi.exists() && fi.isFile() && fi.isExecutable();
}

bool PerforcePluginPrivate::supportsOperation(Operation operation) const
{
    bool supported = isConfigured();
    switch (operation) {
    case AddOperation:
    case DeleteOperation:
    case MoveOperation:
    case AnnotateOperation:
        return supported;
    case CreateRepositoryOperation:
    case SnapshotOperations:
    case InitialCheckoutOperation:
        break;
    }
    return false;
}

IVersionControl::OpenSupportMode PerforcePluginPrivate::openSupportMode(const QString &fileName) const
{
    Q_UNUSED(fileName)
    return OpenOptional;
}

bool PerforcePluginPrivate::vcsOpen(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return vcsOpen(fi.absolutePath(), fi.fileName(), true);
}

IVersionControl::SettingsFlags PerforcePluginPrivate::settingsFlags() const
{
    SettingsFlags rc;
    if (m_settings.autoOpen())
        rc|= AutoOpen;
    return rc;
}

bool PerforcePluginPrivate::vcsAdd(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return vcsAdd(fi.absolutePath(), fi.fileName());
}

bool PerforcePluginPrivate::vcsDelete(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return vcsDelete(fi.absolutePath(), fi.fileName());
}

bool PerforcePluginPrivate::vcsMove(const QString &from, const QString &to)
{
    const QFileInfo fromInfo(from);
    const QFileInfo toInfo(to);
    return vcsMove(fromInfo.absolutePath(), fromInfo.absoluteFilePath(), toInfo.absoluteFilePath());
}

bool PerforcePluginPrivate::vcsCreateRepository(const QString &)
{
    return false;
}

void PerforcePluginPrivate::vcsAnnotate(const QString &file, int line)
{
    const QFileInfo fi(file);
    annotate(fi.absolutePath(), fi.fileName(), QString(), line);
}

QString PerforcePluginPrivate::vcsOpenText() const
{
    return tr("&Edit");
}

QString PerforcePluginPrivate::vcsMakeWritableText() const
{
    return tr("&Hijack");
}

// Run messages

static inline QString msgNotStarted(const QString &cmd)
{
    return PerforcePluginPrivate::tr("Could not start perforce \"%1\". Please check your settings in the preferences.").arg(cmd);
}

static inline QString msgTimeout(int timeOutS)
{
    return PerforcePluginPrivate::tr("Perforce did not respond within timeout limit (%1 s).").arg(timeOutS);
}

static inline QString msgCrash()
{
    return PerforcePluginPrivate::tr("The process terminated abnormally.");
}

static inline QString msgExitCode(int ex)
{
    return PerforcePluginPrivate::tr("The process terminated with exit code %1.").arg(ex);
}

// Run using a SynchronousProcess, emitting signals to the message window
PerforceResponse PerforcePluginPrivate::synchronousProcess(const QString &workingDir,
                                                           const QStringList &args,
                                                           unsigned flags,
                                                           const QByteArray &stdInput,
                                                           QTextCodec *outputCodec) const
{
    QTC_ASSERT(stdInput.isEmpty(), return PerforceResponse()); // Not supported here

    VcsOutputWindow *outputWindow = VcsOutputWindow::instance();
    // Run, connect stderr to the output window
    SynchronousProcess process;
    const int timeOutS = (flags & LongTimeOut) ? m_settings.longTimeOutS() : m_settings.timeOutS();
    process.setTimeoutS(timeOutS);
    if (outputCodec)
        process.setCodec(outputCodec);
    if (flags & OverrideDiffEnvironment)
        process.setProcessEnvironment(overrideDiffEnvironmentVariable());
    if (!workingDir.isEmpty())
        process.setWorkingDirectory(workingDir);

    // connect stderr to the output window if desired
    if (flags & StdErrToWindow) {
        process.setStdErrBufferedSignalsEnabled(true);
        connect(&process, &SynchronousProcess::stdErrBuffered,
                outputWindow, [outputWindow](const QString &lines) {
            outputWindow->append(lines);
        });
    }

    // connect stdout to the output window if desired
    if (flags & StdOutToWindow) {
        process.setStdOutBufferedSignalsEnabled(true);
        if (flags & SilentStdOut) {
            connect(&process, &SynchronousProcess::stdOutBuffered,
                    outputWindow, &VcsOutputWindow::appendSilently);
        } else {
            connect(&process, &SynchronousProcess::stdOutBuffered,
                    outputWindow, [outputWindow](const QString &lines) {
                outputWindow->append(lines);
            });
        }
    }
    process.setTimeOutMessageBoxEnabled(true);
    const SynchronousProcessResponse sp_resp = process.run({m_settings.p4BinaryPath(), args});

    PerforceResponse response;
    response.error = true;
    response.exitCode = sp_resp.exitCode;
    response.stdErr = sp_resp.stdErr();
    response.stdOut = sp_resp.stdOut();
    switch (sp_resp.result) {
    case SynchronousProcessResponse::Finished:
        response.error = false;
        break;
    case SynchronousProcessResponse::FinishedError:
        response.message = msgExitCode(sp_resp.exitCode);
        response.error = !(flags & IgnoreExitCode);
        break;
    case SynchronousProcessResponse::TerminatedAbnormally:
        response.message = msgCrash();
        break;
    case SynchronousProcessResponse::StartFailed:
        response.message = msgNotStarted(m_settings.p4BinaryPath());
        break;
    case SynchronousProcessResponse::Hang:
        response.message = msgCrash();
        break;
    }
    return response;
}

// Run using a QProcess, for short queries
PerforceResponse PerforcePluginPrivate::fullySynchronousProcess(const QString &workingDir,
                                                                const QStringList &args,
                                                                unsigned flags,
                                                                const QByteArray &stdInput,
                                                                QTextCodec *outputCodec) const
{
    QProcess process;

    if (flags & OverrideDiffEnvironment)
        process.setProcessEnvironment(overrideDiffEnvironmentVariable());
    if (!workingDir.isEmpty())
        process.setWorkingDirectory(workingDir);

    PerforceResponse response;
    process.start(m_settings.p4BinaryPath(), args);
    if (stdInput.isEmpty())
        process.closeWriteChannel();

    if (!process.waitForStarted(3000)) {
        response.error = true;
        response.message = msgNotStarted(m_settings.p4BinaryPath());
        return response;
    }
    if (!stdInput.isEmpty()) {
        if (process.write(stdInput) == -1) {
            SynchronousProcess::stopProcess(process);
            response.error = true;
            response.message = tr("Unable to write input data to process %1: %2").
                               arg(QDir::toNativeSeparators(m_settings.p4BinaryPath()),
                                   process.errorString());
            return response;
        }
        process.closeWriteChannel();
    }

    QByteArray stdOut;
    QByteArray stdErr;
    const int timeOutS = (flags & LongTimeOut) ? m_settings.longTimeOutS() : m_settings.timeOutS();
    if (!SynchronousProcess::readDataFromProcess(process, timeOutS, &stdOut, &stdErr, true)) {
        SynchronousProcess::stopProcess(process);
        response.error = true;
        response.message = msgTimeout(timeOutS);
        return response;
    }
    if (process.exitStatus() != QProcess::NormalExit) {
        response.error = true;
        response.message = msgCrash();
        return response;
    }
    response.exitCode = process.exitCode();
    response.error = response.exitCode ? !(flags & IgnoreExitCode) : false;
    response.stdErr = QString::fromLocal8Bit(stdErr);
    response.stdOut = outputCodec ? outputCodec->toUnicode(stdOut.constData(), stdOut.size()) :
                                    QString::fromLocal8Bit(stdOut);
    const QChar cr = QLatin1Char('\r');
    response.stdErr.remove(cr);
    response.stdOut.remove(cr);
    // Logging
    if ((flags & StdErrToWindow) && !response.stdErr.isEmpty())
        VcsOutputWindow::appendError(response.stdErr);
    if ((flags & StdOutToWindow) && !response.stdOut.isEmpty())
        VcsOutputWindow::append(response.stdOut, VcsOutputWindow::None, flags & SilentStdOut);
    return response;
}

PerforceResponse PerforcePluginPrivate::runP4Cmd(const QString &workingDir,
                                                 const QStringList &args,
                                                 unsigned flags,
                                                 const QStringList &extraArgs,
                                                 const QByteArray &stdInput,
                                                 QTextCodec *outputCodec) const
{
    if (!m_settings.isValid()) {
        PerforceResponse invalidConfigResponse;
        invalidConfigResponse.error = true;
        invalidConfigResponse.message = tr("Perforce is not correctly configured.");
        VcsOutputWindow::appendError(invalidConfigResponse.message);
        return invalidConfigResponse;
    }
    QStringList actualArgs = m_settings.commonP4Arguments(workingDir);
    QString errorMessage;
    QSharedPointer<TempFileSaver> tempFile = createTemporaryArgumentFile(extraArgs, &errorMessage);
    if (!tempFile.isNull()) {
        actualArgs << QLatin1String("-x") << tempFile->fileName();
    } else if (!errorMessage.isEmpty()) {
        PerforceResponse tempFailResponse;
        tempFailResponse.error = true;
        tempFailResponse.message = errorMessage;
        return tempFailResponse;
    }
    actualArgs.append(args);

    if (flags & CommandToWindow)
        VcsOutputWindow::appendCommand(workingDir, {m_settings.p4BinaryPath(), actualArgs});

    if (flags & ShowBusyCursor)
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    const PerforceResponse  response = (flags & RunFullySynchronous)  ?
        fullySynchronousProcess(workingDir, actualArgs, flags, stdInput, outputCodec) :
        synchronousProcess(workingDir, actualArgs, flags, stdInput, outputCodec);

    if (flags & ShowBusyCursor)
        QApplication::restoreOverrideCursor();

    if (response.error && (flags & ErrorToWindow))
        VcsOutputWindow::appendError(response.message);
    return response;
}

IEditor *PerforcePluginPrivate::showOutputInEditor(const QString &title,
                                                   const QString &output,
                                                   Utils::Id id,
                                                   const QString &source,
                                                   QTextCodec *codec)
{
    QString s = title;
    QString content = output;
    const int maxSize = int(EditorManager::maxTextFileSize() / 2  - 1000L); // ~25 MB, 600000 lines
    if (content.size() >= maxSize) {
        content = content.left(maxSize);
        content += QLatin1Char('\n')
                   + tr("[Only %n MB of output shown]", nullptr, maxSize / 1024 / 1024);
    }
    IEditor *editor = EditorManager::openEditorWithContents(id, &s, content.toUtf8());
    QTC_ASSERT(editor, return nullptr);
    auto e = qobject_cast<PerforceEditorWidget*>(editor->widget());
    if (!e)
        return nullptr;
    connect(e, &VcsBaseEditorWidget::annotateRevisionRequested, this, &PerforcePluginPrivate::annotate);
    e->setForceReadOnly(true);
    e->setSource(source);
    s.replace(QLatin1Char(' '), QLatin1Char('_'));
    e->textDocument()->setFallbackSaveAsFileName(s);
    if (codec)
        e->setCodec(codec);
    return editor;
}

void PerforcePluginPrivate::slotSubmitDiff(const QStringList &files)
{
    p4Diff(m_settings.topLevel(), files);
}

// Parameter widget controlling whitespace diff mode, associated with a parameter
class PerforceDiffConfig : public VcsBaseEditorConfig
{
    Q_OBJECT
public:
    explicit PerforceDiffConfig(const PerforceDiffParameters &p, QToolBar *toolBar);
    void triggerReRun();

signals:
    void reRunDiff(const Perforce::Internal::PerforceDiffParameters &);

private:
    const PerforceDiffParameters m_parameters;
};

PerforceDiffConfig::PerforceDiffConfig(const PerforceDiffParameters &p, QToolBar *toolBar) :
    VcsBaseEditorConfig(toolBar), m_parameters(p)
{
    setBaseArguments(p.diffArguments);
    addToggleButton(QLatin1String("w"), tr("Ignore Whitespace"));
    connect(this, &VcsBaseEditorConfig::argumentsChanged, this, &PerforceDiffConfig::triggerReRun);
}

void PerforceDiffConfig::triggerReRun()
{
    PerforceDiffParameters effectiveParameters = m_parameters;
    effectiveParameters.diffArguments = arguments();
    emit reRunDiff(effectiveParameters);
}

QString PerforcePluginPrivate::commitDisplayName() const
{
    return tr("Submit");
}

void PerforcePluginPrivate::p4Diff(const QString &workingDir, const QStringList &files)
{
    PerforceDiffParameters p;
    p.workingDir = workingDir;
    p.files = files;
    p.diffArguments.push_back(QString(QLatin1Char('u')));
    p4Diff(p);
}

void PerforcePluginPrivate::p4Diff(const PerforceDiffParameters &p)
{
    QTextCodec *codec = VcsBaseEditor::getCodec(p.workingDir, p.files);
    const QString id = VcsBaseEditor::getTitleId(p.workingDir, p.files);
    // Reuse existing editors for that id
    const QString tag = VcsBaseEditor::editorTag(DiffOutput, p.workingDir, p.files);
    IEditor *existingEditor = VcsBaseEditor::locateEditorByTag(tag);
    // Split arguments according to size
    QStringList args;
    args << QLatin1String("diff");
    if (!p.diffArguments.isEmpty()) // -duw..
        args << (QLatin1String("-d") + p.diffArguments.join(QString()));
    QStringList extraArgs;
    if (p.files.size() > 1)
        extraArgs = p.files;
    else
        args.append(p.files);
    const unsigned flags = CommandToWindow|StdErrToWindow|ErrorToWindow|OverrideDiffEnvironment;
    const PerforceResponse result = runP4Cmd(p.workingDir, args, flags,
                                             extraArgs, QByteArray(), codec);
    if (result.error)
        return;

    if (existingEditor) {
        existingEditor->document()->setContents(result.stdOut.toUtf8());
        EditorManager::activateEditor(existingEditor);
        return;
    }
    // Create new editor
    IEditor *editor = showOutputInEditor(tr("p4 diff %1").arg(id), result.stdOut,
                                         diffEditorParameters.id,
                                         VcsBaseEditor::getSource(p.workingDir, p.files),
                                         codec);
    VcsBaseEditor::tagEditor(editor, tag);
    auto diffEditorWidget = qobject_cast<VcsBaseEditorWidget *>(editor->widget());
    // Wire up the parameter widget to trigger a re-run on
    // parameter change and 'revert' from inside the diff editor.
    auto pw = new PerforceDiffConfig(p, diffEditorWidget->toolBar());
    connect(pw, &PerforceDiffConfig::reRunDiff,
            this, [this](const PerforceDiffParameters &p) { p4Diff(p); });
    connect(diffEditorWidget, &VcsBaseEditorWidget::diffChunkReverted,
            pw, &PerforceDiffConfig::triggerReRun);
    diffEditorWidget->setEditorConfig(pw);
}

void PerforcePluginPrivate::vcsDescribe(const QString & source, const QString &n)
{
    QTextCodec *codec = source.isEmpty() ? static_cast<QTextCodec *>(nullptr)
                                         : VcsBaseEditor::getCodec(source);
    QStringList args;
    args << QLatin1String("describe") << QLatin1String("-du") << n;
    const PerforceResponse result = runP4Cmd(m_settings.topLevel(), args, CommandToWindow|StdErrToWindow|ErrorToWindow,
                                             QStringList(), QByteArray(), codec);
    if (!result.error)
        showOutputInEditor(tr("p4 describe %1").arg(n), result.stdOut, diffEditorParameters.id, source, codec);
}

void PerforcePluginPrivate::commitFromEditor()
{
    m_submitActionTriggered = true;
    QTC_ASSERT(submitEditor(), return);
    EditorManager::closeDocument(submitEditor()->document());
}

void PerforcePluginPrivate::cleanCommitMessageFile()
{
    if (!m_commitMessageFileName.isEmpty()) {
        QFile::remove(m_commitMessageFileName);
        m_commitMessageFileName.clear();
    }
}

bool PerforcePluginPrivate::isCommitEditorOpen() const
{
    return !m_commitMessageFileName.isEmpty();
}

bool PerforcePluginPrivate::submitEditorAboutToClose()
{
    if (!isCommitEditorOpen())
        return true;
    auto perforceEditor = qobject_cast<PerforceSubmitEditor *>(submitEditor());
    QTC_ASSERT(perforceEditor, return true);
    IDocument *editorDocument = perforceEditor->document();
    QTC_ASSERT(editorDocument, return true);
    // Prompt the user. Force a prompt unless submit was actually invoked (that
    // is, the editor was closed or shutdown).
    bool wantsPrompt = m_settings.promptToSubmit();
    const VcsBaseSubmitEditor::PromptSubmitResult answer =
            perforceEditor->promptSubmit(this, &wantsPrompt, !m_submitActionTriggered);
    m_submitActionTriggered = false;

    if (answer == VcsBaseSubmitEditor::SubmitCanceled)
        return false;

    // Set without triggering the checking mechanism
    if (wantsPrompt != m_settings.promptToSubmit()) {
        m_settings.setPromptToSubmit(wantsPrompt);
        m_settings.toSettings(ICore::settings());
    }
    if (!DocumentManager::saveDocument(editorDocument))
        return false;
    if (answer == VcsBaseSubmitEditor::SubmitDiscarded) {
        cleanCommitMessageFile();
        return true;
    }
    // Pipe file into p4 submit -i
    FileReader reader;
    if (!reader.fetch(m_commitMessageFileName, QIODevice::Text)) {
        VcsOutputWindow::appendError(reader.errorString());
        return false;
    }

    QStringList submitArgs;
    submitArgs << QLatin1String("submit") << QLatin1String("-i");
    const PerforceResponse submitResponse = runP4Cmd(m_settings.topLevelSymLinkTarget(), submitArgs,
                                                     LongTimeOut|RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow|ShowBusyCursor,
                                                     QStringList(), reader.data());
    if (submitResponse.error) {
        VcsOutputWindow::appendError(tr("p4 submit failed: %1").arg(submitResponse.message));
        return false;
    }
    VcsOutputWindow::append(submitResponse.stdOut);
    if (submitResponse.stdOut.contains(QLatin1String("Out of date files must be resolved or reverted)")))
        QMessageBox::warning(perforceEditor->widget(), tr("Pending change"), tr("Could not submit the change, because your workspace was out of date. Created a pending submit instead."));

    cleanCommitMessageFile();
    return true;
}

QString PerforcePluginPrivate::clientFilePath(const QString &serverFilePath)
{
    QTC_ASSERT(m_settings.isValid(), return QString());

    QStringList args;
    args << QLatin1String("fstat") << serverFilePath;
    const PerforceResponse response = runP4Cmd(m_settings.topLevelSymLinkTarget(), args,
                                               ShowBusyCursor|RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow);
    if (response.error)
        return QString();

    const QRegularExpression r("\\.\\.\\.\\sclientFile\\s(.+?)\n");
    const QRegularExpressionMatch match = r.match(response.stdOut);
    return match.hasMatch() ? match.captured(1).trimmed() : QString();
}

QString PerforcePluginPrivate::pendingChangesData()
{
    QTC_ASSERT(m_settings.isValid(), return QString());

    QStringList args = QStringList(QLatin1String("info"));
    const PerforceResponse userResponse = runP4Cmd(m_settings.topLevelSymLinkTarget(), args,
                                               RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow);
    if (userResponse.error)
        return QString();

    const QRegularExpression r("User\\sname:\\s(\\S+?)\\s*?\n");
    QTC_ASSERT(r.isValid(), return QString());
    const QRegularExpressionMatch match = r.match(userResponse.stdOut);
    const QString user = match.hasMatch() ? match.captured(1).trimmed() : QString();
    if (user.isEmpty())
        return QString();
    args.clear();
    args << QLatin1String("changes") << QLatin1String("-s") << QLatin1String("pending") << QLatin1String("-u") << user;
    const PerforceResponse dataResponse = runP4Cmd(m_settings.topLevelSymLinkTarget(), args,
                                                   RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow);
    return dataResponse.error ? QString() : dataResponse.stdOut;
}

static inline QString msgWhereFailed(const QString & file, const QString &why)
{
    //: Failed to run p4 "where" to resolve a Perforce file name to a local
    //: file system name.
    return PerforcePluginPrivate::tr("Error running \"where\" on %1: %2").
            arg(QDir::toNativeSeparators(file), why);
}

// Map a perforce name "//xx" to its real name in the file system
QString PerforcePlugin::fileNameFromPerforceName(const QString& perforceName,
                                                 bool quiet,
                                                 QString *errorMessage)
{
    // All happy, already mapped
    if (!perforceName.startsWith(QLatin1String("//")))
        return perforceName;
    // "where" remaps the file to client file tree
    QStringList args;
    args << QLatin1String("where") << perforceName;
    unsigned flags = RunFullySynchronous;
    if (!quiet)
        flags |= CommandToWindow|StdErrToWindow|ErrorToWindow;
    const PerforceResponse response = dd->runP4Cmd(dd->m_settings.topLevelSymLinkTarget(), args, flags);
    if (response.error) {
        *errorMessage = msgWhereFailed(perforceName, response.message);
        return QString();
    }

    QString output = response.stdOut;
    if (output.endsWith(QLatin1Char('\r')))
        output.chop(1);
    if (output.endsWith(QLatin1Char('\n')))
        output.chop(1);

    if (output.isEmpty()) {
        //: File is not managed by Perforce
        *errorMessage = msgWhereFailed(perforceName, tr("The file is not mapped"));
        return QString();
    }
    const QString p4fileSpec = output.mid(output.lastIndexOf(QLatin1Char(' ')) + 1);
    return dd->m_settings.mapToFileSystem(p4fileSpec);
}

void PerforcePluginPrivate::setTopLevel(const QString &topLevel)
{
    if (m_settings.topLevel() == topLevel)
        return;

    m_settings.setTopLevel(topLevel);

    const QString msg = tr("Perforce repository: %1").arg(QDir::toNativeSeparators(topLevel));
    VcsOutputWindow::appendSilently(msg);
}

void PerforcePluginPrivate::applySettings()
{
    m_settings.toSettings(ICore::settings());
    m_managedDirectoryCache.clear();
    getTopLevel();
    emit configurationChanged();
}

void PerforcePluginPrivate::slotTopLevelFailed(const QString &errorMessage)
{
    VcsOutputWindow::appendSilently(tr("Perforce: Unable to determine the repository: %1").arg(errorMessage));
}

void PerforcePluginPrivate::getTopLevel(const QString &workingDirectory, bool isSync)
{
    // Run a new checker
    if (m_settings.p4BinaryPath().isEmpty())
        return;
    auto checker = new PerforceChecker(dd);
    connect(checker, &PerforceChecker::failed, dd, &PerforcePluginPrivate::slotTopLevelFailed);
    connect(checker, &PerforceChecker::failed, checker, &QObject::deleteLater);
    connect(checker, &PerforceChecker::succeeded, dd, &PerforcePluginPrivate::setTopLevel);
    connect(checker, &PerforceChecker::succeeded,checker, &QObject::deleteLater);

    checker->start(m_settings.p4BinaryPath(), workingDirectory,
                   m_settings.commonP4Arguments(QString()), 30000);

    if (isSync)
        checker->waitForFinished();
}

PerforcePlugin::~PerforcePlugin()
{
    delete dd;
    dd = nullptr;
}

bool PerforcePlugin::initialize(const QStringList & /* arguments */, QString *errorMessage)
{
    Q_UNUSED(errorMessage)
    dd = new PerforcePluginPrivate;
    return true;
}

#ifdef WITH_TESTS
void PerforcePlugin::testLogResolving()
{
    // Source: http://mail.opensolaris.org/pipermail/opengrok-discuss/2008-October/001668.html
    QByteArray data(
                "... #4 change 12345 edit on 2013/01/28 by User at UserWorkspaceName(text)\n"
                "\n"
                "        Comment\n"
                "... #3 change 12344 edit on 2013/01/27 by User at UserWorkspaceName(text)\n"
                "\n"
                "        Comment\n"
                );
    VcsBaseEditorWidget::testLogResolving(dd->logEditorFactory, data, "12345", "12344");
}
#endif

} // namespace Internal
} // namespace Perforce

#include "perforceplugin.moc"
