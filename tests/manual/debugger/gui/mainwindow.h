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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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
#endif // MAINWINDOW_H
