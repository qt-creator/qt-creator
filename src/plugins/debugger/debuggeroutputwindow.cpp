/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "debuggeroutputwindow.h"

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
#include <QtGui/QTextBlock>

#ifndef GDBDEBUGGERLEAN

#include <aggregation/aggregate.h>
#include <find/basetextfind.h>

using namespace Find;

#endif // GDBDEBUGGERLEAN

using Debugger::Internal::DebuggerOutputWindow;

/////////////////////////////////////////////////////////////////////
//
// InputPane
//
/////////////////////////////////////////////////////////////////////

class DebuggerPane : public QTextEdit
{
public:
    DebuggerPane(QWidget *parent)
        : QTextEdit(parent)
    {
        m_clearContentsAction = new QAction(this);
        m_clearContentsAction->setText("Clear contents");
        m_clearContentsAction->setEnabled(true);
        m_clearContentsAction->setShortcut(Qt::ControlModifier + Qt::Key_R);
        connect(m_clearContentsAction, SIGNAL(triggered(bool)),
            parent, SLOT(clearContents()));

        m_saveContentsAction = new QAction(this);
        m_saveContentsAction->setText("Save contents");
        m_saveContentsAction->setEnabled(true);
    }

    void contextMenuEvent(QContextMenuEvent *ev)
    {
        QMenu *menu = createStandardContextMenu();
        menu->addAction(m_clearContentsAction);
        //menu->addAction(m_saveContentsAction);
        addContextActions(menu);
        menu->exec(ev->globalPos());
        delete menu;
    }

    virtual void addContextActions(QMenu *) {}

public:
    QAction *m_clearContentsAction;
    QAction *m_saveContentsAction;
};

class InputPane : public DebuggerPane
{
    Q_OBJECT
public:
    InputPane(QWidget *parent) : DebuggerPane(parent)
    {
        m_commandExecutionAction = new QAction(this);
        m_commandExecutionAction->setText("Execute line");
        m_commandExecutionAction->setEnabled(true);
        //m_commandExecutionAction->setShortcut
       //     (Qt::ControlModifier + Qt::Key_Return);

        connect(m_commandExecutionAction, SIGNAL(triggered(bool)),
            this, SLOT(executeCommand()));
    }

signals:
    void commandExecutionRequested(const QString &);
    void clearContentsRequested();
    void statusMessageRequested(const QString &, int);
    void commandSelected(int);

private slots:
    void executeCommand()
    {
        emit commandExecutionRequested(textCursor().block().text());
    }

private:
    void keyPressEvent(QKeyEvent *ev)
    {
        if (ev->modifiers() == Qt::ControlModifier && ev->key() == Qt::Key_Return)
            emit commandExecutionRequested(textCursor().block().text());
        else if (ev->modifiers() == Qt::ControlModifier && ev->key() == Qt::Key_R)
            emit clearContentsRequested();
        else
            QTextEdit::keyPressEvent(ev);
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
       menu->addAction(m_commandExecutionAction);
    }
    
    void focusInEvent(QFocusEvent *ev)  
    {
        emit statusMessageRequested("Type Ctrl-<Return> to execute a line.", -1);
        QTextEdit::focusInEvent(ev);
    }

    void focusOutEvent(QFocusEvent *ev)  
    {
        emit statusMessageRequested(QString(), -1);
        QTextEdit::focusOutEvent(ev);
    }

    QAction *m_commandExecutionAction;
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
    {}

public slots:
    void gotoResult(int i)
    {   
        QString needle = QString::number(i) + '^';
        QString needle2 = "stdout:" + needle;
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
    setWindowTitle(tr("Gdb"));

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

    QGridLayout *layout = new QGridLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_splitter);
    setLayout(layout);

#ifndef GDBDEBUGGERLEAN
    Aggregation::Aggregate *aggregate = new Aggregation::Aggregate;
    aggregate->add(m_combinedText);
    aggregate->add(new BaseTextFind(m_combinedText));

    aggregate = new Aggregation::Aggregate;
    aggregate->add(m_inputText);
    aggregate->add(new BaseTextFind(m_inputText));
#endif

    connect(m_inputText, SIGNAL(commandExecutionRequested(QString)),
       this, SIGNAL(commandExecutionRequested(QString)));
    connect(m_inputText, SIGNAL(statusMessageRequested(QString,int)),
       this, SIGNAL(statusMessageRequested(QString,int)));
    connect(m_inputText, SIGNAL(commandSelected(int)),
       m_combinedText, SLOT(gotoResult(int)));
};

void DebuggerOutputWindow::onReturnPressed()
{
    emit commandExecutionRequested(m_commandEdit->text());
}

void DebuggerOutputWindow::showOutput(const QString &prefix, const QString &output)
{
    if (output.isEmpty())
        return;
    foreach (QString line, output.split("\n")) {
        // FIXME: QTextEdit asserts on really long lines...
        const int n = 3000;
        if (line.size() > n)
            line = line.left(n) + " [...] <cut off>";
        m_combinedText->append(prefix + line);
    }
    QTextCursor cursor = m_combinedText->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_combinedText->setTextCursor(cursor);
    m_combinedText->ensureCursorVisible();
}

void DebuggerOutputWindow::showInput(const QString &prefix, const QString &input)
{
    Q_UNUSED(prefix);
    m_inputText->append(input);
    QTextCursor cursor = m_inputText->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_inputText->setTextCursor(cursor);
    m_inputText->ensureCursorVisible();
    showOutput("input:", input);
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
