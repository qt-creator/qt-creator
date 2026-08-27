// Copyright (C) 2023 Andre Hartmann (aha_1980@gmx.de)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "instantblame.h"

#include "gitclient.h"
#include "gitconstants.h"
#include "gitplugin.h"
#include "gitsettings.h"
#include "gittr.h"

#include <coreplugin/icore.h>
#include <coreplugin/vcsmanager.h>

#ifdef WITH_TESTS
#include "extensionsystem/iplugin.h"
#endif

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/textmark.h>

#include <utils/filepath.h>
#include <utils/icon.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcscommand.h>

#include <QAction>
#include <QDateTime>
#include <QLabel>
#include <QLayout>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QTimer>

#ifdef WITH_TESTS
#include <QTest>
#endif

namespace Git::Internal {

static Q_LOGGING_CATEGORY(log, "qtc.vcs.git.instantblame", QtWarningMsg);

using namespace Core;
using namespace QtTaskTree;
using namespace TextEditor;
using namespace Utils;
using namespace VcsBase;

class BlameController : public QObject
{
public:
    explicit BlameController(QObject *parent = nullptr);

    void setContext(TextEditor::TextEditorWidget *widget,
                    const Utils::FilePath &topLevel,
                    const QString &ref,
                    const QString &commandFilePath,
                    const Utils::FilePath &workingFilePath,
                    bool allowModifiedDocument);
    void setEnabled(bool enabled);
    void schedule(int delay = 0);
    void clear();

private:
    void loadRepositoryConfiguration();
    void perform();

    QPointer<TextEditor::TextEditorWidget> m_widget;
    QPointer<TextEditor::TextDocument> m_document;
    Utils::FilePath m_topLevel;
    QString m_ref;
    QString m_commandFilePath;
    Utils::FilePath m_workingFilePath;
    bool m_allowModifiedDocument = false;
    bool m_enabled = false;
    Utils::TextEncoding m_encoding;
    Author m_author;
    int m_lastLine = -1;
    QTimer *m_timer = nullptr;
    QtTaskTree::QSingleTaskTreeRunner m_taskTreeRunner;
    std::unique_ptr<BlameMark> m_blameMark;
    quint64 m_contextGeneration = 0;
    quint64 m_requestGeneration = 0;
};

BlameMark::BlameMark(TextEditor::TextDocument *document, int lineNumber, const CommitInfo &info)
    : TextEditor::TextMark(document,
                           lineNumber,
                           {Tr::tr("Git Blame"), Constants::TEXT_MARK_CATEGORY_BLAME})
    , m_info(info)
{
    QString text = info.shortAuthor + " " + info.authorDate.toString("yyyy-MM-dd");
    if (settings().instantBlameShowSubject())
        text += " • " + info.subject;

    setPriority(TextEditor::TextMark::LowPriority);
    setToolTipProvider([this] { return toolTipText(m_info); });
    setLineAnnotation(text);
    setSettingsPage(VcsBase::Constants::VCS_ID_GIT);
    setActionsProvider([info] {
        QAction *copyToClipboardAction = new QAction;
        copyToClipboardAction->setIcon(Icon::fromTheme("edit-copy"));
        copyToClipboardAction->setToolTip(Tr::tr("Copy Hash to Clipboard"));
        QObject::connect(copyToClipboardAction, &QAction::triggered, [info] {
            Utils::setClipboardAndSelection(info.hash);
        });
        return QList<QAction *>{copyToClipboardAction};
    });
}

bool BlameMark::addToolTipContent(QLayout *target) const
{
    auto textLabel = new QLabel;
    textLabel->setText(toolTip());
    target->addWidget(textLabel);
    QObject::connect(
        textLabel, &QLabel::linkActivated, textLabel, [info = m_info](const QString &link) {
            qCInfo(log) << "Link activated with target:" << link;
            const QString hash = (link == "blameParent") ? info.hash + "^" : info.hash;
            const FilePath topLevel = info.topLevel;

            if (link.startsWith("blame") || link == "revert" || link == "showFile") {
                const FilePath path = topLevel;
                const QString originalFileName = info.originalFileName;
                if (link.startsWith("blame")) {
                    qCInfo(log).nospace().noquote()
                        << "Blaming: \"" << path << "/" << originalFileName
                        << "\":" << info.originalLine << " @ " << hash;
                    gitClient().annotate(path, originalFileName, info.originalLine, hash);
                } else if (link == "revert") {
                    const QMessageBox::StandardButton result = QMessageBox::question(
                        Core::ICore::dialogParent(),
                        Tr::tr("Revert Commit?"),
                        Tr::tr("Revert the commit %1?").arg(info.hash.left(8)),
                        QMessageBox::Yes | QMessageBox::No);
                    if (result == QMessageBox::Yes) {
                        qCInfo(log).nospace().noquote()
                            << "Reverting: \"" << path << "\" @ " << hash;
                        gitClient().synchronousRevert(path, hash);
                    }
                } else {
                    qCInfo(log).nospace().noquote()
                        << "Showing file: \"" << path << "/" << originalFileName << "\" @ " << hash;

                    const auto fileName = FilePath::fromString(originalFileName);
                    gitClient().openShowEditor(path, hash, fileName, GitClient::ShowEditor::Always,
                                               info.originalLine);
                }
            } else if (link == "diffParent") {
                qCInfo(log).nospace().noquote()
                    << "Inline diff for: \"" << info.filePath << "\" against " << hash << "^";

                // the parent side file name differs when the blamed commit
                // renamed the file
                const QString parentFileName = info.previousFileName.isEmpty()
                                                   ? info.originalFileName
                                                   : info.previousFileName;
                if (info.modified) {
                    // Uncommitted lines are compared against the index, where
                    // staging them has a visible effect. The editor keeps the
                    // cursor's line visible.
                    gitClient().inlineDiffFile(topLevel, info.filePath.path());
                } else {
                    // Show the change that last touched the line: the blamed
                    // commit against its parent. This is the same regardless of
                    // the view the tooltip is shown in (working tree, index, or
                    // a revision), and following it repeatedly walks the line's
                    // history one changing commit at a time.
                    gitClient().inlineDiffRevisions(topLevel, info.filePath,
                                                    hash, info.originalFileName,
                                                    hash + "^", parentFileName,
                                                    info.line);
                }
            } else if (link == "logLine") {
                qCInfo(log).nospace().noquote()
                    << "Showing log for: \"" << info.filePath << "\" line:" << info.line;

                const QString relativeFile = info.filePath.relativeChildPath(topLevel).path();
                const QString lineArg
                    = QString("-L %1,%1:%2").arg(info.line).arg(relativeFile);
                gitClient().log(topLevel, {}, true, {lineArg, "--no-patch"});
            } else {
                qCInfo(log).nospace().noquote()
                    << "Showing commit: " << hash << " for " << info.filePath;
                gitClient().show(info.filePath, hash);
            }
        });

    return true;
}

QString BlameMark::toolTipText(const CommitInfo &info) const
{
    const ColorNames colors = GitClient::colorNames();

    QString actions;
    if (!info.modified) {
        const QString blameRevision = Tr::tr("Blame %1").arg(info.hash.left(8));
        const QString blameParent = Tr::tr("Blame Parent");
        const QString showFile = Tr::tr("File at %1").arg(info.hash.left(8));
        const QString revert = Tr::tr("Revert %1").arg(info.hash.left(8));
        const QString logForLine = Tr::tr("Log for line %1").arg(info.line);
        const QString diffParent = Tr::tr("Diff Against Parent");
        actions = QString(
                      "<table cellspacing=\"10\"><tr>"
                      "  <td><a href=\"blame\">%1</a></td>"
                      "  <td><a href=\"blameParent\">%2</a></td>"
                      "  <td><a href=\"showFile\">%3</a></td>"
                      "  <td><a href=\"revert\">%4</a></td>"
                      "  <td><a href=\"logLine\">%5</a></td>"
                      "  <td><a href=\"diffParent\">%6</a></td>"
                      "</tr></table>"
                      "<p></p>")
                      .arg(blameRevision, blameParent, showFile, revert, logForLine,
                           diffParent);
    } else {
        // uncommitted lines (changed or staged) can still be diffed against
        // the last committed state
        actions = QString(
                      "<table cellspacing=\"10\"><tr>"
                      "  <td><a href=\"diffParent\">%1</a></td>"
                      "</tr></table>"
                      "<p></p>")
                      .arg(Tr::tr("Diff"));
    }

    const QString header = QString(
                         "<table>"
                         "  <tr><td>commit</td><td><a style=\"color: %1;\" href=\"show\">%2</a></td></tr>"
                         "  <tr><td>Author:</td><td style=\"color: %3;\">%4 &lt;%5&gt;</td></tr>"
                         "  <tr><td>Date:</td><td style=\"color: %6;\">%7</td></tr>"
                         "</table>"
                         "<p style=\"color: %8;\">%9</p>")
                         .arg(colors.hash, info.hash,
                              colors.author, info.author, info.authorMail,
                              colors.date, info.authorDate.toString("yyyy-MM-dd hh:mm:ss"),
                              colors.subject, info.subject.toHtmlEscaped());

    QString result = actions + header;

    QString diff;
    if (!info.oldLines.isEmpty()) {
        const QString removed = GitClient::styleColorName(TextEditor::C_REMOVED_LINE);

        QStringList oldLines = info.oldLines;
        if (oldLines.size() > 5) {
            oldLines = info.oldLines.first(2);
            oldLines.append("- ...");
            oldLines.append(info.oldLines.last(2));
        }

        for (const QString &oldLine : std::as_const(oldLines)) {
            diff.append("<p style=\"margin: 0px; color: " + removed + " ;\">" + oldLine.toHtmlEscaped() + "</p>");
        }
    }
    if (!info.newLine.isEmpty()) {
        const QString added = GitClient::styleColorName(TextEditor::C_ADDED_LINE);
        diff.append("<p style=\"margin-top: 0px; color: " + added + ";\">" + info.newLine.toHtmlEscaped() + "</p>");
    }

    if (!diff.isEmpty())
        result.append("<pre>" + diff + "</pre>");

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

void BlameMark::addOldLine(const QString &oldLine)
{
    m_info.oldLines.append(oldLine);
}

void BlameMark::addNewLine(const QString &newLine)
{
    m_info.newLine = newLine;
}

InstantBlame::InstantBlame()
    : m_controller(new BlameController(this))
{
}

void InstantBlame::setup()
{
    qCDebug(log) << "Setup";
    const auto setupForCurrentEditor = [this] { this->setupForCurrentEditor(); };
    connect(&settings().instantBlame, &BaseAspect::changed, this, setupForCurrentEditor);
    connect(&settings().instantBlameIgnoreSpaceChanges, &BaseAspect::changed,
            this, setupForCurrentEditor);
    connect(&settings().instantBlameIgnoreLineMoves, &BaseAspect::changed,
            this, setupForCurrentEditor);
    connect(&settings().instantBlameShowSubject, &BaseAspect::changed,
            this, setupForCurrentEditor);

    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, setupForCurrentEditor);
    connect(EditorManager::instance(), &EditorManager::documentClosed,
            this, [this](IDocument *doc) {
                if (m_document == doc)
                    stop();
    });
    setupForCurrentEditor();
}

void InstantBlame::setupForCurrentEditor()
{
    stop();
    if (!settings().instantBlame()) {
        qCDebug(log) << "Instant blame is disabled.";
        return;
    }

    TextEditorWidget *widget = TextEditorWidget::currentTextEditorWidget();
    if (!widget) {
        qCInfo(log) << "Cannot get current text editor widget.";
        return;
    }
    if (qobject_cast<const VcsBaseEditorWidget *>(widget)) {
        qCDebug(log) << "Deactivating in version control editors";
        return;
    }

    m_document = widget->textDocument();
    FilePath topLevel = currentState().currentFileTopLevel();
    if (topLevel.isEmpty()) {
        const QString repository = m_document->property("GitRepository").toString();
        if (!repository.isEmpty())
            topLevel = FilePath::fromString(repository);
    }
    const FilePath sourceFilePath = VcsBase::source(m_document);
    const FilePath workingFilePath = sourceFilePath.isEmpty() ? m_document->filePath()
                                                              : sourceFilePath;
    const QString ref = m_document->property("GitReference").toString();
    m_controller->setContext(widget, topLevel, ref, workingFilePath.path(), workingFilePath,
                             /*allowModifiedDocument=*/false);
    m_controller->setEnabled(true);

    m_blameCursorPosConn = connect(widget, &PlainTextEdit::cursorPositionChanged,
                                   this, [this] { m_controller->schedule(500); });
    m_documentChangedConn = connect(m_document, &IDocument::changed,
                                    this, &InstantBlame::slotDocumentChanged);
    m_modified = m_document->isModified();
}

void InstantBlame::repeat()
{
    if (!settings().instantBlame())
        return;

    QTimer::singleShot(20, this, [this] {
        setupForCurrentEditor();
    });
}

// Porcelain format of git blame output:
// Consists of 12 or 13 lines (line 11 can be missing, "boundary", or "previous")
// The first line contains hash, original line, current line,
// and optional the  number of lines in this group when blaming multiple lines.
// The last line starts with a tab and is followed by the actual file content.
// ----------------------------------------------------------------------------
// 8b649d2d61416205977aba56ef93e1e1f155005e 4 5 1
// author John Doe
// author-mail <john.doe@gmail.com>
// author-time 1613752276
// author-tz +0100
// committer John Doe
// committer-mail <john.doe@gmail.com>
// committer-time 1613752312
// committer-tz +0100
// summary Add greeting to script
// (missing/boundary/previous f6b5868032a5dc0e73b82b09184086d784949646 oldfile)
// filename foo
// <TAB>echo Hello World!
// ----------------------------------------------------------------------------

static CommitInfo parseBlameOutput(const QStringList &blame, const FilePath &filePath,
                                   int line, const Git::Internal::Author &author)
{
    CommitInfo result;
    if (blame.size() <= 12)
        return result;

    const QStringList firstLineParts = blame.at(0).split(" ");
    result.hash = firstLineParts.first();
    result.modified = !gitClient().isValidRevision(result.hash);
    if (result.modified) {
        result.author = Tr::tr("Not Committed Yet");
        result.subject = Tr::tr("Modified line in %1").arg(filePath.fileName());
    } else {
        result.author = blame.at(1).mid(7);
        result.authorMail = blame.at(2).mid(13).chopped(1);
        result.subject = blame.at(9).mid(8);
    }
    if (result.author == author.name || result.authorMail == author.email)
        result.shortAuthor = Tr::tr("You");
    else
        result.shortAuthor = result.author;
    const uint timeStamp = blame.at(3).mid(12).toUInt();
    result.authorDate = QDateTime::fromSecsSinceEpoch(timeStamp);
    result.filePath = filePath;
    // blame.at(10) can be "boundary", "previous" or "filename"
    if (blame.at(10).startsWith("filename")) {
        result.originalFileName = blame.at(10).mid(9);
    } else {
        // "previous <hash> <filename>" carries the file name on the parent
        // side, which differs when the blamed commit renamed the file
        if (blame.at(10).startsWith("previous ")) {
            const QString previous = blame.at(10).mid(9);
            const int space = previous.indexOf(' ');
            if (space > 0)
                result.previousFileName = previous.mid(space + 1);
        }
        result.originalFileName = blame.at(11).mid(9);
    }
    result.line = line;
    if (firstLineParts.size() > 1)
        result.originalLine = firstLineParts.at(1).toInt();
    else
        result.originalLine = line;
    return result;
}

static QStringList blameCommandArguments(const QString &filePath,
                                         const QString &ref,
                                         int line,
                                         bool ignoreSpaceChanges,
                                         bool detectMovedLines)
{
    QStringList arguments = {"blame", "-p"};
    if (ignoreSpaceChanges)
        arguments.append("-w");
    if (detectMovedLines)
        arguments.append("-M");
    arguments.append({"-L", QString("%1,%1").arg(line)});
    if (!ref.isEmpty())
        arguments.append(ref);
    arguments.append({"--", filePath});
    return arguments;
}

void InstantBlame::once()
{
    if (settings().instantBlame()) {
        scheduleInstantBlame();
        return;
    }

    stop();
    TextEditorWidget *widget = TextEditorWidget::currentTextEditorWidget();
    if (!widget || qobject_cast<const VcsBaseEditorWidget *>(widget)) {
        qCWarning(log) << "Cannot get a suitable text editor widget";
        return;
    }

    m_document = widget->textDocument();
    FilePath topLevel = currentState().currentFileTopLevel();
    if (topLevel.isEmpty()) {
        const QString repository = m_document->property("GitRepository").toString();
        if (!repository.isEmpty())
            topLevel = FilePath::fromString(repository);
    }
    const FilePath sourceFilePath = VcsBase::source(m_document);
    const FilePath workingFilePath = sourceFilePath.isEmpty() ? m_document->filePath()
                                                              : sourceFilePath;
    m_controller->setContext(widget, topLevel,
                             m_document->property("GitReference").toString(),
                             workingFilePath.path(), workingFilePath,
                             /*allowModifiedDocument=*/false);
    m_controller->setEnabled(true);
    m_blameCursorPosConn = connect(widget, &PlainTextEdit::cursorPositionChanged,
                                   this, &InstantBlame::stop, Qt::SingleShotConnection);
}

void InstantBlame::scheduleInstantBlame()
{
    m_controller->schedule();
}

// the empty block after a trailing newline is not a real line
static bool isPhantomLine(const QTextDocument *document, int line)
{
    const int blockCount = document->blockCount();
    if (line > blockCount)
        return true;
    return line == blockCount && blockCount > 1 && document->lastBlock().text().isEmpty();
}

void InstantBlame::stop()
{
    qCInfo(log) << "Stopping blame now";
    m_controller->setEnabled(false);
    disconnect(m_blameCursorPosConn);
    disconnect(m_documentChangedConn);
    m_document = nullptr;
    m_modified = false;
}

void InstantBlame::slotDocumentChanged()
{
    if (!m_document) {
        qCWarning(log) << "Document is invalid, disconnecting.";
        disconnect(m_documentChangedConn);
        return;
    }

    const bool modified = m_document->isModified();
    qCDebug(log) << "Document is changed, modified:" << modified;
    if (modified) {
        m_controller->clear();
    } else if (m_modified) {
        scheduleInstantBlame();
    }
    m_modified = modified;
}

BlameController::BlameController(QObject *parent)
    : QObject(parent)
    , m_encoding(gitClient().defaultCommitEncoding())
    , m_timer(new QTimer(this))
{
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &BlameController::perform);
}

void BlameController::setContext(TextEditorWidget *widget,
                                 const FilePath &topLevel,
                                 const QString &ref,
                                 const QString &commandFilePath,
                                 const FilePath &workingFilePath,
                                 bool allowModifiedDocument)
{
    clear();
    ++m_contextGeneration;
    m_widget = widget;
    m_document = widget ? widget->textDocument() : nullptr;
    m_topLevel = topLevel;
    m_ref = ref;
    m_commandFilePath = commandFilePath;
    m_workingFilePath = workingFilePath;
    m_allowModifiedDocument = allowModifiedDocument;
    m_encoding = gitClient().defaultCommitEncoding();
    m_author = {};
    loadRepositoryConfiguration();
    if (m_enabled)
        schedule();
}

void BlameController::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    if (enabled)
        schedule();
    else
        clear();
}

void BlameController::schedule(int delay)
{
    if (!m_enabled)
        return;
    m_lastLine = -1;
    m_timer->stop();
    m_taskTreeRunner.reset();
    ++m_requestGeneration;
    m_timer->start(delay);
}

void BlameController::clear()
{
    m_timer->stop();
    m_taskTreeRunner.reset();
    m_blameMark.reset();
    m_lastLine = -1;
    ++m_requestGeneration;
}

void BlameController::loadRepositoryConfiguration()
{
    if (m_topLevel.isEmpty())
        return;

    const quint64 generation = m_contextGeneration;
    const FilePath topLevel = m_topLevel;
    const QPointer<BlameController> guard(this);
    gitClient().readConfigAsync(
        topLevel,
        {"config", "i18n.commitEncoding"},
        [guard, generation](const CommandResult &result) {
            if (!guard || guard->m_contextGeneration != generation)
                return;
            TextEncoding encoding = gitClient().defaultCommitEncoding();
            if (result.result() == ProcessResult::FinishedWithSuccess) {
                const QString codecName = result.cleanedStdOut().trimmed();
                if (!codecName.isEmpty())
                    encoding = codecName.toUtf8();
            }
            if (guard->m_encoding != encoding) {
                guard->m_encoding = encoding;
                guard->schedule();
            }
        });
    gitClient().readConfigAsync(
        topLevel,
        {"var", "GIT_AUTHOR_IDENT"},
        [guard, generation](const CommandResult &result) {
            if (!guard || guard->m_contextGeneration != generation
                || result.result() != ProcessResult::FinishedWithSuccess) {
                return;
            }
            const Author author = gitClient().parseAuthor(result.cleanedStdOut().trimmed());
            if (guard->m_author != author) {
                guard->m_author = author;
                guard->schedule();
            }
        });
}

void BlameController::perform()
{
    if (!m_widget || !m_document || m_topLevel.isEmpty()) {
        clear();
        return;
    }
    if (!m_allowModifiedDocument && m_document->isModified()) {
        qCDebug(log) << "Document is modified, pausing blame";
        m_blameMark.reset();
        m_lastLine = -1;
        return;
    }

    const int line = m_widget->textCursor().blockNumber() + 1;
    if (isPhantomLine(m_widget->document(), line)) {
        m_lastLine = -1;
        m_blameMark.reset();
        return;
    }
    if (m_lastLine == line)
        return;
    m_lastLine = line;

    const QStringList options = blameCommandArguments(
        m_commandFilePath,
        m_ref,
        line,
        settings().instantBlameIgnoreSpaceChanges(),
        settings().instantBlameIgnoreLineMoves());
    qCDebug(log) << "Running git" << options.join(' ');

    const quint64 generation = ++m_requestGeneration;
    const FilePath topLevel = m_topLevel;
    const FilePath workingFilePath = m_workingFilePath;
    const TextEncoding encoding = m_encoding;
    const Author author = m_author;
    const QPointer<TextDocument> document = m_document;
    const QPointer<BlameController> guard(this);
    const Storage<CommitInfo> infoStorage;
    const auto blameHandler = [guard, document, generation, workingFilePath, topLevel, line,
                               author, infoStorage](const CommandResult &result) {
        if (!guard || !document || guard->m_requestGeneration != generation)
            return;
        const QString output = result.cleanedStdOut();
        if (result.result() != ProcessResult::FinishedWithSuccess || output.isEmpty()) {
            guard->clear();
            return;
        }
        *infoStorage = parseBlameOutput(output.split('\n'), workingFilePath, line, author);
        infoStorage->topLevel = topLevel;
        guard->m_blameMark = std::make_unique<BlameMark>(document.get(), line, *infoStorage);
    };
    const auto onLogSetup = [infoStorage, topLevel, encoding](Process &process) -> SetupResult {
        if (infoStorage->hash.isEmpty() || infoStorage->modified)
            return SetupResult::StopWithSuccess;
        // Get line diff: `git log -n 1 -p -L47,47:README.md a5c4c34c9ab4`
        const QString origLineString = QString("%1,%1").arg(infoStorage->originalLine);
        const QString fileLineRange = "-L" + origLineString + ":" + infoStorage->originalFileName;
        const QStringList logOptions = {"log", "-n 1", "-p", fileLineRange, infoStorage->hash};
        qCDebug(log) << "Running git" << logOptions.join(' ');
        gitClient().setupCommand(process, topLevel, logOptions);
        process.setEncoding(encoding);
        return SetupResult::Continue;
    };
    const auto onLogDone = [guard, generation](const Process &process) {
        if (!guard || guard->m_requestGeneration != generation || !guard->m_blameMark)
            return;
        const QString error = process.cleanedStdErr().trimmed();
        if (!error.isEmpty())
            qCWarning(log) << error;
        static const QRegularExpression re("^[-+][^-+].*");
        const QStringList diffLines = process.cleanedStdOut().split("\n").filter(re);
        for (const QString &diffLine : diffLines) {
            if (diffLine.startsWith("-"))
                guard->m_blameMark->addOldLine(diffLine);
            else if (diffLine.startsWith("+"))
                guard->m_blameMark->addNewLine(diffLine);
        }
    };

    m_taskTreeRunner.start({
        infoStorage,
        gitClient().commandTask({topLevel, options, RunFlag::NoOutput, {}, encoding,
                                 blameHandler}),
        ProcessTask(onLogSetup, onLogDone, CallDoneFlag::OnSuccess),
    });
}

BaselineBlame::BaselineBlame(TextEditorWidget *widget,
                             const FilePath &topLevel,
                             const QString &ref,
                             const QString &relativeFile,
                             const FilePath &workingFilePath)
    : QObject(widget)
    , m_controller(new BlameController(this))
{
    m_controller->setContext(widget, topLevel, ref, relativeFile, workingFilePath,
                             /*allowModifiedDocument=*/true);
    connect(widget, &PlainTextEdit::cursorPositionChanged,
            this, [this] {
                if (settings().instantBlame())
                    m_controller->schedule(500);
            });
    connect(&settings().instantBlame, &BaseAspect::changed, this,
            [this] { m_controller->setEnabled(settings().instantBlame()); });
    const auto schedule = [this] { m_controller->schedule(); };
    connect(&settings().instantBlameIgnoreSpaceChanges, &BaseAspect::changed, this, schedule);
    connect(&settings().instantBlameIgnoreLineMoves, &BaseAspect::changed, this, schedule);
    connect(&settings().instantBlameShowSubject, &BaseAspect::changed, this, schedule);
    m_controller->setEnabled(settings().instantBlame());
}

#ifdef WITH_TESTS

class InstantBlameTest final : public QObject
{
    Q_OBJECT

private slots:
    void testBlameCommandArguments();
    void testBlameOutputParsing();
};

void InstantBlameTest::testBlameCommandArguments()
{
    QCOMPARE(blameCommandArguments("src/main.cpp", {}, 17, false, false),
             QStringList({"blame", "-p", "-L", "17,17", "--", "src/main.cpp"}));
    QCOMPARE(blameCommandArguments("file.h", "HEAD^", 2, true, false),
             QStringList({"blame", "-p", "-w", "-L", "2,2", "HEAD^", "--", "file.h"}));
    QCOMPARE(blameCommandArguments("file.h", "HEAD^", 3, false, true),
             QStringList({"blame", "-p", "-M", "-L", "3,3", "HEAD^", "--", "file.h"}));
    QCOMPARE(blameCommandArguments("file.h", "HEAD^", 4, true, true),
             QStringList({"blame", "-p", "-w", "-M", "-L", "4,4", "HEAD^", "--", "file.h"}));
}

void InstantBlameTest::testBlameOutputParsing()
{
    const QStringList output = {
        "8b649d2d61416205977aba56ef93e1e1f155005e 4 5 1",
        "author John Doe",
        "author-mail <john.doe@example.com>",
        "author-time 1613752276",
        "author-tz +0100",
        "committer John Doe",
        "committer-mail <john.doe@example.com>",
        "committer-time 1613752312",
        "committer-tz +0100",
        "summary Add greeting",
        "previous f6b5868032a5dc0e73b82b09184086d784949646 bar.cpp",
        "filename foo.cpp",
        "\tcout << \"Hello World!\"",
    };

    const FilePath filePath = FilePath::fromString("/repo/foo.cpp");
    const CommitInfo info = parseBlameOutput(output, filePath, 5,
                                             {"John Doe", "john.doe@example.com"});
    QCOMPARE(info.hash, "8b649d2d61416205977aba56ef93e1e1f155005e");
    QCOMPARE(info.shortAuthor, Tr::tr("You"));
    QCOMPARE(info.subject, "Add greeting");
    QCOMPARE(info.filePath, filePath);
    QCOMPARE(info.originalFileName, "foo.cpp");
    QCOMPARE(info.previousFileName, "bar.cpp");
    QCOMPARE(info.line, 5);
    QCOMPARE(info.originalLine, 4);
    QVERIFY(!info.modified);
}

void registerInstantBlameTests(ExtensionSystem::IPlugin *plugin)
{
    plugin->addTest<InstantBlameTest>();
}

#endif

} // Git::Internal

#include "instantblame.moc"
