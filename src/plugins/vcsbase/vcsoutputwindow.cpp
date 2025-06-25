// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcsoutputwindow.h"

#include "vcsbasetr.h"
#include "vcsoutputformatter.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/outputwindow.h>

#include <texteditor/behaviorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <utils/filepath.h>
#include <utils/qtcprocess.h>
#include <utils/theme/theme.h>

#include <QAction>
#include <QContextMenuEvent>
#include <QMenu>
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

private:
    QString identifierUnderCursor(const QPoint &pos, FilePath *repository = nullptr) const;

    VcsOutputLineParser *m_parser = nullptr;
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

QString OutputWindowPlainTextEdit::identifierUnderCursor(const QPoint &widgetPos, FilePath *repository) const
{
    if (repository)
        repository->clear();
    // Get the blank-delimited word under cursor. Note that
    // using "SelectWordUnderCursor" does not work since it breaks
    // at delimiters like '/'. Get the whole line
    QTextCursor cursor = cursorForPosition(widgetPos);
    const int cursorDocumentPos = cursor.position();
    cursor.select(QTextCursor::BlockUnderCursor);
    if (!cursor.hasSelection())
        return {};
    const QString block = cursor.selectedText();
    // Determine cursor position within line and find blank-delimited word
    const int cursorPos = cursorDocumentPos - cursor.block().position();
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
    for ( ; endPos < blockSize && !block.at(endPos).isSpace(); endPos++) ;
    return endPos > startPos ? block.mid(startPos, endPos - startPos) : QString();
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
        // Check for a file, expand via repository if relative
        if (!repo.isEmpty() && !repo.isFile() && repo.isRelativePath())
            repo = repo.pathAppended(token);
        if (repo.isFile())  {
            menu->addSeparator();
            openAction = menu->addAction(Tr::tr("Open \"%1\"").arg(repo.nativePath()));
            connect(openAction, &QAction::triggered, this, [fp = repo.absoluteFilePath()] {
                EditorManager::openEditor(fp);
            });
        }
    }
}

void OutputWindowPlainTextEdit::handleLink(const QPoint &pos)
{
    const QString href = anchorAt(pos);
    if (href.isEmpty())
        return;
    FilePath repository;
    identifierUnderCursor(pos, &repository);
    if (repository.isEmpty()) {
        OutputWindow::handleLink(pos);
        return;
    }
    if (outputFormatter()->handleFileLink(href))
        return;
    m_parser->handleVcsLink(repository, href);
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
    FilePath repository;
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
        d->widget.setWheelZoomEnabled(
                    TextEditor::globalBehaviorSettings().m_scrollWheelZooming);
    };

    auto updateFontSettings = [] {
        d->widget.setBaseFont(TextEditor::TextEditorSettings::fontSettings().font());
    };

    updateBehaviorSettings();
    updateFontSettings();
    setupContext(Internal::C_VCS_OUTPUT_PANE, &d->widget);

    connect(this, &IOutputPane::zoomInRequested, &d->widget, &OutputWindow::zoomIn);
    connect(this, &IOutputPane::zoomOutRequested, &d->widget, &OutputWindow::zoomOut);
    connect(this, &IOutputPane::resetZoomRequested, &d->widget, &OutputWindow::resetZoom);
    connect(TextEditor::TextEditorSettings::instance(), &TextEditor::TextEditorSettings::behaviorSettingsChanged,
            this, updateBehaviorSettings);
    connect(TextEditor::TextEditorSettings::instance(),
            &TextEditor::TextEditorSettings::fontSettingsChanged, this, updateFontSettings);
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

void VcsOutputWindow::append(const QString &text, MessageStyle style, bool silently)
{
    d->widget.appendLines(text, style, d->repository);

    if (!silently && !d->widget.isVisible())
        m_instance->popup(IOutputPane::NoModeSwitch);
}

void VcsOutputWindow::appendSilently(const QString &text)
{
    append((text.endsWith('\n') || text.endsWith('\r')) ? text : text + '\n', None, true);
}

void VcsOutputWindow::appendMessage(const QString &text)
{
    append(text + '\n', Message, true);
}

void VcsOutputWindow::appendWarning(const QString &text)
{
    append(text + '\n', Warning, false);
}

void VcsOutputWindow::appendError(const QString &text)
{
    append((text.endsWith('\n') || text.endsWith('\r')) ? text : text + '\n', Error, false);
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

void VcsOutputWindow::appendShellCommandLine(const QString &text)
{
    append(filterPasswordFromUrls(text), Command, true);
}

void VcsOutputWindow::appendCommand(const FilePath &workingDirectory, const CommandLine &command)
{
    appendShellCommandLine(msgExecutionLogEntry(workingDirectory, command));
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

void VcsOutputWindow::setRepository(const FilePath &repository)
{
    d->repository = repository;
}

void VcsOutputWindow::clearRepository()
{
    d->repository.clear();
}

} // namespace VcsBase
