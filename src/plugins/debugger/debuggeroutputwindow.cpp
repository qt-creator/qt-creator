/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "debuggeroutputwindow.h"
#include "debuggeractions.h"
#include "debuggermanager.h"

#include <QtCore/QDebug>

#include <QtGui/QAction>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMenu>
#include <QtGui/QSpacerItem>
#include <QtGui/QSplitter>
#include <QtGui/QSyntaxHighlighter>
#include <QtGui/QTextBlock>

#include <aggregation/aggregate.h>
#include <coreplugin/findplaceholder.h>
#include <find/basetextfind.h>

using namespace Debugger;
using namespace Debugger::Internal;
using namespace Find;

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
        m_clearContentsAction = new QAction(this);
        m_clearContentsAction->setText(tr("Clear contents"));
        m_clearContentsAction->setEnabled(true);
        connect(m_clearContentsAction, SIGNAL(triggered(bool)),
            parent, SLOT(clearContents()));

        m_saveContentsAction = new QAction(this);
        m_saveContentsAction->setText(tr("Save contents"));
        m_saveContentsAction->setEnabled(true);
    }

    void contextMenuEvent(QContextMenuEvent *ev)
    {
        QMenu *menu = createStandardContextMenu();
        menu->addAction(m_clearContentsAction);
        //menu->addAction(m_saveContentsAction);
        addContextActions(menu);
        theDebuggerAction(ExecuteCommand)->setData(textCursor().block().text());
        menu->addAction(theDebuggerAction(ExecuteCommand));
        menu->addAction(theDebuggerAction(LogTimeStamps));
        menu->addAction(theDebuggerAction(VerboseLog));
        menu->addSeparator();
        menu->addAction(theDebuggerAction(SettingsDialog));
        menu->exec(ev->globalPos());
        delete menu;
    }

    virtual void addContextActions(QMenu *) {}

public:
    QAction *m_clearContentsAction;
    QAction *m_saveContentsAction;
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
            theDebuggerAction(ExecuteCommand)->trigger(textCursor().block().text());
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

    void addContextActions(QMenu *menu)
    {
       menu->addAction(theDebuggerAction(ExecuteCommand));
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

DebuggerOutputWindow::DebuggerOutputWindow(QWidget *parent)
  : QWidget(parent)
{
    setWindowTitle(tr("Debugger"));

    QSplitter *m_splitter = new QSplitter(Qt::Horizontal, this);
    // mixed input/output
    m_combinedText = new CombinedPane(this);
    m_combinedText->setReadOnly(true);
    m_combinedText->setReadOnly(false);
    m_combinedText->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    // input only
    m_inputText = new InputPane(this);
    m_inputText->setReadOnly(false);
    m_inputText->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    m_splitter->addWidget(m_inputText);
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
    aggregate->add(new BaseTextFind(m_combinedText));

    aggregate = new Aggregation::Aggregate;
    aggregate->add(m_inputText);
    aggregate->add(new BaseTextFind(m_inputText));

    connect(m_inputText, SIGNAL(statusMessageRequested(QString,int)),
       this, SIGNAL(statusMessageRequested(QString,int)));
    connect(m_inputText, SIGNAL(commandSelected(int)),
       m_combinedText, SLOT(gotoResult(int)));
};

void DebuggerOutputWindow::showOutput(int channel, const QString &output)
{
    if (output.isEmpty())
        return;
    QTextCursor oldCursor = m_combinedText->textCursor();
    QTextCursor cursor = oldCursor;
    cursor.movePosition(QTextCursor::End);
    bool atEnd = oldCursor.position() == cursor.position();
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

void DebuggerOutputWindow::showInput(int channel, const QString &input)
{
    Q_UNUSED(channel)
    m_inputText->appendPlainText(input);
    QTextCursor cursor = m_inputText->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_inputText->setTextCursor(cursor);
    m_inputText->ensureCursorVisible();
    showOutput(LogInput, input);
}

void DebuggerOutputWindow::clearContents()
{
    m_combinedText->clear();
    m_inputText->clear();
}

void DebuggerOutputWindow::setCursor(const QCursor &cursor)
{
    m_combinedText->setCursor(cursor);
    m_inputText->setCursor(cursor);
    QWidget::setCursor(cursor);
}

QString DebuggerOutputWindow::combinedContents() const
{
    return m_combinedText->toPlainText();
}

QString DebuggerOutputWindow::inputContents() const
{
    return m_inputText->toPlainText();
}

#include "debuggeroutputwindow.moc"
