// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gitplugin.h"

#include "branchview.h"
#include "changeselectiondialog.h"
#include "commitdata.h"
#include "gitclient.h"
#include "gitconstants.h"
#include "giteditor.h"
#include "gitgrep.h"
#include "gitsettings.h"
#include "gitsubmiteditor.h"
#include "gittr.h"
#include "gitutils.h"
#include "logchangedialog.h"
#include "remotedialog.h"
#include "stashdialog.h"

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

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditortr.h>
#include <texteditor/textmark.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/commandline.h>
#include <utils/infobar.h>
#include <utils/parameteraction.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>

#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/cleandialog.h>
#include <vcsbase/submitfilemodel.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseplugin.h>
#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsoutputwindow.h>

#include <nanotrace/nanotrace.h>

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QTimer>
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

namespace Git::Internal {

using GitClientMemberFunc = void (GitClient::*)(const FilePath &) const;

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

class CommitInfo {
public:
    QString sha1;
    QString shortAuthor;
    QString author;
    QString authorMail;
    QDateTime authorTime;
    QString summary;
    FilePath filePath;
};

class BlameMark : public TextEditor::TextMark
{
    const CommitInfo m_info;
public:
    BlameMark(const FilePath &fileName, int lineNumber, const CommitInfo &info)
        : TextEditor::TextMark(fileName,
                               lineNumber,
                               {Tr::tr("Git Blame"), Constants::TEXT_MARK_CATEGORY_BLAME})
        , m_info(info)
    {
        const QString text = info.shortAuthor + " " + info.authorTime.toString("yyyy-MM-dd");

        setPriority(TextEditor::TextMark::LowPriority);
        setToolTip(toolTipText(info));
        setLineAnnotation(text);
        setSettingsPage(VcsBase::Constants::VCS_ID_GIT);
        setActionsProvider([info] {
            QAction *copyToClipboardAction = new QAction;
            copyToClipboardAction->setIcon(QIcon::fromTheme("edit-copy", Utils::Icons::COPY.icon()));
            copyToClipboardAction->setToolTip(TextEditor::Tr::tr("Copy SHA1 to Clipboard"));
            QObject::connect(copyToClipboardAction, &QAction::triggered, [info] {
                Utils::setClipboardAndSelection(info.sha1);
            });
            return QList<QAction *>{copyToClipboardAction};
        });
    }

    bool addToolTipContent(QLayout *target) const final
    {
        auto textLabel = new QLabel;
        textLabel->setText(toolTip());
        target->addWidget(textLabel);
        QObject::connect(textLabel, &QLabel::linkActivated, textLabel, [this] {
            gitClient().show(m_info.filePath, m_info.sha1);
        });

        return true;
    }

    QString toolTipText(const CommitInfo &info) const
    {
        QString result = QString(
                "<table>"
                "  <tr><td>commit</td><td><a href>%1</a></td></tr>"
                "  <tr><td>Author:</td><td>%2 &lt;%3&gt;</td></tr>"
                "  <tr><td>Date:</td><td>%4</td></tr>"
                "  <tr></tr>"
                "  <tr><td colspan='2' align='left'>%5</td></tr>"
                "</table>")
                .arg(info.sha1, info.author, info.authorMail,
                     info.authorTime.toString("yyyy-MM-dd hh:mm:ss"), info.summary);

        if (settings().instantBlameIgnoreSpaceChanges()
            || settings().instantBlameIgnoreLineMoves()) {
            result.append(
                "<p>"
                //: %1 and %2 are the "ignore whitespace changes" and "ignore line moves" options
                + Tr::tr("<b>Note:</b> \"%1\" or \"%2\""
                         " is enabled in the instant blame settings.")
                      .arg(GitSettings::trIgnoreWhitespaceChanges(),
                           GitSettings::trIgnoreLineMoves())
                + "</p>");
        }
        return result;
    }
};

// GitPlugin

class GitPluginPrivate final : public VcsBasePluginPrivate
{
    Q_OBJECT

public:
    GitPluginPrivate();
    ~GitPluginPrivate() final;

    // IVersionControl
    QString displayName() const final;
    Id id() const final;

    bool isVcsFileOrDirectory(const FilePath &filePath) const final;

    bool managesDirectory(const FilePath &directory, FilePath *topLevel) const final;
    bool managesFile(const FilePath &workingDirectory, const QString &fileName) const final;
    FilePaths unmanagedFiles(const FilePaths &filePaths) const final;

    bool isConfigured() const final;
    bool supportsOperation(Operation operation) const final;
    bool vcsOpen(const FilePath &filePath) final;
    bool vcsAdd(const FilePath &filePath) final;
    bool vcsDelete(const FilePath &filePath) final;
    bool vcsMove(const FilePath &from, const FilePath &to) final;
    bool vcsCreateRepository(const FilePath &directory) final;

    void vcsAnnotate(const FilePath &filePath, int line) final;
    void vcsDescribe(const FilePath &source, const QString &id) final { gitClient().show(source, id); }
    QString vcsTopic(const FilePath &directory) final;

    VcsCommand *createInitialCheckoutCommand(const QString &url,
                                             const FilePath &baseDirectory,
                                             const QString &localName,
                                             const QStringList &extraArgs) final;

    void fillLinkContextMenu(QMenu *menu,
                             const FilePath &workingDirectory,
                             const QString &reference) final
    {
        menu->addAction(Tr::tr("&Copy \"%1\"").arg(reference),
                        [reference] { setClipboardAndSelection(reference); });
        QAction *action = menu->addAction(Tr::tr("&Describe Change %1").arg(reference),
                                          [=] { vcsDescribe(workingDirectory, reference); });
        menu->setDefaultAction(action);
        GitClient::addChangeActions(menu, workingDirectory, reference);
    }

    bool handleLink(const FilePath &workingDirectory, const QString &reference) final
    {
        if (reference.contains(".."))
            gitClient().log(workingDirectory, {}, false, {reference});
        else
            gitClient().show(workingDirectory, reference);
        return true;
    }

    RepoUrl getRepoUrl(const QString &location) const override;

    FilePaths additionalToolsPath() const final;

    bool isCommitEditorOpen() const;
    void startCommit(CommitType commitType = SimpleCommit);
    void updateBranches(const FilePath &repository);
    void updateCurrentBranch();

    void manageRemotes();
    void initRepository();
    void startRebaseFromCommit(const FilePath &workingDirectory, QString commit);

    void updateActions(VcsBasePluginPrivate::ActionState) override;
    bool activateCommit() override;
    void discardCommit() override { cleanCommitMessageFile(); }

    void diffCurrentFile();
    void diffCurrentProject();
    void logFile();
    void blameFile();
    void logProject();
    void logRepository();
    void reflogRepository();
    void undoFileChanges(bool revertStaging);
    void resetRepository();
    void recoverDeletedFiles();
    void startRebase();
    void startChangeRelatedAction(const Id &id);
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
    void updateContinueAndAbortCommands();
    void delayedPushToGerrit();

    Command *createCommand(QAction *action, ActionContainer *ac, Id id,
                           const Context &context, bool addToLocator,
                           const std::function<void()> &callback, const QKeySequence &keys);

    ParameterAction *createParameterAction(ActionContainer *ac,
                                           const QString &defaultText, const QString &parameterText,
                                           Id id, const Context &context, bool addToLocator,
                                           const std::function<void()> &callback,
                                           const QKeySequence &keys = QKeySequence());

    QAction *createFileAction(ActionContainer *ac,
                              const QString &defaultText, const QString &parameterText,
                              Id id, const Context &context, bool addToLocator,
                              const std::function<void()> &callback,
                              const QKeySequence &keys = QKeySequence());

    QAction *createProjectAction(ActionContainer *ac,
                                 const QString &defaultText, const QString &parameterText,
                                 Id id, const Context &context, bool addToLocator,
                                 void (GitPluginPrivate::*func)(),
                                 const QKeySequence &keys = QKeySequence());

    QAction *createRepositoryAction(ActionContainer *ac, const QString &text, Id id,
                                    const Context &context, bool addToLocator,
                                    const std::function<void()> &callback,
                                    const QKeySequence &keys = QKeySequence());
    QAction *createRepositoryAction(ActionContainer *ac, const QString &text, Id id,
                                    const Context &context, bool addToLocator,
                                    GitClientMemberFunc, const QKeySequence &keys = QKeySequence());

    QAction *createChangeRelatedRepositoryAction(const QString &text, Id id,
                                                 const Context &context);

    void updateRepositoryBrowserAction();
    IEditor *openSubmitEditor(const QString &fileName, const CommitData &cd);
    void cleanCommitMessageFile();
    void cleanRepository(const FilePath &directory);
    void applyPatch(const FilePath &workingDirectory, QString file = {});
    void updateVersionWarning();

    void setupInstantBlame();
    void instantBlameOnce();
    void forceInstantBlame();
    void instantBlame();
    void stopInstantBlame();
    bool refreshWorkingDirectory(const FilePath &workingDirectory);

    void onApplySettings();

    CommandLocator *m_commandLocator = nullptr;

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

    QList<ParameterAction *> m_fileActions;
    QList<ParameterAction *> m_projectActions;
    QList<QAction *> m_repositoryActions;
    ParameterAction *m_applyCurrentFilePatchAction = nullptr;

    Gerrit::Internal::GerritPlugin m_gerritPlugin;

    QPointer<StashDialog> m_stashDialog;
    BranchViewFactory m_branchViewFactory;
    QPointer<RemoteDialog> m_remoteDialog;
    FilePath m_submitRepository;
    QString m_commitMessageFileName;

    FilePath m_workingDirectory;
    QTextCodec *m_codec = nullptr;
    Author m_author;
    int m_lastVisitedEditorLine = -1;
    QTimer *m_cursorPositionChangedTimer = nullptr;
    std::unique_ptr<BlameMark> m_blameMark;
    QMetaObject::Connection m_blameCursorPosConn;
    QMetaObject::Connection m_documentChangedConn;

    GitGrep gitGrep;

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

class GitTopicCache : public IVersionControl::TopicCache
{
public:
    GitTopicCache() {}

protected:
    FilePath trackFile(const FilePath &repository) override
    {
        const FilePath gitDir = gitClient().findGitDirForRepository(repository);
        return gitDir.isEmpty() ? FilePath() : gitDir / "HEAD";
    }

    QString refreshTopic(const FilePath &repository) override
    {
        emit dd->repositoryChanged(repository);
        return gitClient().synchronousTopic(repository);
    }
};

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
    emit configurationChanged();
    updateRepositoryBrowserAction();
    bool gitFoundOk;
    QString errorMessage;
    settings().gitExecutable(&gitFoundOk, &errorMessage);
    if (!gitFoundOk) {
        QTimer::singleShot(0, this, [errorMessage] {
            AsynchronousMessageBox::warning(Tr::tr("Git Settings"), errorMessage);
        });
    }
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

IVersionControl *GitPlugin::versionControl()
{
    return dd;
}

const VcsBasePluginState &GitPlugin::currentState()
{
    return dd->currentState();
}

QString GitPlugin::msgRepositoryLabel(const FilePath &repository)
{
    return repository.isEmpty() ?
            Tr::tr("<No repository>")  :
            Tr::tr("Repository: %1").arg(repository.toUserOutput());
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
                                  std::bind(&GitPluginPrivate::startChangeRelatedAction, this, id));
}

// Action to act on the repository forwarded to a git client member function
// taking the directory.
QAction *GitPluginPrivate::createRepositoryAction(ActionContainer *ac, const QString &text, Id id,
                                           const Context &context, bool addToLocator,
                                           GitClientMemberFunc func, const QKeySequence &keys)
{
    auto cb = [this, func] {
        QTC_ASSERT(currentState().hasTopLevel(), return);
        (gitClient().*func)(currentState().topLevel());
    };
    // Set the member func as data and connect to GitClient method
    return createRepositoryAction(ac, text, id, context, addToLocator, cb, keys);
}

bool GitPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(errorMessage)

    dd = new GitPluginPrivate;

    auto cmdContext = new QObject(this);
    connect(ICore::instance(), &ICore::coreOpened, cmdContext, [this, cmdContext, arguments] {
        NANOTRACE_SCOPE("Git", "GitPlugin::initialize::coreOpened");
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
    : VcsBasePluginPrivate(Context(Constants::GIT_CONTEXT))
{
    dd = this;

    setTopicCache(new GitTopicCache);

    m_fileActions.reserve(10);
    m_projectActions.reserve(10);
    m_repositoryActions.reserve(50);
    m_codec = gitClient().defaultCommitEncoding();

    Context context(Constants::GIT_CONTEXT);

    const QString prefix = "git";
    m_commandLocator = new CommandLocator("Git", prefix, prefix, this);
    m_commandLocator->setDescription(Tr::tr("Triggers a Git version control operation."));

    //register actions
    ActionContainer *toolsContainer = ActionManager::actionContainer(Core::Constants::M_TOOLS);

    ActionContainer *gitContainer = ActionManager::createMenu("Git");
    gitContainer->menu()->setTitle(Tr::tr("&Git"));
    toolsContainer->addMenu(gitContainer);
    m_menuAction = gitContainer->menu()->menuAction();


    /*  "Current File" menu */
    ActionContainer *currentFileMenu = ActionManager::createMenu("Git.CurrentFileMenu");
    currentFileMenu->menu()->setTitle(Tr::tr("Current &File"));
    gitContainer->addMenu(currentFileMenu);

    createFileAction(currentFileMenu, Tr::tr("Diff Current File", "Avoid translating \"Diff\""),
                     Tr::tr("Diff of \"%1\"", "Avoid translating \"Diff\""),
                     "Git.Diff", context, true, std::bind(&GitPluginPrivate::diffCurrentFile, this),
                      QKeySequence(useMacShortcuts ? Tr::tr("Meta+G,Meta+D") : Tr::tr("Alt+G,Alt+D")));

    createFileAction(currentFileMenu, Tr::tr("Log Current File", "Avoid translating \"Log\""),
                     Tr::tr("Log of \"%1\"", "Avoid translating \"Log\""),
                     "Git.Log", context, true, std::bind(&GitPluginPrivate::logFile, this),
                     QKeySequence(useMacShortcuts ? Tr::tr("Meta+G,Meta+L") : Tr::tr("Alt+G,Alt+L")));

    createFileAction(currentFileMenu, Tr::tr("Blame Current File", "Avoid translating \"Blame\""),
                     Tr::tr("Blame for \"%1\"", "Avoid translating \"Blame\""),
                     "Git.Blame", context, true, std::bind(&GitPluginPrivate::blameFile, this),
                     QKeySequence(useMacShortcuts ? Tr::tr("Meta+G,Meta+B") : Tr::tr("Alt+G,Alt+B")));

    createFileAction(currentFileMenu, Tr::tr("Instant Blame Current Line", "Avoid translating \"Blame\""),
                     Tr::tr("Instant Blame for \"%1\"", "Avoid translating \"Blame\""),
                     "Git.InstantBlame", context, true, std::bind(&GitPluginPrivate::instantBlameOnce, this),
                     QKeySequence(useMacShortcuts ? Tr::tr("Meta+G,Meta+I") : Tr::tr("Alt+G,Alt+I")));

    currentFileMenu->addSeparator(context);

    createFileAction(currentFileMenu, Tr::tr("Stage File for Commit"), Tr::tr("Stage \"%1\" for Commit"),
                     "Git.Stage", context, true, std::bind(&GitPluginPrivate::stageFile, this),
                     QKeySequence(useMacShortcuts ? Tr::tr("Meta+G,Meta+A") : Tr::tr("Alt+G,Alt+A")));

    createFileAction(currentFileMenu, Tr::tr("Unstage File from Commit"), Tr::tr("Unstage \"%1\" from Commit"),
                     "Git.Unstage", context, true, std::bind(&GitPluginPrivate::unstageFile, this));

    createFileAction(currentFileMenu, Tr::tr("Undo Unstaged Changes"), Tr::tr("Undo Unstaged Changes for \"%1\""),
                     "Git.UndoUnstaged", context,
                     true, std::bind(&GitPluginPrivate::undoFileChanges, this, false));

    createFileAction(currentFileMenu, Tr::tr("Undo Uncommitted Changes"), Tr::tr("Undo Uncommitted Changes for \"%1\""),
                     "Git.Undo", context,
                     true, std::bind(&GitPluginPrivate::undoFileChanges, this, true),
                     QKeySequence(useMacShortcuts ? Tr::tr("Meta+G,Meta+U") : Tr::tr("Alt+G,Alt+U")));


    /*  "Current Project" menu */
    ActionContainer *currentProjectMenu = ActionManager::createMenu("Git.CurrentProjectMenu");
    currentProjectMenu->menu()->setTitle(Tr::tr("Current &Project"));
    gitContainer->addMenu(currentProjectMenu);

    createProjectAction(currentProjectMenu, Tr::tr("Diff Current Project", "Avoid translating \"Diff\""),
                        Tr::tr("Diff Project \"%1\"", "Avoid translating \"Diff\""),
                        "Git.DiffProject", context, true, &GitPluginPrivate::diffCurrentProject);

    createProjectAction(currentProjectMenu, Tr::tr("Log Project", "Avoid translating \"Log\""),
                        Tr::tr("Log Project \"%1\"", "Avoid translating \"Log\""),
                        "Git.LogProject", context, true, &GitPluginPrivate::logProject);

    createProjectAction(currentProjectMenu, Tr::tr("Clean Project...", "Avoid translating \"Clean\""),
                        Tr::tr("Clean Project \"%1\"...", "Avoid translating \"Clean\""),
                        "Git.CleanProject", context, true, &GitPluginPrivate::cleanProject);


    /*  "Local Repository" menu */
    ActionContainer *localRepositoryMenu = ActionManager::createMenu("Git.LocalRepositoryMenu");
    localRepositoryMenu->menu()->setTitle(Tr::tr("&Local Repository"));
    gitContainer->addMenu(localRepositoryMenu);

    createRepositoryAction(localRepositoryMenu, "Diff", "Git.DiffRepository",
                           context, true, &GitClient::diffRepository,
                           QKeySequence(useMacShortcuts ? Tr::tr("Meta+G,Meta+Shift+D") : Tr::tr("Alt+G,Alt+Shift+D")));

    createRepositoryAction(localRepositoryMenu, "Log", "Git.LogRepository",
                           context, true, std::bind(&GitPluginPrivate::logRepository, this),
                           QKeySequence(useMacShortcuts ? Tr::tr("Meta+G,Meta+K") : Tr::tr("Alt+G,Alt+K")));

    createRepositoryAction(localRepositoryMenu, "Reflog", "Git.ReflogRepository",
                           context, true, std::bind(&GitPluginPrivate::reflogRepository, this));

    createRepositoryAction(localRepositoryMenu, "Clean...", "Git.CleanRepository",
                           context, true, [this] { cleanRepository(); });

    createRepositoryAction(localRepositoryMenu, "Status", "Git.StatusRepository",
                           context, true, &GitClient::status);

    // --------------
    localRepositoryMenu->addSeparator(context);

    createRepositoryAction(localRepositoryMenu, "Commit...", "Git.Commit",
                           context, true, std::bind(&GitPluginPrivate::startCommit, this, SimpleCommit),
                           QKeySequence(useMacShortcuts ? Tr::tr("Meta+G,Meta+C") : Tr::tr("Alt+G,Alt+C")));

    createRepositoryAction(localRepositoryMenu,
                           Tr::tr("Amend Last Commit...", "Avoid translating \"Commit\""),
                           "Git.AmendCommit",
                           context, true, std::bind(&GitPluginPrivate::startCommit, this, AmendCommit));

    m_fixupCommitAction
            = createRepositoryAction(localRepositoryMenu,
                                     Tr::tr("Fixup Previous Commit...", "Avoid translating \"Commit\""),
                                     "Git.FixupCommit", context, true,
                                     std::bind(&GitPluginPrivate::startCommit, this, FixupCommit));

    // --------------
    localRepositoryMenu->addSeparator(context);

    createRepositoryAction(localRepositoryMenu, "Reset...", "Git.Reset",
                           context, true, std::bind(&GitPluginPrivate::resetRepository, this));

    createRepositoryAction(localRepositoryMenu, Tr::tr("Recover Deleted Files"), "Git.RecoverDeleted",
                           context, true, std::bind(&GitPluginPrivate::recoverDeletedFiles, this));

    m_interactiveRebaseAction
            = createRepositoryAction(localRepositoryMenu,
                                     Tr::tr("Interactive Rebase...", "Avoid translating \"Rebase\""),
                                     "Git.InteractiveRebase",
                                     context, true, std::bind(&GitPluginPrivate::startRebase, this));

    m_submoduleUpdateAction
            = createRepositoryAction(localRepositoryMenu,
                                     Tr::tr("Update Submodules"), "Git.SubmoduleUpdate",
                                     context, true, std::bind(&GitPluginPrivate::updateSubmodules, this));

    auto createAction = [=](const QString &text, Id id,
                            const std::function<void(const FilePath &)> &callback) {
        auto actionHandler = [this, callback] {
            if (!DocumentManager::saveAllModifiedDocuments())
                return;
            const VcsBasePluginState state = currentState();
            QTC_ASSERT(state.hasTopLevel(), return);

            callback(state.topLevel());

            updateContinueAndAbortCommands();
        };
        return createRepositoryAction(localRepositoryMenu, text, id, context, true, actionHandler);
    };

    m_abortMergeAction = createAction(Tr::tr("Abort Merge", "Avoid translating \"Merge\""), "Git.MergeAbort",
        std::bind(&GitClient::synchronousMerge, &gitClient(), _1, QString("--abort"), true));

    m_abortRebaseAction = createAction(Tr::tr("Abort Rebase", "Avoid translating \"Rebase\""), "Git.RebaseAbort",
        std::bind(&GitClient::rebase, &gitClient(), _1, QString("--abort")));

    m_continueRebaseAction = createAction(Tr::tr("Continue Rebase"), "Git.RebaseContinue",
        std::bind(&GitClient::rebase, &gitClient(), _1, QString("--continue")));

    m_skipRebaseAction = createAction(Tr::tr("Skip Rebase"), "Git.RebaseSkip",
        std::bind(&GitClient::rebase, &gitClient(), _1, QString("--skip")));

    m_abortCherryPickAction = createAction(Tr::tr("Abort Cherry Pick", "Avoid translating \"Cherry Pick\""), "Git.CherryPickAbort",
        std::bind(&GitClient::synchronousCherryPick, &gitClient(), _1, QString("--abort")));

    m_continueCherryPickAction = createAction(Tr::tr("Continue Cherry Pick"), "Git.CherryPickContinue",
        std::bind(&GitClient::cherryPick, &gitClient(), _1, QString("--continue")));

    m_abortRevertAction = createAction(Tr::tr("Abort Revert", "Avoid translating \"Revert\""), "Git.RevertAbort",
        std::bind(&GitClient::synchronousRevert, &gitClient(), _1, QString("--abort")));

    m_continueRevertAction = createAction(Tr::tr("Continue Revert"), "Git.RevertContinue",
        std::bind(&GitClient::revert, &gitClient(), _1, QString("--continue")));

    // --------------
    localRepositoryMenu->addSeparator(context);

    createRepositoryAction(localRepositoryMenu, Tr::tr("Branches..."), "Git.BranchList",
                           context, true, std::bind(&GitPluginPrivate::branchList, this));

    // --------------
    localRepositoryMenu->addSeparator(context);

    // "Patch" menu
    ActionContainer *patchMenu = ActionManager::createMenu("Git.PatchMenu");
    patchMenu->menu()->setTitle(Tr::tr("&Patch"));
    localRepositoryMenu->addMenu(patchMenu);

    // Apply current file as patch is handled specially.
    m_applyCurrentFilePatchAction
            = createParameterAction(patchMenu,
                                    Tr::tr("Apply from Editor"), Tr::tr("Apply \"%1\""),
                                    "Git.ApplyCurrentFilePatch",
                                    context, true, std::bind(&GitPluginPrivate::applyCurrentFilePatch, this));
    createRepositoryAction(patchMenu, Tr::tr("Apply from File..."), "Git.ApplyPatch",
                           context, true, std::bind(&GitPluginPrivate::promptApplyPatch, this));

    // "Stash" menu
    ActionContainer *stashMenu = ActionManager::createMenu("Git.StashMenu");
    stashMenu->menu()->setTitle(Tr::tr("&Stash"));
    localRepositoryMenu->addMenu(stashMenu);

    createRepositoryAction(stashMenu, "Stashes...", "Git.StashList",
                           context, false, std::bind(&GitPluginPrivate::stashList, this));

    stashMenu->addSeparator(context);

    QAction *action = createRepositoryAction(stashMenu, "Stash", "Git.Stash",
                                             context, true, std::bind(&GitPluginPrivate::stash, this, false));
    action->setToolTip(Tr::tr("Saves the current state of your work and resets the repository."));

    action = createRepositoryAction(stashMenu, Tr::tr("Stash Unstaged Files", "Avoid translating \"Stash\""),
                                    "Git.StashUnstaged",
                                    context, true, std::bind(&GitPluginPrivate::stashUnstaged, this));
    action->setToolTip(Tr::tr("Saves the current state of your unstaged files and resets the repository "
                          "to its staged state."));

    action = createRepositoryAction(stashMenu, Tr::tr("Take Snapshot..."), "Git.StashSnapshot",
                                    context, true, std::bind(&GitPluginPrivate::stashSnapshot, this));
    action->setToolTip(Tr::tr("Saves the current state of your work."));

    stashMenu->addSeparator(context);

    action = createRepositoryAction(stashMenu, Tr::tr("Stash Pop", "Avoid translating \"Stash\""),
                                    "Git.StashPop",
                                    context, true, std::bind(&GitPluginPrivate::stashPop, this));
    action->setToolTip(Tr::tr("Restores changes saved to the stash list using \"Stash\"."));


    /* \"Local Repository" menu */

    // --------------

    /*  "Remote Repository" menu */
    ActionContainer *remoteRepositoryMenu = ActionManager::createMenu("Git.RemoteRepositoryMenu");
    remoteRepositoryMenu->menu()->setTitle(Tr::tr("&Remote Repository"));
    gitContainer->addMenu(remoteRepositoryMenu);

    createRepositoryAction(remoteRepositoryMenu, "Fetch", "Git.Fetch",
                           context, true, std::bind(&GitPluginPrivate::fetch, this));

    createRepositoryAction(remoteRepositoryMenu, "Pull", "Git.Pull",
                           context, true, std::bind(&GitPluginPrivate::pull, this));

    createRepositoryAction(remoteRepositoryMenu, "Push", "Git.Push",
                           context, true, std::bind(&GitPluginPrivate::push, this));

    // --------------
    remoteRepositoryMenu->addSeparator(context);

    // "Subversion" menu
    ActionContainer *subversionMenu = ActionManager::createMenu("Git.Subversion");
    subversionMenu->menu()->setTitle(Tr::tr("&Subversion"));
    remoteRepositoryMenu->addMenu(subversionMenu);

    createRepositoryAction(subversionMenu, "Log", "Git.Subversion.Log",
                           context, false, &GitClient::subversionLog);

    createRepositoryAction(subversionMenu, "Fetch", "Git.Subversion.Fetch",
                           context, false, &GitClient::synchronousSubversionFetch);

    createRepositoryAction(subversionMenu, Tr::tr("DCommit"), "Git.Subversion.DCommit",
                           context, false, &GitClient::subversionDeltaCommit);

    // --------------
    remoteRepositoryMenu->addSeparator(context);

    createRepositoryAction(remoteRepositoryMenu, Tr::tr("Manage Remotes..."), "Git.RemoteList",
                           context, false, std::bind(&GitPluginPrivate::manageRemotes, this));

    /* \"Remote Repository" menu */

    // --------------

    /*  Actions only in locator */
    createChangeRelatedRepositoryAction("Show...", "Git.Show", context);
    createChangeRelatedRepositoryAction("Revert...", "Git.Revert", context);
    createChangeRelatedRepositoryAction("Cherry Pick...", "Git.CherryPick", context);
    createChangeRelatedRepositoryAction("Checkout...", "Git.Checkout", context);
    createChangeRelatedRepositoryAction(Tr::tr("Archive..."), "Git.Archive", context);

    createRepositoryAction(nullptr, "Rebase...", "Git.Rebase", context, true,
                           std::bind(&GitPluginPrivate::branchList, this));
    createRepositoryAction(nullptr, "Merge...", "Git.Merge", context, true,
                           std::bind(&GitPluginPrivate::branchList, this));
    /*  \Actions only in locator */

    // --------------

    /*  "Git Tools" menu */
    ActionContainer *gitToolsMenu = ActionManager::createMenu("Git.GitToolsMenu");
    gitToolsMenu->menu()->setTitle(Tr::tr("Git &Tools"));
    gitContainer->addMenu(gitToolsMenu);

    createRepositoryAction(gitToolsMenu, Tr::tr("Gitk"), "Git.LaunchGitK",
                           context, true, &GitClient::launchGitK);

    createFileAction(gitToolsMenu, Tr::tr("Gitk Current File"), Tr::tr("Gitk of \"%1\""),
                     "Git.GitkFile", context, true, std::bind(&GitPluginPrivate::gitkForCurrentFile, this));

    createFileAction(gitToolsMenu, Tr::tr("Gitk for folder of Current File"), Tr::tr("Gitk for folder of \"%1\""),
                     "Git.GitkFolder", context, true, std::bind(&GitPluginPrivate::gitkForCurrentFolder, this));

    // --------------
    gitToolsMenu->addSeparator(context);

    createRepositoryAction(gitToolsMenu, Tr::tr("Git Gui"), "Git.GitGui",
                           context, true, std::bind(&GitPluginPrivate::gitGui, this));

    // --------------
    gitToolsMenu->addSeparator(context);

    m_repositoryBrowserAction
            = createRepositoryAction(gitToolsMenu,
                                     Tr::tr("Repository Browser"), "Git.LaunchRepositoryBrowser",
                                     context, true, &GitClient::launchRepositoryBrowser);

    m_mergeToolAction
            = createRepositoryAction(gitToolsMenu, Tr::tr("Merge Tool"), "Git.MergeTool",
                                     context, true, std::bind(&GitPluginPrivate::startMergeTool, this));

    // --------------
    if (HostOsInfo::isWindowsHost()) {
        gitToolsMenu->addSeparator(context);

        createRepositoryAction(gitToolsMenu, Tr::tr("Git Bash"), "Git.GitBash",
                               context, true, std::bind(&GitPluginPrivate::gitBash, this));
    }

    /* \"Git Tools" menu */

    // --------------
    gitContainer->addSeparator(context);

    QAction *actionsOnCommitsAction = new QAction(Tr::tr("Actions on Commits..."), this);
    Command *actionsOnCommitsCommand = ActionManager::registerAction(
                actionsOnCommitsAction, "Git.ChangeActions");
    connect(actionsOnCommitsAction, &QAction::triggered, this,
            [this] { startChangeRelatedAction("Git.ChangeActions"); });
    gitContainer->addAction(actionsOnCommitsCommand);

    QAction *createRepositoryAction = new QAction(Tr::tr("Create Repository..."), this);
    Command *createRepositoryCommand = ActionManager::registerAction(
                createRepositoryAction, "Git.CreateRepository");
    connect(createRepositoryAction, &QAction::triggered, this, &GitPluginPrivate::createRepository);
    gitContainer->addAction(createRepositoryCommand);

    connect(VcsManager::instance(), &VcsManager::repositoryChanged,
            this, &GitPluginPrivate::updateContinueAndAbortCommands);
    connect(VcsManager::instance(), &VcsManager::repositoryChanged,
            this, &GitPluginPrivate::updateBranches, Qt::QueuedConnection);

    /* "Gerrit" */
    m_gerritPlugin.addToMenu(remoteRepositoryMenu);
    m_gerritPlugin.updateActions(currentState());
    m_gerritPlugin.addToLocator(m_commandLocator);

    connect(&settings(), &AspectContainer::applied, this, &GitPluginPrivate::onApplySettings);

    setupInstantBlame();
}

void GitPluginPrivate::diffCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    gitClient().diffFile(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void GitPluginPrivate::diffCurrentProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    const QString relativeProject = state.relativeCurrentProject();
    if (relativeProject.isEmpty())
        gitClient().diffRepository(state.currentProjectTopLevel());
    else
        gitClient().diffProject(state.currentProjectTopLevel(), relativeProject);
}

void GitPluginPrivate::logFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    gitClient().log(state.currentFileTopLevel(), state.relativeCurrentFile(), true);
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
    const FilePath fileName = state.currentFile().canonicalPath();
    FilePath topLevel;
    VcsManager::findVersionControlForDirectory(fileName.parentDir(), &topLevel);
    gitClient().annotate(topLevel, fileName.relativeChildPath(topLevel).toString(),
                         lineNumber, {}, extraOptions, firstLine);
}

void GitPluginPrivate::logProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    gitClient().log(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void GitPluginPrivate::logRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    gitClient().log(state.topLevel());
}

void GitPluginPrivate::reflogRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    gitClient().reflog(state.topLevel());
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
    gitClient().revertFiles({state.currentFile().toString()}, revertStaging);
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
        : IconItemDelegate(widget, Icons::UNDO)
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
    FilePath topLevel = state.topLevel();

    LogChangeDialog dialog(true, ICore::dialogParent());
    ResetItemDelegate delegate(dialog.widget());
    dialog.setWindowTitle(Tr::tr("Undo Changes to %1").arg(topLevel.toUserOutput()));
    if (dialog.runDialog(topLevel, {}, LogChangeWidget::IncludeRemotes))
        gitClient().reset(topLevel, dialog.resetFlag(), dialog.commit());
}

void GitPluginPrivate::recoverDeletedFiles()
{
    if (!DocumentManager::saveAllModifiedDocuments())
        return;
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    gitClient().recoverDeletedFiles(state.topLevel());
}

void GitPluginPrivate::startRebase()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    const FilePath topLevel = state.topLevel();

    startRebaseFromCommit(topLevel, {});
}

void GitPluginPrivate::startRebaseFromCommit(const FilePath &workingDirectory, QString commit)
{
    if (!DocumentManager::saveAllModifiedDocuments())
        return;
    if (workingDirectory.isEmpty() || !gitClient().canRebase(workingDirectory))
        return;

    if (commit.isEmpty()) {
        LogChangeDialog dialog(false, ICore::dialogParent());
        RebaseItemDelegate delegate(dialog.widget());
        dialog.setWindowTitle(Tr::tr("Interactive Rebase"));
        if (!dialog.runDialog(workingDirectory))
            return;
        commit = dialog.commit();
    }

    if (gitClient().beginStashScope(workingDirectory, "Rebase-i"))
        gitClient().interactiveRebase(workingDirectory, commit, false);
}

void GitPluginPrivate::startChangeRelatedAction(const Id &id)
{
    const VcsBasePluginState state = currentState();

    ChangeSelectionDialog dialog(state.hasTopLevel() ? state.topLevel() : PathChooser::homePath(),
                                 id, ICore::dialogParent());

    int result = dialog.exec();

    if (result == QDialog::Rejected)
        return;

    const FilePath workingDirectory = dialog.workingDirectory();
    const QString change = dialog.change();

    if (workingDirectory.isEmpty() || change.isEmpty())
        return;

    if (dialog.command() == Show) {
        const int colon = change.indexOf(':');
        if (colon > 0) {
            const FilePath path = workingDirectory.resolvePath(change.mid(colon + 1));
            gitClient().openShowEditor(workingDirectory, change.left(colon), path);
        } else {
            gitClient().show(workingDirectory, change);
        }
        return;
    }

    if (dialog.command() == Archive) {
        gitClient().archive(workingDirectory, change);
        return;
    }

    if (!DocumentManager::saveAllModifiedDocuments())
        return;

    switch (dialog.command()) {
    case CherryPick:
        gitClient().synchronousCherryPick(workingDirectory, change);
        break;
    case Revert:
        gitClient().synchronousRevert(workingDirectory, change);
        break;
    case Checkout:
        gitClient().checkout(workingDirectory, change);
        break;
    default:
        return;
    }
}

void GitPluginPrivate::stageFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    gitClient().addFile(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void GitPluginPrivate::unstageFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    gitClient().synchronousReset(state.currentFileTopLevel(), {state.relativeCurrentFile()});
}

void GitPluginPrivate::gitkForCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    gitClient().launchGitK(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void GitPluginPrivate::gitkForCurrentFolder()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);

    /*
     *  entire lower part of the code can be easily replaced with one line:
     *
     *  gitClient().launchGitK(dir.currentFileDirectory(), ".");
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
    QDir dir(state.currentFileDirectory().toString());
    if (QFileInfo(dir,".git").exists() || dir.cd(".git")) {
        gitClient().launchGitK(state.currentFileDirectory());
    } else {
        QString folderName = dir.absolutePath();
        dir.cdUp();
        folderName = folderName.remove(0, dir.absolutePath().length() + 1);
        gitClient().launchGitK(FilePath::fromString(dir.absolutePath()), folderName);
    }
}

void GitPluginPrivate::gitGui()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    gitClient().launchGitGui(state.topLevel());
}

void GitPluginPrivate::gitBash()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    gitClient().launchGitBash(state.topLevel());
}

void GitPluginPrivate::startCommit(CommitType commitType)
{
    if (!promptBeforeCommit())
        return;

    if (raiseSubmitEditor())
        return;
    if (isCommitEditorOpen()) {
        VcsOutputWindow::appendWarning(Tr::tr("Another submit is currently being executed."));
        return;
    }

    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    QString errorMessage, commitTemplate;
    CommitData data(commitType);
    if (!gitClient().getCommitData(state.topLevel(), &commitTemplate, data, &errorMessage)) {
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
    m_commitMessageFileName = saver.filePath().toString();
    openSubmitEditor(m_commitMessageFileName, data);
}

void GitPluginPrivate::updateVersionWarning()
{
    QPointer<IDocument> curDocument = EditorManager::currentDocument();
    if (!curDocument)
        return;
    Utils::onResultReady(gitClient().gitVersion(), this, [curDocument](unsigned version) {
        if (!curDocument || !version || version >= minimumRequiredVersion)
            return;
        InfoBar *infoBar = curDocument->infoBar();
        Id gitVersionWarning("GitVersionWarning");
        if (!infoBar->canInfoBeAdded(gitVersionWarning))
            return;
        infoBar->addInfo(
            InfoBarEntry(gitVersionWarning,
                         Tr::tr("Unsupported version of Git found. Git %1 or later required.")
                             .arg(versionString(minimumRequiredVersion)),
                         InfoBarEntry::GlobalSuppression::Enabled));
    });
}

void GitPluginPrivate::setupInstantBlame()
{
    m_cursorPositionChangedTimer = new QTimer(this);
    m_cursorPositionChangedTimer->setSingleShot(true);
    connect(m_cursorPositionChangedTimer, &QTimer::timeout, this, &GitPluginPrivate::instantBlame);

    auto setupBlameForEditor = [this](Core::IEditor *editor) {
        if (!editor) {
            stopInstantBlame();
            return;
        }

        if (!settings().instantBlame()) {
            m_lastVisitedEditorLine = -1;
            stopInstantBlame();
            return;
        }

        const TextEditorWidget *widget = TextEditorWidget::fromEditor(editor);
        if (!widget)
            return;

        if (qobject_cast<const VcsBaseEditorWidget *>(widget))
            return; // Skip in VCS editors like log or blame

        const Utils::FilePath workingDirectory = GitPlugin::currentState().currentFileTopLevel();
        if (!refreshWorkingDirectory(workingDirectory))
            return;

        m_blameCursorPosConn = connect(widget, &QPlainTextEdit::cursorPositionChanged, this,
                                [this] {
            if (!settings().instantBlame()) {
                disconnect(m_blameCursorPosConn);
                return;
            }
            m_cursorPositionChangedTimer->start(500);
        });
        IDocument *document = editor->document();
        m_documentChangedConn = connect(document, &IDocument::changed, this, [this, document] {
            if (!document->isModified())
                forceInstantBlame();
        });

        forceInstantBlame();
    };

    connect(&settings().instantBlame, &BaseAspect::changed, this, [this, setupBlameForEditor] {
        if (settings().instantBlame())
            setupBlameForEditor(EditorManager::currentEditor());
        else
            stopInstantBlame();
    });

    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, setupBlameForEditor);
}

// Porcelain format of git blame output
// 8b649d2d61416205977aba56ef93e1e1f155005e 5 5 1
// author John Doe
// author-mail <john.doe@gmail.com>
// author-time 1613752276
// author-tz +0100
// committer John Doe
// committer-mail <john.doe@gmail.com>
// committer-time 1613752312
// committer-tz +0100
// summary Add greeting to script
// boundary
// filename foo
//     echo Hello World!

CommitInfo parseBlameOutput(const QStringList &blame, const Utils::FilePath &filePath,
                            const Git::Internal::Author &author)
{
    CommitInfo result;
    if (blame.size() <= 12)
        return result;

    result.sha1 = blame.at(0).left(40);
    result.author = blame.at(1).mid(7);
    result.authorMail = blame.at(2).mid(13).chopped(1);
    if (result.author == author.name || result.authorMail == author.email)
        result.shortAuthor = Tr::tr("You");
    else
        result.shortAuthor = result.author;
    const uint timeStamp = blame.at(3).mid(12).toUInt();
    result.authorTime = QDateTime::fromSecsSinceEpoch(timeStamp);
    result.summary = blame.at(9).mid(8);
    result.filePath = filePath;
    return result;
}

void GitPluginPrivate::instantBlameOnce()
{
    if (!settings().instantBlame()) {
        const TextEditorWidget *widget = TextEditorWidget::currentTextEditorWidget();
        if (!widget)
            return;
        connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
                this, [this] { m_blameMark.reset(); }, Qt::SingleShotConnection);

        connect(widget, &QPlainTextEdit::cursorPositionChanged,
                this, [this] { m_blameMark.reset(); }, Qt::SingleShotConnection);

        const Utils::FilePath workingDirectory = GitPlugin::currentState().topLevel();
        if (!refreshWorkingDirectory(workingDirectory))
            return;
    }

    forceInstantBlame();
}

void GitPluginPrivate::forceInstantBlame()
{
    m_lastVisitedEditorLine = -1;
    instantBlame();
}

void GitPluginPrivate::instantBlame()
{
    const TextEditorWidget *widget = TextEditorWidget::currentTextEditorWidget();
    if (!widget)
        return;

    if (widget->textDocument()->isModified()) {
        m_blameMark.reset();
        m_lastVisitedEditorLine = -1;
        return;
    }

    const QTextCursor cursor = widget->textCursor();
    const QTextBlock block = cursor.block();
    const int line = block.blockNumber() + 1;
    const int lines = widget->document()->blockCount();

    if (line >= lines) {
        m_blameMark.reset();
        m_lastVisitedEditorLine = -1;
        return;
    }

    if (m_lastVisitedEditorLine == line)
        return;

    m_lastVisitedEditorLine = line;

    const Utils::FilePath filePath = widget->textDocument()->filePath();
    const QFileInfo fi(filePath.toString());
    const Utils::FilePath workingDirectory = Utils::FilePath::fromString(fi.path());
    const QString lineString = QString("%1,%1").arg(line);
    const auto commandHandler = [this, filePath, line](const CommandResult &result) {
        if (result.result() == ProcessResult::FinishedWithError &&
                result.cleanedStdErr().contains("no such path")) {
            stopInstantBlame();
            return;
        }
        const QString output = result.cleanedStdOut();
        if (output.isEmpty()) {
            stopInstantBlame();
            return;
        }
        const CommitInfo info = parseBlameOutput(output.split('\n'), filePath, m_author);
        m_blameMark.reset(new BlameMark(filePath, line, info));
    };
    QStringList options = {"blame", "-p"};
    if (settings().instantBlameIgnoreSpaceChanges())
        options.append("-w");
    if (settings().instantBlameIgnoreLineMoves())
        options.append("-M");
    options.append({"-L", lineString, "--", filePath.toString()});
    gitClient().vcsExecWithHandler(workingDirectory, options, this,
                                   commandHandler, RunFlags::NoOutput, m_codec);
}

void GitPluginPrivate::stopInstantBlame()
{
    m_blameMark.reset();
    m_cursorPositionChangedTimer->stop();
    disconnect(m_blameCursorPosConn);
    disconnect(m_documentChangedConn);
}

bool GitPluginPrivate::refreshWorkingDirectory(const FilePath &workingDirectory)
{
    if (workingDirectory.isEmpty())
        return false;

    if (m_workingDirectory == workingDirectory)
        return true;

    m_workingDirectory = workingDirectory;

    const auto commitCodecHandler = [this, workingDirectory](const CommandResult &result) {
        QTextCodec *codec = nullptr;

        if (result.result() == ProcessResult::FinishedWithSuccess) {
            const QString codecName = result.cleanedStdOut().trimmed();
            codec = QTextCodec::codecForName(codecName.toUtf8());
        } else {
            codec = gitClient().defaultCommitEncoding();
        }

        if (m_codec != codec) {
            m_codec = codec;
            forceInstantBlame();
        }
    };
    gitClient().readConfigAsync(workingDirectory, {"config", "i18n.commitEncoding"},
                                           commitCodecHandler);

    const auto authorHandler = [this, workingDirectory](const CommandResult &result) {
        if (result.result() == ProcessResult::FinishedWithSuccess) {
            const QString authorInfo = result.cleanedStdOut().trimmed();
            const Author author = gitClient().parseAuthor(authorInfo);

            if (m_author != author) {
                m_author = author;
                forceInstantBlame();
            }
        }
    };
    gitClient().readConfigAsync(workingDirectory, {"var", "GIT_AUTHOR_IDENT"},
                                           authorHandler);

    return true;
}

IEditor *GitPluginPrivate::openSubmitEditor(const QString &fileName, const CommitData &cd)
{
    IEditor *editor = EditorManager::openEditor(FilePath::fromString(fileName),
                                                Constants::GITSUBMITEDITOR_ID);
    auto submitEditor = qobject_cast<GitSubmitEditor*>(editor);
    QTC_ASSERT(submitEditor, return nullptr);
    setSubmitEditor(submitEditor);
    submitEditor->setCommitData(cd);
    submitEditor->setCheckScriptWorkingDirectory(m_submitRepository);
    QString title;
    switch (cd.commitType) {
    case AmendCommit:
        title = Tr::tr("Amend %1").arg(cd.amendSHA1);
        break;
    case FixupCommit:
        title = Tr::tr("Git Fixup Commit");
        break;
    default:
        title = Tr::tr("Git Commit");
    }
    IDocument *document = submitEditor->document();
    document->setPreferredDisplayName(title);
    VcsBase::setSource(document, m_submitRepository);
    return editor;
}

bool GitPluginPrivate::activateCommit()
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

    auto model = qobject_cast<SubmitFileModel *>(editor->fileModel());
    const CommitType commitType = editor->commitType();
    const QString amendSHA1 = editor->amendSHA1();
    if (model->hasCheckedFiles() || !amendSHA1.isEmpty()) {
        // get message & commit
        if (!DocumentManager::saveDocument(editorDocument))
            return false;

        if (!gitClient().addAndCommit(m_submitRepository, editor->panelData(), commitType,
                                       amendSHA1, m_commitMessageFileName, model)) {
            editor->updateFileModel();
            return false;
        }
    }
    cleanCommitMessageFile();
    if (commitType == FixupCommit) {
        if (!gitClient().beginStashScope(m_submitRepository, "Rebase-fixup",
                                          NoPrompt, editor->panelData().pushAction)) {
            return false;
        }
        gitClient().interactiveRebase(m_submitRepository, amendSHA1, true);
    } else {
        gitClient().continueCommandIfNeeded(m_submitRepository);
        if (editor->panelData().pushAction == NormalPush) {
            gitClient().push(m_submitRepository);
        } else if (editor->panelData().pushAction == PushToGerrit) {
            connect(editor, &QObject::destroyed, this, &GitPluginPrivate::delayedPushToGerrit,
                    Qt::QueuedConnection);
        }
    }

    return true;
}

void GitPluginPrivate::fetch()
{
    gitClient().fetch(currentState().topLevel(), {});
}

void GitPluginPrivate::pull()
{
    if (!DocumentManager::saveAllModifiedDocuments())
        return;
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    FilePath topLevel = state.topLevel();
    bool rebase = settings().pullRebase();

    if (!rebase) {
        QString currentBranch = gitClient().synchronousCurrentLocalBranch(topLevel);
        if (!currentBranch.isEmpty()) {
            currentBranch.prepend("branch.");
            currentBranch.append(".rebase");
            rebase = (gitClient().readConfigValue(topLevel, currentBranch) == "true");
        }
    }

    if (!gitClient().beginStashScope(topLevel, "Pull", rebase ? Default : AllowUnstashed))
        return;
    gitClient().pull(topLevel, rebase);
}

void GitPluginPrivate::push()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    gitClient().push(state.topLevel());
}

void GitPluginPrivate::startMergeTool()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    gitClient().merge(state.topLevel());
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

void GitPluginPrivate::cleanRepository(const FilePath &directory)
{
    // Find files to be deleted
    QString errorMessage;
    QStringList files;
    QStringList ignoredFiles;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const bool gotFiles = gitClient().synchronousCleanList(directory, {}, &files, &ignoredFiles,
                                                           &errorMessage);
    QApplication::restoreOverrideCursor();

    if (!gotFiles) {
        AsynchronousMessageBox::warning(Tr::tr("Unable to Retrieve File List"), errorMessage);
        return;
    }
    if (files.isEmpty() && ignoredFiles.isEmpty()) {
        AsynchronousMessageBox::information(Tr::tr("Repository Clean"),
                                                   Tr::tr("The repository is clean."));
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
    gitClient().updateSubmodulesIfNeeded(state.topLevel(), false);
}

// If the file is modified in an editor, make sure it is saved.
static bool ensureFileSaved(const QString &fileName)
{
    return DocumentManager::saveModifiedDocument(
        DocumentModel::documentForFilePath(FilePath::fromString(fileName)));
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
    applyPatch(state.topLevel(), {});
}

void GitPluginPrivate::applyPatch(const FilePath &workingDirectory, QString file)
{
    // Ensure user has been notified about pending changes
    if (!gitClient().beginStashScope(workingDirectory, "Apply-Patch", AllowUnstashed))
        return;
    // Prompt for file
    if (file.isEmpty()) {
        const QString filter = Tr::tr("Patches (*.patch *.diff)");
        file = QFileDialog::getOpenFileName(ICore::dialogParent(), Tr::tr("Choose Patch"), {}, filter);
        if (file.isEmpty()) {
            gitClient().endStashScope(workingDirectory);
            return;
        }
    }
    // Run!
    QString errorMessage;
    if (gitClient().synchronousApplyPatch(workingDirectory, file, &errorMessage)) {
        if (errorMessage.isEmpty())
            VcsOutputWindow::appendMessage(Tr::tr("Patch %1 successfully applied to %2")
                                           .arg(file, workingDirectory.toUserOutput()));
        else
            VcsOutputWindow::appendError(errorMessage);
    } else {
        VcsOutputWindow::appendError(errorMessage);
    }
    gitClient().endStashScope(workingDirectory);
}

void GitPluginPrivate::stash(bool unstagedOnly)
{
    if (!DocumentManager::saveAllModifiedDocuments())
        return;
    // Simple stash without prompt, reset repo.
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    const FilePath topLevel = state.topLevel();
    gitClient().executeSynchronousStash(topLevel, {}, unstagedOnly);
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
    const QString id = gitClient().synchronousStash(state.topLevel(), {},
                GitClient::StashImmediateRestore | GitClient::StashPromptDescription);
    if (!id.isEmpty() && m_stashDialog)
        m_stashDialog->refresh(state.topLevel(), true);
}

void GitPluginPrivate::stashPop()
{
    if (!DocumentManager::saveAllModifiedDocuments())
        return;
    const FilePath repository = currentState().topLevel();
    gitClient().stashPop(repository);
    if (m_stashDialog)
        m_stashDialog->refresh(repository, true);
}

// Create a non-modal dialog with refresh function or raise if it exists
template <class NonModalDialog>
    inline void showNonModalDialog(const FilePath &topLevel,
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
    ICore::registerWindow(m_remoteDialog, Context("Git.Remotes"));
}

void GitPluginPrivate::initRepository()
{
    createRepository();
}

void GitPluginPrivate::stashList()
{
    showNonModalDialog(currentState().topLevel(), m_stashDialog);
    ICore::registerWindow(m_stashDialog, Context("Git.Stashes"));
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
    for (ParameterAction *fileAction : std::as_const(m_fileActions))
        fileAction->setParameter(fileName);
    // If the current file looks like a patch, offer to apply
    m_applyCurrentFilePatchAction->setParameter(state.currentPatchFileDisplayName());
    const QString projectName = state.currentProjectName();
    for (ParameterAction *projectAction : std::as_const(m_projectActions))
        projectAction->setParameter(projectName);

    for (QAction *repositoryAction : std::as_const(m_repositoryActions))
        repositoryAction->setEnabled(repositoryEnabled);

    m_submoduleUpdateAction->setVisible(repositoryEnabled
            && !gitClient().submoduleList(state.topLevel()).isEmpty());

    updateContinueAndAbortCommands();
    updateRepositoryBrowserAction();

    m_gerritPlugin.updateActions(state);
}

void GitPluginPrivate::updateContinueAndAbortCommands()
{
    if (currentState().hasTopLevel()) {
        GitClient::CommandInProgress gitCommandInProgress =
                gitClient().checkCommandInProgress(currentState().topLevel());

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
    m_gerritPlugin.push(m_submitRepository);
}

void GitPluginPrivate::updateBranches(const FilePath &repository)
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
        gitClient().show(FilePath::fromUserInput(workingDirectory), options.at(1));
    return nullptr;
}

void GitPluginPrivate::updateRepositoryBrowserAction()
{
    const bool repositoryEnabled = currentState().hasTopLevel();
    const bool hasRepositoryBrowserCmd = !settings().repositoryBrowserCmd().isEmpty();
    m_repositoryBrowserAction->setEnabled(repositoryEnabled && hasRepositoryBrowserCmd);
}

QString GitPluginPrivate::displayName() const
{
    return QLatin1String("Git");
}

Id GitPluginPrivate::id() const
{
    return Id(VcsBase::Constants::VCS_ID_GIT);
}

bool GitPluginPrivate::isVcsFileOrDirectory(const FilePath &filePath) const
{
    if (filePath.fileName().compare(".git", HostOsInfo::fileNameCaseSensitivity()))
        return false;
    if (filePath.isDir())
        return true;
    QFile file(filePath.toString());
    if (!file.open(QFile::ReadOnly))
        return false;
    return file.read(8) == "gitdir: ";
}

bool GitPluginPrivate::isConfigured() const
{
    return !gitClient().vcsBinary().isEmpty();
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

bool GitPluginPrivate::vcsOpen(const FilePath & /*filePath*/)
{
    return false;
}

bool GitPluginPrivate::vcsAdd(const FilePath &filePath)
{
    return gitClient().synchronousAdd(filePath.parentDir(), {filePath.fileName()}, {"--intent-to-add"});
}

bool GitPluginPrivate::vcsDelete(const FilePath &filePath)
{
    return gitClient().synchronousDelete(filePath.absolutePath(), true, {filePath.fileName()});
}

bool GitPluginPrivate::vcsMove(const FilePath &from, const FilePath &to)
{
    const QFileInfo fromInfo = from.toFileInfo();
    const QFileInfo toInfo = to.toFileInfo();
    return gitClient().synchronousMove(from.absolutePath(), fromInfo.absoluteFilePath(), toInfo.absoluteFilePath());
}

bool GitPluginPrivate::vcsCreateRepository(const FilePath &directory)
{
    return gitClient().synchronousInit(directory);
}

QString GitPluginPrivate::vcsTopic(const FilePath &directory)
{
    QString topic = IVersionControl::vcsTopic(directory);
    const QString commandInProgress = gitClient().commandInProgressDescription(directory);
    if (!commandInProgress.isEmpty())
        topic += " (" + commandInProgress + ')';
    return topic;
}

VcsCommand *GitPluginPrivate::createInitialCheckoutCommand(const QString &url,
                                                           const FilePath &baseDirectory,
                                                           const QString &localName,
                                                           const QStringList &extraArgs)
{
    QStringList args = {"clone", "--progress"};
    args << extraArgs << url << localName;

    auto command = VcsBaseClient::createVcsCommand(baseDirectory, gitClient().processEnvironment());
    command->addFlags(RunFlags::SuppressStdErr);
    command->addJob({gitClient().vcsBinary(), args}, -1);
    return command;
}

GitPluginPrivate::RepoUrl GitPluginPrivate::getRepoUrl(const QString &location) const
{
    return GitRemote(location);
}

FilePaths GitPluginPrivate::additionalToolsPath() const
{
    FilePaths res = settings().searchPathList();
    const FilePath binaryPath = gitClient().gitBinDirectory();
    if (!binaryPath.isEmpty() && !res.contains(binaryPath))
        res << binaryPath;
    return res;
}

bool GitPluginPrivate::managesDirectory(const FilePath &directory, FilePath *topLevel) const
{
    const FilePath topLevelFound = gitClient().findRepositoryForDirectory(directory);
    if (topLevel)
        *topLevel = topLevelFound;
    return !topLevelFound.isEmpty();
}

bool GitPluginPrivate::managesFile(const FilePath &workingDirectory, const QString &fileName) const
{
    return gitClient().managesFile(workingDirectory, fileName);
}

FilePaths GitPluginPrivate::unmanagedFiles(const FilePaths &filePaths) const
{
    return gitClient().unmanagedFiles(filePaths);
}

void GitPluginPrivate::vcsAnnotate(const FilePath &filePath, int line)
{
    gitClient().annotate(filePath.absolutePath(), filePath.fileName(), line);
}

void GitPlugin::emitFilesChanged(const QStringList &l)
{
    emit dd->filesChanged(l);
}

void GitPlugin::emitRepositoryChanged(const FilePath &r)
{
    emit dd->repositoryChanged(r);
}

void GitPlugin::startRebaseFromCommit(const FilePath &workingDirectory, const QString &commit)
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

void GitPlugin::updateBranches(const FilePath &repository)
{
    dd->updateBranches(repository);
}

void GitPlugin::gerritPush(const FilePath &topLevel)
{
    dd->m_gerritPlugin.push(topLevel);
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

} // Git::Internal

Q_DECLARE_METATYPE(Git::Internal::RemoteTest)

namespace Git::Internal {

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
    if (HostOsInfo::isWindowsHost()) {
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

} // Git::Internal

#include "gitplugin.moc"
