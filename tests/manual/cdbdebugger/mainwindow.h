#pragma once

#include "ui_mainwindow.h"
#include "debugger.h"

class MainWindow : public QMainWindow, private Ui_MainWindow
{
    Q_OBJECT
public:
    MainWindow();

    void setDebuggee(const QString& filename);

private slots:
    void on_actionOpen_triggered();
    void on_actionExit_triggered();
    void on_actionBreak_triggered();
    void on_actionRun_triggered();
    void on_lstStack_itemClicked(QListWidgetItem*);
    void appendOutput(const QString&);
    void onDebuggeePaused();

private:
    Debugger    m_debugger;
    Debugger::StackTrace m_stackTrace;
};