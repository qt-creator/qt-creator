// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcsoutputwindow.h"

#include "vcsbasetr.h"
#include "vcsoutputformatter.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/outputwindow.h>

#include <texteditor/behaviorsettings.h>
#include <texteditor/fontsettings.h>

#include <utils/filepath.h>
#include <utils/qtcprocess.h>
#include <utils/theme/theme.h>

#include <QAction>
#include <QContextMenuEvent>
#include <QEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QPoint>
#include <QPointer>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextBlockUserData>
#include <QTextCharFormat>
#include <QTextStream>
#include <QTime>

using namespace Core;
using namespace Utils;

/*!
    \class VcsBase::VcsBaseOutputWindow

    \brief The VcsBaseOutputWindow class is an output window for Version Control
    System commands and other output (Singleton).

    Installed by the base plugin and accessible for the other plugins
    via static instance()-accessor. Provides slots to append output with
    special formatting.

    It is possible to associate a repository with plain log text, enabling
    an "Open" context menu action over relative file name tokens in the text
    (absolute paths will also work). This can be used for "status" logs,
    showing modified file names, allowing the user to open them.
*/

namespace VcsBase {
namespace Internal {

const char C_VCS_OUTPUT_PANE[] = "Vcs.OutputPane";

const char zoomSettingsKey[] = "Vcs/OutputPane/Zoom";

// Store repository along with text blocks
class RepositoryUserData : public QTextBlockUserData
{
public:
    explicit RepositoryUserData(const FilePath &repository) : m_repository(repository) {}
    const FilePath &repository() const { return m_repository; }

private:
    const FilePath m_repository;
};

// A plain text edit with a special context menu containing "Clear"
// and functions to append specially formatted entries.
class OutputWindowPlainTextEdit : public OutputWindow
{
public:
    explicit OutputWindowPlainTextEdit(QWidget *parent = nullptr);

    void appendLines(const QString &text, VcsOutputWindow::MessageStyle style,
                     const FilePath &repository);

protected:
    void adaptContextMenu(QMenu *menu, const QPoint &pos) override;
    void handleLink(const QPoint &pos) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QString identifierUnderCursor(const QPoint &pos, FilePath *repository = nullptr,
                                  QTextCursor *tokenCursor = nullptr) const;
    void updateFileLink(const QPoint &pos);
    void clearFileLink();
    void clearFileLinkSelection();

    VcsOutputLineParser *m_parser = nullptr;
    QTextCursor m_fileLinkCursor;
    QTextCursor m_fileLinkCandidateCursor;
    bool m_fileLinkCandidateIsFile = false;
};

OutputWindowPlainTextEdit::OutputWindowPlainTextEdit(QWidget *parent)
    : OutputWindow(Context(C_VCS_OUTPUT_PANE), zoomSettingsKey, parent)
    , m_parser(new VcsOutputLineParser)
{
    setReadOnly(true);
    setUndoRedoEnabled(false);
    setFrameStyle(QFrame::NoFrame);
    outputFormatter()->setBoldFontEnabled(false);
    setLineParsers({m_parser});
}

// Search back for beginning of word
static inline int firstWordCharacter(const QString &s, int startPos)
{
    for ( ; startPos >= 0 ; startPos--) {
        if (s.at(startPos).isSpace())
            return startPos + 1;
    }
    return 0;
}

QString OutputWindowPlainTextEdit::identifierUnderCursor(const QPoint &widgetPos,
                                                         FilePath *repository,
                                                         QTextCursor *tokenCursor) const
{
    if (repository)
        repository->clear();
    if (tokenCursor)
        *tokenCursor = {};
    // Get the blank-delimited word under cursor. Note that
    // using "SelectWordUnderCursor" does not work since it breaks
    // at delimiters like '/'. Get the whole line
    QTextCursor cursor = cursorForPosition(widgetPos);
    const int cursorDocumentPos = cursor.position();
    cursor.select(QTextCursor::BlockUnderCursor);
    if (!cursor.hasSelection())
        return {};
    const int blockPosition = cursor.selectionStart();
    const QString block = cursor.selectedText();
    // Determine cursor position within line and find blank-delimited word
    const int cursorPos = cursorDocumentPos - blockPosition;
    const int blockSize = block.size();
    if (cursorPos < 0 || cursorPos >= blockSize || block.at(cursorPos).isSpace())
        return {};
    // Retrieve repository if desired
    if (repository)
        if (QTextBlockUserData *data = cursor.block().userData())
            *repository = static_cast<const RepositoryUserData *>(data)->repository();
    // Find first non-space character of word and find first non-space character past
    const int startPos = firstWordCharacter(block, cursorPos);
    int endPos = cursorPos;
    if (block.at(startPos) == QLatin1Char('"')) {
        endPos = startPos + 1;
        for (; endPos < blockSize; ++endPos) {
            int backslashCount = 0;
            for (int pos = endPos - 1; pos >= startPos && block.at(pos) == QLatin1Char('\\'); --pos)
                ++backslashCount;
            if (block.at(endPos) == QLatin1Char('"') && !(backslashCount % 2)) {
                ++endPos;
                break;
            }
        }
    } else {
        for (; endPos < blockSize && !block.at(endPos).isSpace(); endPos++) {}
    }
    if (tokenCursor && endPos > startPos) {
        *tokenCursor = QTextCursor(document());
        tokenCursor->setPosition(blockPosition + startPos);
        tokenCursor->setPosition(blockPosition + endPos, QTextCursor::KeepAnchor);
    }
    QString token = endPos > startPos ? block.mid(startPos, endPos - startPos) : QString();
    return VcsOutputLineParser::unquoteGitPath(token);
}

void OutputWindowPlainTextEdit::adaptContextMenu(QMenu *menu, const QPoint &pos)
{
    const QString href = anchorAt(pos);
    if (!href.isEmpty())
        menu->clear();

    // Add 'open file'
    FilePath repo;
    const QString token = identifierUnderCursor(pos, &repo);
    if (!repo.isEmpty() && !href.isEmpty())
        m_parser->fillLinkContextMenu(menu, repo, href);
    QAction *openAction = nullptr;
    if (!token.isEmpty()) {
        const FilePath file = m_parser->filePathForLink(repo, token);
        if (VcsOutputLineParser::shouldOfferFileLink(token) && file.isFile()) {
            menu->addSeparator();
            openAction = menu->addAction(Tr::tr("Open \"%1\"").arg(file.nativePath()));
            connect(openAction, &QAction::triggered, this, [fp = file.absoluteFilePath()] {
                EditorManager::openEditor(fp);
            });
        }
    }
}

void OutputWindowPlainTextEdit::handleLink(const QPoint &pos)
{
    const QString href = anchorAt(pos);
    FilePath repository;
    const QString token = identifierUnderCursor(pos, &repository);
    if (repository.isEmpty()) {
        if (!href.isEmpty())
            OutputWindow::handleLink(pos);
        return;
    }
    if (href.isEmpty()) {
        m_parser->handleFileLink(repository, token);
        return;
    }
    if (outputFormatter()->handleFileLink(href))
        return;
    m_parser->handleVcsLink(repository, href);
}

void OutputWindowPlainTextEdit::updateFileLink(const QPoint &pos)
{
    const bool onAnchor = !anchorAt(pos).isEmpty();
    if (!viewport()->rect().contains(pos) || onAnchor) {
        clearFileLink();
        m_fileLinkCandidateCursor = {};
        viewport()->setCursor(onAnchor ? Qt::PointingHandCursor : Qt::IBeamCursor);
        return;
    }

    FilePath repository;
    QTextCursor tokenCursor;
    const QString token = identifierUnderCursor(pos, &repository, &tokenCursor);
    if (token.isEmpty() || repository.isEmpty()
        || !VcsOutputLineParser::shouldOfferFileLink(token)) {
        clearFileLink();
        m_fileLinkCandidateCursor = {};
        viewport()->setCursor(Qt::IBeamCursor);
        return;
    }

    if (m_fileLinkCandidateCursor == tokenCursor) {
        if (m_fileLinkCandidateIsFile)
            viewport()->setCursor(Qt::PointingHandCursor);
        else
            viewport()->setCursor(Qt::IBeamCursor);
        return;
    }

    if (m_fileLinkCursor == tokenCursor) {
        viewport()->setCursor(Qt::PointingHandCursor);
        return;
    }

    m_fileLinkCandidateCursor = tokenCursor;
    m_fileLinkCandidateIsFile = m_parser->filePathForLink(repository, token).isFile();
    if (!m_fileLinkCandidateIsFile) {
        clearFileLinkSelection();
        viewport()->setCursor(Qt::IBeamCursor);
        return;
    }

    QTextEdit::ExtraSelection selection;
    selection.cursor = tokenCursor;
    selection.format.setForeground(creatorColor(Theme::TextColorLink));
    selection.format.setFontUnderline(true);
    setExtraSelections({selection});
    m_fileLinkCursor = tokenCursor;
    viewport()->setCursor(Qt::PointingHandCursor);
}

void OutputWindowPlainTextEdit::clearFileLink()
{
    clearFileLinkSelection();
    m_fileLinkCandidateCursor = {};
}

void OutputWindowPlainTextEdit::clearFileLinkSelection()
{
    if (m_fileLinkCursor.isNull())
        return;
    setExtraSelections({});
    m_fileLinkCursor = {};
}

void OutputWindowPlainTextEdit::mouseMoveEvent(QMouseEvent *event)
{
    QPlainTextEdit::mouseMoveEvent(event);
    updateFileLink(event->pos());
}

void OutputWindowPlainTextEdit::leaveEvent(QEvent *event)
{
    clearFileLink();
    QPlainTextEdit::leaveEvent(event);
}

static OutputFormat styleToFormat(VcsOutputWindow::MessageStyle style)
{
    switch (style) {
    case VcsOutputWindow::Warning:
        return LogMessageFormat;
    case VcsOutputWindow::Error:
        return StdErrFormat;
    case VcsOutputWindow::Message:
        return StdOutFormat;
    case VcsOutputWindow::Command:
        return NormalMessageFormat;
    case VcsOutputWindow::None:
        return OutputFormat::StdOutFormat;
    }
    return OutputFormat::StdOutFormat;
}

void OutputWindowPlainTextEdit::appendLines(const QString &text,
                                            VcsOutputWindow::MessageStyle style,
                                            const FilePath &repository)
{
    if (text.isEmpty())
        return;

    const QString textToAdd = style == VcsOutputWindow::Command
                            ? QTime::currentTime().toString("\nHH:mm:ss ") + text : text;
    const int previousLineCount = document()->lineCount();

    outputFormatter()->setBoldFontEnabled(style == VcsOutputWindow::Command);
    outputFormatter()->appendMessage(textToAdd, styleToFormat(style));

    // Scroll down
    moveCursor(QTextCursor::End);
    ensureCursorVisible();
    if (!repository.isEmpty()) {
        // Associate repository with new data.
        QTextBlock block = document()->findBlockByLineNumber(previousLineCount);
        for ( ; block.isValid(); block = block.next())
            block.setUserData(new RepositoryUserData(repository));
    }
}

} // namespace Internal

// ------------------- VcsBaseOutputWindowPrivate
class VcsOutputWindowPrivate
{
public:
    Internal::OutputWindowPlainTextEdit widget;
    const QRegularExpression passwordRegExp = QRegularExpression("://([^@:]+):([^@]+)@");
};

static VcsOutputWindow *m_instance = nullptr;
static VcsOutputWindowPrivate *d = nullptr;

VcsOutputWindow::VcsOutputWindow()
{
    setId("VersionControl");
    setDisplayName(Tr::tr("Version Control"));
    setPriorityInStatusBar(-20);

    d = new VcsOutputWindowPrivate;
    Q_ASSERT(d->passwordRegExp.isValid());
    m_instance = this;

    auto updateBehaviorSettings = [] {
        d->widget.setWheelZoomEnabled(TextEditor::globalBehaviorSettings().scrollWheelZooming());
    };

    auto updateFontSettings = [] {
        d->widget.setBaseFont(TextEditor::globalFontSettings().data().font());
    };

    updateBehaviorSettings();
    updateFontSettings();
    setupContext(Internal::C_VCS_OUTPUT_PANE, &d->widget);

    connect(this, &IOutputPane::zoomInRequested, &d->widget, &OutputWindow::zoomIn);
    connect(this, &IOutputPane::zoomOutRequested, &d->widget, &OutputWindow::zoomOut);
    connect(this, &IOutputPane::resetZoomRequested, &d->widget, &OutputWindow::resetZoom);
    connect(&TextEditor::globalBehaviorSettings(), &Utils::AspectContainer::changed,
            this, updateBehaviorSettings);
    connect(&TextEditor::globalFontSettings(), &TextEditor::FontSettings::changed,
            this, updateFontSettings);
}

static QString filterPasswordFromUrls(QString input)
{
    return input.replace(d->passwordRegExp, "://\\1:***@");
}

VcsOutputWindow::~VcsOutputWindow()
{
    m_instance = nullptr;
    delete d;
}

QWidget *VcsOutputWindow::outputWidget(QWidget *parent)
{
    if (parent != d->widget.parent())
        d->widget.setParent(parent);
    return &d->widget;
}

const QList<Core::OutputWindow *> VcsOutputWindow::outputWindows() const
{
    return {&d->widget};
}

void VcsOutputWindow::clearContents()
{
    d->widget.clear();
}

void VcsOutputWindow::setFocus()
{
    d->widget.setFocus();
}

bool VcsOutputWindow::hasFocus() const
{
    return d->widget.hasFocus();
}

bool VcsOutputWindow::canFocus() const
{
    return true;
}

bool VcsOutputWindow::canNavigate() const
{
    return false;
}

bool VcsOutputWindow::canNext() const
{
    return false;
}

bool VcsOutputWindow::canPrevious() const
{
    return false;
}

void VcsOutputWindow::goToNext()
{
}

void VcsOutputWindow::goToPrev()
{
}

void VcsOutputWindow::setText(const QString &text)
{
    d->widget.setPlainText(text);
}

void VcsOutputWindow::setData(const QByteArray &data)
{
    setText(TextEncoding::encodingForLocale().decode(data));
}

void VcsOutputWindow::append(const Utils::FilePath &workingDirectory, const QString &text,
                             MessageStyle style, bool silently)
{
    const QString lines = (text.endsWith('\n') || text.endsWith('\r')) ? text : text + '\n';
    d->widget.appendLines(lines, style, workingDirectory);

    if (!silently && !d->widget.isVisible())
        m_instance->popup(IOutputPane::NoModeSwitch);
}

void VcsOutputWindow::appendSilently(const FilePath &workingDirectory, const QString &text)
{
    append(workingDirectory, text, None, true);
}

void VcsOutputWindow::appendText(const Utils::FilePath &workingDirectory, const QString &text)
{
    append(workingDirectory, text, None, false);
}

void VcsOutputWindow::appendMessage(const FilePath &workingDirectory, const QString &text)
{
    append(workingDirectory, text, Message, true);
}

void VcsOutputWindow::appendWarning(const FilePath &workingDirectory, const QString &text)
{
    append(workingDirectory, text, Warning, false);
}

void VcsOutputWindow::appendError(const FilePath &workingDirectory, const QString &text)
{
    append(workingDirectory, text, Error, false);
}

// Helper to format arguments for log windows hiding common password options.
static inline QString formatArguments(const QStringList &args)
{
    const char passwordOptionC[] = "--password";
    QString rc;
    QTextStream str(&rc);
    const int size = args.size();
    // Skip authentication options
    for (int i = 0; i < size; i++) {
        const QString arg = filterPasswordFromUrls(args.at(i));
        if (i)
            str << ' ';
        if (arg.startsWith(QString::fromLatin1(passwordOptionC) + '=')) {
            str << ProcessArgs::quoteArg("--password=********");
            continue;
        }
        str << ProcessArgs::quoteArg(arg);
        if (arg == passwordOptionC) {
            str << ' ' << ProcessArgs::quoteArg("********");
            i++;
        }
    }
    return rc;
}

QString VcsOutputWindow::msgExecutionLogEntry(const FilePath &workingDir, const CommandLine &command)
{
    const QString maskedCmdline = ProcessArgs::quoteArg(command.executable().toUserOutput())
            + ' ' + formatArguments(command.splitArguments());
    if (workingDir.isEmpty())
        return Tr::tr("Running: %1").arg(maskedCmdline) + '\n';
    return Tr::tr("Running in \"%1\": %2").arg(workingDir.toUserOutput(), maskedCmdline) + '\n';
}

void VcsOutputWindow::appendShellCommandLine(const FilePath &workingDirectory, const QString &text)
{
    append(workingDirectory, filterPasswordFromUrls(text), Command, true);
}

void VcsOutputWindow::appendCommand(const FilePath &workingDirectory, const CommandLine &command)
{
    appendShellCommandLine(workingDirectory, msgExecutionLogEntry(workingDirectory, command));
}

void VcsOutputWindow::destroy()
{
    delete m_instance;
    m_instance = nullptr;
}

VcsOutputWindow *VcsOutputWindow::instance()
{
    if (!m_instance)
        (void) new VcsOutputWindow;
    return m_instance;
}

} // namespace VcsBase
