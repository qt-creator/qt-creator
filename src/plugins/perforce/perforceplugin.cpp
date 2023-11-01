// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perforceplugin.h"

#include "changenumberdialog.h"
#include "pendingchangesdialog.h"
#include "perforcechecker.h"
#include "perforceeditor.h"
#include "perforcesettings.h"
#include "perforcesubmiteditor.h"
#include "perforcetr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/locator/commandlocator.h>

#include <texteditor/textdocument.h>

#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/parameteraction.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
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
#include <QGuiApplication>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QRegularExpression>
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
const char PERFORCE_SUBMIT_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::VcsBase", "Perforce.SubmitEditor");

const char PERFORCE_LOG_EDITOR_ID[] = "Perforce.LogEditor";
const char PERFORCE_LOG_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::VcsBase", "Perforce Log Editor");

const char PERFORCE_DIFF_EDITOR_ID[] = "Perforce.DiffEditor";
const char PERFORCE_DIFF_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::VcsBase", "Perforce Diff Editor");

const char PERFORCE_ANNOTATION_EDITOR_ID[] = "Perforce.AnnotationEditor";
const char PERFORCE_ANNOTATION_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::VcsBase", "Perforce Annotation Editor");

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
static Environment overrideDiffEnvironmentVariable()
{
    Environment rc = Environment::systemEnvironment();
    rc.unset("P4DIFF");
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
    FilePath workingDir;
    QStringList diffArguments;
    QStringList files;
};

class PerforcePluginPrivate final : public VcsBasePluginPrivate
{
public:
    PerforcePluginPrivate();

    // IVersionControl
    QString displayName() const final { return {"perforce"}; }
    Id id() const final { return VcsBase::Constants::VCS_ID_PERFORCE; }

    bool isVcsFileOrDirectory(const FilePath &filePath) const final;
    bool managesDirectory(const Utils::FilePath &directory, Utils::FilePath *topLevel = nullptr) const final;
    bool managesFile(const Utils::FilePath &workingDirectory, const QString &fileName) const final;

    bool isConfigured() const final;
    bool supportsOperation(Operation operation) const final;
    OpenSupportMode openSupportMode(const Utils::FilePath &filePath) const final;
    bool vcsOpen(const Utils::FilePath &filePath) final;
    SettingsFlags settingsFlags() const final;
    bool vcsAdd(const Utils::FilePath &filePath) final;
    bool vcsDelete(const Utils::FilePath &filePath) final;
    bool vcsMove(const Utils::FilePath &from, const Utils::FilePath &to) final;
    bool vcsCreateRepository(const Utils::FilePath &directory) final;
    void vcsAnnotate(const Utils::FilePath &filePath, int line) final;
    void vcsDescribe(const Utils::FilePath &source, const QString &n) final;
    QString vcsOpenText() const final;
    QString vcsMakeWritableText() const final;

    ///
    bool vcsOpen(const FilePath &workingDir, const QString &fileName, bool silently = false);
    bool vcsAdd(const FilePath &workingDir, const QString &fileName);
    bool vcsDelete(const FilePath &workingDir, const QString &filename);
    bool vcsMove(const FilePath &workingDir, const QString &from, const QString &to);

    void p4Diff(const FilePath &workingDir, const QStringList &files);

    IEditor *openPerforceSubmitEditor(const QString &fileName, const QStringList &depotFileNames);

    void getTopLevel(const FilePath &workingDirectory = {}, bool isSync = false);

    void updateActions(ActionState) override;
    bool activateCommit() override;
    void discardCommit() override { cleanCommitMessageFile(); }

    QString commitDisplayName() const final;
    QString commitAbortTitle() const final;
    QString commitAbortMessage() const final;
    QString commitErrorMessage(const QString &error) const final;

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

    void printPendingChanges();
    void slotSubmitDiff(const QStringList &files);
    void setTopLevel(const Utils::FilePath &);
    void slotTopLevelFailed(const QString &);

    class DirectoryCacheEntry
    {
    public:
        DirectoryCacheEntry(bool isManaged, const FilePath &topLevel):
            m_isManaged(isManaged), m_topLevel(topLevel)
        { }

        bool m_isManaged;
        FilePath m_topLevel;
    };

    typedef QHash<FilePath, DirectoryCacheEntry> ManagedDirectoryCache;

    IEditor *showOutputInEditor(const QString &title, const QString &output,
                                Id id, const FilePath &source,
                                QTextCodec *codec = nullptr);

    // args are passed as command line arguments
    // extra args via a tempfile and the option -x "temp-filename"
    PerforceResponse runP4Cmd(const FilePath &workingDir,
                              const QStringList &args,
                              unsigned flags = CommandToWindow|StdErrToWindow|ErrorToWindow,
                              const QStringList &extraArgs = {},
                              const QByteArray &stdInput = {},
                              QTextCodec *outputCodec = nullptr) const;

    PerforceResponse synchronousProcess(const FilePath &workingDir,
                                        const QStringList &args,
                                        unsigned flags,
                                        const QByteArray &stdInput,
                                        QTextCodec *outputCodec) const;

    PerforceResponse fullySynchronousProcess(const FilePath &workingDir,
                                             const QStringList &args,
                                             unsigned flags,
                                             const QByteArray &stdInput,
                                             QTextCodec *outputCodec) const;

    QString clientFilePath(const QString &serverFilePath);
    void annotate(const FilePath &workingDir, const QString &fileName,
                  const QString &changeList = QString(), int lineNumber = -1);
    void filelog(const FilePath &workingDir, const QString &fileName = QString(),
                 bool enableAnnotationContextMenu = false);
    void changelists(const FilePath &workingDir, const QString &fileName = QString());
    void cleanCommitMessageFile();
    bool isCommitEditorOpen() const;
    static QSharedPointer<TempFileSaver> createTemporaryArgumentFile(const QStringList &extraArgs,
                                                                            QString *errorString);

    QString pendingChangesData();

    void updateCheckout(const FilePath &workingDir = {}, const QStringList &dirs = {});
    bool revertProject(const FilePath &workingDir, const QStringList &args, bool unchangedOnly);
    bool managesDirectoryFstat(const FilePath &directory);

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
    QString m_commitMessageFileName;
    mutable QString m_tempFilePattern;
    QAction *m_menuAction = nullptr;

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

    const QString prefix = QLatin1String("p4");
    m_commandLocator = new CommandLocator("Perforce", prefix, prefix, this);
    m_commandLocator->setDescription(Tr::tr("Triggers a Perforce version control operation."));

    ActionContainer *mtools = ActionManager::actionContainer(Core::Constants::M_TOOLS);

    ActionContainer *perforceContainer = ActionManager::createMenu(CMD_ID_PERFORCE_MENU);
    perforceContainer->menu()->setTitle(Tr::tr("&Perforce"));
    mtools->addMenu(perforceContainer);
    m_menuAction = perforceContainer->menu()->menuAction();

    Command *command;

    m_diffFileAction = new ParameterAction(Tr::tr("Diff Current File"), Tr::tr("Diff \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_diffFileAction, CMD_ID_DIFF_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(Tr::tr("Diff Current File"));
    connect(m_diffFileAction, &QAction::triggered, this, &PerforcePluginPrivate::diffCurrentFile);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_annotateCurrentAction = new ParameterAction(Tr::tr("Annotate Current File"), Tr::tr("Annotate \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_annotateCurrentAction, CMD_ID_ANNOTATE_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(Tr::tr("Annotate Current File"));
    connect(m_annotateCurrentAction, &QAction::triggered, this, &PerforcePluginPrivate::annotateCurrentFile);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_filelogCurrentAction = new ParameterAction(Tr::tr("Filelog Current File"), Tr::tr("Filelog \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_filelogCurrentAction, CMD_ID_FILELOG_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+P,Meta+F") : Tr::tr("Alt+P,Alt+F")));
    command->setDescription(Tr::tr("Filelog Current File"));
    connect(m_filelogCurrentAction, &QAction::triggered, this, &PerforcePluginPrivate::filelogCurrentFile);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    perforceContainer->addSeparator(context);

    m_editAction = new ParameterAction(Tr::tr("Edit"), Tr::tr("Edit \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_editAction, CMD_ID_EDIT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+P,Meta+E") : Tr::tr("Alt+P,Alt+E")));
    command->setDescription(Tr::tr("Edit File"));
    connect(m_editAction, &QAction::triggered, this, &PerforcePluginPrivate::openCurrentFile);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_addAction = new ParameterAction(Tr::tr("Add"), Tr::tr("Add \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_addAction, CMD_ID_ADD, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+P,Meta+A") : Tr::tr("Alt+P,Alt+A")));
    command->setDescription(Tr::tr("Add File"));
    connect(m_addAction, &QAction::triggered, this, &PerforcePluginPrivate::addCurrentFile);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_deleteAction = new ParameterAction(Tr::tr("Delete..."), Tr::tr("Delete \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_deleteAction, CMD_ID_DELETE_FILE, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(Tr::tr("Delete File"));
    connect(m_deleteAction, &QAction::triggered, this, &PerforcePluginPrivate::promptToDeleteCurrentFile);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertFileAction = new ParameterAction(Tr::tr("Revert"), Tr::tr("Revert \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_revertFileAction, CMD_ID_REVERT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+P,Meta+R") : Tr::tr("Alt+P,Alt+R")));
    command->setDescription(Tr::tr("Revert File"));
    connect(m_revertFileAction, &QAction::triggered, this, &PerforcePluginPrivate::revertCurrentFile);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    perforceContainer->addSeparator(context);

    const QString diffProjectDefaultText = Tr::tr("Diff Current Project/Session");
    m_diffProjectAction = new ParameterAction(diffProjectDefaultText, Tr::tr("Diff Project \"%1\""), ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(m_diffProjectAction, CMD_ID_DIFF_PROJECT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+P,Meta+D") : Tr::tr("Alt+P,Alt+D")));
    command->setDescription(diffProjectDefaultText);
    connect(m_diffProjectAction, &QAction::triggered, this, &PerforcePluginPrivate::diffCurrentProject);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logProjectAction = new ParameterAction(Tr::tr("Log Project"), Tr::tr("Log Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_logProjectAction, CMD_ID_PROJECTLOG, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_logProjectAction, &QAction::triggered, this, &PerforcePluginPrivate::logProject);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_submitProjectAction = new ParameterAction(Tr::tr("Submit Project"), Tr::tr("Submit Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_submitProjectAction, CMD_ID_SUBMIT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+P,Meta+S") : Tr::tr("Alt+P,Alt+S")));
    connect(m_submitProjectAction, &QAction::triggered, this, &PerforcePluginPrivate::startSubmitProject);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    const QString updateProjectDefaultText = Tr::tr("Update Current Project");
    m_updateProjectAction = new ParameterAction(updateProjectDefaultText, Tr::tr("Update Project \"%1\""), ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(m_updateProjectAction, CMD_ID_UPDATE_PROJECT, context);
    command->setDescription(updateProjectDefaultText);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_updateProjectAction, &QAction::triggered, this, &PerforcePluginPrivate::updateCurrentProject);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertUnchangedAction = new ParameterAction(Tr::tr("Revert Unchanged"), Tr::tr("Revert Unchanged Files of Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_revertUnchangedAction, CMD_ID_REVERT_UNCHANGED_PROJECT, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_revertUnchangedAction, &QAction::triggered, this, &PerforcePluginPrivate::revertUnchangedCurrentProject);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertProjectAction = new ParameterAction(Tr::tr("Revert Project"), Tr::tr("Revert Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_revertProjectAction, CMD_ID_REVERT_PROJECT, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_revertProjectAction, &QAction::triggered, this, &PerforcePluginPrivate::revertCurrentProject);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    perforceContainer->addSeparator(context);

    m_diffAllAction = new QAction(Tr::tr("Diff Opened Files"), this);
    command = ActionManager::registerAction(m_diffAllAction, CMD_ID_DIFF_ALL, context);
    connect(m_diffAllAction, &QAction::triggered, this, &PerforcePluginPrivate::diffAllOpened);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_openedAction = new QAction(Tr::tr("Opened"), this);
    command = ActionManager::registerAction(m_openedAction, CMD_ID_OPENED, context);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+P,Meta+O") : Tr::tr("Alt+P,Alt+O")));
    connect(m_openedAction, &QAction::triggered, this, &PerforcePluginPrivate::printOpenedFileList);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logRepositoryAction = new QAction(Tr::tr("Repository Log"), this);
    command = ActionManager::registerAction(m_logRepositoryAction, CMD_ID_REPOSITORYLOG, context);
    connect(m_logRepositoryAction, &QAction::triggered, this, &PerforcePluginPrivate::logRepository);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_pendingAction = new QAction(Tr::tr("Pending Changes..."), this);
    command = ActionManager::registerAction(m_pendingAction, CMD_ID_PENDING_CHANGES, context);
    connect(m_pendingAction, &QAction::triggered, this, &PerforcePluginPrivate::printPendingChanges);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    m_updateAllAction = new QAction(Tr::tr("Update All"), this);
    command = ActionManager::registerAction(m_updateAllAction, CMD_ID_UPDATEALL, context);
    connect(m_updateAllAction, &QAction::triggered, this, &PerforcePluginPrivate::updateAll);
    perforceContainer->addAction(command);
    m_commandLocator->appendCommand(command);

    perforceContainer->addSeparator(context);

    m_describeAction = new QAction(Tr::tr("Describe..."), this);
    command = ActionManager::registerAction(m_describeAction, CMD_ID_DESCRIBE, context);
    connect(m_describeAction, &QAction::triggered, this, &PerforcePluginPrivate::describeChange);
    perforceContainer->addAction(command);

    m_annotateAction = new QAction(Tr::tr("Annotate..."), this);
    command = ActionManager::registerAction(m_annotateAction, CMD_ID_ANNOTATE, context);
    connect(m_annotateAction, &QAction::triggered, this, &PerforcePluginPrivate::annotateFile);
    perforceContainer->addAction(command);

    m_filelogAction = new QAction(Tr::tr("Filelog..."), this);
    command = ActionManager::registerAction(m_filelogAction, CMD_ID_FILELOG, context);
    connect(m_filelogAction, &QAction::triggered, this, &PerforcePluginPrivate::filelogFile);
    perforceContainer->addAction(command);

    QObject::connect(&settings(), &AspectContainer::applied, this, [this] {
        settings().clearTopLevel();
        settings().writeSettings();
        m_managedDirectoryCache.clear();
        getTopLevel();
        emit configurationChanged();
    });
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
                                       {}, {}, codec);
    if (result.error)
        return;
    // "foo.cpp - file(s) not opened on this client."
    // also revert when the output is empty: The file is unchanged but open then.
    if (result.stdOut.contains(QLatin1String(" - ")) || result.stdErr.contains(QLatin1String(" - ")))
        return;

    bool doNotRevert = false;
    if (!result.stdOut.isEmpty())
        doNotRevert = (QMessageBox::warning(ICore::dialogParent(), Tr::tr("p4 revert"),
                                            Tr::tr("The file has been changed. Do you want to revert it?"),
                                            QMessageBox::Yes, QMessageBox::No) == QMessageBox::No);
    if (doNotRevert)
        return;

    FileChangeBlocker fcb(state.currentFile());
    args.clear();
    args << QLatin1String("revert") << state.relativeCurrentFile();
    PerforceResponse result2 = runP4Cmd(state.currentFileTopLevel(), args,
                                        CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    if (!result2.error)
        emit filesChanged(QStringList(state.currentFile().toString()));
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
    p4Diff(settings().topLevel(), QStringList());
}

void PerforcePluginPrivate::updateCurrentProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    updateCheckout(state.currentProjectTopLevel(), perforceRelativeProjectDirectory(state));
}

void PerforcePluginPrivate::updateAll()
{
    updateCheckout(settings().topLevel());
}

void PerforcePluginPrivate::revertCurrentProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);

    const QString msg = Tr::tr("Do you want to revert all changes to the project \"%1\"?").arg(state.currentProjectName());
    if (QMessageBox::warning(ICore::dialogParent(), Tr::tr("p4 revert"), msg, QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
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

bool PerforcePluginPrivate::revertProject(const FilePath &workingDir, const QStringList &pathArgs, bool unchangedOnly)
{
    QStringList args(QLatin1String("revert"));
    if (unchangedOnly)
        args.push_back(QLatin1String("-a"));
    args.append(pathArgs);
    const PerforceResponse resp = runP4Cmd(workingDir, args,
                                           RunFullySynchronous|CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    return !resp.error;
}

void PerforcePluginPrivate::updateCheckout(const FilePath &workingDir, const QStringList &dirs)
{
    QStringList args(QLatin1String("sync"));
    args.append(dirs);
    const PerforceResponse resp = runP4Cmd(workingDir, args,
                                           CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    if (dirs.empty()) {
        if (!workingDir.isEmpty())
            emit repositoryChanged(workingDir);
    } else {
        for (const QString &dir : dirs)
            emit repositoryChanged(workingDir.pathAppended(dir));
    }
}

void PerforcePluginPrivate::printOpenedFileList()
{
    const PerforceResponse perforceResponse = runP4Cmd(settings().topLevel(), {"opened"},
                                              CommandToWindow|StdErrToWindow|ErrorToWindow);
    if (perforceResponse.error || perforceResponse.stdOut.isEmpty())
        return;
    // reformat "//depot/file.cpp#1 - description" into "file.cpp # - description"
    // for context menu opening to work. This produces absolute paths, then.
    QString errorMessage;
    QString mapped;
    const QChar delimiter = QLatin1Char('#');
    const QStringList lines = perforceResponse.stdOut.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
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
        VcsOutputWindow::appendWarning(Tr::tr("Another submit is currently executed."));
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
    m_commitMessageFileName = saver.filePath().toString();

    args.clear();
    args << QLatin1String("files");
    args.append(perforceRelativeProjectDirectory(state));
    PerforceResponse filesResult = runP4Cmd(state.currentProjectTopLevel(), args,
                                            RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow);
    if (filesResult.error) {
        cleanCommitMessageFile();
        return;
    }

    const QStringList filesLines = filesResult.stdOut.split(QLatin1Char('\n'));
    QStringList depotFileNames;
    for (const QString &line : filesLines) {
        depotFileNames.append(line.left(line.lastIndexOf(QRegularExpression("#[0-9]+\\s-\\s"))));
    }
    if (depotFileNames.isEmpty()) {
        VcsOutputWindow::appendWarning(Tr::tr("Project has no files"));
        cleanCommitMessageFile();
        return;
    }

    openPerforceSubmitEditor(m_commitMessageFileName, depotFileNames);
}

IEditor *PerforcePluginPrivate::openPerforceSubmitEditor(const QString &fileName, const QStringList &depotFileNames)
{
    IEditor *editor = EditorManager::openEditor(FilePath::fromString(fileName),
                                                PERFORCE_SUBMIT_EDITOR_ID);
    auto submitEditor = static_cast<PerforceSubmitEditor*>(editor);
    setSubmitEditor(submitEditor);
    submitEditor->restrictToProjectFiles(depotFileNames);
    connect(submitEditor, &VcsBaseSubmitEditor::diffSelectedFiles,
            this, &PerforcePluginPrivate::slotSubmitDiff);
    submitEditor->setCheckScriptWorkingDirectory(settings().topLevel());
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
        runP4Cmd(settings().topLevel(), args,
                 CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    }
}

void PerforcePluginPrivate::describeChange()
{
    ChangeNumberDialog dia;
    if (dia.exec() == QDialog::Accepted && dia.number() > 0)
        vcsDescribe(FilePath(), QString::number(dia.number()));
}

void PerforcePluginPrivate::annotateCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    annotate(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void PerforcePluginPrivate::annotateFile()
{
    const FilePath filePath = FileUtils::getOpenFilePath(nullptr, Tr::tr("p4 annotate"));
    if (!filePath.isEmpty())
        annotate(filePath.parentDir(), filePath.fileName());
}

void PerforcePluginPrivate::annotate(const FilePath &workingDir,
                                     const QString &fileName,
                                     const QString &changeList /* = QString() */,
                                     int lineNumber /* = -1 */)
{
    const QStringList files = QStringList(fileName);
    QTextCodec *codec = VcsBaseEditor::getCodec(workingDir, files);
    const QString id = VcsBaseEditor::getTitleId(workingDir, files, changeList);
    const FilePath source = VcsBaseEditor::getSource(workingDir, files);
    QStringList args;
    args << QLatin1String("annotate") << QLatin1String("-cqi");
    if (changeList.isEmpty())
        args << fileName;
    else
        args << (fileName + QLatin1Char('@') + changeList);
    const PerforceResponse result = runP4Cmd(workingDir, args,
                                             CommandToWindow|StdErrToWindow|ErrorToWindow,
                                             {}, {}, codec);
    if (!result.error) {
        if (lineNumber < 1)
            lineNumber = VcsBaseEditor::lineNumberOfCurrentEditor();
        IEditor *ed = showOutputInEditor(Tr::tr("p4 annotate %1").arg(id),
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
    const FilePath file = FileUtils::getOpenFilePath(nullptr, Tr::tr("p4 filelog"));
    if (!file.isEmpty())
        filelog(file.parentDir(), file.fileName());
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

void PerforcePluginPrivate::filelog(const FilePath &workingDir, const QString &fileName,
                                    bool enableAnnotationContextMenu)
{
    const QString id = VcsBaseEditor::getTitleId(workingDir, QStringList(fileName));
    QTextCodec *codec = VcsBaseEditor::getCodec(workingDir, QStringList(fileName));
    QStringList args;
    args << QLatin1String("filelog") << QLatin1String("-li");
    if (settings().logCount() > 0)
        args << "-m" << QString::number(settings().logCount());
    if (!fileName.isEmpty())
        args.append(fileName);
    const PerforceResponse result = runP4Cmd(workingDir, args,
                                             CommandToWindow|StdErrToWindow|ErrorToWindow,
                                             {}, {}, codec);
    if (!result.error) {
        const FilePath source = VcsBaseEditor::getSource(workingDir, fileName);
        IEditor *editor = showOutputInEditor(Tr::tr("p4 filelog %1").arg(id), result.stdOut,
                                             logEditorParameters.id, source, codec);
        if (enableAnnotationContextMenu)
            VcsBaseEditor::getVcsBaseEditor(editor)->setFileLogAnnotateEnabled(true);
    }
}

void PerforcePluginPrivate::changelists(const FilePath &workingDir, const QString &fileName)
{
    const QString id = VcsBaseEditor::getTitleId(workingDir, QStringList(fileName));
    QTextCodec *codec = VcsBaseEditor::getCodec(workingDir, QStringList(fileName));
    QStringList args;
    args << QLatin1String("changelists") << QLatin1String("-lit");
    if (settings().logCount() > 0)
        args << "-m" << QString::number(settings().logCount());
    if (!fileName.isEmpty())
        args.append(fileName);
    const PerforceResponse result = runP4Cmd(workingDir, args,
                                             CommandToWindow|StdErrToWindow|ErrorToWindow,
                                             {}, {}, codec);
    if (!result.error) {
        const FilePath source = VcsBaseEditor::getSource(workingDir, fileName);
        IEditor *editor = showOutputInEditor(Tr::tr("p4 changelists %1").arg(id), result.stdOut,
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

bool PerforcePluginPrivate::managesDirectory(const FilePath &directory, FilePath *topLevel /* = 0 */) const
{
    const bool rc = const_cast<PerforcePluginPrivate *>(this)->managesDirectoryFstat(directory);
    if (topLevel) {
        if (rc)
            *topLevel = settings().topLevelSymLinkTarget();
        else
            topLevel->clear();
    }
    return rc;
}

bool PerforcePluginPrivate::managesFile(const FilePath &workingDirectory, const QString &fileName) const
{
    QStringList args;
    args << QLatin1String("fstat") << QLatin1String("-m1") << fileName;
    const PerforceResponse result = runP4Cmd(workingDirectory, args, RunFullySynchronous);
    return result.stdOut.contains(QLatin1String("depotFile"));
}

bool PerforcePluginPrivate::managesDirectoryFstat(const FilePath &directory)
{
    // Cached?
    const ManagedDirectoryCache::const_iterator cit = m_managedDirectoryCache.constFind(directory);
    if (cit != m_managedDirectoryCache.constEnd()) {
        const DirectoryCacheEntry &entry = cit.value();
        setTopLevel(entry.m_topLevel);
        return entry.m_isManaged;
    }
    if (!settings().isValid()) {
        if (settings().topLevel().isEmpty())
            getTopLevel(directory, true);

        if (!settings().isValid())
            return false;
    }
    // Determine value and insert into cache
    bool managed = false;
    do {
        // Quick check: Must be at or below top level and not "../../other_path"
        const QString relativeDirArgs = settings().relativeToTopLevelArguments(directory.toString());
        if (!relativeDirArgs.isEmpty() && relativeDirArgs.startsWith(QLatin1String(".."))) {
            if (!settings().defaultEnv())
                break;
            else
                getTopLevel(directory, true);
        }
        // Is it actually managed by perforce?
        QStringList args;
        args << QLatin1String("fstat") << QLatin1String("-m1") << perforceRelativeFileArguments(relativeDirArgs);
        const PerforceResponse result = runP4Cmd(settings().topLevel(), args,
                                                 RunFullySynchronous);

        managed = result.stdOut.contains(QLatin1String("depotFile"))
                  || result.stdErr.contains(QLatin1String("... - no such file(s)"));
    } while (false);

    m_managedDirectoryCache.insert(directory, DirectoryCacheEntry(managed, settings().topLevel()));
    return managed;
}

bool PerforcePluginPrivate::vcsOpen(const FilePath &workingDir, const QString &fileName, bool silently)
{
    QStringList args;
    args << QLatin1String("edit") << QDir::toNativeSeparators(fileName);

    uint flags = CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow;
    if (silently) {
        flags |= SilentStdOut;
    }
    const PerforceResponse result = runP4Cmd(workingDir, args, flags);
    if (result.error)
        return false;
    const FilePath absPath = workingDir.resolvePath(fileName);
    if (DocumentModel::Entry *e = DocumentModel::entryForFilePath(absPath))
        e->document->checkPermissions();
    return true;
}

bool PerforcePluginPrivate::vcsAdd(const FilePath &workingDir, const QString &fileName)
{
    QStringList args;
    args << QLatin1String("add") << fileName;
    const PerforceResponse result = runP4Cmd(workingDir, args,
                                       CommandToWindow|StdOutToWindow|StdErrToWindow|ErrorToWindow);
    return !result.error;
}

bool PerforcePluginPrivate::vcsDelete(const FilePath &workingDir, const QString &fileName)
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

bool PerforcePluginPrivate::vcsMove(const FilePath &workingDir, const QString &from, const QString &to)
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

bool PerforcePluginPrivate::isVcsFileOrDirectory(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return false; // Perforce does not seem to litter its files into the source tree.
}

bool PerforcePluginPrivate::isConfigured() const
{
    const FilePath binary = settings().p4BinaryPath();
    return !binary.isEmpty() && binary.isExecutableFile();
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

IVersionControl::OpenSupportMode PerforcePluginPrivate::openSupportMode(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return OpenOptional;
}

bool PerforcePluginPrivate::vcsOpen(const FilePath &filePath)
{
    return vcsOpen(filePath.parentDir(), filePath.fileName(), true);
}

IVersionControl::SettingsFlags PerforcePluginPrivate::settingsFlags() const
{
    SettingsFlags rc;
    if (settings().autoOpen())
        rc |= AutoOpen;
    return rc;
}

bool PerforcePluginPrivate::vcsAdd(const FilePath &filePath)
{
    return vcsAdd(filePath.parentDir(), filePath.fileName());
}

bool PerforcePluginPrivate::vcsDelete(const FilePath &filePath)
{
    return vcsDelete(filePath.parentDir(), filePath.fileName());
}

bool PerforcePluginPrivate::vcsMove(const FilePath &from, const FilePath &to)
{
    const QFileInfo fromInfo = from.toFileInfo();
    const QFileInfo toInfo = to.toFileInfo();
    return vcsMove(from.absolutePath(), fromInfo.absoluteFilePath(), toInfo.absoluteFilePath());
}

bool PerforcePluginPrivate::vcsCreateRepository(const FilePath &)
{
    return false;
}

void PerforcePluginPrivate::vcsAnnotate(const FilePath &filePath, int line)
{
    annotate(filePath.parentDir(), filePath.fileName(), QString(), line);
}

QString PerforcePluginPrivate::vcsOpenText() const
{
    return Tr::tr("&Edit");
}

QString PerforcePluginPrivate::vcsMakeWritableText() const
{
    return Tr::tr("&Hijack");
}

// Run messages

static QString msgNotStarted(const FilePath &cmd)
{
    return Tr::tr("Could not start perforce \"%1\". Please check your settings in the preferences.")
        .arg(cmd.toUserOutput());
}

static QString msgTimeout(int timeOutS)
{
    return Tr::tr("Perforce did not respond within timeout limit (%1 s).").arg(timeOutS);
}

static QString msgCrash()
{
    return Tr::tr("The process terminated abnormally.");
}

static QString msgExitCode(int ex)
{
    return Tr::tr("The process terminated with exit code %1.").arg(ex);
}

// Run using a SynchronousProcess, emitting signals to the message window
PerforceResponse PerforcePluginPrivate::synchronousProcess(const FilePath &workingDir,
                                                           const QStringList &args,
                                                           unsigned flags,
                                                           const QByteArray &stdInput,
                                                           QTextCodec *outputCodec) const
{
    QTC_ASSERT(stdInput.isEmpty(), return PerforceResponse()); // Not supported here

    // Run, connect stderr to the output window
    Process process;
    const int timeOutS = (flags & LongTimeOut) ? settings().longTimeOutS() : settings().timeOutS();
    process.setTimeoutS(timeOutS);
    if (outputCodec)
        process.setCodec(outputCodec);
    if (flags & OverrideDiffEnvironment)
        process.setEnvironment(overrideDiffEnvironmentVariable());
    if (!workingDir.isEmpty())
        process.setWorkingDirectory(workingDir);

    // connect stderr to the output window if desired
    if (flags & StdErrToWindow)
        process.setStdErrCallback([](const QString &lines) { VcsOutputWindow::append(lines); });

    // connect stdout to the output window if desired
    if (flags & StdOutToWindow) {
        if (flags & SilentStdOut)
            process.setStdOutCallback(&VcsOutputWindow::appendSilently);
        else
            process.setStdOutCallback([](const QString &lines) { VcsOutputWindow::append(lines); });
    }
    process.setTimeOutMessageBoxEnabled(true);
    process.setCommand({settings().p4BinaryPath(), args});
    process.runBlocking(EventLoopMode::On);

    PerforceResponse response;
    response.error = true;
    response.exitCode = process.exitCode();
    response.stdErr = process.cleanedStdErr();
    response.stdOut = process.cleanedStdOut();
    switch (process.result()) {
    case ProcessResult::FinishedWithSuccess:
        response.error = false;
        break;
    case ProcessResult::FinishedWithError:
        response.message = msgExitCode(process.exitCode());
        response.error = !(flags & IgnoreExitCode);
        break;
    case ProcessResult::TerminatedAbnormally:
        response.message = msgCrash();
        break;
    case ProcessResult::StartFailed:
        response.message = msgNotStarted(settings().p4BinaryPath());
        break;
    case ProcessResult::Hang:
        response.message = msgCrash();
        break;
    }
    return response;
}

// Run using a QProcess, for short queries
PerforceResponse PerforcePluginPrivate::fullySynchronousProcess(const FilePath &workingDir,
                                                                const QStringList &args,
                                                                unsigned flags,
                                                                const QByteArray &stdInput,
                                                                QTextCodec *outputCodec) const
{
    Process process;

    if (flags & OverrideDiffEnvironment)
        process.setEnvironment(overrideDiffEnvironmentVariable());
    if (!workingDir.isEmpty())
        process.setWorkingDirectory(workingDir);

    PerforceResponse response;
    process.setCommand({settings().p4BinaryPath(), args});
    process.setWriteData(stdInput);
    process.start();

    if (!process.waitForStarted(3000)) {
        response.error = true;
        response.message = msgNotStarted(settings().p4BinaryPath());
        return response;
    }

    QByteArray stdOut;
    QByteArray stdErr;
    const int timeOutS = (flags & LongTimeOut) ? settings().longTimeOutS() : settings().timeOutS();
    if (!process.readDataFromProcess(&stdOut, &stdErr, timeOutS)) {
        process.stop();
        process.waitForFinished();
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

PerforceResponse PerforcePluginPrivate::runP4Cmd(const FilePath &workingDir,
                                                 const QStringList &args,
                                                 unsigned flags,
                                                 const QStringList &extraArgs,
                                                 const QByteArray &stdInput,
                                                 QTextCodec *outputCodec) const
{
    if (!settings().isValid()) {
        PerforceResponse invalidConfigResponse;
        invalidConfigResponse.error = true;
        invalidConfigResponse.message = Tr::tr("Perforce is not correctly configured.");
        VcsOutputWindow::appendError(invalidConfigResponse.message);
        return invalidConfigResponse;
    }
    QStringList actualArgs = settings().commonP4Arguments(workingDir.toString());
    QString errorMessage;
    QSharedPointer<TempFileSaver> tempFile = createTemporaryArgumentFile(extraArgs, &errorMessage);
    if (!tempFile.isNull()) {
        actualArgs << QLatin1String("-x") << tempFile->filePath().toString();
    } else if (!errorMessage.isEmpty()) {
        PerforceResponse tempFailResponse;
        tempFailResponse.error = true;
        tempFailResponse.message = errorMessage;
        return tempFailResponse;
    }
    actualArgs.append(args);

    if (flags & CommandToWindow)
        VcsOutputWindow::appendCommand(workingDir, {settings().p4BinaryPath(), actualArgs});

    if (flags & ShowBusyCursor)
        QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    const PerforceResponse  response = (flags & RunFullySynchronous)  ?
        fullySynchronousProcess(workingDir, actualArgs, flags, stdInput, outputCodec) :
        synchronousProcess(workingDir, actualArgs, flags, stdInput, outputCodec);

    if (flags & ShowBusyCursor)
        QGuiApplication::restoreOverrideCursor();

    if (response.error && (flags & ErrorToWindow))
        VcsOutputWindow::appendError(response.message);
    return response;
}

IEditor *PerforcePluginPrivate::showOutputInEditor(const QString &title,
                                                   const QString &output,
                                                   Utils::Id id,
                                                   const FilePath &source,
                                                   QTextCodec *codec)
{
    QString s = title;
    QString content = output;
    const int maxSize = int(EditorManager::maxTextFileSize() / 2  - 1000L); // ~25 MB, 600000 lines
    if (content.size() >= maxSize) {
        content = content.left(maxSize);
        content += QLatin1Char('\n')
                   + Tr::tr("[Only %n MB of output shown]", nullptr, maxSize / 1024 / 1024);
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
    p4Diff(settings().topLevel(), files);
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
    addToggleButton(QLatin1String("w"), Tr::tr("Ignore Whitespace"));
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
    //: Name of the "commit" action of the VCS
    return Tr::tr("Submit");
}

QString PerforcePluginPrivate::commitAbortTitle() const
{
    return Tr::tr("Close Submit Editor");
}

QString PerforcePluginPrivate::commitAbortMessage() const
{
    return Tr::tr("Closing this editor will abort the submit.");
}

QString PerforcePluginPrivate::commitErrorMessage(const QString &error) const
{
    if (error.isEmpty())
        return Tr::tr("Cannot submit.");
    return Tr::tr("Cannot submit: %1.").arg(error);
}

void PerforcePluginPrivate::p4Diff(const FilePath &workingDir, const QStringList &files)
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
    const PerforceResponse result = runP4Cmd(p.workingDir, args, flags, extraArgs, {}, codec);
    if (result.error)
        return;

    if (existingEditor) {
        existingEditor->document()->setContents(result.stdOut.toUtf8());
        EditorManager::activateEditor(existingEditor);
        return;
    }
    // Create new editor
    IEditor *editor = showOutputInEditor(Tr::tr("p4 diff %1").arg(id), result.stdOut,
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

void PerforcePluginPrivate::vcsDescribe(const FilePath &source, const QString &n)
{
    QTextCodec *codec = source.isEmpty() ? static_cast<QTextCodec *>(nullptr)
                                         : VcsBaseEditor::getCodec(source);
    QStringList args;
    args << QLatin1String("describe") << QLatin1String("-du") << n;
    const PerforceResponse result = runP4Cmd(settings().topLevel(), args, CommandToWindow|StdErrToWindow|ErrorToWindow,
                                             {}, {}, codec);
    if (!result.error)
        showOutputInEditor(Tr::tr("p4 describe %1").arg(n), result.stdOut, diffEditorParameters.id, source, codec);
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

bool PerforcePluginPrivate::activateCommit()
{
    if (!isCommitEditorOpen())
        return true;
    auto perforceEditor = qobject_cast<PerforceSubmitEditor *>(submitEditor());
    QTC_ASSERT(perforceEditor, return true);
    IDocument *editorDocument = perforceEditor->document();
    QTC_ASSERT(editorDocument, return true);

    if (!DocumentManager::saveDocument(editorDocument))
        return false;

    // Pipe file into p4 submit -i
    FileReader reader;
    if (!reader.fetch(Utils::FilePath::fromString(m_commitMessageFileName), QIODevice::Text)) {
        VcsOutputWindow::appendError(reader.errorString());
        return false;
    }

    QStringList submitArgs;
    submitArgs << QLatin1String("submit") << QLatin1String("-i");
    const PerforceResponse submitResponse = runP4Cmd(settings().topLevelSymLinkTarget(), submitArgs,
                                                     LongTimeOut|RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow|ShowBusyCursor,
                                                     {}, reader.data());
    if (submitResponse.error) {
        VcsOutputWindow::appendError(Tr::tr("p4 submit failed: %1").arg(submitResponse.message));
        return false;
    }
    VcsOutputWindow::append(submitResponse.stdOut);
    if (submitResponse.stdOut.contains(QLatin1String("Out of date files must be resolved or reverted)")))
        QMessageBox::warning(perforceEditor->widget(), Tr::tr("Pending change"), Tr::tr("Could not submit the change, because your workspace was out of date. Created a pending submit instead."));

    cleanCommitMessageFile();
    return true;
}

QString PerforcePluginPrivate::clientFilePath(const QString &serverFilePath)
{
    QTC_ASSERT(settings().isValid(), return QString());

    QStringList args;
    args << QLatin1String("fstat") << serverFilePath;
    const PerforceResponse response = runP4Cmd(settings().topLevelSymLinkTarget(), args,
                                               ShowBusyCursor|RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow);
    if (response.error)
        return {};

    const QRegularExpression r("\\.\\.\\.\\sclientFile\\s(.+?)\n");
    const QRegularExpressionMatch match = r.match(response.stdOut);
    return match.hasMatch() ? match.captured(1).trimmed() : QString();
}

QString PerforcePluginPrivate::pendingChangesData()
{
    QTC_ASSERT(settings().isValid(), return QString());

    QStringList args = QStringList(QLatin1String("info"));
    const PerforceResponse userResponse = runP4Cmd(settings().topLevelSymLinkTarget(), args,
                                               RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow);
    if (userResponse.error)
        return {};

    const QRegularExpression r("User\\sname:\\s(\\S+?)\\s*?\n");
    QTC_ASSERT(r.isValid(), return QString());
    const QRegularExpressionMatch match = r.match(userResponse.stdOut);
    const QString user = match.hasMatch() ? match.captured(1).trimmed() : QString();
    if (user.isEmpty())
        return {};
    args.clear();
    args << QLatin1String("changes") << QLatin1String("-s") << QLatin1String("pending") << QLatin1String("-u") << user;
    const PerforceResponse dataResponse = runP4Cmd(settings().topLevelSymLinkTarget(), args,
                                                   RunFullySynchronous|CommandToWindow|StdErrToWindow|ErrorToWindow);
    return dataResponse.error ? QString() : dataResponse.stdOut;
}

static QString msgWhereFailed(const QString & file, const QString &why)
{
    //: Failed to run p4 "where" to resolve a Perforce file name to a local
    //: file system name.
    return Tr::tr("Error running \"where\" on %1: %2").
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
    const PerforceResponse response = dd->runP4Cmd(settings().topLevelSymLinkTarget(), args, flags);
    if (response.error) {
        *errorMessage = msgWhereFailed(perforceName, response.message);
        return {};
    }

    QString output = response.stdOut;
    if (output.endsWith(QLatin1Char('\r')))
        output.chop(1);
    if (output.endsWith(QLatin1Char('\n')))
        output.chop(1);

    if (output.isEmpty()) {
        //: File is not managed by Perforce
        *errorMessage = msgWhereFailed(perforceName, Tr::tr("The file is not mapped"));
        return {};
    }
    const QString p4fileSpec = output.mid(output.lastIndexOf(QLatin1Char(' ')) + 1);
    return settings().mapToFileSystem(p4fileSpec);
}

void PerforcePluginPrivate::setTopLevel(const FilePath &topLevel)
{
    if (settings().topLevel() == topLevel)
        return;

    settings().setTopLevel(topLevel.toString());

    const QString msg = Tr::tr("Perforce repository: %1").arg(topLevel.toUserOutput());
    VcsOutputWindow::appendSilently(msg);
}

void PerforcePluginPrivate::slotTopLevelFailed(const QString &errorMessage)
{
    VcsOutputWindow::appendSilently(Tr::tr("Perforce: Unable to determine the repository: %1").arg(errorMessage));
}

void PerforcePluginPrivate::getTopLevel(const FilePath &workingDirectory, bool isSync)
{
    // Run a new checker
    if (settings().p4BinaryPath().isEmpty())
        return;
    auto checker = new PerforceChecker(dd);
    connect(checker, &PerforceChecker::failed, dd, &PerforcePluginPrivate::slotTopLevelFailed);
    connect(checker, &PerforceChecker::failed, checker, &QObject::deleteLater);
    connect(checker, &PerforceChecker::succeeded, dd, &PerforcePluginPrivate::setTopLevel);
    connect(checker, &PerforceChecker::succeeded,checker, &QObject::deleteLater);

    checker->start(settings().p4BinaryPath(), workingDirectory,
                   settings().commonP4Arguments(QString()), 30000);

    if (isSync)
        checker->waitForFinished();
}

PerforcePlugin::~PerforcePlugin()
{
    delete dd;
    dd = nullptr;
}

void PerforcePlugin::initialize()
{
    dd = new PerforcePluginPrivate;
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
