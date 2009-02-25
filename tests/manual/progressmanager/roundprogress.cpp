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
