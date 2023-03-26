// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "threadedparser.h"
#include "parser.h"
#include "error.h"
#include "status.h"

#include <utils/qtcassert.h>

#include <QIODevice>
#include <QMetaType>
#include <QThread>
#include <QPointer>

namespace {

class Thread : public QThread
{
public:
    void run() override
    {
        QTC_ASSERT(QThread::currentThread() == this, return);
        parser->parse(device);
        delete parser;
        parser = nullptr;
        delete device;
        device = nullptr;
    }

    Valgrind::XmlProtocol::Parser *parser = nullptr;
    QIODevice *device = nullptr;
};

} // namespace anon


namespace Valgrind {
namespace XmlProtocol {

class ThreadedParser::Private
{
public:
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
    return d->parserThread ? d->parserThread.data()->isRunning() : false;
}

void ThreadedParser::parse(QIODevice *device)
{
    QTC_ASSERT(!d->parserThread, return);

    auto parser = new Parser;
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


    auto thread = new Thread;
    d->parserThread = thread;
    connect(thread, &QThread::finished,
            thread, &QObject::deleteLater);
    device->setParent(nullptr);
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
