// Copyright (C) 2023 Andre Hartmann (aha_1980@gmx.de)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "instantblame.h"

#include "gitclient.h"
#include "gitconstants.h"
#include "gitplugin.h"
#include "gitsettings.h"
#include "gittr.h"

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditortr.h>
#include <texteditor/textmark.h>

#include <utils/filepath.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>

#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcscommand.h>

#include <QAction>
#include <QDateTime>
#include <QLabel>
#include <QLayout>
#include <QLoggingCategory>
#include <QTextCodec>
#include <QTimer>

namespace Git::Internal {

static Q_LOGGING_CATEGORY(log, "qtc.vcs.git.instantblame", QtWarningMsg);

using namespace Core;
using namespace TextEditor;
using namespace Utils;
using namespace VcsBase;

BlameMark::BlameMark(const FilePath &fileName, int lineNumber, const CommitInfo &info)
    : TextEditor::TextMark(fileName,
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
        copyToClipboardAction->setIcon(QIcon::fromTheme("edit-copy", Utils::Icons::COPY.icon()));
        copyToClipboardAction->setToolTip(TextEditor::Tr::tr("Copy SHA1 to Clipboard"));
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
    QObject::connect(textLabel, &QLabel::linkActivated, textLabel, [this](const QString &link) {
        qCInfo(log) << "Link activated with target:" << link;
        const QString hash = (link == "blameParent") ? m_info.hash + "^" : m_info.hash;

        if (link.startsWith("blame") || link == "showFile") {
            const VcsBasePluginState state = currentState();
            QTC_ASSERT(state.hasTopLevel(), return);
            const Utils::FilePath path = state.topLevel();

            const QString originalFileName = m_info.originalFileName;
            if (link.startsWith("blame")) {
                qCInfo(log).nospace().noquote() << "Blaming: \"" << path << "/" << originalFileName
                                                << "\":" << m_info.originalLine << " @ " << hash;
                gitClient().annotate(path, originalFileName, m_info.originalLine, hash);
            } else {
                qCInfo(log).nospace().noquote() << "Showing file: \"" << path << "/"
                                                << originalFileName << "\" @ " << hash;

                const auto fileName = Utils::FilePath::fromString(originalFileName);
                gitClient().openShowEditor(path, hash, fileName);
            }
        } else if (link == "logLine") {
            const VcsBasePluginState state = currentState();
            QTC_ASSERT(state.hasFile(), return);

            qCInfo(log).nospace().noquote() << "Showing log for: \"" << m_info.filePath
                                            << "\" line:" << m_info.line;

            const QString lineArg = QString("-L %1,%1:%2")
                                        .arg(m_info.line).arg(state.relativeCurrentFile());
            gitClient().log(state.currentFileTopLevel(), {}, true, {lineArg, "--no-patch"});
        } else {
            qCInfo(log).nospace().noquote() << "Showing commit: " << hash << " for " << m_info.filePath;
            gitClient().show(m_info.filePath, hash);
        }
    });

    return true;
}

QString BlameMark::toolTipText(const CommitInfo &info) const
{
    const ColorNames colors = GitClient::colorNames();

    QString result = QString(
                         "<table cellspacing=\"10\"><tr>"
                         "  <td><a href=\"blame\">Blame %2</a></td>"
                         "  <td><a href=\"blameParent\">Blame Parent</a></td>"
                         "  <td><a href=\"showFile\">File at %2</a></td>"
                         "  <td><a href=\"logLine\">Log for line %4</a></td>"
                         "</tr></table>"
                         "<p></p>"
                         "<table>"
                         "  <tr><td>commit</td><td><a style=\"color: %1;\" href=\"show\">%3</a></td></tr>"
                         "  <tr><td>Author:</td><td style=\"color: %5;\">%6 &lt;%7&gt;</td></tr>"
                         "  <tr><td>Date:</td><td style=\"color: %8;\">%9</td></tr>"
                         "</table>"
                         "<p style=\"color: %10;\">%11</p>")
                         .arg(colors.hash, info.hash.left(8), info.hash, QString::number(info.line),
                              colors.author, info.author, info.authorMail,
                              colors.date, info.authorDate.toString("yyyy-MM-dd hh:mm:ss"),
                              colors.subject, info.subject);

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
    m_codec = gitClient().defaultCommitEncoding();
    m_cursorPositionChangedTimer = new QTimer(this);
    m_cursorPositionChangedTimer->setSingleShot(true);
    connect(m_cursorPositionChangedTimer, &QTimer::timeout, this, &InstantBlame::perform);
}

void InstantBlame::setup()
{
    qCDebug(log) << "Setup";

    auto setupBlameForEditor = [this] {
        Core::IEditor *editor = EditorManager::currentEditor();
        if (!editor) {
            stop();
            return;
        }

        if (!settings().instantBlame()) {
            m_lastVisitedEditorLine = -1;
            stop();
            return;
        }

        const TextEditorWidget *widget = TextEditorWidget::fromEditor(editor);
        if (!widget) {
            qCInfo(log) << "Cannot get widget for editor" << editor;
            return;
        }

        if (qobject_cast<const VcsBaseEditorWidget *>(widget)) {
            qCDebug(log) << "Deactivating in VCS editors";
            return; // Skip in VCS editors like log or blame
        }

        const FilePath workingDirectory = currentState().currentFileTopLevel();
        if (!refreshWorkingDirectory(workingDirectory))
            return;

        qCInfo(log) << "Adding blame cursor connection";
        m_blameCursorPosConn = connect(widget, &QPlainTextEdit::cursorPositionChanged, this,
                                       [this] {
                                           if (!settings().instantBlame()) {
                                               disconnect(m_blameCursorPosConn);
                                               return;
                                           }
                                           m_cursorPositionChangedTimer->start(500);
                                       });
        m_document = editor->document();
        m_documentChangedConn = connect(m_document, &IDocument::changed,
                                        this, &InstantBlame::slotDocumentChanged,
                                        Qt::UniqueConnection);

        force();
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

// Porcelain format of git blame output:
// Consists of 12 or 13 lines (line 11 can be missing, "boundary", or "previous")
// The first line contains SHA-1, original line, current line,
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

static CommitInfo parseBlameOutput(const QStringList &blame, const Utils::FilePath &filePath,
                                   int line, const Git::Internal::Author &author)
{
    CommitInfo result;
    if (blame.size() <= 12)
        return result;

    const QStringList firstLineParts = blame.at(0).split(" ");
    result.hash = firstLineParts.first();
    result.author = blame.at(1).mid(7);
    result.authorMail = blame.at(2).mid(13).chopped(1);
    if (result.author == author.name || result.authorMail == author.email)
        result.shortAuthor = Tr::tr("You");
    else
        result.shortAuthor = result.author;
    const uint timeStamp = blame.at(3).mid(12).toUInt();
    result.authorDate = QDateTime::fromSecsSinceEpoch(timeStamp);
    result.subject = blame.at(9).mid(8);
    result.filePath = filePath;
    // blame.at(10) can be "boundary", "previous" or "filename"
    if (blame.at(10).startsWith("filename"))
        result.originalFileName = blame.at(10).mid(9);
    else
        result.originalFileName = blame.at(11).mid(9);
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

        connect(widget, &QPlainTextEdit::cursorPositionChanged,
            this, [this] { m_blameMark.reset(); }, Qt::SingleShotConnection);

        const FilePath workingDirectory = currentState().topLevel();
        if (!refreshWorkingDirectory(workingDirectory))
            return;
    }

    force();
}

void InstantBlame::force()
{
    qCDebug(log) << "Forcing blame now";
    m_lastVisitedEditorLine = -1;
    perform();
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
    const int lines = widget->document()->blockCount();

    if (line >= lines) {
        m_lastVisitedEditorLine = -1;
        m_blameMark.reset();
        return;
    }

    if (m_lastVisitedEditorLine == line)
        return;

    qCDebug(log) << "New editor line:" << line;
    m_lastVisitedEditorLine = line;

    const Utils::FilePath filePath = widget->textDocument()->filePath();
    const QFileInfo fi(filePath.toString());
    const Utils::FilePath workingDirectory = Utils::FilePath::fromString(fi.path());
    const QString lineString = QString("%1,%1").arg(line);
    const auto lineDiffHandler = [this](const CommandResult &result) {
        const QString error = result.cleanedStdErr();
        if (!error.isEmpty()) {
            qCWarning(log) << error;
        }
        if (!m_blameMark) {
            qCInfo(log) << "m_blameMark is invalid";
            return;
        }

        static const QRegularExpression re("^[-+][^-+].*");
        const QStringList lines = result.cleanedStdOut().split("\n").filter(re);
        for (const QString &line : lines) {
            if (line.startsWith("-")) {
                m_blameMark->addOldLine(line);
                qCDebug(log) << "Found removed line: " << line;
            } else if (line.startsWith("+")) {
                m_blameMark->addNewLine(line);
                qCDebug(log) << "Found added line: " << line;
            }
        }
    };
    const auto commandHandler = [this, filePath, line, lineDiffHandler]
        (const CommandResult &result) {
        if (result.result() == ProcessResult::FinishedWithError &&
            result.cleanedStdErr().contains("no such path")) {
            stop();
            return;
        }
        const QString output = result.cleanedStdOut();
        if (output.isEmpty()) {
            stop();
            return;
        }
        const CommitInfo info = parseBlameOutput(output.split('\n'), filePath, line, m_author);
        m_blameMark.reset(new BlameMark(filePath, line, info));

        // Get line diff: `git log -n 1 -p -L47,47:README.md a5c4c34c9ab4`
        const QString origLineString = QString("%1,%1").arg(info.originalLine);
        const QString fileLineRange = "-L" + origLineString + ":" + info.originalFileName;
        const QStringList lineDiffOptions = {"log", "-n 1", "-p", fileLineRange, info.hash};
        const FilePath topLevel = currentState().topLevel();

        qCDebug(log) << "Running git" << lineDiffOptions.join(' ');
        gitClient().vcsExecWithHandler(topLevel, lineDiffOptions, this,
                                       lineDiffHandler, RunFlags::NoOutput, m_codec);
    };
    QStringList options = {"blame", "-p"};
    if (settings().instantBlameIgnoreSpaceChanges())
        options.append("-w");
    if (settings().instantBlameIgnoreLineMoves())
        options.append("-M");
    options.append({"-L", lineString, "--", filePath.toString()});
    qCDebug(log) << "Running git" << options.join(' ');
    gitClient().vcsExecWithHandler(workingDirectory, options, this,
                                   commandHandler, RunFlags::NoOutput, m_codec);
}

void InstantBlame::stop()
{
    qCInfo(log) << "Stopping blame now";
    m_blameMark.reset();
    m_cursorPositionChangedTimer->stop();
    disconnect(m_blameCursorPosConn);
    disconnect(m_documentChangedConn);
}

bool InstantBlame::refreshWorkingDirectory(const FilePath &workingDirectory)
{
    if (workingDirectory.isEmpty())
        return false;

    if (m_workingDirectory == workingDirectory)
        return true;

    qCInfo(log) << "Setting new working directory:" << workingDirectory;
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
            qCInfo(log) << "Setting new text codec:" << codec->name();
            m_codec = codec;
            force();
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
                force();
            }
        }
    };
    gitClient().readConfigAsync(workingDirectory, {"var", "GIT_AUTHOR_IDENT"},
                                authorHandler);

    return true;
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
        force();
    m_modified = modified;
}

} // Git::Internal
