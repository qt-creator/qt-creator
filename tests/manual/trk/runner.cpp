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

#include "trkgdbadapter.h"
#include "trkoptions.h"

#include <QtCore/QDebug>

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QMainWindow>
#include <QtGui/QKeyEvent>
#include <QtGui/QTextBlock>
#include <QtGui/QTextEdit>
#include <QtGui/QToolBar>
#include <QtCore/QSharedPointer>
#include <QtCore/QTimer>
#include <QtCore/QDir>

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
    void handleOutput(const QString &str0)
    {
        QString str = str0;
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
    RunnerGui(TrkGdbAdapter *adapter);

private slots:
    void executeStepICommand() { executeCommand("-exec-step-instruction"); }
    void executeStepCommand() { executeCommand("-exec-step"); }
    void executeNextICommand() { executeCommand("-exec-next-instruction"); }
    void executeNextCommand() { executeCommand("-exec-next"); }
    void executeContinueCommand() { executeCommand("-exec-continue"); }
    void executeDisassICommand() { executeCommand("disass $pc $pc+4"); }
    void executeStopCommand() { executeCommand("I"); }

    void handleReadyReadStandardError();
    void handleReadyReadStandardOutput();

    void run();
    void started();

private:
    void executeCommand(const QString &cmd) { m_adapter->executeCommand(cmd); }
    void connectAction(QAction *&, QString name, const char *slot);

    TrkGdbAdapter *m_adapter;
    TextEdit m_textEdit;
    QToolBar m_toolBar;
    QAction *m_stopAction;
    QAction *m_stepIAction;
    QAction *m_stepAction;
    QAction *m_nextIAction;
    QAction *m_nextAction;
    QAction *m_disassIAction;
    QAction *m_continueAction;
};

RunnerGui::RunnerGui(TrkGdbAdapter *adapter)
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
    connectAction(m_stopAction, "Stop", SLOT(executeStopCommand()));

    connect(adapter, SIGNAL(output(QString)),
        &m_textEdit, SLOT(handleOutput(QString)));
    connect(&m_textEdit, SIGNAL(executeCommand(QString)),
        m_adapter, SLOT(executeCommand(QString)));

    connect(adapter, SIGNAL(readyReadStandardError()),
        this, SLOT(handleReadyReadStandardError()));
    connect(adapter, SIGNAL(readyReadStandardOutput()),
        this, SLOT(handleReadyReadStandardOutput()));
    connect(adapter, SIGNAL(started()),
        this, SLOT(started()));
}

void RunnerGui::connectAction(QAction *&action, QString name, const char *slot)
{
    action = new QAction(this);
    action->setText(name);
    m_toolBar.addAction(action);
    connect(action, SIGNAL(triggered()), this, slot);
}

void RunnerGui::handleReadyReadStandardError()
{
    QByteArray ba = m_adapter->readAllStandardError();
    qDebug() << ba;
    m_textEdit.handleOutput(ba);
}

void RunnerGui::handleReadyReadStandardOutput()
{
    QByteArray ba = m_adapter->readAllStandardOutput();
    qDebug() << ba;
    m_textEdit.handleOutput("-> GDB: " + ba);
}

void RunnerGui::run()
{
    m_adapter->run();
}

void RunnerGui::started()
{
    qDebug() << "\nSTARTED\n";
    executeCommand("set confirm off"); // confirm potentially dangerous operations?
    executeCommand("set endian little");
    executeCommand("set remotebreak on");
    executeCommand("set breakpoint pending on");
    executeCommand("set trust-readonly-sections on");
    //executeCommand("mem 0 ~0ll rw 8 cache");

    // FIXME: "remote noack" does not seem to be supported on cs-gdb?
    //executeCommand("set remote noack-packet");

    // FIXME: creates a lot of noise a la  '&"putpkt: Junk: Ack " &'
    // even though the communication seems sane
    //executeCommand("set debug remote 1"); // creates l

    executeCommand("add-symbol-file filebrowseapp.sym "
        + trk::hexxNumber(m_adapter->session().codeseg));
    executeCommand("symbol-file filebrowseapp.sym");

    //executeCommand("info address CFileBrowseAppUi::HandleCommandL",
    //    GdbCB(handleInfoMainAddress));

    executeCommand("-break-insert filebrowseappui.cpp:39");
    executeCommand("target remote " + m_adapter->gdbServerName());
    executeCommand("-exec-continue");
}

///////////////////////////////////////////////////////////////////////
//
// main
//
///////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QSharedPointer<TrkOptions> options(new TrkOptions);
    options->gdb = QDir::currentPath() + QLatin1String("/cs-gdb");
    TrkGdbAdapter adapter(options);
    adapter.setVerbose(2);
    RunnerGui gui(&adapter);
    gui.show();
    QTimer::singleShot(0, &gui, SLOT(run()));
    return app.exec();
}

#include "runner.moc"
