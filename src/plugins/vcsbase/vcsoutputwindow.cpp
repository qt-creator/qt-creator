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

#include "vcsoutputwindow.h"

#include <coreplugin/editormanager/editormanager.h>

#include <aggregation/aggregate.h>
#include <coreplugin/find/basetextfind.h>
#include <coreplugin/outputwindow.h>
#include <utils/fileutils.h>
#include <utils/outputformatter.h>
#include <utils/theme/theme.h>

#include <QPlainTextEdit>
#include <QTextCharFormat>
#include <QContextMenuEvent>
#include <QTextBlock>
#include <QMenu>
#include <QAction>
#include <QTextBlockUserData>

#include <QPointer>
#include <QTextCodec>
#include <QDir>
#include <QRegExp>
#include <QTextStream>
#include <QTime>
#include <QPoint>
#include <QFileInfo>

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

// Store repository along with text blocks
class RepositoryUserData : public QTextBlockUserData
{
public:
    explicit RepositoryUserData(const QString &repo) : m_repository(repo) {}
    const QString &repository() const { return m_repository; }

private:
    const QString m_repository;
};

// A plain text edit with a special context menu containing "Clear" and
// and functions to append specially formatted entries.
class OutputWindowPlainTextEdit : public Core::OutputWindow
{
public:
    explicit OutputWindowPlainTextEdit(QWidget *parent = 0);
    ~OutputWindowPlainTextEdit();

    void appendLines(QString const& s, const QString &repository = QString());
    void appendLinesWithStyle(QString const& s, enum VcsOutputWindow::MessageStyle style, const QString &repository = QString());

protected:
    virtual void contextMenuEvent(QContextMenuEvent *event);

private:
    void setFormat(enum VcsOutputWindow::MessageStyle style);
    QString identifierUnderCursor(const QPoint &pos, QString *repository = 0) const;

    const QTextCharFormat m_defaultFormat;
    QTextCharFormat m_errorFormat;
    QTextCharFormat m_warningFormat;
    QTextCharFormat m_commandFormat;
    QTextCharFormat m_messageFormat;
    Utils::OutputFormatter *m_formatter;
};

OutputWindowPlainTextEdit::OutputWindowPlainTextEdit(QWidget *parent) :
    Core::OutputWindow(Core::Context(C_VCS_OUTPUT_PANE), parent),
    m_defaultFormat(currentCharFormat()),
    m_errorFormat(m_defaultFormat),
    m_warningFormat(m_defaultFormat),
    m_commandFormat(m_defaultFormat),
    m_messageFormat(m_defaultFormat)
{
    using Utils::Theme;
    setReadOnly(true);
    setUndoRedoEnabled(false);
    setFrameStyle(QFrame::NoFrame);
    m_errorFormat.setForeground(Utils::creatorTheme()->color(Theme::OutputPanes_ErrorMessageTextColor));
    m_warningFormat.setForeground(Utils::creatorTheme()->color(Theme::OutputPanes_WarningMessageTextColor));
    m_commandFormat.setFontWeight(QFont::Bold);
    m_messageFormat.setForeground(Utils::creatorTheme()->color(Theme::OutputPanes_MessageOutput));
    m_formatter = new Utils::OutputFormatter;
    m_formatter->setPlainTextEdit(this);
    Aggregation::Aggregate *agg = new Aggregation::Aggregate;
    agg->add(this);
    agg->add(new Core::BaseTextFind(this));
}

OutputWindowPlainTextEdit::~OutputWindowPlainTextEdit()
{
    delete m_formatter;
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

QString OutputWindowPlainTextEdit::identifierUnderCursor(const QPoint &widgetPos, QString *repository) const
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
        return QString();
    QString block = cursor.selectedText();
    // Determine cursor position within line and find blank-delimited word
    const int cursorPos = cursorDocumentPos - cursor.block().position();
    const int blockSize = block.size();
    if (cursorPos < 0 || cursorPos >= blockSize || block.at(cursorPos).isSpace())
        return QString();
    // Retrieve repository if desired
    if (repository)
        if (QTextBlockUserData *data = cursor.block().userData())
            *repository = static_cast<const RepositoryUserData*>(data)->repository();
    // Find first non-space character of word and find first non-space character past
    const int startPos = firstWordCharacter(block, cursorPos);
    int endPos = cursorPos;
    for ( ; endPos < blockSize && !block.at(endPos).isSpace(); endPos++) ;
    return endPos > startPos ? block.mid(startPos, endPos - startPos) : QString();
}

void OutputWindowPlainTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();
    // Add 'open file'
    QString repository;
    const QString token = identifierUnderCursor(event->pos(), &repository);
    QAction *openAction = 0;
    if (!token.isEmpty()) {
        // Check for a file, expand via repository if relative
        QFileInfo fi(token);
        if (!repository.isEmpty() && !fi.isFile() && fi.isRelative())
            fi = QFileInfo(repository + QLatin1Char('/') + token);
        if (fi.isFile())  {
            menu->addSeparator();
            openAction = menu->addAction(VcsOutputWindow::tr("Open \"%1\"").
                                         arg(QDir::toNativeSeparators(fi.fileName())));
            openAction->setData(fi.absoluteFilePath());
        }
    }
    // Add 'clear'
    menu->addSeparator();
    QAction *clearAction = menu->addAction(VcsOutputWindow::tr("Clear"));

    // Run
    QAction *action = menu->exec(event->globalPos());
    if (action) {
        if (action == clearAction) {
            clear();
            return;
        }
        if (action == openAction) {
            const QString fileName = action->data().toString();
            Core::EditorManager::openEditor(fileName);
        }
    }
    delete menu;
}

void OutputWindowPlainTextEdit::appendLines(QString const& s, const QString &repository)
{
    if (s.isEmpty())
        return;

    const int previousLineCount = document()->lineCount();

    const QChar newLine(QLatin1Char('\n'));
    const QChar lastChar = s.at(s.size() - 1);
    const bool appendNewline = (lastChar != QLatin1Char('\r') && lastChar != newLine);
    m_formatter->appendMessage(appendNewline ? s + newLine : s, currentCharFormat());

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

void OutputWindowPlainTextEdit::appendLinesWithStyle(QString const& s, enum VcsOutputWindow::MessageStyle style, const QString &repository)
{
    setFormat(style);

    if (style == VcsOutputWindow::Command) {
        const QString timeStamp = QTime::currentTime().toString(QLatin1String("\nHH:mm "));
        appendLines(timeStamp + s, repository);
    }
    else {
        appendLines(s, repository);
    }

    setCurrentCharFormat(m_defaultFormat);
}

void OutputWindowPlainTextEdit::setFormat(enum VcsOutputWindow::MessageStyle style)
{
    switch (style) {
    case VcsOutputWindow::Warning:
        setCurrentCharFormat(m_warningFormat);
        break;
    case VcsOutputWindow::Error:
        setCurrentCharFormat(m_errorFormat);
        break;
    case VcsOutputWindow::Message:
        setCurrentCharFormat(m_messageFormat);
        break;
    case VcsOutputWindow::Command:
        setCurrentCharFormat(m_commandFormat);
        break;
    default:
    case VcsOutputWindow::None:
        setCurrentCharFormat(m_defaultFormat);
        break;
    }
}

} // namespace Internal

// ------------------- VcsBaseOutputWindowPrivate
class VcsOutputWindowPrivate
{
public:
    Internal::OutputWindowPlainTextEdit widget;
    QString repository;
    QRegExp passwordRegExp;
};

static VcsOutputWindow *m_instance = 0;
static VcsOutputWindowPrivate *d = 0;

VcsOutputWindow::VcsOutputWindow()
{
    d = new VcsOutputWindowPrivate;
    d->passwordRegExp = QRegExp(QLatin1String("://([^@:]+):([^@]+)@"));
    Q_ASSERT(d->passwordRegExp.isValid());
    m_instance = this;
}

static QString filterPasswordFromUrls(const QString &input)
{
    int pos = 0;
    QString result = input;
    while ((pos = d->passwordRegExp.indexIn(result, pos)) >= 0) {
        QString tmp = result.left(pos + 3) + d->passwordRegExp.cap(1) + QLatin1String(":***@");
        int newStart = tmp.count();
        tmp += result.mid(pos + d->passwordRegExp.matchedLength());
        result = tmp;
        pos = newStart;
    }
    return result;
}

VcsOutputWindow::~VcsOutputWindow()
{
    m_instance = 0;
    delete d;
}

QWidget *VcsOutputWindow::outputWidget(QWidget *parent)
{
    if (parent != d->widget.parent())
        d->widget.setParent(parent);
    return &d->widget;
}

QList<QWidget *> VcsOutputWindow::toolBarWidgets() const
{
    return {};
}

QString VcsOutputWindow::displayName() const
{
    return tr("Version Control");
}

int VcsOutputWindow::priorityInStatusBar() const
{
    return -1;
}

void VcsOutputWindow::clearContents()
{
    d->widget.clear();
}

void VcsOutputWindow::visibilityChanged(bool visible)
{
    if (visible)
        d->widget.setFocus();
}

void VcsOutputWindow::setFocus()
{
}

bool VcsOutputWindow::hasFocus() const
{
    return false;
}

bool VcsOutputWindow::canFocus() const
{
    return false;
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
    setText(QTextCodec::codecForLocale()->toUnicode(data));
}

void VcsOutputWindow::appendSilently(const QString &text)
{
    append(text, None, true);
}

void VcsOutputWindow::append(const QString &text, enum MessageStyle style, bool silently)
{
    d->widget.appendLinesWithStyle(text, style, d->repository);

    if (!silently && !d->widget.isVisible())
        m_instance->popup(Core::IOutputPane::NoModeSwitch);
}

void VcsOutputWindow::appendError(const QString &text)
{
    append(text, Error, false);
}

void VcsOutputWindow::appendWarning(const QString &text)
{
    append(text, Warning, false);
}

// Helper to format arguments for log windows hiding common password
// options.
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
        if (arg.startsWith(QString::fromLatin1(passwordOptionC) + QLatin1Char('='))) {
            str << "--password=********";
            continue;
        }
        str << arg;
        if (arg == QLatin1String(passwordOptionC)) {
            str << " ********";
            i++;
        }
    }
    return rc;
}

QString VcsOutputWindow::msgExecutionLogEntry(const QString &workingDir,
                                                  const Utils::FileName &executable,
                                                  const QStringList &arguments)
{
    const QString args = formatArguments(arguments);
    const QString nativeExecutable = executable.toUserOutput();
    if (workingDir.isEmpty())
        return tr("Running: %1 %2").arg(nativeExecutable, args) + QLatin1Char('\n');
    return tr("Running in %1: %2 %3").
            arg(QDir::toNativeSeparators(workingDir), nativeExecutable, args) + QLatin1Char('\n');
}

void VcsOutputWindow::appendShellCommandLine(const QString &text)
{
    append(filterPasswordFromUrls(text), Command, true);
}

void VcsOutputWindow::appendCommand(const QString &workingDirectory,
                                    const Utils::FileName &binary,
                                    const QStringList &args)
{
    appendShellCommandLine(msgExecutionLogEntry(workingDirectory, binary, args));
}

void VcsOutputWindow::appendMessage(const QString &text)
{
    append(text, Message, true);
}

VcsOutputWindow *VcsOutputWindow::instance()
{
    if (!m_instance)
        (void) new VcsOutputWindow;
    return m_instance;
}

QString VcsOutputWindow::repository() const
{
    return d->repository;
}

void VcsOutputWindow::setRepository(const QString &r)
{
    d->repository = r;
}

void VcsOutputWindow::clearRepository()
{
    d->repository.clear();
}

} // namespace VcsBase
