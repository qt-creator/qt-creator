// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "connectionmanagerinterface.h"
#include "nodeinstancetracing.h"

#include <QLocalSocket>
#include <QLocalServer>
#include <QTimer>

namespace QmlDesigner {

using NanotraceHR::keyValue;
using NodeInstanceTracing::category;

void convertToString(NanotraceHR::ArgumentsString &string,
                     const ConnectionManagerInterface::Connection &connection)
{
    using NanotraceHR::dictionary;
    using NanotraceHR::keyValue;
    auto dict = dictionary(keyValue("name", connection.name), keyValue("mode", connection.mode));
    convertToString(string, dict);
}

ConnectionManagerInterface::~ConnectionManagerInterface()
{
    NanotraceHR::Tracer tracer{"connection manager interface destructor", category()};
}

ConnectionManagerInterface::Connection::~Connection()

{
    NanotraceHR::Tracer tracer{"connection manager interface connection destructor", category()};
}

ConnectionManagerInterface::Connection::Connection(const QString &name, const QString &mode)
    : name{name}
    , mode{mode}
{
    NanotraceHR::Tracer tracer{"connection manager interface connection constructor", category()};
}

ConnectionManagerInterface::Connection::Connection(Connection &&)
{
    NanotraceHR::Tracer tracer{"connection manager interface connection move constructor", category()};
}

void ConnectionManagerInterface::Connection::clear()
{
    NanotraceHR::Tracer tracer{"connection manager interface connection clear", category()};

    qmlPuppetProcess.reset();
    socket.reset();
    localServer.reset();
    blockSize = 0;
    lastReadCommandCounter = 0;
    timer.reset();
}

} // namespace QmlDesigner
