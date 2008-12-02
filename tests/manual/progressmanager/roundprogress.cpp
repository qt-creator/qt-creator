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

#include "roundprogress.h"
#include "multitask.h"
#include "runextensions.h"

roundprogress::roundprogress(QWidget *parent)
    : QWidget(parent), task(MyInternalTask(true)), task2(MyInternalTask(false))
{
    ui.setupUi(this);
    ui.progressButton->setIcon(QIcon("find.png"));
    connect(ui.startButton, SIGNAL(clicked()), this, SLOT(start()));
    connect(ui.startButton2, SIGNAL(clicked()), this, SLOT(start2()));
    connect(ui.startBothButton, SIGNAL(clicked()), this, SLOT(start3()));
    ui.startButton->setFocus();
}

void roundprogress::start()
{
    if (future.isRunning())
        return;
    future = QtConcurrent::run(&MyInternalTask::run, &task);
    ui.progressButton->setFuture(future);
}

void roundprogress::start2()
{
    if (future.isRunning())
        return;
    future = QtConcurrent::run(&MyInternalTask::run, &task2);
    ui.progressButton->setFuture(future);
}

void roundprogress::start3()
{
    if (future.isRunning())
        return;
    future = QtConcurrent::run(&MyInternalTask::run, QList<MyInternalTask*>() << &task2 << &task);
    ui.progressButton->setFuture(future);
}
