// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QThread;

namespace Ui{
    class MainWindowClass;
}
QT_END_NAMESPACE

namespace Foo {
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindowClass *ui;
    void simpleBP(int c, const QString &x);
    void complexBP(int *c, QString x);

private slots:
    void on_actionLongString_triggered();
    void on_actionScopes_triggered();
    void on_actionAssert_triggered();
    void on_actionForeach_triggered();
    void on_actionExtTypes_triggered();
    void on_actionDumperBP_triggered();
    void on_actionUncaughtException_triggered();
    void on_actionException_triggered();
    void on_actionThread_triggered();
    void on_actionIncr_watch_triggered();
    void on_actionDialog_triggered();
    void on_actionCrash_triggered();
    void on_actionSimpleBP_triggered();

    void on_actionStdTypes_triggered();

    void on_actionVariousQtTypes_triggered();

private:
    void terminateThread();
    int m_w;
    const char *m_wc;
    QString m_wqs;
    QThread *m_thread1, *m_thread2;
    QThread *m_fastThread;
};

}
