// Copyright (C) 2023 Andre Hartmann (aha_1980@gmx.de)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "instantblame.h"

#include "gitclient.h"
#include "gitconstants.h"
#include "gitplugin.h"
#include "gitsettings.h"
#include "gittr.h"
#include "gitutils.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/vcsmanager.h>

#include <diffeditor/diffutils.h>
#include <diffeditor/inlinediff.h>

#ifdef WITH_TESTS
#include "commitdata.h"

#include <extensionsystem/iplugin.h>
#include <vcsbase/submitfilemodel.h>
#endif

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/textmark.h>

#include <utils/async.h>
#include <utils/filepath.h>
#include <utils/icon.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/tooltip/tooltip.h>
#include <utils/theme/theme.h>

#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcscommand.h>

#include <QAction>
#include <QDateTime>
#include <QFuture>
#include <QLabel>
#include <QLayout>
#include <QLoggingCategory>
#include <QMenu>
#include <QMessageBox>
#include <QPromise>
#include <QScopeGuard>
#include <QTimer>

#include <memory>
#include <optional>
#include <utility>

#ifdef WITH_TESTS
#include <QApplication>
#include <QClipboard>
#include <QMetaObject>
#include <QTemporaryDir>
#include <QTest>
#include <QVBoxLayout>
#include <QWidget>
#endif

namespace Git::Internal {

static Q_LOGGING_CATEGORY(log, "qtc.vcs.git.instantblame", QtWarningMsg);

using namespace Core;
using namespace QtTaskTree;
using namespace TextEditor;
using namespace Utils;
using namespace VcsBase;

struct PreviousCommitLookup;

class CommitInfo {
public:
    QString hash;
    QString shortAuthor;
    QString author;
    QString authorMail;
    QDateTime authorDate;
    QString subject;
    // Shared so actions captured before the asynchronous lookup see its result.
    std::shared_ptr<PreviousCommitLookup> previousCommit;
    QStringList oldLines;     ///< the previous line contents
    QString newLine;          ///< the new line contents
    Utils::FilePath topLevel; ///< repository top level path
    Utils::FilePath filePath; ///< absolute file path for current file
    QString originalFileName; ///< relative file path from project root for the original file
    QString previousFileName; ///< relative file path in the parent commit, if it differs (renames)
    int line = -1;            ///< current line number in current file
    int originalLine = -1;    ///< original line number in the original file
    bool modified = false;    ///< line is locally modified (uncommitted)
};

struct PreviousCommitLookup
{
    enum class State { Pending, Found, NoPreviousLine, Unavailable };
    State state = State::Pending;
    CommitInfo info;
};

class BlameMark : public TextEditor::TextMark
{
public:
    BlameMark(TextEditor::TextDocument *document, int lineNumber, const CommitInfo &info);
    bool addToolTipContent(QLayout *target) const final;
    void addOldLine(const QString &oldLine);
    void addNewLine(const QString &newLine);
    void setPreviousCommit(const CommitInfo &info);
    void setNoPreviousLine()
    {
        m_info.previousCommit->state = PreviousCommitLookup::State::NoPreviousLine;
    }
    void finishPreviousLookup()
    {
        if (m_info.previousCommit
            && m_info.previousCommit->state == PreviousCommitLookup::State::Pending) {
            m_info.previousCommit->state = PreviousCommitLookup::State::Unavailable;
        }
    }

private:
    void initialize();

    CommitInfo m_info;
};

struct EditorLineDiff
{
    QStringList oldLines;
    QList<int> oldLineNumbers;
    QString newLine;
    bool isValid = false;
};

static EditorLineDiff editorLineDiff(const DiffEditor::ChunkData &chunk, int editorLine)
{
    EditorLineDiff result;
    QStringList pendingOldLines;
    QList<int> pendingOldLineNumbers;
    int currentBaseLine = chunk.startingLineNumber[DiffEditor::LeftSide];
    int currentEditorLine = chunk.startingLineNumber[DiffEditor::RightSide];
    for (const DiffEditor::RowData &row : chunk.rows) {
        if (row.equal) {
            pendingOldLines.clear();
            pendingOldLineNumbers.clear();
            if (row.line[DiffEditor::LeftSide].textLineType
                == DiffEditor::TextLineData::TextLine) {
                ++currentBaseLine;
            }
            if (row.line[DiffEditor::RightSide].textLineType
                == DiffEditor::TextLineData::TextLine) {
                ++currentEditorLine;
            }
            continue;
        }

        const DiffEditor::TextLineData &oldLine = row.line[DiffEditor::LeftSide];
        if (oldLine.textLineType == DiffEditor::TextLineData::TextLine) {
            ++currentBaseLine;
            pendingOldLines.append(oldLine.text);
            pendingOldLineNumbers.append(currentBaseLine);
        }

        const DiffEditor::TextLineData &newLine = row.line[DiffEditor::RightSide];
        if (newLine.textLineType != DiffEditor::TextLineData::TextLine)
            continue;
        if (++currentEditorLine == editorLine) {
            result.oldLines = pendingOldLines;
            result.oldLineNumbers = pendingOldLineNumbers;
            result.newLine = newLine.text;
            result.isValid = true;
            return result;
        }
        pendingOldLines.clear();
        pendingOldLineNumbers.clear();
    }
    return result;
}

#ifdef WITH_TESTS
static EditorLineDiff editorLineDiffAgainstEditorText(const QString &baseText,
                                                       const QString &editorText,
                                                       int editorLine)
{
    return editorLineDiff(diffChunkAgainstEditorText(baseText, editorText), editorLine);
}
#endif

static void computeEditorLineDiff(QPromise<EditorLineDiff> &promise,
                                  const QString &baseText,
                                  const QString &editorText,
                                  int editorLine)
{
    const DiffEditor::ChunkData chunk = diffChunkAgainstEditorText(
        baseText, editorText, QFuture<void>(promise.future()));
    if (!promise.isCanceled())
        promise.addResult(editorLineDiff(chunk, editorLine));
}

class BlameController : public QObject
{
public:
    explicit BlameController(QObject *parent = nullptr);

    void setContext(TextEditor::TextEditorWidget *widget,
                    const Utils::FilePath &topLevel,
                    const QString &ref,
                    const QString &commandFilePath,
                    const Utils::FilePath &workingFilePath);
    void setEnabled(bool enabled);
    void schedule(int delay = 0);
    void clear();

private:
    void clearHelper();
    void loadRepositoryConfiguration();
    void perform();
    void waitForRepositoryChange();

    QPointer<TextEditor::TextEditorWidget> m_widget;
    QPointer<TextEditor::TextDocument> m_document;
    Utils::FilePath m_topLevel;
    QString m_ref;
    QString m_commandFilePath;
    Utils::FilePath m_workingFilePath;
    bool m_enabled = false;
    Utils::TextEncoding m_encoding;
    Author m_author;
    int m_lastLine = -1;
    QTimer *m_timer = nullptr;
    QtTaskTree::QSingleTaskTreeRunner m_taskTreeRunner;
    std::unique_ptr<BlameMark> m_blameMark;
    QMetaObject::Connection m_repositoryChangedConn;
    quint64 m_contextGeneration = 0;
    quint64 m_requestGeneration = 0;
};

static QString blameToolTipText(const CommitInfo &info);

BlameMark::BlameMark(TextEditor::TextDocument *document, int lineNumber, const CommitInfo &info)
    : TextEditor::TextMark(document,
                           lineNumber,
                           {Tr::tr("Git Blame"), Constants::TEXT_MARK_CATEGORY_BLAME})
    , m_info(info)
{
    if (info.modified)
        m_info.previousCommit = std::make_shared<PreviousCommitLookup>();
    QString text = info.modified
        ? Tr::tr("Modified locally")
        : info.shortAuthor + " " + info.authorDate.toString("yyyy-MM-dd");
    if (!info.modified && settings().instantBlameShowSubject())
        text += " • " + info.subject;

    setPriority(TextEditor::TextMark::LowPriority);
    setToolTipProvider([this] { return blameToolTipText(m_info); });
    setLineAnnotation(text);
    setSettingsPage(VcsBase::Constants::VCS_ID_GIT);
    setActionsProvider([info = m_info] {
        QAction *copyToClipboardAction = new QAction;
        copyToClipboardAction->setIcon(Icon::fromTheme("edit-copy"));
        copyToClipboardAction->setToolTip(Tr::tr("Copy Hash to Clipboard"));
        QObject::connect(copyToClipboardAction, &QAction::triggered, [info] {
            const QString hash = info.modified ? info.previousCommit->info.hash : info.hash;
            if (!hash.isEmpty())
                Utils::setClipboardAndSelection(hash);
        });
        return QList<QAction *>{copyToClipboardAction};
    });
}

static void addBlameToolTipContent(QLayout *layout, const CommitInfo &info)
{
    auto textLabel = new QLabel;
    textLabel->setText(blameToolTipText(info));
    layout->addWidget(textLabel);
    QObject::connect(
        textLabel, &QLabel::linkActivated, textLabel, [info](QString link) {
            qCInfo(log) << "Link activated with target:" << link;
            Utils::ToolTip::hideImmediately();
            constexpr QLatin1StringView previousPrefix("previous");
            const bool usePrevious = link.startsWith(previousPrefix);
            if (usePrevious) {
                if (!info.previousCommit || info.previousCommit->state != PreviousCommitLookup::State::Found)
                    return;
                link = link.sliced(previousPrefix.size());
                if (link.isEmpty())
                    return;
                link[0] = link.at(0).toLower();
            }
            const CommitInfo &target = usePrevious ? info.previousCommit->info : info;
            if (link == "copyHash") {
                Utils::setClipboardAndSelection(target.hash);
                return;
            }
            const QString hash = (link == "blameParent") ? target.hash + "^" : target.hash;
            const FilePath topLevel = target.topLevel;

            if (link.startsWith("blame") || link == "revert" || link == "showFile") {
                const FilePath path = topLevel;
                const QString originalFileName = target.originalFileName;
                if (link.startsWith("blame")) {
                    qCInfo(log).nospace().noquote()
                        << "Blaming: \"" << path << "/" << originalFileName
                        << "\":" << target.originalLine << " @ " << hash;
                    gitClient().annotate(path, originalFileName, target.originalLine, hash);
                } else if (link == "revert") {
                    const QMessageBox::StandardButton result = QMessageBox::question(
                        Core::ICore::dialogParent(),
                        Tr::tr("Revert Commit?"),
                        Tr::tr("Revert the commit %1?").arg(target.hash.left(8)),
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
                                               target.originalLine);
                }
            } else if (link == "diffParent") {
                qCInfo(log).nospace().noquote()
                    << "Inline diff for: \"" << target.filePath << "\" against " << hash << "^";

                // the parent side file name differs when the blamed commit
                // renamed the file
                const QString parentFileName = target.previousFileName.isEmpty()
                                                   ? target.originalFileName
                                                   : target.previousFileName;
                if (target.modified) {
                    // Uncommitted lines are compared against the index, where
                    // staging them has a visible effect. The editor keeps the
                    // cursor's line visible.
                    gitClient().inlineDiffFile(topLevel, target.filePath.path());
                } else {
                    // Show the change that last touched the line: the blamed
                    // commit against its parent. This is the same regardless of
                    // the view the tooltip is shown in (working tree, index, or
                    // a revision), and following it repeatedly walks the line's
                    // history one changing commit at a time.
                    gitClient().inlineDiffRevisions(topLevel, target.filePath,
                                                    hash, target.originalFileName,
                                                    hash + "^", parentFileName,
                                                    target.originalLine);
                }
            } else if (link == "logLine") {
                qCInfo(log).nospace().noquote()
                    << "Showing log for: \"" << target.filePath << "\" line:" << target.line;

                const QString relativeFile = usePrevious ? target.originalFileName
                    : target.filePath.relativeChildPath(topLevel).path();
                const int line = usePrevious ? target.originalLine : target.line;
                const QString lineArg = QString("-L %1,%1:%2").arg(line).arg(relativeFile);
                QStringList arguments = {lineArg, "--no-patch"};
                if (usePrevious)
                    arguments.append(target.hash);
                gitClient().log(topLevel, {}, true, arguments);
            } else {
                qCInfo(log).nospace().noquote()
                    << "Showing commit: " << hash << " for " << target.filePath;
                gitClient().show(target.filePath, hash);
            }
        });
}

bool BlameMark::addToolTipContent(QLayout *target) const
{
    addBlameToolTipContent(target, m_info);
    return true;
}

static QString blameToolTipText(const CommitInfo &info)
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
                      "  <td><a href=\"copyHash\">%7</a></td>"
                      "</tr></table>"
                      "<p></p>")
                      .arg(blameRevision, blameParent, showFile, revert, logForLine,
                           diffParent, Tr::tr("Copy Hash"));
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

    QString result = actions;
    if (info.modified) {
        if (!diff.isEmpty()) {
            result += "<p><b>" + Tr::tr("Local changes") + "</b></p>";
            result += "<pre>" + diff + "</pre>";
        }
        const auto lookupState = info.previousCommit ? info.previousCommit->state
                                                     : PreviousCommitLookup::State::Unavailable;
        switch (lookupState) {
        case PreviousCommitLookup::State::Found: {
            const CommitInfo &previous = info.previousCommit->info;
            result += "<p><b>" + Tr::tr("Last committed version") + "</b></p>";
            result += QString(
                          "<table>"
                          "  <tr><td>commit</td><td><a style=\"color: %1;\" "
                          "href=\"previousShow\">%2</a></td></tr>"
                          "  <tr><td>Author:</td><td style=\"color: %3;\">%4 &lt;%5&gt;</td></tr>"
                          "  <tr><td>Date:</td><td style=\"color: %6;\">%7</td></tr>"
                          "</table>"
                          "<p style=\"color: %8;\">%9</p>")
                          .arg(colors.hash, previous.hash,
                               colors.author, previous.author.toHtmlEscaped(),
                               previous.authorMail.toHtmlEscaped(),
                               colors.date,
                               previous.authorDate.toString("yyyy-MM-dd hh:mm:ss"),
                               colors.subject, previous.subject.toHtmlEscaped());
            result += "<p><b>"
                      + Tr::tr("Actions for last committed version").toHtmlEscaped()
                      + "</b></p>";
            result += QString(
                          "<p><a href=\"previousBlame\">%1</a> | "
                          "<a href=\"previousBlameParent\">%2</a> | "
                          "<a href=\"previousShowFile\">%3</a> | "
                          "<a href=\"previousLogLine\">%4</a> | "
                          "<a href=\"previousDiffParent\">%5</a> | "
                          "<a href=\"previousCopyHash\">%6</a></p>")
                          .arg(Tr::tr("Blame").toHtmlEscaped(),
                               Tr::tr("Blame Parent").toHtmlEscaped(),
                               Tr::tr("File at Commit").toHtmlEscaped(),
                               Tr::tr("Log for Line").toHtmlEscaped(),
                               Tr::tr("Diff Against Parent").toHtmlEscaped(),
                               Tr::tr("Copy Hash").toHtmlEscaped());
            break;
        }
        case PreviousCommitLookup::State::Pending:
            result += "<p><b>" + Tr::tr("Looking up last committed version") + "</b></p>";
            break;
        case PreviousCommitLookup::State::NoPreviousLine:
            result += "<p><b>" + Tr::tr("No previous committed line") + "</b></p>";
            break;
        case PreviousCommitLookup::State::Unavailable:
            result += "<p><b>" + Tr::tr("Last committed version is unavailable") + "</b></p>";
            break;
        }
    } else {
        const QString header = QString(
                                   "<table>"
                                   "  <tr><td>commit</td><td><a style=\"color: %1;\" href=\"show\">%2</a></td></tr>"
                                   "  <tr><td>Author:</td><td style=\"color: %3;\">%4 &lt;%5&gt;</td></tr>"
                                   "  <tr><td>Date:</td><td style=\"color: %6;\">%7</td></tr>"
                                   "</table>"
                                   "<p style=\"color: %8;\">%9</p>")
                                   .arg(colors.hash, info.hash,
                                        colors.author, info.author.toHtmlEscaped(),
                                        info.authorMail.toHtmlEscaped(),
                                        colors.date,
                                        info.authorDate.toString("yyyy-MM-dd hh:mm:ss"),
                                        colors.subject, info.subject.toHtmlEscaped());
        result += header;
        if (!diff.isEmpty())
            result += "<pre>" + diff + "</pre>";
    }

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

void BlameMark::setPreviousCommit(const CommitInfo &info)
{
    m_info.previousCommit->info = info;
    m_info.previousCommit->state = PreviousCommitLookup::State::Found;
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
    if (!setEditor(widget)) {
        qCInfo(log) << "Cannot set up instant blame for the current editor.";
        return;
    }

    m_blameCursorPosConn = connect(widget, &PlainTextEdit::cursorPositionChanged,
                                   this, [this] { m_controller->schedule(500); });
    m_documentChangedConn = connect(m_document, &IDocument::changed,
                                    this, &InstantBlame::slotDocumentChanged);
    m_documentContentsChangedConn = connect(m_document, &IDocument::contentsChanged,
                                            this, &InstantBlame::slotDocumentChanged);
    m_modified = m_document->isModified();
}

bool InstantBlame::setEditor(TextEditorWidget *widget)
{
    if (!widget)
        return false;
    if (qobject_cast<const VcsBaseEditorWidget *>(widget)) {
        qCDebug(log) << "Deactivating in version control editors";
        return false;
    }

    m_document = widget->textDocument();

    const FilePath sourceFilePath = VcsBase::source(m_document);
    const FilePath workingFilePath = sourceFilePath.isEmpty() ? m_document->filePath()
                                                              : sourceFilePath;

    FilePath topLevel = FilePath::fromString(m_document->property("GitRepository").toString());
    if (topLevel.isEmpty())
        topLevel = currentState().currentFileTopLevel();

    if (topLevel.isEmpty() || !workingFilePath.isChildOf(topLevel)) {
        m_document = nullptr;
        return false;
    }

    m_controller->setContext(widget, topLevel,
                             m_document->property("GitReference").toString(),
                             workingFilePath.path(), workingFilePath);
    m_controller->setEnabled(true);
    return true;
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
// Each record starts with a header followed by key/value metadata. Optional
// records such as "previous" may be absent, and a "boundary" record may occur.
// The header contains the hash, original line, current line, and optionally the
// number of lines in this group when blaming multiple lines. The last line
// starts with a tab and is followed by the actual file content.
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
    if (blame.isEmpty())
        return result;

    const QStringList firstLineParts = blame.first().split(" ", Qt::SkipEmptyParts);
    if (firstLineParts.size() < 3)
        return result;

    result.hash = firstLineParts.first();
    result.originalLine = firstLineParts.at(1).toInt();
    result.line = line;
    result.filePath = filePath;

    qint64 authorTime = 0;
    for (const QString &entry : blame) {
        if (entry.startsWith('\t'))
            break;
        if (entry.startsWith("author ")) {
            result.author = entry.mid(7);
        } else if (entry.startsWith("author-mail ")) {
            result.authorMail = entry.mid(12);
            if (result.authorMail.startsWith('<') && result.authorMail.endsWith('>'))
                result.authorMail = result.authorMail.mid(1, result.authorMail.size() - 2);
        } else if (entry.startsWith("author-time ")) {
            authorTime = entry.mid(12).toLongLong();
        } else if (entry.startsWith("summary ")) {
            result.subject = entry.mid(8);
        } else if (entry.startsWith("previous ")) {
            const QString previous = entry.mid(9);
            const int space = previous.indexOf(' ');
            if (space > 0)
                result.previousFileName = previous.mid(space + 1);
        } else if (entry.startsWith("filename ")) {
            result.originalFileName = entry.mid(9);
        }
    }

    if (result.hash.isEmpty() || result.originalLine <= 0 || result.originalFileName.isEmpty())
        return {};

    result.modified = !GitClient::isValidRevision(result.hash);
    if (result.modified) {
        result.author = Tr::tr("Not Committed Yet");
        result.subject = Tr::tr("Modified line in %1").arg(filePath.fileName());
    }
    if (result.author == author.name || result.authorMail == author.email)
        result.shortAuthor = Tr::tr("You");
    else
        result.shortAuthor = result.author;
    result.authorDate = QDateTime::fromSecsSinceEpoch(authorTime);
    return result;
}

struct BlameGutterData : TextEditorWidget::AnnotationGutter
{
    QList<CommitInfo> commits;
    QList<int> baseLines;
    QList<int> originalLines;
    QHash<int, int> localItems;

    int localItem(int previous)
    {
        const auto found = localItems.constFind(previous);
        if (found != localItems.cend())
            return found.value();

        TextEditorWidget::AnnotationGutterItem item;
        CommitInfo info;
        if (previous >= 0) {
            item = items.at(previous);
            item.toolTip.prepend(Tr::tr("Last committed version") + '\n');
            info = commits.at(previous);
        } else {
            item.toolTip = Tr::tr("This line has uncommitted changes.");
        }
        item.text = Tr::tr("Local changes");
        item.displayGroup = "Git.BlameGutter.LocalChanges";
        info.modified = true;
        const int index = items.size();
        items.append(std::move(item));
        commits.append(std::move(info));
        localItems.insert(previous, index);
        return index;
    }
};

static BlameGutterData parseBlameGutter(const QByteArray &output,
                                        const FilePath &filePath,
                                        int lineCount)
{
    BlameGutterData gutter;
    if (output.isEmpty())
        return gutter;
    gutter.lineItems.fill(-1, lineCount);
    gutter.originalLines.fill(-1, lineCount);
    const QStringList lines = QString::fromUtf8(output).split('\n');
    QHash<QString, int> itemByHash;
    for (qsizetype i = 0; i < lines.size();) {
        const qsizetype header = i++;
        const QStringList fields = lines.at(header).split(' ', Qt::SkipEmptyParts);
        if (fields.size() < 3)
            return gutter;
        const QString &hash = fields.first();
        const int line = fields.at(2).toInt();
        while (i < lines.size() && !lines.at(i).startsWith('\t'))
            ++i;
        if (i == lines.size())
            break;
        const qsizetype metadataEnd = i++;
        if (line < 1 || line > lineCount)
            continue;

        auto it = itemByHash.constFind(hash);
        int itemIndex = -1;
        if (it != itemByHash.cend()) {
            itemIndex = it.value();
        } else {
            const CommitInfo info = parseBlameOutput(
                lines.sliced(header, metadataEnd - header), filePath, line, {});
            if (info.hash.isEmpty())
                continue;
            TextEditorWidget::AnnotationGutterItem item;
            if (info.modified) {
                item.text = Tr::tr("Local changes");
                item.displayGroup = "Git.BlameGutter.LocalChanges";
                item.toolTip = Tr::tr("This line has uncommitted changes.");
            } else {
                item.text = info.hash.left(8) + "  " + info.author;
                item.toolTip = QString("%1\n%2\n%3\n%4")
                                   .arg(info.hash, info.author,
                                        info.authorDate.toString("yyyy-MM-dd hh:mm"), info.subject);
                item.id = info.hash;
            }
            itemIndex = gutter.items.size();
            itemByHash.insert(hash, itemIndex);
            gutter.items.append(std::move(item));
            gutter.commits.append(info);
        }
        gutter.lineItems[line - 1] = itemIndex;
        gutter.originalLines[line - 1] = fields.at(1).toInt();
    }
    return gutter;
}

struct BlameGutterSnapshot
{
    QString text;
    QStringList lines;
    BlameGutterData data;
};

static BlameGutterSnapshot blameGutterSnapshot(const QByteArray &output,
                                              const FilePath &path,
                                              const TextEncoding &encoding)
{
    // Porcelain carries the original file bytes after each tab, independently of
    // the encoding used for commit metadata.
    QByteArray contents;
    int lineCount = 0;
    for (qsizetype start = 0; start < output.size();) {
        const qsizetype newline = output.indexOf('\n', start);
        const qsizetype end = newline < 0 ? output.size() : newline + 1;
        if (output.at(start) == '\t') {
            contents.append(QByteArrayView(output).sliced(start + 1, end - start - 1));
            ++lineCount;
        }
        start = end;
    }
    QString text = encoding.isValid() ? encoding.decode(contents) : QString::fromUtf8(contents);
    text.replace("\r\n", "\n");
    if (text.startsWith(QChar::ByteOrderMark))
        text.remove(0, 1);
    return {text, text.split('\n'), parseBlameGutter(output, path, lineCount)};
}

static void mapBlameGutter(QPromise<BlameGutterData> &promise,
                         const BlameGutterSnapshot &snapshot,
                         const QString &text,
                         int lineCount)
{
    BlameGutterData data = snapshot.data;
    data.baseLines.fill(-1, lineCount);
    if (snapshot.text == text) {
        for (int line = 0; line < qMin(lineCount, data.lineItems.size()); ++line)
            data.baseLines[line] = line;
        data.lineItems.resize(lineCount, -1);
        promise.addResult(std::move(data));
        return;
    }
    data.lineItems.fill(-1, lineCount);

    const auto chunk = diffChunkAgainstEditorText(snapshot.text, text,
                                                 QFuture<void>(promise.future()));
    if (promise.isCanceled())
        return;
    int oldLine = 0;
    int newLine = 0;
    QList<int> removed;
    QList<int> added;
    const auto applyChanges = [&] {
        for (int i = 0; i < added.size(); ++i) {
            const int baseLine = removed.size() == added.size() ? removed.at(i) : -1;
            const int line = added.at(i);
            data.baseLines[line] = baseLine;
            data.lineItems[line] = data.localItem(snapshot.data.lineItems.value(baseLine, -1));
        }
        removed.clear();
        added.clear();
    };
    for (const auto &row : chunk.rows) {
        const bool oldText = row.line[DiffEditor::LeftSide].textLineType
                             == DiffEditor::TextLineData::TextLine;
        const bool newText = row.line[DiffEditor::RightSide].textLineType
                             == DiffEditor::TextLineData::TextLine;
        const int baseLine = oldText ? oldLine++ : -1;
        if (row.equal) {
            applyChanges();
            if (newText && newLine < lineCount) {
                data.baseLines[newLine] = baseLine;
                data.lineItems[newLine] = snapshot.data.lineItems.value(baseLine, -1);
            }
        } else {
            if (oldText)
                removed.append(baseLine);
            if (newText && newLine < lineCount)
                added.append(newLine);
        }
        if (newText)
            ++newLine;
    }
    applyChanges();
    promise.addResult(std::move(data));
}

class BlameGutter final : public QObject
{
    Q_OBJECT

public:
    BlameGutter(TextEditorWidget *widget, const FilePath &topLevel)
        : QObject(widget)
        , m_widget(widget)
        , m_document(widget->textDocument())
        , m_topLevel(topLevel)
        , m_filePath(m_document->filePath())
    {
        m_timer.setSingleShot(true);
        connect(&m_timer, &QTimer::timeout, this, &BlameGutter::refresh);
        m_mappingTimer.setSingleShot(true);
        m_mappingTimer.setInterval(80);
        connect(&m_mappingTimer, &QTimer::timeout, this, &BlameGutter::updateMapping);
        m_blockCount = m_document->document()->blockCount();
        connect(m_document->document(), &QTextDocument::contentsChange,
                this, &BlameGutter::contentsChanged);
        connect(m_document, &IDocument::filePathChanged, this, &BlameGutter::detach);
        connect(widget, &TextEditorWidget::textDocumentChanged, this, &BlameGutter::detach);
        connect(VcsManager::instance(), &VcsManager::repositoryChanged, this,
                [this](const FilePath &repository) {
            if (repository == m_topLevel)
                schedule(100);
        });
        connect(&settings().instantBlameIgnoreSpaceChanges, &BaseAspect::changed, this,
                [this] { schedule(0); });
        connect(&settings().instantBlameIgnoreLineMoves, &BaseAspect::changed, this,
                [this] { schedule(0); });
        connect(&settings().blameGutterFields, &BaseAspect::changed, this, &BlameGutter::present);
        connect(&settings().blameGutterHeatmap, &BaseAspect::changed, this, &BlameGutter::present);
        connect(&settings().blameGutterHeatmapOnly, &BaseAspect::changed, this,
                &BlameGutter::present);
        present();
        schedule(0);
    }

    bool isAttached() const { return !m_document.isNull(); }

    void detach()
    {
        m_timer.stop();
        m_mappingTimer.stop();
        m_runner.reset();
        m_mappingRunner.reset();
        m_document.clear();
        m_widget->setAnnotationGutter(std::nullopt);
        deleteLater();
    }

private:
#ifdef WITH_TESTS
    friend class InstantBlameTest;
#endif

    void schedule(int delay)
    {
        if (!m_document)
            return;
        m_runner.reset();
        m_mappingTimer.stop();
        m_mappingRunner.reset();
        m_snapshot = {};
        m_data.lineItems.clear();
        m_data.baseLines.clear();
        m_presented.lineItems.clear();
        m_widget->setAnnotationGutter(m_presented);
        m_timer.start(delay);
    }

    void contentsChanged(int position, int removed, int added)
    {
        if (!m_document || (!removed && !added))
            return;

        const QTextDocument *document = m_document->document();
        const int blockCount = document->blockCount();
        const int oldBlockCount = std::exchange(m_blockCount, blockCount);
        if (m_blameSkippedForSize && document->characterCount() <= MaxMappedCharacters) {
            m_blameSkippedForSize = false;
            schedule(80);
            return;
        }
        const QTextBlock firstBlock = document->findBlock(position);
        const int first = firstBlock.blockNumber();
        const int last = document->findBlock(position + added).blockNumber();
        if (first >= 0 && first == last && oldBlockCount == blockCount
            && !m_mappingTimer.isActive() && !m_mappingRunner.isRunning()
            && m_data.baseLines.size() == blockCount) {
            // Preserve the original row even when several committed lines have identical text.
            const int baseLine = m_data.baseLines.at(first);
            const int previous = m_snapshot.data.lineItems.value(baseLine, -1);
            const int itemCount = m_data.items.size();
            const int item = baseLine >= 0 && firstBlock.text() == m_snapshot.lines.value(baseLine)
                                 ? previous : m_data.localItem(previous);
            if (m_data.lineItems.at(first) != item) {
                m_data.lineItems[first] = item;
                if (m_data.items.size() != itemCount) {
                    present();
                } else {
                    m_presented.lineItems = m_data.lineItems;
                    m_widget->setAnnotationGutter(m_presented);
                }
            }
            return;
        }
        if (m_data.baseLines.size() == oldBlockCount) {
            const int newCount = last - first + 1;
            const int oldCount = newCount + oldBlockCount - blockCount;
            if (first >= 0 && last >= first && oldCount > 0 && first + oldCount <= oldBlockCount) {
                // Keep unaffected labels aligned while the background mapping resolves this edit.
                m_data.lineItems.remove(first, oldCount);
                m_data.lineItems.insert(first, newCount, -1);
                m_data.baseLines.remove(first, oldCount);
                m_data.baseLines.insert(first, newCount, -1);
                m_presented.lineItems = m_data.lineItems;
                m_widget->setAnnotationGutter(m_presented);
            }
        }
        m_mappingRunner.reset();
        if (!m_snapshot.data.items.isEmpty())
            m_mappingTimer.start();
    }

    void updateMapping()
    {
        m_mappingRunner.reset();
        if (!m_document || m_snapshot.data.items.isEmpty())
            return;
        if (m_document->document()->characterCount() > MaxMappedCharacters) {
            showStatus(Tr::tr("File too large"));
            return;
        }
        const QString text = m_document->plainText();
        const int lineCount = m_document->document()->blockCount();
        m_mappingRunner.start({AsyncTask<BlameGutterData>(
            [this, text, lineCount](Async<BlameGutterData> &task) {
                task.setConcurrentCallData(mapBlameGutter, m_snapshot, text, lineCount);
            },
            [this](const Async<BlameGutterData> &task) {
                m_data = task.result();
                present();
            }, CallDoneFlag::OnSuccess)});
    }

    void refresh()
    {
        m_runner.reset();
        if (!m_document)
            return;

        m_blameSkippedForSize = m_document->document()->characterCount() > MaxMappedCharacters;
        if (m_blameSkippedForSize) {
            showStatus(Tr::tr("File too large"));
            return;
        }

        QStringList args = {"blame", "--porcelain", "--encoding=UTF-8"};
        if (settings().instantBlameIgnoreSpaceChanges())
            args.append("-w");
        if (settings().instantBlameIgnoreLineMoves())
            args.append("-M");
        args.append({"HEAD", "--", m_filePath.relativeChildPath(m_topLevel).path()});
        const Storage<QByteArray> output;
        const Storage<QString> error;
        const auto onSetup = [output, path = m_filePath, encoding = m_document->encoding()](
                                 Async<BlameGutterSnapshot> &task) {
            task.setConcurrentCallData(blameGutterSnapshot, *output, path, encoding);
        };
        const auto onDone = [this, error](const Async<BlameGutterSnapshot> &task) {
            m_snapshot = task.result();
            if (m_snapshot.data.items.isEmpty()) {
                m_mappingRunner.reset();
                const QString reason = error->section('\n', 0, 0).left(200);
                showStatus(Tr::tr("No blame available"), reason);
                return;
            }
            updateMapping();
        };
        m_runner.start({
            output,
            error,
            Group {
                finishAllAndSuccess,
                gitClient().commandTask({
                    .workingDirectory = m_topLevel,
                    .arguments = args,
                    .flags = RunFlag::NoOutput,
                    .commandHandler = [output, error](const CommandResult &result) {
                        if (result.result() == ProcessResult::FinishedWithSuccess)
                            *output = result.rawStdOut();
                        else
                            *error = result.cleanedStdErr().trimmed();
                    }})
            },
            AsyncTask<BlameGutterSnapshot>(onSetup, onDone, CallDoneFlag::OnSuccess)
        });
    }

    bool addLineToolTip(int line, QLayout *layout) const
    {
        const int index = m_data.lineItems.value(line, -1);
        if (!m_document || index < 0 || index >= m_data.commits.size())
            return false;

        const int baseLine = m_data.baseLines.value(line, -1);
        const int previousIndex = m_snapshot.data.lineItems.value(baseLine, -1);
        CommitInfo info = m_data.commits.at(index);
        info.topLevel = m_topLevel;
        info.filePath = m_filePath;
        info.line = line + 1;
        info.originalLine = m_snapshot.data.originalLines.value(baseLine, -1);
        if (info.modified) {
            info.newLine = m_document->document()->findBlockByNumber(line).text();
            info.previousCommit = std::make_shared<PreviousCommitLookup>();
            info.previousCommit->state = PreviousCommitLookup::State::NoPreviousLine;
            if (previousIndex >= 0) {
                info.oldLines = {m_snapshot.lines.value(baseLine)};
                auto &previous = info.previousCommit->info;
                previous = m_snapshot.data.commits.at(previousIndex);
                previous.topLevel = m_topLevel;
                previous.line = baseLine + 1;
                previous.originalLine = info.originalLine;
                info.previousCommit->state = PreviousCommitLookup::State::Found;
            }
        }
        addBlameToolTipContent(layout, info);
        return true;
    }

    static constexpr int MaxMappedCharacters = 2 * 1024 * 1024;

    void present()
    {
        if (!m_document)
            return;
        TextEditorWidget::AnnotationGutter gutter = m_data;
        const bool heatmapOnly = settings().blameGutterHeatmapOnly();
        gutter.colorOnly = heatmapOnly && !m_data.commits.isEmpty();
        gutter.populateToolTip = [this](int line, QLayout *layout) {
            return addLineToolTip(line, layout);
        };
        gutter.populateContextMenu = [this](QMenu &menu) {
            const int fields = settings().blameGutterFields();
            const auto addField = [this, &menu, fields](BlameGutterField field, const QString &title) {
                QAction *action = menu.addAction(title, this, [field](bool checked) {
                    const int current = settings().blameGutterFields();
                    settings().blameGutterFields.setValue(checked ? current | field
                                                                  : current & ~field);
                    settings().writeSettings();
                });
                action->setCheckable(true);
                action->setChecked(fields & field);
                action->setEnabled(!settings().blameGutterHeatmapOnly() && fields != field);
            };
            addField(BlameHash, Tr::tr("Hash"));
            addField(BlameDate, Tr::tr("Date"));
            addField(BlameAuthor, Tr::tr("Author"));
            addField(BlameSubject, Tr::tr("Subject"));
            menu.addSeparator();
            QAction *heatmap = menu.addAction(Tr::tr("Show Age Heatmap"), this, [](bool checked) {
                settings().blameGutterHeatmap.setValue(checked);
                settings().writeSettings();
            });
            heatmap->setCheckable(true);
            heatmap->setChecked(settings().blameGutterHeatmap());
            heatmap->setEnabled(!settings().blameGutterHeatmapOnly());
            QAction *ageOnly = menu.addAction(Tr::tr("Heatmap Only"), this, [](bool checked) {
                settings().blameGutterHeatmapOnly.setValue(checked);
                settings().writeSettings();
            });
            ageOnly->setCheckable(true);
            ageOnly->setChecked(settings().blameGutterHeatmapOnly());
            menu.addAction(Tr::tr("Hide Blame Gutter"), this, &BlameGutter::detach);
        };
        gutter.activate = [path = m_filePath](const QString &hash) { gitClient().show(path, hash); };

        QDateTime oldest;
        QDateTime newest;
        for (const CommitInfo &info : std::as_const(m_data.commits)) {
            if (info.modified || !info.authorDate.isValid())
                continue;
            if (!oldest.isValid() || info.authorDate < oldest)
                oldest = info.authorDate;
            if (!newest.isValid() || info.authorDate > newest)
                newest = info.authorDate;
        }
        const QColor blue = Utils::creatorColor(Utils::Theme::IconsInfoColor);
        const QColor red = Utils::creatorColor(Utils::Theme::TextColorError);
        const qint64 duration = oldest.secsTo(newest);
        for (int i = 0; i < m_data.commits.size(); ++i) {
            const CommitInfo &info = m_data.commits.at(i);
            if (info.modified)
                continue;
            auto &item = gutter.items[i];
            const int fields = settings().blameGutterFields();
            QStringList text;
            if (fields & BlameHash)
                text.append(info.hash.left(8));
            if (fields & BlameDate)
                text.append(info.authorDate.toString("yyyy-MM-dd"));
            if (fields & BlameAuthor)
                text.append(info.author);
            if (fields & BlameSubject)
                text.append(info.subject);
            item.text = text.join("  ");
            if ((heatmapOnly || settings().blameGutterHeatmap()) && info.authorDate.isValid()) {
                const qreal t = duration ? qreal(oldest.secsTo(info.authorDate)) / duration : 0.5;
                item.foreground = QColor(qRound(blue.red() + t * (red.red() - blue.red())),
                                         qRound(blue.green() + t * (red.green() - blue.green())),
                                         qRound(blue.blue() + t * (red.blue() - blue.blue())));
            }
        }
        m_presented = std::move(gutter);
        m_widget->setAnnotationGutter(m_presented);
    }

    void showStatus(const QString &text, const QString &details = {})
    {
        m_data = {};
        m_data.lineItems.append(0);
        m_data.items.append({text, details.isEmpty() ? text : details, {}, {}});
        present();
    }

    BlameGutterData m_data;
    TextEditorWidget::AnnotationGutter m_presented;
    BlameGutterSnapshot m_snapshot;
    int m_blockCount = 0;
    bool m_blameSkippedForSize = false;
    TextEditorWidget *const m_widget;
    QPointer<TextDocument> m_document;
    const FilePath m_topLevel;
    const FilePath m_filePath;
    QTimer m_timer;
    QTimer m_mappingTimer;
    QSingleTaskTreeRunner m_runner;
    QSingleTaskTreeRunner m_mappingRunner;
};

void toggleBlameGutter(TextEditorWidget *widget, const FilePath &topLevel)
{
    if (!widget || qobject_cast<VcsBaseEditorWidget *>(widget))
        return;
    for (BlameGutter *gutter : widget->findChildren<BlameGutter *>(Qt::FindDirectChildrenOnly)) {
        if (gutter->isAttached()) {
            gutter->detach();
            return;
        }
    }
    if (!topLevel.isEmpty() && widget->textDocument()->filePath().isChildOf(topLevel))
        new BlameGutter(widget, topLevel);
}

static QStringList blameCommandArguments(const QString &filePath,
                                         const QString &ref,
                                         int line,
                                         bool ignoreSpaceChanges,
                                         bool detectMovedLines,
                                         bool useDocumentContents)
{
    QStringList arguments = {"blame", "-p"};
    if (useDocumentContents)
        arguments.append({"--contents", "-"});
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

QStringList instantBlameArguments(const QString &filePath, const QString &ref, int line)
{
    return blameCommandArguments(filePath, ref, line, settings().instantBlameIgnoreSpaceChanges(),
                                 settings().instantBlameIgnoreLineMoves(), false);
}

DiffEditor::LineAnnotation instantBlameAnnotation(const QString &output,
                                                  const FilePath &topLevel,
                                                  const FilePath &filePath, int line)
{
    CommitInfo info = parseBlameOutput(output.split('\n'), filePath, line, {});
    if (info.hash.isEmpty())
        return {};
    info.topLevel = topLevel;
    return [info](TextDocument *document, int displayLine) {
        return std::make_unique<BlameMark>(document, displayLine, info);
    };
}

void InstantBlame::once()
{
    if (settings().instantBlame()) {
        scheduleInstantBlame();
        return;
    }

    stop();
    TextEditorWidget *widget = TextEditorWidget::currentTextEditorWidget();
    if (!setEditor(widget)) {
        qCWarning(log) << "Cannot get a suitable text editor widget";
        return;
    }
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
    disconnect(m_documentContentsChangedConn);
    m_document = nullptr;
    m_modified = false;
}

void InstantBlame::slotDocumentChanged()
{
    if (!m_document) {
        qCWarning(log) << "Document is invalid, disconnecting.";
        disconnect(m_documentChangedConn);
        disconnect(m_documentContentsChangedConn);
        return;
    }

    const bool modified = m_document->isModified();
    qCDebug(log) << "Document is changed, modified:" << modified;
    if (modified) {
        m_controller->schedule(500);
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
                                 const FilePath &workingFilePath)
{
    clear();
    ++m_contextGeneration;
    m_widget = widget;
    m_document = widget ? widget->textDocument() : nullptr;
    m_topLevel = topLevel;
    m_ref = ref;
    m_commandFilePath = commandFilePath;
    m_workingFilePath = workingFilePath;
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
    clearHelper();
    m_taskTreeRunner.reset();
}

void BlameController::clearHelper()
{
    m_timer->stop();
    m_blameMark.reset();
    disconnect(m_repositoryChangedConn);
    m_lastLine = -1;
    ++m_requestGeneration;
}

void BlameController::waitForRepositoryChange()
{
    // called from the blame handler: resetting it would delete task tree running that handler
    m_enabled = false;
    clearHelper();
    const FilePath topLevel = m_topLevel;
    m_repositoryChangedConn = connect(
        VcsManager::instance(), &VcsManager::repositoryChanged, this,
        [this, topLevel](const FilePath &repository) {
            if (topLevel != repository && !topLevel.isChildOf(repository))
                return;
            disconnect(m_repositoryChangedConn);
            m_repositoryChangedConn = {};
            setEnabled(true);
        });
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

static void addModifiedLineDiff(BlameMark &mark, const EditorLineDiff &diff)
{
    if (!diff.isValid)
        return;
    for (const QString &oldLine : diff.oldLines)
        mark.addOldLine("-" + oldLine);
    mark.addNewLine("+" + diff.newLine);
}

void BlameController::perform()
{
    if (!m_widget || !m_document || m_topLevel.isEmpty()) {
        clear();
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

    const bool useDocumentContents = m_document->isModified();
    const bool ignoreSpaceChanges = settings().instantBlameIgnoreSpaceChanges();
    const bool ignoreLineMoves = settings().instantBlameIgnoreLineMoves();
    const QStringList options = blameCommandArguments(
        m_commandFilePath,
        m_ref,
        line,
        ignoreSpaceChanges,
        ignoreLineMoves,
        useDocumentContents);
    qCDebug(log) << "Running git" << options.join(' ');

    const quint64 generation = ++m_requestGeneration;
    const FilePath topLevel = m_topLevel;
    const FilePath workingFilePath = m_workingFilePath;
    const QString ref = m_ref;
    const TextEncoding encoding = m_encoding;
    const Author author = m_author;
    const QPointer<TextDocument> document = m_document;
    // Editor text is needed for diff generation: a document can be unmodified while
    // its contents still differ from HEAD. This is independent of useDocumentContents,
    // which only controls whether the text is passed to git blame via stdin.
    const QString editorText = document->plainText();
    const TextEncoding sourceEncoding = document->encoding();
    const QByteArray writeData = [sourceEncoding, editorText, useDocumentContents] {
        if (!useDocumentContents)
            return QByteArray();
        if (sourceEncoding.isUtf8())
            return editorText.toUtf8();
        return sourceEncoding.encode(editorText);
    }();
    const QPointer<BlameController> guard(this);
    const Storage<CommitInfo> infoStorage;
    const Storage<QString> baselineTextStorage;
    const Storage<EditorLineDiff> lineDiffStorage;
    const auto blameHandler = [guard, document, generation, workingFilePath, topLevel, line,
                               author, infoStorage](const CommandResult &result) {
        if (!guard || !document || guard->m_requestGeneration != generation)
            return;
        const QString output = result.cleanedStdOut();
        if (result.result() != ProcessResult::FinishedWithSuccess || output.isEmpty()) {
            guard->waitForRepositoryChange();
            return;
        }
        *infoStorage = parseBlameOutput(output.split('\n'), workingFilePath, line, author);
        if (infoStorage->hash.isEmpty()) {
            guard->m_blameMark.reset();
            guard->m_lastLine = -1;
            return;
        }
        infoStorage->topLevel = topLevel;
        guard->m_blameMark = std::make_unique<BlameMark>(document.get(), line, *infoStorage);
    };
    const auto onLogSetup = [infoStorage, topLevel, ref, encoding,
                             sourceEncoding](Process &process) -> SetupResult {
        if (infoStorage->hash.isEmpty())
            return SetupResult::StopWithSuccess;

        if (infoStorage->modified) {
            const QString baselineRef = ref.isEmpty() ? QString("HEAD") : ref;
            const QStringList showOptions
                = {"show", baselineRef + ":" + infoStorage->originalFileName};
            qCDebug(log) << "Running git" << showOptions.join(' ');
            gitClient().setupCommand(process, topLevel, showOptions);
            Environment environment = process.environment();
            environment.set("LANG", "C");
            environment.set("LANGUAGE", "C");
            process.setEnvironment(environment);
            process.setEncoding(sourceEncoding);
            return SetupResult::Continue;
        }

        // Get line diff: `git log -n 1 -p -L47,47:README.md a5c4c34c9ab4`
        const QString origLineString = QString("%1,%1").arg(infoStorage->originalLine);
        const QString fileLineRange = "-L" + origLineString + ":" + infoStorage->originalFileName;
        const QStringList logOptions = {"log", "-n 1", "-p", fileLineRange, infoStorage->hash};
        qCDebug(log) << "Running git" << logOptions.join(' ');
        gitClient().setupCommand(process, topLevel, logOptions);
        process.setEncoding(encoding);
        return SetupResult::Continue;
    };
    const auto onLogDone = [guard, generation, infoStorage, baselineTextStorage](
                               const Process &process) -> DoneResult {
        if (!guard || guard->m_requestGeneration != generation || !guard->m_blameMark)
            return DoneResult::Error;
        const QString error = process.cleanedStdErr().trimmed();
        if (process.result() != ProcessResult::FinishedWithSuccess) {
            if (infoStorage->modified
                && (error.contains("does not exist in") || error.contains("exists on disk, but not in"))) {
                // A staged addition has no contents at the baseline revision.
                return DoneResult::Success;
            }
            if (!error.isEmpty())
                qCWarning(log) << error;
            return DoneResult::Error;
        }
        if (infoStorage->modified) {
            *baselineTextStorage = process.cleanedStdOut();
            baselineTextStorage->replace("\r\n", "\n");
            return DoneResult::Success;
        }
        static const QRegularExpression re("^[-+][^-+].*");
        const QStringList diffLines = process.cleanedStdOut().split("\n").filter(re);
        for (const QString &diffLine : diffLines) {
            if (diffLine.startsWith("-"))
                guard->m_blameMark->addOldLine(diffLine);
            else if (diffLine.startsWith("+"))
                guard->m_blameMark->addNewLine(diffLine);
        }
        return DoneResult::Success;
    };
    const auto onDiffSetup
        = [infoStorage, baselineTextStorage, editorText](Async<EditorLineDiff> &async) {
        if (!infoStorage->modified)
            return SetupResult::StopWithSuccess;
        async.setConcurrentCallData(computeEditorLineDiff, *baselineTextStorage,
                                    editorText, infoStorage->line);
        return SetupResult::Continue;
    };
    const auto onDiffDone = [guard, generation, lineDiffStorage](
                                const Async<EditorLineDiff> &async) {
        if (!guard || guard->m_requestGeneration != generation || !guard->m_blameMark
            || !async.isResultAvailable()) {
            return;
        }
        *lineDiffStorage = async.result();
        addModifiedLineDiff(*guard->m_blameMark, *lineDiffStorage);
        if (lineDiffStorage->isValid
            && lineDiffStorage->oldLineNumbers.isEmpty()) {
            guard->m_blameMark->setNoPreviousLine();
        }
    };

    const auto onPreviousBlameSetup = [infoStorage, lineDiffStorage, topLevel, ref,
                                       ignoreSpaceChanges, ignoreLineMoves,
                                       encoding](Process &process) -> SetupResult {
        if (!infoStorage->modified || !lineDiffStorage->isValid
            || lineDiffStorage->oldLineNumbers.isEmpty()) {
            return SetupResult::StopWithSuccess;
        }

        const QString baselineRef = ref.isEmpty() ? QString("HEAD") : ref;
        const int previousLine = lineDiffStorage->oldLineNumbers.first();
        const QStringList arguments = blameCommandArguments(
            infoStorage->originalFileName, baselineRef, previousLine,
            ignoreSpaceChanges, ignoreLineMoves, false);
        gitClient().setupCommand(process, topLevel, arguments);
        process.setEncoding(encoding);
        return SetupResult::Continue;
    };
    const auto onPreviousBlameDone = [guard, generation, lineDiffStorage,
                                      workingFilePath, topLevel, author](const Process &process) {
        if (!guard || guard->m_requestGeneration != generation || !guard->m_blameMark
            || !lineDiffStorage->isValid || lineDiffStorage->oldLineNumbers.isEmpty()
            || process.result() != ProcessResult::FinishedWithSuccess) {
            return;
        }

        const int previousLine = lineDiffStorage->oldLineNumbers.first();
        CommitInfo previous = parseBlameOutput(process.cleanedStdOut().split('\n'),
                                               workingFilePath, previousLine, author);
        if (previous.hash.isEmpty() || previous.modified)
            return;
        previous.topLevel = topLevel;
        guard->m_blameMark->setPreviousCommit(previous);
    };

    m_taskTreeRunner.start({
        infoStorage,
        baselineTextStorage,
        lineDiffStorage,
        gitClient().commandTask({topLevel, options, RunFlag::NoOutput, {}, encoding,
                                 blameHandler, writeData}),
        ProcessTask(onLogSetup, onLogDone, CallDoneFlag::Always),
        AsyncTask<EditorLineDiff>(onDiffSetup, onDiffDone, CallDoneFlag::OnSuccess),
        ProcessTask(onPreviousBlameSetup, onPreviousBlameDone, CallDoneFlag::Always),
        onGroupDone([guard, generation] {
            if (guard && guard->m_requestGeneration == generation && guard->m_blameMark)
                guard->m_blameMark->finishPreviousLookup();
        }),
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
    m_controller->setContext(widget, topLevel, ref, relativeFile, workingFilePath);
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
    void testBlameDocumentContents();
    void testBlameGutter();
    void testBlameGutterEditor();
};

void InstantBlameTest::testBlameCommandArguments()
{
    QCOMPARE(blameCommandArguments("src/main.cpp", {}, 17, false, false, false),
             QStringList({"blame", "-p", "-L", "17,17", "--", "src/main.cpp"}));
    QCOMPARE(blameCommandArguments("file.h", "HEAD^", 2, true, false, false),
             QStringList({"blame", "-p", "-w", "-L", "2,2", "HEAD^", "--", "file.h"}));
    QCOMPARE(blameCommandArguments("file.h", "HEAD^", 3, false, true, false),
             QStringList({"blame", "-p", "-M", "-L", "3,3", "HEAD^", "--", "file.h"}));
    QCOMPARE(blameCommandArguments("file.h", "HEAD^", 4, true, true, false),
             QStringList({"blame", "-p", "-w", "-M", "-L", "4,4", "HEAD^", "--", "file.h"}));
    QCOMPARE(blameCommandArguments("index.cpp", {}, 9, false, false, true),
             QStringList({"blame", "-p", "--contents", "-", "-L", "9,9", "--", "index.cpp"}));
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

    QStringList directOutput = output;
    directOutput.removeAt(10);
    const CommitInfo directInfo = parseBlameOutput(directOutput, filePath, 5, {});
    QCOMPARE(directInfo.originalFileName, "foo.cpp");
    QVERIFY(directInfo.previousFileName.isEmpty());

    const QStringList modifiedOutput = {
        "0000000000000000000000000000000000000000 7 7 1",
        "author Not Committed Yet",
        "author-mail <not.committed.yet>",
        "author-time 0",
        "author-tz +0000",
        "committer Not Committed Yet",
        "committer-mail <not.committed.yet>",
        "committer-time 0",
        "committer-tz +0000",
        "summary Version of foo.cpp from foo.cpp",
        "filename foo.cpp",
        "\tchanged contents",
    };
    const CommitInfo modifiedInfo = parseBlameOutput(modifiedOutput, filePath, 7, {});
    QVERIFY(modifiedInfo.modified);
    QCOMPARE(modifiedInfo.author, Tr::tr("Not Committed Yet"));
    QCOMPARE(modifiedInfo.subject, Tr::tr("Modified line in %1").arg(filePath.fileName()));
    QCOMPARE(modifiedInfo.originalLine, 7);

    const CommitInfo malformedInfo = parseBlameOutput({"invalid"}, filePath, 1, {});
    QVERIFY(malformedInfo.hash.isEmpty());
    QCOMPARE(malformedInfo.line, -1);

    const CommitInfo incompleteInfo = parseBlameOutput({
        "8b649d2d61416205977aba56ef93e1e1f155005e 4 5 1",
        "author John Doe",
        "author-mail <john.doe@example.com>",
        "author-time 1613752276",
        "\techo Hello World!",
    }, filePath, 5, {});
    QVERIFY(incompleteInfo.hash.isEmpty());
    QCOMPARE(incompleteInfo.line, -1);
}

void InstantBlameTest::testBlameGutter()
{
    const QString hash(40, 'a');
    const QString local(40, '0');
    const QString output = hash + " 1 1 2\n"
        "author Alex Example\n"
        "author-time 1613752276\n"
        "summary Initial commit\n"
        "filename sample.cpp\n"
        "\tfirst\n"
        + hash + " 2 2\n\tsecond\n"
        + local + " 3 3 1\n"
        "author Not Committed Yet\n"
        "filename sample.cpp\n"
        "\tthird\n";
    const auto gutter = parseBlameGutter(output.toUtf8(), FilePath::fromString("sample.cpp"), 4);
    QCOMPARE(gutter.lineItems, QList<int>({0, 0, 1, -1}));
    QCOMPARE(gutter.items.size(), 2);
    QCOMPARE(gutter.items.first().text, QString("aaaaaaaa  Alex Example"));
    QCOMPARE(gutter.items.first().id, hash);
    QVERIFY(gutter.items.first().toolTip.contains("Initial commit"));
    QCOMPARE(gutter.items.last().text, Tr::tr("Local changes"));
    QVERIFY(gutter.items.last().id.isEmpty());
}

void InstantBlameTest::testBlameGutterEditor()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const FilePath repo = FilePath::fromString(directory.path()).canonicalPath();
    const auto git = [repo](const QStringList &arguments) {
        return gitClient().vcsSynchronousExec(repo, arguments).result()
               == ProcessResult::FinishedWithSuccess;
    };
    QVERIFY(git({"init", "-b", "main", "."}));
    QVERIFY(git({"config", "user.name", "Gutter Tester"}));
    QVERIFY(git({"config", "user.email", "gutter@test"}));
    const FilePath file = repo / "sample.txt";
    QVERIFY(file.writeFileContents("first\nsecond\nthird\n"));
    QVERIFY(git({"add", "sample.txt"}));
    QVERIFY(git({"-c", "commit.gpgsign=false", "commit", "-m", "Initial contents"}));

    IEditor *editor = EditorManager::openEditor(file);
    QVERIFY(editor);
    const auto cleanup = qScopeGuard([guard = QPointer<IEditor>(editor)] {
        if (guard)
            EditorManager::closeEditors({guard}, false);
    });
    auto *textEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(textEditor);
    auto *widget = textEditor->editorWidget();
    toggleBlameGutter(widget, repo);
    QVERIFY(widget->hasAnnotationGutter());
    QPointer<BlameGutter> gutter = widget->findChild<BlameGutter *>();
    QVERIFY(gutter);
    QTRY_COMPARE(gutter->m_data.lineItems.value(2, -1), 0);
    {
        QWidget toolTip;
        auto *layout = new QVBoxLayout(&toolTip);
        QVERIFY(gutter->addLineToolTip(2, layout));
        const auto label = toolTip.findChild<QLabel *>();
        QVERIFY(label);
        QVERIFY(label->text().contains("Log for line 3"));
        QVERIFY(label->text().contains("href=\"diffParent\""));
        QVERIFY(label->text().contains("href=\"copyHash\""));
    }
    {
        const bool oldMode = settings().blameGutterHeatmapOnly();
        const auto resetMode = qScopeGuard([oldMode] {
            settings().blameGutterHeatmapOnly.setValue(oldMode);
        });
        settings().blameGutterHeatmapOnly.setValue(false);
        const int textWidth = widget->extraAreaWidth();
        settings().blameGutterHeatmapOnly.setValue(true);
        QVERIFY(gutter->m_presented.colorOnly);
        QVERIFY(gutter->m_presented.items.first().foreground.isValid());
        QVERIFY(widget->extraAreaWidth() < textWidth);
    }

    QTextCursor cursor(widget->document());
    cursor.insertText("inserted\n");
    QTRY_COMPARE(gutter->m_data.lineItems.value(3, -1), 0);
    QVERIFY(!gutter->m_timer.isActive());
    QTRY_VERIFY(gutter->m_data.lineItems.value(0, -1) >= 0);
    QVERIFY(gutter->m_data.commits.at(gutter->m_data.lineItems.at(0)).modified);

    cursor.movePosition(QTextCursor::NextBlock);
    cursor.movePosition(QTextCursor::EndOfBlock);
    cursor.insertText(" changed");
    QVERIFY(!gutter->m_mappingTimer.isActive());
    QVERIFY(!gutter->m_mappingRunner.isRunning());
    QVERIFY(gutter->m_data.lineItems.value(2, -1) > 0);
    QVERIFY(gutter->m_data.items.at(gutter->m_data.lineItems.at(2))
                .toolTip.contains("Initial contents"));

    {
        QWidget toolTip;
        auto *layout = new QVBoxLayout(&toolTip);
        QVERIFY(gutter->addLineToolTip(2, layout));
        const auto label = toolTip.findChild<QLabel *>();
        QVERIFY(label);
        QVERIFY(label->text().contains("second changed"));
        QVERIFY(label->text().contains("href=\"previousDiffParent\""));
    }

    // A gutter opened on a dirty buffer also compares against the committed version.
    toggleBlameGutter(widget, repo);
    QTRY_VERIFY(gutter.isNull());
    toggleBlameGutter(widget, repo);
    gutter = widget->findChild<BlameGutter *>();
    QVERIFY(gutter);
    QTRY_VERIFY(gutter->m_data.lineItems.value(2, -1) > 0);
    QVERIFY(gutter->m_data.items.at(gutter->m_data.lineItems.at(2))
                .toolTip.contains("Initial contents"));
    widget->document()->undo();
    QTRY_COMPARE(gutter->m_data.lineItems.value(2, -1), 0);

    cursor = QTextCursor(widget->document());
    cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    QTRY_COMPARE(gutter->m_data.lineItems.value(0, -1), 0);
    cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    QTRY_COMPARE(gutter->m_data.lineItems.value(0, -1), 0);
    QCOMPARE(gutter->m_data.lineItems.size(), 3);
    QCOMPARE(gutter->m_data.lineItems.value(1, -1), 0);
    QVERIFY(!gutter->m_timer.isActive());

    const QString initialHash = gutter->m_data.commits.first().hash;
    QVERIFY(git({"branch", "initial"}));
    QVERIFY(git({"config", "commit.gpgsign", "false"}));
    QVERIFY(file.writeFileContents("second committed\nthird\n"));
    widget->document()->setPlainText("second committed\nthird\n");
    QTRY_VERIFY(gutter->m_data.lineItems.value(0, -1) > 0);
    QVERIFY(gutter->m_data.commits.at(gutter->m_data.lineItems.at(0)).modified);

    SubmitFileModel model;
    model.addFile("sample.txt", "Modified", Checked, int(ModifiedFile));
    const FilePath message = repo / "message.txt";
    QVERIFY(message.writeFileContents("Updated contents\n"));
    gutter->refresh();
    QVERIFY(gitClient().addAndCommit(repo, {}, SimpleCommit, {}, message, &model));
    QTRY_VERIFY(gutter->m_data.lineItems.value(0, -1) >= 0
                && !gutter->m_data.commits.at(gutter->m_data.lineItems.at(0)).modified);
    const QString updatedHash
        = gutter->m_data.commits.at(gutter->m_data.lineItems.at(0)).hash;
    QVERIFY(updatedHash != initialHash);

    QVERIFY(git({"checkout", "-b", "onto", "initial"}));
    QVERIFY((repo / "unrelated.txt").writeFileContents("unrelated\n"));
    QVERIFY(git({"add", "unrelated.txt"}));
    QVERIFY(git({"-c", "commit.gpgsign=false", "commit", "-m", "Unrelated contents"}));
    QVERIFY(git({"checkout", "main"}));
    QVERIFY(git({"-c", "commit.gpgsign=false", "rebase", "onto"}));
    emit VcsManager::instance()->repositoryChanged(repo);
    QTRY_VERIFY(gutter->m_data.lineItems.value(0, -1) >= 0);
    const QString rebasedHash = gutter->m_data.commits.at(gutter->m_data.lineItems.at(0)).hash;
    QVERIFY(rebasedHash != updatedHash);
    QVERIFY(rebasedHash != initialHash);

    QVERIFY(git({"checkout", "initial"}));
    widget->document()->setPlainText("first\nsecond\nthird\n");
    emit VcsManager::instance()->repositoryChanged(repo);
    QTRY_VERIFY(gutter->m_data.lineItems.value(0, -1) >= 0);
    QCOMPARE(gutter->m_data.commits.at(gutter->m_data.lineItems.at(0)).hash, initialHash);

    cursor = QTextCursor(widget->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(QString("local line\n").repeated(10000));
    cursor.insertText("last local line");
    QVERIFY(gutter->m_mappingTimer.isActive());
    QVERIFY(!gutter->m_mappingRunner.isRunning());
    QTRY_VERIFY(gutter->m_data.lineItems.last() >= 0);
    QCOMPARE(gutter->m_data.lineItems.size(), widget->document()->blockCount());
    // A new line and a changed committed line share a display group, but have distinct tooltips.
    auto data = gutter->m_data;
    const int newItem = data.localItem(-1);
    const int changedItem = data.localItem(0);
    QVERIFY(newItem != changedItem);
    QVERIFY(data.items.at(newItem).displayGroup.isValid());
    QCOMPARE(data.items.at(newItem).displayGroup, data.items.at(changedItem).displayGroup);
    QVERIFY(data.items.at(newItem).id.isEmpty());
    QVERIFY(!data.items.at(changedItem).id.isEmpty());

    const int lastItem = gutter->m_data.lineItems.last();
    QVERIFY(gutter->m_data.commits.at(lastItem).modified);
    QVERIFY(!gutter->m_runner.isRunning());

    gutter->refresh();
    toggleBlameGutter(widget, repo);
    QVERIFY(!widget->hasAnnotationGutter());
    QTRY_VERIFY(gutter.isNull());

    const QString longLine = QString(1023, u'x') + u'\n';
    widget->document()->setPlainText(longLine.repeated(BlameGutter::MaxMappedCharacters / 1024 + 1));
    toggleBlameGutter(widget, repo);
    gutter = widget->findChild<BlameGutter *>();
    QVERIFY(gutter);
    gutter->refresh();
    QVERIFY(!gutter->m_runner.isRunning());
    QCOMPARE(gutter->m_data.items.first().text, Tr::tr("File too large"));

    widget->document()->setPlainText("first\nsecond\nthird\n");
    QTRY_VERIFY(!gutter->m_data.commits.isEmpty());
    QCOMPARE(gutter->m_data.commits.at(gutter->m_data.lineItems.at(0)).hash, initialHash);

    gutter->refresh();
    QVERIFY(EditorManager::closeEditors({editor}, false));
    QTRY_VERIFY(gutter.isNull());
}

void InstantBlameTest::testBlameDocumentContents()
{
    const EditorLineDiff insertion
        = editorLineDiffAgainstEditorText("first\n", "inserted\nfirst\n", 1);
    QVERIFY(insertion.isValid);
    QVERIFY(insertion.oldLines.isEmpty());
    QVERIFY(insertion.oldLineNumbers.isEmpty());
    QCOMPARE(insertion.newLine, QString("inserted"));

    const EditorLineDiff replacement = editorLineDiffAgainstEditorText(
        "first\nsecond\nthird\n", "inserted\nfirst\nchanged\nthird\n", 3);
    QVERIFY(replacement.isValid);
    QCOMPARE(replacement.oldLines, QStringList{"second"});
    QCOMPARE(replacement.oldLineNumbers, QList<int>{2});

    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());
    const FilePath repo = FilePath::fromString(temporaryDir.path());
    const auto runGit = [repo](const QStringList &arguments) {
        return gitClient().vcsSynchronousExec(repo, arguments).result()
               == ProcessResult::FinishedWithSuccess;
    };
    QVERIFY(runGit({"init", "."}));
    QVERIFY(runGit({"config", "user.email", "test@test"}));
    QVERIFY(runGit({"config", "user.name", "test"}));
    QVERIFY(runGit({"config", "commit.gpgsign", "false"}));
    const FilePath file = repo / "file.txt";
    QVERIFY(file.writeFileContents("first\nold\n"));
    QVERIFY(runGit({"add", "file.txt"}));
    QVERIFY(runGit({"commit", "-m", "Parent"}));
    QVERIFY(file.writeFileContents("first\ncommitted\n"));
    QVERIFY(runGit({"add", "file.txt"}));
    QVERIFY(runGit({"commit", "-m", "initial"}));
    const CommandResult headResult = gitClient().vcsSynchronousExec(repo, {"rev-parse", "HEAD"});
    QCOMPARE(headResult.result(), ProcessResult::FinishedWithSuccess);
    const QString committedHash = headResult.cleanedStdOut().trimmed();
    QVERIFY(file.writeFileContents("prefix\nfirst\ncommitted\n"));
    QVERIFY(runGit({"commit", "-am", "Insert line above"}));
    QVERIFY(file.writeFileContents("prefix\nfirst\nworking tree\n"));

    const auto result = std::make_shared<std::optional<CommandResult>>();
    gitClient().enqueueCommand(
        {repo,
         blameCommandArguments("file.txt", {}, 1, false, false, true),
         RunFlag::NoOutput,
         {},
         {},
         [result](const CommandResult &commandResult) { *result = commandResult; },
         QByteArray("displayed snapshot\n")});
    QTRY_VERIFY(result->has_value());
    QCOMPARE((*result)->result(), ProcessResult::FinishedWithSuccess);
    QVERIFY((*result)->cleanedStdOut().contains("\tdisplayed snapshot"));

    auto document = QSharedPointer<TextDocument>::create();
    document->setFilePath(file);
    QVERIFY(document->setPlainText("prefix\nfirst\nworking tree\n"));
    document->document()->setModified(false);
    TextEditorWidget widget;
    widget.setTextDocument(document);
    QTextCursor cursor(widget.document());
    cursor.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, 2);
    widget.setTextCursor(cursor);

    document->setProperty("GitRepository", repo.path());
    InstantBlame instantBlame;
    QVERIFY(instantBlame.setEditor(&widget));
    instantBlame.scheduleInstantBlame();

    const auto markToolTip = [&document](int line) {
        for (const TextMark *mark : document->marks()) {
            if (mark->lineNumber() == line) {
                if (const auto blameMark = dynamic_cast<const BlameMark *>(mark))
                    return blameMark->toolTip();
            }
        }
        return QString();
    };

    QTRY_VERIFY(markToolTip(3).contains("-committed"));
    QVERIFY(markToolTip(3).contains("+working tree"));
    QTRY_VERIFY(markToolTip(3).contains("Last committed version"));
    QTRY_VERIFY(markToolTip(3).contains("initial"));
    QTRY_VERIFY(markToolTip(3).contains("test@test"));
    QTRY_VERIFY(markToolTip(3).contains("href=\"previousShow\""));
    QTRY_VERIFY(markToolTip(3).contains(
        "<b>Actions for last committed version</b></p><p><a href=\"previousBlame\""));
    QVERIFY(markToolTip(3).contains("href=\"previousBlame\""));
    QVERIFY(markToolTip(3).contains("href=\"previousBlameParent\""));
    QVERIFY(markToolTip(3).contains("href=\"previousShowFile\""));
    QVERIFY(markToolTip(3).contains("href=\"previousLogLine\""));
    QVERIFY(markToolTip(3).contains("href=\"previousDiffParent\""));
    QVERIFY(markToolTip(3).contains("href=\"previousCopyHash\""));

    const TextMark *modifiedMark = nullptr;
    for (const TextMark *mark : document->marks()) {
        if (mark->lineNumber() == 3)
            modifiedMark = mark;
    }
    QVERIFY(modifiedMark);
    QWidget toolTipWidget;
    QVBoxLayout toolTipLayout(&toolTipWidget);
    QVERIFY(modifiedMark->addToolTipContent(&toolTipLayout));
    QLabel *toolTipLabel = toolTipWidget.findChild<QLabel *>();
    QVERIFY(toolTipLabel);
    QVERIFY(QMetaObject::invokeMethod(toolTipLabel, "linkActivated", Qt::DirectConnection,
                                      Q_ARG(QString, QString("previousCopyHash"))));
    QCOMPARE(QApplication::clipboard()->text(), committedHash);
    QVERIFY(QMetaObject::invokeMethod(toolTipLabel, "linkActivated", Qt::DirectConnection,
                                      Q_ARG(QString, QString("previousLogLine"))));
    QPointer<Core::IEditor> logEditor;
    QTRY_VERIFY(([&] {
        Core::IEditor *editor = Core::EditorManager::currentEditor();
        if (!editor || !editor->document()->displayName().contains("Log"))
            return false;
        auto *logWidget = VcsBaseEditor::getVcsBaseEditor(editor);
        if (!logWidget || !logWidget->toPlainText().contains("initial"))
            return false;
        logEditor = editor;
        return true;
    })());
    QVERIFY(Core::EditorManager::closeDocuments({logEditor->document()}, false));

    QVERIFY(file.writeFileContents("prefix\nfirst\ncommitted\n"));
    QVERIFY(document->setPlainText("prefix\nfirst\ncommitted\n"));
    document->document()->setModified(false);
    cursor = QTextCursor(widget.document()->findBlockByNumber(2));
    widget.setTextCursor(cursor);
    instantBlame.scheduleInstantBlame();
    QTRY_VERIFY(markToolTip(3).contains(committedHash)
                && !markToolTip(3).contains("Local changes"));
    const BlameMark *committedMark = nullptr;
    for (const TextMark *mark : document->marks()) {
        if (mark->lineNumber() == 3)
            committedMark = dynamic_cast<const BlameMark *>(mark);
    }
    QVERIFY(committedMark);
    QWidget committedTooltip;
    QVBoxLayout committedLayout(&committedTooltip);
    QVERIFY(committedMark->addToolTipContent(&committedLayout));
    QLabel *committedLabel = committedTooltip.findChild<QLabel *>();
    QVERIFY(committedLabel);
    QVERIFY(QMetaObject::invokeMethod(committedLabel, "linkActivated", Qt::DirectConnection,
                                      Q_ARG(QString, QString("diffParent"))));
    Core::IEditor *snapshotEditor = nullptr;
    QTRY_VERIFY((snapshotEditor = Core::EditorManager::currentEditor())
                && DiffEditor::inlineDiffEditorWidget(snapshotEditor));
    const auto *snapshot = DiffEditor::inlineDiffEditorWidget(snapshotEditor);
    QCOMPARE(snapshot->textCursor().blockNumber() + 1, 2);
    QVERIFY(Core::EditorManager::closeDocuments({snapshotEditor->document()}, false));

    QVERIFY(document->setPlainText("inserted\nprefix\nfirst\ncommitted\n"));
    document->document()->setModified(true);
    cursor = QTextCursor(widget.document());
    widget.setTextCursor(cursor);
    instantBlame.scheduleInstantBlame();
    QTRY_VERIFY(markToolTip(1).contains("+inserted"));
    QTRY_VERIFY(markToolTip(1).contains("No previous committed line"));

    const FilePath addedFile = repo / "added.txt";
    QVERIFY(addedFile.writeFileContents("staged addition\n"));
    QVERIFY(runGit({"add", "added.txt"}));
    auto addedDocument = QSharedPointer<TextDocument>::create();
    addedDocument->setFilePath(addedFile);
    QVERIFY(addedDocument->setPlainText("staged addition\n"));
    addedDocument->document()->setModified(false);
    TextEditorWidget addedWidget;
    addedWidget.setTextDocument(addedDocument);
    BlameController addedController;
    addedController.setContext(&addedWidget, repo, {}, "added.txt", addedFile);
    addedController.setEnabled(true);
    QTRY_VERIFY(([&] {
        for (const TextMark *mark : addedDocument->marks()) {
            if (mark->lineNumber() == 1
                && mark->toolTip().contains("No previous committed line")
                && mark->toolTip().contains("+staged addition")) {
                return true;
            }
        }
        return false;
    })());

    const QByteArray latin1Contents = "caf\xe9\ncaf\xe9 old\n";
    QVERIFY(file.writeFileContents(latin1Contents));
    QVERIFY(runGit({"add", "file.txt"}));
    QVERIFY(runGit({"commit", "-m", "latin1"}));

    auto latinDocument = QSharedPointer<TextDocument>::create();
    latinDocument->setFilePath(file);
    latinDocument->setEncoding(TextEncoding::Latin1);
    QVERIFY(latinDocument->setPlainText(QString::fromUtf8("café\ncafé changed\n")));
    latinDocument->document()->setModified(true);
    TextEditorWidget latinWidget;
    latinWidget.setTextDocument(latinDocument);
    QTextCursor latinCursor(latinWidget.document());
    latinWidget.setTextCursor(latinCursor);

    BlameController latinController;
    latinController.setContext(&latinWidget, repo, {}, "file.txt", file);
    latinController.setEnabled(true);

    const auto latinMarkToolTip = [&latinDocument](int line) {
        for (const TextMark *mark : latinDocument->marks()) {
            if (mark->lineNumber() == line) {
                if (const auto blameMark = dynamic_cast<const BlameMark *>(mark))
                    return blameMark->toolTip();
            }
        }
        return QString();
    };

    QTRY_VERIFY(latinMarkToolTip(1).contains("latin1"));

    latinCursor = QTextCursor(latinWidget.document());
    latinCursor.movePosition(QTextCursor::NextBlock);
    latinWidget.setTextCursor(latinCursor);
    latinController.schedule();
    QTRY_VERIFY(latinMarkToolTip(2).contains("-café old"));
    QVERIFY(latinMarkToolTip(2).contains("+café changed"));
    QTRY_VERIFY(latinMarkToolTip(2).contains("latin1"));
    QVERIFY(latinMarkToolTip(2).contains("Last committed version"));
}

void registerInstantBlameTests(ExtensionSystem::IPlugin *plugin)
{
    plugin->addTest<InstantBlameTest>();
}

#endif

} // Git::Internal

#include "instantblame.moc"
