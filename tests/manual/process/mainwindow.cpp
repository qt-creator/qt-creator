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
*************************************************************************/

#include "mainwindow.h"

#include <utils/synchronousprocess.h>

#include <QtGui/QPlainTextEdit>
#include <QtGui/QApplication>
#include <QtCore/QDebug>
#include <QtCore/QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_logWindow(new QPlainTextEdit)
{
    setCentralWidget(m_logWindow);
    QTimer::singleShot(200, this, SLOT(test()));
}

void MainWindow::append(const QByteArray &a)
{
    m_logWindow->appendPlainText(QString::fromLocal8Bit(a));
}

void MainWindow::test()
{
    QStringList args = QApplication::arguments();
    args.pop_front();
    const QString cmd = args.front();
    args.pop_front();
    Utils::SynchronousProcess process;
    process.setTimeout(2000);
    qDebug() << "Async: " << cmd << args;
    connect(&process, SIGNAL(stdOut(QByteArray,bool)), this, SLOT(append(QByteArray)));
    connect(&process, SIGNAL(stdErr(QByteArray,bool)), this, SLOT(append(QByteArray)));
    const Utils::SynchronousProcessResponse resp = process.run(cmd, args);
    qDebug() << resp;
}
