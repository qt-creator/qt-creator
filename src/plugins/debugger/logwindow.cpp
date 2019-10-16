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

#include "logwindow.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggericons.h"

#include <QDebug>
#include <QTime>

#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QSyntaxHighlighter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QToolButton>

#include <aggregation/aggregate.h>

#include <app/app_version.h>

#include <coreplugin/findplaceholder.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/find/basetextfind.h>

#include <utils/savedaction.h>
#include <utils/fancylineedit.h>
#include <utils/fileutils.h>
#include <utils/theme/theme.h>

namespace Debugger {
namespace Internal {

GlobalLogWindow *theGlobalLog = nullptr;

static LogChannel channelForChar(QChar c)
{
    switch (c.unicode()) {
        case 'd': return LogDebug;
        case 'w': return LogWarning;
        case 'e': return LogError;
        case '<': return LogInput;
        case '>': return LogOutput;
        case 's': return LogStatus;
        case 't': return LogTime;
        default: return LogMisc;
    }
}

QChar static charForChannel(int channel)
{
    switch (channel) {
        case LogDebug: return QLatin1Char('d');
        case LogWarning: return QLatin1Char('w');
        case LogError: return QLatin1Char('e');
        case LogInput: return QLatin1Char('<');
        case LogOutput: return QLatin1Char('>');
        case LogStatus: return QLatin1Char('s');
        case LogTime: return QLatin1Char('t');
        case LogMisc:
        default: return QLatin1Char(' ');
    }
}

static bool writeLogContents(const QPlainTextEdit *editor, QWidget *parent)
{
    bool success = false;
    while (!success) {
        const QString fileName = QFileDialog::getSaveFileName(parent, LogWindow::tr("Log File"));
        if (fileName.isEmpty())
            break;
        Utils::FileSaver saver(fileName, QIODevice::Text);
        saver.write(editor->toPlainText().toUtf8());
        if (saver.finalize(parent))
            success = true;
    }
    return success;
}

/////////////////////////////////////////////////////////////////////
//
// OutputHighlighter
//
/////////////////////////////////////////////////////////////////////

class OutputHighlighter : public QSyntaxHighlighter
{
public:
    OutputHighlighter(QPlainTextEdit *parent)
        : QSyntaxHighlighter(parent->document()), m_parent(parent)
    {}

private:
    void highlightBlock(const QString &text) override
    {
        using Utils::Theme;
        QTextCharFormat format;
        Theme *theme = Utils::creatorTheme();
        switch (channelForChar(text.isEmpty() ? QChar() : text.at(0))) {
            case LogInput:
                format.setForeground(theme->color(Theme::Debugger_LogWindow_LogInput));
                setFormat(1, text.size(), format);
                break;
            case LogStatus:
                format.setForeground(theme->color(Theme::Debugger_LogWindow_LogStatus));
                setFormat(1, text.size(), format);
                break;
            case LogWarning:
                format.setForeground(theme->color(Theme::OutputPanes_WarningMessageTextColor));
                setFormat(1, text.size(), format);
                break;
            case LogError:
                format.setForeground(theme->color(Theme::OutputPanes_ErrorMessageTextColor));
                setFormat(1, text.size(), format);
                break;
            case LogTime:
                format.setForeground(theme->color(Theme::Debugger_LogWindow_LogTime));
                setFormat(1, text.size(), format);
                break;
            default:
                break;
        }
        QColor base = m_parent->palette().color(QPalette::Base);
        format.setForeground(base);
        format.setFontPointSize(1);
        setFormat(0, 1, format);
    }

    QPlainTextEdit *m_parent;
};


/////////////////////////////////////////////////////////////////////
//
// InputHighlighter
//
/////////////////////////////////////////////////////////////////////

class InputHighlighter : public QSyntaxHighlighter
{
public:
    InputHighlighter(QPlainTextEdit *parent)
        : QSyntaxHighlighter(parent->document())
    {}

private:
    void highlightBlock(const QString &text) override
    {
        using Utils::Theme;
        Theme *theme = Utils::creatorTheme();
        if (text.size() > 3 && text.at(2) == ':') {
            QTextCharFormat format;
            format.setForeground(theme->color(Theme::Debugger_LogWindow_LogTime));
            setFormat(1, text.size(), format);
        }
    }
};


/////////////////////////////////////////////////////////////////////
//
// DebbuggerPane base class
//
/////////////////////////////////////////////////////////////////////

class DebuggerPane : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit DebuggerPane()
    {
        setFrameStyle(QFrame::NoFrame);
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        m_clearContentsAction = new QAction(this);
        m_clearContentsAction->setText(tr("Clear Contents"));
        m_clearContentsAction->setEnabled(true);

        m_saveContentsAction = new QAction(this);
        m_saveContentsAction->setText(tr("Save Contents"));
        m_saveContentsAction->setEnabled(true);
        connect(m_saveContentsAction, &QAction::triggered,
                this, &DebuggerPane::saveContents);

        m_reloadDebuggingHelpersAction = new QAction(this);
        m_reloadDebuggingHelpersAction->setText(tr("Reload Debugging Helpers"));
        m_reloadDebuggingHelpersAction->setEnabled(true);
    }

    void contextMenuEvent(QContextMenuEvent *ev) override
    {
        QMenu *menu = createStandardContextMenu();
        menu->addAction(m_clearContentsAction);
        menu->addAction(m_saveContentsAction); // X11 clipboard is unreliable for long texts
        menu->addAction(action(LogTimeStamps));
        menu->addAction(m_reloadDebuggingHelpersAction);
        menu->addSeparator();
        menu->addAction(action(SettingsDialog));
        menu->exec(ev->globalPos());
        delete menu;
    }

    void append(const QString &text)
    {
        const int N = 100000;
        const int bc = blockCount();
        if (bc > N) {
            QTextDocument *doc = document();
            QTextBlock block = doc->findBlockByLineNumber(bc * 9 / 10);
            QTextCursor tc(block);
            tc.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
            tc.removeSelectedText();
            // Seems to be the only way to force shrinking of the
            // allocated data.
            QString contents = doc->toHtml();
            doc->clear();
            doc->setHtml(contents);
        }
        appendPlainText(text);
    }

    void clearUndoRedoStacks()
    {
        if (!isUndoRedoEnabled())
            return;
        setUndoRedoEnabled(false);
        setUndoRedoEnabled(true);
    }

    QAction *clearContentsAction() const { return m_clearContentsAction; }
    QAction *reloadDebuggingHelpersAction() const { return m_reloadDebuggingHelpersAction; }

private:
    void saveContents() { writeLogContents(this, this); }

    QAction *m_clearContentsAction;
    QAction *m_saveContentsAction;
    QAction *m_reloadDebuggingHelpersAction;
};



/////////////////////////////////////////////////////////////////////
//
// InputPane
//
/////////////////////////////////////////////////////////////////////

class InputPane : public DebuggerPane
{
    Q_OBJECT
public:
    InputPane(LogWindow *logWindow)
    {
        connect(clearContentsAction(), &QAction::triggered,
                logWindow, &LogWindow::clearContents);
        connect(reloadDebuggingHelpersAction(), &QAction::triggered,
                logWindow->engine(), &DebuggerEngine::reloadDebuggingHelpers);
        (void) new InputHighlighter(this);
    }

signals:
    void executeLineRequested();
    void clearContentsRequested();
    void statusMessageRequested(const QString &, int);
    void commandSelected(int);

private:
    void keyPressEvent(QKeyEvent *ev) override
    {
        if (ev->modifiers() == Qt::ControlModifier && ev->key() == Qt::Key_Return)
            emit executeLineRequested();
        else if (ev->modifiers() == Qt::ControlModifier && ev->key() == Qt::Key_R)
            emit clearContentsRequested();
        else
            QPlainTextEdit::keyPressEvent(ev);
    }

    void mouseDoubleClickEvent(QMouseEvent *ev) override
    {
        QString line = cursorForPosition(ev->pos()).block().text();
        int n = 0;

        // cut time string
        if (line.size() > 18 && line.at(0) == '[')
            line = line.mid(18);
        //qDebug() << line;

        for (int i = 0; i != line.size(); ++i) {
            QChar c = line.at(i);
            if (!c.isDigit())
                break;
            n = 10 * n + c.unicode() - '0';
        }
        emit commandSelected(n);
    }

    void focusInEvent(QFocusEvent *ev) override
    {
        emit statusMessageRequested(tr("Type Ctrl-<Return> to execute a line."), -1);
        QPlainTextEdit::focusInEvent(ev);
    }

    void focusOutEvent(QFocusEvent *ev) override
    {
        emit statusMessageRequested(QString(), -1);
        QPlainTextEdit::focusOutEvent(ev);
    }
};


/////////////////////////////////////////////////////////////////////
//
// CombinedPane
//
/////////////////////////////////////////////////////////////////////

class CombinedPane : public DebuggerPane
{
    Q_OBJECT
public:
    CombinedPane(LogWindow *logWindow)
    {
        (void) new OutputHighlighter(this);
        connect(clearContentsAction(), &QAction::triggered,
                logWindow, &LogWindow::clearContents);
        connect(reloadDebuggingHelpersAction(), &QAction::triggered,
                logWindow->engine(), &DebuggerEngine::reloadDebuggingHelpers);
    }

    void gotoResult(int i)
    {
        QString needle = QString::number(i) + '^';
        QString needle2 = '>' + needle;
        QString needle3 = QString::fromLatin1("dtoken(\"%1\")@").arg(i);
        QTextCursor cursor(document());
        do {
            QTextCursor newCursor = document()->find(needle, cursor);
            if (newCursor.isNull()) {
                newCursor = document()->find(needle3, cursor);
                if (newCursor.isNull())
                    break; // Not found.
            }
            cursor = newCursor;
            const QString line = cursor.block().text();
            if (line.startsWith(needle) || line.startsWith(needle2) || line.startsWith(needle3)) {
                setFocus();
                setTextCursor(cursor);
                ensureCursorVisible();
                cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor);
                setTextCursor(cursor);
                break;
            }
        } while (cursor.movePosition(QTextCursor::Down));
    }
};


/////////////////////////////////////////////////////////////////////
//
// DebuggerOutputWindow
//
/////////////////////////////////////////////////////////////////////

LogWindow::LogWindow(DebuggerEngine *engine)
    : m_engine(engine)
{
    setWindowTitle(tr("Debugger &Log"));
    setObjectName("Log");

    m_ignoreNextInputEcho = false;

    auto m_splitter = new Core::MiniSplitter(Qt::Horizontal);
    m_splitter->setParent(this);

    // Mixed input/output.
    m_combinedText = new CombinedPane(this);
    m_combinedText->setReadOnly(true);
    m_combinedText->setReadOnly(false);

    // Input only.
    m_inputText = new InputPane(this);
    m_inputText->setReadOnly(false);

    m_commandEdit = new Utils::FancyLineEdit(this);
    m_commandEdit->setFrame(false);
    m_commandEdit->setHistoryCompleter("DebuggerInput");

    auto repeatButton = new QToolButton(this);
    repeatButton->setIcon(Icons::STEP_OVER.icon());
    repeatButton->setFixedSize(QSize(18, 18));
    repeatButton->setToolTip(tr("Repeat last command for debug reasons."));

    auto commandBox = new QHBoxLayout;
    commandBox->addWidget(repeatButton);
    commandBox->addWidget(new QLabel(tr("Command:"), this));
    commandBox->addWidget(m_commandEdit);
    commandBox->setContentsMargins(2, 2, 2, 2);
    commandBox->setSpacing(6);

    auto leftBox = new QVBoxLayout;
    leftBox->addWidget(m_inputText);
    leftBox->addItem(commandBox);
    leftBox->setContentsMargins(0, 0, 0, 0);
    leftBox->setSpacing(0);

    auto leftDummy = new QWidget;
    leftDummy->setLayout(leftBox);

    m_splitter->addWidget(leftDummy);
    m_splitter->addWidget(m_combinedText);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 3);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_splitter);
    layout->addWidget(new Core::FindToolBarPlaceHolder(this));
    setLayout(layout);

    auto aggregate = new Aggregation::Aggregate;
    aggregate->add(m_combinedText);
    aggregate->add(new Core::BaseTextFind(m_combinedText));

    aggregate = new Aggregation::Aggregate;
    aggregate->add(m_inputText);
    aggregate->add(new Core::BaseTextFind(m_inputText));

    connect(m_inputText, &InputPane::statusMessageRequested,
            this, &LogWindow::statusMessageRequested);
    connect(m_inputText, &InputPane::commandSelected,
            m_combinedText, &CombinedPane::gotoResult);
    connect(m_commandEdit, &QLineEdit::returnPressed,
            this, &LogWindow::sendCommand);
    connect(m_inputText, &InputPane::executeLineRequested,
            this, &LogWindow::executeLine);
    connect(repeatButton, &QAbstractButton::clicked,
            this, &LogWindow::repeatLastCommand);

    connect(&m_outputTimer, &QTimer::timeout,
            this, &LogWindow::doOutput);

    setMinimumHeight(60);

    showOutput(LogWarning,
        tr("Note: This log contains possibly confidential information about your machine, "
           "environment variables, in-memory data of the processes you are debugging, and more. "
           "It is never transferred over the internet by %1, and only stored "
           "to disk if you manually use the respective option from the context menu, or through "
           "mechanisms that are not under the control of %1's Debugger plugin, "
           "for instance in swap files, or other plugins you might use.\n"
           "You may be asked to share the contents of this log when reporting bugs related "
           "to debugger operation. In this case, make sure your submission does not "
           "contain data you do not want to or you are not allowed to share.\n\n")
               .arg(Core::Constants::IDE_DISPLAY_NAME));
}

LogWindow::~LogWindow()
{
    disconnect(&m_outputTimer, &QTimer::timeout, this, &LogWindow::doOutput);
    m_outputTimer.stop();
    doOutput();
}

void LogWindow::executeLine()
{
    m_ignoreNextInputEcho = true;
    m_engine->executeDebuggerCommand(m_inputText->textCursor().block().text());
}

void LogWindow::repeatLastCommand()
{
    m_engine->debugLastCommand();
}

DebuggerEngine *LogWindow::engine() const
{
    return m_engine;
}

void LogWindow::sendCommand()
{
    if (m_engine->acceptsDebuggerCommands())
        m_engine->executeDebuggerCommand(m_commandEdit->text());
    else
        showOutput(LogError, tr("User commands are not accepted in the current state."));
}

void LogWindow::showOutput(int channel, const QString &output)
{
    if (output.isEmpty())
        return;

    const QChar cchar = charForChannel(channel);
    const QChar nchar = '\n';

    QString out;
    out.reserve(output.size() + 1000);

    if (output.at(0) != '~' && boolSetting(LogTimeStamps)) {
        out.append(charForChannel(LogTime));
        out.append(logTimeStamp());
        out.append(nchar);
    }

    for (int pos = 0, n = output.size(); pos < n; ) {
        const int npos = output.indexOf(nchar, pos);
        const int nnpos = npos == -1 ? n : npos;
        const int l = nnpos - pos;
        if (l != 6 || output.midRef(pos, 6) != "(gdb) ")  {
            out.append(cchar);
            if (l > 30000) {
                // FIXME: QTextEdit asserts on really long lines...
                out.append(output.midRef(pos, 30000));
                out.append(" [...] <cut off>\n");
            } else {
                out.append(output.midRef(pos, l + 1));
            }
        }
        pos = nnpos + 1;
    }
    if (!out.endsWith(nchar))
        out.append(nchar);

    m_queuedOutput.append(out);
    // flush the output if it exceeds 16k to prevent out of memory exceptions on regular output
    if (m_queuedOutput.size() > 16 * 1024) {
        m_outputTimer.stop();
        doOutput();
    } else {
        m_outputTimer.setSingleShot(true);
        m_outputTimer.start(80);
    }
}

void LogWindow::doOutput()
{
    if (m_queuedOutput.isEmpty())
        return;

    if (theGlobalLog)
        theGlobalLog->doOutput(m_queuedOutput);

    QTextCursor cursor = m_combinedText->textCursor();
    const bool atEnd = cursor.atEnd();

    m_combinedText->append(m_queuedOutput);
    m_queuedOutput.clear();

    if (atEnd) {
        cursor.movePosition(QTextCursor::End);
        m_combinedText->setTextCursor(cursor);
        m_combinedText->ensureCursorVisible();
    }
}

void LogWindow::showInput(int channel, const QString &input)
{
    Q_UNUSED(channel)
    if (m_ignoreNextInputEcho) {
        m_ignoreNextInputEcho = false;
        QTextCursor cursor = m_inputText->textCursor();
        cursor.movePosition(QTextCursor::Down);
        cursor.movePosition(QTextCursor::EndOfLine);
        m_inputText->setTextCursor(cursor);
        return;
    }
    if (boolSetting(LogTimeStamps))
        m_inputText->append(logTimeStamp());
    m_inputText->append(input);
    QTextCursor cursor = m_inputText->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_inputText->setTextCursor(cursor);
    m_inputText->ensureCursorVisible();

    theGlobalLog->doInput(input);
}

void LogWindow::clearContents()
{
    m_combinedText->clear();
    m_inputText->clear();
}

void LogWindow::setCursor(const QCursor &cursor)
{
    m_combinedText->viewport()->setCursor(cursor);
    m_inputText->viewport()->setCursor(cursor);
    QWidget::setCursor(cursor);
}

QString LogWindow::combinedContents() const
{
    return m_combinedText->toPlainText();
}

QString LogWindow::inputContents() const
{
    return m_inputText->toPlainText();
}

void LogWindow::clearUndoRedoStacks()
{
    m_inputText->clearUndoRedoStacks();
    m_combinedText->clearUndoRedoStacks();
}

QString LogWindow::logTimeStamp()
{
    // Cache the last log time entry by ms. If time progresses,
    // report the difference to the last time stamp in ms.
    static const QString logTimeFormat("hh:mm:ss.zzz");
    static QTime lastTime = QTime::currentTime();
    static QString lastTimeStamp = lastTime.toString(logTimeFormat);

    const QTime currentTime = QTime::currentTime();
    if (currentTime != lastTime) {
        const int elapsedMS = lastTime.msecsTo(currentTime);
        lastTime = currentTime;
        lastTimeStamp = lastTime.toString(logTimeFormat);
        // Append time elapsed
        QString rc = lastTimeStamp;
        rc += " [";
        rc += QString::number(elapsedMS);
        rc += "ms]";
        return rc;
    }
    return lastTimeStamp;
}

/////////////////////////////////////////////////////////////////////
//
// GlobalLogWindow
//
/////////////////////////////////////////////////////////////////////

GlobalLogWindow::GlobalLogWindow()
{
    theGlobalLog = this;

    setWindowTitle(tr("Global Debugger &Log"));
    setObjectName("GlobalLog");

    auto m_splitter = new Core::MiniSplitter(Qt::Horizontal);
    m_splitter->setParent(this);

    m_rightPane = new DebuggerPane;
    m_rightPane->setReadOnly(true);

    m_leftPane = new DebuggerPane;
    m_leftPane->setReadOnly(true);

    m_splitter->addWidget(m_leftPane);
    m_splitter->addWidget(m_rightPane);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 3);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_splitter);
    layout->addWidget(new Core::FindToolBarPlaceHolder(this));
    setLayout(layout);

    auto aggregate = new Aggregation::Aggregate;
    aggregate->add(m_rightPane);
    aggregate->add(new Core::BaseTextFind(m_rightPane));

    aggregate = new Aggregation::Aggregate;
    aggregate->add(m_leftPane);
    aggregate->add(new Core::BaseTextFind(m_leftPane));

    connect(m_leftPane->clearContentsAction(), &QAction::triggered,
            this, &GlobalLogWindow::clearContents);
    connect(m_rightPane->clearContentsAction(), &QAction::triggered,
            this, &GlobalLogWindow::clearContents);
}

GlobalLogWindow::~GlobalLogWindow()
{
    theGlobalLog = nullptr;
}

void GlobalLogWindow::doOutput(const QString &output)
{
    QTextCursor cursor = m_rightPane->textCursor();
    const bool atEnd = cursor.atEnd();

    m_rightPane->append(output);

    if (atEnd) {
        cursor.movePosition(QTextCursor::End);
        m_rightPane->setTextCursor(cursor);
        m_rightPane->ensureCursorVisible();
    }
}

void GlobalLogWindow::doInput(const QString &input)
{
    if (boolSetting(LogTimeStamps))
        m_leftPane->append(LogWindow::logTimeStamp());
    m_leftPane->append(input);
    QTextCursor cursor = m_leftPane->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_leftPane->setTextCursor(cursor);
    m_leftPane->ensureCursorVisible();
}

void GlobalLogWindow::clearContents()
{
    m_rightPane->clear();
    m_leftPane->clear();
}

void GlobalLogWindow::setCursor(const QCursor &cursor)
{
    m_rightPane->viewport()->setCursor(cursor);
    m_leftPane->viewport()->setCursor(cursor);
    QWidget::setCursor(cursor);
}

void GlobalLogWindow::clearUndoRedoStacks()
{
    m_leftPane->clearUndoRedoStacks();
    m_rightPane->clearUndoRedoStacks();
}

} // namespace Internal
} // namespace Debugger

#include "logwindow.moc"
