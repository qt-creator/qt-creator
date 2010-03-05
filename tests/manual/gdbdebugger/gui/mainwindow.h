/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>

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
