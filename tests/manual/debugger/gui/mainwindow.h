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
