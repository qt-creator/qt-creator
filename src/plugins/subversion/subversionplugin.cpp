// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "subversionplugin.h"

#include "subversionclient.h"
#include "subversionconstants.h"
#include "subversioneditor.h"
#include "subversionsettings.h"
#include "subversionsubmiteditor.h"
#include "subversiontr.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/locator/commandlocator.h>
#include <coreplugin/vcsmanager.h>

#include <extensionsystem/iplugin.h>

#include <texteditor/textdocument.h>

#include <utils/action.h>
#include <utils/algorithm.h>
#include <utils/commandline.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <vcsbase/commonvcssettings.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsbaseplugin.h>
#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsoutputwindow.h>

#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QLoggingCategory>
#include <QMenu>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QQueue>
#include <QTimer>
#include <QUrl>
#include <QXmlStreamReader>

#include <climits>

#ifdef WITH_TESTS
#include <QTest>
#endif

using namespace Core;
using namespace QtTaskTree;
using namespace Utils;
using namespace VcsBase;

namespace Subversion::Internal {

static Q_LOGGING_CATEGORY(Log, "qtc.vcs.svn", QtWarningMsg);
static Q_LOGGING_CATEGORY(Status, "qtc.vcs.svn.status", QtWarningMsg);

const char CMD_ID_SUBVERSION_MENU[]    = "Subversion.Menu";
const char CMD_ID_ADD[]                = "Subversion.Add";
const char CMD_ID_DELETE_FILE[]        = "Subversion.Delete";
const char CMD_ID_REVERT[]             = "Subversion.Revert";
const char CMD_ID_DIFF_PROJECT[]       = "Subversion.DiffAll";
const char CMD_ID_DIFF_CURRENT[]       = "Subversion.DiffCurrent";
const char CMD_ID_COMMIT_ALL[]         = "Subversion.CommitAll";
const char CMD_ID_REVERT_ALL[]         = "Subversion.RevertAll";
const char CMD_ID_COMMIT_CURRENT[]     = "Subversion.CommitCurrent";
const char CMD_ID_FILELOG_CURRENT[]    = "Subversion.FilelogCurrent";
const char CMD_ID_ANNOTATE_CURRENT[]   = "Subversion.AnnotateCurrent";
const char CMD_ID_STATUS[]             = "Subversion.Status";
const char CMD_ID_PROJECTLOG[]         = "Subversion.ProjectLog";
const char CMD_ID_REPOSITORYLOG[]      = "Subversion.RepositoryLog";
const char CMD_ID_REPOSITORYUPDATE[]   = "Subversion.RepositoryUpdate";
const char CMD_ID_REPOSITORYDIFF[]     = "Subversion.RepositoryDiff";
const char CMD_ID_REPOSITORYSTATUS[]   = "Subversion.RepositoryStatus";
const char CMD_ID_UPDATE[]             = "Subversion.Update";
const char CMD_ID_COMMIT_PROJECT[]     = "Subversion.CommitProject";
const char CMD_ID_DESCRIBE[]           = "Subversion.Describe";

// Parse "svn status" output for added/conflicted/deleted/modified files
// "M<7blanks>file"
using StatusPair = SubversionSubmitEditor::StatusFilePair;
using StatusList = QList<StatusPair>;

static StatusList parseStatusOutput(const QString &output)
{
    StatusList changeSet;
    const QStringList list = output.split('\n', Qt::SkipEmptyParts);
    for (const QString &l : list) {
        const QString line = l.trimmed();
        if (line.size() <= 8)
            continue;

        const char state = line.at(0).toLatin1();
        if (state == FileUntrackedC || state == FileAddedC || state == FileConflictedC
            || state == FileDeletedC || state == FileModifiedC) {
            const QString fileName = line.mid(7); // Column 8 starting from svn 1.6
            changeSet.append(StatusPair(state, fileName.trimmed()));
        }
    }
    return changeSet;
}

// Return a list of names for the internal svn directories
static inline QStringList svnDirectories()
{
    QStringList rc(".svn");
    if (HostOsInfo::isWindowsHost())
        // Option on Windows systems to avoid hassle with some IDEs
        rc.push_back("_svn");
    return rc;
}

/*!
 * Describes one svn:externals subproject.
 */
struct SubversionExternal {
    QString revision;  ///< Revision, empty if none, e.g. 123
    QString url;       ///< URL to external repo, e.g. http://svn.example.com/repo
    QString directory; ///< Local repo subdirectory, e.g. externals/repo
};

using SubversionExternals = QList<SubversionExternal>;
using SubversionExternalsMap = QMap<Utils::FilePath, SubversionExternals>;

class SubversionPluginPrivate final : public VcsBase::VersionControlBase
{
public:
    VcsEditorFactory logEditorFactory{
        {LogOutput,
         Constants::SUBVERSION_LOG_EDITOR_ID,
         Tr::tr("Subversion File Log Editor"),
         Constants::SUBVERSION_LOG_MIMETYPE,
         [] { return new SubversionEditorWidget; },
         [this](const FilePath &source, const QString &changeNumber) {
             vcsDescribe(source, changeNumber);
         }}};

    VcsEditorFactory blameEditorFactory{
        {AnnotateOutput,
         Constants::SUBVERSION_BLAME_EDITOR_ID,
         Tr::tr("Subversion Annotation Editor"),
         Constants::SUBVERSION_BLAME_MIMETYPE,
         [] { return new SubversionEditorWidget; },
         [this](const FilePath &source, const QString &changeNumber) {
             vcsDescribe(source, changeNumber);
         }}};

    SubversionPluginPrivate();
    ~SubversionPluginPrivate() final;

    // IVersionControl
    QString displayName() const final { return "Subversion"; }
    Utils::Id id() const final;
    bool isVcsFileOrDirectory(const FilePath &filePath) const final;

    bool managesDirectory(const FilePath &directory, FilePath *topLevel) const final;
    bool managesFile(const FilePath &workingDirectory, const QString &fileName) const final;

    bool isConfigured() const final;
    FilePaths monitorDirectory(const FilePath &path, bool monitor) final;
    void updateModificationInfos();
    void updateNextModificationInfo();

    // Changes view
    bool supportsChangesView() const final { return true; }
    void requestRepositoryStatus(const FilePath &repository) final;
    void revertChangedFile(const FilePath &repository, const QString &relativePath) final;

    bool supportsOperation(Operation operation) const final;
    bool vcsOpen(const FilePath &filePath) final;
    bool vcsAdd(const FilePath &filePath) final;
    bool vcsDelete(const FilePath &filePath) final;
    bool vcsMove(const FilePath &from, const FilePath &to) final;
    bool vcsCreateRepository(const FilePath &directory) final;

    void vcsAnnotate(const FilePath &file, int line) final;
    void vcsLog(const Utils::FilePath &topLevel, const Utils::FilePath &relativePath) final {
        filelog(topLevel, relativePath.path());
    }
    void vcsDiff(const Utils::FilePath &topLevel, const Utils::FilePath &relativePath) final {
        subversionClient().showDiffEditor(topLevel, {relativePath.path()});
    }
    void vcsDescribe(const FilePath &source, const QString &changeNumber) final;

    ExecutableItem cloneTask(const CloneTaskData &data) const final;

    bool isVcsDirectory(const Utils::FilePath &fileName) const;

    SubversionSubmitEditor *openSubversionSubmitEditor(const QString &fileName);

    // IVersionControl
    bool vcsAdd(const FilePath &workingDir, const QString &fileName);
    bool vcsDelete(const FilePath &workingDir, const QString &fileName);
    bool vcsMove(const FilePath &workingDir, const QString &from, const QString &to);

    FilePath monitorFile(const FilePath &repository) const;
    QString synchronousTopic(const FilePath &repository) const;
    CommandResult runSvn(const FilePath &workingDir, const CommandLine &command,
                         RunFlags flags = RunFlag::None, const TextEncoding &encoding = {},
                         int timeoutMutiplier = 1) const;
    void vcsAnnotateHelper(const FilePath &workingDir, const QString &file,
                           const QString &revision = {}, int lineNumber = -1);
    SubversionExternals subversionExternals(const FilePath &directory);

protected:
    void updateActions(VcsBase::VersionControlBase::ActionState) override;
    bool activateCommit() override;
    void discardCommit() override { cleanCommitMessageFile(); }

private:
    QString synchronousProperty(const FilePath &workingDirectory,
                                const QString &property,
                                const QString &fileName = {});
    void addCurrentFile();
    void revertCurrentFile();
    void diffProjectDirectory();
    void diffCurrentFile();
    void cleanCommitMessageFile();
    void startCommitAll();
    void startCommitProjectDirectory();
    void startCommitCurrentFile();
    void revertAll();
    void filelogCurrentFile();
    void annotateCurrentFile();
    void projectDirectoryStatus();
    void slotDescribe();
    void updateProjectDirectory();
    void diffCommitFiles(const QStringList &);
    void logProjectDirectory();
    void logRepository();
    void diffRepository();
    void statusRepository();
    void updateRepository();

    inline bool isCommitEditorOpen() const;
    Core::IEditor *showOutputInEditor(const QString &title, const QString &output,
                                      Id id, const FilePath &source,
                                      const TextEncoding &encoding);

    void filelog(const FilePath &workingDir,
                 const QString &file = {},
                 bool enableAnnotationContextMenu = false);
    CommandResult runSvnStatus(const FilePath &workingDir, const QStringList &relativePaths,
                               RunFlags flags = RunFlag::None) const;
    void svnStatus(const FilePath &workingDir, const QString &relativePath = {});
    void svnUpdate(const FilePath &workingDir, const QString &relativePath = {});
    void startCommit(const FilePath &workingDir, const QStringList &files = {});

    const QStringList m_svnDirectories;

    QString m_commitMessageFileName;
    FilePath m_commitRepository;

    Core::CommandLocator *m_commandLocator = nullptr;

    enum ActionGroup { FileGroup, ProjectGroup };

    QHash<ActionGroup, QList<Action *>> m_actions;
    QList<QAction *> m_topLevelActions;

    QAction *m_menuAction = nullptr;

    QSet<FilePath> m_monitoredPaths;
    QQueue<FilePath> m_statusUpdateQueue;
    QTimer m_timer;
    SubversionExternalsMap m_externalsMap;
};


// ------------- SubversionPlugin

static SubversionPluginPrivate *dd = nullptr;

SubversionPluginPrivate::~SubversionPluginPrivate()
{
    cleanCommitMessageFile();
}

void SubversionPluginPrivate::cleanCommitMessageFile()
{
    if (!m_commitMessageFileName.isEmpty()) {
        QFile::remove(m_commitMessageFileName);
        m_commitMessageFileName.clear();
        m_commitRepository.clear();
    }
}

bool SubversionPluginPrivate::isCommitEditorOpen() const
{
    return !m_commitMessageFileName.isEmpty();
}

SubversionPluginPrivate::SubversionPluginPrivate()
    : VersionControlBase(Context(Constants::SUBVERSION_CONTEXT)),
      m_svnDirectories(svnDirectories())
{
    dd = this;

    setTopicFileTracker([this](const FilePath &repository) {
        return monitorFile(repository);
    });
    setTopicRefresher([this](const FilePath &repository) {
        return synchronousTopic(repository);
    });

    using namespace Constants;
    using namespace Core::Constants;
    const Context context(SUBVERSION_CONTEXT);

    const QString prefix = "svn";
    m_commandLocator = new CommandLocator("Subversion", prefix, prefix, this);
    m_commandLocator->setDescription(Tr::tr("Triggers a Subversion version control operation."));

    // Register actions
    ActionContainer *toolsContainer = ActionManager::actionContainer(M_TOOLS);

    ActionContainer *subversionMenu = ActionManager::createMenu(Id(CMD_ID_SUBVERSION_MENU));
    subversionMenu->menu()->setTitle(Tr::tr("&Subversion"));
    toolsContainer->addMenu(subversionMenu);
    m_menuAction = subversionMenu->menu()->menuAction();

    using Callback = void (SubversionPluginPrivate::*)();
    const auto addAction = [this, context, subversionMenu](ActionGroup actionGroup,
                                                           const QString &emptyText,
                                                           const QString &parameterText,
                                                           Id id, const Callback &callback,
                                                           const std::optional<QKeySequence> &keySequence = std::nullopt)
    {
        Action *action = new Action(emptyText, parameterText, Action::EnabledWithParameter, this);
        Command *command = ActionManager::registerAction(action, id, context);
        command->setAttribute(Command::CA_UpdateText);
        if (keySequence)
            command->setDefaultKeySequence(*keySequence);
        connect(action, &QAction::triggered, this, callback);
        subversionMenu->addAction(command);
        m_commandLocator->appendCommand(command);
        m_actions[actionGroup].append(action);
    };
    const auto addTopLevelAction = [this, context, subversionMenu](const QString &text, Id id,
                                                                   const Callback &callback)
    {
        QAction *action = new QAction(text, this);
        Command * command = ActionManager::registerAction(action, id, context);
        connect(action, &QAction::triggered, this, callback);
        subversionMenu->addAction(command);
        m_commandLocator->appendCommand(command);
        m_topLevelActions.append(action);
    };

    addAction(FileGroup, Tr::tr("Diff Current File"), Tr::tr("Diff \"%1\""),
              CMD_ID_DIFF_CURRENT, &SubversionPluginPrivate::diffCurrentFile,
              QKeySequence(useMacShortcuts ? Tr::tr("Meta+S,Meta+D") : Tr::tr("Alt+S,Alt+D")));
    addAction(FileGroup, Tr::tr("Filelog Current File"), Tr::tr("Filelog \"%1\""),
              CMD_ID_FILELOG_CURRENT, &SubversionPluginPrivate::filelogCurrentFile);
    addAction(FileGroup, Tr::tr("Annotate Current File"), Tr::tr("Annotate \"%1\""),
              CMD_ID_ANNOTATE_CURRENT, &SubversionPluginPrivate::annotateCurrentFile);
    subversionMenu->addSeparator(context);
    addAction(FileGroup, Tr::tr("Add"), Tr::tr("Add \"%1\""),
              CMD_ID_ADD, &SubversionPluginPrivate::addCurrentFile,
              QKeySequence(useMacShortcuts ? Tr::tr("Meta+S,Meta+A") : Tr::tr("Alt+S,Alt+A")));
    addAction(FileGroup, Tr::tr("Commit Current File"), Tr::tr("Commit \"%1\""),
              CMD_ID_COMMIT_CURRENT, &SubversionPluginPrivate::startCommitCurrentFile,
              QKeySequence(useMacShortcuts ? Tr::tr("Meta+S,Meta+C") : Tr::tr("Alt+S,Alt+C")));
    addAction(FileGroup, Tr::tr("Delete..."), Tr::tr("Delete \"%1\"..."),
              CMD_ID_DELETE_FILE, &SubversionPluginPrivate::promptToDeleteCurrentFile);
    addAction(FileGroup, Tr::tr("Revert..."), Tr::tr("Revert \"%1\"..."),
              CMD_ID_REVERT, &SubversionPluginPrivate::revertCurrentFile);
    subversionMenu->addSeparator(context);
    addAction(ProjectGroup, Tr::tr("Diff Project Directory"),
              Tr::tr("Diff Directory of Project \"%1\""),
              CMD_ID_DIFF_PROJECT, &SubversionPluginPrivate::diffProjectDirectory);
    addAction(ProjectGroup, Tr::tr("Project Directory Status"),
              Tr::tr("Status of Directory of Project \"%1\""),
              CMD_ID_STATUS, &SubversionPluginPrivate::projectDirectoryStatus);
    addAction(ProjectGroup, Tr::tr("Log Project Directory"),
              Tr::tr("Log Directory of Project \"%1\""),
              CMD_ID_PROJECTLOG, &SubversionPluginPrivate::logProjectDirectory);
    addAction(ProjectGroup, Tr::tr("Update Project Directory"),
              Tr::tr("Update Directory of Project \"%1\""),
              CMD_ID_UPDATE, &SubversionPluginPrivate::updateProjectDirectory);
    addAction(ProjectGroup, Tr::tr("Commit Project Directory"),
              Tr::tr("Commit Directory of Project \"%1\""),
              CMD_ID_COMMIT_PROJECT, &SubversionPluginPrivate::startCommitProjectDirectory);
    subversionMenu->addSeparator(context);
    addTopLevelAction(Tr::tr("Diff Repository"), CMD_ID_REPOSITORYDIFF,
                      &SubversionPluginPrivate::diffRepository);
    addTopLevelAction(Tr::tr("Repository Status"), CMD_ID_REPOSITORYSTATUS,
                      &SubversionPluginPrivate::statusRepository);
    addTopLevelAction(Tr::tr("Log Repository"), CMD_ID_REPOSITORYLOG,
                      &SubversionPluginPrivate::logRepository);
    addTopLevelAction(Tr::tr("Update Repository"), CMD_ID_REPOSITORYUPDATE,
                      &SubversionPluginPrivate::updateRepository);
    addTopLevelAction(Tr::tr("Commit All Files"), CMD_ID_COMMIT_ALL,
                      &SubversionPluginPrivate::startCommitAll);
    addTopLevelAction(Tr::tr("Describe..."), CMD_ID_DESCRIBE,
                      &SubversionPluginPrivate::slotDescribe);
    addTopLevelAction(Tr::tr("Revert Repository..."), CMD_ID_REVERT_ALL,
                      &SubversionPluginPrivate::revertAll);

    connect(&settings(), &AspectContainer::applied, this, &IVersionControl::configurationChanged);

    setupVcsSubmitEditor(
        this,
        {
            Constants::SUBVERSION_SUBMIT_MIMETYPE,
            Constants::SUBVERSION_COMMIT_EDITOR_ID,
            Tr::tr("Subversion Commit Editor"),
            VcsBaseSubmitEditorParameters::DiffFiles,
            [] { return new SubversionSubmitEditor; },
        });

    connect(&m_timer, &QTimer::timeout, this, &SubversionPluginPrivate::updateModificationInfos);

    auto setInterval = [this] {
        const int seconds = VcsBase::Internal::commonSettings().vcsShowStatusInterval();
        m_timer.setInterval(std::chrono::seconds(seconds));
    };

    setInterval();
    m_timer.setSingleShot(true);

    if (VcsBase::Internal::commonSettings().vcsShowStatus())
        m_timer.start();

    connect(&VcsBase::Internal::commonSettings().vcsShowStatus, &Utils::BaseAspect::changed,
            this, [this] {
        if (VcsBase::Internal::commonSettings().vcsShowStatus())
            m_timer.start();
        else
            m_timer.stop();

        for (const FilePath &path : std::as_const(m_monitoredPaths))
            VcsManager::emitClearFileState(path);
    });
    connect(&VcsBase::Internal::commonSettings().vcsShowStatusInterval, &Utils::BaseAspect::changed,
            this, setInterval);
    connect(qApp, &QApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        if (!VcsBase::Internal::commonSettings().vcsShowStatus())
            return;

        if (state == Qt::ApplicationActive)
            updateModificationInfos();
    });
}

bool SubversionPluginPrivate::isVcsDirectory(const FilePath &filePath) const
{
    if (!filePath.isDir())
        return false;

    for (const auto &svnDirName : m_svnDirectories) {
        if (filePath == filePath.withNewFileName(svnDirName))
            return true;
    }

    return false;
}

bool SubversionPluginPrivate::activateCommit()
{
    if (!isCommitEditorOpen())
        return true;

    auto editor = qobject_cast<SubversionSubmitEditor *>(submitEditor());
    QTC_ASSERT(editor, return true);
    IDocument *editorDocument = editor->document();
    QTC_ASSERT(editorDocument, return true);

    // Submit editor closing. Make it write out the commit message
    // and retrieve files
    const QFileInfo editorFile = editorDocument->filePath().toFileInfo();
    const QFileInfo changeFile(m_commitMessageFileName);
    if (editorFile.absoluteFilePath() != changeFile.absoluteFilePath())
        return true; // Oops?!

    const QStringList fileList = editor->checkedFiles();
    bool closeEditor = true;
    if (!fileList.empty()) {
        // get message & commit
        closeEditor = DocumentManager::saveDocument(editorDocument)
            && subversionClient().doCommit(m_commitRepository, fileList, m_commitMessageFileName);
        if (closeEditor)
            cleanCommitMessageFile();
    }
    return closeEditor;
}

QString SubversionPluginPrivate::synchronousProperty(const Utils::FilePath &workingDirectory,
                                                     const QString &property,
                                                     const QString &fileName)
{
    QStringList args = {"propget", property};
    if (!fileName.isEmpty())
        args.append(fileName);
    const CommandLine commandLine{settings().binaryPath(), args};
    const CommandResult result = runSvn(workingDirectory, commandLine, RunFlag::NoOutput);
    return result.cleanedStdOut();
}

void SubversionPluginPrivate::diffCommitFiles(const QStringList &files)
{
    subversionClient().showDiffEditor(m_commitRepository, files);
}

SubversionSubmitEditor *SubversionPluginPrivate::openSubversionSubmitEditor(const QString &fileName)
{
    IEditor *editor = EditorManager::openEditor(FilePath::fromString(fileName),
                                                Constants::SUBVERSION_COMMIT_EDITOR_ID);
    auto submitEditor = qobject_cast<SubversionSubmitEditor *>(editor);
    QTC_ASSERT(submitEditor, return nullptr);
    setSubmitEditor(submitEditor);
    connect(submitEditor, &VcsBaseSubmitEditor::diffSelectedFiles,
            this, &SubversionPluginPrivate::diffCommitFiles);
    submitEditor->setCheckScriptWorkingDirectory(m_commitRepository);
    return submitEditor;
}

void SubversionPluginPrivate::updateActions(VersionControlBase::ActionState as)
{
    if (!enableMenuAction(as, m_menuAction)) {
        m_commandLocator->setEnabled(false);
        return;
    }
    const bool hasTopLevel = currentState().hasTopLevel();
    m_commandLocator->setEnabled(hasTopLevel);
    for (QAction *action : std::as_const(m_topLevelActions))
        action->setEnabled(hasTopLevel);

    const QHash<ActionGroup, QString> groupLabels = {
        {FileGroup, currentState().currentFileName()},
        {ProjectGroup, currentState().currentProjectName()},
    };

    for (auto it = m_actions.cbegin(); it != m_actions.cend(); ++it) {
        const QList<Action *> &actions = it.value();
        const QString groupLabel = groupLabels.value(it.key());
        for (Action *action : actions)
            action->setParameter(groupLabel);
    };
}

void SubversionPluginPrivate::addCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    vcsAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void SubversionPluginPrivate::revertAll()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    const QString title = Tr::tr("Revert repository");
    if (QMessageBox::warning(ICore::dialogParent(), title,
                             Tr::tr("Revert all pending changes to the repository?"),
                             QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
        return;
    // NoteL: Svn "revert ." doesn not work.
    CommandLine args{settings().binaryPath(), {"revert"}};
    args << "--recursive" << state.topLevel().toUrlishString();
    const auto revertResponse = runSvn(state.topLevel(), args, RunFlag::ShowStdOut);
    if (revertResponse.result() != ProcessResult::FinishedWithSuccess) {
        QMessageBox::warning(ICore::dialogParent(), title, Tr::tr("Revert failed: %1")
                             .arg(revertResponse.exitMessage()), QMessageBox::Ok);
        return;
    }
    emit repositoryChanged(state.topLevel());
}

void SubversionPluginPrivate::revertCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);

    CommandLine diffArgs{settings().binaryPath(), {"diff"}};
    diffArgs << SubversionClient::escapeFile(state.relativeCurrentFile());

    const auto diffResponse = runSvn(state.currentFileTopLevel(), diffArgs);
    if (diffResponse.result() != ProcessResult::FinishedWithSuccess)
        return;
    if (diffResponse.cleanedStdOut().isEmpty())
        return;
    if (QMessageBox::warning(ICore::dialogParent(), Tr::tr("Revert"),
                             Tr::tr("The file has been changed. Do you want to revert it?"),
                             QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
        return;
    }

    FileChangeBlocker fcb(state.currentFile());

    // revert
    CommandLine args{settings().binaryPath(), {"revert"}};
    args << SubversionClient::escapeFile(state.relativeCurrentFile());

    const auto revertResponse = runSvn(state.currentFileTopLevel(), args, RunFlag::ShowStdOut);
    if (revertResponse.result() == ProcessResult::FinishedWithSuccess)
        emit filesChanged({state.currentFile()});
}

void SubversionPluginPrivate::revertChangedFile(const FilePath &repository,
                                                const QString &relativePath)
{
    FileChangeBlocker fcb(repository.pathAppended(relativePath));

    CommandLine args{settings().binaryPath(), {"revert"}};
    args << SubversionClient::escapeFile(relativePath);

    const auto revertResponse = runSvn(repository, args, RunFlag::ShowStdOut);
    if (revertResponse.result() == ProcessResult::FinishedWithSuccess)
        emit filesChanged({repository.pathAppended(relativePath)});
}

void SubversionPluginPrivate::diffProjectDirectory()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    const QString relativeProject = state.relativeCurrentProject();
    subversionClient().showDiffEditor(state.currentProjectTopLevel(),
                             relativeProject.isEmpty() ? QStringList()
                                                       : QStringList(relativeProject));
}

void SubversionPluginPrivate::diffCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    subversionClient().showDiffEditor(state.currentFileTopLevel(), {state.relativeCurrentFile()});
}

void SubversionPluginPrivate::startCommitCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    startCommit(state.currentFileTopLevel(), {state.relativeCurrentFile()});
}

void SubversionPluginPrivate::startCommitAll()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    startCommit(state.topLevel());
}

void SubversionPluginPrivate::startCommitProjectDirectory()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    startCommit(state.currentProjectPath());
}

/* Start commit of files of a single repository by displaying
 * template and files in a submit editor. On closing, the real
 * commit will start. */
void SubversionPluginPrivate::startCommit(const FilePath &workingDir, const QStringList &files)
{
    if (!promptBeforeCommit())
        return;

    if (raiseSubmitEditor())
        return;
    if (isCommitEditorOpen()) {
        VcsOutputWindow::appendWarning(workingDir, Tr::tr("Another commit is currently being executed."));
        return;
    }

    const auto response = runSvnStatus(workingDir, files);
    if (response.result() != ProcessResult::FinishedWithSuccess)
        return;

    // Get list of added/modified/deleted files
    const StatusList statusOutput = parseStatusOutput(response.cleanedStdOut());
    if (statusOutput.empty()) {
        VcsOutputWindow::appendWarning(workingDir, Tr::tr("There are no modified files."));
        return;
    }
    m_commitRepository = workingDir;
    // Create a new submit change file containing the submit template
    TempFileSaver saver;
    saver.setAutoRemove(false);
    // TODO: Retrieve submit template from
    const QString submitTemplate;
    // Create a submit
    saver.write(submitTemplate.toUtf8());
    if (const Result<> res = saver.finalize(); !res) {
        VcsOutputWindow::appendError(m_commitRepository, res.error());
        return;
    }
    m_commitMessageFileName = saver.filePath().toUrlishString();
    // Create a submit editor and set file list
    SubversionSubmitEditor *editor = openSubversionSubmitEditor(m_commitMessageFileName);
    QTC_ASSERT(editor, return);
    editor->setStatusList(statusOutput);
}

void SubversionPluginPrivate::filelogCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    filelog(state.currentFileTopLevel(), state.relativeCurrentFile(), true);
}

void SubversionPluginPrivate::logProjectDirectory()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    filelog(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void SubversionPluginPrivate::logRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    filelog(state.topLevel());
}

void SubversionPluginPrivate::diffRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    subversionClient().showDiffEditor(state.topLevel());
}

void SubversionPluginPrivate::statusRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    svnStatus(state.topLevel());
}

void SubversionPluginPrivate::updateRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    svnUpdate(state.topLevel());
}

CommandResult SubversionPluginPrivate::runSvnStatus(const FilePath &workingDir,
                                                    const QStringList &relativePaths,
                                                    RunFlags flags) const
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return {});
    CommandLine args{settings().binaryPath(), {"status"}};
    if (!relativePaths.isEmpty())
        args << SubversionClient::escapeFiles(relativePaths);
    return runSvn(workingDir, args, flags);
}

void SubversionPluginPrivate::svnStatus(const FilePath &workingDir, const QString &relativePath)
{
    runSvnStatus(workingDir, {relativePath}, RunFlag::ShowStdOut | RunFlag::ShowSuccessMessage);
}

void SubversionPluginPrivate::filelog(const FilePath &workingDir,
                                      const QString &file,
                                      bool enableAnnotationContextMenu)
{
    subversionClient().log(workingDir, {file}, {}, enableAnnotationContextMenu,
                  [](CommandLine &command) { command << SubversionClient::AddAuthOptions(); });
}

void SubversionPluginPrivate::updateProjectDirectory()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    svnUpdate(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void SubversionPluginPrivate::svnUpdate(const FilePath &workingDir, const QString &relativePath)
{
    CommandLine args{settings().binaryPath(), {"update"}};
    args << SubversionClient::AddAuthOptions();
    args << Constants::NON_INTERACTIVE_OPTION;
    if (!relativePath.isEmpty())
        args << relativePath;
    const auto response = runSvn(workingDir, args, RunFlag::ShowStdOut, {}, 10);
    if (response.result() == ProcessResult::FinishedWithSuccess)
        emit repositoryChanged(workingDir);
}

void SubversionPluginPrivate::annotateCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    vcsAnnotateHelper(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void SubversionPluginPrivate::vcsAnnotateHelper(const FilePath &workingDir, const QString &file,
                                                const QString &revision /* = QString() */,
                                                int lineNumber /* = -1 */)
{
    const FilePath source = VcsBaseEditor::getSource(workingDir, file);
    const TextEncoding encoding = VcsBaseEditor::getEncoding(source);

    CommandLine args{settings().binaryPath(), {"annotate"}};
    args << SubversionClient::AddAuthOptions();
    if (settings().spaceIgnorantAnnotation())
        args << "-x" << "-uw";
    if (!revision.isEmpty())
        args << "-r" << revision;
    args << "-v" << QDir::toNativeSeparators(SubversionClient::escapeFile(file));

    const auto response = runSvn(workingDir, args, RunFlag::ForceCLocale, encoding);
    if (response.result() != ProcessResult::FinishedWithSuccess)
        return;

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    if (lineNumber <= 0)
        lineNumber = VcsBaseEditor::lineNumberOfCurrentEditor(source);
    // Determine id
    const QStringList files = QStringList(file);
    const QString id = VcsBaseEditor::getTitleId(workingDir, files, revision);
    const QString tag = VcsBaseEditor::editorTag(AnnotateOutput, workingDir, files);
    if (IEditor *editor = VcsBaseEditor::locateEditorByTag(tag)) {
        editor->document()->setContents(response.cleanedStdOut().toUtf8());
        VcsBaseEditor::gotoLineOfEditor(editor, lineNumber);
        EditorManager::activateEditor(editor);
    } else {
        const QString title = QString::fromLatin1("svn annotate %1").arg(id);
        IEditor *newEditor = showOutputInEditor(title, response.cleanedStdOut(),
                                            Constants::SUBVERSION_BLAME_EDITOR_ID, source, encoding);
        VcsBaseEditor::tagEditor(newEditor, tag);
        VcsBaseEditor::gotoLineOfEditor(newEditor, lineNumber);
    }
}

void SubversionPluginPrivate::projectDirectoryStatus()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    svnStatus(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void SubversionPluginPrivate::vcsDescribe(const FilePath &source, const QString &changeNumber)
{
    // To describe a complete change, find the top level and then do
    // svn diff -r 472958:472959 <top level>
    const FilePath directory = source.isDir() ? source : source.absolutePath();
    FilePath topLevel;
    const bool manages = managesDirectory(directory, &topLevel);
    if (!manages || topLevel.isEmpty())
        return;

    qCDebug(Log) << Q_FUNC_INFO << source << topLevel << changeNumber;

    // Number must be >= 1
    bool ok = false;
    const int number = changeNumber.toInt(&ok);
    if (!ok || number < 1)
        return;

    const QString fileName = source.fileName();
    const QString title = QString::fromLatin1("svn describe %1#%2").arg(fileName, changeNumber);
    subversionClient().describe(topLevel, number, title);
}

void SubversionPluginPrivate::slotDescribe()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    QInputDialog inputDialog(ICore::dialogParent());
    inputDialog.setInputMode(QInputDialog::IntInput);
    inputDialog.setIntRange(1, INT_MAX);
    inputDialog.setWindowTitle(Tr::tr("Describe"));
    inputDialog.setLabelText(Tr::tr("Revision number:"));
    if (inputDialog.exec() != QDialog::Accepted)
        return;

    const int revision = inputDialog.intValue();
    vcsDescribe(state.topLevel(), QString::number(revision));
}

CommandResult SubversionPluginPrivate::runSvn(const FilePath &workingDir,
                                              const CommandLine &command, RunFlags flags,
                                              const TextEncoding &encoding, int timeoutMutiplier) const
{
    if (settings().binaryPath().isEmpty())
        return CommandResult(ProcessResult::StartFailed, Tr::tr("No subversion executable specified."));

    const int timeoutS = settings().timeout() * timeoutMutiplier;
    return subversionClient().vcsSynchronousExec(workingDir, command, flags, timeoutS, encoding);
}

IEditor *SubversionPluginPrivate::showOutputInEditor(const QString &title, const QString &output,
                                                     Id id, const FilePath &source,
                                                     const TextEncoding &encoding)
{
    qCDebug(Log) << "SubversionPlugin::showOutputInEditor" << title << id.toString()
                 << "Size =" << output.size() << " Type =" << id << encoding.name();
    QString s = title;
    IEditor *editor = EditorManager::openEditorWithContents(id, &s, output.toUtf8());
    auto e = qobject_cast<SubversionEditorWidget *>(editor->widget());
    if (!e)
        return nullptr;
    connect(e, &VcsBaseEditorWidget::annotateRevisionRequested,
            this, &SubversionPluginPrivate::vcsAnnotateHelper);
    e->setForceReadOnly(true);
    s.replace(' ', '_');
    e->textDocument()->setFallbackSaveAsFileName(s);
    if (!source.isEmpty())
        e->setSource(source);
    if (encoding.isValid())
        e->setEncoding(encoding);
    return editor;
}

FilePath SubversionPluginPrivate::monitorFile(const FilePath &repository) const
{
    QTC_ASSERT(!repository.isEmpty(), return {});

    for (const QString &svnDir : m_svnDirectories) {
        const FilePath dir = repository.pathAppended(svnDir);
        if (!dir.exists())
            continue;

        const FilePath file = dir.pathAppended("wc.db");
        if (file.exists() && file.isFile())
            return file.absoluteFilePath();
    }
    return {};
}

QString SubversionPluginPrivate::synchronousTopic(const FilePath &repository) const
{
    return subversionClient().synchronousTopic(repository);
}

bool SubversionPluginPrivate::vcsAdd(const FilePath &workingDir, const QString &rawFileName)
{
    const QString file = QDir::toNativeSeparators(SubversionClient::escapeFile(rawFileName));
    CommandLine args{settings().binaryPath()};
    args << "add" << "--parents" << file;
    return runSvn(workingDir, args, RunFlag::ShowStdOut).result()
            == ProcessResult::FinishedWithSuccess;
}

bool SubversionPluginPrivate::vcsDelete(const FilePath &workingDir, const QString &rawFileName)
{
    const QString file = QDir::toNativeSeparators(SubversionClient::escapeFile(rawFileName));

    CommandLine args{settings().binaryPath()};
    args << "delete" << "--force" << file;

    return runSvn(workingDir, args, RunFlag::ShowStdOut).result()
            == ProcessResult::FinishedWithSuccess;
}

bool SubversionPluginPrivate::vcsMove(const FilePath &workingDir, const QString &from, const QString &to)
{
    CommandLine args{settings().binaryPath(), {"move"}};
    args << QDir::toNativeSeparators(SubversionClient::escapeFile(from))
         << QDir::toNativeSeparators(SubversionClient::escapeFile(to));
    return runSvn(workingDir, args, RunFlag::ShowStdOut).result()
            == ProcessResult::FinishedWithSuccess;
}

bool SubversionPluginPrivate::managesDirectory(const FilePath &directory, FilePath *topLevel /* = 0 */) const
{
    const QStringList filesToCheck = transform(m_svnDirectories, [](const QString &s) {
        return QString(s + "/wc.db");
    });
    const FilePath topLevelFound = VcsManager::findRepositoryForFiles(directory, filesToCheck);
    if (topLevel)
        *topLevel = topLevelFound;
    return !topLevelFound.isEmpty();
}

bool SubversionPluginPrivate::managesFile(const FilePath &workingDirectory, const QString &fileName) const
{
    const QString output = runSvnStatus(workingDirectory, {fileName}).cleanedStdOut();
    return output.isEmpty() || output.front() != '?';
}

Utils::Id SubversionPluginPrivate::id() const
{
    return Utils::Id(VcsBase::Constants::VCS_ID_SUBVERSION);
}

bool SubversionPluginPrivate::isVcsFileOrDirectory(const FilePath &filePath) const
{
    return isVcsDirectory(filePath);
}

bool SubversionPluginPrivate::isConfigured() const
{
    const FilePath binary = settings().binaryPath.effectiveBinary();
    if (binary.isEmpty())
        return false;
    QFileInfo fi = binary.toFileInfo();
    return fi.exists() && fi.isFile() && fi.isExecutable();
}

/*!
 * Splits the externals \a line at spaces, preserving spaces within quotes.
 */
static QStringList splitExternalsLineParts(const QString &line)
{
    QStringList parts;
    QString current;
    bool inQuotes = false;

    for (QChar c : line) {
        if (c == '"' && !inQuotes) {
            inQuotes = true;
        } else if (c == '"' && inQuotes) {
            inQuotes = false;
            parts << current;
            current.clear();
        } else if (c.isSpace() && !inQuotes) {
            if (!current.isEmpty()) {
                parts << current;
                current.clear();
            }
        } else {
            current += c;
        }
    }

    if (!current.isEmpty())
        parts << current;

    return parts;
}

/*!
 * Returns a SubversionExternal from the svn:externals \a line.
 *
 * The external line has one of the following formats, while URL
 * and local directory can contain quoted spaces:
 *
 * `http://svn.example.com/repo repo`
 * `-r 123 http://svn.example.com/repo "local path"`
 */
static SubversionExternal parseExternalLine(const QString &line)
{
    const QString trimmed = line.trimmed();
    SubversionExternal result;

    if (trimmed.isEmpty() || trimmed.startsWith('#'))
        return result; // Skip comments

    const QStringList parts = splitExternalsLineParts(trimmed);

    if (parts.size() == 2) {
        result.url = parts.at(0);
        result.directory = parts.at(1);
    } else if (parts.size() == 3 && parts.at(0).startsWith("-r")) {
        result.revision = parts.at(0).mid(2); // Extract revision number from `-r123`
        result.url = parts.at(1);
        result.directory = parts.at(2);
    }
    return result;
}

/*!
 * Returns a list of all SubversionExternal subprojects for \a directory.
 */
SubversionExternals SubversionPluginPrivate::subversionExternals(const FilePath &directory)
{
    if (m_externalsMap.contains(directory))
        return m_externalsMap.value(directory);

    SubversionExternals result;
    const QStringList externals = synchronousProperty(directory, "svn:externals").split('\n');
    for (const QString &externalLine : externals) {
        const SubversionExternal external = parseExternalLine(externalLine);
        if (external.directory.isEmpty())
            continue;

        result.append(external);
    }

    m_externalsMap.insert(directory, result);
    return result;
}

FilePaths SubversionPluginPrivate::monitorDirectory(const FilePath &path, bool monitor)
{
    qCDebug(Status).nospace() << "monitorDirectory(" << path << ", " << monitor << ")";

    const QStringList filesToCheck = transform(m_svnDirectories, [](const QString &s) {
        return QString(s + "/wc.db");
    });

    const FilePath directory = VcsManager::findRepositoryForFiles(path, filesToCheck);
    if (directory.isEmpty())
        return {};

    FilePaths result;
    const bool monitored = m_monitoredPaths.contains(directory);
    if (monitor && !monitored) {
        qCDebug(Status) << "Start monitoring:" << directory;
        m_monitoredPaths.insert(directory);
        result.append(directory);
    } else if (!monitor && monitored) {
        qCDebug(Status) << "Stop monitoring:" << directory;
        m_monitoredPaths.remove(directory);
        result.append(directory);
    } else {
        return {};
    }

    // svn:externals management
    const SubversionExternals externals = subversionExternals(directory);
    for (const SubversionExternal &external : externals) {
        const Utils::FilePath externalPath = directory.pathAppended(external.directory);
        result.append(externalPath);
        if (monitor && !monitored) {
            qCDebug(Status) << "Start monitoring external:" << externalPath;
            m_monitoredPaths.insert(externalPath);
        } else {
            qCDebug(Status) << "Stop monitoring external:" << externalPath;
            m_monitoredPaths.remove(externalPath);
        }
    }

    if (m_monitoredPaths.isEmpty())
        m_timer.stop();
    else if (VcsBase::Internal::commonSettings().vcsShowStatus())
        updateModificationInfos();

    return result;
}

void SubversionPluginPrivate::updateModificationInfos()
{
    for (const FilePath &path : std::as_const(m_monitoredPaths))
        m_statusUpdateQueue.append(path);

    updateNextModificationInfo();
}

void SubversionPluginPrivate::requestRepositoryStatus(const FilePath &repository)
{
    if (repository.isEmpty())
        return;
    if (!m_statusUpdateQueue.contains(repository))
        m_statusUpdateQueue.append(repository);
    updateNextModificationInfo();
}

void SubversionPluginPrivate::updateNextModificationInfo()
{
    using FileState = Core::VcsFileState;

    if (qApp->applicationState() != Qt::ApplicationActive)
        return;

    if (m_statusUpdateQueue.isEmpty()) {
        if (VcsBase::Internal::commonSettings().vcsShowStatus())
            m_timer.start();
        return;
    }

    const FilePath path = m_statusUpdateQueue.dequeue();

    const auto command = [path, this](const CommandResult &result) {
        updateNextModificationInfo();

        const QStringList res = result.cleanedStdOut().split('\n', Qt::SkipEmptyParts);
        FileStateHash modifiedFiles;
        VcsFileStatusList fileStatus;
        for (const QString &line : res) {
            if (line.size() <= 8)
                continue;

            static const QHash<QChar, FileState> svnStates {
                {FileModifiedC,   FileState::Modified},
                {FileUntrackedC,  FileState::Untracked},
                {FileAddedC,      FileState::Added},
                {FileDeletedC,    FileState::Deleted},
                {FileConflictedC, FileState::Unmerged},
                // Renamed is Deleted+Added in Subversion
            };

            const FileState modification = svnStates.value(line.at(0), FileState::Unknown);
            if (modification != FileState::Unknown) {
                QString relativePath = line.mid(7).trimmed();
                relativePath.replace('\\', '/');
                modifiedFiles.insert(relativePath, modification);
                fileStatus.append({relativePath, modification, VcsFileStatus::Section::Changed});
            }
        }

        // Only feed the file decoration registry for monitored repositories while
        // "Show VCS file status" is enabled; on-demand requests from the Changes
        // view must not start decorating files when the setting is off.
        if (m_monitoredPaths.contains(path)
            && VcsBase::Internal::commonSettings().vcsShowStatus()) {
            VcsManager::updateModifiedFiles(path, modifiedFiles);
        }
        setRepositoryStatus(path, fileStatus);
    };
    subversionClient().enqueueCommand({path, {"status"}, RunFlag::NoOutput, {}, {}, command});
}

bool SubversionPluginPrivate::supportsOperation(Operation operation) const
{
    bool rc = isConfigured();
    switch (operation) {
    case AddOperation:
    case DeleteOperation:
    case MoveOperation:
    case AnnotateOperation:
    case InitialCheckoutOperation:
        break;
    case CreateRepositoryOperation:
    case SnapshotOperations:
        rc = false;
        break;
    }
    return rc;
}

bool SubversionPluginPrivate::vcsOpen(const FilePath & /* filePath */)
{
    // Open for edit: N/A
    return true;
}

bool SubversionPluginPrivate::vcsAdd(const FilePath &filePath)
{
    return vcsAdd(filePath.parentDir(), filePath.fileName());
}

bool SubversionPluginPrivate::vcsDelete(const FilePath &filePath)
{
    return vcsDelete(filePath.parentDir(), filePath.fileName());
}

bool SubversionPluginPrivate::vcsMove(const FilePath &from, const FilePath &to)
{
    const QFileInfo fromInfo = from.toFileInfo();
    const QFileInfo toInfo = to.toFileInfo();
    return vcsMove(from.parentDir(), fromInfo.absoluteFilePath(), toInfo.absoluteFilePath());
}

bool SubversionPluginPrivate::vcsCreateRepository(const FilePath &)
{
    return false;
}

void SubversionPluginPrivate::vcsAnnotate(const FilePath &filePath, int line)
{
    vcsAnnotateHelper(filePath.parentDir(), filePath.fileName(), {}, line);
}

ExecutableItem SubversionPluginPrivate::cloneTask(const CloneTaskData &data) const
{
    CommandLine command{settings().binaryPath()};
    command << "checkout";
    command << SubversionClient::AddAuthOptions();
    command << Subversion::Constants::NON_INTERACTIVE_OPTION << data.extraArgs << data.url << data.localName;

    return vcsProcessTask({.runData = {command, data.baseDirectory,
                                       subversionClient().processEnvironment(data.baseDirectory)},
                           .stdOutHandler = data.stdOutHandler,
                           .stdErrHandler = data.stdErrHandler});
}

#ifdef WITH_TESTS

class SubversionTest final : public QObject
{
    Q_OBJECT

private slots:
    void testLogResolving();
};

void SubversionTest::testLogResolving()
{
    QByteArray data(
                "------------------------------------------------------------------------\n"
                "r1439551 | philip | 2013-01-28 20:19:55 +0200 (Mon, 28 Jan 2013) | 4 lines\n"
                "\n"
                "* subversion/tests/cmdline/update_tests.py\n"
                "  (update_moved_dir_file_move): Resolve conflict, adjust expectations,\n"
                "   remove XFail.\n"
                "\n"
                "------------------------------------------------------------------------\n"
                "r1439540 | philip | 2013-01-28 20:06:36 +0200 (Mon, 28 Jan 2013) | 4 lines\n"
                "\n"
                "* subversion/tests/cmdline/update_tests.py\n"
                "  (update_moved_dir_edited_leaf_del): Do non-recursive resolution, adjust\n"
                "   expectations, remove XFail.\n"
                "\n"
                );
    VcsBaseEditorWidget::testLogResolving(dd->logEditorFactory, data, "r1439551", "r1439540");
}

#endif

class SubversionPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Subversion.json")

    ~SubversionPlugin() final
    {
        delete dd;
        dd = nullptr;
    }

    void initialize() final
    {
        dd = new SubversionPluginPrivate;

#ifdef WITH_TESTS
    addTest<SubversionTest>();
#endif
    }

    void extensionsInitialized() final
    {
        dd->extensionsInitialized();
    }
};

} // Subversion::Internal

#include "subversionplugin.moc"
