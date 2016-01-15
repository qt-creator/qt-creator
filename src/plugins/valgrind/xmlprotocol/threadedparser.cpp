/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
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
    connect(parser, &Parser::status,
            this, &ThreadedParser::status,
            Qt::QueuedConnection);
    connect(parser, &Parser::error,
            this, &ThreadedParser::error,
            Qt::QueuedConnection);
    connect(parser, &Parser::internalError,
            this, &ThreadedParser::slotInternalError,
            Qt::QueuedConnection);
    connect(parser, &Parser::errorCount,
            this, &ThreadedParser::errorCount,
            Qt::QueuedConnection);
    connect(parser, &Parser::suppressionCount,
            this, &ThreadedParser::suppressionCount,
            Qt::QueuedConnection);
    connect(parser, &Parser::finished,
            this, &ThreadedParser::finished,
            Qt::QueuedConnection);


    Thread *thread = new Thread;
    d->parserThread = thread;
    connect(thread, &QThread::finished,
            thread, &QObject::deleteLater);
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
