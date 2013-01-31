/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
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

#include "threadedparser.h"
#include "parser.h"
#include "error.h"
#include "frame.h"
#include "status.h"
#include "suppression.h"
#include <utils/qtcassert.h>

#include <QIODevice>
#include <QMetaType>
#include <QThread>
#include <QPointer>

namespace {

class Thread : public QThread
{
public:
    Thread() : parser(0) , device(0) {}

    void run()
    {
        QTC_ASSERT(QThread::currentThread() == this, return);
        parser->parse(device);
        delete parser;
        parser = 0;
        delete device;
        device = 0;
    }

    Valgrind::XmlProtocol::Parser *parser;
    QIODevice *device;
};

} // namespace anon


namespace Valgrind {
namespace XmlProtocol {

class ThreadedParser::Private
{
public:
    Private()
    {}

    QPointer<Thread> parserThread;
    QString errorString;
};


ThreadedParser::ThreadedParser(QObject *parent)
    : QObject(parent),
      d(new Private)
{
}

ThreadedParser::~ThreadedParser()
{
    delete d;
}

QString ThreadedParser::errorString() const
{
    return d->errorString;
}

bool ThreadedParser::isRunning() const
{
    return d->parserThread ? d->parserThread.data()->isRunning() : 0;
}

void ThreadedParser::parse(QIODevice *device)
{
    QTC_ASSERT(!d->parserThread, return);

    Parser *parser = new Parser;
    qRegisterMetaType<Valgrind::XmlProtocol::Status>();
    qRegisterMetaType<Valgrind::XmlProtocol::Error>();
    connect(parser, SIGNAL(status(Valgrind::XmlProtocol::Status)),
            SIGNAL(status(Valgrind::XmlProtocol::Status)),
            Qt::QueuedConnection);
    connect(parser, SIGNAL(error(Valgrind::XmlProtocol::Error)),
            SIGNAL(error(Valgrind::XmlProtocol::Error)),
            Qt::QueuedConnection);
    connect(parser, SIGNAL(internalError(QString)),
            SLOT(slotInternalError(QString)),
            Qt::QueuedConnection);
    connect(parser, SIGNAL(errorCount(qint64,qint64)),
            SIGNAL(errorCount(qint64,qint64)),
            Qt::QueuedConnection);
    connect(parser, SIGNAL(suppressionCount(QString,qint64)),
            SIGNAL(suppressionCount(QString,qint64)),
            Qt::QueuedConnection);
    connect(parser, SIGNAL(finished()), SIGNAL(finished()),
            Qt::QueuedConnection);


    Thread *thread = new Thread;
    d->parserThread = thread;
    connect(thread, SIGNAL(finished()),
            thread, SLOT(deleteLater()));
    device->setParent(0);
    device->moveToThread(thread);
    parser->moveToThread(thread);
    thread->device = device;
    thread->parser = parser;
    thread->start();
}

void ThreadedParser::slotInternalError(const QString &errorString)
{
    d->errorString = errorString;
    emit internalError(errorString);
}

bool ThreadedParser::waitForFinished()
{
    return d->parserThread ? d->parserThread.data()->wait() : true;
}

} // namespace XmlProtocol
} // namespace Valgrind
