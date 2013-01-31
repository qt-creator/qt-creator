/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "logwindow.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"

#include <QDebug>
#include <QFile>
#include <QTime>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QKeyEvent>
#include <QMenu>
#include <QSpacerItem>
#include <QSyntaxHighlighter>
#include <QPlainTextEdit>
#include <QFileDialog>
#include <QMessageBox>

#include <aggregation/aggregate.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>
#include <find/basetextfind.h>

#include <utils/savedaction.h>
#include <utils/fileutils.h>
#include <utils/fancylineedit.h>

namespace Debugger {
namespace Internal {

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
    void highlightBlock(const QString &text)
    {
        QTextCharFormat format;
        switch (LogWindow::channelForChar(text.isEmpty() ? QChar() : text.at(0))) {
            case LogInput:
                format.setForeground(Qt::blue);
                setFormat(1, text.size(), format);
                break;
            case LogStatus:
                format.setForeground(Qt::darkGreen);
                setFormat(1, text.size(), format);
                break;
            case LogWarning:
                format.setForeground(Qt::darkYellow);
                setFormat(1, text.size(), format);
                break;
            case LogError:
                format.setForeground(Qt::red);
                setFormat(1, text.size(), format);
                break;
            case LogTime:
                format.setForeground(Qt::darkRed);
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
    void highlightBlock(const QString &text)
    {
        if (text.size() > 3 && text.at(2) == QLatin1Char(':')) {
            QTextCharFormat format;
            format.setForeground(Qt::darkRed);
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
    DebuggerPane(QWidget *parent)
        : QPlainTextEdit(parent)
    {
        setFrameStyle(QFrame::NoFrame);
        m_clearContentsAction = new QAction(this);
        m_clearContentsAction->setText(tr("Clear Contents"));
        m_clearContentsAction->setEnabled(true);
        connect(m_clearContentsAction, SIGNAL(triggered(bool)),
            parent, SLOT(clearContents()));

        m_saveContentsAction = new QAction(this);
        m_saveContentsAction->setText(tr("Save Contents"));
        m_saveContentsAction->setEnabled(true);
        connect(m_saveContentsAction, SIGNAL(triggered()),
            this, SLOT(saveContents()));

        m_reloadDebuggingHelpersAction = new QAction(this);
        m_reloadDebuggingHelpersAction->setText(tr("Reload Debugging Helpers"));
        m_reloadDebuggingHelpersAction->setEnabled(true);
        connect(m_reloadDebuggingHelpersAction, SIGNAL(triggered()),
            this, SLOT(reloadDebuggingHelpers()));
    }

    void contextMenuEvent(QContextMenuEvent *ev)
    {
        QMenu *menu = createStandardContextMenu();
        menu->addAction(m_clearContentsAction);
        menu->addAction(m_saveContentsAction); // X11 clipboard is unreliable for long texts
        menu->addAction(debuggerCore()->action(LogTimeStamps));
        menu->addAction(debuggerCore()->action(VerboseLog));
        menu->addAction(m_reloadDebuggingHelpersAction);
        menu->addSeparator();
        menu->addAction(debuggerCore()->action(SettingsDialog));
        menu->exec(ev->globalPos());
        delete menu;
    }

    void append(const QString &text)
    {
        const int N = 100000;
        if (blockCount() > N) {
            QTextBlock block = document()->findBlock(9 * N / 10);
            QTextCursor tc(block);
            tc.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
            tc.removeSelectedText();
        }
        appendPlainText(text);
    }


private slots:
    void saveContents();
    void reloadDebuggingHelpers();

private:
    QAction *m_clearContentsAction;
    QAction *m_saveContentsAction;
    QAction *m_reloadDebuggingHelpersAction;
};

void DebuggerPane::saveContents()
{
    LogWindow::writeLogContents(this, this);
}

void DebuggerPane::reloadDebuggingHelpers()
{
    debuggerCore()->currentEngine()->reloadDebuggingHelpers();
}

/////////////////////////////////////////////////////////////////////
//
// InputPane
//
/////////////////////////////////////////////////////////////////////

class InputPane : public DebuggerPane
{
    Q_OBJECT
public:
    InputPane(QWidget *parent)
        : DebuggerPane(parent)
    {
        (void) new InputHighlighter(this);
    }

signals:
    void executeLineRequested();
    void clearContentsRequested();
    void statusMessageRequested(const QString &, int);
    void commandSelected(int);

private:
    void keyPressEvent(QKeyEvent *ev)
    {
        if (ev->modifiers() == Qt::ControlModifier && ev->key() == Qt::Key_Return)
            emit executeLineRequested();
        else if (ev->modifiers() == Qt::ControlModifier && ev->key() == Qt::Key_R)
            emit clearContentsRequested();
        else
            QPlainTextEdit::keyPressEvent(ev);
    }

    void mouseDoubleClickEvent(QMouseEvent *ev)
    {
        QString line = cursorForPosition(ev->pos()).block().text();
        int n = 0;

        // cut time string
        if (line.size() > 18 && line.at(0) == QLatin1Char('['))
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

    void focusInEvent(QFocusEvent *ev)
    {
        emit statusMessageRequested(tr("Type Ctrl-<Return> to execute a line."), -1);
        QPlainTextEdit::focusInEvent(ev);
    }

    void focusOutEvent(QFocusEvent *ev)
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
    CombinedPane(QWidget *parent)
        : DebuggerPane(parent)
    {
        (void) new OutputHighlighter(this);
    }

public slots:
    void gotoResult(int i)
    {
        QString needle = QString::number(i) + QLatin1Char('^');
        QString needle2 = QLatin1Char('>') + needle;
        QTextCursor cursor(document());
        do {
            cursor = document()->find(needle, cursor);
            if (cursor.isNull())
                break; // Not found.
            const QString line = cursor.block().text();
            if (line.startsWith(needle) || line.startsWith(needle2)) {
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

LogWindow::LogWindow(QWidget *parent)
  : QWidget(parent)
{
    setWindowTitle(tr("Debugger Log"));
    setObjectName(QLatin1String("Log"));

    m_ignoreNextInputEcho = false;

    QSplitter *m_splitter = new Core::MiniSplitter(Qt::Horizontal);
    m_splitter->setParent(this);

    // Mixed input/output.
    m_combinedText = new CombinedPane(this);
    m_combinedText->setReadOnly(true);
    m_combinedText->setReadOnly(false);
    m_combinedText->setSizePolicy(QSizePolicy::MinimumExpanding,
        QSizePolicy::MinimumExpanding);

    // Input only.
    m_inputText = new InputPane(this);
    m_inputText->setReadOnly(false);
    m_inputText->setSizePolicy(QSizePolicy::MinimumExpanding,
        QSizePolicy::MinimumExpanding);

    m_commandLabel = new QLabel(tr("Command:"), this);
    m_commandEdit = new Utils::FancyLineEdit(this);
    m_commandEdit->setFrame(false);
    m_commandEdit->setHistoryCompleter(QLatin1String("DebuggerInput"));
    QHBoxLayout *commandBox = new QHBoxLayout;
    commandBox->addWidget(m_commandLabel);
    commandBox->addWidget(m_commandEdit);
    commandBox->setMargin(2);
    commandBox->setSpacing(6);

    QVBoxLayout *leftBox = new QVBoxLayout;
    leftBox->addWidget(m_inputText);
    leftBox->addItem(commandBox);
    leftBox->setMargin(0);
    leftBox->setSpacing(0);

    QWidget *leftDummy = new QWidget;
    leftDummy->setLayout(leftBox);

    m_splitter->addWidget(leftDummy);
    m_splitter->addWidget(m_combinedText);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 3);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_splitter);
    layout->addWidget(new Core::FindToolBarPlaceHolder(this));
    setLayout(layout);

    Aggregation::Aggregate *aggregate = new Aggregation::Aggregate;
    aggregate->add(m_combinedText);
    aggregate->add(new Find::BaseTextFind(m_combinedText));

    aggregate = new Aggregation::Aggregate;
    aggregate->add(m_inputText);
    aggregate->add(new Find::BaseTextFind(m_inputText));

    connect(m_inputText, SIGNAL(statusMessageRequested(QString,int)),
        SIGNAL(statusMessageRequested(QString,int)));
    connect(m_inputText, SIGNAL(commandSelected(int)),
        m_combinedText, SLOT(gotoResult(int)));
    connect(m_commandEdit, SIGNAL(returnPressed()),
        SLOT(sendCommand()));
    connect(m_inputText, SIGNAL(executeLineRequested()),
        SLOT(executeLine()));

    connect(&m_outputTimer, SIGNAL(timeout()), SLOT(doOutput()));

    setMinimumHeight(60);
}

void LogWindow::executeLine()
{
    m_ignoreNextInputEcho = true;
    debuggerCore()->executeDebuggerCommand(m_inputText->textCursor().block().text(),
                                           CppLanguage);
}

void LogWindow::sendCommand()
{
    debuggerCore()->executeDebuggerCommand(m_commandEdit->text(), CppLanguage);
}

void LogWindow::showOutput(int channel, const QString &output)
{
    if (output.isEmpty())
        return;

    const QChar cchar = charForChannel(channel);
    const QChar nchar = QLatin1Char('\n');

    QString out;
    out.reserve(output.size() + 1000);

    if (output.at(0) != QLatin1Char('~') && debuggerCore()->boolSetting(LogTimeStamps)) {
        out.append(charForChannel(LogTime));
        out.append(logTimeStamp());
        out.append(nchar);
    }

    for (int pos = 0, n = output.size(); pos < n; ) {
        const int npos = output.indexOf(nchar, pos);
        const int nnpos = npos == -1 ? n : npos;
        const int l = nnpos - pos;
        if (l != 6 || output.midRef(pos, 6) != QLatin1String("(gdb) "))  {
            out.append(cchar);
            if (l > 30000) {
                // FIXME: QTextEdit asserts on really long lines...
                out.append(output.midRef(pos, 30000));
                out.append(QLatin1String(" [...] <cut off>\n"));
            } else {
                out.append(output.midRef(pos, l + 1));
            }
        }
        pos = nnpos + 1;
    }
    if (!out.endsWith(nchar))
        out.append(nchar);

    m_queuedOutput.append(out);
    m_outputTimer.setSingleShot(true);
    m_outputTimer.start(80);
}

void LogWindow::doOutput()
{
    if (m_queuedOutput.isEmpty())
        return;

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
    if (debuggerCore()->boolSetting(LogTimeStamps))
        m_inputText->append(logTimeStamp());
    m_inputText->append(input);
    QTextCursor cursor = m_inputText->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_inputText->setTextCursor(cursor);
    m_inputText->ensureCursorVisible();
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

QString LogWindow::logTimeStamp()
{
    // Cache the last log time entry by ms. If time progresses,
    // report the difference to the last time stamp in ms.
    static const QString logTimeFormat(QLatin1String("hh:mm:ss.zzz"));
    static QTime lastTime = QTime::currentTime();
    static QString lastTimeStamp = lastTime.toString(logTimeFormat);

    const QTime currentTime = QTime::currentTime();
    if (currentTime != lastTime) {
        const int elapsedMS = lastTime.msecsTo(currentTime);
        lastTime = currentTime;
        lastTimeStamp = lastTime.toString(logTimeFormat);
        // Append time elapsed
        QString rc = lastTimeStamp;
        rc += QLatin1String(" [");
        rc += QString::number(elapsedMS);
        rc += QLatin1String("ms]");
        return rc;
    }
    return lastTimeStamp;
}

bool LogWindow::writeLogContents(const QPlainTextEdit *editor, QWidget *parent)
{
    bool success = false;
    while (!success) {
        const QString fileName = QFileDialog::getSaveFileName(parent, tr("Log File"));
        if (fileName.isEmpty())
            break;
        Utils::FileSaver saver(fileName, QIODevice::Text);
        saver.write(editor->toPlainText().toUtf8());
        if (saver.finalize(parent))
            success = true;
    }
    return success;
}

QChar LogWindow::charForChannel(int channel)
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

LogChannel LogWindow::channelForChar(QChar c)
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

} // namespace Internal
} // namespace Debugger

#include "logwindow.moc"
