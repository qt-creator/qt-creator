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

#include "gitclient.h"
#include "gitutils.h"

#include "commitdata.h"
#include "gitconstants.h"
#include "giteditor.h"
#include "gitplugin.h"
#include "gitsubmiteditor.h"
#include "mergetool.h"
#include "branchadddialog.h"
#include "gerrit/gerritplugin.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/synchronousprocess.h>
#include <utils/temporaryfile.h>
#include <utils/theme/theme.h>

#include <vcsbase/submitfilemodel.h>
#include <vcsbase/vcsbasediffeditorcontroller.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseeditorconfig.h>
#include <vcsbase/vcsbaseplugin.h>
#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsoutputwindow.h>

#include <diffeditor/descriptionwidgetwatcher.h>
#include <diffeditor/diffeditorconstants.h>
#include <diffeditor/diffeditorcontroller.h>
#include <diffeditor/diffutils.h>

#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <QAction>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHash>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextBlock>
#include <QToolButton>
#include <QTextCodec>

const char GIT_DIRECTORY[] = ".git";
const char HEAD[] = "HEAD";
const char CHERRY_PICK_HEAD[] = "CHERRY_PICK_HEAD";
const char BRANCHES_PREFIX[] = "Branches: ";
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
using namespace Utils;
using namespace VcsBase;

namespace Git {
namespace Internal {

static GitClient *m_instance = nullptr;

// Suppress git diff warnings about "LF will be replaced by CRLF..." on Windows.
static unsigned diffExecutionFlags()
{
    return HostOsInfo::isWindowsHost() ? unsigned(VcsCommand::SuppressStdErr) : 0u;
}

const unsigned silentFlags = unsigned(VcsCommand::SuppressCommandLogging
                                      | VcsCommand::SuppressStdErr
                                      | VcsCommand::SuppressFailMessage);

static QString branchesDisplay(const QString &prefix, QStringList *branches, bool *first)
{
    const int limit = 12;
    const int count = branches->count();
    int more = 0;
    QString output;
    if (*first)
        *first = false;
    else
        output += QString(sizeof(BRANCHES_PREFIX) - 1, ' '); // Align
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
        output += ' ' + GitClient::tr("and %n more", nullptr, more);
    return output;
}

class DescriptionWidgetDecorator : public QObject
{
    Q_OBJECT
public:
    DescriptionWidgetDecorator(DescriptionWidgetWatcher *watcher);

    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void branchListRequested();

private:
    bool checkContentsUnderCursor(const QTextCursor &cursor) const;
    void highlightCurrentContents(TextEditor::TextEditorWidget *textEditor,
                                  const QTextCursor &cursor);
    void handleCurrentContents(const QTextCursor &cursor);
    void addWatch(TextEditor::TextEditorWidget *widget);
    void removeWatch(TextEditor::TextEditorWidget *widget);

    DescriptionWidgetWatcher *m_watcher;
    QHash<QObject *, TextEditor::TextEditorWidget *> m_viewportToTextEditor;
};

DescriptionWidgetDecorator::DescriptionWidgetDecorator(DescriptionWidgetWatcher *watcher)
    : QObject(),
      m_watcher(watcher)
{
    QList<TextEditor::TextEditorWidget *> widgets = m_watcher->descriptionWidgets();
    for (auto *widget : widgets)
        addWatch(widget);

    connect(m_watcher, &DescriptionWidgetWatcher::descriptionWidgetAdded,
            this, &DescriptionWidgetDecorator::addWatch);
    connect(m_watcher, &DescriptionWidgetWatcher::descriptionWidgetRemoved,
            this, &DescriptionWidgetDecorator::removeWatch);
}

bool DescriptionWidgetDecorator::eventFilter(QObject *watched, QEvent *event)
{
    TextEditor::TextEditorWidget *textEditor = m_viewportToTextEditor.value(watched);
    if (!textEditor)
        return QObject::eventFilter(watched, event);

    if (event->type() == QEvent::MouseMove) {
        auto mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->buttons())
            return QObject::eventFilter(watched, event);

        Qt::CursorShape cursorShape;

        const QTextCursor cursor = textEditor->cursorForPosition(mouseEvent->pos());
        if (checkContentsUnderCursor(cursor)) {
            highlightCurrentContents(textEditor, cursor);
            cursorShape = Qt::PointingHandCursor;
        } else {
            textEditor->setExtraSelections(TextEditor::TextEditorWidget::OtherSelection,
                                           QList<QTextEdit::ExtraSelection>());
            cursorShape = Qt::IBeamCursor;
        }

        bool ret = QObject::eventFilter(watched, event);
        textEditor->viewport()->setCursor(cursorShape);
        return ret;
    } else if (event->type() == QEvent::MouseButtonRelease) {
        auto mouseEvent = static_cast<QMouseEvent *>(event);

        if (mouseEvent->button() == Qt::LeftButton && !(mouseEvent->modifiers() & Qt::ShiftModifier)) {
            const QTextCursor cursor = textEditor->cursorForPosition(mouseEvent->pos());
            if (checkContentsUnderCursor(cursor)) {
                handleCurrentContents(cursor);
                return true;
            }
        }

        return QObject::eventFilter(watched, event);
    }
    return QObject::eventFilter(watched, event);
}

bool DescriptionWidgetDecorator::checkContentsUnderCursor(const QTextCursor &cursor) const
{
    return cursor.block().text() == Constants::EXPAND_BRANCHES;
}

void DescriptionWidgetDecorator::highlightCurrentContents(
        TextEditor::TextEditorWidget *textEditor, const QTextCursor &cursor)
{
    QTextEdit::ExtraSelection sel;
    sel.cursor = cursor;
    sel.cursor.select(QTextCursor::LineUnderCursor);
    sel.format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
    const QColor textColor = TextEditor::TextEditorSettings::fontSettings().formatFor(TextEditor::C_TEXT).foreground();
    sel.format.setUnderlineColor(textColor.isValid() ? textColor : textEditor->palette().color(QPalette::WindowText));
    textEditor->setExtraSelections(TextEditor::TextEditorWidget::OtherSelection,
                       QList<QTextEdit::ExtraSelection>() << sel);
}

void DescriptionWidgetDecorator::handleCurrentContents(const QTextCursor &cursor)
{
    QTextCursor copy = cursor;

    copy.select(QTextCursor::LineUnderCursor);
    copy.removeSelectedText();
    copy.insertText("Branches: Expanding...");
    emit branchListRequested();
}

void DescriptionWidgetDecorator::addWatch(TextEditor::TextEditorWidget *widget)
{
    m_viewportToTextEditor.insert(widget->viewport(), widget);
    widget->viewport()->installEventFilter(this);
}

void DescriptionWidgetDecorator::removeWatch(TextEditor::TextEditorWidget *widget)
{
    widget->viewport()->removeEventFilter(this);
    m_viewportToTextEditor.remove(widget->viewport());
}

///////////////////////////////

class GitBaseDiffEditorController : public VcsBaseDiffEditorController
{
    Q_OBJECT

protected:
    explicit GitBaseDiffEditorController(IDocument *document,
                                         const QString &leftCommit,
                                         const QString &rightCommit);

    void runCommand(const QList<QStringList> &args, QTextCodec *codec = nullptr);

    QStringList addConfigurationArguments(const QStringList &args) const;
    QStringList baseArguments() const;

public:
    void initialize();

private:
    void updateBranchList();

    DescriptionWidgetWatcher m_watcher;
    DescriptionWidgetDecorator m_decorator;
    QString m_leftCommit;
    QString m_rightCommit;
};

class GitDiffEditorController : public GitBaseDiffEditorController
{
public:
    explicit GitDiffEditorController(IDocument *document,
                                     const QString &leftCommit,
                                     const QString &rightCommit,
                                     const QStringList &extraArgs)
        : GitBaseDiffEditorController(document, leftCommit, rightCommit)
    {
        setReloader([this, extraArgs] {
            runCommand({addConfigurationArguments(baseArguments() << extraArgs)});
        });
    }
};

GitBaseDiffEditorController::GitBaseDiffEditorController(IDocument *document,
                                                         const QString &leftCommit,
                                                         const QString &rightCommit) :
    VcsBaseDiffEditorController(document),
    m_watcher(this),
    m_decorator(&m_watcher),
    m_leftCommit(leftCommit),
    m_rightCommit(rightCommit)
{
    connect(&m_decorator, &DescriptionWidgetDecorator::branchListRequested,
            this, &GitBaseDiffEditorController::updateBranchList);
    setDisplayName("Git Diff");
}

void GitBaseDiffEditorController::initialize()
{
    if (m_rightCommit.isEmpty()) {
        // This is workaround for lack of support for merge commits and resolving conflicts,
        // we compare the current state of working tree to the HEAD of current branch
        // instead of showing unsupported combined diff format.
        GitClient::CommandInProgress commandInProgress = m_instance->checkCommandInProgress(workingDirectory());
        if (commandInProgress != GitClient::NoCommand)
            m_rightCommit = HEAD;
    }
}

void GitBaseDiffEditorController::updateBranchList()
{
    const QString revision = description().mid(7, 12);
    if (revision.isEmpty())
        return;

    const QString workingDirectory = baseDirectory();
    VcsCommand *command = m_instance->vcsExec(
                workingDirectory, {"branch", noColorOption, "-a", "--contains", revision}, nullptr,
                false, 0, workingDirectory);
    connect(command, &VcsCommand::stdOutText, this, [this](const QString &text) {
        const QString remotePrefix = "remotes/";
        const QString localPrefix = "<Local>";
        const int prefixLength = remotePrefix.length();
        QString output = BRANCHES_PREFIX;
        QStringList branches;
        QString previousRemote = localPrefix;
        bool first = true;
        for (const QString &branch : text.split('\n')) {
            const QString b = branch.mid(2).trimmed();
            if (b.isEmpty())
                continue;
            if (b.startsWith(remotePrefix)) {
                const int nextSlash = b.indexOf('/', prefixLength);
                if (nextSlash < 0)
                    continue;
                const QString remote = b.mid(prefixLength, nextSlash - prefixLength);
                if (remote != previousRemote) {
                    output += branchesDisplay(previousRemote, &branches, &first) + '\n';
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
                output += tr("<None>");
        } else {
            output += branchesDisplay(previousRemote, &branches, &first);
        }
        const QString branchList = output.trimmed();
        QString newDescription = description();
        newDescription.replace(Constants::EXPAND_BRANCHES, branchList);
        setDescription(newDescription);
    });
}

///////////////////////////////

void GitBaseDiffEditorController::runCommand(const QList<QStringList> &args, QTextCodec *codec)
{
    VcsBaseDiffEditorController::runCommand(args, diffExecutionFlags(), codec);
}

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

QStringList GitBaseDiffEditorController::baseArguments() const
{
    QStringList res = {"diff"};
    if (!m_leftCommit.isEmpty())
        res << m_leftCommit;
    if (!m_rightCommit.isEmpty())
        res << m_rightCommit;
    return res;
}

class FileListDiffController : public GitBaseDiffEditorController
{
public:
    FileListDiffController(IDocument *document,
                           const QStringList &stagedFiles, const QStringList &unstagedFiles) :
        GitBaseDiffEditorController(document, {}, {})
    {
        setReloader([this, stagedFiles, unstagedFiles] {
            QList<QStringList> argLists;
            if (!stagedFiles.isEmpty()) {
                QStringList stagedArgs = QStringList({"diff", "--cached", "--"}) << stagedFiles;
                argLists << addConfigurationArguments(stagedArgs);
            }

            if (!unstagedFiles.isEmpty())
                argLists << addConfigurationArguments(baseArguments() << "--" << unstagedFiles);

            if (!argLists.isEmpty())
                runCommand(argLists);
        });
    }
};

class ShowController : public GitBaseDiffEditorController
{
    Q_OBJECT
public:
    ShowController(IDocument *document, const QString &id) :
        GitBaseDiffEditorController(document, {}, {}),
        m_id(id),
        m_state(Idle)
    {
        setDisplayName("Git Show");
        setReloader([this] {
            m_state = GettingDescription;
            const QStringList args = {"show", "-s", noColorOption, showFormatC, m_id};
            runCommand({args}, m_instance->encoding(workingDirectory(), "i18n.commitEncoding"));
            setStartupFile(VcsBase::source(this->document()));
        });
    }

    void processCommandOutput(const QString &output) override;

private:
    const QString m_id;
    enum State { Idle, GettingDescription, GettingDiff };
    State m_state;
};

void ShowController::processCommandOutput(const QString &output)
{
    QTC_ASSERT(m_state != Idle, return);
    if (m_state == GettingDescription) {
        setDescription(m_instance->extendedShowDescription(workingDirectory(), output));
        // stage 2
        m_state = GettingDiff;
        const QStringList args = {"show", "--format=format:", // omit header, already generated
                                  noColorOption, decorateOption, m_id};
        runCommand(QList<QStringList>() << addConfigurationArguments(args));
    } else if (m_state == GettingDiff) {
        m_state = Idle;
        GitBaseDiffEditorController::processCommandOutput(output);
    }
}

///////////////////////////////

class BaseGitDiffArgumentsWidget : public VcsBaseEditorConfig
{
    Q_OBJECT

public:
    BaseGitDiffArgumentsWidget(VcsBaseClientSettings &settings, QToolBar *toolBar) :
        VcsBaseEditorConfig(toolBar)
    {
        m_patienceButton
                = addToggleButton("--patience", tr("Patience"),
                                  tr("Use the patience algorithm for calculating the differences."));
        mapSetting(m_patienceButton, settings.boolPointer(GitSettings::diffPatienceKey));
        m_ignoreWSButton = addToggleButton("--ignore-space-change", tr("Ignore Whitespace"),
                                           tr("Ignore whitespace only changes."));
        mapSetting(m_ignoreWSButton,
                   settings.boolPointer(GitSettings::ignoreSpaceChangesInDiffKey));
    }

protected:
    QAction *m_patienceButton;
    QAction *m_ignoreWSButton;
};

class GitBlameArgumentsWidget : public VcsBaseEditorConfig
{
    Q_OBJECT

public:
    GitBlameArgumentsWidget(VcsBaseClientSettings &settings, QToolBar *toolBar) :
        VcsBaseEditorConfig(toolBar)
    {
        mapSetting(addToggleButton(QString(), tr("Omit Date"),
                                   tr("Hide the date of a change from the output.")),
                   settings.boolPointer(GitSettings::omitAnnotationDateKey));
        mapSetting(addToggleButton("-w", tr("Ignore Whitespace"),
                                   tr("Ignore whitespace only changes.")),
                   settings.boolPointer(GitSettings::ignoreSpaceChangesInBlameKey));

        const QList<ChoiceItem> logChoices = {
            ChoiceItem(tr("No Move Detection"), ""),
            ChoiceItem(tr("Detect Moves Within File"), "-M"),
            ChoiceItem(tr("Detect Moves Between Files"), "-M -C"),
            ChoiceItem(tr("Detect Moves and Copies Between Files"), "-M -C -C")
        };
        mapSetting(addChoices(tr("Move detection"), {}, logChoices),
                   settings.intPointer(GitSettings::blameMoveDetection));

        addReloadButton();
    }
};

class BaseGitLogArgumentsWidget : public BaseGitDiffArgumentsWidget
{
    Q_OBJECT

public:
    BaseGitLogArgumentsWidget(VcsBaseClientSettings &settings, GitEditorWidget *editor) :
        BaseGitDiffArgumentsWidget(settings, editor->toolBar())
    {
        QToolBar *toolBar = editor->toolBar();
        QAction *diffButton = addToggleButton(patchOption, tr("Diff"),
                                              tr("Show difference."));
        mapSetting(diffButton, settings.boolPointer(GitSettings::logDiffKey));
        connect(diffButton, &QAction::toggled, m_patienceButton, &QAction::setVisible);
        connect(diffButton, &QAction::toggled, m_ignoreWSButton, &QAction::setVisible);
        m_patienceButton->setVisible(diffButton->isChecked());
        m_ignoreWSButton->setVisible(diffButton->isChecked());
        auto filterAction = new QAction(tr("Filter"), toolBar);
        filterAction->setToolTip(tr("Filter commits by message or content."));
        filterAction->setCheckable(true);
        connect(filterAction, &QAction::toggled, editor, &GitEditorWidget::toggleFilters);
        toolBar->addAction(filterAction);
    }
};

static bool gitHasRgbColors()
{
    const unsigned gitVersion = GitClient::instance()->gitVersion();
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
    GitLogArgumentsWidget(VcsBaseClientSettings &settings, bool fileRelated, GitEditorWidget *editor) :
        BaseGitLogArgumentsWidget(settings, editor)
    {
        QAction *firstParentButton =
                addToggleButton({"-m", "--first-parent"},
                                tr("First Parent"),
                                tr("Follow only the first parent on merge commits."));
        mapSetting(firstParentButton, settings.boolPointer(GitSettings::firstParentKey));
        QAction *graphButton = addToggleButton(graphArguments(), tr("Graph"),
                                               tr("Show textual graph log."));
        mapSetting(graphButton, settings.boolPointer(GitSettings::graphLogKey));

        QAction *colorButton = addToggleButton(QStringList{colorOption},
                                        tr("Color"), tr("Use colors in log."));
        mapSetting(colorButton, settings.boolPointer(GitSettings::colorLogKey));

        if (fileRelated) {
            QAction *followButton = addToggleButton(
                        "--follow", tr("Follow"),
                        tr("Show log also for previous names of the file."));
            mapSetting(followButton, settings.boolPointer(GitSettings::followRenamesKey));
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
    GitRefLogArgumentsWidget(VcsBaseClientSettings &settings, GitEditorWidget *editor) :
        BaseGitLogArgumentsWidget(settings, editor)
    {
        QAction *showDateButton =
                addToggleButton("--date=iso",
                                tr("Show Date"),
                                tr("Show date instead of sequence."));
        mapSetting(showDateButton, settings.boolPointer(GitSettings::refLogShowDateKey));

        addReloadButton();
    }
};

class ConflictHandler final : public QObject
{
    Q_OBJECT
public:
    static void attachToCommand(VcsCommand *command, const QString &abortCommand = QString()) {
        auto handler = new ConflictHandler(command->defaultWorkingDirectory(), abortCommand);
        handler->setParent(command); // delete when command goes out of scope

        command->addFlags(VcsCommand::ExpectRepoChanges);
        connect(command, &VcsCommand::stdOutText, handler, &ConflictHandler::readStdOut);
        connect(command, &VcsCommand::stdErrText, handler, &ConflictHandler::readStdErr);
    }

    static void handleResponse(const Utils::SynchronousProcessResponse &response,
                               const QString &workingDirectory,
                               const QString &abortCommand = QString())
    {
        ConflictHandler handler(workingDirectory, abortCommand);
        // No conflicts => do nothing
        if (response.result == SynchronousProcessResponse::Finished)
            return;
        handler.readStdOut(response.stdOut());
        handler.readStdErr(response.stdErr());
    }

private:
    ConflictHandler(const QString &workingDirectory, const QString &abortCommand) :
          m_workingDirectory(workingDirectory),
          m_abortCommand(abortCommand)
    { }

    ~ConflictHandler() final
    {
        // If interactive rebase editor window is closed, plugin is terminated
        // but referenced here when the command ends
        if (m_commit.isEmpty() && m_files.isEmpty()) {
            if (m_instance->checkCommandInProgress(m_workingDirectory) == GitClient::NoCommand)
                m_instance->endStashScope(m_workingDirectory);
        } else {
            m_instance->handleMergeConflicts(m_workingDirectory, m_commit, m_files, m_abortCommand);
        }
    }

    void readStdOut(const QString &data)
    {
        static const QRegularExpression patchFailedRE("Patch failed at ([^\\n]*)");
        static const QRegularExpression conflictedFilesRE("Merge conflict in ([^\\n]*)");
        const QRegularExpressionMatch match = patchFailedRE.match(data);
        if (match.hasMatch())
            m_commit = match.captured(1);
        QRegularExpressionMatchIterator it = conflictedFilesRE.globalMatch(data);
        while (it.hasNext())
            m_files.append(it.next().captured(1));
    }

    void readStdErr(const QString &data)
    {
        static const QRegularExpression couldNotApplyRE("[Cc]ould not (?:apply|revert) ([^\\n]*)");
        const QRegularExpressionMatch match = couldNotApplyRE.match(data);
        if (match.hasMatch())
            m_commit = match.captured(1);
    }
private:
    QString m_workingDirectory;
    QString m_abortCommand;
    QString m_commit;
    QStringList m_files;
};

class GitProgressParser : public ProgressParser
{
public:
    static void attachToCommand(VcsCommand *command)
    {
        command->setProgressParser(new GitProgressParser);
    }

private:
    GitProgressParser() : m_progressExp("\\((\\d+)/(\\d+)\\)") // e.g. Rebasing (7/42)
    { }

    void parseProgress(const QString &text) override
    {
        const QRegularExpressionMatch match = m_progressExp.match(text);
        if (match.hasMatch())
            setProgressAndMaximum(match.captured(1).toInt(), match.captured(2).toInt());
    }

    const QRegularExpression m_progressExp;
};

static inline QString msgRepositoryNotFound(const QString &dir)
{
    return GitClient::tr("Cannot determine the repository for \"%1\".").arg(dir);
}

static inline QString msgParseFilesFailed()
{
    return  GitClient::tr("Cannot parse the file output.");
}

static inline QString msgCannotLaunch(const QString &binary)
{
    return GitClient::tr("Cannot launch \"%1\".").arg(QDir::toNativeSeparators(binary));
}

static inline void msgCannotRun(const QString &message, QString *errorMessage)
{
    if (errorMessage)
        *errorMessage = message;
    else
        VcsOutputWindow::appendError(message);
}

static inline void msgCannotRun(const QStringList &args, const QString &workingDirectory,
                                const QString &error, QString *errorMessage)
{
    const QString message = GitClient::tr("Cannot run \"%1\" in \"%2\": %3")
            .arg("git " + args.join(' '),
                 QDir::toNativeSeparators(workingDirectory),
                 error);

    msgCannotRun(message, errorMessage);
}

// ---------------- GitClient

GitClient::GitClient(GitSettings *settings) : VcsBase::VcsBaseClientImpl(settings),
    m_cachedGitVersion(0),
    m_disableEditor(false)
{
    m_instance = this;
    m_gitQtcEditor = QString::fromLatin1("\"%1\" -client -block -pid %2")
            .arg(QCoreApplication::applicationFilePath())
            .arg(QCoreApplication::applicationPid());
}

GitClient *GitClient::instance()
{
    return m_instance;
}

QString GitClient::findRepositoryForDirectory(const QString &directory) const
{
    if (directory.isEmpty() || directory.endsWith("/.git") || directory.contains("/.git/"))
        return QString();
    // QFileInfo is outside loop, because it is faster this way
    QFileInfo fileInfo;
    FilePath parent;
    for (FilePath dir = FilePath::fromString(directory); !dir.isEmpty(); dir = dir.parentDir()) {
        const FilePath gitName = dir.pathAppended(GIT_DIRECTORY);
        if (!gitName.exists())
            continue; // parent might exist
        fileInfo.setFile(gitName.toString());
        if (fileInfo.isFile())
            return dir.toString();
        if (gitName.pathAppended("config").exists())
            return dir.toString();
    }
    return QString();
}

QString GitClient::findGitDirForRepository(const QString &repositoryDir) const
{
    static QHash<QString, QString> repoDirCache;
    QString &res = repoDirCache[repositoryDir];
    if (!res.isEmpty())
        return res;

    synchronousRevParseCmd(repositoryDir, "--git-dir", &res);

    if (!QDir(res).isAbsolute())
        res.prepend(repositoryDir + '/');
    return res;
}

bool GitClient::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    return vcsFullySynchronousExec(workingDirectory, {"ls-files", "--error-unmatch", fileName},
                                   Core::ShellCommand::NoOutput).result
            == SynchronousProcessResponse::Finished;
}

QStringList GitClient::unmanagedFiles(const QString &workingDirectory,
                                      const QStringList &filePaths) const
{
    QStringList args({"ls-files", "-z"});
    QDir wd(workingDirectory);
    args << transform(filePaths, [&wd](const QString &fp) { return wd.relativeFilePath(fp); });
    const SynchronousProcessResponse response
            = vcsFullySynchronousExec(workingDirectory, args, Core::ShellCommand::NoOutput);
    if (response.result != SynchronousProcessResponse::Finished)
        return filePaths;
    const QStringList managedFilePaths
            = transform(response.stdOut().split('\0', Qt::SkipEmptyParts),
                        [&wd](const QString &fp) { return wd.absoluteFilePath(fp); });
    return filtered(filePaths, [&managedFilePaths](const QString &fp) {
        return !managedFilePaths.contains(fp);
    });
}

QTextCodec *GitClient::codecFor(GitClient::CodecType codecType, const QString &source) const
{
    if (codecType == CodecSource) {
        return QFileInfo(source).isFile() ? VcsBaseEditor::getCodec(source)
                                          : encoding(source, "gui.encoding");
    }
    if (codecType == CodecLogOutput)
        return encoding(source, "i18n.logOutputEncoding");
    return nullptr;
}

void GitClient::chunkActionsRequested(QMenu *menu, int fileIndex, int chunkIndex,
                                      const DiffEditor::ChunkSelection &selection)
{
    QPointer<DiffEditor::DiffEditorController> diffController
            = qobject_cast<DiffEditorController *>(sender());

    auto stageChunk = [this](QPointer<DiffEditor::DiffEditorController> diffController,
            int fileIndex, int chunkIndex, DiffEditorController::PatchOptions options,
            const DiffEditor::ChunkSelection &selection) {
        if (diffController.isNull())
            return;

        options |= DiffEditorController::AddPrefix;
        const QString patch = diffController->makePatch(fileIndex, chunkIndex, selection, options);
        stage(diffController, patch, options & Revert);
    };

    menu->addSeparator();
    QAction *stageChunkAction = menu->addAction(tr("Stage Chunk"));
    connect(stageChunkAction, &QAction::triggered, this,
            [stageChunk, diffController, fileIndex, chunkIndex]() {
        stageChunk(diffController, fileIndex, chunkIndex,
                   DiffEditorController::NoOption, DiffEditor::ChunkSelection());
    });
    QAction *stageLinesAction = menu->addAction(tr("Stage Selection (%n Lines)", "", selection.selectedRowsCount()));
    connect(stageLinesAction, &QAction::triggered, this,
            [stageChunk, diffController, fileIndex, chunkIndex, selection]() {
        stageChunk(diffController, fileIndex, chunkIndex,
                   DiffEditorController::NoOption, selection);
    });
    QAction *unstageChunkAction = menu->addAction(tr("Unstage Chunk"));
    connect(unstageChunkAction, &QAction::triggered, this,
            [stageChunk, diffController, fileIndex, chunkIndex]() {
        stageChunk(diffController, fileIndex, chunkIndex,
                   DiffEditorController::Revert, DiffEditor::ChunkSelection());
    });
    QAction *unstageLinesAction = menu->addAction(tr("Unstage Selection (%n Lines)", "", selection.selectedRowsCount()));
    connect(unstageLinesAction, &QAction::triggered, this,
            [stageChunk, diffController, fileIndex, chunkIndex, selection]() {
        stageChunk(diffController, fileIndex, chunkIndex,
                   DiffEditorController::Revert,
                   selection);
    });
    if (selection.isNull()) {
        stageLinesAction->setVisible(false);
        unstageLinesAction->setVisible(false);
    }
    if (!diffController || !diffController->chunkExists(fileIndex, chunkIndex)) {
        stageChunkAction->setEnabled(false);
        stageLinesAction->setEnabled(false);
        unstageChunkAction->setEnabled(false);
        unstageLinesAction->setEnabled(false);
    }
}

void GitClient::stage(DiffEditor::DiffEditorController *diffController,
                      const QString &patch, bool revert)
{
    Utils::TemporaryFile patchFile("git-patchfile");
    if (!patchFile.open())
        return;

    const QString baseDir = diffController->baseDirectory();
    QTextCodec *codec = EditorManager::defaultTextCodec();
    const QByteArray patchData = codec
            ? codec->fromUnicode(patch) : patch.toLocal8Bit();
    patchFile.write(patchData);
    patchFile.close();

    QStringList args = {"--cached"};
    if (revert)
        args << "--reverse";
    QString errorMessage;
    if (synchronousApplyPatch(baseDir, patchFile.fileName(),
                              &errorMessage, args)) {
        if (errorMessage.isEmpty()) {
            if (revert)
                VcsOutputWindow::appendSilently(tr("Chunk successfully unstaged"));
            else
                VcsOutputWindow::appendSilently(tr("Chunk successfully staged"));
        } else {
            VcsOutputWindow::appendError(errorMessage);
        }
        diffController->requestReload();
    } else {
        VcsOutputWindow::appendError(errorMessage);
    }
}

void GitClient::requestReload(const QString &documentId, const QString &source,
                              const QString &title, const QString &workingDirectory,
                              std::function<GitBaseDiffEditorController *(IDocument *)> factory) const
{
    // Creating document might change the referenced source. Store a copy and use it.
    const QString sourceCopy = source;

    IDocument *document = DiffEditorController::findOrCreateDocument(documentId, title);
    QTC_ASSERT(document, return);
    GitBaseDiffEditorController *controller = factory(document);
    QTC_ASSERT(controller, return);
    controller->setVcsBinary(settings().binaryPath());
    controller->setVcsTimeoutS(settings().vcsTimeoutS());
    controller->setProcessEnvironment(processEnvironment());
    controller->setWorkingDirectory(workingDirectory);
    controller->initialize();

    connect(controller, &DiffEditorController::chunkActionsRequested,
            this, &GitClient::chunkActionsRequested, Qt::DirectConnection);

    VcsBase::setSource(document, sourceCopy);
    EditorManager::activateEditorForDocument(document);
    controller->requestReload();
}

void GitClient::diffFiles(const QString &workingDirectory,
                          const QStringList &unstagedFileNames,
                          const QStringList &stagedFileNames) const
{
    const QString documentId = QLatin1String(Constants::GIT_PLUGIN)
            + QLatin1String(".DiffFiles.") + workingDirectory;
    requestReload(documentId,
                  workingDirectory, tr("Git Diff Files"), workingDirectory,
                  [stagedFileNames, unstagedFileNames](IDocument *doc) {
                      return new FileListDiffController(doc, stagedFileNames, unstagedFileNames);
                  });
}

void GitClient::diffProject(const QString &workingDirectory, const QString &projectDirectory) const
{
    const QString documentId = QLatin1String(Constants::GIT_PLUGIN)
            + QLatin1String(".DiffProject.") + workingDirectory;
    requestReload(documentId,
                  workingDirectory, tr("Git Diff Project"), workingDirectory,
                  [projectDirectory](IDocument *doc){
                      return new GitDiffEditorController(doc, {}, {}, {"--", projectDirectory});
                  });
}

void GitClient::diffRepository(const QString &workingDirectory,
                               const QString &leftCommit,
                               const QString &rightCommit) const
{
    const QString documentId = QLatin1String(Constants::GIT_PLUGIN)
            + QLatin1String(".DiffRepository.") + workingDirectory;
    requestReload(documentId, workingDirectory, tr("Git Diff Repository"), workingDirectory,
                  [&leftCommit, &rightCommit](IDocument *doc) {
        return new GitDiffEditorController(doc, leftCommit, rightCommit, {});
    });
}

void GitClient::diffFile(const QString &workingDirectory, const QString &fileName) const
{
    const QString title = tr("Git Diff \"%1\"").arg(fileName);
    const QString sourceFile = VcsBaseEditor::getSource(workingDirectory, fileName);
    const QString documentId = QLatin1String(Constants::GIT_PLUGIN)
            + QLatin1String(".DifFile.") + sourceFile;
    requestReload(documentId, sourceFile, title, workingDirectory,
                  [&fileName](IDocument *doc) {
        return new GitDiffEditorController(doc, {}, {}, {"--", fileName});
    });
}

void GitClient::diffBranch(const QString &workingDirectory, const QString &branchName) const
{
    const QString title = tr("Git Diff Branch \"%1\"").arg(branchName);
    const QString documentId = QLatin1String(Constants::GIT_PLUGIN)
            + QLatin1String(".DiffBranch.") + branchName;
    requestReload(documentId, workingDirectory, title, workingDirectory,
                  [branchName](IDocument *doc) {
        return new GitDiffEditorController(doc, branchName, {}, {});
    });
}

void GitClient::merge(const QString &workingDirectory,
                      const QStringList &unmergedFileNames)
{
    auto mergeTool = new MergeTool(this);
    if (!mergeTool->start(workingDirectory, unmergedFileNames))
        delete mergeTool;
}

void GitClient::status(const QString &workingDirectory) const
{
    VcsOutputWindow::setRepository(workingDirectory);
    VcsCommand *command = vcsExec(workingDirectory, {"status", "-u"}, nullptr, true);
    connect(command, &VcsCommand::finished, VcsOutputWindow::instance(), &VcsOutputWindow::clearRepository,
            Qt::QueuedConnection);
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
                "Date:   %C(%4)%cD%Creset%n%n"
                "%C(%5)%w(0,4,4)%s%Creset%n%n%b"
                ).arg(commitHash, decoration, authorName, commitDate, commitSubject);

    return {logArgs};
}

void GitClient::log(const QString &workingDirectory, const QString &fileName,
                    bool enableAnnotationContextMenu, const QStringList &args)
{
    QString msgArg;
    if (!fileName.isEmpty())
        msgArg = fileName;
    else if (!args.isEmpty() && !args.first().startsWith('-'))
        msgArg = args.first();
    else
        msgArg = workingDirectory;
    // Creating document might change the referenced workingDirectory. Store a copy and use it.
    const QString workingDir = workingDirectory;
    const QString title = tr("Git Log \"%1\"").arg(msgArg);
    const Id editorId = Git::Constants::GIT_LOG_EDITOR_ID;
    const QString sourceFile = VcsBaseEditor::getSource(workingDir, fileName);
    GitEditorWidget *editor = static_cast<GitEditorWidget *>(
                createVcsEditor(editorId, title, sourceFile,
                                codecFor(CodecLogOutput), "logTitle", msgArg));
    VcsBaseEditorConfig *argWidget = editor->editorConfig();
    if (!argWidget) {
        argWidget = new GitLogArgumentsWidget(settings(), !fileName.isEmpty(), editor);
        argWidget->setBaseArguments(args);
        connect(argWidget, &VcsBaseEditorConfig::commandExecutionRequested, this,
                [=]() { this->log(workingDir, fileName, enableAnnotationContextMenu, args); });
        editor->setEditorConfig(argWidget);
    }
    editor->setFileLogAnnotateEnabled(enableAnnotationContextMenu);
    editor->setWorkingDirectory(workingDir);

    QStringList arguments = {"log", decorateOption};
    int logCount = settings().intValue(GitSettings::logCountKey);
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

    const QString grepValue = editor->grepValue();
    if (!grepValue.isEmpty())
        arguments << "--grep=" + grepValue;

    const QString pickaxeValue = editor->pickaxeValue();
    if (!pickaxeValue.isEmpty())
        arguments << "-S" << pickaxeValue;

    if ((!grepValue.isEmpty() || !pickaxeValue.isEmpty()) && !editor->caseSensitive())
        arguments << "-i";

    if (!fileName.isEmpty())
        arguments << "--" << fileName;

    vcsExec(workingDir, arguments, editor);
}

void GitClient::reflog(const QString &workingDirectory, const QString &ref)
{
    const QString title = tr("Git Reflog \"%1\"").arg(workingDirectory);
    const Id editorId = Git::Constants::GIT_REFLOG_EDITOR_ID;
    // Creating document might change the referenced workingDirectory. Store a copy and use it.
    const QString workingDir = workingDirectory;
    GitEditorWidget *editor = static_cast<GitEditorWidget *>(
                createVcsEditor(editorId, title, workingDir, codecFor(CodecLogOutput),
                                "reflogRepository", workingDir));
    VcsBaseEditorConfig *argWidget = editor->editorConfig();
    if (!argWidget) {
        argWidget = new GitRefLogArgumentsWidget(settings(), editor);
        if (!ref.isEmpty())
            argWidget->setBaseArguments({ref});
        connect(argWidget, &VcsBaseEditorConfig::commandExecutionRequested, this,
                [=] { this->reflog(workingDir, ref); });
        editor->setEditorConfig(argWidget);
    }
    editor->setWorkingDirectory(workingDir);

    QStringList arguments = {"reflog", noColorOption, decorateOption};
    arguments << argWidget->arguments();
    int logCount = settings().intValue(GitSettings::logCountKey);
    if (logCount > 0)
        arguments << "-n" << QString::number(logCount);

    vcsExec(workingDir, arguments, editor);
}

// Do not show "0000" or "^32ae4"
static inline bool canShow(const QString &sha)
{
    return !sha.startsWith('^') && sha.count('0') != sha.size();
}

static inline QString msgCannotShow(const QString &sha)
{
    return GitClient::tr("Cannot describe \"%1\".").arg(sha);
}

void GitClient::show(const QString &source, const QString &id, const QString &name)
{
    if (!canShow(id)) {
        VcsOutputWindow::appendError(msgCannotShow(id));
        return;
    }

    const QString title = tr("Git Show \"%1\"").arg(name.isEmpty() ? id : name);
    const QFileInfo sourceFi(source);
    QString workingDirectory = sourceFi.isDir()
            ? sourceFi.absoluteFilePath() : sourceFi.absolutePath();
    const QString repoDirectory = VcsManager::findTopLevelForDirectory(workingDirectory);
    if (!repoDirectory.isEmpty())
        workingDirectory = repoDirectory;
    const QString documentId = QLatin1String(Constants::GIT_PLUGIN)
            + QLatin1String(".Show.") + id;
    requestReload(documentId, source, title, workingDirectory,
                  [id](IDocument *doc) { return new ShowController(doc, id); });
}

void GitClient::archive(const QString &workingDirectory, QString commit)
{
    QString repoDirectory = VcsManager::findTopLevelForDirectory(workingDirectory);
    if (repoDirectory.isEmpty())
        repoDirectory = workingDirectory;
    QString repoName = QFileInfo(repoDirectory).fileName();

    QHash<QString, QString> filters {
        { tr("Tarball (*.tar.gz)"), ".tar.gz" },
        { tr("Zip archive (*.zip)"), ".zip" }
    };
    QString selectedFilter;
    if (HostOsInfo::isWindowsHost())
        selectedFilter = filters.key(".zip");
    else
        selectedFilter = filters.key(".tar.gz");

    QString output;
    if (synchronousRevParseCmd(repoDirectory, commit, &output))
        commit = output.trimmed();

    QString archiveName = QFileDialog::getSaveFileName(
                ICore::dialogParent(),
                tr("Generate %1 archive").arg(repoName),
                repoDirectory + QString("/%1-%2").arg(repoName).arg(commit.left(8)),
                filters.keys().join(";;"),
                &selectedFilter);
    if (archiveName.isEmpty())
        return;
    QString extension = filters.value(selectedFilter);
    QFileInfo archive(archiveName);
    if (archive.completeSuffix() != extension) {
        archive = QFileInfo(archive.absoluteDir().absoluteFilePath(archive.baseName() + extension));
    }

    if (archive.exists()) {
        if (QMessageBox::warning(ICore::dialogParent(), tr("Overwrite?"),
            tr("An item named \"%1\" already exists at this location. "
               "Do you want to overwrite it?").arg(QDir::toNativeSeparators(archive.absoluteFilePath())),
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
            return;
        }
    }

    vcsExec(workingDirectory, {"archive", commit, "-o", archive.absoluteFilePath()}, nullptr, true);
}

VcsBaseEditorWidget *GitClient::annotate(
        const QString &workingDir, const QString &file, const QString &revision,
        int lineNumber, const QStringList &extraOptions)
{
    const Id editorId = Git::Constants::GIT_BLAME_EDITOR_ID;
    const QString id = VcsBaseEditor::getTitleId(workingDir, {file}, revision);
    const QString title = tr("Git Blame \"%1\"").arg(id);
    const QString sourceFile = VcsBaseEditor::getSource(workingDir, file);

    VcsBaseEditorWidget *editor
            = createVcsEditor(editorId, title, sourceFile, codecFor(CodecSource, sourceFile),
                              "blameFileName", id);
    VcsBaseEditorConfig *argWidget = editor->editorConfig();
    if (!argWidget) {
        argWidget = new GitBlameArgumentsWidget(settings(), editor->toolBar());
        argWidget->setBaseArguments(extraOptions);
        connect(argWidget, &VcsBaseEditorConfig::commandExecutionRequested, this,
                [=] {
                    const int line = VcsBaseEditor::lineNumberOfCurrentEditor();
                    annotate(workingDir, file, revision, line, extraOptions);
                } );
        editor->setEditorConfig(argWidget);
    }

    editor->setWorkingDirectory(workingDir);
    QStringList arguments = {"blame", "--root"};
    arguments << argWidget->arguments() << "--" << file;
    if (!revision.isEmpty())
        arguments << revision;
    vcsExec(workingDir, arguments, editor, false, 0, lineNumber);
    return editor;
}

VcsCommand *GitClient::checkout(const QString &workingDirectory, const QString &ref,
                                StashMode stashMode)
{
    if (stashMode == StashMode::TryStash && !beginStashScope(workingDirectory, "Checkout"))
        return nullptr;
    QStringList arguments = setupCheckoutArguments(workingDirectory, ref);
    VcsCommand *command = vcsExec(
                workingDirectory, arguments, nullptr, true,
                VcsCommand::ExpectRepoChanges | VcsCommand::ShowSuccessMessage);
    connect(command, &VcsCommand::finished,
            this, [this, workingDirectory, stashMode](bool success) {
        if (stashMode == StashMode::TryStash)
            endStashScope(workingDirectory);
        if (success)
            updateSubmodulesIfNeeded(workingDirectory, true);
    });
    return command;
}

/* method used to setup arguments for checkout, in case user wants to create local branch */
QStringList GitClient::setupCheckoutArguments(const QString &workingDirectory,
                                              const QString &ref)
{
    QStringList arguments = {"checkout", ref};

    QStringList localBranches = synchronousRepositoryBranches(workingDirectory);
    if (localBranches.contains(ref))
        return arguments;

    if (Utils::CheckableMessageBox::doNotAskAgainQuestion(
                ICore::dialogParent() /*parent*/,
                tr("Create Local Branch") /*title*/,
                tr("Would you like to create a local branch?") /*message*/,
                ICore::settings(), "Git.CreateLocalBranchOnCheckout" /*setting*/,
                QDialogButtonBox::Yes | QDialogButtonBox::No /*buttons*/,
                QDialogButtonBox::No /*default button*/,
                QDialogButtonBox::No /*button to save*/) != QDialogButtonBox::Yes) {
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

    const QString suggestedName = suggestedLocalBranchName(localBranches, remoteBranch);
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

void GitClient::reset(const QString &workingDirectory, const QString &argument, const QString &commit)
{
    QStringList arguments = {"reset", argument};
    if (!commit.isEmpty())
        arguments << commit;

    unsigned flags = VcsCommand::ShowSuccessMessage;
    if (argument == "--hard") {
        if (gitStatus(workingDirectory, StatusMode(NoUntracked | NoSubmodules)) != StatusUnchanged) {
            if (QMessageBox::question(
                        Core::ICore::dialogParent(), tr("Reset"),
                        tr("All changes in working directory will be discarded. Are you sure?"),
                        QMessageBox::Yes | QMessageBox::No,
                        QMessageBox::No) == QMessageBox::No) {
                return;
            }
        }
        flags |= VcsCommand::ExpectRepoChanges;
    }
    vcsExec(workingDirectory, arguments, nullptr, true, flags);
}

void GitClient::removeStaleRemoteBranches(const QString &workingDirectory, const QString &remote)
{
    const QStringList arguments = {"remote", "prune", remote};

    VcsCommand *command = vcsExec(workingDirectory, arguments, nullptr, true,
                                  VcsCommand::ShowSuccessMessage);

    connect(command, &VcsCommand::success,
            this, [workingDirectory]() { GitPlugin::updateBranches(workingDirectory); });
}

void GitClient::recoverDeletedFiles(const QString &workingDirectory)
{
    const SynchronousProcessResponse response =
            vcsFullySynchronousExec(workingDirectory, {"ls-files", "--deleted"},
                                    VcsCommand::SuppressCommandLogging);
    if (response.result == SynchronousProcessResponse::Finished) {
        const QString stdOut = response.stdOut().trimmed();
        if (stdOut.isEmpty()) {
            VcsOutputWindow::appendError(tr("Nothing to recover"));
            return;
        }
        const QStringList files = stdOut.split('\n');
        synchronousCheckoutFiles(workingDirectory, files, QString(), nullptr, false);
        VcsOutputWindow::append(tr("Files recovered"), VcsOutputWindow::Message);
    }
}

void GitClient::addFile(const QString &workingDirectory, const QString &fileName)
{
    vcsExec(workingDirectory, {"add", fileName});
}

bool GitClient::synchronousLog(const QString &workingDirectory, const QStringList &arguments,
                               QString *output, QString *errorMessageIn, unsigned flags)
{
    QStringList allArguments = {"log", noColorOption};

    allArguments.append(arguments);

    const SynchronousProcessResponse resp = vcsFullySynchronousExec(
                workingDirectory, allArguments, flags, vcsTimeoutS(),
                encoding(workingDirectory, "i18n.logOutputEncoding"));
    if (resp.result == SynchronousProcessResponse::Finished) {
        *output = resp.stdOut();
        return true;
    } else {
        msgCannotRun(tr("Cannot obtain log of \"%1\": %2")
                     .arg(QDir::toNativeSeparators(workingDirectory), resp.stdErr()), errorMessageIn);
        return false;
    }
}

bool GitClient::synchronousAdd(const QString &workingDirectory, const QStringList &files)
{
    return vcsFullySynchronousExec(workingDirectory, QStringList({"add"}) + files).result
            == SynchronousProcessResponse::Finished;
}

bool GitClient::synchronousDelete(const QString &workingDirectory,
                                  bool force,
                                  const QStringList &files)
{
    QStringList arguments = {"rm"};
    if (force)
        arguments << "--force";
    arguments.append(files);
    return vcsFullySynchronousExec(workingDirectory, arguments).result
            == SynchronousProcessResponse::Finished;
}

bool GitClient::synchronousMove(const QString &workingDirectory,
                                const QString &from,
                                const QString &to)
{
    return vcsFullySynchronousExec(workingDirectory, {"mv", from, to}).result
            == SynchronousProcessResponse::Finished;
}

bool GitClient::synchronousReset(const QString &workingDirectory,
                                 const QStringList &files,
                                 QString *errorMessage)
{
    QStringList arguments = {"reset"};
    if (files.isEmpty())
        arguments << "--hard";
    else
        arguments << HEAD << "--" << files;

    const SynchronousProcessResponse resp = vcsFullySynchronousExec(workingDirectory, arguments);
    const QString stdOut = resp.stdOut();
    VcsOutputWindow::append(stdOut);
    // Note that git exits with 1 even if the operation is successful
    // Assume real failure if the output does not contain "foo.cpp modified"
    // or "Unstaged changes after reset" (git 1.7.0).
    if (resp.result != SynchronousProcessResponse::Finished
        && (!stdOut.contains("modified") && !stdOut.contains("Unstaged changes after reset"))) {
        if (files.isEmpty()) {
            msgCannotRun(arguments, workingDirectory, resp.stdErr(), errorMessage);
        } else {
            msgCannotRun(tr("Cannot reset %n files in \"%1\": %2", nullptr, files.size())
                         .arg(QDir::toNativeSeparators(workingDirectory), resp.stdErr()),
                         errorMessage);
        }
        return false;
    }
    return true;
}

// Initialize repository
bool GitClient::synchronousInit(const QString &workingDirectory)
{
    const SynchronousProcessResponse resp = vcsFullySynchronousExec(workingDirectory,
                                                                    QStringList{"init"});
    // '[Re]Initialized...'
    VcsOutputWindow::append(resp.stdOut());
    if (resp.result == SynchronousProcessResponse::Finished) {
        resetCachedVcsInfo(workingDirectory);
        return true;
    } else {
        return false;
    }
}

/* Checkout, supports:
 * git checkout -- <files>
 * git checkout revision -- <files>
 * git checkout revision -- . */
bool GitClient::synchronousCheckoutFiles(const QString &workingDirectory, QStringList files,
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
    const SynchronousProcessResponse resp = vcsFullySynchronousExec(
                workingDirectory, arguments, VcsCommand::ExpectRepoChanges);
    if (resp.result != SynchronousProcessResponse::Finished) {
        const QString fileArg = files.join(", ");
        //: Meaning of the arguments: %1: revision, %2: files, %3: repository,
        //: %4: Error message
        msgCannotRun(tr("Cannot checkout \"%1\" of %2 in \"%3\": %4")
                     .arg(revision, fileArg, workingDirectory, resp.stdErr()),
                     errorMessage);
        return false;
    }
    return true;
}

static inline QString msgParentRevisionFailed(const QString &workingDirectory,
                                              const QString &revision,
                                              const QString &why)
{
    //: Failed to find parent revisions of a SHA1 for "annotate previous"
    return GitClient::tr("Cannot find parent revisions of \"%1\" in \"%2\": %3").arg(revision, workingDirectory, why);
}

static inline QString msgInvalidRevision()
{
    return GitClient::tr("Invalid revision");
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

bool GitClient::synchronousRevListCmd(const QString &workingDirectory, const QStringList &extraArguments,
                                      QString *output, QString *errorMessage) const
{
    const QStringList arguments = QStringList({"rev-list", noColorOption}) + extraArguments;
    const SynchronousProcessResponse resp = vcsFullySynchronousExec(
                workingDirectory, arguments, silentFlags);
    if (resp.result != SynchronousProcessResponse::Finished) {
        msgCannotRun(arguments, workingDirectory, resp.stdErr(), errorMessage);
        return false;
    }
    *output = resp.stdOut();
    return true;
}

// Find out the immediate parent revisions of a revision of the repository.
// Might be several in case of merges.
bool GitClient::synchronousParentRevisions(const QString &workingDirectory,
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

QString GitClient::synchronousShortDescription(const QString &workingDirectory, const QString &revision) const
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

QString GitClient::synchronousCurrentLocalBranch(const QString &workingDirectory) const
{
    QString branch;
    const SynchronousProcessResponse resp = vcsFullySynchronousExec(
                workingDirectory, {"symbolic-ref", HEAD}, silentFlags);
    if (resp.result == SynchronousProcessResponse::Finished) {
        branch = resp.stdOut().trimmed();
    } else {
        const QString gitDir = findGitDirForRepository(workingDirectory);
        const QString rebaseHead = gitDir + "/rebase-merge/head-name";
        QFile head(rebaseHead);
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
    return QString();
}

bool GitClient::synchronousHeadRefs(const QString &workingDirectory, QStringList *output,
                                    QString *errorMessage) const
{
    const QStringList arguments = {"show-ref", "--head", "--abbrev=10", "--dereference"};
    const SynchronousProcessResponse resp = vcsFullySynchronousExec(
                workingDirectory, arguments, silentFlags);
    if (resp.result != SynchronousProcessResponse::Finished) {
        msgCannotRun(arguments, workingDirectory, resp.stdErr(), errorMessage);
        return false;
    }

    const QString stdOut = resp.stdOut();
    const QString headSha = stdOut.left(10);
    QString rest = stdOut.mid(15);

    const QStringList headShaLines = Utils::filtered(
                rest.split('\n'), [&headSha](const QString &s) { return s.startsWith(headSha); });
    *output = Utils::transform(headShaLines, [](const QString &s) { return s.mid(11); }); // sha + space

    return true;
}

// Retrieve topic (branch, tag or HEAD hash)
QString GitClient::synchronousTopic(const QString &workingDirectory) const
{
    // First try to find branch
    QString branch = synchronousCurrentLocalBranch(workingDirectory);
    if (!branch.isEmpty())
        return branch;

    // Detached HEAD, try a tag or remote branch
    QStringList references;
    if (!synchronousHeadRefs(workingDirectory, &references))
        return QString();

    const QString tagStart("refs/tags/");
    const QString remoteStart("refs/remotes/");
    const QString dereference("^{}");
    QString remoteBranch;

    for (const QString &ref : qAsConst(references)) {
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
    const SynchronousProcessResponse resp =
        vcsFullySynchronousExec(workingDirectory, QStringList{"describe"}, VcsCommand::NoOutput);
    if (resp.result == SynchronousProcessResponse::Finished) {
        const QString stdOut = resp.stdOut().trimmed();
        if (!stdOut.isEmpty())
            return stdOut;
    }
    return tr("Detached HEAD");
}

bool GitClient::synchronousRevParseCmd(const QString &workingDirectory, const QString &ref,
                                       QString *output, QString *errorMessage) const
{
    const QStringList arguments = {"rev-parse", ref};
    const SynchronousProcessResponse resp = vcsFullySynchronousExec(
                workingDirectory, arguments, silentFlags);
    *output = resp.stdOut().trimmed();
    if (resp.result != SynchronousProcessResponse::Finished) {
        msgCannotRun(arguments, workingDirectory, resp.stdErr(), errorMessage);
        return false;
    }

    return true;
}

// Retrieve head revision
QString GitClient::synchronousTopRevision(const QString &workingDirectory, QDateTime *dateTime)
{
    const QStringList arguments = {"show", "-s", "--pretty=format:%H:%ct", HEAD};
    const SynchronousProcessResponse resp = vcsFullySynchronousExec(
                workingDirectory, arguments, silentFlags);
    if (resp.result != SynchronousProcessResponse::Finished)
        return QString();
    const QStringList output = resp.stdOut().trimmed().split(':');
    if (dateTime && output.size() > 1) {
        bool ok = false;
        const qint64 timeT = output.at(1).toLongLong(&ok);
        *dateTime = ok ? QDateTime::fromSecsSinceEpoch(timeT) : QDateTime();
    }
    return output.first();
}

void GitClient::synchronousTagsForCommit(const QString &workingDirectory, const QString &revision,
                                         QString &precedes, QString &follows) const
{
    const SynchronousProcessResponse resp1 = vcsFullySynchronousExec(
                workingDirectory, {"describe", "--contains", revision}, silentFlags);
    precedes = resp1.stdOut();
    int tilde = precedes.indexOf('~');
    if (tilde != -1)
        precedes.truncate(tilde);
    else
        precedes = precedes.trimmed();

    QStringList parents;
    QString errorMessage;
    synchronousParentRevisions(workingDirectory, revision, &parents, &errorMessage);
    for (const QString &p : qAsConst(parents)) {
        const SynchronousProcessResponse resp2 = vcsFullySynchronousExec(
                    workingDirectory, {"describe", "--tags", "--abbrev=0", p}, silentFlags);
        QString pf = resp2.stdOut();
        pf.truncate(pf.lastIndexOf('\n'));
        if (!pf.isEmpty()) {
            if (!follows.isEmpty())
                follows += ", ";
            follows += pf;
        }
    }
}

bool GitClient::isRemoteCommit(const QString &workingDirectory, const QString &commit)
{
    return !vcsFullySynchronousExec(
                workingDirectory, {"branch", "-r", "--contains", commit}, silentFlags).rawStdOut.isEmpty();
}

bool GitClient::isFastForwardMerge(const QString &workingDirectory, const QString &branch)
{
    const SynchronousProcessResponse resp = vcsFullySynchronousExec(
                workingDirectory, {"merge-base", HEAD, branch}, silentFlags);
    return resp.stdOut().trimmed() == synchronousTopRevision(workingDirectory);
}

// Format an entry in a one-liner for selection list using git log.
QString GitClient::synchronousShortDescription(const QString &workingDirectory, const QString &revision,
                                            const QString &format) const
{
    const QStringList arguments = {"log", noColorOption, ("--pretty=format:" + format),
                                   "--max-count=1", revision};
    const SynchronousProcessResponse resp = vcsFullySynchronousExec(
                workingDirectory, arguments, silentFlags);
    if (resp.result != SynchronousProcessResponse::Finished) {
        VcsOutputWindow::appendSilently(tr("Cannot describe revision \"%1\" in \"%2\": %3")
                                        .arg(revision, workingDirectory, resp.stdErr()));
        return revision;
    }
    return stripLastNewline(resp.stdOut());
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

QString GitClient::synchronousStash(const QString &workingDirectory, const QString &messageKeyword,
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
                               tr("Stash Description"), tr("Description:"), &message))
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

bool GitClient::executeSynchronousStash(const QString &workingDirectory,
                                        const QString &message,
                                        bool unstagedOnly,
                                        QString *errorMessage) const
{
    QStringList arguments = {"stash", "save"};
    if (unstagedOnly)
        arguments << "--keep-index";
    if (!message.isEmpty())
        arguments << message;
    const unsigned flags = VcsCommand::ShowStdOut
            | VcsCommand::ExpectRepoChanges
            | VcsCommand::ShowSuccessMessage;
    const SynchronousProcessResponse resp = vcsSynchronousExec(workingDirectory, arguments, flags);
    if (resp.result != SynchronousProcessResponse::Finished) {
        msgCannotRun(arguments, workingDirectory, resp.stdErr(), errorMessage);
        return false;
    }

    return true;
}

// Resolve a stash name from message
bool GitClient::stashNameFromMessage(const QString &workingDirectory,
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
    for (const Stash &s : qAsConst(stashes)) {
        if (s.message == message) {
            *name = s.name;
            return true;
        }
    }
    //: Look-up of a stash via its descriptive message failed.
    msgCannotRun(tr("Cannot resolve stash message \"%1\" in \"%2\".")
                 .arg(message, workingDirectory), errorMessage);
    return  false;
}

bool GitClient::synchronousBranchCmd(const QString &workingDirectory, QStringList branchArgs,
                                     QString *output, QString *errorMessage) const
{
    branchArgs.push_front("branch");
    const SynchronousProcessResponse resp = vcsFullySynchronousExec(workingDirectory, branchArgs);
    *output = resp.stdOut();
    if (resp.result != SynchronousProcessResponse::Finished) {
        msgCannotRun(branchArgs, workingDirectory, resp.stdErr(), errorMessage);
        return false;
    }
    return true;
}

bool GitClient::synchronousTagCmd(const QString &workingDirectory, QStringList tagArgs,
                                  QString *output, QString *errorMessage) const
{
    tagArgs.push_front("tag");
    const SynchronousProcessResponse resp = vcsFullySynchronousExec(workingDirectory, tagArgs);
    *output = resp.stdOut();
    if (resp.result != SynchronousProcessResponse::Finished) {
        msgCannotRun(tagArgs, workingDirectory, resp.stdErr(), errorMessage);
        return false;
    }
    return true;
}

bool GitClient::synchronousForEachRefCmd(const QString &workingDirectory, QStringList args,
                                      QString *output, QString *errorMessage) const
{
    args.push_front("for-each-ref");
    const SynchronousProcessResponse resp = vcsFullySynchronousExec(workingDirectory, args,
                                                                    silentFlags);
    *output = resp.stdOut();
    if (resp.result != SynchronousProcessResponse::Finished) {
        msgCannotRun(args, workingDirectory, resp.stdErr(), errorMessage);
        return false;
    }
    return true;
}

VcsCommand *GitClient::asyncForEachRefCmd(const QString &workingDirectory, QStringList args) const
{
    args.push_front("for-each-ref");
    return vcsExec(workingDirectory, args, nullptr, false, silentFlags);
}

bool GitClient::synchronousRemoteCmd(const QString &workingDirectory, QStringList remoteArgs,
                                     QString *output, QString *errorMessage, bool silent) const
{
    remoteArgs.push_front("remote");
    const SynchronousProcessResponse resp = vcsFullySynchronousExec(workingDirectory, remoteArgs,
                                                                    silent ? silentFlags : 0);

    const QString stdErr = resp.stdErr();
    *errorMessage = stdErr;
    *output = resp.stdOut();

    if (resp.result != SynchronousProcessResponse::Finished) {
        msgCannotRun(remoteArgs, workingDirectory, stdErr, errorMessage);
        return false;
    }
    return true;
}

QMap<QString,QString> GitClient::synchronousRemotesList(const QString &workingDirectory,
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

QStringList GitClient::synchronousSubmoduleStatus(const QString &workingDirectory,
                                                  QString *errorMessage) const
{
    // get submodule status
    const SynchronousProcessResponse resp =
         vcsFullySynchronousExec(workingDirectory, {"submodule", "status"}, silentFlags);

    if (resp.result != SynchronousProcessResponse::Finished) {
        msgCannotRun(tr("Cannot retrieve submodule status of \"%1\": %2")
                     .arg(QDir::toNativeSeparators(workingDirectory), resp.stdErr()), errorMessage);
        return QStringList();
    }
    return splitLines(resp.stdOut());
}

SubmoduleDataMap GitClient::submoduleList(const QString &workingDirectory) const
{
    SubmoduleDataMap result;
    QString gitmodulesFileName = workingDirectory + "/.gitmodules";
    if (!QFile::exists(gitmodulesFileName))
        return result;

    static QMap<QString, SubmoduleDataMap> cachedSubmoduleData;

    if (cachedSubmoduleData.contains(workingDirectory))
        return cachedSubmoduleData.value(workingDirectory);

    const QStringList allConfigs = readConfigValue(workingDirectory, "-l").split('\n');
    const QString submoduleLineStart = "submodule.";
    for (const QString &configLine : allConfigs) {
        if (!configLine.startsWith(submoduleLineStart))
            continue;

        int nameStart = submoduleLineStart.size();
        int nameEnd   = configLine.indexOf('.', nameStart);

        QString submoduleName = configLine.mid(nameStart, nameEnd - nameStart);

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
        QSettings gitmodulesFile(gitmodulesFileName, QSettings::IniFormat);

        const QList<QString> submodules = result.keys();
        for (const QString &submoduleName : submodules) {
            gitmodulesFile.beginGroup("submodule \"" + submoduleName + '"');
            const QString path = gitmodulesFile.value("path").toString();
            if (path.isEmpty()) { // invalid submodule entry in config
                result.remove(submoduleName);
            } else {
                SubmoduleData &submoduleRef = result[submoduleName];
                submoduleRef.dir = path;
                QString ignore = gitmodulesFile.value("ignore").toString();
                if (!ignore.isEmpty() && submoduleRef.ignore.isEmpty())
                    submoduleRef.ignore = ignore;
            }
            gitmodulesFile.endGroup();
        }
    }
    cachedSubmoduleData.insert(workingDirectory, result);

    return result;
}

bool GitClient::synchronousShow(const QString &workingDirectory, const QString &id,
                                QByteArray *output, QString *errorMessage) const
{
    if (!canShow(id)) {
        *errorMessage = msgCannotShow(id);
        return false;
    }
    const QStringList arguments = {"show", decorateOption, noColorOption, id};
    const SynchronousProcessResponse resp = vcsFullySynchronousExec(workingDirectory, arguments);
    if (resp.result != SynchronousProcessResponse::Finished) {
        msgCannotRun(arguments, workingDirectory, resp.stdErr(), errorMessage);
        return false;
    }
    *output = resp.rawStdOut;
    return true;
}

// Retrieve list of files to be cleaned
bool GitClient::cleanList(const QString &workingDirectory, const QString &modulePath,
                          const QString &flag, QStringList *files, QString *errorMessage)
{
    const QString directory = workingDirectory + '/' + modulePath;
    const QStringList arguments = {"clean", "--dry-run", flag};

    const SynchronousProcessResponse resp = vcsFullySynchronousExec(directory, arguments,
                                                                    VcsCommand::ForceCLocale);
    if (resp.result != SynchronousProcessResponse::Finished) {
        msgCannotRun(arguments, directory, resp.stdErr(), errorMessage);
        return false;
    }

    // Filter files that git would remove
    const QString relativeBase = modulePath.isEmpty() ? QString() : modulePath + '/';
    const QString prefix = "Would remove ";
    const QStringList removeLines = Utils::filtered(
                splitLines(resp.stdOut()), [](const QString &s) {
        return s.startsWith("Would remove ");
    });
    *files = Utils::transform(removeLines, [&relativeBase, &prefix](const QString &s) -> QString {
        return relativeBase + s.mid(prefix.size());
    });
    return true;
}

bool GitClient::synchronousCleanList(const QString &workingDirectory, const QString &modulePath,
                                     QStringList *files, QStringList *ignoredFiles,
                                     QString *errorMessage)
{
    bool res = cleanList(workingDirectory, modulePath, "-df", files, errorMessage);
    res &= cleanList(workingDirectory, modulePath, "-dXf", ignoredFiles, errorMessage);

    const SubmoduleDataMap submodules = submoduleList(workingDirectory + '/' + modulePath);
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

bool GitClient::synchronousApplyPatch(const QString &workingDirectory,
                                      const QString &file, QString *errorMessage,
                                      const QStringList &extraArguments)
{
    QStringList arguments = {"apply", "--whitespace=fix"};
    arguments << extraArguments << file;

    const SynchronousProcessResponse resp = vcsFullySynchronousExec(workingDirectory, arguments);
    const QString stdErr = resp.stdErr();
    if (resp.result == SynchronousProcessResponse::Finished) {
        if (!stdErr.isEmpty())
            *errorMessage = tr("There were warnings while applying \"%1\" to \"%2\":\n%3")
                .arg(file, workingDirectory, stdErr);
        return true;
    } else {
        *errorMessage = tr("Cannot apply patch \"%1\" to \"%2\": %3")
                .arg(QDir::toNativeSeparators(file), workingDirectory, stdErr);
        return false;
    }
}

QProcessEnvironment GitClient::processEnvironment() const
{
    QProcessEnvironment environment = VcsBaseClientImpl::processEnvironment();
    QString gitPath = settings().stringValue(GitSettings::pathKey);
    if (!gitPath.isEmpty()) {
        gitPath += HostOsInfo::pathListSeparator();
        gitPath += environment.value("PATH");
        environment.insert("PATH", gitPath);
    }
    if (HostOsInfo::isWindowsHost()
            && settings().boolValue(GitSettings::winSetHomeEnvironmentKey)) {
        environment.insert("HOME", QDir::toNativeSeparators(QDir::homePath()));
    }
    environment.insert("GIT_EDITOR", m_disableEditor ? "true" : m_gitQtcEditor);
    return environment;
}

bool GitClient::beginStashScope(const QString &workingDirectory, const QString &command,
                                StashFlag flag, PushAction pushAction)
{
    const QString repoDirectory = VcsManager::findTopLevelForDirectory(workingDirectory);
    QTC_ASSERT(!repoDirectory.isEmpty(), return false);
    StashInfo &stashInfo = m_stashInfo[repoDirectory];
    return stashInfo.init(repoDirectory, command, flag, pushAction);
}

GitClient::StashInfo &GitClient::stashInfo(const QString &workingDirectory)
{
    const QString repoDirectory = VcsManager::findTopLevelForDirectory(workingDirectory);
    QTC_CHECK(m_stashInfo.contains(repoDirectory));
    return m_stashInfo[repoDirectory];
}

void GitClient::endStashScope(const QString &workingDirectory)
{
    const QString repoDirectory = VcsManager::findTopLevelForDirectory(workingDirectory);
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

void GitClient::updateSubmodulesIfNeeded(const QString &workingDirectory, bool prompt)
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

    if (prompt && QMessageBox::question(ICore::dialogParent(), tr("Submodules Found"),
            tr("Would you like to update submodules?"),
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
        const QString submoduleDir = workingDirectory + '/' + statusLine.mid(nameStart, nameLength);

        if (beginStashScope(submoduleDir, "SubmoduleUpdate")) {
            m_updatedSubmodules.append(submoduleDir);
        } else {
            finishSubmoduleUpdate();
            return;
        }
    }

    VcsCommand *cmd = vcsExec(workingDirectory, {"submodule", "update"}, nullptr, true,
                              VcsCommand::ExpectRepoChanges);
    connect(cmd, &VcsCommand::finished, this, &GitClient::finishSubmoduleUpdate);
}

void GitClient::finishSubmoduleUpdate()
{
    for (const QString &submoduleDir : qAsConst(m_updatedSubmodules))
        endStashScope(submoduleDir);
    m_updatedSubmodules.clear();
}

GitClient::StatusResult GitClient::gitStatus(const QString &workingDirectory, StatusMode mode,
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

    const SynchronousProcessResponse resp = vcsFullySynchronousExec(
                workingDirectory, arguments, silentFlags);
    const QString stdOut = resp.stdOut();

    if (output)
        *output = stdOut;

    const bool statusRc = resp.result == SynchronousProcessResponse::Finished;
    const bool branchKnown = !stdOut.startsWith("## HEAD (no branch)\n");
    // Is it something really fatal?
    if (!statusRc && !branchKnown) {
        if (errorMessage) {
            *errorMessage = tr("Cannot obtain status: %1").arg(resp.stdErr());
        }
        return StatusFailed;
    }
    // Unchanged (output text depending on whether -u was passed)
    const bool hasChanges = Utils::contains(stdOut.split('\n'), [](const QString &s) {
                                                return !s.isEmpty() && !s.startsWith('#');
                                            });
    return hasChanges ? StatusChanged : StatusUnchanged;
}

QString GitClient::commandInProgressDescription(const QString &workingDirectory) const
{
    switch (checkCommandInProgress(workingDirectory)) {
    case NoCommand:
        break;
    case Rebase:
    case RebaseMerge:
        return tr("REBASING");
    case Revert:
        return tr("REVERTING");
    case CherryPick:
        return tr("CHERRY-PICKING");
    case Merge:
        return tr("MERGING");
    }
    return QString();
}

GitClient::CommandInProgress GitClient::checkCommandInProgress(const QString &workingDirectory) const
{
    const QString gitDir = findGitDirForRepository(workingDirectory);
    if (QFile::exists(gitDir + "/MERGE_HEAD"))
        return Merge;
    else if (QFile::exists(gitDir + "/rebase-apply"))
        return Rebase;
    else if (QFile::exists(gitDir + "/rebase-merge"))
        return RebaseMerge;
    else if (QFile::exists(gitDir + "/REVERT_HEAD"))
        return Revert;
    else if (QFile::exists(gitDir + "/CHERRY_PICK_HEAD"))
        return CherryPick;
    else
        return NoCommand;
}

void GitClient::continueCommandIfNeeded(const QString &workingDirectory, bool allowContinue)
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
        continuePreviousGitCommand(workingDirectory, tr("Continue Rebase"),
                                   tr("Rebase is in progress. What do you want to do?"),
                                   tr("Continue"), "rebase", continueMode);
        break;
    case Merge:
        continuePreviousGitCommand(workingDirectory, tr("Continue Merge"),
                tr("You need to commit changes to finish merge.\nCommit now?"),
                tr("Commit"), "merge", continueMode);
        break;
    case Revert:
        continuePreviousGitCommand(workingDirectory, tr("Continue Revert"),
                tr("You need to commit changes to finish revert.\nCommit now?"),
                tr("Commit"), "revert", continueMode);
        break;
    case CherryPick:
        continuePreviousGitCommand(workingDirectory, tr("Continue Cherry-Picking"),
                tr("You need to commit changes to finish cherry-picking.\nCommit now?"),
                tr("Commit"), "cherry-pick", continueMode);
        break;
    default:
        break;
    }
}

void GitClient::continuePreviousGitCommand(const QString &workingDirectory,
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
            msgBoxText.prepend(tr("No changes found.") + ' ');
        break;
    case SkipOnly:
        hasChanges = false;
        break;
    }

    QMessageBox msgBox(QMessageBox::Question, msgBoxTitle, msgBoxText,
                       QMessageBox::NoButton, ICore::dialogParent());
    if (hasChanges || isRebase)
        msgBox.addButton(hasChanges ? buttonName : tr("Skip"), QMessageBox::AcceptRole);
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

QString GitClient::extendedShowDescription(const QString &workingDirectory, const QString &text) const
{
    if (!text.startsWith("commit "))
        return text;
    QString modText = text;
    QString precedes, follows;
    int lastHeaderLine = modText.indexOf("\n\n") + 1;
    const QString commit = modText.mid(7, 8);
    synchronousTagsForCommit(workingDirectory, commit, precedes, follows);
    if (!precedes.isEmpty())
        modText.insert(lastHeaderLine, "Precedes: " + precedes + '\n');
    if (!follows.isEmpty())
        modText.insert(lastHeaderLine, "Follows: " + follows + '\n');

    // Empty line before headers and commit message
    const int emptyLine = modText.indexOf("\n\n");
    if (emptyLine != -1)
        modText.insert(emptyLine, QString('\n') + Constants::EXPAND_BRANCHES);

    return modText;
}

// Quietly retrieve branch list of remote repository URL
//
// The branch HEAD is pointing to is always returned first.
QStringList GitClient::synchronousRepositoryBranches(const QString &repositoryURL,
                                                     const QString &workingDirectory) const
{
    const unsigned flags = VcsCommand::SshPasswordPrompt
            | VcsCommand::SuppressStdErr
            | VcsCommand::SuppressFailMessage;
    const SynchronousProcessResponse resp = vcsSynchronousExec(
                workingDirectory, {"ls-remote", repositoryURL, HEAD, "refs/heads/*"}, flags);
    QStringList branches;
    branches << tr("<Detached HEAD>");
    QString headSha;
    // split "82bfad2f51d34e98b18982211c82220b8db049b<tab>refs/heads/master"
    bool headFound = false;
    bool branchFound = false;
    const QStringList lines = resp.stdOut().split('\n');
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
            const QString branchName = line.mid(pos + pattern.count());
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

void GitClient::launchGitK(const QString &workingDirectory, const QString &fileName) const
{
    const QFileInfo binaryInfo = vcsBinary().toFileInfo();
    QDir foundBinDir(binaryInfo.dir());
    const bool foundBinDirIsBinDir = foundBinDir.dirName() == "bin";
    QProcessEnvironment env = processEnvironment();
    if (tryLauchingGitK(env, workingDirectory, fileName, foundBinDir.path()))
        return;

    QString gitkPath = foundBinDir.path() + "/gitk";
    VcsOutputWindow::appendSilently(msgCannotLaunch(gitkPath));

    if (foundBinDirIsBinDir) {
        foundBinDir.cdUp();
        const QString binDirName = foundBinDir.dirName();
        if (binDirName == "usr" || binDirName.startsWith("mingw"))
            foundBinDir.cdUp();
        if (tryLauchingGitK(env, workingDirectory, fileName,
                            foundBinDir.path() + "/cmd")) {
            return;
        }
        gitkPath = foundBinDir.path() + "/cmd/gitk";
        VcsOutputWindow::appendSilently(msgCannotLaunch(gitkPath));
    }

    Environment sysEnv = Environment::systemEnvironment();
    const FilePath exec = sysEnv.searchInPath("gitk");

    if (!exec.isEmpty() && tryLauchingGitK(env, workingDirectory, fileName,
                                           exec.parentDir().toString())) {
        return;
    }

    VcsOutputWindow::appendError(msgCannotLaunch("gitk"));
}

void GitClient::launchRepositoryBrowser(const QString &workingDirectory) const
{
    const QString repBrowserBinary = settings().stringValue(GitSettings::repositoryBrowserCmd);
    if (!repBrowserBinary.isEmpty())
        QProcess::startDetached(repBrowserBinary, {workingDirectory}, workingDirectory);
}

bool GitClient::tryLauchingGitK(const QProcessEnvironment &env,
                                const QString &workingDirectory,
                                const QString &fileName,
                                const QString &gitBinDirectory) const
{
    QString binary = gitBinDirectory + "/gitk";
    QStringList arguments;
    if (HostOsInfo::isWindowsHost()) {
        // If git/bin is in path, use 'wish' shell to run. Otherwise (git/cmd), directly run gitk
        QString wish = gitBinDirectory + "/wish";
        if (QFileInfo::exists(wish + ".exe")) {
            arguments << binary;
            binary = wish;
        }
    }
    const QString gitkOpts = settings().stringValue(GitSettings::gitkOptionsKey);
    if (!gitkOpts.isEmpty())
        arguments.append(QtcProcess::splitArgs(gitkOpts, HostOsInfo::hostOs()));
    if (!fileName.isEmpty())
        arguments << "--" << fileName;
    VcsOutputWindow::appendCommand(workingDirectory, {binary, arguments});
    // This should always use QProcess::startDetached (as not to kill
    // the child), but that does not have an environment parameter.
    bool success = false;
    if (!settings().stringValue(GitSettings::pathKey).isEmpty()) {
        auto process = new QProcess;
        process->setWorkingDirectory(workingDirectory);
        process->setProcessEnvironment(env);
        process->start(binary, arguments);
        success = process->waitForStarted();
        if (success)
            connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                    process, &QProcess::deleteLater);
        else
            delete process;
    } else {
        success = QProcess::startDetached(binary, arguments, workingDirectory);
    }

    return success;
}

bool GitClient::launchGitGui(const QString &workingDirectory) {
    bool success = true;
    FilePath gitBinary = vcsBinary();
    if (gitBinary.isEmpty()) {
        success = false;
    } else {
        success = QProcess::startDetached(gitBinary.toString(), {"gui"},
                                          workingDirectory);
    }

    if (!success)
        VcsOutputWindow::appendError(msgCannotLaunch("git gui"));

    return success;
}

FilePath GitClient::gitBinDirectory() const
{
    const QString git = vcsBinary().toString();
    if (git.isEmpty())
        return FilePath();

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
            if (QFile::exists(usrBinPath))
                path = usrBinPath;
        }
    }
    return FilePath::fromString(path);
}

bool GitClient::launchGitBash(const QString &workingDirectory)
{
    bool success = true;
    const QString git = vcsBinary().toString();

    if (git.isEmpty()) {
        success = false;
    } else {
        const QString gitBash = QFileInfo(git).absolutePath() + "/../git-bash.exe";
        success = QProcess::startDetached(gitBash, {}, workingDirectory);
    }

    if (!success)
        VcsOutputWindow::appendError(msgCannotLaunch("git-bash"));

    return success;
}

FilePath GitClient::vcsBinary() const
{
    bool ok;
    Utils::FilePath binary = static_cast<GitSettings &>(settings()).gitExecutable(&ok);
    if (!ok)
        return Utils::FilePath();
    return binary;
}

QTextCodec *GitClient::encoding(const QString &workingDirectory, const QString &configVar) const
{
    QString codecName = readConfigValue(workingDirectory, configVar).trimmed();
    // Set default commit encoding to 'UTF-8', when it's not set,
    // to solve displaying error of commit log with non-latin characters.
    if (codecName.isEmpty())
        return QTextCodec::codecForName("UTF-8");
    return QTextCodec::codecForName(codecName.toUtf8());
}

// returns first line from log and removes it
static QByteArray shiftLogLine(QByteArray &logText)
{
    const int index = logText.indexOf('\n');
    const QByteArray res = logText.left(index);
    logText.remove(0, index + 1);
    return res;
}

bool GitClient::readDataFromCommit(const QString &repoDirectory, const QString &commit,
                                   CommitData &commitData, QString *errorMessage,
                                   QString *commitTemplate)
{
    // Get commit data as "SHA1<lf>author<lf>email<lf>message".
    const QStringList arguments = {"log", "--max-count=1", "--pretty=format:%h\n%an\n%ae\n%B", commit};
    const SynchronousProcessResponse resp = vcsFullySynchronousExec(repoDirectory, arguments, silentFlags);

    if (resp.result != SynchronousProcessResponse::Finished) {
        if (errorMessage) {
            *errorMessage = tr("Cannot retrieve last commit data of repository \"%1\".")
                .arg(QDir::toNativeSeparators(repoDirectory));
        }
        return false;
    }

    QTextCodec *authorCodec = HostOsInfo::isWindowsHost()
            ? QTextCodec::codecForName("UTF-8")
            : commitData.commitEncoding;
    QByteArray stdOut = resp.rawStdOut;
    commitData.amendSHA1 = QLatin1String(shiftLogLine(stdOut));
    commitData.panelData.author = authorCodec->toUnicode(shiftLogLine(stdOut));
    commitData.panelData.email = authorCodec->toUnicode(shiftLogLine(stdOut));
    if (commitTemplate)
        *commitTemplate = commitData.commitEncoding->toUnicode(stdOut);
    return true;
}

bool GitClient::getCommitData(const QString &workingDirectory,
                              QString *commitTemplate,
                              CommitData &commitData,
                              QString *errorMessage)
{
    commitData.clear();

    // Find repo
    const QString repoDirectory = VcsManager::findTopLevelForDirectory(workingDirectory);
    if (repoDirectory.isEmpty()) {
        *errorMessage = msgRepositoryNotFound(workingDirectory);
        return false;
    }

    commitData.panelInfo.repository = repoDirectory;

    QString gitDir = findGitDirForRepository(repoDirectory);
    if (gitDir.isEmpty()) {
        *errorMessage = tr("The repository \"%1\" is not initialized.").arg(repoDirectory);
        return false;
    }

    // Run status. Note that it has exitcode 1 if there are no added files.
    QString output;
    if (commitData.commitType == FixupCommit) {
        synchronousLog(repoDirectory, {HEAD, "--not", "--remotes", "-n1"}, &output, errorMessage,
                       VcsCommand::SuppressCommandLogging);
        if (output.isEmpty()) {
            *errorMessage = msgNoCommits(false);
            return false;
        }
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

    commitData.commitEncoding = encoding(workingDirectory, "i18n.commitEncoding");

    // Get the commit template or the last commit message
    switch (commitData.commitType) {
    case AmendCommit: {
        if (!readDataFromCommit(repoDirectory, HEAD, commitData, errorMessage, commitTemplate))
            return false;
        break;
    }
    case SimpleCommit: {
        bool authorFromCherryPick = false;
        QDir gitDirectory(gitDir);
        // For cherry-picked commit, read author data from the commit (but template from MERGE_MSG)
        if (gitDirectory.exists(CHERRY_PICK_HEAD)) {
            authorFromCherryPick = readDataFromCommit(repoDirectory, CHERRY_PICK_HEAD, commitData);
            commitData.amendSHA1.clear();
        }
        if (!authorFromCherryPick) {
            // the format is:
            // Joe Developer <joedev@example.com> unixtimestamp +HHMM
            QString author_info = readGitVar(workingDirectory, "GIT_AUTHOR_IDENT");
            int lt = author_info.lastIndexOf('<');
            int gt = author_info.lastIndexOf('>');
            if (gt == -1 || uint(lt) > uint(gt)) {
                // shouldn't happen!
                commitData.panelData.author.clear();
                commitData.panelData.email.clear();
            } else {
                commitData.panelData.author = author_info.left(lt - 1);
                commitData.panelData.email = author_info.mid(lt + 1, gt - lt - 1);
            }
        }
        // Commit: Get the commit template
        QString templateFilename = gitDirectory.absoluteFilePath("MERGE_MSG");
        if (!QFile::exists(templateFilename))
            templateFilename = gitDirectory.absoluteFilePath("SQUASH_MSG");
        if (!QFile::exists(templateFilename)) {
            FilePath templateName = FilePath::fromUserInput(
                        readConfigValue(workingDirectory, "commit.template"));
            templateFilename = templateName.toString();
        }
        if (!templateFilename.isEmpty()) {
            // Make relative to repository
            const QFileInfo templateFileInfo(templateFilename);
            if (templateFileInfo.isRelative())
                templateFilename = repoDirectory + '/' + templateFilename;
            FileReader reader;
            if (!reader.fetch(templateFilename, QIODevice::Text, errorMessage))
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
        return GitClient::tr("Committed %n files.", nullptr, fileCount) + '\n';
    if (fileCount)
        return GitClient::tr("Amended \"%1\" (%n files).", nullptr, fileCount).arg(amendSHA1) + '\n';
    return GitClient::tr("Amended \"%1\".").arg(amendSHA1);
}

bool GitClient::addAndCommit(const QString &repositoryDirectory,
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
        QString file = model->file(i);
        const bool checked = model->checked(i);

        if (checked)
            ++commitCount;

        if (state == UntrackedFile && checked)
            filesToAdd.append(file);

        if ((state & StagedFile) && !checked) {
            if (state & (ModifiedFile | AddedFile | DeletedFile | TypeChangedFile)) {
                filesToReset.append(file);
            } else if (state & (RenamedFile | CopiedFile)) {
                const QString newFile = file.mid(file.indexOf(renameSeparator) + renameSeparator.count());
                filesToReset.append(newFile);
            }
        } else if (state & UnmergedFile && checked) {
            QTC_ASSERT(false, continue); // There should not be unmerged files when committing!
        }

        if ((state == ModifiedFile || state == TypeChangedFile) && checked) {
            filesToReset.removeAll(file);
            filesToAdd.append(file);
        } else if (state == AddedFile && checked) {
            QTC_ASSERT(false, continue); // these should be untracked!
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

    const SynchronousProcessResponse resp = vcsSynchronousExec(repositoryDirectory, arguments,
                                                               VcsCommand::NoFullySync);
    const QString stdErr = resp.stdErr();
    if (resp.result == SynchronousProcessResponse::Finished) {
        VcsOutputWindow::appendMessage(msgCommitted(amendSHA1, commitCount));
        VcsOutputWindow::appendError(stdErr);
        GitPlugin::updateCurrentBranch();
        return true;
    } else {
        VcsOutputWindow::appendError(tr("Cannot commit %n files: %1\n", nullptr, commitCount).arg(stdErr));
        return false;
    }
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
    const QString workingDirectory = isDirectory ? firstFile.absoluteFilePath() : firstFile.absolutePath();

    const QString repoDirectory = VcsManager::findTopLevelForDirectory(workingDirectory);
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
        const QDir repoDir(repoDirectory);
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
                                tr("Revert"),
                                tr("The file has been changed. Do you want to revert it?"),
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

void GitClient::revert(const QStringList &files, bool revertStaging)
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
        const QString msg = (isDirectory || files.size() > 1) ? msgNoChangedFiles() : tr("The file is not modified.");
        VcsOutputWindow::appendWarning(msg);
    }
        break;
    case RevertFailed:
        VcsOutputWindow::appendError(errorMessage);
        break;
    }
}

void GitClient::fetch(const QString &workingDirectory, const QString &remote)
{
    QStringList const arguments = {"fetch", (remote.isEmpty() ? "--all" : remote)};
    VcsCommand *command = vcsExec(workingDirectory, arguments, nullptr, true,
                                  VcsCommand::ShowSuccessMessage);
    connect(command, &VcsCommand::success,
            this, [workingDirectory] { GitPlugin::updateBranches(workingDirectory); });
}

bool GitClient::executeAndHandleConflicts(const QString &workingDirectory,
                                          const QStringList &arguments,
                                          const QString &abortCommand) const
{
    // Disable UNIX terminals to suppress SSH prompting.
    const unsigned flags = VcsCommand::SshPasswordPrompt
            | VcsCommand::ShowStdOut
            | VcsCommand::ExpectRepoChanges
            | VcsCommand::ShowSuccessMessage;
    const SynchronousProcessResponse resp = vcsSynchronousExec(workingDirectory, arguments, flags);
    // Notify about changed files or abort the rebase.
    ConflictHandler::handleResponse(resp, workingDirectory, abortCommand);
    return resp.result == SynchronousProcessResponse::Finished;
}

void GitClient::pull(const QString &workingDirectory, bool rebase)
{
    QString abortCommand;
    QStringList arguments = {"pull"};
    if (rebase) {
        arguments << "--rebase";
        abortCommand = "rebase";
    } else {
        abortCommand = "merge";
    }

    VcsCommand *command = vcsExecAbortable(workingDirectory, arguments, rebase, abortCommand);
    connect(command, &VcsCommand::success, this,
            [this, workingDirectory] { updateSubmodulesIfNeeded(workingDirectory, true); },
            Qt::QueuedConnection);
}

void GitClient::synchronousAbortCommand(const QString &workingDir, const QString &abortCommand)
{
    // Abort to clean if something goes wrong
    if (abortCommand.isEmpty()) {
        // no abort command - checkout index to clean working copy.
        synchronousCheckoutFiles(VcsManager::findTopLevelForDirectory(workingDir),
                                 QStringList(), QString(), nullptr, false);
        return;
    }

    const SynchronousProcessResponse resp = vcsFullySynchronousExec(
                workingDir, {abortCommand, "--abort"},
                VcsCommand::ExpectRepoChanges | VcsCommand::ShowSuccessMessage);
    VcsOutputWindow::append(resp.stdOut());
}

QString GitClient::synchronousTrackingBranch(const QString &workingDirectory, const QString &branch)
{
    QString remote;
    QString localBranch = branch.isEmpty() ? synchronousCurrentLocalBranch(workingDirectory) : branch;
    if (localBranch.isEmpty())
        return QString();
    localBranch.prepend("branch.");
    remote = readConfigValue(workingDirectory, localBranch + ".remote");
    if (remote.isEmpty())
        return QString();
    const QString rBranch = readConfigValue(workingDirectory, localBranch + ".merge")
            .replace("refs/heads/", QString());
    if (rBranch.isEmpty())
        return QString();
    return remote + '/' + rBranch;
}

bool GitClient::synchronousSetTrackingBranch(const QString &workingDirectory,
                                             const QString &branch, const QString &tracking)
{
    return vcsFullySynchronousExec(
                workingDirectory, {"branch", "--set-upstream-to=" + tracking, branch}).result
            == SynchronousProcessResponse::Finished;
}

VcsBase::VcsCommand *GitClient::asyncUpstreamStatus(const QString &workingDirectory,
                                                    const QString &branch,
                                                    const QString &upstream)
{
    const QStringList args {"rev-list", noColorOption, "--left-right", "--count",
                branch + "..." + upstream};
    return vcsExec(workingDirectory, args, nullptr, false, silentFlags);
}

void GitClient::handleMergeConflicts(const QString &workingDir, const QString &commit,
                                     const QStringList &files, const QString &abortCommand)
{
    QString message;
    if (!commit.isEmpty()) {
        message = tr("Conflicts detected with commit %1.").arg(commit);
    } else if (!files.isEmpty()) {
        QString fileList;
        QStringList partialFiles = files;
        while (partialFiles.count() > 20)
            partialFiles.removeLast();
        fileList = partialFiles.join('\n');
        if (partialFiles.count() != files.count())
            fileList += "\n...";
        message = tr("Conflicts detected with files:\n%1").arg(fileList);
    } else {
        message = tr("Conflicts detected.");
    }
    QMessageBox mergeOrAbort(QMessageBox::Question, tr("Conflicts Detected"), message,
                             QMessageBox::NoButton, ICore::dialogParent());
    QPushButton *mergeToolButton = mergeOrAbort.addButton(tr("Run &Merge Tool"),
                                                          QMessageBox::AcceptRole);
    const QString mergeTool = readConfigValue(workingDir, "merge.tool");
    if (mergeTool.isEmpty() || mergeTool.startsWith("vimdiff")) {
        mergeToolButton->setEnabled(false);
        mergeToolButton->setToolTip(tr("Only graphical merge tools are supported. "
                                       "Please configure merge.tool."));
    }
    mergeOrAbort.addButton(QMessageBox::Ignore);
    if (abortCommand == "rebase")
        mergeOrAbort.addButton(tr("&Skip"), QMessageBox::RejectRole);
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

void GitClient::addFuture(const QFuture<void> &future)
{
    m_synchronizer.addFuture(future);
}

// Subversion: git svn
void GitClient::synchronousSubversionFetch(const QString &workingDirectory) const
{
    // Disable UNIX terminals to suppress SSH prompting.
    const unsigned flags = VcsCommand::SshPasswordPrompt
            | VcsCommand::ShowStdOut
            | VcsCommand::ShowSuccessMessage;
    vcsSynchronousExec(workingDirectory, {"svn", "fetch"}, flags);
}

void GitClient::subversionLog(const QString &workingDirectory) const
{
    QStringList arguments = {"svn", "log"};
    int logCount = settings().intValue(GitSettings::logCountKey);
    if (logCount > 0)
         arguments << ("--limit=" + QString::number(logCount));

    // Create a command editor, no highlighting or interaction.
    const QString title = tr("Git SVN Log");
    const Id editorId = Git::Constants::GIT_SVN_LOG_EDITOR_ID;
    const QString sourceFile = VcsBaseEditor::getSource(workingDirectory, QStringList());
    VcsBaseEditorWidget *editor = createVcsEditor(editorId, title, sourceFile, codecFor(CodecNone),
                                                  "svnLog", sourceFile);
    editor->setWorkingDirectory(workingDirectory);
    vcsExec(workingDirectory, arguments, editor);
}

void GitClient::subversionDeltaCommit(const QString &workingDirectory) const
{
    vcsExec(workingDirectory, {"svn", "dcommit"}, nullptr, true,
            VcsCommand::ShowSuccessMessage);
}

void GitClient::push(const QString &workingDirectory, const QStringList &pushArgs)
{
    VcsCommand *command = vcsExec(
                workingDirectory, QStringList({"push"}) + pushArgs, nullptr, true,
                VcsCommand::ShowSuccessMessage);
    connect(command, &VcsCommand::stdErrText, this, [this, command](const QString &text) {
        if (text.contains("non-fast-forward"))
            command->setCookie(NonFastForward);
        else if (text.contains("has no upstream branch"))
            command->setCookie(NoRemoteBranch);

        if (command->cookie().toInt() == NoRemoteBranch) {
            const QStringList lines = text.split('\n', Qt::SkipEmptyParts);
            for (const QString &line : lines) {
                /* Extract the suggested command from the git output which
                 * should be similar to the following:
                 *
                 *     git push --set-upstream origin add_set_upstream_dialog
                 */
                const QString trimmedLine = line.trimmed();
                if (trimmedLine.startsWith("git push")) {
                    m_pushFallbackCommand = trimmedLine;
                    break;
                }
            }
        }
    });
    connect(command, &VcsCommand::finished,
            this, [this, command, workingDirectory, pushArgs](bool success) {
        if (!success) {
            switch (static_cast<PushFailure>(command->cookie().toInt())) {
            case Unknown:
                break;
            case NonFastForward: {
                const QColor warnColor = Utils::creatorTheme()->color(Theme::TextColorError);
                if (QMessageBox::question(
                            Core::ICore::dialogParent(), tr("Force Push"),
                            tr("Push failed. Would you like to force-push <span style=\"color:#%1\">"
                            "(rewrites remote history)</span>?")
                            .arg(QString::number(warnColor.rgba(), 16)),
                            QMessageBox::Yes | QMessageBox::No,
                            QMessageBox::No) == QMessageBox::Yes) {
                    VcsCommand *rePushCommand = vcsExec(workingDirectory,
                            QStringList({"push", "--force-with-lease"}) + pushArgs,
                            nullptr, true, VcsCommand::ShowSuccessMessage);
                    connect(rePushCommand, &VcsCommand::success,
                            this, []() { GitPlugin::updateCurrentBranch(); });
                }
                break;
            }
            case NoRemoteBranch:
                if (QMessageBox::question(
                            Core::ICore::dialogParent(), tr("No Upstream Branch"),
                            tr("Push failed because the local branch \"%1\" "
                               "does not have an upstream branch on the remote.\n\n"
                               "Would you like to create the branch \"%1\" on the "
                               "remote and set it as upstream?")
                            .arg(synchronousCurrentLocalBranch(workingDirectory)),
                            QMessageBox::Yes | QMessageBox::No,
                            QMessageBox::No) == QMessageBox::Yes) {

                    const QStringList fallbackCommandParts =
                            m_pushFallbackCommand.split(' ', Qt::SkipEmptyParts);
                    VcsCommand *rePushCommand = vcsExec(workingDirectory,
                                                        fallbackCommandParts.mid(1),
                            nullptr, true, VcsCommand::ShowSuccessMessage);
                    connect(rePushCommand, &VcsCommand::success, this, [workingDirectory]() {
                        GitPlugin::updateBranches(workingDirectory);
                    });
                }
                break;
            }
        } else {
            GitPlugin::updateCurrentBranch();
        }
    });
}

bool GitClient::synchronousMerge(const QString &workingDirectory, const QString &branch,
                                 bool allowFastForward)
{
    QString command = "merge";
    QStringList arguments = {command};
    if (!allowFastForward)
        arguments << "--no-ff";
    arguments << branch;
    return executeAndHandleConflicts(workingDirectory, arguments, command);
}

bool GitClient::canRebase(const QString &workingDirectory) const
{
    const QString gitDir = findGitDirForRepository(workingDirectory);
    if (QFileInfo::exists(gitDir + "/rebase-apply")
            || QFileInfo::exists(gitDir + "/rebase-merge")) {
        VcsOutputWindow::appendError(
                    tr("Rebase, merge or am is in progress. Finish "
                       "or abort it and then try again."));
        return false;
    }
    return true;
}

void GitClient::rebase(const QString &workingDirectory, const QString &argument)
{
    vcsExecAbortable(workingDirectory, {"rebase", argument}, true);
}

void GitClient::cherryPick(const QString &workingDirectory, const QString &argument)
{
    vcsExecAbortable(workingDirectory, {"cherry-pick", argument});
}

void GitClient::revert(const QString &workingDirectory, const QString &argument)
{
    vcsExecAbortable(workingDirectory, {"revert", argument});
}

// Executes a command asynchronously. Work tree is expected to be clean.
// Stashing is handled prior to this call.
VcsCommand *GitClient::vcsExecAbortable(const QString &workingDirectory,
                                        const QStringList &arguments,
                                        bool isRebase,
                                        QString abortCommand)
{
    QTC_ASSERT(!arguments.isEmpty(), return nullptr);

    if (abortCommand.isEmpty())
        abortCommand = arguments.at(0);
    VcsCommand *command = createCommand(workingDirectory, nullptr, VcsWindowOutputBind);
    command->setCookie(workingDirectory);
    command->addFlags(VcsCommand::SshPasswordPrompt
                      | VcsCommand::ShowStdOut
                      | VcsCommand::ShowSuccessMessage);
    // For rebase, Git might request an editor (which means the process keeps running until the
    // user closes it), so run without timeout.
    command->addJob({vcsBinary(), arguments}, isRebase ? 0 : command->defaultTimeoutS());
    ConflictHandler::attachToCommand(command, abortCommand);
    if (isRebase)
        GitProgressParser::attachToCommand(command);
    command->execute();

    return command;
}

bool GitClient::synchronousRevert(const QString &workingDirectory, const QString &commit)
{
    const QString command = "revert";
    // Do not stash if --continue or --abort is given as the commit
    if (!commit.startsWith('-') && !beginStashScope(workingDirectory, command))
        return false;
    return executeAndHandleConflicts(workingDirectory, {command, "--no-edit", commit}, command);
}

bool GitClient::synchronousCherryPick(const QString &workingDirectory, const QString &commit)
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

void GitClient::interactiveRebase(const QString &workingDirectory, const QString &commit, bool fixup)
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
    return tr("There are no modified files.");
}

QString GitClient::msgNoCommits(bool includeRemote)
{
    return includeRemote ? tr("No commits were found") : tr("No local commits were found");
}

void GitClient::stashPop(const QString &workingDirectory, const QString &stash)
{
    QStringList arguments = {"stash", "pop"};
    if (!stash.isEmpty())
        arguments << stash;
    VcsCommand *cmd = vcsExec(workingDirectory, arguments, nullptr, true, VcsCommand::ExpectRepoChanges);
    ConflictHandler::attachToCommand(cmd);
}

bool GitClient::synchronousStashRestore(const QString &workingDirectory,
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

bool GitClient::synchronousStashRemove(const QString &workingDirectory, const QString &stash,
                                       QString *errorMessage) const
{
    QStringList arguments = {"stash"};
    if (stash.isEmpty())
        arguments << "clear";
    else
        arguments << "drop" << stash;

    const SynchronousProcessResponse resp = vcsFullySynchronousExec(workingDirectory, arguments);
    if (resp.result == SynchronousProcessResponse::Finished) {
        const QString output = resp.stdOut();
        if (!output.isEmpty())
            VcsOutputWindow::append(output);
        return true;
    } else {
        msgCannotRun(arguments, workingDirectory, resp.stdErr(), errorMessage);
        return false;
    }
}

bool GitClient::synchronousStashList(const QString &workingDirectory, QList<Stash> *stashes,
                                     QString *errorMessage) const
{
    stashes->clear();

    const QStringList arguments = {"stash", "list", noColorOption};
    const SynchronousProcessResponse resp = vcsFullySynchronousExec(workingDirectory, arguments,
                                                                    VcsCommand::ForceCLocale);
    if (resp.result != SynchronousProcessResponse::Finished) {
        msgCannotRun(arguments, workingDirectory, resp.stdErr(), errorMessage);
        return false;
    }
    Stash stash;
    const QStringList lines = splitLines(resp.stdOut());
    for (const QString &line : lines) {
        if (stash.parseStashLine(line))
            stashes->push_back(stash);
    }
    return true;
}

// Read a single-line config value, return trimmed
QString GitClient::readConfigValue(const QString &workingDirectory, const QString &configVar) const
{
    return readOneLine(workingDirectory, {"config", configVar});
}

void GitClient::setConfigValue(const QString &workingDirectory, const QString &configVar,
                               const QString &value) const
{
    readOneLine(workingDirectory, {"config", configVar, value});
}

QString GitClient::readGitVar(const QString &workingDirectory, const QString &configVar) const
{
    return readOneLine(workingDirectory, {"var", configVar});
}

QString GitClient::readOneLine(const QString &workingDirectory, const QStringList &arguments) const
{
    // Git for Windows always uses UTF-8 for configuration:
    // https://github.com/msysgit/msysgit/wiki/Git-for-Windows-Unicode-Support#convert-config-files
    static QTextCodec *codec = HostOsInfo::isWindowsHost()
            ? QTextCodec::codecForName("UTF-8")
            : QTextCodec::codecForLocale();

    const SynchronousProcessResponse resp = vcsFullySynchronousExec(
                workingDirectory, arguments, silentFlags, vcsTimeoutS(), codec);
    if (resp.result != SynchronousProcessResponse::Finished)
        return QString();
    return resp.stdOut().trimmed();
}

// determine version as '(major << 16) + (minor << 8) + patch' or 0.
unsigned GitClient::gitVersion(QString *errorMessage) const
{
    const FilePath newGitBinary = vcsBinary();
    if (m_gitVersionForBinary != newGitBinary && !newGitBinary.isEmpty()) {
        // Do not execute repeatedly if that fails (due to git
        // not being installed) until settings are changed.
        m_cachedGitVersion = synchronousGitVersion(errorMessage);
        m_gitVersionForBinary = newGitBinary;
    }
    return m_cachedGitVersion;
}

// determine version as '(major << 16) + (minor << 8) + patch' or 0.
unsigned GitClient::synchronousGitVersion(QString *errorMessage) const
{
    if (vcsBinary().isEmpty())
        return 0;

    // run git --version
    const SynchronousProcessResponse resp = vcsSynchronousExec(
                QString(), {"--version"}, silentFlags);
    if (resp.result != SynchronousProcessResponse::Finished) {
        msgCannotRun(tr("Cannot determine Git version: %1").arg(resp.stdErr()), errorMessage);
        return 0;
    }

    // cut 'git version 1.6.5.1.sha'
    // another form: 'git version 1.9.rc1'
    const QString output = resp.stdOut();
    const QRegularExpression versionPattern("^[^\\d]+(\\d+)\\.(\\d+)\\.(\\d+|rc\\d).*$");
    QTC_ASSERT(versionPattern.isValid(), return 0);
    const QRegularExpressionMatch match = versionPattern.match(output);
    QTC_ASSERT(match.hasMatch(), return 0);
    const unsigned majorV = match.captured(1).toUInt(nullptr, 16);
    const unsigned minorV = match.captured(2).toUInt(nullptr, 16);
    const unsigned patchV = match.captured(3).toUInt(nullptr, 16);
    return version(majorV, minorV, patchV);
}

bool GitClient::StashInfo::init(const QString &workingDirectory, const QString &command,
                                StashFlag flag, PushAction pushAction)
{
    m_workingDir = workingDirectory;
    m_flags = flag;
    m_pushAction = pushAction;
    QString errorMessage;
    QString statusOutput;
    switch (m_instance->gitStatus(m_workingDir, StatusMode(NoUntracked | NoSubmodules),
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
    QMessageBox msgBox(QMessageBox::Question, tr("Uncommitted Changes Found"),
                       tr("What would you like to do with local changes in:") + "\n\n"
                       + QDir::toNativeSeparators(m_workingDir) + '\"',
                       QMessageBox::NoButton, ICore::dialogParent());

    msgBox.setDetailedText(statusOutput);

    QPushButton *stashAndPopButton = msgBox.addButton(tr("Stash && Pop"), QMessageBox::AcceptRole);
    stashAndPopButton->setToolTip(tr("Stash local changes and pop when %1 finishes.").arg(command));

    QPushButton *stashButton = msgBox.addButton(tr("Stash"), QMessageBox::AcceptRole);
    stashButton->setToolTip(tr("Stash local changes and execute %1.").arg(command));

    QPushButton *discardButton = msgBox.addButton(tr("Discard"), QMessageBox::AcceptRole);
    discardButton->setToolTip(tr("Discard (reset) local changes and execute %1.").arg(command));

    QPushButton *ignoreButton = nullptr;
    if (m_flags & AllowUnstashed) {
        ignoreButton = msgBox.addButton(QMessageBox::Ignore);
        ignoreButton->setToolTip(tr("Execute %1 with local changes in working directory.")
                                 .arg(command));
    }

    QPushButton *cancelButton = msgBox.addButton(QMessageBox::Cancel);
    cancelButton->setToolTip(tr("Cancel %1.").arg(command));

    msgBox.exec();

    if (msgBox.clickedButton() == discardButton) {
        m_stashResult = m_instance->synchronousReset(m_workingDir, QStringList(), errorMessage) ?
                    StashUnchanged : StashFailed;
    } else if (msgBox.clickedButton() == ignoreButton) { // At your own risk, so.
        m_stashResult = NotStashed;
    } else if (msgBox.clickedButton() == cancelButton) {
        m_stashResult = StashCanceled;
    } else if (msgBox.clickedButton() == stashButton) {
        const bool result = m_instance->executeSynchronousStash(
                    m_workingDir, creatorStashMessage(command), false, errorMessage);
        m_stashResult = result ? StashUnchanged : StashFailed;
    } else if (msgBox.clickedButton() == stashAndPopButton) {
        executeStash(command, errorMessage);
    }
}

void GitClient::StashInfo::executeStash(const QString &command, QString *errorMessage)
{
    m_message = creatorStashMessage(command);
    if (!m_instance->executeSynchronousStash(m_workingDir, m_message, false, errorMessage))
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
        if (m_instance->stashNameFromMessage(m_workingDir, m_message, &stashName))
            m_instance->stashPop(m_workingDir, stashName);
    }

    if (m_pushAction == NormalPush)
        m_instance->push(m_workingDir);
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

QString GitClient::suggestedLocalBranchName(const QStringList localNames,
                                            const QString trackedBranch)
{
    const QString suggestedNameBase = trackedBranch.mid(trackedBranch.lastIndexOf('/') + 1);
    QString suggestedName = suggestedNameBase;
    int i = 2;
    while (localNames.contains(suggestedName)) {
        suggestedName = suggestedNameBase + QString::number(i);
        ++i;
    }

    return suggestedName;
}

void GitClient::addChangeActions(QMenu *menu, const QString &workingDir, const QString &change)
{
    QTC_ASSERT(!change.isEmpty(), return);
    menu->addAction(tr("Cherr&y-Pick Change %1").arg(change), [workingDir, change] {
        m_instance->synchronousCherryPick(workingDir, change);
    });
    menu->addAction(tr("Re&vert Change %1").arg(change), [workingDir, change] {
        m_instance->synchronousRevert(workingDir, change);
    });
    menu->addAction(tr("C&heckout Change %1").arg(change), [workingDir, change] {
        m_instance->checkout(workingDir, change);
    });
    connect(menu->addAction(tr("&Interactive Rebase from Change %1...").arg(change)),
            &QAction::triggered, [workingDir, change] {
        GitPlugin::startRebaseFromCommit(workingDir, change);
    });
    QAction *logAction = menu->addAction(tr("&Log for Change %1").arg(change), [workingDir, change] {
        m_instance->log(workingDir, QString(), false, {change});
    });
    if (change.contains(".."))
        menu->setDefaultAction(logAction);
    menu->addAction(tr("Add &Tag for Change %1...").arg(change), [workingDir, change] {
        QString output;
        QString errorMessage;
        m_instance->synchronousTagCmd(workingDir, QStringList(),
                                               &output, &errorMessage);

        const QStringList tags = output.split('\n');
        BranchAddDialog dialog(tags, BranchAddDialog::Type::AddTag, Core::ICore::dialogParent());

        if (dialog.exec() == QDialog::Rejected)
            return;

        m_instance->synchronousTagCmd(workingDir,
                                               {dialog.branchName(), change},
                                               &output, &errorMessage);
        VcsOutputWindow::append(output);
        if (!errorMessage.isEmpty())
            VcsOutputWindow::append(errorMessage, VcsOutputWindow::MessageStyle::Error);
    });

    auto resetChange = [workingDir, change](const QByteArray &resetType) {
        m_instance->reset(
                    workingDir, QLatin1String("--" + resetType), change);
    };
    auto resetMenu = new QMenu(tr("&Reset to Change %1").arg(change), menu);
    resetMenu->addAction(tr("&Hard"), std::bind(resetChange, "hard"));
    resetMenu->addAction(tr("&Mixed"), std::bind(resetChange, "mixed"));
    resetMenu->addAction(tr("&Soft"), std::bind(resetChange, "soft"));
    menu->addMenu(resetMenu);

    menu->addAction(tr("Di&ff Against %1").arg(change),
                    [workingDir, change] {
        m_instance->diffRepository(workingDir, change, {});
    });
    if (!m_instance->m_diffCommit.isEmpty()) {
        menu->addAction(tr("Diff &Against Saved %1").arg(m_instance->m_diffCommit),
                        [workingDir, change] {
            m_instance->diffRepository(workingDir, m_instance->m_diffCommit, change);
            m_instance->m_diffCommit.clear();
        });
    }
    menu->addAction(tr("&Save for Diff"), [change] {
        m_instance->m_diffCommit = change;
    });
}

} // namespace Internal
} // namespace Git

#include "gitclient.moc"
