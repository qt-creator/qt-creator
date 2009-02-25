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

#ifndef ROUNDPROGRESS_H
#define ROUNDPROGRESS_H

#include "ui_roundprogress.h"

#include <QtCore>
#include <QtGui>
#include <QtDebug>
#include <QtGui/QMainWindow>
#include <QtCore/QFutureInterface>
#include <QtCore/QFuture>

class MyInternalTask : public QObject
{
    Q_OBJECT
public:
    static const int MAX = 10;

    MyInternalTask(bool withProgress)
    {
        m_hasProgress = withProgress;
        if (m_hasProgress)
            m_duration = 4000;
        else
            m_duration = 2500;
    }

    MyInternalTask(const MyInternalTask &) : QObject() {}

    void run(QFutureInterface<void> &f)
    {
        m_future = &f;
        m_count = 0;
        m_future->setProgressRange(0, (m_hasProgress ? MAX : 0));
        m_future->setProgressValueAndText(m_count, tr("Starting..."));
        m_loop = new QEventLoop;
        m_timer = new QTimer;
        m_timer->setInterval(m_duration/MAX);
        m_timer->setSingleShot(false);
        connect(m_timer, SIGNAL(timeout()), this, SLOT(advance()));
        m_timer->start();
        m_loop->exec();
        delete m_timer;
        delete m_loop;
    }

private slots:
    void advance()
    {
        ++m_count;
        m_future->setProgressValueAndText(m_count, tr("Something happening %1").arg(m_count));
        if (m_count == MAX || m_future->isCanceled()) {
            m_future->setProgressValueAndText(m_count, tr("Finished!"));
            m_loop->quit();
        }
    }
private:
    bool m_hasProgress;
    QEventLoop *m_loop;
    QTimer *m_timer;
    QFutureInterface<void> *m_future;
    int m_count;
    int m_duration;
};

class roundprogress : public QWidget
{
    Q_OBJECT

public:
    roundprogress(QWidget *parent = 0);
    ~roundprogress() {}

private slots:
    void start();
    void start2();
    void start3();
private:
    Ui::roundprogressClass ui;
    MyInternalTask task;
    MyInternalTask task2;
    QFuture<void> future;
};

#endif // ROUNDPROGRESS_H
