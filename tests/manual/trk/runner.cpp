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

#include "symbianadapter.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QPointer>
#include <QtCore/QProcess>
#include <QtCore/QQueue>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QMainWindow>
#include <QtGui/QKeyEvent>
#include <QtGui/QTextBlock>
#include <QtGui/QTextEdit>
#include <QtGui/QToolBar>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>

/*
fetch-register          p                       info registers
set-register            P                       set
binary-download         X                       load, set
read-aux-vector         qXfer:auxv:read         info auxv
symbol-lookup           qSymbol                 Detecting multiple threads
attach                  vAttach                 attach
verbose-resume          vCont                   Stepping or resuming multiple threads
run                     vRun                    run
software-breakpoint     Z0                      break
hardware-breakpoint     Z1                      hbreak
write-watchpoint        Z2                      watch
read-watchpoint         Z3                      rwatch
access-watchpoint       Z4                      awatch
target-features         qXfer:features:read     set architecture
library-info            qXfer:libraries:read    info sharedlibrary
memory-map              qXfer:memory-map:read   info mem
read-spu-object         qXfer:spu:read          info spu
write-spu-object        qXfer:spu:write         info spu
get-thread-local-
storage-address         qGetTLSAddr             Displaying __thread variables
supported-packets       qSupported              Remote communications parameters
pass-signals            QPassSignals            handle signal
hostio-close-packet     vFile:close             remote get, remote put
hostio-open-packet      vFile:open              remote get, remote put
hostio-pread-packet     vFile:pread             remote get, remote put
hostio-pwrite-packet    vFile:pwrite            remote get, remote put
hostio-unlink-packet    vFile:unlink            remote delete
*/

using namespace trk;

enum { KnownRegisters = RegisterPSGdb + 1};

static const char *registerNames[KnownRegisters] =
{
    "A1", "A2", "A3", "A4",
    0, 0, 0, 0,
    0, 0, 0, "AP",
    "IP", "SP", "LR", "PC",
    "PSTrk", 0, 0, 0,
    0, 0, 0, 0,
    0, "PSGdb"
};

static QByteArray dumpRegister(int n, uint value)
{
    QByteArray ba;
    ba += ' ';
    if (n < KnownRegisters && registerNames[n]) {
        ba += registerNames[n];
    } else {
        ba += '#';
        ba += QByteArray::number(n);
    }
    ba += "=" + hexxNumber(value);
    return ba;
}


///////////////////////////////////////////////////////////////////////
//
// RunnerGui
//
///////////////////////////////////////////////////////////////////////

class TextEdit : public QTextEdit
{
    Q_OBJECT

signals:
    void executeCommand(QString);

public slots:
    void handleOutput(const QString &senderName, const QString &data)
    {
        QString str = senderName + data;
        str.replace("\\t", QString(QChar(0x09)));
        str.replace("\\n", QString("\n"));
        append(str);

        QTextCursor tc = textCursor();
        tc.movePosition(QTextCursor::End);
        setTextCursor(tc);
    /*
        int pos1 = str.indexOf("#");
        int pos2 = str.indexOf(")", pos1);
        if (pos1 != -1 && pos2 != -1)
            str = str.left(pos1) + "<b>" + str.mid(pos1, pos2 - pos1 + 1)
                + "</b> " + str.mid(pos2 + 1);
        insertHtml(str + "\n");
    */
        setCurrentCharFormat(QTextCharFormat());
        ensureCursorVisible();
    }

    void keyPressEvent(QKeyEvent *ev)
    {
        if (ev->modifiers() == Qt::ControlModifier && ev->key() == Qt::Key_Return)
            emit executeCommand(textCursor().block().text());
        else
            QTextEdit::keyPressEvent(ev);
    }
};

///////////////////////////////////////////////////////////////////////
//
// RunnerGui
//
///////////////////////////////////////////////////////////////////////

using namespace Debugger::Internal;

class RunnerGui : public QMainWindow
{
    Q_OBJECT

public:
    RunnerGui(SymbianAdapter *adapter);

private slots:
    void executeStepICommand() { executeCommand("-exec-step-instruction"); }
    void executeStepCommand() { executeCommand("-exec-step"); }
    void executeNextICommand() { executeCommand("-exec-next-instruction"); }
    void executeNextCommand() { executeCommand("-exec-next"); }
    void executeContinueCommand() { executeCommand("-exec-continue"); }
    void executeDisassICommand() { executeCommand("disass $pc $pc+4"); }

private:
    void executeCommand(const QString &cmd) { m_adapter->executeCommand(cmd); }
    void connectAction(QAction *&, QString name, const char *slot);

    SymbianAdapter *m_adapter;
    TextEdit m_textEdit;
    QToolBar m_toolBar;
    QAction *m_stepIAction;
    QAction *m_stepAction;
    QAction *m_nextIAction;
    QAction *m_nextAction;
    QAction *m_disassIAction;
    QAction *m_continueAction;
};

RunnerGui::RunnerGui(SymbianAdapter *adapter)
    : m_adapter(adapter)
{
    resize(1200, 1000);
    setCentralWidget(&m_textEdit);

    addToolBar(&m_toolBar);

    connectAction(m_stepIAction, "Step Inst", SLOT(executeStepICommand()));
    connectAction(m_stepAction, "Step", SLOT(executeStepCommand()));
    connectAction(m_nextIAction, "Next Inst", SLOT(executeNextICommand()));
    connectAction(m_nextAction, "Next", SLOT(executeNextCommand()));
    connectAction(m_disassIAction, "Disass Inst", SLOT(executeDisassICommand()));
    connectAction(m_continueAction, "Continue", SLOT(executeContinueCommand()));

    connect(adapter, SIGNAL(output(QString,QString)),
        &m_textEdit, SLOT(handleOutput(QString,QString)));
    connect(&m_textEdit, SIGNAL(executeCommand(QString)),
        m_adapter, SLOT(executeCommand(QString)));
}

void RunnerGui::connectAction(QAction *&action, QString name, const char *slot)
{
    action = new QAction(this);
    action->setText(name);
    m_toolBar.addAction(action);
    connect(action, SIGNAL(triggered()), this, slot);
}

///////////////////////////////////////////////////////////////////////
//
// main
//
///////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    SymbianAdapter adapter;
    RunnerGui gui(&adapter);
    gui.show();
    QTimer::singleShot(0, &adapter, SLOT(run()));
    return app.exec();
}

#include "runner.moc"
