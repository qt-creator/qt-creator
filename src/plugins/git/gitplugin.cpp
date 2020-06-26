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

#include "gitplugin.h"

#include "branchview.h"
#include "changeselectiondialog.h"
#include "commitdata.h"
#include "gitclient.h"
#include "gitconstants.h"
#include "giteditor.h"
#include "gitsubmiteditor.h"
#include "remotedialog.h"
#include "stashdialog.h"
#include "settingspage.h"
#include "logchangedialog.h"
#include "mergetool.h"
#include "gitutils.h"
#include "gitgrep.h"

#include "gerrit/gerritplugin.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/locator/commandlocator.h>
#include <coreplugin/messagebox.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/vcsmanager.h>

#include <aggregation/aggregate.h>

#include <texteditor/texteditor.h>
#include <utils/infobar.h>
#include <utils/parameteraction.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>

#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/submitfilemodel.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcscommand.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsoutputwindow.h>
#include <vcsbase/cleandialog.h>
#include <vcsbase/vcsbaseplugin.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QMenu>
#include <QVBoxLayout>

#ifdef WITH_TESTS
#include <QTest>
#endif

Q_DECLARE_METATYPE(Git::Internal::FileStates)

using namespace Core;
using namespace TextEditor;
using namespace Utils;
using namespace VcsBase;
using namespace std::placeholders;

namespace Git {
namespace Internal {

using GitClientMemberFunc = void (GitClient::*)(const QString &) const;

class GitTopicCache : public Core::IVersionControl::TopicCache
{
public:
    GitTopicCache(GitClient *client) :
        m_client(client)
    { }

protected:
    QString trackFile(const QString &repository) override
    {
        const QString gitDir = m_client->findGitDirForRepository(repository);
        return gitDir.isEmpty() ? QString() : (gitDir + "/HEAD");
    }

    QString refreshTopic(const QString &repository) override
    {
        return m_client->synchronousTopic(repository);
    }

private:
    GitClient *m_client;
};

class GitReflogEditorWidget : public GitEditorWidget
{
public:
    GitReflogEditorWidget()
    {
        setLogEntryPattern("^([0-9a-f]{8,}) [^}]*\\}: .*$");
    }

    QString revisionSubject(const QTextBlock &inBlock) const override
    {
        const QString text = inBlock.text();
        return text.mid(text.indexOf(' ') + 1);
    }
};

class GitLogEditorWidget : public QWidget
{
public:
    GitLogEditorWidget(GitEditorWidget *gitEditor)
    {
        auto vlayout = new QVBoxLayout;
        vlayout->setSpacing(0);
        vlayout->setContentsMargins(0, 0, 0, 0);
        vlayout->addWidget(gitEditor->addFilterWidget());
        vlayout->addWidget(gitEditor);
        setLayout(vlayout);

        auto textAgg = Aggregation::Aggregate::parentAggregate(gitEditor);
        auto agg = textAgg ? textAgg : new Aggregation::Aggregate;
        agg->add(this);
        agg->add(gitEditor);
        setFocusProxy(gitEditor);
    }
};

template<class Editor>
class GitLogEditorWidgetT : public GitLogEditorWidget
{
public:
    GitLogEditorWidgetT() : GitLogEditorWidget(new Editor) {}
};

const unsigned minimumRequiredVersion = 0x010900;

const VcsBaseSubmitEditorParameters submitParameters {
    Git::Constants::SUBMIT_MIMETYPE,
    Git::Constants::GITSUBMITEDITOR_ID,
    Git::Constants::GITSUBMITEDITOR_DISPLAY_NAME,
    VcsBaseSubmitEditorParameters::DiffRows
};

const VcsBaseEditorParameters svnLogEditorParameters {
    OtherContent,
    Git::Constants::GIT_SVN_LOG_EDITOR_ID,
    Git::Constants::GIT_SVN_LOG_EDITOR_DISPLAY_NAME,
    "text/vnd.qtcreator.git.svnlog"
};

const VcsBaseEditorParameters logEditorParameters {
    LogOutput,
    Git::Constants::GIT_LOG_EDITOR_ID,
    Git::Constants::GIT_LOG_EDITOR_DISPLAY_NAME,
    "text/vnd.qtcreator.git.log"
};

const VcsBaseEditorParameters reflogEditorParameters {
    LogOutput,
    Git::Constants::GIT_REFLOG_EDITOR_ID,
    Git::Constants::GIT_REFLOG_EDITOR_DISPLAY_NAME,
    "text/vnd.qtcreator.git.reflog"
};

const VcsBaseEditorParameters blameEditorParameters {
    AnnotateOutput,
    Git::Constants::GIT_BLAME_EDITOR_ID,
    Git::Constants::GIT_BLAME_EDITOR_DISPLAY_NAME,
    "text/vnd.qtcreator.git.annotation"
};

const VcsBaseEditorParameters commitTextEditorParameters {
    OtherContent,
    Git::Constants::GIT_COMMIT_TEXT_EDITOR_ID,
    Git::Constants::GIT_COMMIT_TEXT_EDITOR_DISPLAY_NAME,
    "text/vnd.qtcreator.git.commit"
};

const VcsBaseEditorParameters rebaseEditorParameters {
    OtherContent,
    Git::Constants::GIT_REBASE_EDITOR_ID,
    Git::Constants::GIT_REBASE_EDITOR_DISPLAY_NAME,
    "text/vnd.qtcreator.git.rebase"
};

// GitPlugin

class GitPluginPrivate final : public VcsBase::VcsBasePluginPrivate
{
    Q_OBJECT

public:
    GitPluginPrivate();
    ~GitPluginPrivate() final;

    // IVersionControl
    QString displayName() const final;
    Utils::Id id() const final;

    bool isVcsFileOrDirectory(const Utils::FilePath &fileName) const final;

    bool managesDirectory(const QString &directory, QString *topLevel) const final;
    bool managesFile(const QString &workingDirectory, const QString &fileName) const final;
    QStringList unmanagedFiles(const QString &workingDir, const QStringList &filePaths) const final;

    bool isConfigured() const final;
    bool supportsOperation(Operation operation) const final;
    bool vcsOpen(const QString &fileName) final;
    bool vcsAdd(const QString &fileName) final;
    bool vcsDelete(const QString &filename) final;
    bool vcsMove(const QString &from, const QString &to) final;
    bool vcsCreateRepository(const QString &directory) final;

    void vcsAnnotate(const QString &file, int line) final;
    void vcsDescribe(const QString &source, const QString &id) final { m_gitClient.show(source, id); };
    QString vcsTopic(const QString &directory) final;

    Core::ShellCommand *createInitialCheckoutCommand(const QString &url,
                                                     const Utils::FilePath &baseDirectory,
                                                     const QString &localName,
                                                     const QStringList &extraArgs) final;

    void fillLinkContextMenu(QMenu *menu,
                             const QString &workingDirectory,
                             const QString &reference) final
    {
        menu->addAction(tr("&Copy \"%1\"").arg(reference),
                        [reference] { QApplication::clipboard()->setText(reference); });
        QAction *action = menu->addAction(tr("&Describe Change %1").arg(reference),
                                          [=] { vcsDescribe(workingDirectory, reference); });
        menu->setDefaultAction(action);
        GitClient::addChangeActions(menu, workingDirectory, reference);
    }

    bool handleLink(const QString &workingDirectory, const QString &reference) final
    {
        if (reference.contains(".."))
            GitClient::instance()->log(workingDirectory, {}, false, {reference});
        else
            GitClient::instance()->show(workingDirectory, reference);
        return true;
    }

    RepoUrl getRepoUrl(const QString &location) const override;

    QStringList additionalToolsPath() const final;

    bool isCommitEditorOpen() const;
    void startCommit(CommitType commitType = SimpleCommit);
    void updateBranches(const QString &repository);
    void updateCurrentBranch();

    void manageRemotes();
    void initRepository();
    void startRebaseFromCommit(const QString &workingDirectory, QString commit);

    void updateActions(VcsBase::VcsBasePluginPrivate::ActionState) override;
    bool submitEditorAboutToClose() override;

    void diffCurrentFile();
    void diffCurrentProject();
    void commitFromEditor() override;
    void logFile();
    void blameFile();
    void logProject();
    void logRepository();
    void reflogRepository();
    void undoFileChanges(bool revertStaging);
    void resetRepository();
    void recoverDeletedFiles();
    void startRebase();
    void startChangeRelatedAction(const Utils::Id &id);
    void stageFile();
    void unstageFile();
    void gitkForCurrentFile();
    void gitkForCurrentFolder();
    void gitGui();
    void gitBash();
    void cleanProject();
    void cleanRepository();
    void updateSubmodules();
    void applyCurrentFilePatch();
    void promptApplyPatch();

    void stash(bool unstagedOnly = false);
    void stashUnstaged();
    void stashSnapshot();
    void stashPop();
    void branchList();
    void stashList();
    void fetch();
    void pull();
    void push();
    void startMergeTool();
    void continueOrAbortCommand();
    void updateContinueAndAbortCommands();
    void delayedPushToGerrit();

    Core::Command *createCommand(QAction *action, Core::ActionContainer *ac, Utils::Id id,
                                 const Core::Context &context, bool addToLocator,
                                 const std::function<void()> &callback, const QKeySequence &keys);

    Utils::ParameterAction *createParameterAction(Core::ActionContainer *ac,
                                                  const QString &defaultText, const QString &parameterText,
                                                  Utils::Id id, const Core::Context &context, bool addToLocator,
                                                  const std::function<void()> &callback,
                                                  const QKeySequence &keys = QKeySequence());

    QAction *createFileAction(Core::ActionContainer *ac,
                              const QString &defaultText, const QString &parameterText,
                              Utils::Id id, const Core::Context &context, bool addToLocator,
                              const std::function<void()> &callback,
                              const QKeySequence &keys = QKeySequence());

    QAction *createProjectAction(Core::ActionContainer *ac,
                                 const QString &defaultText, const QString &parameterText,
                                 Utils::Id id, const Core::Context &context, bool addToLocator,
                                 void (GitPluginPrivate::*func)(),
                                 const QKeySequence &keys = QKeySequence());

    QAction *createRepositoryAction(Core::ActionContainer *ac, const QString &text, Utils::Id id,
                                    const Core::Context &context, bool addToLocator,
                                    const std::function<void()> &callback,
                                    const QKeySequence &keys = QKeySequence());
    QAction *createRepositoryAction(Core::ActionContainer *ac, const QString &text, Utils::Id id,
                                    const Core::Context &context, bool addToLocator,
                                    GitClientMemberFunc, const QKeySequence &keys = QKeySequence());

    QAction *createChangeRelatedRepositoryAction(const QString &text, Utils::Id id,
                                                 const Core::Context &context);

    void updateRepositoryBrowserAction();
    Core::IEditor *openSubmitEditor(const QString &fileName, const CommitData &cd);
    void cleanCommitMessageFile();
    void cleanRepository(const QString &directory);
    void applyPatch(const QString &workingDirectory, QString file = QString());
    void updateVersionWarning();


    void onApplySettings();;

    Core::CommandLocator *m_commandLocator = nullptr;

    QAction *m_menuAction = nullptr;
    QAction *m_repositoryBrowserAction = nullptr;
    QAction *m_mergeToolAction = nullptr;
    QAction *m_submoduleUpdateAction = nullptr;
    QAction *m_abortMergeAction = nullptr;
    QAction *m_abortRebaseAction = nullptr;
    QAction *m_abortCherryPickAction = nullptr;
    QAction *m_abortRevertAction = nullptr;
    QAction *m_skipRebaseAction = nullptr;
    QAction *m_continueRebaseAction = nullptr;
    QAction *m_continueCherryPickAction = nullptr;
    QAction *m_continueRevertAction = nullptr;
    QAction *m_fixupCommitAction = nullptr;
    QAction *m_interactiveRebaseAction = nullptr;

    QVector<Utils::ParameterAction *> m_fileActions;
    QVector<Utils::ParameterAction *> m_projectActions;
    QVector<QAction *> m_repositoryActions;
    Utils::ParameterAction *m_applyCurrentFilePatchAction = nullptr;
    Gerrit::Internal::GerritPlugin *m_gerritPlugin = nullptr;

    GitSettings m_settings;
    GitClient m_gitClient{&m_settings};
    QPointer<StashDialog> m_stashDialog;
    BranchViewFactory m_branchViewFactory;
    QPointer<RemoteDialog> m_remoteDialog;
    QString m_submitRepository;
    QString m_commitMessageFileName;
    bool m_submitActionTriggered = false;

    GitSettingsPage settingPage{&m_settings, std::bind(&GitPluginPrivate::onApplySettings, this)};

    GitGrep gitGrep{&m_gitClient};

    VcsEditorFactory svnLogEditorFactory {
        &svnLogEditorParameters,
        [] { return new GitEditorWidget; },
        std::bind(&GitPluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsEditorFactory logEditorFactory {
        &logEditorParameters,
        [] { return new GitLogEditorWidgetT<GitEditorWidget>; },
        std::bind(&GitPluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsEditorFactory reflogEditorFactory {
        &reflogEditorParameters,
                [] { return new GitLogEditorWidgetT<GitReflogEditorWidget>; },
        std::bind(&GitPluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsEditorFactory blameEditorFactory {
        &blameEditorParameters,
        [] { return new GitEditorWidget; },
        std::bind(&GitPluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsEditorFactory commitTextEditorFactory {
        &commitTextEditorParameters,
        [] { return new GitEditorWidget; },
        std::bind(&GitPluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsEditorFactory rebaseEditorFactory {
        &rebaseEditorParameters,
        [] { return new GitEditorWidget; },
        std::bind(&GitPluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsSubmitEditorFactory submitEditorFactory {
        submitParameters,
        [] { return new GitSubmitEditor; },
        this
    };
};

static GitPluginPrivate *dd = nullptr;

GitPluginPrivate::~GitPluginPrivate()
{
    cleanCommitMessageFile();
}

GitPlugin::~GitPlugin()
{
    delete dd;
    dd = nullptr;
}

void GitPluginPrivate::onApplySettings()
{
    configurationChanged();
    updateRepositoryBrowserAction();
    bool gitFoundOk;
    QString errorMessage;
    m_settings.gitExecutable(&gitFoundOk, &errorMessage);
    if (!gitFoundOk)
        Core::AsynchronousMessageBox::warning(tr("Git Settings"), errorMessage);
}

void GitPluginPrivate::cleanCommitMessageFile()
{
    if (!m_commitMessageFileName.isEmpty()) {
        QFile::remove(m_commitMessageFileName);
        m_commitMessageFileName.clear();
    }
}

bool GitPluginPrivate::isCommitEditorOpen() const
{
    return !m_commitMessageFileName.isEmpty();
}

GitClient *GitPlugin::client()
{
    return &dd->m_gitClient;
}

IVersionControl *GitPlugin::versionControl()
{
    return dd;
}

const GitSettings &GitPlugin::settings()
{
    return dd->m_settings;
}

const VcsBasePluginState &GitPlugin::currentState()
{
    return dd->currentState();
}

QString GitPlugin::msgRepositoryLabel(const QString &repository)
{
    return repository.isEmpty() ?
            tr("<No repository>")  :
            tr("Repository: %1").arg(QDir::toNativeSeparators(repository));
}

// Returns a regular expression pattern with characters not allowed
// in branch and remote names.
QString GitPlugin::invalidBranchAndRemoteNamePattern()
{
    return QLatin1String(
        "\\s"     // no whitespace
        "|~"      // no "~"
        "|\\^"    // no "^"
        "|\\["    // no "["
        "|\\.\\." // no ".."
        "|/\\."   // no slashdot
        "|:"      // no ":"
        "|@\\{"   // no "@{" sequence
        "|\\\\"   // no backslash
        "|//"     // no double slash
        "|^[/-]"  // no leading slash or dash
        "|\""     // no quotes
        "|\\*"    // no asterisk
        "|(^|[A-Z]+_)HEAD" // no HEAD, FETCH_HEAD etc.
    );
}

Command *GitPluginPrivate::createCommand(QAction *action, ActionContainer *ac, Id id,
                                  const Context &context, bool addToLocator,
                                  const std::function<void()> &callback, const QKeySequence &keys)
{
    Command *command = ActionManager::registerAction(action, id, context);
    if (!keys.isEmpty())
        command->setDefaultKeySequence(keys);
    if (ac)
        ac->addAction(command);
    if (addToLocator)
        m_commandLocator->appendCommand(command);
    connect(action, &QAction::triggered, this, callback);
    return command;
}

// Create a parameter action
ParameterAction *GitPluginPrivate::createParameterAction(ActionContainer *ac,
                                                  const QString &defaultText, const QString &parameterText,
                                                  Id id, const Context &context,
                                                  bool addToLocator, const std::function<void()> &callback,
                                                  const QKeySequence &keys)
{
    auto action = new ParameterAction(defaultText, parameterText, ParameterAction::EnabledWithParameter, this);
    Command *command = createCommand(action, ac, id, context, addToLocator, callback, keys);
    command->setAttribute(Command::CA_UpdateText);
    return action;
}

// Create an action to act on a file.
QAction *GitPluginPrivate::createFileAction(ActionContainer *ac,
                                     const QString &defaultText, const QString &parameterText,
                                     Id id, const Context &context, bool addToLocator,
                                     const std::function<void()> &callback,
                                     const QKeySequence &keys)
{
    ParameterAction *action = createParameterAction(ac, defaultText, parameterText, id, context,
                                                    addToLocator, callback, keys);
    m_fileActions.push_back(action);
    return action;
}

QAction *GitPluginPrivate::createProjectAction(ActionContainer *ac, const QString &defaultText,
                                        const QString &parameterText, Id id, const Context &context,
                                        bool addToLocator, void (GitPluginPrivate::*func)(),
                                        const QKeySequence &keys)
{
    ParameterAction *action = createParameterAction(ac, defaultText, parameterText, id, context,
                                                    addToLocator, std::bind(func, this), keys);
    m_projectActions.push_back(action);
    return action;
}

// Create an action to act on the repository
QAction *GitPluginPrivate::createRepositoryAction(ActionContainer *ac, const QString &text, Id id,
                                           const Context &context, bool addToLocator,
                                           const std::function<void()> &callback,
                                           const QKeySequence &keys)
{
    auto action = new QAction(text, this);
    createCommand(action, ac, id, context, addToLocator, callback, keys);
    m_repositoryActions.push_back(action);
    return action;
}

QAction *GitPluginPrivate::createChangeRelatedRepositoryAction(const QString &text, Id id,
                                                        const Context &context)
{
    return createRepositoryAction(nullptr, text, id, context, true,
                                  std::bind(&GitPluginPrivate::startChangeRelatedAction, this, id),
                                  QKeySequence());
}

// Action to act on the repository forwarded to a git client member function
// taking the directory.
QAction *GitPluginPrivate::createRepositoryAction(ActionContainer *ac, const QString &text, Id id,
                                           const Context &context, bool addToLocator,
                                           GitClientMemberFunc func, const QKeySequence &keys)
{
    auto cb = [this, func]() -> void {
        QTC_ASSERT(currentState().hasTopLevel(), return);
        (m_gitClient.*func)(currentState().topLevel());
    };
    // Set the member func as data and connect to GitClient method
    return createRepositoryAction(ac, text, id, context, addToLocator, cb, keys);
}

bool GitPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(errorMessage)

    dd = new GitPluginPrivate;

    auto cmdContext = new QObject(this);
    connect(Core::ICore::instance(), &Core::ICore::coreOpened, cmdContext, [this, cmdContext, arguments] {
        remoteCommand(arguments, QDir::currentPath(), {});
        cmdContext->deleteLater();
    });

    return true;
}

void GitPlugin::extensionsInitialized()
{
    dd->extensionsInitialized()    ;
}

GitPluginPrivate::GitPluginPrivate()
    : VcsBase::VcsBasePluginPrivate(Context(Constants::GIT_CONTEXT))
{
    dd = this;

    setTopicCache(new GitTopicCache(&m_gitClient));

    m_fileActions.reserve(10);
    m_projectActions.reserve(10);
    m_repositoryActions.reserve(50);

    Context context(Constants::GIT_CONTEXT);

    const QString prefix = "git";
    m_commandLocator = new CommandLocator("Git", prefix, prefix, this);

    //register actions
    ActionContainer *toolsContainer = ActionManager::actionContainer(Core::Constants::M_TOOLS);

    ActionContainer *gitContainer = ActionManager::createMenu("Git");
    gitContainer->menu()->setTitle(tr("&Git"));
    toolsContainer->addMenu(gitContainer);
    m_menuAction = gitContainer->menu()->menuAction();


    /*  "Current File" menu */
    ActionContainer *currentFileMenu = ActionManager::createMenu("Git.CurrentFileMenu");
    currentFileMenu->menu()->setTitle(tr("Current &File"));
    gitContainer->addMenu(currentFileMenu);

    createFileAction(currentFileMenu, tr("Diff Current File"), tr("Diff of \"%1\""),
                     "Git.Diff", context, true, std::bind(&GitPluginPrivate::diffCurrentFile, this),
                      QKeySequence(useMacShortcuts ? tr("Meta+G,Meta+D") : tr("Alt+G,Alt+D")));

    createFileAction(currentFileMenu, tr("Log Current File"), tr("Log of \"%1\""),
                     "Git.Log", context, true, std::bind(&GitPluginPrivate::logFile, this),
                     QKeySequence(useMacShortcuts ? tr("Meta+G,Meta+L") : tr("Alt+G,Alt+L")));

    createFileAction(currentFileMenu, tr("Blame Current File"), tr("Blame for \"%1\""),
                     "Git.Blame", context, true, std::bind(&GitPluginPrivate::blameFile, this),
                     QKeySequence(useMacShortcuts ? tr("Meta+G,Meta+B") : tr("Alt+G,Alt+B")));

    currentFileMenu->addSeparator(context);

    createFileAction(currentFileMenu, tr("Stage File for Commit"), tr("Stage \"%1\" for Commit"),
                     "Git.Stage", context, true, std::bind(&GitPluginPrivate::stageFile, this),
                     QKeySequence(useMacShortcuts ? tr("Meta+G,Meta+A") : tr("Alt+G,Alt+A")));

    createFileAction(currentFileMenu, tr("Unstage File from Commit"), tr("Unstage \"%1\" from Commit"),
                     "Git.Unstage", context, true, std::bind(&GitPluginPrivate::unstageFile, this));

    createFileAction(currentFileMenu, tr("Undo Unstaged Changes"), tr("Undo Unstaged Changes for \"%1\""),
                     "Git.UndoUnstaged", context,
                     true, std::bind(&GitPluginPrivate::undoFileChanges, this, false));

    createFileAction(currentFileMenu, tr("Undo Uncommitted Changes"), tr("Undo Uncommitted Changes for \"%1\""),
                     "Git.Undo", context,
                     true, std::bind(&GitPluginPrivate::undoFileChanges, this, true),
                     QKeySequence(useMacShortcuts ? tr("Meta+G,Meta+U") : tr("Alt+G,Alt+U")));


    /*  "Current Project" menu */
    ActionContainer *currentProjectMenu = ActionManager::createMenu("Git.CurrentProjectMenu");
    currentProjectMenu->menu()->setTitle(tr("Current &Project"));
    gitContainer->addMenu(currentProjectMenu);

    createProjectAction(currentProjectMenu, tr("Diff Current Project"), tr("Diff Project \"%1\""),
                        "Git.DiffProject", context, true, &GitPluginPrivate::diffCurrentProject,
                        QKeySequence(useMacShortcuts ? tr("Meta+G,Meta+Shift+D") : tr("Alt+G,Alt+Shift+D")));

    createProjectAction(currentProjectMenu, tr("Log Project"), tr("Log Project \"%1\""),
                        "Git.LogProject", context, true, &GitPluginPrivate::logProject,
                        QKeySequence(useMacShortcuts ? tr("Meta+G,Meta+K") : tr("Alt+G,Alt+K")));

    createProjectAction(currentProjectMenu, tr("Clean Project..."), tr("Clean Project \"%1\"..."),
                        "Git.CleanProject", context, true, &GitPluginPrivate::cleanProject);


    /*  "Local Repository" menu */
    ActionContainer *localRepositoryMenu = ActionManager::createMenu("Git.LocalRepositoryMenu");
    localRepositoryMenu->menu()->setTitle(tr("&Local Repository"));
    gitContainer->addMenu(localRepositoryMenu);

    createRepositoryAction(localRepositoryMenu, tr("Diff"), "Git.DiffRepository",
                           context, true, &GitClient::diffRepository);

    createRepositoryAction(localRepositoryMenu, tr("Log"), "Git.LogRepository",
                           context, true, std::bind(&GitPluginPrivate::logRepository, this));

    createRepositoryAction(localRepositoryMenu, tr("Reflog"), "Git.ReflogRepository",
                           context, true, std::bind(&GitPluginPrivate::reflogRepository, this));

    createRepositoryAction(localRepositoryMenu, tr("Clean..."), "Git.CleanRepository",
                           context, true, [this] { cleanRepository(); });

    createRepositoryAction(localRepositoryMenu, tr("Status"), "Git.StatusRepository",
                           context, true, &GitClient::status);

    // --------------
    localRepositoryMenu->addSeparator(context);

    createRepositoryAction(localRepositoryMenu, tr("Commit..."), "Git.Commit",
                           context, true, std::bind(&GitPluginPrivate::startCommit, this, SimpleCommit),
                           QKeySequence(useMacShortcuts ? tr("Meta+G,Meta+C") : tr("Alt+G,Alt+C")));

    createRepositoryAction(localRepositoryMenu, tr("Amend Last Commit..."), "Git.AmendCommit",
                           context, true, std::bind(&GitPluginPrivate::startCommit, this, AmendCommit));

    m_fixupCommitAction
            = createRepositoryAction(localRepositoryMenu,
                                     tr("Fixup Previous Commit..."), "Git.FixupCommit", context, true,
                                     std::bind(&GitPluginPrivate::startCommit, this, FixupCommit));

    // --------------
    localRepositoryMenu->addSeparator(context);

    createRepositoryAction(localRepositoryMenu, tr("Reset..."), "Git.Reset",
                           context, true, std::bind(&GitPluginPrivate::resetRepository, this));

    createRepositoryAction(localRepositoryMenu, tr("Recover Deleted Files"), "Git.RecoverDeleted",
                           context, true, std::bind(&GitPluginPrivate::recoverDeletedFiles, this));

    m_interactiveRebaseAction
            = createRepositoryAction(localRepositoryMenu,
                                     tr("Interactive Rebase..."), "Git.InteractiveRebase",
                                     context, true, std::bind(&GitPluginPrivate::startRebase, this));

    m_submoduleUpdateAction
            = createRepositoryAction(localRepositoryMenu,
                                     tr("Update Submodules"), "Git.SubmoduleUpdate",
                                     context, true, std::bind(&GitPluginPrivate::updateSubmodules, this));
    m_abortMergeAction
            = createRepositoryAction(localRepositoryMenu,
                                     tr("Abort Merge"), "Git.MergeAbort",
                                     context, true,
                                     std::bind(&GitPluginPrivate::continueOrAbortCommand, this));

    m_abortRebaseAction
            = createRepositoryAction(localRepositoryMenu,
                                     tr("Abort Rebase"), "Git.RebaseAbort",
                                     context, true,
                                     std::bind(&GitPluginPrivate::continueOrAbortCommand, this));

    m_abortCherryPickAction
            = createRepositoryAction(localRepositoryMenu,
                                     tr("Abort Cherry Pick"), "Git.CherryPickAbort",
                                     context, true,
                                     std::bind(&GitPluginPrivate::continueOrAbortCommand, this));

    m_abortRevertAction
            = createRepositoryAction(localRepositoryMenu,
                                     tr("Abort Revert"), "Git.RevertAbort",
                                     context, true,
                                     std::bind(&GitPluginPrivate::continueOrAbortCommand, this));

    m_continueRebaseAction
            = createRepositoryAction(localRepositoryMenu,
                                     tr("Continue Rebase"), "Git.RebaseContinue",
                                     context, true,
                                     std::bind(&GitPluginPrivate::continueOrAbortCommand, this));

    m_skipRebaseAction
            = createRepositoryAction(localRepositoryMenu,
                                     tr("Skip Rebase"), "Git.RebaseSkip",
                                     context, true,
                                     std::bind(&GitPluginPrivate::continueOrAbortCommand, this));

    m_continueCherryPickAction
            = createRepositoryAction(localRepositoryMenu,
                                     tr("Continue Cherry Pick"), "Git.CherryPickContinue",
                                     context, true,
                                     std::bind(&GitPluginPrivate::continueOrAbortCommand, this));

    m_continueRevertAction
            = createRepositoryAction(localRepositoryMenu,
                                     tr("Continue Revert"), "Git.RevertContinue",
                                     context, true,
                                     std::bind(&GitPluginPrivate::continueOrAbortCommand, this));

    // --------------
    localRepositoryMenu->addSeparator(context);

    createRepositoryAction(localRepositoryMenu, tr("Branches..."), "Git.BranchList",
                           context, true, std::bind(&GitPluginPrivate::branchList, this));

    // --------------
    localRepositoryMenu->addSeparator(context);

    // "Patch" menu
    ActionContainer *patchMenu = ActionManager::createMenu("Git.PatchMenu");
    patchMenu->menu()->setTitle(tr("&Patch"));
    localRepositoryMenu->addMenu(patchMenu);

    // Apply current file as patch is handled specially.
    m_applyCurrentFilePatchAction
            = createParameterAction(patchMenu,
                                    tr("Apply from Editor"), tr("Apply \"%1\""),
                                    "Git.ApplyCurrentFilePatch",
                                    context, true, std::bind(&GitPluginPrivate::applyCurrentFilePatch, this));
    createRepositoryAction(patchMenu, tr("Apply from File..."), "Git.ApplyPatch",
                           context, true, std::bind(&GitPluginPrivate::promptApplyPatch, this));

    // "Stash" menu
    ActionContainer *stashMenu = ActionManager::createMenu("Git.StashMenu");
    stashMenu->menu()->setTitle(tr("&Stash"));
    localRepositoryMenu->addMenu(stashMenu);

    createRepositoryAction(stashMenu, tr("Stashes..."), "Git.StashList",
                           context, false, std::bind(&GitPluginPrivate::stashList, this));

    stashMenu->addSeparator(context);

    QAction *action = createRepositoryAction(stashMenu, tr("Stash"), "Git.Stash",
                                             context, true, std::bind(&GitPluginPrivate::stash, this, false));
    action->setToolTip(tr("Saves the current state of your work and resets the repository."));

    action = createRepositoryAction(stashMenu, tr("Stash Unstaged Files"), "Git.StashUnstaged",
                                    context, true, std::bind(&GitPluginPrivate::stashUnstaged, this));
    action->setToolTip(tr("Saves the current state of your unstaged files and resets the repository "
                          "to its staged state."));

    action = createRepositoryAction(stashMenu, tr("Take Snapshot..."), "Git.StashSnapshot",
                                    context, true, std::bind(&GitPluginPrivate::stashSnapshot, this));
    action->setToolTip(tr("Saves the current state of your work."));

    stashMenu->addSeparator(context);

    action = createRepositoryAction(stashMenu, tr("Stash Pop"), "Git.StashPop",
                                    context, true, std::bind(&GitPluginPrivate::stashPop, this));
    action->setToolTip(tr("Restores changes saved to the stash list using \"Stash\"."));


    /* \"Local Repository" menu */

    // --------------

    /*  "Remote Repository" menu */
    ActionContainer *remoteRepositoryMenu = ActionManager::createMenu("Git.RemoteRepositoryMenu");
    remoteRepositoryMenu->menu()->setTitle(tr("&Remote Repository"));
    gitContainer->addMenu(remoteRepositoryMenu);

    createRepositoryAction(remoteRepositoryMenu, tr("Fetch"), "Git.Fetch",
                           context, true, std::bind(&GitPluginPrivate::fetch, this));

    createRepositoryAction(remoteRepositoryMenu, tr("Pull"), "Git.Pull",
                           context, true, std::bind(&GitPluginPrivate::pull, this));

    createRepositoryAction(remoteRepositoryMenu, tr("Push"), "Git.Push",
                           context, true, std::bind(&GitPluginPrivate::push, this));

    // --------------
    remoteRepositoryMenu->addSeparator(context);

    // "Subversion" menu
    ActionContainer *subversionMenu = ActionManager::createMenu("Git.Subversion");
    subversionMenu->menu()->setTitle(tr("&Subversion"));
    remoteRepositoryMenu->addMenu(subversionMenu);

    createRepositoryAction(subversionMenu, tr("Log"), "Git.Subversion.Log",
                           context, false, &GitClient::subversionLog);

    createRepositoryAction(subversionMenu, tr("Fetch"), "Git.Subversion.Fetch",
                           context, false, &GitClient::synchronousSubversionFetch);

    createRepositoryAction(subversionMenu, tr("DCommit"), "Git.Subversion.DCommit",
                           context, false, &GitClient::subversionDeltaCommit);

    // --------------
    remoteRepositoryMenu->addSeparator(context);

    createRepositoryAction(remoteRepositoryMenu, tr("Manage Remotes..."), "Git.RemoteList",
                           context, false, std::bind(&GitPluginPrivate::manageRemotes, this));

    /* \"Remote Repository" menu */

    // --------------

    /*  Actions only in locator */
    createChangeRelatedRepositoryAction(tr("Show..."), "Git.Show", context);
    createChangeRelatedRepositoryAction(tr("Revert..."), "Git.Revert", context);
    createChangeRelatedRepositoryAction(tr("Cherry Pick..."), "Git.CherryPick", context);
    createChangeRelatedRepositoryAction(tr("Checkout..."), "Git.Checkout", context);
    createChangeRelatedRepositoryAction(tr("Archive..."), "Git.Archive", context);

    createRepositoryAction(nullptr, tr("Rebase..."), "Git.Rebase", context, true,
                           std::bind(&GitPluginPrivate::branchList, this));
    createRepositoryAction(nullptr, tr("Merge..."), "Git.Merge", context, true,
                           std::bind(&GitPluginPrivate::branchList, this));
    /*  \Actions only in locator */

    // --------------

    /*  "Git Tools" menu */
    ActionContainer *gitToolsMenu = ActionManager::createMenu("Git.GitToolsMenu");
    gitToolsMenu->menu()->setTitle(tr("Git &Tools"));
    gitContainer->addMenu(gitToolsMenu);

    createRepositoryAction(gitToolsMenu, tr("Gitk"), "Git.LaunchGitK",
                           context, true, &GitClient::launchGitK);

    createFileAction(gitToolsMenu, tr("Gitk Current File"), tr("Gitk of \"%1\""),
                     "Git.GitkFile", context, true, std::bind(&GitPluginPrivate::gitkForCurrentFile, this));

    createFileAction(gitToolsMenu, tr("Gitk for folder of Current File"), tr("Gitk for folder of \"%1\""),
                     "Git.GitkFolder", context, true, std::bind(&GitPluginPrivate::gitkForCurrentFolder, this));

    // --------------
    gitToolsMenu->addSeparator(context);

    createRepositoryAction(gitToolsMenu, tr("Git Gui"), "Git.GitGui",
                           context, true, std::bind(&GitPluginPrivate::gitGui, this));

    // --------------
    gitToolsMenu->addSeparator(context);

    m_repositoryBrowserAction
            = createRepositoryAction(gitToolsMenu,
                                     tr("Repository Browser"), "Git.LaunchRepositoryBrowser",
                                     context, true, &GitClient::launchRepositoryBrowser);

    m_mergeToolAction
            = createRepositoryAction(gitToolsMenu, tr("Merge Tool"), "Git.MergeTool",
                                     context, true, std::bind(&GitPluginPrivate::startMergeTool, this));

    // --------------
    if (Utils::HostOsInfo::isWindowsHost()) {
        gitToolsMenu->addSeparator(context);

        createRepositoryAction(gitToolsMenu, tr("Git Bash"), "Git.GitBash",
                               context, true, std::bind(&GitPluginPrivate::gitBash, this));
    }

    /* \"Git Tools" menu */

    // --------------
    gitContainer->addSeparator(context);

    QAction *actionsOnCommitsAction = new QAction(tr("Actions on Commits..."), this);
    Command *actionsOnCommitsCommand = ActionManager::registerAction(
                actionsOnCommitsAction, "Git.ChangeActions");
    connect(actionsOnCommitsAction, &QAction::triggered, this,
            [this] { startChangeRelatedAction("Git.ChangeActions"); });
    gitContainer->addAction(actionsOnCommitsCommand);

    QAction *createRepositoryAction = new QAction(tr("Create Repository..."), this);
    Command *createRepositoryCommand = ActionManager::registerAction(
                createRepositoryAction, "Git.CreateRepository");
    connect(createRepositoryAction, &QAction::triggered, this, &GitPluginPrivate::createRepository);
    gitContainer->addAction(createRepositoryCommand);

    connect(VcsManager::instance(), &VcsManager::repositoryChanged,
            this, &GitPluginPrivate::updateContinueAndAbortCommands);
    connect(VcsManager::instance(), &VcsManager::repositoryChanged,
            this, &GitPluginPrivate::updateBranches, Qt::QueuedConnection);

    /* "Gerrit" */
    m_gerritPlugin = new Gerrit::Internal::GerritPlugin(this);
    m_gerritPlugin->initialize(remoteRepositoryMenu);
    m_gerritPlugin->updateActions(currentState());
    m_gerritPlugin->addToLocator(m_commandLocator);

}

void GitPluginPrivate::diffCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_gitClient.diffFile(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void GitPluginPrivate::diffCurrentProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    const QString relativeProject = state.relativeCurrentProject();
    if (relativeProject.isEmpty())
        m_gitClient.diffRepository(state.currentProjectTopLevel());
    else
        m_gitClient.diffProject(state.currentProjectTopLevel(), relativeProject);
}

void GitPluginPrivate::logFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_gitClient.log(state.currentFileTopLevel(), state.relativeCurrentFile(), true);
}

void GitPluginPrivate::blameFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    const int lineNumber = VcsBaseEditor::lineNumberOfCurrentEditor(state.currentFile());
    QStringList extraOptions;
    int firstLine = -1;
    if (BaseTextEditor *textEditor = BaseTextEditor::currentTextEditor()) {
        QTextCursor cursor = textEditor->textCursor();
        if (cursor.hasSelection()) {
            QString argument = "-L ";
            int selectionStart = cursor.selectionStart();
            int selectionEnd = cursor.selectionEnd();
            cursor.setPosition(selectionStart);
            const int startBlock = cursor.blockNumber();
            cursor.setPosition(selectionEnd);
            int endBlock = cursor.blockNumber();
            if (startBlock != endBlock) {
                firstLine = startBlock + 1;
                if (cursor.atBlockStart())
                    --endBlock;
                if (auto widget = qobject_cast<VcsBaseEditorWidget *>(textEditor->widget())) {
                    const int previousFirstLine = widget->firstLineNumber();
                    if (previousFirstLine > 0)
                        firstLine = previousFirstLine;
                }
                argument += QString::number(firstLine) + ',';
                argument += QString::number(endBlock + firstLine - startBlock);
                extraOptions << argument;
            }
        }
    }
    VcsBaseEditorWidget *editor = m_gitClient.annotate(
                state.currentFileTopLevel(), state.relativeCurrentFile(), QString(),
                lineNumber, extraOptions);
    if (firstLine > 0)
        editor->setFirstLineNumber(firstLine);
}

void GitPluginPrivate::logProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    m_gitClient.log(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void GitPluginPrivate::logRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_gitClient.log(state.topLevel());
}

void GitPluginPrivate::reflogRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_gitClient.reflog(state.topLevel());
}

void GitPluginPrivate::undoFileChanges(bool revertStaging)
{
    if (IDocument *document = EditorManager::currentDocument()) {
        if (!DocumentManager::saveModifiedDocumentSilently(document))
            return;
    }
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    FileChangeBlocker fcb(state.currentFile());
    m_gitClient.revert({state.currentFile()}, revertStaging);
}

class ResetItemDelegate : public LogItemDelegate
{
public:
    ResetItemDelegate(LogChangeWidget *widget) : LogItemDelegate(widget) {}
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override
    {
        if (index.row() < currentRow())
            option->font.setStrikeOut(true);
        LogItemDelegate::initStyleOption(option, index);
    }
};

class RebaseItemDelegate : public IconItemDelegate
{
public:
    RebaseItemDelegate(LogChangeWidget *widget)
        : IconItemDelegate(widget, Utils::Icons::UNDO)
    {
    }

protected:
    bool hasIcon(int row) const override
    {
        return row <= currentRow();
    }
};

void GitPluginPrivate::resetRepository()
{
    if (!DocumentManager::saveAllModifiedDocuments())
        return;
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    QString topLevel = state.topLevel();

    LogChangeDialog dialog(true, ICore::dialogParent());
    ResetItemDelegate delegate(dialog.widget());
    dialog.setWindowTitle(tr("Undo Changes to %1").arg(QDir::toNativeSeparators(topLevel)));
    if (dialog.runDialog(topLevel, QString(), LogChangeWidget::IncludeRemotes))
        m_gitClient.reset(topLevel, dialog.resetFlag(), dialog.commit());
}

void GitPluginPrivate::recoverDeletedFiles()
{
    if (!DocumentManager::saveAllModifiedDocuments())
        return;
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_gitClient.recoverDeletedFiles(state.topLevel());
}

void GitPluginPrivate::startRebase()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    const QString topLevel = state.topLevel();

    startRebaseFromCommit(topLevel, QString());
}

void GitPluginPrivate::startRebaseFromCommit(const QString &workingDirectory, QString commit)
{
    if (!DocumentManager::saveAllModifiedDocuments())
        return;
    if (workingDirectory.isEmpty() || !m_gitClient.canRebase(workingDirectory))
        return;

    if (commit.isEmpty()) {
        LogChangeDialog dialog(false, ICore::dialogParent());
        RebaseItemDelegate delegate(dialog.widget());
        dialog.setWindowTitle(tr("Interactive Rebase"));
        if (!dialog.runDialog(workingDirectory))
            return;
        commit = dialog.commit();
    }

    if (m_gitClient.beginStashScope(workingDirectory, "Rebase-i"))
        m_gitClient.interactiveRebase(workingDirectory, commit, false);
}

void GitPluginPrivate::startChangeRelatedAction(const Id &id)
{
    const VcsBasePluginState state = currentState();

    ChangeSelectionDialog dialog(state.hasTopLevel() ? state.topLevel() : PathChooser::homePath(),
                                 id, ICore::dialogParent());

    int result = dialog.exec();

    if (result == QDialog::Rejected)
        return;

    const QString workingDirectory = dialog.workingDirectory();
    const QString change = dialog.change();

    if (workingDirectory.isEmpty() || change.isEmpty())
        return;

    if (dialog.command() == Show) {
        m_gitClient.show(workingDirectory, change);
        return;
    }

    if (dialog.command() == Archive) {
        m_gitClient.archive(workingDirectory, change);
        return;
    }

    if (!DocumentManager::saveAllModifiedDocuments())
        return;

    switch (dialog.command()) {
    case CherryPick:
        m_gitClient.synchronousCherryPick(workingDirectory, change);
        break;
    case Revert:
        m_gitClient.synchronousRevert(workingDirectory, change);
        break;
    case Checkout:
        m_gitClient.checkout(workingDirectory, change);
        break;
    default:
        return;
    }
}

void GitPluginPrivate::stageFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_gitClient.addFile(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void GitPluginPrivate::unstageFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_gitClient.synchronousReset(state.currentFileTopLevel(), {state.relativeCurrentFile()});
}

void GitPluginPrivate::gitkForCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_gitClient.launchGitK(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void GitPluginPrivate::gitkForCurrentFolder()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);

    /*
     *  entire lower part of the code can be easily replaced with one line:
     *
     *  m_gitClient.launchGitK(dir.currentFileDirectory(), ".");
     *
     *  However, there is a bug in gitk in version 1.7.9.5, and if you run above
     *  command, there will be no documents listed in lower right section.
     *
     *  This is why I use lower combination in order to avoid this problems in gitk.
     *
     *  Git version 1.7.10.4 does not have this issue, and it can easily use
     *  one line command mentioned above.
     *
     */
    QDir dir(state.currentFileDirectory());
    if (QFileInfo(dir,".git").exists() || dir.cd(".git")) {
        m_gitClient.launchGitK(state.currentFileDirectory());
    } else {
        QString folderName = dir.absolutePath();
        dir.cdUp();
        folderName = folderName.remove(0, dir.absolutePath().length() + 1);
        m_gitClient.launchGitK(dir.absolutePath(), folderName);
    }
}

void GitPluginPrivate::gitGui()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_gitClient.launchGitGui(state.topLevel());
}

void GitPluginPrivate::gitBash()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_gitClient.launchGitBash(state.topLevel());
}

void GitPluginPrivate::startCommit(CommitType commitType)
{
    if (!promptBeforeCommit())
        return;

    if (raiseSubmitEditor())
        return;
    if (isCommitEditorOpen()) {
        VcsOutputWindow::appendWarning(tr("Another submit is currently being executed."));
        return;
    }

    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    QString errorMessage, commitTemplate;
    CommitData data(commitType);
    if (!m_gitClient.getCommitData(state.topLevel(), &commitTemplate, data, &errorMessage)) {
        VcsOutputWindow::appendError(errorMessage);
        return;
    }

    // Store repository for diff and the original list of
    // files to be able to unstage files the user unchecks
    m_submitRepository = data.panelInfo.repository;

    // Start new temp file with message template
    TempFileSaver saver;
    // Keep the file alive, else it removes self and forgets its name
    saver.setAutoRemove(false);
    saver.write(commitTemplate.toLocal8Bit());
    if (!saver.finalize()) {
        VcsOutputWindow::appendError(saver.errorString());
        return;
    }
    m_commitMessageFileName = saver.fileName();
    openSubmitEditor(m_commitMessageFileName, data);
}

void GitPluginPrivate::updateVersionWarning()
{
    unsigned version = m_gitClient.gitVersion();
    if (!version || version >= minimumRequiredVersion)
        return;
    IDocument *curDocument = EditorManager::currentDocument();
    if (!curDocument)
        return;
    InfoBar *infoBar = curDocument->infoBar();
    Id gitVersionWarning("GitVersionWarning");
    if (!infoBar->canInfoBeAdded(gitVersionWarning))
        return;
    infoBar->addInfo(InfoBarEntry(gitVersionWarning,
                        tr("Unsupported version of Git found. Git %1 or later required.")
                        .arg(versionString(minimumRequiredVersion)),
                        InfoBarEntry::GlobalSuppression::Enabled));
}

IEditor *GitPluginPrivate::openSubmitEditor(const QString &fileName, const CommitData &cd)
{
    IEditor *editor = EditorManager::openEditor(fileName, Constants::GITSUBMITEDITOR_ID);
    auto submitEditor = qobject_cast<GitSubmitEditor*>(editor);
    QTC_ASSERT(submitEditor, return nullptr);
    setSubmitEditor(submitEditor);
    submitEditor->setCommitData(cd);
    submitEditor->setCheckScriptWorkingDirectory(m_submitRepository);
    QString title;
    switch (cd.commitType) {
    case AmendCommit:
        title = tr("Amend %1").arg(cd.amendSHA1);
        break;
    case FixupCommit:
        title = tr("Git Fixup Commit");
        break;
    default:
        title = tr("Git Commit");
    }
    IDocument *document = submitEditor->document();
    document->setPreferredDisplayName(title);
    VcsBase::setSource(document, m_submitRepository);
    return editor;
}

void GitPluginPrivate::commitFromEditor()
{
    // Close the submit editor
    m_submitActionTriggered = true;
    QTC_ASSERT(submitEditor(), return);
    EditorManager::closeDocument(submitEditor()->document());
}

bool GitPluginPrivate::submitEditorAboutToClose()
{
    if (!isCommitEditorOpen())
        return true;
    auto editor = qobject_cast<GitSubmitEditor *>(submitEditor());
    QTC_ASSERT(editor, return true);
    IDocument *editorDocument = editor->document();
    QTC_ASSERT(editorDocument, return true);
    // Submit editor closing. Make it write out the commit message
    // and retrieve files
    const QFileInfo editorFile = editorDocument->filePath().toFileInfo();
    const QFileInfo changeFile(m_commitMessageFileName);
    // Paranoia!
    if (editorFile.absoluteFilePath() != changeFile.absoluteFilePath())
        return true;
    // Prompt user. Force a prompt unless submit was actually invoked (that
    // is, the editor was closed or shutdown).
    const VcsBaseSubmitEditor::PromptSubmitResult answer
            = editor->promptSubmit(this, nullptr, !m_submitActionTriggered, false);
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


    // Go ahead!
    auto model = qobject_cast<SubmitFileModel *>(editor->fileModel());
    CommitType commitType = editor->commitType();
    QString amendSHA1 = editor->amendSHA1();
    if (model->hasCheckedFiles() || !amendSHA1.isEmpty()) {
        // get message & commit
        if (!DocumentManager::saveDocument(editorDocument))
            return false;

        if (!m_gitClient.addAndCommit(m_submitRepository, editor->panelData(), commitType,
                                       amendSHA1, m_commitMessageFileName, model)) {
            editor->updateFileModel();
            return false;
        }
    }
    cleanCommitMessageFile();
    if (commitType == FixupCommit) {
        if (!m_gitClient.beginStashScope(m_submitRepository, "Rebase-fixup",
                                          NoPrompt, editor->panelData().pushAction)) {
            return false;
        }
        m_gitClient.interactiveRebase(m_submitRepository, amendSHA1, true);
    } else {
        m_gitClient.continueCommandIfNeeded(m_submitRepository);
        if (editor->panelData().pushAction == NormalPush) {
            m_gitClient.push(m_submitRepository);
        } else if (editor->panelData().pushAction == PushToGerrit) {
            connect(editor, &QObject::destroyed, this, &GitPluginPrivate::delayedPushToGerrit,
                    Qt::QueuedConnection);
        }
    }

    return true;
}

void GitPluginPrivate::fetch()
{
    m_gitClient.fetch(currentState().topLevel(), QString());
}

void GitPluginPrivate::pull()
{
    if (!DocumentManager::saveAllModifiedDocuments())
        return;
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    QString topLevel = state.topLevel();
    bool rebase = m_settings.boolValue(GitSettings::pullRebaseKey);

    if (!rebase) {
        QString currentBranch = m_gitClient.synchronousCurrentLocalBranch(topLevel);
        if (!currentBranch.isEmpty()) {
            currentBranch.prepend("branch.");
            currentBranch.append(".rebase");
            rebase = (m_gitClient.readConfigValue(topLevel, currentBranch) == "true");
        }
    }

    if (!m_gitClient.beginStashScope(topLevel, "Pull", rebase ? Default : AllowUnstashed))
        return;
    m_gitClient.pull(topLevel, rebase);
}

void GitPluginPrivate::push()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_gitClient.push(state.topLevel());
}

void GitPluginPrivate::startMergeTool()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_gitClient.merge(state.topLevel());
}

void GitPluginPrivate::continueOrAbortCommand()
{
    if (!DocumentManager::saveAllModifiedDocuments())
        return;
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    QObject *action = QObject::sender();

    if (action == m_abortMergeAction)
        m_gitClient.synchronousMerge(state.topLevel(), "--abort");
    else if (action == m_abortRebaseAction)
        m_gitClient.rebase(state.topLevel(), "--abort");
    else if (action == m_abortCherryPickAction)
        m_gitClient.synchronousCherryPick(state.topLevel(), "--abort");
    else if (action == m_abortRevertAction)
        m_gitClient.synchronousRevert(state.topLevel(), "--abort");
    else if (action == m_skipRebaseAction)
        m_gitClient.rebase(state.topLevel(), "--skip");
    else if (action == m_continueRebaseAction)
        m_gitClient.rebase(state.topLevel(), "--continue");
    else if (action == m_continueCherryPickAction)
        m_gitClient.cherryPick(state.topLevel(), "--continue");
    else if (action == m_continueRevertAction)
        m_gitClient.revert(state.topLevel(), "--continue");

    updateContinueAndAbortCommands();
}

void GitPluginPrivate::cleanProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    cleanRepository(state.currentProjectPath());
}

void GitPluginPrivate::cleanRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    cleanRepository(state.topLevel());
}

void GitPluginPrivate::cleanRepository(const QString &directory)
{
    // Find files to be deleted
    QString errorMessage;
    QStringList files;
    QStringList ignoredFiles;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const bool gotFiles = m_gitClient.synchronousCleanList(directory, QString(), &files, &ignoredFiles, &errorMessage);
    QApplication::restoreOverrideCursor();

    if (!gotFiles) {
        Core::AsynchronousMessageBox::warning(tr("Unable to Retrieve File List"), errorMessage);
        return;
    }
    if (files.isEmpty() && ignoredFiles.isEmpty()) {
        Core::AsynchronousMessageBox::information(tr("Repository Clean"),
                                                   tr("The repository is clean."));
        return;
    }

    // Show in dialog
    CleanDialog dialog(ICore::dialogParent());
    dialog.setFileList(directory, files, ignoredFiles);
    dialog.exec();
}

void GitPluginPrivate::updateSubmodules()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_gitClient.updateSubmodulesIfNeeded(state.topLevel(), false);
}

// If the file is modified in an editor, make sure it is saved.
static bool ensureFileSaved(const QString &fileName)
{
    return DocumentManager::saveModifiedDocument(DocumentModel::documentForFilePath(fileName));
}

void GitPluginPrivate::applyCurrentFilePatch()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasPatchFile() && state.hasTopLevel(), return);
    const QString patchFile = state.currentPatchFile();
    if (!ensureFileSaved(patchFile))
        return;
    applyPatch(state.topLevel(), patchFile);
}

void GitPluginPrivate::promptApplyPatch()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    applyPatch(state.topLevel(), QString());
}

void GitPluginPrivate::applyPatch(const QString &workingDirectory, QString file)
{
    // Ensure user has been notified about pending changes
    if (!m_gitClient.beginStashScope(workingDirectory, "Apply-Patch", AllowUnstashed))
        return;
    // Prompt for file
    if (file.isEmpty()) {
        const QString filter = tr("Patches (*.patch *.diff)");
        file = QFileDialog::getOpenFileName(ICore::dialogParent(), tr("Choose Patch"), QString(), filter);
        if (file.isEmpty()) {
            m_gitClient.endStashScope(workingDirectory);
            return;
        }
    }
    // Run!
    QString errorMessage;
    if (m_gitClient.synchronousApplyPatch(workingDirectory, file, &errorMessage)) {
        if (errorMessage.isEmpty())
            VcsOutputWindow::appendMessage(tr("Patch %1 successfully applied to %2").arg(file, workingDirectory));
        else
            VcsOutputWindow::appendError(errorMessage);
    } else {
        VcsOutputWindow::appendError(errorMessage);
    }
    m_gitClient.endStashScope(workingDirectory);
}

void GitPluginPrivate::stash(bool unstagedOnly)
{
    if (!DocumentManager::saveAllModifiedDocuments())
        return;
    // Simple stash without prompt, reset repo.
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    const QString topLevel = state.topLevel();
    m_gitClient.executeSynchronousStash(topLevel, QString(), unstagedOnly);
    if (m_stashDialog)
        m_stashDialog->refresh(topLevel, true);
}

void GitPluginPrivate::stashUnstaged()
{
    stash(true);
}

void GitPluginPrivate::stashSnapshot()
{
    // Prompt for description, restore immediately and keep on working.
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    const QString id = m_gitClient.synchronousStash(state.topLevel(), QString(),
                GitClient::StashImmediateRestore | GitClient::StashPromptDescription);
    if (!id.isEmpty() && m_stashDialog)
        m_stashDialog->refresh(state.topLevel(), true);
}

void GitPluginPrivate::stashPop()
{
    if (!DocumentManager::saveAllModifiedDocuments())
        return;
    const QString repository = currentState().topLevel();
    m_gitClient.stashPop(repository);
    if (m_stashDialog)
        m_stashDialog->refresh(repository, true);
}

// Create a non-modal dialog with refresh function or raise if it exists
template <class NonModalDialog>
    inline void showNonModalDialog(const QString &topLevel,
                                   QPointer<NonModalDialog> &dialog)
{
    if (dialog) {
        dialog->show();
        dialog->raise();
    } else {
        dialog = new NonModalDialog(ICore::dialogParent());
        dialog->refresh(topLevel, true);
        dialog->show();
    }
}

void GitPluginPrivate::branchList()
{
    ModeManager::activateMode(Core::Constants::MODE_EDIT);
    NavigationWidget::activateSubWidget(Constants::GIT_BRANCH_VIEW_ID, Side::Right);
}

void GitPluginPrivate::manageRemotes()
{
    showNonModalDialog(currentState().topLevel(), m_remoteDialog);
}

void GitPluginPrivate::initRepository()
{
    createRepository();
}

void GitPluginPrivate::stashList()
{
    showNonModalDialog(currentState().topLevel(), m_stashDialog);
}

void GitPluginPrivate::updateActions(VcsBasePluginPrivate::ActionState as)
{
    const VcsBasePluginState state = currentState();
    const bool repositoryEnabled = state.hasTopLevel();
    if (m_stashDialog)
        m_stashDialog->refresh(state.topLevel(), false);
    if (m_branchViewFactory.view())
        m_branchViewFactory.view()->refresh(state.topLevel(), false);
    if (m_remoteDialog)
        m_remoteDialog->refresh(state.topLevel(), false);

    m_commandLocator->setEnabled(repositoryEnabled);
    if (!enableMenuAction(as, m_menuAction))
        return;
    if (repositoryEnabled)
        updateVersionWarning();
    // Note: This menu is visible if there is no repository. Only
    // 'Create Repository'/'Show' actions should be available.
    const QString fileName = Utils::quoteAmpersands(state.currentFileName());
    for (ParameterAction *fileAction : qAsConst(m_fileActions))
        fileAction->setParameter(fileName);
    // If the current file looks like a patch, offer to apply
    m_applyCurrentFilePatchAction->setParameter(state.currentPatchFileDisplayName());
    const QString projectName = state.currentProjectName();
    for (ParameterAction *projectAction : qAsConst(m_projectActions))
        projectAction->setParameter(projectName);

    for (QAction *repositoryAction : qAsConst(m_repositoryActions))
        repositoryAction->setEnabled(repositoryEnabled);

    m_submoduleUpdateAction->setVisible(repositoryEnabled
            && !m_gitClient.submoduleList(state.topLevel()).isEmpty());

    updateContinueAndAbortCommands();
    updateRepositoryBrowserAction();

    m_gerritPlugin->updateActions(state);
}

void GitPluginPrivate::updateContinueAndAbortCommands()
{
    if (currentState().hasTopLevel()) {
        GitClient::CommandInProgress gitCommandInProgress =
                m_gitClient.checkCommandInProgress(currentState().topLevel());

        m_mergeToolAction->setVisible(gitCommandInProgress != GitClient::NoCommand);
        m_abortMergeAction->setVisible(gitCommandInProgress == GitClient::Merge);
        m_abortCherryPickAction->setVisible(gitCommandInProgress == GitClient::CherryPick);
        m_abortRevertAction->setVisible(gitCommandInProgress == GitClient::Revert);
        m_abortRebaseAction->setVisible(gitCommandInProgress == GitClient::Rebase
                                        || gitCommandInProgress == GitClient::RebaseMerge);
        m_skipRebaseAction->setVisible(gitCommandInProgress == GitClient::Rebase
                                        || gitCommandInProgress == GitClient::RebaseMerge);
        m_continueCherryPickAction->setVisible(gitCommandInProgress == GitClient::CherryPick);
        m_continueRevertAction->setVisible(gitCommandInProgress == GitClient::Revert);
        m_continueRebaseAction->setVisible(gitCommandInProgress == GitClient::Rebase
                                           || gitCommandInProgress == GitClient::RebaseMerge);
        m_fixupCommitAction->setEnabled(gitCommandInProgress == GitClient::NoCommand);
        m_interactiveRebaseAction->setEnabled(gitCommandInProgress == GitClient::NoCommand);
    } else {
        m_mergeToolAction->setVisible(false);
        m_abortMergeAction->setVisible(false);
        m_abortCherryPickAction->setVisible(false);
        m_abortRevertAction->setVisible(false);
        m_abortRebaseAction->setVisible(false);
        m_skipRebaseAction->setVisible(false);
        m_continueCherryPickAction->setVisible(false);
        m_continueRevertAction->setVisible(false);
        m_continueRebaseAction->setVisible(false);
    }
}

void GitPluginPrivate::delayedPushToGerrit()
{
    m_gerritPlugin->push(m_submitRepository);
}

void GitPluginPrivate::updateBranches(const QString &repository)
{
    if (m_branchViewFactory.view())
        m_branchViewFactory.view()->refreshIfSame(repository);
}

void GitPluginPrivate::updateCurrentBranch()
{
    if (m_branchViewFactory.view())
        m_branchViewFactory.view()->refreshCurrentBranch();
}

QObject *GitPlugin::remoteCommand(const QStringList &options, const QString &workingDirectory,
                                  const QStringList &)
{
    if (options.size() < 2)
        return nullptr;

    if (options.first() == "-git-show")
        dd->m_gitClient.show(workingDirectory, options.at(1));
    return nullptr;
}

void GitPluginPrivate::updateRepositoryBrowserAction()
{
    const bool repositoryEnabled = currentState().hasTopLevel();
    const bool hasRepositoryBrowserCmd
            = !m_settings.stringValue(GitSettings::repositoryBrowserCmd).isEmpty();
    m_repositoryBrowserAction->setEnabled(repositoryEnabled && hasRepositoryBrowserCmd);
}

QString GitPluginPrivate::displayName() const
{
    return QLatin1String("Git");
}

Utils::Id GitPluginPrivate::id() const
{
    return Utils::Id(VcsBase::Constants::VCS_ID_GIT);
}

bool GitPluginPrivate::isVcsFileOrDirectory(const Utils::FilePath &fileName) const
{
    if (fileName.fileName().compare(".git", Utils::HostOsInfo::fileNameCaseSensitivity()))
        return false;
    if (fileName.isDir())
        return true;
    QFile file(fileName.toString());
    if (!file.open(QFile::ReadOnly))
        return false;
    return file.read(8) == "gitdir: ";
}

bool GitPluginPrivate::isConfigured() const
{
    return !m_gitClient.vcsBinary().isEmpty();
}

bool GitPluginPrivate::supportsOperation(Operation operation) const
{
    if (!isConfigured())
        return false;

    switch (operation) {
    case AddOperation:
    case DeleteOperation:
    case MoveOperation:
    case CreateRepositoryOperation:
    case SnapshotOperations:
    case AnnotateOperation:
    case InitialCheckoutOperation:
        return true;
    }
    return false;
}

bool GitPluginPrivate::vcsOpen(const QString & /*fileName*/)
{
    return false;
}

bool GitPluginPrivate::vcsAdd(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return m_gitClient.synchronousAdd(fi.absolutePath(), {fi.fileName()});
}

bool GitPluginPrivate::vcsDelete(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return m_gitClient.synchronousDelete(fi.absolutePath(), true, {fi.fileName()});
}

bool GitPluginPrivate::vcsMove(const QString &from, const QString &to)
{
    const QFileInfo fromInfo(from);
    const QFileInfo toInfo(to);
    return m_gitClient.synchronousMove(fromInfo.absolutePath(), fromInfo.absoluteFilePath(), toInfo.absoluteFilePath());
}

bool GitPluginPrivate::vcsCreateRepository(const QString &directory)
{
    return m_gitClient.synchronousInit(directory);
}

QString GitPluginPrivate::vcsTopic(const QString &directory)
{
    QString topic = Core::IVersionControl::vcsTopic(directory);
    const QString commandInProgress = m_gitClient.commandInProgressDescription(directory);
    if (!commandInProgress.isEmpty())
        topic += " (" + commandInProgress + ')';
    return topic;
}

Core::ShellCommand *GitPluginPrivate::createInitialCheckoutCommand(const QString &url,
                                                                    const Utils::FilePath &baseDirectory,
                                                                    const QString &localName,
                                                                    const QStringList &extraArgs)
{
    QStringList args = {"clone", "--progress"};
    args << extraArgs << url << localName;

    auto command = new VcsBase::VcsCommand(baseDirectory.toString(), m_gitClient.processEnvironment());
    command->addFlags(VcsBase::VcsCommand::SuppressStdErr);
    command->addJob({m_gitClient.vcsBinary(), args}, -1);
    return command;
}

GitPluginPrivate::RepoUrl GitPluginPrivate::getRepoUrl(const QString &location) const
{
    return GitRemote(location);
}

QStringList GitPluginPrivate::additionalToolsPath() const
{
    QStringList res = m_gitClient.settings().searchPathList();
    const QString binaryPath = m_gitClient.gitBinDirectory().toString();
    if (!binaryPath.isEmpty() && !res.contains(binaryPath))
        res << binaryPath;
    return res;
}

bool GitPluginPrivate::managesDirectory(const QString &directory, QString *topLevel) const
{
    const QString topLevelFound = m_gitClient.findRepositoryForDirectory(directory);
    if (topLevel)
        *topLevel = topLevelFound;
    return !topLevelFound.isEmpty();
}

bool GitPluginPrivate::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    return m_gitClient.managesFile(workingDirectory, fileName);
}

QStringList GitPluginPrivate::unmanagedFiles(const QString &workingDir,
                                              const QStringList &filePaths) const
{
    return m_gitClient.unmanagedFiles(workingDir, filePaths);
}

void GitPluginPrivate::vcsAnnotate(const QString &file, int line)
{
    const QFileInfo fi(file);
    m_gitClient.annotate(fi.absolutePath(), fi.fileName(), QString(), line);
}

void GitPlugin::emitFilesChanged(const QStringList &l)
{
    emit dd->filesChanged(l);
}

void GitPlugin::emitRepositoryChanged(const QString &r)
{
    emit dd->repositoryChanged(r);
}

void GitPlugin::startRebaseFromCommit(const QString &workingDirectory, const QString &commit)
{
    dd->startRebaseFromCommit(workingDirectory, commit);
}

void GitPlugin::manageRemotes()
{
    dd->manageRemotes();
}

void GitPlugin::initRepository()
{
    dd->initRepository();
}

void GitPlugin::startCommit()
{
    dd->startCommit();
}

void GitPlugin::updateCurrentBranch()
{
    dd->updateCurrentBranch();
}

void GitPlugin::updateBranches(const QString &repository)
{
    dd->updateBranches(repository);
}

void GitPlugin::gerritPush(const QString &topLevel)
{
    dd->m_gerritPlugin->push(topLevel);
}

bool GitPlugin::isCommitEditorOpen()
{
    return dd->isCommitEditorOpen();
}

#ifdef WITH_TESTS

void GitPlugin::testStatusParsing_data()
{
    QTest::addColumn<FileStates>("first");
    QTest::addColumn<FileStates>("second");

    QTest::newRow(" M") << FileStates(ModifiedFile) << FileStates(UnknownFileState);
    QTest::newRow(" D") << FileStates(DeletedFile) << FileStates(UnknownFileState);
    QTest::newRow(" T") << FileStates(TypeChangedFile) << FileStates(UnknownFileState);
    QTest::newRow("T ") << (TypeChangedFile | StagedFile) << FileStates(UnknownFileState);
    QTest::newRow("TM") << (TypeChangedFile | StagedFile) << FileStates(ModifiedFile);
    QTest::newRow("MT") << (ModifiedFile | StagedFile) << FileStates(TypeChangedFile);
    QTest::newRow("M ") << (ModifiedFile | StagedFile) << FileStates(UnknownFileState);
    QTest::newRow("MM") << (ModifiedFile | StagedFile) << FileStates(ModifiedFile);
    QTest::newRow("MD") << (ModifiedFile | StagedFile) << FileStates(DeletedFile);
    QTest::newRow("A ") << (AddedFile | StagedFile) << FileStates(UnknownFileState);
    QTest::newRow("AM") << (AddedFile | StagedFile) << FileStates(ModifiedFile);
    QTest::newRow("AD") << (AddedFile | StagedFile) << FileStates(DeletedFile);
    QTest::newRow("D ") << (DeletedFile | StagedFile) << FileStates(UnknownFileState);
    QTest::newRow("DM") << (DeletedFile | StagedFile) << FileStates(ModifiedFile);
    QTest::newRow("R ") << (RenamedFile | StagedFile) << FileStates(UnknownFileState);
    QTest::newRow("RM") << (RenamedFile | StagedFile) << FileStates(ModifiedFile);
    QTest::newRow("RD") << (RenamedFile | StagedFile) << FileStates(DeletedFile);
    QTest::newRow("C ") << (CopiedFile | StagedFile) << FileStates(UnknownFileState);
    QTest::newRow("CM") << (CopiedFile | StagedFile) << FileStates(ModifiedFile);
    QTest::newRow("CD") << (CopiedFile | StagedFile) << FileStates(DeletedFile);
    QTest::newRow("??") << FileStates(UntrackedFile) << FileStates(UnknownFileState);

    // Merges
    QTest::newRow("DD") << (DeletedFile | UnmergedFile | UnmergedUs | UnmergedThem) << FileStates(UnknownFileState);
    QTest::newRow("AA") << (AddedFile | UnmergedFile | UnmergedUs | UnmergedThem) << FileStates(UnknownFileState);
    QTest::newRow("UU") << (ModifiedFile | UnmergedFile | UnmergedUs | UnmergedThem) << FileStates(UnknownFileState);
    QTest::newRow("AU") << (AddedFile | UnmergedFile | UnmergedUs) << FileStates(UnknownFileState);
    QTest::newRow("UD") << (DeletedFile | UnmergedFile | UnmergedThem) << FileStates(UnknownFileState);
    QTest::newRow("UA") << (AddedFile | UnmergedFile | UnmergedThem) << FileStates(UnknownFileState);
    QTest::newRow("DU") << (DeletedFile | UnmergedFile | UnmergedUs) << FileStates(UnknownFileState);
}

void GitPlugin::testStatusParsing()
{
    CommitData data;
    QFETCH(FileStates, first);
    QFETCH(FileStates, second);
    QString output = "## master...origin/master [ahead 1]\n";
    output += QString::fromLatin1(QTest::currentDataTag()) + " main.cpp\n";
    data.parseFilesFromStatus(output);
    QCOMPARE(data.files.at(0).first, first);
    if (second == UnknownFileState)
        QCOMPARE(data.files.size(), 1);
    else
        QCOMPARE(data.files.at(1).first, second);
}

void GitPlugin::testDiffFileResolving_data()
{
    QTest::addColumn<QByteArray>("header");
    QTest::addColumn<QByteArray>("fileName");

    QTest::newRow("New") << QByteArray(
            "diff --git a/src/plugins/git/giteditor.cpp b/src/plugins/git/giteditor.cpp\n"
            "new file mode 100644\n"
            "index 0000000..40997ff\n"
            "--- /dev/null\n"
            "+++ b/src/plugins/git/giteditor.cpp\n"
            "@@ -0,0 +1,281 @@\n\n")
        << QByteArray("src/plugins/git/giteditor.cpp");
    QTest::newRow("Deleted") << QByteArray(
            "diff --git a/src/plugins/git/giteditor.cpp b/src/plugins/git/giteditor.cpp\n"
            "deleted file mode 100644\n"
            "index 40997ff..0000000\n"
            "--- a/src/plugins/git/giteditor.cpp\n"
            "+++ /dev/null\n"
            "@@ -1,281 +0,0 @@\n\n")
        << QByteArray("src/plugins/git/giteditor.cpp");
    QTest::newRow("Normal") << QByteArray(
            "diff --git a/src/plugins/git/giteditor.cpp b/src/plugins/git/giteditor.cpp\n"
            "index 69e0b52..8fc974d 100644\n"
            "--- a/src/plugins/git/giteditor.cpp\n"
            "+++ b/src/plugins/git/giteditor.cpp\n"
            "@@ -49,6 +49,8 @@\n\n")
        << QByteArray("src/plugins/git/giteditor.cpp");
}

void GitPlugin::testDiffFileResolving()
{
    VcsBaseEditorWidget::testDiffFileResolving(dd->commitTextEditorFactory);
}

void GitPlugin::testLogResolving()
{
    QByteArray data(
                "commit 50a6b54c03219ad74b9f3f839e0321be18daeaf6 (HEAD, origin/master)\n"
                "Merge: 3587b51 bc93ceb\n"
                "Author: Junio C Hamano <gitster@pobox.com>\n"
                "Date:   Fri Jan 25 12:53:31 2013 -0800\n"
                "\n"
                "   Merge branch 'for-junio' of git://bogomips.org/git-svn\n"
                "    \n"
                "    * 'for-junio' of git://bogomips.org/git-svn:\n"
                "      git-svn: Simplify calculation of GIT_DIR\n"
                "      git-svn: cleanup sprintf usage for uppercasing hex\n"
                "\n"
                "commit 3587b513bafd7a83d8c816ac1deed72b5e3a27e9\n"
                "Author: Junio C Hamano <gitster@pobox.com>\n"
                "Date:   Fri Jan 25 12:52:55 2013 -0800\n"
                "\n"
                "    Update draft release notes to 1.8.2\n"
                "    \n"
                "    Signed-off-by: Junio C Hamano <gitster@pobox.com>\n"
                );

    VcsBaseEditorWidget::testLogResolving(dd->logEditorFactory, data,
                            "50a6b54c - Merge branch 'for-junio' of git://bogomips.org/git-svn",
                            "3587b513 - Update draft release notes to 1.8.2");
}

class RemoteTest {
public:
    RemoteTest() = default;
    explicit RemoteTest(const QString &url): m_url(url) {}

    inline RemoteTest &protocol(const QString &p) { m_protocol = p; return *this; }
    inline RemoteTest &userName(const QString &u) { m_userName = u; return *this; }
    inline RemoteTest &host(const QString &h) { m_host = h; return *this; }
    inline RemoteTest &port(quint16 p) { m_port = p; return *this; }
    inline RemoteTest &path(const QString &p) { m_path = p; return *this; }
    inline RemoteTest &isLocal(bool l) { m_isLocal = l; return *this; }
    inline RemoteTest &isValid(bool v) { m_isValid = v; return *this; }

    const QString m_url;
    QString m_protocol;
    QString m_userName;
    QString m_host;
    QString m_path;
    quint16 m_port = 0;
    bool    m_isLocal = false;
    bool    m_isValid = true;
};

} // namespace Internal
} // namespace Git

Q_DECLARE_METATYPE(Git::Internal::RemoteTest)

namespace Git {
namespace Internal {

void GitPlugin::testGitRemote_data()
{
    QTest::addColumn<RemoteTest>("rt");

    QTest::newRow("http-no-user")
            << RemoteTest("http://code.qt.io/qt-creator/qt-creator.git")
               .protocol("http")
               .host("code.qt.io")
               .path("/qt-creator/qt-creator.git");
    QTest::newRow("https-with-port")
            << RemoteTest("https://code.qt.io:80/qt-creator/qt-creator.git")
               .protocol("https")
               .host("code.qt.io")
               .port(80)
               .path("/qt-creator/qt-creator.git");
    QTest::newRow("invalid-port")
            << RemoteTest("https://code.qt.io:99999/qt-creator/qt-creator.git")
               .protocol("https")
               .host("code.qt.io")
               .path("/qt-creator/qt-creator.git")
               .isValid(false);
    QTest::newRow("ssh-user-foo")
            << RemoteTest("ssh://foo@codereview.qt-project.org:29418/qt-creator/qt-creator.git")
               .protocol("ssh")
               .userName("foo")
               .host("codereview.qt-project.org")
               .port(29418)
               .path("/qt-creator/qt-creator.git");
    QTest::newRow("ssh-github")
            << RemoteTest("git@github.com:qt-creator/qt-creator.git")
               .userName("git")
               .host("github.com")
               .path("qt-creator/qt-creator.git");
    QTest::newRow("local-file-protocol")
            << RemoteTest("file:///tmp/myrepo.git")
               .protocol("file")
               .path("/tmp/myrepo.git")
               .isLocal(true);
    QTest::newRow("local-absolute-path-unix")
            << RemoteTest("/tmp/myrepo.git")
               .protocol("file")
               .path("/tmp/myrepo.git")
               .isLocal(true);
    if (Utils::HostOsInfo::isWindowsHost()) {
        QTest::newRow("local-absolute-path-unix")
                << RemoteTest("c:/git/myrepo.git")
                   .protocol("file")
                   .path("c:/git/myrepo.git")
                   .isLocal(true);
    }
    QTest::newRow("local-relative-path-children")
            << RemoteTest("./git/myrepo.git")
               .protocol("file")
               .path("./git/myrepo.git")
               .isLocal(true);
    QTest::newRow("local-relative-path-parent")
            << RemoteTest("../myrepo.git")
               .protocol("file")
               .path("../myrepo.git")
               .isLocal(true);
}

void GitPlugin::testGitRemote()
{
    QFETCH(RemoteTest, rt);

    const GitRemote remote = GitRemote(rt.m_url);

    if (!rt.m_isLocal) {
        // local repositories must exist to be valid, so skip the test
        QCOMPARE(remote.isValid, rt.m_isValid);
    }

    QCOMPARE(remote.protocol, rt.m_protocol);
    QCOMPARE(remote.userName, rt.m_userName);
    QCOMPARE(remote.host,     rt.m_host);
    QCOMPARE(remote.port,     rt.m_port);
    QCOMPARE(remote.path,     rt.m_path);
}

#endif

} // namespace Internal
} // namespace Git

#include "gitplugin.moc"
