/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "logwindow.h"

#include "debuggeractions.h"
#include "debuggercore.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QTime>

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QKeyEvent>
#include <QtGui/QMenu>
#include <QtGui/QSpacerItem>
#include <QtGui/QSyntaxHighlighter>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

#include <aggregation/aggregate.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/minisplitter.h>
#include <find/basetextfind.h>

#include <utils/savedaction.h>
#include <utils/fileutils.h>

namespace Debugger {
namespace Internal {

static QChar charForChannel(int channel)
{
    switch (channel) {
        case LogDebug: return 'd';
        case LogWarning: return 'w';
        case LogError: return 'e';
        case LogInput: return '<';
        case LogOutput: return '>';
        case LogStatus: return 's';
        case LogTime: return 't';
        case LogMisc:
        default: return ' ';
    }
}

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
        switch (channelForChar(text.isEmpty() ? QChar() : text.at(0))) {
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
        : QSyntaxHighlighter(parent->document()), m_parent(parent)
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

    QPlainTextEdit *m_parent;
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
        setMaximumBlockCount(100000);
        setFrameStyle(QFrame::NoFrame);
        m_clearContentsAction = new QAction(this);
        m_clearContentsAction->setText(tr("Clear Contents"));
        m_clearContentsAction->setEnabled(true);
        connect(m_clearContentsAction, SIGNAL(triggered(bool)),
            parent, SLOT(clearContents()));

        m_saveContentsAction = new QAction(this);
        m_saveContentsAction->setText(tr("Save Contents"));
        m_saveContentsAction->setEnabled(true);
        connect(m_saveContentsAction, SIGNAL(triggered()), this, SLOT(saveContents()));
    }

    void contextMenuEvent(QContextMenuEvent *ev)
    {
        QMenu *menu = createStandardContextMenu();
        menu->addAction(m_clearContentsAction);
        menu->addAction(m_saveContentsAction); // X11 clipboard is unreliable for long texts
        menu->addAction(debuggerCore()->action(LogTimeStamps));
        menu->addAction(debuggerCore()->action(VerboseLog));
        menu->addSeparator();
        menu->addAction(debuggerCore()->action(SettingsDialog));
        menu->exec(ev->globalPos());
        delete menu;
    }


private slots:
    void saveContents();

private:
    QAction *m_clearContentsAction;
    QAction *m_saveContentsAction;
};

void DebuggerPane::saveContents()
{
    LogWindow::writeLogContents(this, this);
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
    void clearContentsRequested();
    void statusMessageRequested(const QString &, int);
    void commandSelected(int);

private:
    void keyPressEvent(QKeyEvent *ev)
    {
        if (ev->modifiers() == Qt::ControlModifier && ev->key() == Qt::Key_Return)
            debuggerCore()->executeDebuggerCommand(textCursor().block().text());
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
        QString needle = QString::number(i) + '^';
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
    setObjectName("Log");

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
    m_commandEdit = new QLineEdit(this);
    m_commandEdit->setFrame(false);
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

    setMinimumHeight(60);
}

void LogWindow::sendCommand()
{
    debuggerCore()->executeDebuggerCommand(m_commandEdit->text());
}

void LogWindow::showOutput(int channel, const QString &output)
{
    if (output.isEmpty())
        return;
    QTextCursor oldCursor = m_combinedText->textCursor();
    QTextCursor cursor = oldCursor;
    cursor.movePosition(QTextCursor::End);
    bool atEnd = oldCursor.position() == cursor.position();

    if (debuggerCore()->boolSetting(LogTimeStamps))
        m_combinedText->appendPlainText(charForChannel(LogTime) + logTimeStamp());
    foreach (QString line, output.split('\n')) {
        // FIXME: QTextEdit asserts on really long lines...
        const int n = 30000;
        if (line.size() > n) {
            line.truncate(n);
            line += QLatin1String(" [...] <cut off>");
        }
        if (line != QLatin1String("(gdb) "))
            m_combinedText->appendPlainText(charForChannel(channel) + line);
    }
    cursor.movePosition(QTextCursor::End);
    if (atEnd) {
        m_combinedText->setTextCursor(cursor);
        m_combinedText->ensureCursorVisible();
    }
}

void LogWindow::showInput(int channel, const QString &input)
{
    Q_UNUSED(channel)
    if (debuggerCore()->boolSetting(LogTimeStamps))
        m_inputText->appendPlainText(logTimeStamp());
    m_inputText->appendPlainText(input);
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

} // namespace Internal
} // namespace Debugger

#include "logwindow.moc"
