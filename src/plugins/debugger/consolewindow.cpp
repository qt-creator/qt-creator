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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "consolewindow.h"
#include "logwindow.h"

#include "debuggeractions.h"
#include "debuggercore.h"

#include <QtCore/QDebug>

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QKeyEvent>
#include <QtGui/QMenu>
#include <QtGui/QSyntaxHighlighter>
#include <QtGui/QPlainTextEdit>

#include <aggregation/aggregate.h>
#include <coreplugin/findplaceholder.h>
#include <find/basetextfind.h>

#include <utils/savedaction.h>

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

static int channelForChar(QChar c)
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
// ConsoleHighlighter
//
/////////////////////////////////////////////////////////////////////

class ConsoleHighlighter : public QSyntaxHighlighter
{
public:
    ConsoleHighlighter(QPlainTextEdit *parent)
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
/*
        if (text.size() > 3 && text.at(2) == QLatin1Char(':')) {
            QTextCharFormat format;
            format.setForeground(Qt::darkRed);
            setFormat(1, text.size(), format);
        }
*/
    }

    QPlainTextEdit *m_parent;
};

/////////////////////////////////////////////////////////////////////
//
// DebbuggerPane base class
//
/////////////////////////////////////////////////////////////////////

// FIXME: Code duplication with FakeVim
class History
{
public:
    History() : m_index(0) {}
    void append(const QString &item) {
        m_items.removeAll(item);
        m_items.append(item); m_index = m_items.size() - 1;
    }
    void down() { m_index = qMin(m_index + 1, m_items.size()); }
    void up() { m_index = qMax(m_index - 1, 0); }
    //void clear() { m_items.clear(); m_index = 0; }
    void restart() { m_index = m_items.size(); }
    QString current() const { return m_items.value(m_index, QString()); }
    QStringList items() const { return m_items; }
private:
    QStringList m_items;
    int m_index;
};

class Console : public QPlainTextEdit
{
    Q_OBJECT

public:
    Console(QWidget *parent)
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
        debuggerCore()->executeDebuggerCommand(textCursor().block().text());
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

    void keyPressEvent(QKeyEvent *ev)
    {
        if (ev->key() == Qt::Key_Return) {
            if (ev->modifiers() == 0) {
                QString cmd = textCursor().block().text();
                if (cmd.isEmpty())
                    cmd = m_history.current();
                QString cleanCmd;
                foreach (QChar c, cmd)
                    if (c.unicode() >= 32 && c.unicode() < 128)
                        cleanCmd.append(c);
                if (!cleanCmd.isEmpty()) {
                    debuggerCore()->executeDebuggerCommand(cleanCmd);
                    m_history.append(cleanCmd);
                }
            }
            QPlainTextEdit::keyPressEvent(ev);
        } else if (ev->key() == Qt::Key_Up) {
            m_history.up();
        } else if (ev->key() == Qt::Key_Down) {
            m_history.down();
        } else {
            QPlainTextEdit::keyPressEvent(ev);
        }
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
        //emit commandSelected(n);
    }


private slots:
    void saveContents();

private:
    QAction *m_clearContentsAction;
    QAction *m_saveContentsAction;
    History m_history;
};

void Console::saveContents()
{
    LogWindow::writeLogContents(this, this);
}

/////////////////////////////////////////////////////////////////////
//
// ConsoleWindow
//
/////////////////////////////////////////////////////////////////////

ConsoleWindow::ConsoleWindow(QWidget *parent)
  : QWidget(parent)
{
    setWindowTitle(tr("Console"));
    setObjectName("Console");

    m_console = new Console(this);
    m_console->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_console);
    layout->addWidget(new Core::FindToolBarPlaceHolder(this));
    setLayout(layout);

    Aggregation::Aggregate *aggregate = new Aggregation::Aggregate;
    aggregate->add(m_console);
    aggregate->add(new Find::BaseTextFind(m_console));

    //connect(m_console, SIGNAL(statusMessageRequested(QString,int)),
    //   this, SIGNAL(statusMessageRequested(QString,int)));
}

void ConsoleWindow::showOutput(int channel, const QString &output)
{
    if (output.isEmpty())
        return;
    //QTextCursor oldCursor = m_console->textCursor();
    //QTextCursor cursor = oldCursor;
    //cursor.movePosition(QTextCursor::End);
    //bool atEnd = oldCursor.position() == cursor.position();

    foreach (QString line, output.split('\n')) {
        // FIXME: QTextEdit asserts on really long lines...
        const int n = 30000;
        if (line.size() > n) {
            line.truncate(n);
            line += QLatin1String(" [...] <cut off>");
        }
        m_console->appendPlainText(charForChannel(channel) + line + '\n');
    }
    QTextCursor cursor = m_console->textCursor();
    cursor.movePosition(QTextCursor::End);
    //if (atEnd) {
        m_console->setTextCursor(cursor);
        m_console->ensureCursorVisible();
    //}
}

void ConsoleWindow::showInput(int channel, const QString &input)
{
    Q_UNUSED(channel)
    m_console->appendPlainText(input);
    QTextCursor cursor = m_console->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_console->setTextCursor(cursor);
    m_console->ensureCursorVisible();
}

void ConsoleWindow::clearContents()
{
    m_console->clear();
}

void ConsoleWindow::setCursor(const QCursor &cursor)
{
    m_console->viewport()->setCursor(cursor);
    QWidget::setCursor(cursor);
}

QString ConsoleWindow::combinedContents() const
{
    return m_console->toPlainText();
}

QString ConsoleWindow::inputContents() const
{
    return m_console->toPlainText();
}

} // namespace Internal
} // namespace Debugger

#include "consolewindow.moc"
