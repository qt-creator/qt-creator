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

namespace Git::Internal {

static Q_LOGGING_CATEGORY(log, "qtc.vcs.git.instantblame", QtWarningMsg);

using namespace Core;
using namespace QtTaskTree;
using namespace TextEditor;
using namespace Utils;
using namespace VcsBase;

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
{
    m_encoding = gitClient().defaultCommitEncoding();
    m_cursorPositionChangedTimer = new QTimer(this);
    m_cursorPositionChangedTimer->setSingleShot(true);
    connect(m_cursorPositionChangedTimer, &QTimer::timeout, this, &InstantBlame::perform);
    m_scheduleTimer = new QTimer(this);
    m_scheduleTimer->setSingleShot(true);
    connect(m_scheduleTimer, &QTimer::timeout, this, &InstantBlame::perform);
}

void InstantBlame::setup()
{
    qCDebug(log) << "Setup";

    auto setupBlameForEditor = [this] {
        qCDebug(log) << "Setting up blame for editor.";

        if (!settings().instantBlame()) {
            qCDebug(log) << "Instant blame is disabled.";
            m_lastVisitedEditorLine = -1;
            stop();
            m_taskTreeRunner.reset();
            return;
        }

        TextEditorWidget *widget = TextEditorWidget::currentTextEditorWidget();
        if (!widget) {
            qCInfo(log) << "Cannot get current text editor widget.";
            stop();
            m_taskTreeRunner.reset();
            return;
        }

        if (qobject_cast<const VcsBaseEditorWidget *>(widget)) {
            qCDebug(log) << "Deactivating in version control editors";
            return; // Skip in version control editors like log or blame
        }

        refreshWorkingDirectory();

        qCInfo(log) << "Adding blame cursor connection";
        m_blameCursorPosConn = connect(widget, &PlainTextEdit::cursorPositionChanged, this,
                                       [this] {
                                           if (!settings().instantBlame()) {
                                               disconnect(m_blameCursorPosConn);
                                               return;
                                           }
                                           if (!m_scheduleTimer->isActive())
                                               m_cursorPositionChangedTimer->start(500);
                                       });
        m_document = widget->textDocument();
        m_documentChangedConn = connect(m_document, &IDocument::changed,
                                        this, &InstantBlame::slotDocumentChanged,
                                        Qt::UniqueConnection);

        scheduleInstantBlame();
    };

    connect(&settings().instantBlame, &BaseAspect::changed, this, setupBlameForEditor);
    connect(&settings().instantBlameIgnoreSpaceChanges, &BaseAspect::changed, this, setupBlameForEditor);
    connect(&settings().instantBlameIgnoreLineMoves, &BaseAspect::changed, this, setupBlameForEditor);
    connect(&settings().instantBlameShowSubject, &BaseAspect::changed, this, setupBlameForEditor);

    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, setupBlameForEditor);
    connect(EditorManager::instance(), &EditorManager::documentClosed,
            this, [this](IDocument *doc) {
        if (m_document != doc)
            return;
        disconnect(m_documentChangedConn);
        m_document = nullptr;
    });
}

void InstantBlame::repeat()
{
    if (!settings().instantBlame())
        return;

    QTimer::singleShot(20, this, [this] {
        refreshWorkingDirectory();
        scheduleInstantBlame();
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

void InstantBlame::once()
{
    if (!settings().instantBlame()) {
        const TextEditorWidget *widget = TextEditorWidget::currentTextEditorWidget();
        if (!widget) {
            qCWarning(log) << "Cannot get current text editor widget";
            return;
        }
        connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, [this] { m_blameMark.reset(); }, Qt::SingleShotConnection);

        connect(widget, &PlainTextEdit::cursorPositionChanged,
            this, [this] { m_blameMark.reset(); }, Qt::SingleShotConnection);

        refreshWorkingDirectory();
    }

    scheduleInstantBlame();
}

void InstantBlame::scheduleInstantBlame()
{
    m_lastVisitedEditorLine = -1;
    m_cursorPositionChangedTimer->stop();
    if (m_scheduleTimer->isActive())
        m_taskTreeRunner.reset();
    else
        m_scheduleTimer->start(0);
}

// the empty block after a trailing newline is not a real line
static bool isPhantomLine(const QTextDocument *document, int line)
{
    const int blockCount = document->blockCount();
    if (line > blockCount)
        return true;
    return line == blockCount && blockCount > 1 && document->lastBlock().text().isEmpty();
}

void InstantBlame::perform()
{
    const TextEditorWidget *widget = TextEditorWidget::currentTextEditorWidget();
    if (!widget) {
        qCWarning(log) << "Cannot get current text editor widget";
        return;
    }

    if (widget->textDocument()->isModified()) {
        qCDebug(log) << "Document is modified, pausing blame";
        m_blameMark.reset();
        m_lastVisitedEditorLine = -1;
        return;
    }

    const QTextCursor cursor = widget->textCursor();
    const QTextBlock block = cursor.block();
    const int line = block.blockNumber() + 1;

    if (isPhantomLine(widget->document(), line)) {
        m_lastVisitedEditorLine = -1;
        m_blameMark.reset();
        return;
    }

    if (m_lastVisitedEditorLine == line)
        return;

    qCDebug(log) << "New editor line:" << line;
    m_lastVisitedEditorLine = line;

    const FilePath sourceFilePath = VcsBase::source(widget->textDocument());
    const FilePath filePath = !sourceFilePath.isEmpty()
                                  ? sourceFilePath
                                  : widget->textDocument()->filePath();
    const QString lineString = QString("%1,%1").arg(line);

    QStringList options = {"blame", "-p"};
    if (settings().instantBlameIgnoreSpaceChanges())
        options.append("-w");
    if (settings().instantBlameIgnoreLineMoves())
        options.append("-M");
    if (const QVariant ref = widget->textDocument()->property("GitReference"); ref.isValid())
        options.append(ref.toString());
    options.append({"-L", lineString, "--", filePath.path()});
    qCDebug(log) << "Running git" << options.join(' ');

    const Storage<CommitInfo> infoStorage;
    const auto blameHandler = [this, filePath, line, infoStorage,
                               doc = QPointer(widget->textDocument())](const CommandResult &result) {
        if (doc.isNull())
            return;
        if (result.result() == ProcessResult::FinishedWithError
                && result.cleanedStdErr().contains("no such path")) {
            stop();
            return;
        }
        const QString output = result.cleanedStdOut();
        if (output.isEmpty()) {
            stop();
            return;
        }
        *infoStorage = parseBlameOutput(output.split('\n'), filePath, line, m_author);
        infoStorage->topLevel = m_workingDirectory;
        m_blameMark.reset(new BlameMark(doc.get(), line, *infoStorage));
    };
    const TextEncoding encoding = m_encoding;
    const auto onLogSetup = [this, infoStorage, encoding](Process &process) -> SetupResult {
        if (infoStorage->hash.isEmpty() || infoStorage->modified)
            return SetupResult::StopWithSuccess;
        // Get line diff: `git log -n 1 -p -L47,47:README.md a5c4c34c9ab4`
        const QString origLineString = QString("%1,%1").arg(infoStorage->originalLine);
        const QString fileLineRange = "-L" + origLineString + ":" + infoStorage->originalFileName;
        const QStringList logOptions = {"log", "-n 1", "-p", fileLineRange, infoStorage->hash};
        qCDebug(log) << "Running git" << logOptions.join(' ');
        gitClient().setupCommand(process, m_workingDirectory, logOptions);
        process.setEncoding(encoding);
        return SetupResult::Continue;
    };
    const auto onLogDone = [this](const Process &process) {
        if (!m_blameMark)
            return;
        const QString error = process.cleanedStdErr().trimmed();
        if (!error.isEmpty())
            qCWarning(log) << error;
        static const QRegularExpression re("^[-+][^-+].*");
        const QStringList diffLines = process.cleanedStdOut().split("\n").filter(re);
        for (const QString &diffLine : diffLines) {
            if (diffLine.startsWith("-")) {
                m_blameMark->addOldLine(diffLine);
                qCDebug(log) << "Found removed line: " << diffLine;
            } else if (diffLine.startsWith("+")) {
                m_blameMark->addNewLine(diffLine);
                qCDebug(log) << "Found added line: " << diffLine;
            }
        }
    };

    m_taskTreeRunner.start({
        infoStorage,
        gitClient().commandTask({m_workingDirectory, options, RunFlag::NoOutput, {}, m_encoding,
                                 blameHandler}),
        ProcessTask(onLogSetup, onLogDone, CallDoneFlag::OnSuccess)
    });
}

void InstantBlame::stop()
{
    qCInfo(log) << "Stopping blame now";
    m_scheduleTimer->stop();
    m_blameMark.reset();
    m_cursorPositionChangedTimer->stop();
    disconnect(m_blameCursorPosConn);
    disconnect(m_documentChangedConn);
}

void InstantBlame::refreshWorkingDirectory()
{
    FilePath workingDirectory = currentState().currentFileTopLevel();

    if (workingDirectory.isEmpty()) {
        const TextEditorWidget *widget = TextEditorWidget::currentTextEditorWidget();
        if (!widget)
            return;
        const QString repo = widget->textDocument()->property("GitRepository").toString();
        if (!repo.isEmpty())
            workingDirectory = FilePath::fromString(repo);
    }

    if (m_workingDirectory == workingDirectory)
        return;

    qCInfo(log) << "Setting new working directory:" << workingDirectory;
    m_workingDirectory = workingDirectory;

    const auto commitCodecHandler = [this, workingDirectory](const CommandResult &result) {
        TextEncoding encoding;

        if (result.result() == ProcessResult::FinishedWithSuccess) {
            const QString codecName = result.cleanedStdOut().trimmed();
            encoding = codecName.toUtf8();
        } else {
            encoding = gitClient().defaultCommitEncoding();
        }

        if (m_encoding != encoding) {
            qCInfo(log) << "Setting new text codec:" << encoding.name();
            m_encoding = encoding;
            scheduleInstantBlame();
        }
    };
    gitClient().readConfigAsync(workingDirectory, {"config", "i18n.commitEncoding"},
                                commitCodecHandler);

    const auto authorHandler = [this, workingDirectory](const CommandResult &result) {
        if (result.result() == ProcessResult::FinishedWithSuccess) {
            const QString authorInfo = result.cleanedStdOut().trimmed();
            const Author author = gitClient().parseAuthor(authorInfo);

            if (m_author != author) {
                qCInfo(log) << "Setting new author name:" << author.name << author.email;
                m_author = author;
                scheduleInstantBlame();
            }
        }
    };
    gitClient().readConfigAsync(workingDirectory, {"var", "GIT_AUTHOR_IDENT"},
                                authorHandler);
}

void InstantBlame::slotDocumentChanged()
{
    if (m_document == nullptr) {
        qCWarning(log) << "Document is invalid, disconnecting.";
        disconnect(m_documentChangedConn);
        return;
    }

    const bool modified = m_document->isModified();
    qCDebug(log) << "Document is changed, modified:" << modified;
    if (m_modified && !modified)
        scheduleInstantBlame();
    m_modified = modified;
}

BaselineBlame::BaselineBlame(TextEditorWidget *widget,
                             const FilePath &topLevel,
                             const QString &ref,
                             const QString &relativeFile,
                             const FilePath &workingFilePath)
    : QObject(widget)
    , m_widget(widget)
    , m_topLevel(topLevel)
    , m_ref(ref)
    , m_relativeFile(relativeFile)
    , m_workingFilePath(workingFilePath)
    , m_encoding(gitClient().defaultCommitEncoding())
{
    // author names and subjects are encoded with the repository's commit
    // encoding, not necessarily the default
    gitClient().readConfigAsync(topLevel, {"config", "i18n.commitEncoding"},
                                [guard = QPointer<BaselineBlame>(this)](
                                    const CommandResult &result) {
        if (!guard || result.result() != ProcessResult::FinishedWithSuccess)
            return;
        const QString codecName = result.cleanedStdOut().trimmed();
        if (!codecName.isEmpty())
            guard->m_encoding = codecName.toUtf8();
    });
    m_cursorTimer = new QTimer(this);
    m_cursorTimer->setSingleShot(true);
    m_cursorTimer->setInterval(500);
    connect(m_cursorTimer, &QTimer::timeout, this, &BaselineBlame::perform);
    connect(widget, &PlainTextEdit::cursorPositionChanged,
            this, [this] { m_cursorTimer->start(); });
    m_cursorTimer->start();
}

void BaselineBlame::perform()
{
    if (!settings().instantBlame()) {
        m_blameMark.reset();
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

    const QString lineString = QString("%1,%1").arg(line);
    QStringList options = {"blame", "-p"};
    if (settings().instantBlameIgnoreSpaceChanges())
        options.append("-w");
    if (settings().instantBlameIgnoreLineMoves())
        options.append("-M");
    options.append({"-L", lineString});
    if (!m_ref.isEmpty()) // no revision blames the working tree
        options.append(m_ref);
    options.append({"--", m_relativeFile});
    qCDebug(log) << "Running git" << options.join(' ');

    const Storage<CommitInfo> infoStorage;
    const auto blameHandler = [this, line, infoStorage](const CommandResult &result) {
        const QString output = result.cleanedStdOut();
        if (result.result() != ProcessResult::FinishedWithSuccess || output.isEmpty()) {
            m_blameMark.reset();
            return;
        }
        *infoStorage = parseBlameOutput(output.split('\n'), m_workingFilePath, line, {});
        infoStorage->topLevel = m_topLevel;
        m_blameMark.reset(new BlameMark(m_widget->textDocument(), line, *infoStorage));
    };
    const TextEncoding encoding = m_encoding;
    const FilePath topLevel = m_topLevel;
    const auto onLogSetup = [infoStorage, topLevel, encoding](Process &process) -> SetupResult {
        if (infoStorage->hash.isEmpty())
            return SetupResult::StopWithSuccess;
        const QString origLineString = QString("%1,%1").arg(infoStorage->originalLine);
        const QString fileLineRange = "-L" + origLineString + ":" + infoStorage->originalFileName;
        const QStringList logOptions = {"log", "-n 1", "-p", fileLineRange, infoStorage->hash};
        gitClient().setupCommand(process, topLevel, logOptions);
        process.setEncoding(encoding);
        return SetupResult::Continue;
    };
    const auto onLogDone = [this](const Process &process) {
        if (!m_blameMark)
            return;
        static const QRegularExpression re("^[-+][^-+].*");
        const QStringList diffLines = process.cleanedStdOut().split("\n").filter(re);
        for (const QString &diffLine : diffLines) {
            if (diffLine.startsWith("-"))
                m_blameMark->addOldLine(diffLine);
            else if (diffLine.startsWith("+"))
                m_blameMark->addNewLine(diffLine);
        }
    };

    m_taskTreeRunner.start({
        infoStorage,
        gitClient().commandTask({m_topLevel, options, RunFlag::NoOutput, {}, m_encoding,
                                 blameHandler}),
        ProcessTask(onLogSetup, onLogDone, CallDoneFlag::OnSuccess)
    });
}

} // Git::Internal
