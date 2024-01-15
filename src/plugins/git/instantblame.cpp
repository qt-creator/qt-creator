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
#include <QTextCodec>
#include <QLabel>
#include <QLayout>
#include <QTimer>

namespace Git::Internal {

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

bool BlameMark::addToolTipContent(QLayout *target) const
{
    auto textLabel = new QLabel;
    textLabel->setText(toolTip());
    target->addWidget(textLabel);
    QObject::connect(textLabel, &QLabel::linkActivated, textLabel, [this] {
        gitClient().show(m_info.filePath, m_info.sha1);
    });

    return true;
}

QString BlameMark::toolTipText(const CommitInfo &info) const
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

InstantBlame::InstantBlame()
{
    m_codec = gitClient().defaultCommitEncoding();
}

void InstantBlame::setup()
{
    m_cursorPositionChangedTimer = new QTimer(this);
    m_cursorPositionChangedTimer->setSingleShot(true);
    connect(m_cursorPositionChangedTimer, &QTimer::timeout, this, &InstantBlame::perform);

    auto setupBlameForEditor = [this](Core::IEditor *editor) {
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
        if (!widget)
            return;

        if (qobject_cast<const VcsBaseEditorWidget *>(widget))
            return; // Skip in VCS editors like log or blame

        const FilePath workingDirectory = currentState().currentFileTopLevel();
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
                force();
        });

        force();
    };

    connect(&settings().instantBlame, &BaseAspect::changed, this, [this, setupBlameForEditor] {
        if (settings().instantBlame())
            setupBlameForEditor(EditorManager::currentEditor());
        else
            stop();
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

static CommitInfo parseBlameOutput(const QStringList &blame, const Utils::FilePath &filePath,
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

void InstantBlame::once()
{
    if (!settings().instantBlame()) {
        const TextEditorWidget *widget = TextEditorWidget::currentTextEditorWidget();
        if (!widget)
            return;
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
    m_lastVisitedEditorLine = -1;
    perform();
}

void InstantBlame::perform()
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
        m_lastVisitedEditorLine = -1;
        m_blameMark.reset();
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
            stop();
            return;
        }
        const QString output = result.cleanedStdOut();
        if (output.isEmpty()) {
            stop();
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

void InstantBlame::stop()
{
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
                m_author = author;
                force();
            }
        }
    };
    gitClient().readConfigAsync(workingDirectory, {"var", "GIT_AUTHOR_IDENT"},
                                authorHandler);

    return true;
}

} // Git::Internal
