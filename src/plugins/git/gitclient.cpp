// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gitclient.h"

#include "branchadddialog.h"
#include "commitdata.h"
#include "gitconstants.h"
#include "giteditor.h"
#include "gitplugin.h"
#include "gittr.h"
#include "gitutils.h"
#include "mergetool.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <utils/async.h>
#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/mimeutils.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/temporaryfile.h>
#include <utils/theme/theme.h>

#include <vcsbase/submitfilemodel.h>
#include <vcsbase/vcsbasediffeditorcontroller.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseeditorconfig.h>
#include <vcsbase/vcsbaseplugin.h>
#include <vcsbase/vcsbasesubmiteditor.h>
#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsoutputwindow.h>

#include <diffeditor/diffeditorconstants.h>

#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <QAction>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextCodec>

const char GIT_DIRECTORY[] = ".git";
const char HEAD[] = "HEAD";
const char CHERRY_PICK_HEAD[] = "CHERRY_PICK_HEAD";
[[maybe_unused]] const char BRANCHES_PREFIX[] = "Branches: ";
const char stashNamePrefix[] = "stash@{";
const char noColorOption[] = "--no-color";
const char colorOption[] = "--color=always";
const char patchOption[] = "--patch";
const char graphOption[] = "--graph";
const char decorateOption[] = "--decorate";
const char showFormatC[] =
        "--pretty=format:commit %H%d%n"
        "Author: %an <%ae>, %ad (%ar)%n"
        "Committer: %cn <%ce>, %cd (%cr)%n"
        "%n"
        "%B";

using namespace Core;
using namespace DiffEditor;
using namespace Tasking;
using namespace Utils;
using namespace VcsBase;

namespace Git::Internal {

static QString branchesDisplay(const QString &prefix, QStringList *branches, bool *first)
{
    const int limit = 12;
    const int count = branches->count();
    int more = 0;
    QString output;
    if (*first)
        *first = false;
    else
        output += QString(sizeof(BRANCHES_PREFIX) - 1 /* the \0 */, ' '); // Align
    output += prefix + ": ";
    // If there are more than 'limit' branches, list limit/2 (first limit/4 and last limit/4)
    if (count > limit) {
        const int leave = limit / 2;
        more = count - leave;
        branches->erase(branches->begin() + leave / 2 + 1, branches->begin() + count - leave / 2);
        (*branches)[leave / 2] = "...";
    }
    output += branches->join(", ");
    //: Displayed after the untranslated message "Branches: branch1, branch2 'and %n more'"
    //  in git show.
    if (more > 0)
        output += ' ' + Tr::tr("and %n more", nullptr, more);
    return output;
}

///////////////////////////////

static void stage(DiffEditorController *diffController, const QString &patch, bool revert)
{
    TemporaryFile patchFile("git-patchfile");
    if (!patchFile.open())
        return;

    const FilePath baseDir = diffController->workingDirectory();
    QTextCodec *codec = EditorManager::defaultTextCodec();
    const QByteArray patchData = codec ? codec->fromUnicode(patch) : patch.toLocal8Bit();
    patchFile.write(patchData);
    patchFile.close();

    QStringList args = {"--cached"};
    if (revert)
        args << "--reverse";
    QString errorMessage;
    if (gitClient().synchronousApplyPatch(baseDir, patchFile.fileName(),
                                                     &errorMessage, args)) {
        if (errorMessage.isEmpty()) {
            if (revert)
                VcsOutputWindow::appendSilently(Tr::tr("Chunk successfully unstaged"));
            else
                VcsOutputWindow::appendSilently(Tr::tr("Chunk successfully staged"));
        } else {
            VcsOutputWindow::appendError(errorMessage);
        }
        diffController->requestReload();
    } else {
        VcsOutputWindow::appendError(errorMessage);
    }
}

class GitBaseDiffEditorController : public VcsBaseDiffEditorController
{
    Q_OBJECT

protected:
    explicit GitBaseDiffEditorController(IDocument *document);

    QStringList addConfigurationArguments(const QStringList &args) const;

private:
    void addExtraActions(QMenu *menu, int fileIndex, int chunkIndex,
                         const ChunkSelection &selection) final
    {
        menu->addSeparator();

        auto stageChunk = [this, fileIndex, chunkIndex](DiffEditorController::PatchOptions options,
                                                        const DiffEditor::ChunkSelection &selection) {
            options |= DiffEditorController::AddPrefix;
            const QString patch = makePatch(fileIndex, chunkIndex, selection, options);
            stage(this, patch, options & Revert);
        };

        QAction *stageChunkAction = menu->addAction(Tr::tr("Stage Chunk"));
        connect(stageChunkAction, &QAction::triggered, this, [stageChunk] {
            stageChunk(DiffEditorController::NoOption, {});
        });
        QAction *stageLinesAction = menu->addAction(Tr::tr("Stage Selection (%n Lines)", "",
                                                           selection.selectedRowsCount()));
        connect(stageLinesAction, &QAction::triggered,  this, [stageChunk, selection] {
            stageChunk(DiffEditorController::NoOption, selection);
        });
        QAction *unstageChunkAction = menu->addAction(Tr::tr("Unstage Chunk"));
        connect(unstageChunkAction, &QAction::triggered, this, [stageChunk] {
            stageChunk(DiffEditorController::Revert, {});
        });
        QAction *unstageLinesAction = menu->addAction(Tr::tr("Unstage Selection (%n Lines)", "",
                                                             selection.selectedRowsCount()));
        connect(unstageLinesAction, &QAction::triggered, this, [stageChunk, selection] {
            stageChunk(DiffEditorController::Revert, selection);
        });

        if (selection.isNull()) {
            stageLinesAction->setVisible(false);
            unstageLinesAction->setVisible(false);
        }
        if (!chunkExists(fileIndex, chunkIndex)) {
            stageChunkAction->setEnabled(false);
            stageLinesAction->setEnabled(false);
            unstageChunkAction->setEnabled(false);
            unstageLinesAction->setEnabled(false);
        }
    }
};

class GitDiffEditorController : public GitBaseDiffEditorController
{
public:
    explicit GitDiffEditorController(IDocument *document,
                                     const QString &leftCommit,
                                     const QString &rightCommit,
                                     const QStringList &extraArgs);
private:
    QStringList diffArgs(const QString &leftCommit, const QString &rightCommit,
                         const QStringList &extraArgs) const
    {
        QStringList res = {"diff"};
        if (!leftCommit.isEmpty())
            res << leftCommit;

        // This is workaround for lack of support for merge commits and resolving conflicts,
        // we compare the current state of working tree to the HEAD of current branch
        // instead of showing unsupported combined diff format.
        auto fixRightCommit = [this](const QString &commit) {
            if (!commit.isEmpty())
                return commit;
            if (gitClient().checkCommandInProgress(workingDirectory()) == GitClient::NoCommand)
                return QString();
            return QString(HEAD);
        };
        const QString fixedRightCommit = fixRightCommit(rightCommit);
        if (!fixedRightCommit.isEmpty())
            res << fixedRightCommit;

        res << extraArgs;
        return res;
    }
};

GitDiffEditorController::GitDiffEditorController(IDocument *document,
                                                 const QString &leftCommit,
                                                 const QString &rightCommit,
                                                 const QStringList &extraArgs)
    : GitBaseDiffEditorController(document)
{
    const TreeStorage<QString> diffInputStorage;

    const auto setupDiff = [=](Process &process) {
        process.setCodec(VcsBaseEditor::getCodec(workingDirectory(), {}));
        setupCommand(process, {addConfigurationArguments(diffArgs(leftCommit, rightCommit, extraArgs))});
        VcsOutputWindow::appendCommand(process.workingDirectory(), process.commandLine());
    };
    const auto onDiffDone = [diffInputStorage](const Process &process) {
        *diffInputStorage = process.cleanedStdOut();
    };

    const Group root {
        Tasking::Storage(diffInputStorage),
        ProcessTask(setupDiff, onDiffDone),
        postProcessTask(diffInputStorage)
    };
    setReloadRecipe(root);
}

GitBaseDiffEditorController::GitBaseDiffEditorController(IDocument *document)
    : VcsBaseDiffEditorController(document)
{
    setDisplayName("Git Diff");
}

///////////////////////////////

QStringList GitBaseDiffEditorController::addConfigurationArguments(const QStringList &args) const
{
    QTC_ASSERT(!args.isEmpty(), return args);

    QStringList realArgs = {
        "-c",
        "diff.color=false",
        args.at(0),
        "-m", // show diff against parents instead of merge commits
        "-M", "-C", // Detect renames and copies
        "--first-parent" // show only first parent
    };
    if (ignoreWhitespace())
        realArgs << "--ignore-space-change";
    realArgs << "--unified=" + QString::number(contextLineCount())
             << "--src-prefix=a/" << "--dst-prefix=b/" << args.mid(1);

    return realArgs;
}

class FileListDiffController : public GitBaseDiffEditorController
{
public:
    FileListDiffController(IDocument *document, const QStringList &stagedFiles,
                           const QStringList &unstagedFiles);
};

FileListDiffController::FileListDiffController(IDocument *document, const QStringList &stagedFiles,
                                               const QStringList &unstagedFiles)
        : GitBaseDiffEditorController(document)
{
    struct DiffStorage {
        QString m_stagedOutput;
        QString m_unstagedOutput;
    };

    const TreeStorage<DiffStorage> storage;
    const TreeStorage<QString> diffInputStorage;

    const auto setupStaged = [this, stagedFiles](Process &process) {
        if (stagedFiles.isEmpty())
            return SetupResult::StopWithError;
        process.setCodec(VcsBaseEditor::getCodec(workingDirectory(), stagedFiles));
        setupCommand(process, addConfigurationArguments(
                              QStringList({"diff", "--cached", "--"}) + stagedFiles));
        VcsOutputWindow::appendCommand(process.workingDirectory(), process.commandLine());
        return SetupResult::Continue;
    };
    const auto onStagedDone = [storage](const Process &process) {
        storage->m_stagedOutput = process.cleanedStdOut();
    };

    const auto setupUnstaged = [this, unstagedFiles](Process &process) {
        if (unstagedFiles.isEmpty())
            return SetupResult::StopWithError;
        process.setCodec(VcsBaseEditor::getCodec(workingDirectory(), unstagedFiles));
        setupCommand(process, addConfigurationArguments(
                              QStringList({"diff", "--"}) + unstagedFiles));
        VcsOutputWindow::appendCommand(process.workingDirectory(), process.commandLine());
        return SetupResult::Continue;
    };
    const auto onUnstagedDone = [storage](const Process &process) {
        storage->m_unstagedOutput = process.cleanedStdOut();
    };

    const auto onStagingDone = [storage, diffInputStorage] {
        *diffInputStorage = storage->m_stagedOutput + storage->m_unstagedOutput;
    };

    const Group root {
        Tasking::Storage(storage),
        Tasking::Storage(diffInputStorage),
        Group {
            parallel,
            continueOnDone,
            ProcessTask(setupStaged, onStagedDone),
            ProcessTask(setupUnstaged, onUnstagedDone),
            onGroupDone(onStagingDone)
        },
        postProcessTask(diffInputStorage)
    };
    setReloadRecipe(root);
}

class ShowController : public GitBaseDiffEditorController
{
    Q_OBJECT
public:
    ShowController(IDocument *document, const QString &id);
};

ShowController::ShowController(IDocument *document, const QString &id)
    : GitBaseDiffEditorController(document)
{
    setDisplayName("Git Show");
    static const QString busyMessage = Tr::tr("<resolving>");

    struct ReloadStorage {
        bool m_postProcessDescription = false;
        QString m_commit;

        QString m_header;
        QString m_body;
        QString m_branches;
        QString m_precedes;
        QStringList m_follows;
    };

    const TreeStorage<ReloadStorage> storage;
    const TreeStorage<QString> diffInputStorage;

    const auto updateDescription = [this](const ReloadStorage &storage) {
        QString desc = storage.m_header;
        if (!storage.m_branches.isEmpty())
            desc.append("Branches: " + storage.m_branches + '\n');
        if (!storage.m_precedes.isEmpty())
            desc.append("Precedes: " + storage.m_precedes + '\n');
        QStringList follows;
        for (const QString &str : storage.m_follows) {
            if (!str.isEmpty())
                follows.append(str);
        }
        if (!follows.isEmpty())
            desc.append("Follows: " + follows.join(", ") + '\n');
        desc.append('\n' + storage.m_body);
        setDescription(desc);
    };

    const auto setupDescription = [this, id](Process &process) {
        process.setCodec(gitClient().encoding(GitClient::EncodingCommit, workingDirectory()));
        setupCommand(process, {"show", "-s", noColorOption, showFormatC, id});
        VcsOutputWindow::appendCommand(process.workingDirectory(), process.commandLine());
        setDescription(Tr::tr("Waiting for data..."));
    };
    const auto onDescriptionDone = [this, storage, updateDescription](const Process &process) {
        ReloadStorage *data = storage.activeStorage();
        const QString output = process.cleanedStdOut();
        data->m_postProcessDescription = output.startsWith("commit ");
        if (!data->m_postProcessDescription) {
            setDescription(output);
            return;
        }
        const int lastHeaderLine = output.indexOf("\n\n") + 1;
        data->m_commit = output.mid(7, 12);
        data->m_header = output.left(lastHeaderLine);
        data->m_body = output.mid(lastHeaderLine + 1);
        updateDescription(*data);
    };

    const auto desciptionDetailsSetup = [storage] {
        if (!storage->m_postProcessDescription)
            return SetupResult::StopWithDone;
        return SetupResult::Continue;
    };

    const auto setupBranches = [this, storage](Process &process) {
        storage->m_branches = busyMessage;
        setupCommand(process, {"branch", noColorOption, "-a", "--contains", storage->m_commit});
        VcsOutputWindow::appendCommand(process.workingDirectory(), process.commandLine());
    };
    const auto onBranchesDone = [storage, updateDescription](const Process &process) {
        ReloadStorage *data = storage.activeStorage();
        data->m_branches.clear();
        const QString remotePrefix = "remotes/";
        const QString localPrefix = "<Local>";
        const int prefixLength = remotePrefix.length();
        QStringList branches;
        QString previousRemote = localPrefix;
        bool first = true;
        const QStringList branchList = process.cleanedStdOut().split('\n');
        for (const QString &branch : branchList) {
            const QString b = branch.mid(2).trimmed();
            if (b.isEmpty())
                continue;
            if (b.startsWith(remotePrefix)) {
                const int nextSlash = b.indexOf('/', prefixLength);
                if (nextSlash < 0)
                    continue;
                const QString remote = b.mid(prefixLength, nextSlash - prefixLength);
                if (remote != previousRemote) {
                    data->m_branches += branchesDisplay(previousRemote, &branches, &first) + '\n';
                    branches.clear();
                    previousRemote = remote;
                }
                branches << b.mid(nextSlash + 1);
            } else {
                branches << b;
            }
        }
        if (branches.isEmpty()) {
            if (previousRemote == localPrefix)
                data->m_branches += Tr::tr("<None>");
        } else {
            data->m_branches += branchesDisplay(previousRemote, &branches, &first);
        }
        data->m_branches = data->m_branches.trimmed();
        updateDescription(*data);
    };
    const auto onBranchesError = [storage, updateDescription](const Process &) {
        ReloadStorage *data = storage.activeStorage();
        data->m_branches.clear();
        updateDescription(*data);
    };

    const auto setupPrecedes = [this, storage](Process &process) {
        storage->m_precedes = busyMessage;
        setupCommand(process, {"describe", "--contains", storage->m_commit});
    };
    const auto onPrecedesDone = [storage, updateDescription](const Process &process) {
        ReloadStorage *data = storage.activeStorage();
        data->m_precedes = process.cleanedStdOut().trimmed();
        const int tilde = data->m_precedes.indexOf('~');
        if (tilde != -1)
            data->m_precedes.truncate(tilde);
        if (data->m_precedes.endsWith("^0"))
            data->m_precedes.chop(2);
        updateDescription(*data);
    };
    const auto onPrecedesError = [storage, updateDescription](const Process &) {
        ReloadStorage *data = storage.activeStorage();
        data->m_precedes.clear();
        updateDescription(*data);
    };

    const auto setupFollows = [this, storage, updateDescription](TaskTree &taskTree) {
        ReloadStorage *data = storage.activeStorage();
        QStringList parents;
        QString errorMessage;
        // TODO: it's trivial now to call below asynchonously, too
        gitClient().synchronousParentRevisions(workingDirectory(), data->m_commit,
                                               &parents, &errorMessage);
        data->m_follows = {busyMessage};
        data->m_follows.resize(parents.size());

        const auto setupFollow = [this](Process &process, const QString &parent) {
            setupCommand(process, {"describe", "--tags", "--abbrev=0", parent});
        };
        const auto onFollowDone = [data, updateDescription](const Process &process, int index) {
            data->m_follows[index] = process.cleanedStdOut().trimmed();
            updateDescription(*data);
        };
        const auto onFollowsError = [data, updateDescription] {
            data->m_follows.clear();
            updateDescription(*data);
        };

        using namespace std::placeholders;
        QList<GroupItem> tasks {parallel, continueOnDone, onGroupError(onFollowsError)};
        for (int i = 0, total = parents.size(); i < total; ++i) {
            tasks.append(ProcessTask(std::bind(setupFollow, _1, parents.at(i)),
                                 std::bind(onFollowDone, _1, i)));
        }
        taskTree.setRecipe(tasks);
    };

    const auto setupDiff = [this, id](Process &process) {
        setupCommand(process, addConfigurationArguments(
                                  {"show", "--format=format:", // omit header, already generated
                                   noColorOption, decorateOption, id}));
        VcsOutputWindow::appendCommand(process.workingDirectory(), process.commandLine());
    };
    const auto onDiffDone = [diffInputStorage](const Process &process) {
        *diffInputStorage = process.cleanedStdOut();
    };

    const Group root {
        Tasking::Storage(storage),
        Tasking::Storage(diffInputStorage),
        parallel,
        onGroupSetup([this] { setStartupFile(VcsBase::source(this->document()).toString()); }),
        Group {
            finishAllAndDone,
            ProcessTask(setupDescription, onDescriptionDone),
            Group {
                parallel,
                finishAllAndDone,
                onGroupSetup(desciptionDetailsSetup),
                ProcessTask(setupBranches, onBranchesDone, onBranchesError),
                ProcessTask(setupPrecedes, onPrecedesDone, onPrecedesError),
                TaskTreeTask(setupFollows)
            }
        },
        Group {
            ProcessTask(setupDiff, onDiffDone),
            postProcessTask(diffInputStorage)
        }
    };
    setReloadRecipe(root);
}

///////////////////////////////

class BaseGitDiffArgumentsWidget : public VcsBaseEditorConfig
{
    Q_OBJECT

public:
    explicit BaseGitDiffArgumentsWidget(QToolBar *toolBar)
        : VcsBaseEditorConfig(toolBar)
    {
        m_patienceButton
                = addToggleButton("--patience", Tr::tr("Patience"),
                                  Tr::tr("Use the patience algorithm for calculating the differences."));
        mapSetting(m_patienceButton, &settings().diffPatience);
        m_ignoreWSButton = addToggleButton("--ignore-space-change", Tr::tr("Ignore Whitespace"),
                                           Tr::tr("Ignore whitespace only changes."));
        mapSetting(m_ignoreWSButton, &settings().ignoreSpaceChangesInDiff);
    }

protected:
    QAction *m_patienceButton;
    QAction *m_ignoreWSButton;
};

class GitBlameArgumentsWidget : public VcsBaseEditorConfig
{
    Q_OBJECT

public:
    explicit GitBlameArgumentsWidget(QToolBar *toolBar)
        : VcsBaseEditorConfig(toolBar)
    {
        mapSetting(addToggleButton(QString(), Tr::tr("Omit Date"),
                                   Tr::tr("Hide the date of a change from the output.")),
                   &settings().omitAnnotationDate);
        mapSetting(addToggleButton("-w", Tr::tr("Ignore Whitespace"),
                                   Tr::tr("Ignore whitespace only changes.")),
                   &settings().ignoreSpaceChangesInBlame);

        const QList<ChoiceItem> logChoices = {
            ChoiceItem(Tr::tr("No Move Detection"), ""),
            ChoiceItem(Tr::tr("Detect Moves Within File"), "-M"),
            ChoiceItem(Tr::tr("Detect Moves Between Files"), "-M -C"),
            ChoiceItem(Tr::tr("Detect Moves and Copies Between Files"), "-M -C -C")
        };
        mapSetting(addChoices(Tr::tr("Move detection"), {}, logChoices),
                   &settings().blameMoveDetection);

        addReloadButton();
    }
};

class BaseGitLogArgumentsWidget : public BaseGitDiffArgumentsWidget
{
    Q_OBJECT

public:
    BaseGitLogArgumentsWidget(GitEditorWidget *editor)
        : BaseGitDiffArgumentsWidget(editor->toolBar())
    {
        QToolBar *toolBar = editor->toolBar();
        QAction *diffButton = addToggleButton(patchOption, Tr::tr("Diff"),
                                              Tr::tr("Show difference."));
        mapSetting(diffButton, &settings().logDiff);
        connect(diffButton, &QAction::toggled, m_patienceButton, &QAction::setVisible);
        connect(diffButton, &QAction::toggled, m_ignoreWSButton, &QAction::setVisible);
        m_patienceButton->setVisible(diffButton->isChecked());
        m_ignoreWSButton->setVisible(diffButton->isChecked());
        auto filterAction = new QAction(Tr::tr("Filter"), toolBar);
        filterAction->setToolTip(Tr::tr("Filter commits by message or content."));
        filterAction->setCheckable(true);
        connect(filterAction, &QAction::toggled, editor, &GitEditorWidget::toggleFilters);
        toolBar->addAction(filterAction);
    }
};

static bool gitHasRgbColors()
{
    const unsigned gitVersion = gitClient().gitVersion().result();
    return gitVersion >= 0x020300U;
}

static QString logColorName(TextEditor::TextStyle style)
{
    using namespace TextEditor;

    const ColorScheme &scheme = TextEditorSettings::fontSettings().colorScheme();
    QColor color = scheme.formatFor(style).foreground();
    if (!color.isValid())
        color = scheme.formatFor(C_TEXT).foreground();
    return color.name();
};

class GitLogArgumentsWidget : public BaseGitLogArgumentsWidget
{
    Q_OBJECT

public:
    GitLogArgumentsWidget(bool fileRelated, GitEditorWidget *editor)
        : BaseGitLogArgumentsWidget(editor)
    {
        QAction *firstParentButton =
                addToggleButton({"-m", "--first-parent"},
                                Tr::tr("First Parent"),
                                Tr::tr("Follow only the first parent on merge commits."));
        mapSetting(firstParentButton, &settings().firstParent);
        QAction *graphButton = addToggleButton(graphArguments(), Tr::tr("Graph"),
                                               Tr::tr("Show textual graph log."));
        mapSetting(graphButton, &settings().graphLog);

        QAction *colorButton = addToggleButton(QStringList{colorOption},
                                        Tr::tr("Color"), Tr::tr("Use colors in log."));
        mapSetting(colorButton, &settings().colorLog);

        if (fileRelated) {
            QAction *followButton = addToggleButton(
                        "--follow", Tr::tr("Follow"),
                        Tr::tr("Show log also for previous names of the file."));
            mapSetting(followButton, &settings().followRenames);
        }

        addReloadButton();
    }

    QStringList graphArguments() const
    {
        const QString authorName = logColorName(TextEditor::C_LOG_AUTHOR_NAME);
        const QString commitDate = logColorName(TextEditor::C_LOG_COMMIT_DATE);
        const QString commitHash = logColorName(TextEditor::C_LOG_COMMIT_HASH);
        const QString commitSubject = logColorName(TextEditor::C_LOG_COMMIT_SUBJECT);
        const QString decoration = logColorName(TextEditor::C_LOG_DECORATION);

        const QString formatArg = QStringLiteral(
                    "--pretty=format:"
                    "%C(%1)%h%Creset "
                    "%C(%2)%d%Creset "
                    "%C(%3)%an%Creset "
                    "%C(%4)%s%Creset "
                    "%C(%5)%ci%Creset"
                    ).arg(commitHash, decoration, authorName, commitSubject, commitDate);

        QStringList graphArgs = {graphOption, "--oneline", "--topo-order"};

        if (gitHasRgbColors())
            graphArgs << formatArg;
        else
            graphArgs << "--pretty=format:%h %d %an %s %ci";

        return graphArgs;
    }
};

class GitRefLogArgumentsWidget : public BaseGitLogArgumentsWidget
{
    Q_OBJECT

public:
    explicit GitRefLogArgumentsWidget(GitEditorWidget *editor)
        : BaseGitLogArgumentsWidget(editor)
    {
        QAction *showDateButton =
                addToggleButton("--date=iso",
                                Tr::tr("Show Date"),
                                Tr::tr("Show date instead of sequence."));
        mapSetting(showDateButton, &settings().refLogShowDate);

        addReloadButton();
    }
};

static void handleConflictResponse(const VcsBase::CommandResult &result,
                                   const FilePath &workingDirectory,
                                   const QString &abortCommand = {})
{
    const bool success = result.result() == ProcessResult::FinishedWithSuccess;
    const QString stdOutData = success ? QString() : result.cleanedStdOut();
    const QString stdErrData = success ? QString() : result.cleanedStdErr();
    static const QRegularExpression patchFailedRE("Patch failed at ([^\\n]*)");
    static const QRegularExpression conflictedFilesRE("Merge conflict in ([^\\n]*)");
    static const QRegularExpression couldNotApplyRE("[Cc]ould not (?:apply|revert) ([^\\n]*)");
    QString commit;
    QStringList files;

    const QRegularExpressionMatch outMatch = patchFailedRE.match(stdOutData);
    if (outMatch.hasMatch())
        commit = outMatch.captured(1);
    QRegularExpressionMatchIterator it = conflictedFilesRE.globalMatch(stdOutData);
    while (it.hasNext())
        files.append(it.next().captured(1));
    const QRegularExpressionMatch errMatch = couldNotApplyRE.match(stdErrData);
    if (errMatch.hasMatch())
        commit = errMatch.captured(1);

    if (commit.isEmpty() && files.isEmpty()) {
        if (gitClient().checkCommandInProgress(workingDirectory) == GitClient::NoCommand)
            gitClient().endStashScope(workingDirectory);
    } else {
        gitClient().handleMergeConflicts(workingDirectory, commit, files, abortCommand);
    }
}

class GitProgressParser
{
public:
    void operator()(QFutureInterface<void> &fi, const QString &inputText) const {
        const QRegularExpressionMatch match = m_progressExp.match(inputText);
        if (match.hasMatch()) {
            fi.setProgressRange(0, match.captured(2).toInt());
            fi.setProgressValue(match.captured(1).toInt());
        }
    }

private:
    const QRegularExpression m_progressExp{"\\((\\d+)/(\\d+)\\)"}; // e.g. Rebasing (7/42)
};

static inline QString msgRepositoryNotFound(const FilePath &dir)
{
    return Tr::tr("Cannot determine the repository for \"%1\".").arg(dir.toUserOutput());
}

static inline QString msgParseFilesFailed()
{
    return  Tr::tr("Cannot parse the file output.");
}

static QString msgCannotLaunch(const FilePath &binary)
{
    return Tr::tr("Cannot launch \"%1\".").arg(binary.toUserOutput());
}

static inline void msgCannotRun(const QString &message, QString *errorMessage)
{
    if (errorMessage)
        *errorMessage = message;
    else
        VcsOutputWindow::appendError(message);
}

static inline void msgCannotRun(const QStringList &args, const FilePath &workingDirectory,
                                const QString &error, QString *errorMessage)
{
    const QString message = Tr::tr("Cannot run \"%1\" in \"%2\": %3")
            .arg("git " + args.join(' '),
                 workingDirectory.toUserOutput(),
                 error);

    msgCannotRun(message, errorMessage);
}

// ---------------- GitClient

GitClient &gitClient()
{
    static GitClient client;
    return client;
}

GitClient::GitClient()
    : VcsBase::VcsBaseClientImpl(&Internal::settings())
{
    m_gitQtcEditor = QString::fromLatin1("\"%1\" -client -block -pid %2")
            .arg(QCoreApplication::applicationFilePath())
            .arg(QCoreApplication::applicationPid());
}

GitClient::~GitClient() = default;

GitSettings &GitClient::settings()
{
    return Internal::settings();
}

FilePath GitClient::findRepositoryForDirectory(const FilePath &directory) const
{
    if (directory.isEmpty() || directory.endsWith("/.git") || directory.path().contains("/.git/"))
        return {};
    // QFileInfo is outside loop, because it is faster this way
    QFileInfo fileInfo;
    FilePath parent;
    for (FilePath dir = directory; !dir.isEmpty(); dir = dir.parentDir()) {
        const FilePath gitName = dir.pathAppended(GIT_DIRECTORY);
        if (!gitName.exists())
            continue; // parent might exist
        fileInfo.setFile(gitName.toString());
        if (fileInfo.isFile())
            return dir;
        if (gitName.pathAppended("config").exists())
            return dir;
    }
    return {};
}

FilePath GitClient::findGitDirForRepository(const FilePath &repositoryDir) const
{
    static QHash<FilePath, FilePath> repoDirCache;
    FilePath &res = repoDirCache[repositoryDir];
    if (!res.isEmpty())
        return res;

    QString output;
    synchronousRevParseCmd(repositoryDir, "--git-dir", &output);

    res = repositoryDir.resolvePath(output);
    return res;
}

bool GitClient::managesFile(const FilePath &workingDirectory, const QString &fileName) const
{
    const CommandResult result = vcsSynchronousExec(workingDirectory,
                                 {"ls-files", "--error-unmatch", fileName}, RunFlags::NoOutput);
    return result.result() == ProcessResult::FinishedWithSuccess;
}

FilePaths GitClient::unmanagedFiles(const FilePaths &filePaths) const
{
    QMap<FilePath, QStringList> filesForDir;
    for (const FilePath &fp : filePaths) {
        filesForDir[fp.parentDir()] << fp.fileName();
    }
    FilePaths res;
    for (auto it = filesForDir.begin(), end = filesForDir.end(); it != end; ++it) {
        QStringList args({"ls-files", "-z"});
        const QDir wd(it.key().toString());
        args << transform(it.value(), [&wd](const QString &fp) { return wd.relativeFilePath(fp); });
        const CommandResult result = vcsSynchronousExec(it.key(), args, RunFlags::NoOutput);
        if (result.result() != ProcessResult::FinishedWithSuccess)
            return filePaths;
        const auto toAbs = [&wd](const QString &fp) { return wd.absoluteFilePath(fp); };
        const QStringList managedFilePaths =
                Utils::transform(result.cleanedStdOut().split('\0', Qt::SkipEmptyParts), toAbs);
        const QStringList absPaths = Utils::transform(it.value(), toAbs);
        const QStringList filtered = Utils::filtered(absPaths, [&managedFilePaths](const QString &fp) {
            return !managedFilePaths.contains(fp);
        });
        res += FileUtils::toFilePathList(filtered);
    }
    return res;
}

QTextCodec *GitClient::defaultCommitEncoding() const
{
    // Set default commit encoding to 'UTF-8', when it's not set,
    // to solve displaying error of commit log with non-latin characters.
    return QTextCodec::codecForName("UTF-8");
}

QTextCodec *GitClient::encoding(GitClient::EncodingType encodingType, const FilePath &source) const
{
    auto codec = [this](const FilePath &workingDirectory, const QString &configVar) {
        const QString codecName = readConfigValue(workingDirectory, configVar).trimmed();
        if (codecName.isEmpty())
            return defaultCommitEncoding();
        return QTextCodec::codecForName(codecName.toUtf8());
    };

    switch (encodingType) {
    case EncodingSource:
        return source.isFile() ? VcsBaseEditor::getCodec(source) : codec(source, "gui.encoding");
    case EncodingLogOutput:
        return codec(source, "i18n.logOutputEncoding");
    case EncodingCommit:
        return codec(source, "i18n.commitEncoding");
    default:
        return nullptr;
    }
}

void GitClient::requestReload(const QString &documentId, const FilePath &source,
                              const QString &title, const FilePath &workingDirectory,
                              std::function<GitBaseDiffEditorController *(IDocument *)> factory) const
{
    // Creating document might change the referenced source. Store a copy and use it.
    const FilePath sourceCopy = source;

    IDocument *document = DiffEditorController::findOrCreateDocument(documentId, title);
    QTC_ASSERT(document, return);
    GitBaseDiffEditorController *controller = factory(document);
    QTC_ASSERT(controller, return);
    controller->setVcsBinary(settings().gitExecutable());
    controller->setProcessEnvironment(processEnvironment());
    controller->setWorkingDirectory(workingDirectory);

    using namespace std::placeholders;

    VcsBase::setSource(document, sourceCopy);
    EditorManager::activateEditorForDocument(document);
    controller->requestReload();
}

void GitClient::diffFiles(const FilePath &workingDirectory,
                          const QStringList &unstagedFileNames,
                          const QStringList &stagedFileNames) const
{
    const QString documentId = QLatin1String(Constants::GIT_PLUGIN)
            + QLatin1String(".DiffFiles.") + workingDirectory.toString();
    requestReload(documentId,
                  workingDirectory, Tr::tr("Git Diff Files"), workingDirectory,
                  [stagedFileNames, unstagedFileNames](IDocument *doc) {
                      return new FileListDiffController(doc, stagedFileNames, unstagedFileNames);
                  });
}

void GitClient::diffProject(const FilePath &workingDirectory, const QString &projectDirectory) const
{
    const QString documentId = QLatin1String(Constants::GIT_PLUGIN)
            + QLatin1String(".DiffProject.") + workingDirectory.toString();
    requestReload(documentId,
                  workingDirectory, Tr::tr("Git Diff Project"), workingDirectory,
                  [projectDirectory](IDocument *doc){
                      return new GitDiffEditorController(doc, {}, {}, {"--", projectDirectory});
                  });
}

void GitClient::diffRepository(const FilePath &workingDirectory,
                               const QString &leftCommit,
                               const QString &rightCommit) const
{
    const QString documentId = QLatin1String(Constants::GIT_PLUGIN)
            + QLatin1String(".DiffRepository.") + workingDirectory.toString();
    requestReload(documentId, workingDirectory, Tr::tr("Git Diff Repository"), workingDirectory,
                  [&leftCommit, &rightCommit](IDocument *doc) {
        return new GitDiffEditorController(doc, leftCommit, rightCommit, {});
    });
}

void GitClient::diffFile(const FilePath &workingDirectory, const QString &fileName) const
{
    const QString title = Tr::tr("Git Diff \"%1\"").arg(fileName);
    const FilePath sourceFile = VcsBaseEditor::getSource(workingDirectory, fileName);
    const QString documentId = QLatin1String(Constants::GIT_PLUGIN)
            + QLatin1String(".DifFile.") + sourceFile.toString();
    requestReload(documentId, sourceFile, title, workingDirectory,
                  [&fileName](IDocument *doc) {
        return new GitDiffEditorController(doc, {}, {}, {"--", fileName});
    });
}

void GitClient::diffBranch(const FilePath &workingDirectory, const QString &branchName) const
{
    const QString title = Tr::tr("Git Diff Branch \"%1\"").arg(branchName);
    const QString documentId = QLatin1String(Constants::GIT_PLUGIN)
            + QLatin1String(".DiffBranch.") + branchName;
    requestReload(documentId, workingDirectory, title, workingDirectory,
                  [branchName](IDocument *doc) {
        return new GitDiffEditorController(doc, branchName, {}, {});
    });
}

void GitClient::merge(const FilePath &workingDirectory, const QStringList &unmergedFileNames)
{
    auto mergeTool = new MergeTool(this);
    mergeTool->start(workingDirectory, unmergedFileNames);
}

void GitClient::status(const FilePath &workingDirectory) const
{
    VcsOutputWindow::setRepository(workingDirectory);
    vcsExecWithHandler(workingDirectory, {"status", "-u"}, this, [](const CommandResult &) {
        VcsOutputWindow::instance()->clearRepository();
    }, RunFlags::ShowStdOut);
}

static QStringList normalLogArguments()
{
    if (!gitHasRgbColors())
        return {};

    const QString authorName = logColorName(TextEditor::C_LOG_AUTHOR_NAME);
    const QString commitDate = logColorName(TextEditor::C_LOG_COMMIT_DATE);
    const QString commitHash = logColorName(TextEditor::C_LOG_COMMIT_HASH);
    const QString commitSubject = logColorName(TextEditor::C_LOG_COMMIT_SUBJECT);
    const QString decoration = logColorName(TextEditor::C_LOG_DECORATION);

    const QString logArgs = QStringLiteral(
                "--pretty=format:"
                "commit %C(%1)%H%Creset %C(%2)%d%Creset%n"
                "Author: %C(%3)%an <%ae>%Creset%n"
                "Date:   %C(%4)%cD %Creset%n%n"
                "%C(%5)%w(0,4,4)%s%Creset%n%n%b"
                ).arg(commitHash, decoration, authorName, commitDate, commitSubject);

    return {logArgs};
}

void GitClient::log(const FilePath &workingDirectory, const QString &fileName,
                    bool enableAnnotationContextMenu, const QStringList &args)
{
    QString msgArg;
    if (!fileName.isEmpty())
        msgArg = fileName;
    else if (!args.isEmpty() && !args.first().startsWith('-'))
        msgArg = args.first();
    else
        msgArg = workingDirectory.toString();
    // Creating document might change the referenced workingDirectory. Store a copy and use it.
    const FilePath workingDir = workingDirectory;
    const QString title = Tr::tr("Git Log \"%1\"").arg(msgArg);
    const Id editorId = Git::Constants::GIT_LOG_EDITOR_ID;
    const FilePath sourceFile = VcsBaseEditor::getSource(workingDir, fileName);
    GitEditorWidget *editor = static_cast<GitEditorWidget *>(
                createVcsEditor(editorId, title, sourceFile,
                                encoding(EncodingLogOutput), "logTitle", msgArg));
    VcsBaseEditorConfig *argWidget = editor->editorConfig();
    if (!argWidget) {
        argWidget = new GitLogArgumentsWidget(!fileName.isEmpty(), editor);
        argWidget->setBaseArguments(args);
        connect(argWidget, &VcsBaseEditorConfig::commandExecutionRequested, this,
                [=] { this->log(workingDir, fileName, enableAnnotationContextMenu, args); });
        editor->setEditorConfig(argWidget);
    }
    editor->setFileLogAnnotateEnabled(enableAnnotationContextMenu);
    editor->setWorkingDirectory(workingDir);

    QStringList arguments = {"log", decorateOption};
    int logCount = settings().logCount();
    if (logCount > 0)
        arguments << "-n" << QString::number(logCount);

    arguments << argWidget->arguments();
    if (arguments.contains(patchOption)) {
        arguments.removeAll(colorOption);
        editor->setHighlightingEnabled(true);
    } else if (gitHasRgbColors()) {
        editor->setHighlightingEnabled(false);
    }
    if (!arguments.contains(graphOption) && !arguments.contains(patchOption))
        arguments << normalLogArguments();

    const QString authorValue = editor->authorValue();
    if (!authorValue.isEmpty())
        arguments << "--author=" + ProcessArgs::quoteArg(authorValue);

    const QString grepValue = editor->grepValue();
    if (!grepValue.isEmpty())
        arguments << "--grep=" + ProcessArgs::quoteArg(grepValue);

    const QString pickaxeValue = editor->pickaxeValue();
    if (!pickaxeValue.isEmpty())
        arguments << "-S" << ProcessArgs::quoteArg(pickaxeValue);

    if (!editor->caseSensitive())
        arguments << "-i";

    if (!fileName.isEmpty())
        arguments << "--" << fileName;

    vcsExecWithEditor(workingDir, arguments, editor);
}

void GitClient::reflog(const FilePath &workingDirectory, const QString &ref)
{
    const QString title = Tr::tr("Git Reflog \"%1\"").arg(workingDirectory.toUserOutput());
    const Id editorId = Git::Constants::GIT_REFLOG_EDITOR_ID;
    // Creating document might change the referenced workingDirectory. Store a copy and use it.
    const FilePath workingDir = workingDirectory;
    GitEditorWidget *editor = static_cast<GitEditorWidget *>(
                createVcsEditor(editorId, title, workingDir, encoding(EncodingLogOutput),
                                "reflogRepository", workingDir.toString()));
    VcsBaseEditorConfig *argWidget = editor->editorConfig();
    if (!argWidget) {
        argWidget = new GitRefLogArgumentsWidget(editor);
        if (!ref.isEmpty())
            argWidget->setBaseArguments({ref});
        connect(argWidget, &VcsBaseEditorConfig::commandExecutionRequested, this,
                [=] { this->reflog(workingDir, ref); });
        editor->setEditorConfig(argWidget);
    }
    editor->setWorkingDirectory(workingDir);

    QStringList arguments = {"reflog", noColorOption, decorateOption};
    arguments << argWidget->arguments();
    int logCount = settings().logCount();
    if (logCount > 0)
        arguments << "-n" << QString::number(logCount);

    vcsExecWithEditor(workingDir, arguments, editor);
}

// Do not show "0000" or "^32ae4"
static inline bool canShow(const QString &sha)
{
    return !sha.startsWith('^') && sha.count('0') != sha.size();
}

static inline QString msgCannotShow(const QString &sha)
{
    return Tr::tr("Cannot describe \"%1\".").arg(sha);
}

void GitClient::show(const FilePath &source, const QString &id, const QString &name)
{
    if (!canShow(id)) {
        VcsOutputWindow::appendError(msgCannotShow(id));
        return;
    }

    const QString title = Tr::tr("Git Show \"%1\"").arg(name.isEmpty() ? id : name);
    FilePath workingDirectory =
        source.isDir() ? source.absoluteFilePath() : source.absolutePath();
    const FilePath repoDirectory = VcsManager::findTopLevelForDirectory(workingDirectory);
    if (!repoDirectory.isEmpty())
        workingDirectory = repoDirectory;
    const QString documentId = QLatin1String(Constants::GIT_PLUGIN)
            + QLatin1String(".Show.") + id;
    requestReload(documentId, source, title, workingDirectory,
                  [id](IDocument *doc) { return new ShowController(doc, id); });
}

void GitClient::archive(const FilePath &workingDirectory, QString commit)
{
    FilePath repoDirectory = VcsManager::findTopLevelForDirectory(workingDirectory);
    if (repoDirectory.isEmpty())
        repoDirectory = workingDirectory;
    const QString repoName = repoDirectory.fileName();

    QHash<QString, QString> filters;
    QString selectedFilter;
    auto appendFilter = [&filters, &selectedFilter](const QString &name, bool isSelected){
        const auto mimeType = Utils::mimeTypeForName(name);
        const auto filterString = mimeType.filterString();
        filters.insert(filterString, "." + mimeType.preferredSuffix());
        if (isSelected)
            selectedFilter = filterString;
    };

    bool windows = HostOsInfo::isWindowsHost();
    appendFilter("application/zip", windows);
    appendFilter("application/x-compressed-tar", !windows);

    QString output;
    if (synchronousRevParseCmd(repoDirectory, commit, &output))
        commit = output.trimmed();

    FilePath archiveName = FileUtils::getSaveFilePath(
                nullptr,
                Tr::tr("Generate %1 archive").arg(repoName),
                repoDirectory.pathAppended(QString("../%1-%2").arg(repoName, commit.left(8))),
                filters.keys().join(";;"),
                &selectedFilter);
    if (archiveName.isEmpty())
        return;
    const QString extension = filters.value(selectedFilter);
    QFileInfo archive(archiveName.toString());
    if (extension != "." + archive.completeSuffix()) {
        archive = QFileInfo(archive.filePath() + extension);
    }

    if (archive.exists()) {
        if (QMessageBox::warning(ICore::dialogParent(), Tr::tr("Overwrite?"),
            Tr::tr("An item named \"%1\" already exists at this location. "
               "Do you want to overwrite it?").arg(QDir::toNativeSeparators(archive.absoluteFilePath())),
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
            return;
        }
    }

    vcsExec(workingDirectory, {"archive", commit, "-o", archive.absoluteFilePath()},
            RunFlags::ShowStdOut);
}

void GitClient::annotate(const Utils::FilePath &workingDir, const QString &file, int lineNumber,
                         const QString &revision, const QStringList &extraOptions, int firstLine)
{
    const Id editorId = Git::Constants::GIT_BLAME_EDITOR_ID;
    const QString id = VcsBaseEditor::getTitleId(workingDir, {file}, revision);
    const QString title = Tr::tr("Git Blame \"%1\"").arg(id);
    const FilePath sourceFile = VcsBaseEditor::getSource(workingDir, file);

    VcsBaseEditorWidget *editor = createVcsEditor(editorId, title, sourceFile,
            encoding(EncodingSource, sourceFile), "blameFileName", id);
    VcsBaseEditorConfig *argWidget = editor->editorConfig();
    if (!argWidget) {
        argWidget = new GitBlameArgumentsWidget(editor->toolBar());
        argWidget->setBaseArguments(extraOptions);
        connect(argWidget, &VcsBaseEditorConfig::commandExecutionRequested, this, [=] {
            const int line = VcsBaseEditor::lineNumberOfCurrentEditor();
            annotate(workingDir, file, line, revision, extraOptions);
        });
        editor->setEditorConfig(argWidget);
    }

    editor->setWorkingDirectory(workingDir);
    QStringList arguments = {"blame", "--root"};
    arguments << argWidget->arguments();
    if (!revision.isEmpty())
        arguments << revision;
    arguments << "--" << file;
    editor->setDefaultLineNumber(lineNumber);
    if (firstLine > 0)
        editor->setFirstLineNumber(firstLine);
    vcsExecWithEditor(workingDir, arguments, editor);
}

void GitClient::checkout(const FilePath &workingDirectory, const QString &ref, StashMode stashMode,
                         const QObject *context, const VcsBase::CommandHandler &handler)
{
    if (stashMode == StashMode::TryStash && !beginStashScope(workingDirectory, "Checkout"))
        return;

    const QStringList arguments = setupCheckoutArguments(workingDirectory, ref);
    const auto commandHandler = [=](const CommandResult &result) {
        if (stashMode == StashMode::TryStash)
            endStashScope(workingDirectory);
        if (result.result() == ProcessResult::FinishedWithSuccess)
            updateSubmodulesIfNeeded(workingDirectory, true);
        if (handler)
            handler(result);
    };
    vcsExecWithHandler(workingDirectory, arguments, context, commandHandler,
               RunFlags::ShowStdOut | RunFlags::ExpectRepoChanges | RunFlags::ShowSuccessMessage);
}

/* method used to setup arguments for checkout, in case user wants to create local branch */
QStringList GitClient::setupCheckoutArguments(const FilePath &workingDirectory,
                                              const QString &ref)
{
    QStringList arguments = {"checkout", ref};

    QStringList localBranches = synchronousRepositoryBranches(workingDirectory.toString());
    if (localBranches.contains(ref))
        return arguments;

    if (Utils::CheckableMessageBox::question(
            ICore::dialogParent() /*parent*/,
            Tr::tr("Create Local Branch") /*title*/,
            Tr::tr("Would you like to create a local branch?") /*message*/,
            Key("Git.CreateLocalBranchOnCheckout"), /* decider */
            QMessageBox::Yes | QMessageBox::No /*buttons*/,
            QMessageBox::No /*default button*/,
            QMessageBox::No /*button to save*/)
        != QMessageBox::Yes) {
        return arguments;
    }

    if (synchronousCurrentLocalBranch(workingDirectory).isEmpty())
        localBranches.removeFirst();

    QString refSha;
    if (!synchronousRevParseCmd(workingDirectory, ref, &refSha))
        return arguments;

    QString output;
    const QStringList forEachRefArgs = {"refs/remotes/", "--format=%(objectname) %(refname:short)"};
    if (!synchronousForEachRefCmd(workingDirectory, forEachRefArgs, &output))
        return arguments;

    QString remoteBranch;
    const QString head("/HEAD");

    const QStringList refs = output.split('\n');
    for (const QString &singleRef : refs) {
        if (singleRef.startsWith(refSha)) {
            // branch name might be origin/foo/HEAD
            if (!singleRef.endsWith(head) || singleRef.count('/') > 1) {
                remoteBranch = singleRef.mid(refSha.length() + 1);
                if (remoteBranch == ref)
                    break;
            }
        }
    }

    QString target = remoteBranch;
    BranchTargetType targetType = BranchTargetType::Remote;
    if (remoteBranch.isEmpty()) {
        target = ref;
        targetType = BranchTargetType::Commit;
    }
    const QString suggestedName = suggestedLocalBranchName(
                workingDirectory, localBranches, target, targetType);
    BranchAddDialog branchAddDialog(localBranches, BranchAddDialog::Type::AddBranch, ICore::dialogParent());
    branchAddDialog.setBranchName(suggestedName);
    branchAddDialog.setTrackedBranchName(remoteBranch, true);

    if (branchAddDialog.exec() != QDialog::Accepted)
        return arguments;

    arguments.removeLast();
    arguments << "-b" << branchAddDialog.branchName();
    if (branchAddDialog.track())
        arguments << "--track" << remoteBranch;
    else
        arguments << "--no-track" << ref;

    return arguments;
}

void GitClient::reset(const FilePath &workingDirectory, const QString &argument, const QString &commit)
{
    QStringList arguments = {"reset", argument};
    if (!commit.isEmpty())
        arguments << commit;

    RunFlags flags = RunFlags::ShowStdOut | RunFlags::ShowSuccessMessage;
    if (argument == "--hard") {
        if (gitStatus(workingDirectory, StatusMode(NoUntracked | NoSubmodules)) != StatusUnchanged) {
            if (QMessageBox::question(
                        Core::ICore::dialogParent(), Tr::tr("Reset"),
                        Tr::tr("All changes in working directory will be discarded. Are you sure?"),
                        QMessageBox::Yes | QMessageBox::No,
                        QMessageBox::No) == QMessageBox::No) {
                return;
            }
        }
        flags |= RunFlags::ExpectRepoChanges;
    }
    vcsExec(workingDirectory, arguments, flags);
}

void GitClient::removeStaleRemoteBranches(const FilePath &workingDirectory, const QString &remote)
{
    const QStringList arguments = {"remote", "prune", remote};
    const auto commandHandler = [workingDirectory](const CommandResult &result) {
        if (result.result() == ProcessResult::FinishedWithSuccess)
            GitPlugin::updateBranches(workingDirectory);
    };
    vcsExecWithHandler(workingDirectory, arguments, this, commandHandler,
                       RunFlags::ShowStdOut | RunFlags::ShowSuccessMessage);
}

void GitClient::recoverDeletedFiles(const FilePath &workingDirectory)
{
    const CommandResult result = vcsSynchronousExec(workingDirectory, {"ls-files", "--deleted"},
                                                    RunFlags::SuppressCommandLogging);
    if (result.result() == ProcessResult::FinishedWithSuccess) {
        const QString stdOut = result.cleanedStdOut().trimmed();
        if (stdOut.isEmpty()) {
            VcsOutputWindow::appendError(Tr::tr("Nothing to recover"));
            return;
        }
        const QStringList files = stdOut.split('\n');
        synchronousCheckoutFiles(workingDirectory, files, QString(), nullptr, false);
        VcsOutputWindow::append(Tr::tr("Files recovered"), VcsOutputWindow::Message);
    }
}

void GitClient::addFile(const FilePath &workingDirectory, const QString &fileName)
{
    vcsExec(workingDirectory, {"add", fileName});
}

bool GitClient::synchronousLog(const FilePath &workingDirectory, const QStringList &arguments,
                               QString *output, QString *errorMessageIn, RunFlags flags)
{
    QStringList allArguments = {"log", noColorOption};

    allArguments.append(arguments);

    const CommandResult result = vcsSynchronousExec(workingDirectory, allArguments, flags,
                        vcsTimeoutS(), encoding(EncodingLogOutput, workingDirectory));
    if (result.result() == ProcessResult::FinishedWithSuccess) {
        *output = result.cleanedStdOut();
        return true;
    }
    msgCannotRun(Tr::tr("Cannot obtain log of \"%1\": %2")
                 .arg(workingDirectory.toUserOutput(), result.cleanedStdErr()), errorMessageIn);
    return false;
}

bool GitClient::synchronousAdd(const FilePath &workingDirectory,
                               const QStringList &files,
                               const QStringList &extraOptions)
{
    QStringList args{"add"};
    args += extraOptions;
    args += "--";
    args += files;
    return vcsSynchronousExec(workingDirectory, args).result()
            == ProcessResult::FinishedWithSuccess;
}

bool GitClient::synchronousDelete(const FilePath &workingDirectory,
                                  bool force,
                                  const QStringList &files)
{
    QStringList arguments = {"rm"};
    if (force)
        arguments << "--force";
    arguments << "--";
    arguments.append(files);
    return vcsSynchronousExec(workingDirectory, arguments).result()
            == ProcessResult::FinishedWithSuccess;
}

bool GitClient::synchronousMove(const FilePath &workingDirectory,
                                const QString &from,
                                const QString &to)
{
    return vcsSynchronousExec(workingDirectory, {"mv", from, to}).result()
            == ProcessResult::FinishedWithSuccess;
}

bool GitClient::synchronousReset(const FilePath &workingDirectory,
                                 const QStringList &files,
                                 QString *errorMessage)
{
    QStringList arguments = {"reset"};
    if (files.isEmpty())
        arguments << "--hard";
    else
        arguments << HEAD << "--" << files;

    const CommandResult result = vcsSynchronousExec(workingDirectory, arguments);
    const QString stdOut = result.cleanedStdOut();
    VcsOutputWindow::append(stdOut);
    // Note that git exits with 1 even if the operation is successful
    // Assume real failure if the output does not contain "foo.cpp modified"
    // or "Unstaged changes after reset" (git 1.7.0).
    if (result.result() != ProcessResult::FinishedWithSuccess
        && (!stdOut.contains("modified") && !stdOut.contains("Unstaged changes after reset"))) {
        if (files.isEmpty()) {
            msgCannotRun(arguments, workingDirectory, result.cleanedStdErr(), errorMessage);
        } else {
            msgCannotRun(Tr::tr("Cannot reset %n files in \"%1\": %2", nullptr, files.size())
                .arg(workingDirectory.toUserOutput(), result.cleanedStdErr()),
                         errorMessage);
        }
        return false;
    }
    return true;
}

// Initialize repository
bool GitClient::synchronousInit(const FilePath &workingDirectory)
{
    const CommandResult result = vcsSynchronousExec(workingDirectory, QStringList{"init"});
    // '[Re]Initialized...'
    VcsOutputWindow::append(result.cleanedStdOut());
    if (result.result() == ProcessResult::FinishedWithSuccess) {
        resetCachedVcsInfo(workingDirectory);
        return true;
    }
    return false;
}

/* Checkout, supports:
 * git checkout -- <files>
 * git checkout revision -- <files>
 * git checkout revision -- . */
bool GitClient::synchronousCheckoutFiles(const FilePath &workingDirectory, QStringList files,
                                         QString revision, QString *errorMessage,
                                         bool revertStaging)
{
    if (revertStaging && revision.isEmpty())
        revision = HEAD;
    if (files.isEmpty())
        files = QStringList(".");
    QStringList arguments = {"checkout"};
    if (revertStaging)
        arguments << revision;
    arguments << "--" << files;
    const CommandResult result = vcsSynchronousExec(workingDirectory, arguments,
                                                    RunFlags::ExpectRepoChanges);
    if (result.result() == ProcessResult::FinishedWithSuccess)
        return true;

    const QString fileArg = files.join(", ");
    //: Meaning of the arguments: %1: revision, %2: files, %3: repository,
    //: %4: Error message
    msgCannotRun(Tr::tr("Cannot checkout \"%1\" of %2 in \"%3\": %4")
                 .arg(revision, fileArg, workingDirectory.toUserOutput(), result.cleanedStdErr()),
                 errorMessage);
    return false;
}

static QString msgParentRevisionFailed(const FilePath &workingDirectory,
                                              const QString &revision,
                                              const QString &why)
{
    //: Failed to find parent revisions of a SHA1 for "annotate previous"
    return Tr::tr("Cannot find parent revisions of \"%1\" in \"%2\": %3")
            .arg(revision, workingDirectory.toUserOutput(), why);
}

static QString msgInvalidRevision()
{
    return Tr::tr("Invalid revision");
}

// Split a line of "<commit> <parent1> ..." to obtain parents from "rev-list" or "log".
static inline bool splitCommitParents(const QString &line,
                                      QString *commit = nullptr,
                                      QStringList *parents = nullptr)
{
    if (commit)
        commit->clear();
    if (parents)
        parents->clear();
    QStringList tokens = line.trimmed().split(' ');
    if (tokens.size() < 2)
        return false;
    if (commit)
        *commit = tokens.front();
    tokens.pop_front();
    if (parents)
        *parents = tokens;
    return true;
}

bool GitClient::synchronousRevListCmd(const FilePath &workingDirectory, const QStringList &extraArguments,
                                      QString *output, QString *errorMessage) const
{
    const QStringList arguments = QStringList({"rev-list", noColorOption}) + extraArguments;
    const CommandResult result = vcsSynchronousExec(workingDirectory, arguments, RunFlags::NoOutput);
    if (result.result() != ProcessResult::FinishedWithSuccess) {
        msgCannotRun(arguments, workingDirectory, result.cleanedStdErr(), errorMessage);
        return false;
    }
    *output = result.cleanedStdOut();
    return true;
}

// Find out the immediate parent revisions of a revision of the repository.
// Might be several in case of merges.
bool GitClient::synchronousParentRevisions(const FilePath &workingDirectory,
                                           const QString &revision,
                                           QStringList *parents,
                                           QString *errorMessage) const
{
    if (parents && !isValidRevision(revision)) { // Not Committed Yet
        *parents = QStringList(HEAD);
        return true;
    }
    QString outputText;
    QString errorText;
    if (!synchronousRevListCmd(workingDirectory, {"--parents", "--max-count=1", revision},
                               &outputText, &errorText)) {
        *errorMessage = msgParentRevisionFailed(workingDirectory, revision, errorText);
        return false;
    }
    // Should result in one line of blank-delimited revisions, specifying current first
    // unless it is top.
    outputText.remove('\n');
    if (!splitCommitParents(outputText, nullptr, parents)) {
        *errorMessage = msgParentRevisionFailed(workingDirectory, revision, msgInvalidRevision());
        return false;
    }
    return true;
}

QString GitClient::synchronousShortDescription(const FilePath &workingDirectory, const QString &revision) const
{
    // HACK: The hopefully rare "_-_" will be replaced by quotes in the output,
    // leaving it in breaks command line quoting on Windows, see QTCREATORBUG-23208.
    const QString quoteReplacement = "_-_";

    // Short SHA1, author, subject
    const QString defaultShortLogFormat = "%h (%an " + quoteReplacement + "%s";
    const int maxShortLogLength = 120;

    // Short SHA 1, author, subject
    QString output = synchronousShortDescription(workingDirectory, revision, defaultShortLogFormat);
    output.replace(quoteReplacement, "\"");
    if (output != revision) {
        if (output.length() > maxShortLogLength) {
            output.truncate(maxShortLogLength);
            output.append("...");
        }
        output.append("\")");
    }
    return output;
}

QString GitClient::synchronousCurrentLocalBranch(const FilePath &workingDirectory) const
{
    QString branch;
    const CommandResult result = vcsSynchronousExec(workingDirectory, {"symbolic-ref", HEAD},
                                                    RunFlags::NoOutput);
    if (result.result() == ProcessResult::FinishedWithSuccess) {
        branch = result.cleanedStdOut().trimmed();
    } else {
        const FilePath gitDir = findGitDirForRepository(workingDirectory);
        const FilePath rebaseHead = gitDir / "rebase-merge/head-name";
        QFile head(rebaseHead.toFSPathString());
        if (head.open(QFile::ReadOnly))
            branch = QString::fromUtf8(head.readLine()).trimmed();
    }
    if (!branch.isEmpty()) {
        const QString refsHeadsPrefix = "refs/heads/";
        if (branch.startsWith(refsHeadsPrefix)) {
            branch.remove(0, refsHeadsPrefix.count());
            return branch;
        }
    }
    return {};
}

bool GitClient::synchronousHeadRefs(const FilePath &workingDirectory, QStringList *output,
                                    QString *errorMessage) const
{
    const QStringList arguments = {"show-ref", "--head", "--abbrev=10", "--dereference"};
    const CommandResult result = vcsSynchronousExec(workingDirectory, arguments, RunFlags::NoOutput);
    if (result.result() != ProcessResult::FinishedWithSuccess) {
        msgCannotRun(arguments, workingDirectory, result.cleanedStdErr(), errorMessage);
        return false;
    }

    const QString stdOut = result.cleanedStdOut();
    const QString headSha = stdOut.left(10);
    QString rest = stdOut.mid(15);

    const QStringList headShaLines = Utils::filtered(
                rest.split('\n'), [&headSha](const QString &s) { return s.startsWith(headSha); });
    *output = Utils::transform(headShaLines, [](const QString &s) { return s.mid(11); }); // sha + space

    return true;
}

// Retrieve topic (branch, tag or HEAD hash)
QString GitClient::synchronousTopic(const FilePath &workingDirectory) const
{
    // First try to find branch
    const QString branch = synchronousCurrentLocalBranch(workingDirectory);
    if (!branch.isEmpty())
        return branch;

    // Detached HEAD, try a tag or remote branch
    QStringList references;
    if (!synchronousHeadRefs(workingDirectory, &references))
        return {};

    const QString tagStart("refs/tags/");
    const QString remoteStart("refs/remotes/");
    const QString dereference("^{}");
    QString remoteBranch;

    for (const QString &ref : std::as_const(references)) {
        int derefInd = ref.indexOf(dereference);
        if (ref.startsWith(tagStart))
            return ref.mid(tagStart.size(), (derefInd == -1) ? -1 : derefInd - tagStart.size());
        if (ref.startsWith(remoteStart)) {
            remoteBranch = ref.mid(remoteStart.size(),
                                   (derefInd == -1) ? -1 : derefInd - remoteStart.size());
        }
    }
    if (!remoteBranch.isEmpty())
        return remoteBranch;

    // No tag or remote branch - try git describe
    const CommandResult result = vcsSynchronousExec(workingDirectory, QStringList{"describe"},
                                                    RunFlags::NoOutput);
    if (result.result() == ProcessResult::FinishedWithSuccess) {
        const QString stdOut = result.cleanedStdOut().trimmed();
        if (!stdOut.isEmpty())
            return stdOut;
    }
    return Tr::tr("Detached HEAD");
}

bool GitClient::synchronousRevParseCmd(const FilePath &workingDirectory, const QString &ref,
                                       QString *output, QString *errorMessage) const
{
    const QStringList arguments = {"rev-parse", ref};
    const CommandResult result = vcsSynchronousExec(workingDirectory, arguments, RunFlags::NoOutput);
    *output = result.cleanedStdOut().trimmed();
    if (result.result() == ProcessResult::FinishedWithSuccess)
        return true;
    msgCannotRun(arguments, workingDirectory, result.cleanedStdErr(), errorMessage);
    return false;
}

// Retrieve head revision
ProcessTask GitClient::topRevision(const FilePath &workingDirectory,
    const std::function<void(const QString &, const QDateTime &)> &callback)
{
    const auto setupProcess = [=](Process &process) {
        setupCommand(process, workingDirectory, {"show", "-s", "--pretty=format:%H:%ct", HEAD});
    };
    const auto onProcessDone = [=](const Process &process) {
        const QStringList output = process.cleanedStdOut().trimmed().split(':');
        QDateTime dateTime;
        if (output.size() > 1) {
            bool ok = false;
            const qint64 timeT = output.at(1).toLongLong(&ok);
            if (ok)
                dateTime = QDateTime::fromSecsSinceEpoch(timeT);
        }
        callback(output.first(), dateTime);
    };

    return ProcessTask(setupProcess, onProcessDone);
}

bool GitClient::isRemoteCommit(const FilePath &workingDirectory, const QString &commit)
{
    const CommandResult result = vcsSynchronousExec(workingDirectory,
                                 {"branch", "-r", "--contains", commit}, RunFlags::NoOutput);
    return !result.rawStdOut().isEmpty();
}

// Format an entry in a one-liner for selection list using git log.
QString GitClient::synchronousShortDescription(const FilePath &workingDirectory, const QString &revision,
                                            const QString &format) const
{
    const QStringList arguments = {"log", noColorOption, ("--pretty=format:" + format),
                                   "--max-count=1", revision};
    const CommandResult result = vcsSynchronousExec(workingDirectory, arguments, RunFlags::NoOutput);
    if (result.result() != ProcessResult::FinishedWithSuccess) {
        VcsOutputWindow::appendSilently(Tr::tr("Cannot describe revision \"%1\" in \"%2\": %3")
                        .arg(revision, workingDirectory.toUserOutput(), result.cleanedStdErr()));
        return revision;
    }
    return stripLastNewline(result.cleanedStdOut());
}

// Create a default message to be used for describing stashes
static inline QString creatorStashMessage(const QString &keyword = QString())
{
    QString rc = QCoreApplication::applicationName() + ' ';
    if (!keyword.isEmpty())
        rc += keyword + ' ';
    rc += QDateTime::currentDateTime().toString(Qt::ISODate);
    return rc;
}

/* Do a stash and return the message as identifier. Note that stash names (stash{n})
 * shift as they are pushed, so, enforce the use of messages to identify them. Flags:
 * StashPromptDescription: Prompt the user for a description message.
 * StashImmediateRestore: Immediately re-apply this stash (used for snapshots), user keeps on working
 * StashIgnoreUnchanged: Be quiet about unchanged repositories (used for IVersionControl's snapshots). */

QString GitClient::synchronousStash(const FilePath &workingDirectory, const QString &messageKeyword,
                                    unsigned flags, bool *unchanged) const
{
    if (unchanged)
        *unchanged = false;
    QString message;
    bool success = false;
    // Check for changes and stash
    QString errorMessage;
    switch (gitStatus(workingDirectory, StatusMode(NoUntracked | NoSubmodules), nullptr, &errorMessage)) {
    case  StatusChanged: {
        message = creatorStashMessage(messageKeyword);
        do {
            if ((flags & StashPromptDescription)) {
                if (!inputText(ICore::dialogParent(),
                               Tr::tr("Stash Description"), Tr::tr("Description:"), &message))
                    break;
            }
            if (!executeSynchronousStash(workingDirectory, message))
                break;
            if ((flags & StashImmediateRestore)
                && !synchronousStashRestore(workingDirectory, "stash@{0}"))
                break;
            success = true;
        } while (false);
        break;
    }
    case StatusUnchanged:
        if (unchanged)
            *unchanged = true;
        if (!(flags & StashIgnoreUnchanged))
            VcsOutputWindow::appendWarning(msgNoChangedFiles());
        break;
    case StatusFailed:
        VcsOutputWindow::appendError(errorMessage);
        break;
    }
    if (!success)
        message.clear();
    return message;
}

bool GitClient::executeSynchronousStash(const FilePath &workingDirectory,
                                        const QString &message,
                                        bool unstagedOnly,
                                        QString *errorMessage) const
{
    QStringList arguments = {"stash", "save"};
    if (unstagedOnly)
        arguments << "--keep-index";
    if (!message.isEmpty())
        arguments << message;
    const RunFlags flags = RunFlags::ShowStdOut
                         | RunFlags::ExpectRepoChanges
                         | RunFlags::ShowSuccessMessage;
    const CommandResult result = vcsSynchronousExec(workingDirectory, arguments, flags);
    if (result.result() == ProcessResult::FinishedWithSuccess)
        return true;
    msgCannotRun(arguments, workingDirectory, result.cleanedStdErr(), errorMessage);
    return false;
}

// Resolve a stash name from message
bool GitClient::stashNameFromMessage(const FilePath &workingDirectory,
                                     const QString &message, QString *name,
                                     QString *errorMessage) const
{
    // All happy
    if (message.startsWith(stashNamePrefix)) {
        *name = message;
        return true;
    }
    // Retrieve list and find via message
    QList<Stash> stashes;
    if (!synchronousStashList(workingDirectory, &stashes, errorMessage))
        return false;
    for (const Stash &s : std::as_const(stashes)) {
        if (s.message == message) {
            *name = s.name;
            return true;
        }
    }
    //: Look-up of a stash via its descriptive message failed.
    msgCannotRun(Tr::tr("Cannot resolve stash message \"%1\" in \"%2\".")
                 .arg(message, workingDirectory.toUserOutput()), errorMessage);
    return  false;
}

bool GitClient::synchronousBranchCmd(const FilePath &workingDirectory, QStringList branchArgs,
                                     QString *output, QString *errorMessage) const
{
    branchArgs.push_front("branch");
    const CommandResult result = vcsSynchronousExec(workingDirectory, branchArgs);
    *output = result.cleanedStdOut();
    if (result.result() == ProcessResult::FinishedWithSuccess)
        return true;
    msgCannotRun(branchArgs, workingDirectory, result.cleanedStdErr(), errorMessage);
    return false;
}

bool GitClient::synchronousTagCmd(const FilePath &workingDirectory, QStringList tagArgs,
                                  QString *output, QString *errorMessage) const
{
    tagArgs.push_front("tag");
    const CommandResult result = vcsSynchronousExec(workingDirectory, tagArgs);
    *output = result.cleanedStdOut();
    if (result.result() == ProcessResult::FinishedWithSuccess)
        return true;
    msgCannotRun(tagArgs, workingDirectory, result.cleanedStdErr(), errorMessage);
    return false;
}

bool GitClient::synchronousForEachRefCmd(const FilePath &workingDirectory, QStringList args,
                                      QString *output, QString *errorMessage) const
{
    args.push_front("for-each-ref");
    const CommandResult result = vcsSynchronousExec(workingDirectory, args, RunFlags::NoOutput);
    *output = result.cleanedStdOut();
    if (result.result() == ProcessResult::FinishedWithSuccess)
        return true;
    msgCannotRun(args, workingDirectory, result.cleanedStdErr(), errorMessage);
    return false;
}

bool GitClient::synchronousRemoteCmd(const FilePath &workingDirectory, QStringList remoteArgs,
                                     QString *output, QString *errorMessage, bool silent) const
{
    remoteArgs.push_front("remote");
    const CommandResult result = vcsSynchronousExec(workingDirectory, remoteArgs,
                                                    silent ? RunFlags::NoOutput : RunFlags::None);
    const QString stdErr = result.cleanedStdErr();
    *errorMessage = stdErr;
    *output = result.cleanedStdOut();

    if (result.result() == ProcessResult::FinishedWithSuccess)
        return true;
    msgCannotRun(remoteArgs, workingDirectory, stdErr, errorMessage);
    return false;
}

QMap<QString,QString> GitClient::synchronousRemotesList(const FilePath &workingDirectory,
                                                        QString *errorMessage) const
{
    QMap<QString,QString> result;

    QString output;
    QString error;
    if (!synchronousRemoteCmd(workingDirectory, {"-v"}, &output, &error, true)) {
        msgCannotRun(error, errorMessage);
        return result;
    }

    const QStringList remotes = output.split("\n");
    for (const QString &remote : remotes) {
        if (!remote.endsWith(" (push)"))
            continue;

        const int tabIndex = remote.indexOf('\t');
        if (tabIndex == -1)
            continue;
        const QString url = remote.mid(tabIndex + 1, remote.length() - tabIndex - 8);
        result.insert(remote.left(tabIndex), url);
    }
    return result;
}

QStringList GitClient::synchronousSubmoduleStatus(const FilePath &workingDirectory,
                                                  QString *errorMessage) const
{
    // get submodule status
    const CommandResult result = vcsSynchronousExec(workingDirectory, {"submodule", "status"},
                                                    RunFlags::NoOutput);

    if (result.result() != ProcessResult::FinishedWithSuccess) {
        msgCannotRun(Tr::tr("Cannot retrieve submodule status of \"%1\": %2")
                     .arg(workingDirectory.toUserOutput(), result.cleanedStdErr()), errorMessage);
        return {};
    }
    return splitLines(result.cleanedStdOut());
}

SubmoduleDataMap GitClient::submoduleList(const FilePath &workingDirectory) const
{
    SubmoduleDataMap result;
    FilePath gitmodulesFileName = workingDirectory.pathAppended(".gitmodules");
    if (!gitmodulesFileName.exists())
        return result;

    static QMap<FilePath, SubmoduleDataMap> cachedSubmoduleData;

    if (cachedSubmoduleData.contains(workingDirectory))
        return cachedSubmoduleData.value(workingDirectory);

    const QStringList allConfigs = readConfigValue(workingDirectory, "-l").split('\n');
    const QString submoduleLineStart = "submodule.";
    for (const QString &configLine : allConfigs) {
        if (!configLine.startsWith(submoduleLineStart))
            continue;

        const int nameStart = submoduleLineStart.size();
        const int nameEnd   = configLine.indexOf('.', nameStart);

        const QString submoduleName = configLine.mid(nameStart, nameEnd - nameStart);

        SubmoduleData submoduleData;
        if (result.contains(submoduleName))
            submoduleData = result[submoduleName];

        if (configLine.mid(nameEnd, 5) == ".url=")
            submoduleData.url = configLine.mid(nameEnd + 5);
        else if (configLine.mid(nameEnd, 8) == ".ignore=")
            submoduleData.ignore = configLine.mid(nameEnd + 8);
        else
            continue;

        result.insert(submoduleName, submoduleData);
    }

    // if config found submodules
    if (!result.isEmpty()) {
        QSettings gitmodulesFile(gitmodulesFileName.toString(), QSettings::IniFormat);

        const QList<QString> submodules = result.keys();
        for (const QString &submoduleName : submodules) {
            gitmodulesFile.beginGroup("submodule \"" + submoduleName + '"');
            const QString path = gitmodulesFile.value("path").toString();
            if (path.isEmpty()) { // invalid submodule entry in config
                result.remove(submoduleName);
            } else {
                SubmoduleData &submoduleRef = result[submoduleName];
                submoduleRef.dir = path;
                const QString ignore = gitmodulesFile.value("ignore").toString();
                if (!ignore.isEmpty() && submoduleRef.ignore.isEmpty())
                    submoduleRef.ignore = ignore;
            }
            gitmodulesFile.endGroup();
        }
    }
    cachedSubmoduleData.insert(workingDirectory, result);

    return result;
}

QByteArray GitClient::synchronousShow(const FilePath &workingDirectory, const QString &id,
                                      RunFlags flags) const
{
    if (!canShow(id)) {
        VcsOutputWindow::appendError(msgCannotShow(id));
        return {};
    }
    const QStringList arguments = {"show", decorateOption, noColorOption, "--no-patch", id};
    const CommandResult result = vcsSynchronousExec(workingDirectory, arguments, flags);
    if (result.result() != ProcessResult::FinishedWithSuccess) {
        msgCannotRun(arguments, workingDirectory, result.cleanedStdErr(), nullptr);
        return {};
    }
    return result.rawStdOut();
}

// Retrieve list of files to be cleaned
bool GitClient::cleanList(const FilePath &workingDirectory, const QString &modulePath,
                          const QString &flag, QStringList *files, QString *errorMessage)
{
    const FilePath directory = workingDirectory.pathAppended(modulePath);
    const QStringList arguments = {"clean", "--dry-run", flag};

    const CommandResult result = vcsSynchronousExec(directory, arguments, RunFlags::ForceCLocale);
    if (result.result() != ProcessResult::FinishedWithSuccess) {
        msgCannotRun(arguments, directory, result.cleanedStdErr(), errorMessage);
        return false;
    }

    // Filter files that git would remove
    const QString relativeBase = modulePath.isEmpty() ? QString() : modulePath + '/';
    const QString prefix = "Would remove ";
    const QStringList removeLines = Utils::filtered(
                splitLines(result.cleanedStdOut()), [](const QString &s) {
        return s.startsWith("Would remove ");
    });
    *files = Utils::transform(removeLines, [&relativeBase, &prefix](const QString &s) -> QString {
        return relativeBase + s.mid(prefix.size());
    });
    return true;
}

bool GitClient::synchronousCleanList(const FilePath &workingDirectory, const QString &modulePath,
                                     QStringList *files, QStringList *ignoredFiles,
                                     QString *errorMessage)
{
    bool res = cleanList(workingDirectory, modulePath, "-df", files, errorMessage);
    res &= cleanList(workingDirectory, modulePath, "-dXf", ignoredFiles, errorMessage);

    const SubmoduleDataMap submodules = submoduleList(workingDirectory.pathAppended(modulePath));
    for (const SubmoduleData &submodule : submodules) {
        if (submodule.ignore != "all"
                && submodule.ignore != "dirty") {
            const QString submodulePath = modulePath.isEmpty() ? submodule.dir
                                                               : modulePath + '/' + submodule.dir;
            res &= synchronousCleanList(workingDirectory, submodulePath,
                                        files, ignoredFiles, errorMessage);
        }
    }
    return res;
}

bool GitClient::synchronousApplyPatch(const FilePath &workingDirectory,
                                      const QString &file, QString *errorMessage,
                                      const QStringList &extraArguments) const
{
    QStringList arguments = {"apply", "--whitespace=fix"};
    arguments << extraArguments << file;

    const CommandResult result = vcsSynchronousExec(workingDirectory, arguments);
    const QString stdErr = result.cleanedStdErr();
    if (result.result() == ProcessResult::FinishedWithSuccess) {
        if (!stdErr.isEmpty())
            *errorMessage = Tr::tr("There were warnings while applying \"%1\" to \"%2\":\n%3")
                .arg(file, workingDirectory.toUserOutput(), stdErr);
        return true;
    }

    *errorMessage = Tr::tr("Cannot apply patch \"%1\" to \"%2\": %3")
            .arg(QDir::toNativeSeparators(file), workingDirectory.toUserOutput(), stdErr);
    return false;
}

Environment GitClient::processEnvironment() const
{
    Environment environment = VcsBaseClientImpl::processEnvironment();
    environment.prependOrSetPath(settings().path());
    if (HostOsInfo::isWindowsHost() && settings().winSetHomeEnvironment()) {
        QString homePath;
        if (qtcEnvironmentVariableIsEmpty("HOMESHARE")) {
            homePath = QDir::toNativeSeparators(QDir::homePath());
        } else {
            homePath = qtcEnvironmentVariable("HOMEDRIVE") + qtcEnvironmentVariable("HOMEPATH");
        }
        environment.set("HOME", homePath);
    }
    environment.set("GIT_EDITOR", m_disableEditor ? "true" : m_gitQtcEditor);
    return environment;
}

bool GitClient::beginStashScope(const FilePath &workingDirectory, const QString &command,
                                StashFlag flag, PushAction pushAction)
{
    const FilePath repoDirectory = VcsManager::findTopLevelForDirectory(workingDirectory);
    QTC_ASSERT(!repoDirectory.isEmpty(), return false);
    StashInfo &stashInfo = m_stashInfo[repoDirectory];
    return stashInfo.init(repoDirectory, command, flag, pushAction);
}

GitClient::StashInfo &GitClient::stashInfo(const FilePath &workingDirectory)
{
    const FilePath repoDirectory = VcsManager::findTopLevelForDirectory(workingDirectory);
    QTC_CHECK(m_stashInfo.contains(repoDirectory));
    return m_stashInfo[repoDirectory];
}

void GitClient::endStashScope(const FilePath &workingDirectory)
{
    const FilePath repoDirectory = VcsManager::findTopLevelForDirectory(workingDirectory);
    if (!m_stashInfo.contains(repoDirectory))
        return;
    m_stashInfo[repoDirectory].end();
}

bool GitClient::isValidRevision(const QString &revision) const
{
    if (revision.length() < 1)
        return false;
    for (const auto i : revision)
        if (i != '0')
            return true;
    return false;
}

void GitClient::updateSubmodulesIfNeeded(const FilePath &workingDirectory, bool prompt)
{
    if (!m_updatedSubmodules.isEmpty() || submoduleList(workingDirectory).isEmpty())
        return;

    const QStringList submoduleStatus = synchronousSubmoduleStatus(workingDirectory);
    if (submoduleStatus.isEmpty())
        return;

    bool updateNeeded = false;
    for (const QString &status : submoduleStatus) {
        if (status.startsWith('+')) {
            updateNeeded = true;
            break;
        }
    }
    if (!updateNeeded)
        return;

    if (prompt && QMessageBox::question(ICore::dialogParent(), Tr::tr("Submodules Found"),
            Tr::tr("Would you like to update submodules?"),
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
        return;
    }

    for (const QString &statusLine : submoduleStatus) {
        // stash only for lines starting with +
        // because only they would be updated
        if (!statusLine.startsWith('+'))
            continue;

        // get submodule name
        const int nameStart  = statusLine.indexOf(' ', 2) + 1;
        const int nameLength = statusLine.indexOf(' ', nameStart) - nameStart;
        const FilePath submoduleDir = workingDirectory.pathAppended(statusLine.mid(nameStart, nameLength));

        if (beginStashScope(submoduleDir, "SubmoduleUpdate")) {
            m_updatedSubmodules.append(submoduleDir);
        } else {
            finishSubmoduleUpdate();
            return;
        }
    }

    vcsExecWithHandler(workingDirectory, {"submodule", "update"},
                       this, [this](const CommandResult &) { finishSubmoduleUpdate(); },
                       RunFlags::ShowStdOut | RunFlags::ExpectRepoChanges);
}

void GitClient::finishSubmoduleUpdate()
{
    for (const FilePath &submoduleDir : std::as_const(m_updatedSubmodules))
        endStashScope(submoduleDir);
    m_updatedSubmodules.clear();
}

GitClient::StatusResult GitClient::gitStatus(const FilePath &workingDirectory, StatusMode mode,
                                             QString *output, QString *errorMessage) const
{
    // Run 'status'. Note that git returns exitcode 1 if there are no added files.
    QStringList arguments = {"status"};
    if (mode & NoUntracked)
        arguments << "--untracked-files=no";
    else
        arguments << "--untracked-files=all";
    if (mode & NoSubmodules)
        arguments << "--ignore-submodules=all";
    arguments << "--porcelain" << "-b";

    const CommandResult result = vcsSynchronousExec(workingDirectory, arguments, RunFlags::NoOutput);
    const QString stdOut = result.cleanedStdOut();

    if (output)
        *output = stdOut;

    const bool statusRc = result.result() == ProcessResult::FinishedWithSuccess;
    const bool branchKnown = !stdOut.startsWith("## HEAD (no branch)\n");
    // Is it something really fatal?
    if (!statusRc && !branchKnown) {
        if (errorMessage) {
            *errorMessage = Tr::tr("Cannot obtain status: %1").arg(result.cleanedStdErr());
        }
        return StatusFailed;
    }
    // Unchanged (output text depending on whether -u was passed)
    const bool hasChanges = Utils::contains(stdOut.split('\n'), [](const QString &s) {
                                                return !s.isEmpty() && !s.startsWith('#');
                                            });
    return hasChanges ? StatusChanged : StatusUnchanged;
}

QString GitClient::commandInProgressDescription(const FilePath &workingDirectory) const
{
    switch (checkCommandInProgress(workingDirectory)) {
    case NoCommand:
        break;
    case Rebase:
    case RebaseMerge:
        return Tr::tr("REBASING");
    case Revert:
        return Tr::tr("REVERTING");
    case CherryPick:
        return Tr::tr("CHERRY-PICKING");
    case Merge:
        return Tr::tr("MERGING");
    }
    return {};
}

GitClient::CommandInProgress GitClient::checkCommandInProgress(const FilePath &workingDirectory) const
{
    const FilePath gitDir = findGitDirForRepository(workingDirectory);
    if (gitDir.pathAppended("MERGE_HEAD").exists())
        return Merge;
    if (gitDir.pathAppended("rebase-apply").exists())
        return Rebase;
    if (gitDir.pathAppended("rebase-merge").exists())
        return RebaseMerge;
    if (gitDir.pathAppended("REVERT_HEAD").exists())
        return Revert;
    if (gitDir.pathAppended("CHERRY_PICK_HEAD").exists())
        return CherryPick;
    return NoCommand;
}

void GitClient::continueCommandIfNeeded(const FilePath &workingDirectory, bool allowContinue)
{
    if (GitPlugin::isCommitEditorOpen())
        return;
    CommandInProgress command = checkCommandInProgress(workingDirectory);
    ContinueCommandMode continueMode;
    if (allowContinue)
        continueMode = command == RebaseMerge ? ContinueOnly : SkipIfNoChanges;
    else
        continueMode = SkipOnly;
    switch (command) {
    case Rebase:
    case RebaseMerge:
        continuePreviousGitCommand(workingDirectory, Tr::tr("Continue Rebase"),
                                   Tr::tr("Rebase is in progress. What do you want to do?"),
                                   Tr::tr("Continue"), "rebase", continueMode);
        break;
    case Merge:
        continuePreviousGitCommand(workingDirectory, Tr::tr("Continue Merge"),
                Tr::tr("You need to commit changes to finish merge.\nCommit now?"),
                Tr::tr("Commit"), "merge", continueMode);
        break;
    case Revert:
        continuePreviousGitCommand(workingDirectory, Tr::tr("Continue Revert"),
                Tr::tr("You need to commit changes to finish revert.\nCommit now?"),
                Tr::tr("Commit"), "revert", continueMode);
        break;
    case CherryPick:
        continuePreviousGitCommand(workingDirectory, Tr::tr("Continue Cherry-Picking"),
                Tr::tr("You need to commit changes to finish cherry-picking.\nCommit now?"),
                Tr::tr("Commit"), "cherry-pick", continueMode);
        break;
    default:
        break;
    }
}

void GitClient::continuePreviousGitCommand(const FilePath &workingDirectory,
                                           const QString &msgBoxTitle, QString msgBoxText,
                                           const QString &buttonName, const QString &gitCommand,
                                           ContinueCommandMode continueMode)
{
    bool isRebase = gitCommand == "rebase";
    bool hasChanges = false;
    switch (continueMode) {
    case ContinueOnly:
        hasChanges = true;
        break;
    case SkipIfNoChanges:
        hasChanges = gitStatus(workingDirectory, StatusMode(NoUntracked | NoSubmodules))
            == GitClient::StatusChanged;
        if (!hasChanges)
            msgBoxText.prepend(Tr::tr("No changes found.") + ' ');
        break;
    case SkipOnly:
        hasChanges = false;
        break;
    }

    QMessageBox msgBox(QMessageBox::Question, msgBoxTitle, msgBoxText,
                       QMessageBox::NoButton, ICore::dialogParent());
    if (hasChanges || isRebase)
        msgBox.addButton(hasChanges ? buttonName : Tr::tr("Skip"), QMessageBox::AcceptRole);
    msgBox.addButton(QMessageBox::Abort);
    msgBox.addButton(QMessageBox::Ignore);
    switch (msgBox.exec()) {
    case QMessageBox::Ignore:
        break;
    case QMessageBox::Abort:
        synchronousAbortCommand(workingDirectory, gitCommand);
        break;
    default: // Continue/Skip
        if (isRebase)
            rebase(workingDirectory, QLatin1String(hasChanges ? "--continue" : "--skip"));
        else
            GitPlugin::startCommit();
    }
}

// Quietly retrieve branch list of remote repository URL
//
// The branch HEAD is pointing to is always returned first.
QStringList GitClient::synchronousRepositoryBranches(const QString &repositoryURL,
                                                     const FilePath &workingDirectory) const
{
    const CommandResult result = vcsSynchronousExec(workingDirectory,
                                 {"ls-remote", repositoryURL, HEAD, "refs/heads/*"},
                                 RunFlags::SuppressStdErr | RunFlags::SuppressFailMessage);
    QStringList branches;
    branches << Tr::tr("<Detached HEAD>");
    QString headSha;
    // split "82bfad2f51d34e98b18982211c82220b8db049b<tab>refs/heads/master"
    bool headFound = false;
    bool branchFound = false;
    const QStringList lines = result.cleanedStdOut().split('\n');
    for (const QString &line : lines) {
        if (line.endsWith("\tHEAD")) {
            QTC_CHECK(headSha.isNull());
            headSha = line.left(line.indexOf('\t'));
            continue;
        }

        const QString pattern = "\trefs/heads/";
        const int pos = line.lastIndexOf(pattern);
        if (pos != -1) {
            branchFound = true;
            const QString branchName = line.mid(pos + pattern.size());
            if (!headFound && line.startsWith(headSha)) {
                branches[0] = branchName;
                headFound = true;
            } else {
                branches.push_back(branchName);
            }
        }
    }
    if (!branchFound)
        branches.clear();
    return branches;
}

void GitClient::launchGitK(const FilePath &workingDirectory, const QString &fileName) const
{
    tryLaunchingGitK(processEnvironment(), workingDirectory, fileName);
}

void GitClient::launchRepositoryBrowser(const FilePath &workingDirectory) const
{
    const FilePath repBrowserBinary = settings().repositoryBrowserCmd();
    if (!repBrowserBinary.isEmpty())
        Process::startDetached({repBrowserBinary, {workingDirectory.toString()}}, workingDirectory);
}

static FilePath gitBinDir(const GitClient::GitKLaunchTrial trial, const FilePath &parentDir)
{
    if (trial == GitClient::Bin)
        return parentDir;
    if (trial == GitClient::ParentOfBin) {
        QTC_CHECK(parentDir.fileName() == "bin");
        FilePath foundBinDir = parentDir.parentDir();
        const QString binDirName = foundBinDir.fileName();
        if (binDirName == "usr" || binDirName.startsWith("mingw"))
            foundBinDir = foundBinDir.parentDir();
        return foundBinDir / "cmd";
    }
    if (trial == GitClient::SystemPath)
        return Environment::systemEnvironment().searchInPath("gitk").parentDir();
    QTC_CHECK(false);
    return {};
}

void GitClient::tryLaunchingGitK(const Environment &env,
                                 const FilePath &workingDirectory,
                                 const QString &fileName,
                                 GitClient::GitKLaunchTrial trial) const
{
    const FilePath gitBinDirectory = gitBinDir(trial, vcsBinary().parentDir());
    FilePath binary = gitBinDirectory.pathAppended("gitk").withExecutableSuffix();
    QStringList arguments;
    if (HostOsInfo::isWindowsHost()) {
        // If git/bin is in path, use 'wish' shell to run. Otherwise (git/cmd), directly run gitk
        const FilePath wish = gitBinDirectory.pathAppended("wish").withExecutableSuffix();
        if (wish.withExecutableSuffix().exists()) {
            arguments << binary.toString();
            binary = wish;
        }
    }
    const QString gitkOpts = settings().gitkOptions();
    if (!gitkOpts.isEmpty())
        arguments.append(ProcessArgs::splitArgs(gitkOpts, HostOsInfo::hostOs()));
    if (!fileName.isEmpty())
        arguments << "--" << fileName;
    VcsOutputWindow::appendCommand(workingDirectory, {binary, arguments});

    // This should always use Process::startDetached (as not to kill
    // the child), but that does not have an environment parameter.
    if (!settings().path().isEmpty()) {
        auto process = new Process(const_cast<GitClient*>(this));
        process->setWorkingDirectory(workingDirectory);
        process->setEnvironment(env);
        process->setCommand({binary, arguments});
        connect(process, &Process::done, this, [=] {
            if (process->result() == ProcessResult::StartFailed)
                handleGitKFailedToStart(env, workingDirectory, fileName, trial, gitBinDirectory);
            process->deleteLater();
        });
        process->start();
    } else {
        if (!Process::startDetached({binary, arguments}, workingDirectory))
            handleGitKFailedToStart(env, workingDirectory, fileName, trial, gitBinDirectory);
    }
}

void GitClient::handleGitKFailedToStart(const Environment &env,
                                        const FilePath &workingDirectory,
                                        const QString &fileName,
                                        const GitClient::GitKLaunchTrial oldTrial,
                                        const FilePath &oldGitBinDir) const
{
    QTC_ASSERT(oldTrial != None, return);
    VcsOutputWindow::appendSilently(msgCannotLaunch(oldGitBinDir / "gitk"));

    GitKLaunchTrial nextTrial = None;

    if (oldTrial == Bin && vcsBinary().parentDir().fileName() == "bin") {
        nextTrial = ParentOfBin;
    } else if (oldTrial != SystemPath
               && !Environment::systemEnvironment().searchInPath("gitk").isEmpty()) {
        nextTrial = SystemPath;
    }

    if (nextTrial == None) {
        VcsOutputWindow::appendError(msgCannotLaunch("gitk"));
        return;
    }

    tryLaunchingGitK(env, workingDirectory, fileName, nextTrial);
}

bool GitClient::launchGitGui(const FilePath &workingDirectory) {
    bool success = true;
    FilePath gitBinary = vcsBinary();
    if (gitBinary.isEmpty()) {
        success = false;
    } else {
        success = Process::startDetached({gitBinary, {"gui"}}, workingDirectory);
    }

    if (!success)
        VcsOutputWindow::appendError(msgCannotLaunch("git gui"));

    return success;
}

FilePath GitClient::gitBinDirectory() const
{
    const QString git = vcsBinary().toString();
    if (git.isEmpty())
        return {};

    // Is 'git\cmd' in the path (folder containing .bats)?
    QString path = QFileInfo(git).absolutePath();
    // Git for Windows has git and gitk redirect executables in {setup dir}/cmd
    // and the real binaries are in {setup dir}/bin. If cmd is configured in PATH
    // or in Git settings, return bin instead.
    if (HostOsInfo::isWindowsHost()) {
        if (path.endsWith("/cmd", Qt::CaseInsensitive))
            path.replace(path.size() - 3, 3, "bin");
        if (path.endsWith("/bin", Qt::CaseInsensitive)
                && !path.endsWith("/usr/bin", Qt::CaseInsensitive)) {
            // Legacy msysGit used Git/bin for additional tools.
            // Git for Windows uses Git/usr/bin. Prefer that if it exists.
            QString usrBinPath = path;
            usrBinPath.replace(usrBinPath.size() - 3, 3, "usr/bin");
            if (QFileInfo::exists(usrBinPath))
                path = usrBinPath;
        }
    }
    return FilePath::fromString(path);
}

bool GitClient::launchGitBash(const FilePath &workingDirectory)
{
    bool success = true;
    const FilePath git = vcsBinary();

    if (git.isEmpty()) {
        success = false;
    } else {
        const FilePath gitBash = git.absolutePath().parentDir() / "git-bash.exe";
        success = Process::startDetached({gitBash, {}}, workingDirectory);
    }

    if (!success)
        VcsOutputWindow::appendError(msgCannotLaunch("git-bash"));

    return success;
}

FilePath GitClient::vcsBinary() const
{
    bool ok;
    Utils::FilePath binary = settings().gitExecutable(&ok);
    if (!ok)
        return {};
    return binary;
}

// returns first line from log and removes it
static QByteArray shiftLogLine(QByteArray &logText)
{
    const int index = logText.indexOf('\n');
    const QByteArray res = logText.left(index);
    logText.remove(0, index + 1);
    return res;
}

bool GitClient::readDataFromCommit(const FilePath &repoDirectory, const QString &commit,
                                   CommitData &commitData, QString *errorMessage,
                                   QString *commitTemplate)
{
    // Get commit data as "SHA1<lf>author<lf>email<lf>message".
    const QStringList arguments = {"log", "--max-count=1", "--pretty=format:%h\n%an\n%ae\n%B", commit};
    const CommandResult result = vcsSynchronousExec(repoDirectory, arguments, RunFlags::NoOutput);

    if (result.result() != ProcessResult::FinishedWithSuccess) {
        if (errorMessage) {
            *errorMessage = Tr::tr("Cannot retrieve last commit data of repository \"%1\".")
                .arg(repoDirectory.toUserOutput());
        }
        return false;
    }

    QTextCodec *authorCodec = HostOsInfo::isWindowsHost()
            ? QTextCodec::codecForName("UTF-8")
            : commitData.commitEncoding;
    QByteArray stdOut = result.rawStdOut();
    commitData.amendSHA1 = QLatin1String(shiftLogLine(stdOut));
    commitData.panelData.author = authorCodec->toUnicode(shiftLogLine(stdOut));
    commitData.panelData.email = authorCodec->toUnicode(shiftLogLine(stdOut));
    if (commitTemplate)
        *commitTemplate = commitData.commitEncoding->toUnicode(stdOut);
    return true;
}

Author GitClient::parseAuthor(const QString &authorInfo)
{
    // The format is:
    // Joe Developer <joedev@example.com> unixtimestamp +HHMM
    int lt = authorInfo.lastIndexOf('<');
    int gt = authorInfo.lastIndexOf('>');
    if (gt == -1 || uint(lt) > uint(gt)) {
        // shouldn't happen!
        return {};
    }

    const Author result {authorInfo.left(lt - 1), authorInfo.mid(lt + 1, gt - lt - 1)};
    return result;
}

Author GitClient::getAuthor(const Utils::FilePath &workingDirectory)
{
    const QString authorInfo = readGitVar(workingDirectory, "GIT_AUTHOR_IDENT");
    return parseAuthor(authorInfo);
}

bool GitClient::getCommitData(const FilePath &workingDirectory,
                              QString *commitTemplate,
                              CommitData &commitData,
                              QString *errorMessage)
{
    commitData.clear();

    // Find repo
    const FilePath repoDirectory = VcsManager::findTopLevelForDirectory(workingDirectory);
    if (repoDirectory.isEmpty()) {
        *errorMessage = msgRepositoryNotFound(workingDirectory);
        return false;
    }

    commitData.panelInfo.repository = repoDirectory;

    const FilePath gitDir = findGitDirForRepository(repoDirectory);
    if (gitDir.isEmpty()) {
        *errorMessage = Tr::tr("The repository \"%1\" is not initialized.")
            .arg(repoDirectory.toUserOutput());
        return false;
    }

    // Run status. Note that it has exitcode 1 if there are no added files.
    QString output;
    if (commitData.commitType == FixupCommit) {
        synchronousLog(repoDirectory, {HEAD, "--not", "--remotes", "-n1"}, &output, errorMessage,
                       RunFlags::SuppressCommandLogging);
        if (output.isEmpty()) {
            *errorMessage = msgNoCommits(false);
            return false;
        }
    } else {
        commitData.commentChar = commentChar(repoDirectory);
    }
    const StatusResult status = gitStatus(repoDirectory, ShowAll, &output, errorMessage);
    switch (status) {
    case  StatusChanged:
        break;
    case StatusUnchanged:
        if (commitData.commitType == AmendCommit) // amend might be run just for the commit message
            break;
        *errorMessage = msgNoChangedFiles();
        return false;
    case StatusFailed:
        return false;
    }

    //    Output looks like:
    //    ## branch_name
    //    MM filename
    //     A new_unstaged_file
    //    R  old -> new
    //     D deleted_file
    //    ?? untracked_file
    if (status != StatusUnchanged) {
        if (!commitData.parseFilesFromStatus(output)) {
            *errorMessage = msgParseFilesFailed();
            return false;
        }

        // Filter out untracked files that are not part of the project
        QStringList untrackedFiles = commitData.filterFiles(UntrackedFile);

        VcsBaseSubmitEditor::filterUntrackedFilesOfProject(repoDirectory, &untrackedFiles);
        QList<CommitData::StateFilePair> filteredFiles;
        QList<CommitData::StateFilePair>::const_iterator it = commitData.files.constBegin();
        for ( ; it != commitData.files.constEnd(); ++it) {
            if (it->first == UntrackedFile && !untrackedFiles.contains(it->second))
                continue;
            filteredFiles.append(*it);
        }
        commitData.files = filteredFiles;

        if (commitData.files.isEmpty() && commitData.commitType != AmendCommit) {
            *errorMessage = msgNoChangedFiles();
            return false;
        }
    }

    commitData.commitEncoding = encoding(EncodingCommit, workingDirectory);

    // Get the commit template or the last commit message
    switch (commitData.commitType) {
    case AmendCommit: {
        if (!readDataFromCommit(repoDirectory, HEAD, commitData, errorMessage, commitTemplate))
            return false;
        break;
    }
    case SimpleCommit: {
        bool authorFromCherryPick = false;
        // For cherry-picked commit, read author data from the commit (but template from MERGE_MSG)
        if (gitDir.pathAppended(CHERRY_PICK_HEAD).exists()) {
            authorFromCherryPick = readDataFromCommit(repoDirectory, CHERRY_PICK_HEAD, commitData);
            commitData.amendSHA1.clear();
        }
        if (!authorFromCherryPick) {
            const Author author = getAuthor(workingDirectory);
            commitData.panelData.author = author.name;
            commitData.panelData.email = author.email;
        }
        // Commit: Get the commit template
        FilePath templateFile = gitDir / "MERGE_MSG";
        if (!templateFile.exists())
            templateFile = gitDir / "SQUASH_MSG";
        if (!templateFile.exists()) {
            templateFile = FilePath::fromUserInput(
                readConfigValue(workingDirectory, "commit.template"));
        }
        if (!templateFile.isEmpty()) {
            templateFile = repoDirectory.resolvePath(templateFile);
            FileReader reader;
            if (!reader.fetch(templateFile, QIODevice::Text, errorMessage))
                return false;
            *commitTemplate = QString::fromLocal8Bit(reader.data());
        }
        break;
    }
    case FixupCommit:
        break;
    }

    commitData.enablePush = !synchronousRemotesList(repoDirectory).isEmpty();
    if (commitData.enablePush) {
        CommandInProgress commandInProgress = checkCommandInProgress(repoDirectory);
        if (commandInProgress == Rebase || commandInProgress == RebaseMerge)
            commitData.enablePush = false;
    }

    return true;
}

// Log message for commits/amended commits to go to output window
static inline QString msgCommitted(const QString &amendSHA1, int fileCount)
{
    if (amendSHA1.isEmpty())
        return Tr::tr("Committed %n files.", nullptr, fileCount);
    if (fileCount)
        return Tr::tr("Amended \"%1\" (%n files).", nullptr, fileCount).arg(amendSHA1);
    return Tr::tr("Amended \"%1\".").arg(amendSHA1);
}

bool GitClient::addAndCommit(const FilePath &repositoryDirectory,
                             const GitSubmitEditorPanelData &data,
                             CommitType commitType,
                             const QString &amendSHA1,
                             const QString &messageFile,
                             SubmitFileModel *model)
{
    const QString renameSeparator = " -> ";

    QStringList filesToAdd;
    QStringList filesToRemove;
    QStringList filesToReset;

    int commitCount = 0;

    for (int i = 0; i < model->rowCount(); ++i) {
        const FileStates state = static_cast<FileStates>(model->extraData(i).toInt());
        const QString file = model->file(i);
        const bool checked = model->checked(i);

        if (checked)
            ++commitCount;

        if (state == UntrackedFile && checked)
            filesToAdd.append(file);

        if ((state & StagedFile) && !checked) {
            if (state & (ModifiedFile | AddedFile | DeletedFile | TypeChangedFile)) {
                filesToReset.append(file);
            } else if (state & (RenamedFile | CopiedFile)) {
                const QString newFile = file.mid(file.indexOf(renameSeparator) + renameSeparator.size());
                filesToReset.append(newFile);
            }
        } else if (state & UnmergedFile && checked) {
            QTC_ASSERT(false, continue); // There should not be unmerged files when committing!
        }

        if ((state == ModifiedFile || state == TypeChangedFile) && checked) {
            filesToReset.removeAll(file);
            filesToAdd.append(file);
        } else if (state == AddedFile && checked) {
            filesToAdd.append(file);
        } else if (state == DeletedFile && checked) {
            filesToReset.removeAll(file);
            filesToRemove.append(file);
        } else if (state == RenamedFile && checked) {
            QTC_ASSERT(false, continue); // git mv directly stages.
        } else if (state == CopiedFile && checked) {
            QTC_ASSERT(false, continue); // only is noticed after adding a new file to the index
        } else if (state == UnmergedFile && checked) {
            QTC_ASSERT(false, continue); // There should not be unmerged files when committing!
        }
    }

    if (!filesToReset.isEmpty() && !synchronousReset(repositoryDirectory, filesToReset))
        return false;

    if (!filesToRemove.isEmpty() && !synchronousDelete(repositoryDirectory, true, filesToRemove))
        return false;

    if (!filesToAdd.isEmpty() && !synchronousAdd(repositoryDirectory, filesToAdd))
        return false;

    // Do the final commit
    QStringList arguments = {"commit"};
    if (commitType == FixupCommit) {
        arguments << "--fixup" << amendSHA1;
    } else {
        arguments << "-F" << QDir::toNativeSeparators(messageFile);
        if (commitType == AmendCommit)
            arguments << "--amend";
        const QString &authorString =  data.authorString();
        if (!authorString.isEmpty())
             arguments << "--author" << authorString;
        if (data.bypassHooks)
            arguments << "--no-verify";
        if (data.signOff)
            arguments << "--signoff";
    }

    const CommandResult result = vcsSynchronousExec(repositoryDirectory, arguments,
                                                    RunFlags::UseEventLoop);
    if (result.result() == ProcessResult::FinishedWithSuccess) {
        VcsOutputWindow::appendMessage(msgCommitted(amendSHA1, commitCount));
        GitPlugin::updateCurrentBranch();
        return true;
    }
    VcsOutputWindow::appendError(Tr::tr("Cannot commit %n file(s).", nullptr, commitCount) + "\n");
    return false;
}

/* Revert: This function can be called with a file list (to revert single
 * files)  or a single directory (revert all). Qt Creator currently has only
 * 'revert single' in its VCS menus, but the code is prepared to deal with
 * reverting a directory pending a sophisticated selection dialog in the
 * VcsBase plugin. */
GitClient::RevertResult GitClient::revertI(QStringList files,
                                           bool *ptrToIsDirectory,
                                           QString *errorMessage,
                                           bool revertStaging)
{
    if (files.empty())
        return RevertCanceled;

    // Figure out the working directory
    const QFileInfo firstFile(files.front());
    const bool isDirectory = firstFile.isDir();
    if (ptrToIsDirectory)
        *ptrToIsDirectory = isDirectory;
    const FilePath workingDirectory =
        FilePath::fromString(isDirectory ? firstFile.absoluteFilePath() : firstFile.absolutePath());

    const FilePath repoDirectory = VcsManager::findTopLevelForDirectory(workingDirectory);
    if (repoDirectory.isEmpty()) {
        *errorMessage = msgRepositoryNotFound(workingDirectory);
        return RevertFailed;
    }

    // Check for changes
    QString output;
    switch (gitStatus(repoDirectory, StatusMode(NoUntracked | NoSubmodules), &output, errorMessage)) {
    case StatusChanged:
        break;
    case StatusUnchanged:
        return RevertUnchanged;
    case StatusFailed:
        return RevertFailed;
    }
    CommitData data;
    if (!data.parseFilesFromStatus(output)) {
        *errorMessage = msgParseFilesFailed();
        return RevertFailed;
    }

    // If we are looking at files, make them relative to the repository
    // directory to match them in the status output list.
    if (!isDirectory) {
        const QDir repoDir(repoDirectory.toString());
        const QStringList::iterator cend = files.end();
        for (QStringList::iterator it = files.begin(); it != cend; ++it)
            *it = repoDir.relativeFilePath(*it);
    }

    // From the status output, determine all modified [un]staged files.
    const QStringList allStagedFiles = data.filterFiles(StagedFile | ModifiedFile);
    const QStringList allUnstagedFiles = data.filterFiles(ModifiedFile);
    // Unless a directory was passed, filter all modified files for the
    // argument file list.
    QStringList stagedFiles = allStagedFiles;
    QStringList unstagedFiles = allUnstagedFiles;
    if (!isDirectory) {
        const QSet<QString> filesSet = Utils::toSet(files);
        stagedFiles = Utils::toList(Utils::toSet(allStagedFiles).intersect(filesSet));
        unstagedFiles = Utils::toList(Utils::toSet(allUnstagedFiles).intersect(filesSet));
    }
    if ((!revertStaging || stagedFiles.empty()) && unstagedFiles.empty())
        return RevertUnchanged;

    // Ask to revert (to do: Handle lists with a selection dialog)
    const QMessageBox::StandardButton answer
        = QMessageBox::question(ICore::dialogParent(),
                                Tr::tr("Revert"),
                                Tr::tr("The file has been changed. Do you want to revert it?"),
                                QMessageBox::Yes | QMessageBox::No,
                                QMessageBox::No);
    if (answer == QMessageBox::No)
        return RevertCanceled;

    // Unstage the staged files
    if (revertStaging && !stagedFiles.empty() && !synchronousReset(repoDirectory, stagedFiles, errorMessage))
        return RevertFailed;
    QStringList filesToRevert = unstagedFiles;
    if (revertStaging)
        filesToRevert += stagedFiles;
    // Finally revert!
    if (!synchronousCheckoutFiles(repoDirectory, filesToRevert, QString(), errorMessage, revertStaging))
        return RevertFailed;
    return RevertOk;
}

void GitClient::revertFiles(const QStringList &files, bool revertStaging)
{
    bool isDirectory;
    QString errorMessage;
    switch (revertI(files, &isDirectory, &errorMessage, revertStaging)) {
    case RevertOk:
        GitPlugin::emitFilesChanged(files);
        break;
    case RevertCanceled:
        break;
    case RevertUnchanged: {
        const QString msg = (isDirectory || files.size() > 1) ? msgNoChangedFiles() : Tr::tr("The file is not modified.");
        VcsOutputWindow::appendWarning(msg);
    }
        break;
    case RevertFailed:
        VcsOutputWindow::appendError(errorMessage);
        break;
    }
}

void GitClient::fetch(const FilePath &workingDirectory, const QString &remote)
{
    const QStringList arguments{"fetch", (remote.isEmpty() ? "--all" : remote)};
    const auto commandHandler = [workingDirectory](const CommandResult &result) {
        if (result.result() == ProcessResult::FinishedWithSuccess)
            GitPlugin::updateBranches(workingDirectory);
    };
    vcsExecWithHandler(workingDirectory, arguments, this, commandHandler,
                       RunFlags::ShowStdOut | RunFlags::ShowSuccessMessage);
}

bool GitClient::executeAndHandleConflicts(const FilePath &workingDirectory,
                                          const QStringList &arguments,
                                          const QString &abortCommand) const
{
    const RunFlags flags = RunFlags::ShowStdOut
                         | RunFlags::ExpectRepoChanges
                         | RunFlags::ShowSuccessMessage;
    const CommandResult result = vcsSynchronousExec(workingDirectory, arguments, flags);
    // Notify about changed files or abort the rebase.
    handleConflictResponse(result, workingDirectory, abortCommand);
    return result.result() == ProcessResult::FinishedWithSuccess;
}

void GitClient::pull(const FilePath &workingDirectory, bool rebase)
{
    QString abortCommand;
    QStringList arguments = {"pull"};
    if (rebase) {
        arguments << "--rebase";
        abortCommand = "rebase";
    } else {
        abortCommand = "merge";
    }

    const auto commandHandler = [this, workingDirectory](const CommandResult &result) {
        if (result.result() == ProcessResult::FinishedWithSuccess)
            updateSubmodulesIfNeeded(workingDirectory, true);
    };
    vcsExecAbortable(workingDirectory, arguments, rebase, abortCommand, this, commandHandler);
}

void GitClient::synchronousAbortCommand(const FilePath &workingDir, const QString &abortCommand)
{
    // Abort to clean if something goes wrong
    if (abortCommand.isEmpty()) {
        // no abort command - checkout index to clean working copy.
        synchronousCheckoutFiles(VcsManager::findTopLevelForDirectory(workingDir),
                                 {}, {}, nullptr, false);
        return;
    }

    const CommandResult result = vcsSynchronousExec(workingDir, {abortCommand, "--abort"},
                                 RunFlags::ExpectRepoChanges | RunFlags::ShowSuccessMessage);
    VcsOutputWindow::append(result.cleanedStdOut());
}

QString GitClient::synchronousTrackingBranch(const FilePath &workingDirectory, const QString &branch)
{
    QString remote;
    QString localBranch = branch.isEmpty() ? synchronousCurrentLocalBranch(workingDirectory) : branch;
    if (localBranch.isEmpty())
        return {};
    localBranch.prepend("branch.");
    remote = readConfigValue(workingDirectory, localBranch + ".remote");
    if (remote.isEmpty())
        return {};
    const QString rBranch = readConfigValue(workingDirectory, localBranch + ".merge")
            .replace("refs/heads/", QString());
    if (rBranch.isEmpty())
        return {};
    return remote + '/' + rBranch;
}

bool GitClient::synchronousSetTrackingBranch(const FilePath &workingDirectory,
                                             const QString &branch, const QString &tracking)
{
    const CommandResult result = vcsSynchronousExec(workingDirectory,
                                 {"branch", "--set-upstream-to=" + tracking, branch});
    return result.result() == ProcessResult::FinishedWithSuccess;
}

void GitClient::handleMergeConflicts(const FilePath &workingDir, const QString &commit,
                                     const QStringList &files, const QString &abortCommand)
{
    QString message;
    if (!commit.isEmpty()) {
        message = Tr::tr("Conflicts detected with commit %1.").arg(commit);
    } else if (!files.isEmpty()) {
        QString fileList;
        QStringList partialFiles = files;
        while (partialFiles.count() > 20)
            partialFiles.removeLast();
        fileList = partialFiles.join('\n');
        if (partialFiles.count() != files.count())
            fileList += "\n...";
        message = Tr::tr("Conflicts detected with files:\n%1").arg(fileList);
    } else {
        message = Tr::tr("Conflicts detected.");
    }
    QMessageBox mergeOrAbort(QMessageBox::Question, Tr::tr("Conflicts Detected"), message,
                             QMessageBox::NoButton, ICore::dialogParent());
    QPushButton *mergeToolButton = mergeOrAbort.addButton(Tr::tr("Run &Merge Tool"),
                                                          QMessageBox::AcceptRole);
    const QString mergeTool = readConfigValue(workingDir, "merge.tool");
    if (mergeTool.isEmpty() || mergeTool.startsWith("vimdiff")) {
        mergeToolButton->setEnabled(false);
        mergeToolButton->setToolTip(Tr::tr("Only graphical merge tools are supported. "
                                       "Please configure merge.tool."));
    }
    mergeOrAbort.addButton(QMessageBox::Ignore);
    if (abortCommand == "rebase")
        mergeOrAbort.addButton(Tr::tr("&Skip"), QMessageBox::RejectRole);
    if (!abortCommand.isEmpty())
        mergeOrAbort.addButton(QMessageBox::Abort);
    switch (mergeOrAbort.exec()) {
    case QMessageBox::Abort:
        synchronousAbortCommand(workingDir, abortCommand);
        break;
    case QMessageBox::Ignore:
        break;
    default: // Merge or Skip
        if (mergeOrAbort.clickedButton() == mergeToolButton)
            merge(workingDir);
        else if (!abortCommand.isEmpty())
            executeAndHandleConflicts(workingDir, {abortCommand, "--skip"}, abortCommand);
    }
}

// Subversion: git svn
void GitClient::synchronousSubversionFetch(const FilePath &workingDirectory) const
{
    vcsSynchronousExec(workingDirectory, {"svn", "fetch"},
                       RunFlags::ShowStdOut | RunFlags::ShowSuccessMessage);
}

void GitClient::subversionLog(const FilePath &workingDirectory) const
{
    QStringList arguments = {"svn", "log"};
    int logCount = settings().logCount();
    if (logCount > 0)
         arguments << ("--limit=" + QString::number(logCount));

    // Create a command editor, no highlighting or interaction.
    const QString title = Tr::tr("Git SVN Log");
    const Id editorId = Git::Constants::GIT_SVN_LOG_EDITOR_ID;
    const FilePath sourceFile = VcsBaseEditor::getSource(workingDirectory, QStringList());
    VcsBaseEditorWidget *editor = createVcsEditor(editorId, title, sourceFile, encoding(EncodingDefault),
                                                  "svnLog", sourceFile.toString());
    editor->setWorkingDirectory(workingDirectory);
    vcsExecWithEditor(workingDirectory, arguments, editor);
}

void GitClient::subversionDeltaCommit(const FilePath &workingDirectory) const
{
    vcsExec(workingDirectory, {"svn", "dcommit"}, RunFlags::ShowStdOut | RunFlags::ShowSuccessMessage);
}

enum class PushFailure { Unknown, NonFastForward, NoRemoteBranch };

static PushFailure handleError(const QString &text, QString *pushFallbackCommand)
{
    if (text.contains("non-fast-forward"))
        return PushFailure::NonFastForward;

    if (text.contains("has no upstream branch")) {
        const QStringList lines = text.split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            /* Extract the suggested command from the git output which
                 * should be similar to the following:
                 *
                 *     git push --set-upstream origin add_set_upstream_dialog
                 */
            const QString trimmedLine = line.trimmed();
            if (trimmedLine.startsWith("git push")) {
                *pushFallbackCommand = trimmedLine;
                break;
            }
        }
        return PushFailure::NoRemoteBranch;
    }
    return PushFailure::Unknown;
};

void GitClient::push(const FilePath &workingDirectory, const QStringList &pushArgs)
{
    const auto commandHandler = [=](const CommandResult &result) {
        QString pushFallbackCommand;
        const PushFailure pushFailure = handleError(result.cleanedStdErr(),
                                                    &pushFallbackCommand);
        if (result.result() == ProcessResult::FinishedWithSuccess) {
            GitPlugin::updateCurrentBranch();
            return;
        }
        if (pushFailure == PushFailure::Unknown)
            return;

        if (pushFailure == PushFailure::NonFastForward) {
            const QColor warnColor = Utils::creatorTheme()->color(Theme::TextColorError);
            if (QMessageBox::question(
                    Core::ICore::dialogParent(), Tr::tr("Force Push"),
                    Tr::tr("Push failed. Would you like to force-push <span style=\"color:#%1\">"
                           "(rewrites remote history)</span>?")
                        .arg(QString::number(warnColor.rgba(), 16)),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No) != QMessageBox::Yes) {
                return;
            }
            const auto commandHandler = [](const CommandResult &result) {
                if (result.result() == ProcessResult::FinishedWithSuccess)
                    GitPlugin::updateCurrentBranch();
            };
            vcsExecWithHandler(workingDirectory,
                               QStringList{"push", "--force-with-lease"} + pushArgs,
                               this, commandHandler,
                               RunFlags::ShowStdOut | RunFlags::ShowSuccessMessage);
            return;
        }
        // NoRemoteBranch case
        if (QMessageBox::question(
                Core::ICore::dialogParent(), Tr::tr("No Upstream Branch"),
                Tr::tr("Push failed because the local branch \"%1\" "
                       "does not have an upstream branch on the remote.\n\n"
                       "Would you like to create the branch \"%1\" on the "
                       "remote and set it as upstream?")
                    .arg(synchronousCurrentLocalBranch(workingDirectory)),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
            return;
        }
        const QStringList fallbackCommandParts = pushFallbackCommand.split(' ', Qt::SkipEmptyParts);
        const auto commandHandler = [workingDirectory](const CommandResult &result) {
            if (result.result() == ProcessResult::FinishedWithSuccess)
                GitPlugin::updateBranches(workingDirectory);
        };
        vcsExecWithHandler(workingDirectory, fallbackCommandParts.mid(1),
                           this, commandHandler,
                           RunFlags::ShowStdOut | RunFlags::ShowSuccessMessage);
    };
    vcsExecWithHandler(workingDirectory, QStringList({"push"}) + pushArgs, this, commandHandler,
                       RunFlags::ShowStdOut | RunFlags::ShowSuccessMessage);
}

bool GitClient::synchronousMerge(const FilePath &workingDirectory, const QString &branch,
                                 bool allowFastForward)
{
    const QString command = "merge";
    QStringList arguments = {command};
    if (!allowFastForward)
        arguments << "--no-ff";
    arguments << branch;
    return executeAndHandleConflicts(workingDirectory, arguments, command);
}

bool GitClient::canRebase(const FilePath &workingDirectory) const
{
    const FilePath gitDir = findGitDirForRepository(workingDirectory);
    if (gitDir.pathAppended("rebase-apply").exists()
            || gitDir.pathAppended("rebase-merge").exists()) {
        VcsOutputWindow::appendError(
                    Tr::tr("Rebase, merge or am is in progress. Finish "
                       "or abort it and then try again."));
        return false;
    }
    return true;
}

void GitClient::rebase(const FilePath &workingDirectory, const QString &argument)
{
    vcsExecAbortable(workingDirectory, {"rebase", argument}, true);
}

void GitClient::cherryPick(const FilePath &workingDirectory, const QString &argument)
{
    vcsExecAbortable(workingDirectory, {"cherry-pick", argument});
}

void GitClient::revert(const FilePath &workingDirectory, const QString &argument)
{
    vcsExecAbortable(workingDirectory, {"revert", argument});
}

// Executes a command asynchronously. Work tree is expected to be clean.
// Stashing is handled prior to this call.
void GitClient::vcsExecAbortable(const FilePath &workingDirectory, const QStringList &arguments,
                                 bool isRebase, const QString &abortCommand,
                                 const QObject *context, const CommandHandler &handler)
{
    QTC_ASSERT(!arguments.isEmpty(), return);
    const QString abortString = abortCommand.isEmpty() ? arguments.at(0) : abortCommand;
    VcsCommand *command = createCommand(workingDirectory);
    command->addFlags(RunFlags::ShowStdOut | RunFlags::ShowSuccessMessage);
    // For rebase, Git might request an editor (which means the process keeps running until the
    // user closes it), so run without timeout.
    command->addJob({vcsBinary(), arguments}, isRebase ? 0 : vcsTimeoutS());
    const QObject *actualContext = context ? context : this;
    connect(command, &VcsCommand::done, actualContext, [=] {
        const CommandResult result = CommandResult(*command);
        handleConflictResponse(result, workingDirectory, abortString);
        if (handler)
            handler(result);
    });
    if (isRebase)
        command->setProgressParser(GitProgressParser());
    command->start();
}

bool GitClient::synchronousRevert(const FilePath &workingDirectory, const QString &commit)
{
    const QString command = "revert";
    // Do not stash if --continue or --abort is given as the commit
    if (!commit.startsWith('-') && !beginStashScope(workingDirectory, command))
        return false;
    return executeAndHandleConflicts(workingDirectory, {command, "--no-edit", commit}, command);
}

bool GitClient::synchronousCherryPick(const FilePath &workingDirectory, const QString &commit)
{
    const QString command = "cherry-pick";
    // "commit" might be --continue or --abort
    const bool isRealCommit = !commit.startsWith('-');
    if (isRealCommit && !beginStashScope(workingDirectory, command))
        return false;

    QStringList arguments = {command};
    if (isRealCommit && isRemoteCommit(workingDirectory, commit))
        arguments << "-x";
    arguments << commit;

    return executeAndHandleConflicts(workingDirectory, arguments, command);
}

void GitClient::interactiveRebase(const FilePath &workingDirectory, const QString &commit, bool fixup)
{
    QStringList arguments = {"rebase", "-i"};
    if (fixup)
        arguments << "--autosquash";
    arguments << commit + '^';
    if (fixup)
        m_disableEditor = true;
    vcsExecAbortable(workingDirectory, arguments, true);
    if (fixup)
        m_disableEditor = false;
}

QString GitClient::msgNoChangedFiles()
{
    return Tr::tr("There are no modified files.");
}

QString GitClient::msgNoCommits(bool includeRemote)
{
    return includeRemote ? Tr::tr("No commits were found") : Tr::tr("No local commits were found");
}

void GitClient::stashPop(const FilePath &workingDirectory, const QString &stash)
{
    QStringList arguments = {"stash", "pop"};
    if (!stash.isEmpty())
        arguments << stash;
    const auto commandHandler = [workingDirectory](const CommandResult &result) {
        handleConflictResponse(result, workingDirectory);
    };
    vcsExecWithHandler(workingDirectory, arguments, this, commandHandler,
                       RunFlags::ShowStdOut | RunFlags::ExpectRepoChanges);
}

bool GitClient::synchronousStashRestore(const FilePath &workingDirectory,
                                        const QString &stash,
                                        bool pop,
                                        const QString &branch /* = QString()*/) const
{
    QStringList arguments = {"stash"};
    if (branch.isEmpty())
        arguments << QLatin1String(pop ? "pop" : "apply") << stash;
    else
        arguments << "branch" << branch << stash;
    return executeAndHandleConflicts(workingDirectory, arguments);
}

bool GitClient::synchronousStashRemove(const FilePath &workingDirectory, const QString &stash,
                                       QString *errorMessage) const
{
    QStringList arguments = {"stash"};
    if (stash.isEmpty())
        arguments << "clear";
    else
        arguments << "drop" << stash;

    const CommandResult result = vcsSynchronousExec(workingDirectory, arguments);
    if (result.result() == ProcessResult::FinishedWithSuccess) {
        const QString output = result.cleanedStdOut();
        if (!output.isEmpty())
            VcsOutputWindow::append(output);
        return true;
    }
    msgCannotRun(arguments, workingDirectory, result.cleanedStdErr(), errorMessage);
    return false;
}

bool GitClient::synchronousStashList(const FilePath &workingDirectory, QList<Stash> *stashes,
                                     QString *errorMessage) const
{
    stashes->clear();

    const QStringList arguments = {"stash", "list", noColorOption};
    const CommandResult result = vcsSynchronousExec(workingDirectory, arguments,
                                                    RunFlags::ForceCLocale);
    if (result.result() != ProcessResult::FinishedWithSuccess) {
        msgCannotRun(arguments, workingDirectory, result.cleanedStdErr(), errorMessage);
        return false;
    }
    Stash stash;
    const QStringList lines = splitLines(result.cleanedStdOut());
    for (const QString &line : lines) {
        if (stash.parseStashLine(line))
            stashes->push_back(stash);
    }
    return true;
}

// Read a single-line config value, return trimmed
QString GitClient::readConfigValue(const FilePath &workingDirectory, const QString &configVar) const
{
    return readOneLine(workingDirectory, {"config", configVar});
}

QChar GitClient::commentChar(const Utils::FilePath &workingDirectory)
{
    const QString commentChar = readConfigValue(workingDirectory, "core.commentChar");
    return commentChar.isEmpty() ? QChar(Constants::DEFAULT_COMMENT_CHAR) : commentChar.at(0);
}

void GitClient::setConfigValue(const FilePath &workingDirectory, const QString &configVar,
                               const QString &value) const
{
    readOneLine(workingDirectory, {"config", configVar, value});
}

QString GitClient::readGitVar(const FilePath &workingDirectory, const QString &configVar) const
{
    return readOneLine(workingDirectory, {"var", configVar});
}

static QTextCodec *configFileCodec()
{
    // Git for Windows always uses UTF-8 for configuration:
    // https://github.com/msysgit/msysgit/wiki/Git-for-Windows-Unicode-Support#convert-config-files
    static QTextCodec *codec = HostOsInfo::isWindowsHost()
            ? QTextCodec::codecForName("UTF-8")
            : QTextCodec::codecForLocale();
    return codec;
}

QString GitClient::readOneLine(const FilePath &workingDirectory, const QStringList &arguments) const
{
    const CommandResult result = vcsSynchronousExec(workingDirectory, arguments,
                                                    RunFlags::NoOutput, vcsTimeoutS(),
                                                    configFileCodec());
    if (result.result() == ProcessResult::FinishedWithSuccess)
        return result.cleanedStdOut().trimmed();
    return {};
}

void GitClient::readConfigAsync(const FilePath &workingDirectory, const QStringList &arguments,
                                const CommandHandler &handler) const
{
    vcsExecWithHandler(workingDirectory, arguments, this, handler, RunFlags::NoOutput,
                       configFileCodec());
}

static unsigned parseGitVersion(const QString &output)
{
    // cut 'git version 1.6.5.1.sha'
    // another form: 'git version 1.9.rc1'
    const QRegularExpression versionPattern("^[^\\d]+(\\d+)\\.(\\d+)\\.(\\d+|rc\\d).*$");
    QTC_ASSERT(versionPattern.isValid(), return 0);
    const QRegularExpressionMatch match = versionPattern.match(output);
    QTC_ASSERT(match.hasMatch(), return 0);
    const unsigned majorV = match.captured(1).toUInt(nullptr, 16);
    const unsigned minorV = match.captured(2).toUInt(nullptr, 16);
    const unsigned patchV = match.captured(3).toUInt(nullptr, 16);
    return version(majorV, minorV, patchV);
}

// determine version as '(major << 16) + (minor << 8) + patch' or 0.
QFuture<unsigned> GitClient::gitVersion() const
{
    QFutureInterface<unsigned> fi;
    fi.reportStarted();

    // Do not execute repeatedly if that fails (due to git
    // not being installed) until settings are changed.
    const FilePath newGitBinary = vcsBinary();
    const bool needToRunGit = m_gitVersionForBinary != newGitBinary && !newGitBinary.isEmpty();
    if (needToRunGit) {
        auto proc = new Process(const_cast<GitClient *>(this));
        connect(proc, &Process::done, this, [this, proc, fi, newGitBinary]() mutable {
            if (proc->result() == ProcessResult::FinishedWithSuccess) {
                m_cachedGitVersion = parseGitVersion(proc->cleanedStdOut());
                m_gitVersionForBinary = newGitBinary;
                fi.reportResult(m_cachedGitVersion);
                fi.reportFinished();
            }
            proc->deleteLater();
        });

        proc->setTimeoutS(vcsTimeoutS());
        proc->setEnvironment(processEnvironment());
        proc->setCommand({newGitBinary, {"--version"}});
        proc->start();
    } else {
        // already cached
        fi.reportResult(m_cachedGitVersion);
        fi.reportFinished();
    }

    return fi.future();
}

bool GitClient::StashInfo::init(const FilePath &workingDirectory, const QString &command,
                                StashFlag flag, PushAction pushAction)
{
    m_workingDir = workingDirectory;
    m_flags = flag;
    m_pushAction = pushAction;
    QString errorMessage;
    QString statusOutput;
    switch (gitClient().gitStatus(m_workingDir, StatusMode(NoUntracked | NoSubmodules),
                                &statusOutput, &errorMessage)) {
    case GitClient::StatusChanged:
        if (m_flags & NoPrompt)
            executeStash(command, &errorMessage);
        else
            stashPrompt(command, statusOutput, &errorMessage);
        break;
    case GitClient::StatusUnchanged:
        m_stashResult = StashUnchanged;
        break;
    case GitClient::StatusFailed:
        m_stashResult = StashFailed;
        break;
    }

    if (m_stashResult == StashFailed)
        VcsOutputWindow::appendError(errorMessage);
    return !stashingFailed();
}

void GitClient::StashInfo::stashPrompt(const QString &command, const QString &statusOutput,
                                       QString *errorMessage)
{
    QMessageBox msgBox(QMessageBox::Question, Tr::tr("Uncommitted Changes Found"),
                       Tr::tr("What would you like to do with local changes in:") + "\n\n\""
                       + m_workingDir.toUserOutput() + '\"',
                       QMessageBox::NoButton, ICore::dialogParent());

    msgBox.setDetailedText(statusOutput);

    QPushButton *stashAndPopButton = msgBox.addButton(Tr::tr("Stash && &Pop"), QMessageBox::AcceptRole);
    stashAndPopButton->setToolTip(Tr::tr("Stash local changes and pop when %1 finishes.").arg(command));

    QPushButton *stashButton = msgBox.addButton(Tr::tr("&Stash"), QMessageBox::AcceptRole);
    stashButton->setToolTip(Tr::tr("Stash local changes and execute %1.").arg(command));

    QPushButton *discardButton = msgBox.addButton(Tr::tr("&Discard"), QMessageBox::AcceptRole);
    discardButton->setToolTip(Tr::tr("Discard (reset) local changes and execute %1.").arg(command));

    QPushButton *ignoreButton = nullptr;
    if (m_flags & AllowUnstashed) {
        ignoreButton = msgBox.addButton(QMessageBox::Ignore);
        ignoreButton->setToolTip(Tr::tr("Execute %1 with local changes in working directory.")
                                 .arg(command));
    }

    QPushButton *cancelButton = msgBox.addButton(QMessageBox::Cancel);
    cancelButton->setToolTip(Tr::tr("Cancel %1.").arg(command));

    msgBox.exec();

    if (msgBox.clickedButton() == discardButton) {
        m_stashResult = gitClient().synchronousReset(m_workingDir, QStringList(), errorMessage) ?
                    StashUnchanged : StashFailed;
    } else if (msgBox.clickedButton() == ignoreButton) { // At your own risk, so.
        m_stashResult = NotStashed;
    } else if (msgBox.clickedButton() == cancelButton) {
        m_stashResult = StashCanceled;
    } else if (msgBox.clickedButton() == stashButton) {
        const bool result = gitClient().executeSynchronousStash(
                    m_workingDir, creatorStashMessage(command), false, errorMessage);
        m_stashResult = result ? StashUnchanged : StashFailed;
    } else if (msgBox.clickedButton() == stashAndPopButton) {
        executeStash(command, errorMessage);
    }
}

void GitClient::StashInfo::executeStash(const QString &command, QString *errorMessage)
{
    m_message = creatorStashMessage(command);
    if (!gitClient().executeSynchronousStash(m_workingDir, m_message, false, errorMessage))
        m_stashResult = StashFailed;
    else
        m_stashResult = Stashed;
 }

bool GitClient::StashInfo::stashingFailed() const
{
    switch (m_stashResult) {
    case StashCanceled:
    case StashFailed:
        return true;
    case NotStashed:
        return !(m_flags & AllowUnstashed);
    default:
        return false;
    }
}

void GitClient::StashInfo::end()
{
    if (m_stashResult == Stashed) {
        QString stashName;
        if (gitClient().stashNameFromMessage(m_workingDir, m_message, &stashName))
            gitClient().stashPop(m_workingDir, stashName);
    }

    if (m_pushAction == NormalPush)
        gitClient().push(m_workingDir);
    else if (m_pushAction == PushToGerrit)
        GitPlugin::gerritPush(m_workingDir);

    m_pushAction = NoPush;
    m_stashResult = NotStashed;
}

GitRemote::GitRemote(const QString &location) : Core::IVersionControl::RepoUrl(location)
{
    if (isValid && protocol == "file")
        isValid = QDir(path).exists() || QDir(path + ".git").exists();
}

QString GitClient::suggestedLocalBranchName(
        const FilePath &workingDirectory,
        const QStringList &localNames,
        const QString &target,
        BranchTargetType targetType)
{
    QString initialName;
    if (targetType == BranchTargetType::Remote) {
        initialName = target.mid(target.lastIndexOf('/') + 1);
    } else {
        QString subject;
        gitClient().synchronousLog(workingDirectory, {"-n", "1", "--format=%s", target},
                                   &subject, nullptr, RunFlags::NoOutput);
        initialName = subject.trimmed();
    }
    QString suggestedName = initialName;
    int i = 2;
    while (localNames.contains(suggestedName)) {
        suggestedName = initialName + QString::number(i);
        ++i;
    }

    return suggestedName;
}

void GitClient::addChangeActions(QMenu *menu, const FilePath &source, const QString &change)
{
    QTC_ASSERT(!change.isEmpty(), return);
    const FilePath &workingDir = fileWorkingDirectory(source);
    const bool isRange = change.contains("..");
    menu->addAction(Tr::tr("Cherr&y-Pick %1").arg(change), [workingDir, change] {
        gitClient().synchronousCherryPick(workingDir, change);
    });
    menu->addAction(Tr::tr("Re&vert %1").arg(change), [workingDir, change] {
        gitClient().synchronousRevert(workingDir, change);
    });
    if (!isRange) {
        menu->addAction(Tr::tr("C&heckout %1").arg(change), [workingDir, change] {
            gitClient().checkout(workingDir, change);
        });
        connect(menu->addAction(Tr::tr("&Interactive Rebase from %1...").arg(change)),
                &QAction::triggered, [workingDir, change] {
            GitPlugin::startRebaseFromCommit(workingDir, change);
        });
    }
    QAction *logAction = menu->addAction(Tr::tr("&Log for %1").arg(change), [workingDir, change] {
        gitClient().log(workingDir, QString(), false, {change});
    });
    if (isRange) {
        menu->setDefaultAction(logAction);
    } else {
        const FilePath filePath = source;
        if (!filePath.isDir()) {
            menu->addAction(Tr::tr("Sh&ow file \"%1\" on revision %2").arg(filePath.fileName(), change),
                            [workingDir, change, source] {
                gitClient().openShowEditor(workingDir, change, source);
            });
        }
        menu->addAction(Tr::tr("Add &Tag for %1...").arg(change), [workingDir, change] {
            QString output;
            QString errorMessage;
            gitClient().synchronousTagCmd(workingDir, QStringList(), &output, &errorMessage);

            const QStringList tags = output.split('\n');
            BranchAddDialog dialog(tags, BranchAddDialog::Type::AddTag, Core::ICore::dialogParent());

            if (dialog.exec() == QDialog::Rejected)
                return;

            gitClient().synchronousTagCmd(workingDir, {dialog.branchName(), change},
                                          &output, &errorMessage);
            VcsOutputWindow::append(output);
            if (!errorMessage.isEmpty())
                VcsOutputWindow::append(errorMessage, VcsOutputWindow::MessageStyle::Error);
        });

        auto resetChange = [workingDir, change](const QByteArray &resetType) {
            gitClient().reset(workingDir, QLatin1String("--" + resetType), change);
        };
        auto resetMenu = new QMenu(Tr::tr("&Reset to Change %1").arg(change), menu);
        resetMenu->addAction(Tr::tr("&Hard"), std::bind(resetChange, "hard"));
        resetMenu->addAction(Tr::tr("&Mixed"), std::bind(resetChange, "mixed"));
        resetMenu->addAction(Tr::tr("&Soft"), std::bind(resetChange, "soft"));
        menu->addMenu(resetMenu);
    }

    menu->addAction((isRange ? Tr::tr("Di&ff %1") : Tr::tr("Di&ff Against %1")).arg(change),
                    [workingDir, change] {
        gitClient().diffRepository(workingDir, change, {});
    });
    if (!isRange) {
        if (!gitClient().m_diffCommit.isEmpty()) {
            menu->addAction(Tr::tr("Diff &Against Saved %1").arg(gitClient().m_diffCommit),
                            [workingDir, change] {
                gitClient().diffRepository(workingDir, gitClient().m_diffCommit, change);
                gitClient().m_diffCommit.clear();
            });
        }
        menu->addAction(Tr::tr("&Save for Diff"), [change] {
            gitClient().m_diffCommit = change;
        });
    }
}

FilePath GitClient::fileWorkingDirectory(const Utils::FilePath &file)
{
    Utils::FilePath path = file;
    if (!path.isEmpty() && !path.isDir())
        path = path.parentDir();
    while (!path.isEmpty() && !path.exists())
        path = path.parentDir();
    return path;
}

IEditor *GitClient::openShowEditor(const FilePath &workingDirectory, const QString &ref,
                                   const FilePath &path, ShowEditor showSetting)
{
    const FilePath topLevel = VcsManager::findTopLevelForDirectory(workingDirectory);
    const QString topLevelString = topLevel.toString();
    const QString relativePath = QDir(topLevelString).relativeFilePath(path.toString());
    const QByteArray content = synchronousShow(topLevel, ref + ":" + relativePath);
    if (showSetting == ShowEditor::OnlyIfDifferent) {
        if (content.isEmpty())
            return nullptr;
        QByteArray fileContent;
        if (TextFileFormat::readFileUTF8(path,
                                         nullptr,
                                         &fileContent,
                                         nullptr)
            == TextFileFormat::ReadSuccess) {
            if (fileContent == content)
                return nullptr; // open the file for read/write
        }
    }

    const QString documentId = QLatin1String(Git::Constants::GIT_PLUGIN)
            + QLatin1String(".GitShow.") + topLevelString
            + QLatin1String(".") + relativePath;
    QString title = Tr::tr("Git Show %1:%2").arg(ref, relativePath);
    IEditor *editor = EditorManager::openEditorWithContents(Id(), &title, content, documentId,
                                                            EditorManager::DoNotSwitchToDesignMode);
    editor->document()->setTemporary(true);
    // FIXME: Check should that be relative
    VcsBase::setSource(editor->document(), path);
    return editor;
}

} // Git::Internal

#include "gitclient.moc"
