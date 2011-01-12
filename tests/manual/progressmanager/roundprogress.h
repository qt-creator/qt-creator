/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
