// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "threadedparser.h"

#include "error.h"
#include "parser.h"
#include "status.h"

#include <utils/qtcassert.h>

#include <QIODevice>
#include <QMetaType>
#include <QThread>

namespace Valgrind::XmlProtocol {

class Thread : public QThread
{
public:
    void run() override
    {
        QTC_ASSERT(QThread::currentThread() == this, return);
        m_parser->parse(m_device);
        delete m_parser;
        m_parser = nullptr;
        delete m_device;
        m_device = nullptr;
    }

    Valgrind::XmlProtocol::Parser *m_parser = nullptr;
    QIODevice *m_device = nullptr;
};

ThreadedParser::ThreadedParser(QObject *parent)
    : QObject(parent)
{}

bool ThreadedParser::isRunning() const
{
    return m_parserThread ? m_parserThread->isRunning() : false;
}

void ThreadedParser::setIODevice(QIODevice *device)
{
    QTC_ASSERT(device, return);
    QTC_ASSERT(device->isOpen(), return);
    m_device.reset(device);
}

void ThreadedParser::start()
{
    QTC_ASSERT(!m_parserThread, return);
    QTC_ASSERT(m_device, return);

    auto parser = new Parser;
    qRegisterMetaType<Status>();
    qRegisterMetaType<Error>();
    connect(parser, &Parser::status, this, &ThreadedParser::status, Qt::QueuedConnection);
    connect(parser, &Parser::error, this, &ThreadedParser::error, Qt::QueuedConnection);
    connect(parser, &Parser::done, this, &ThreadedParser::done, Qt::QueuedConnection);

    m_parserThread = new Thread;
    connect(m_parserThread.get(), &QThread::finished, m_parserThread.get(), &QObject::deleteLater);
    m_device->setParent(nullptr);
    m_device->moveToThread(m_parserThread);
    parser->moveToThread(m_parserThread);
    m_parserThread->m_device = m_device.release();
    m_parserThread->m_parser = parser;
    m_parserThread->start();
}

bool ThreadedParser::waitForFinished()
{
    return m_parserThread ? m_parserThread->wait() : true;
}

} // namespace Valgrind::XmlProtocol
