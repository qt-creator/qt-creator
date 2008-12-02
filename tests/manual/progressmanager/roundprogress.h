/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#ifndef ROUNDPROGRESS_H
#define ROUNDPROGRESS_H

#include <QtGui/QMainWindow>
#include "ui_roundprogress.h"
#include <QtCore>
#include <QtGui>
#include <QtDebug>
#include <QtCore/QFutureInterface>
#include <QtCore/QFuture>

class MyInternalTask;

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
