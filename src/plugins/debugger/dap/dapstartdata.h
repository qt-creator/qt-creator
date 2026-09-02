// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../debuggerengineinterface.h"

#include <utils/commandline.h>
#include <utils/filepath.h>
#include <utils/processinterface.h>

#include <QJsonObject>
#include <QString>
#include <QStringList>

#include <functional>
#include <memory>

namespace Debugger::Internal {

// Where a debug adapter is, and how to reach it.
class DEBUGGER_EXPORT DapAdapterDescriptor
{
public:
    enum class Kind { Executable, Server, Pipe };

    Kind kind = Kind::Executable;
    // Executable: what to run, and the environment to run it in.
    Utils::CommandLine command;
    Utils::ProcessRunData runData;
    // Server: where it is already listening.
    QString host;
    quint16 port = 0;
    // Pipe: the local socket or named pipe it is already listening on.
    QString pipePath;
};

class DEBUGGER_EXPORT BridgeStartData
{
public:
    QStringList startupArguments;
    QString bridgeModule;
    QString serverCall;
};

DEBUGGER_EXPORT BridgeStartData dapHostRecipe(bool loadInitFile);

// What a backend speaking a DAP-shaped protocol is started with.
// A way to the adapter for whoever asked for the session. The backend fills
// this in as the session comes up and empties it when the session is over, so
// a caller holding the channel can tell whether there is anything to ask.
class DEBUGGER_EXPORT DapSessionChannel
{
public:
    using Answer = std::function<void(const Utils::Result<QJsonObject> &body)>;
    // Requests only the adapter itself defines. An extension asks its own
    // adapter things the protocol does not cover - cortex-debug asks for the
    // arguments its session was launched with, before it does anything else.
    using Sender = std::function<void(const QString &command, const QJsonObject &arguments,
                                      const Answer &answer)>;
    Sender send;

    // Set by the caller: the session is up - the adapter has accepted the
    // launch and can be asked things - or it ended before it ever was. Only
    // then is there a session to announce; an adapter told about it earlier
    // answers out of an empty state.
    std::function<void(bool running)> reportRunning;
};

class DEBUGGER_EXPORT DapStartData
{
public:
    // An adapter someone else provides, spoken to in stock DAP.
    DapAdapterDescriptor adapter;
    // Filled in by whoever wants to follow the session and reach its adapter.
    std::shared_ptr<DapSessionChannel> channel;
    // What to call ourselves in the initialize request.
    QString adapterId = "qtcreator";
    // The launch or attach body. It follows the adapter's own schema, so it is
    // passed through untouched.
    QJsonObject configuration;
    bool attach = false;

    // A host of Qt Creator's own, with the Qt dumpers behind it.
    Utils::ProcessRunData debuggerRunData;
    InferiorStartData inferiorStartData;
    Utils::FilePath dumperScriptsDir;
    BridgeStartData bridgeStartData;
    Utils::FilePaths extraDumperFiles;
    QStringList extraDumperCommands;
    Utils::FilePath sysroot;
    QList<QPair<QString, QString>> sourcePathMap;
    Utils::FilePaths sourceDirectories;
    bool nativeMixedDebugging = false;
    // Dumper context the interface's RefreshRequest does not carry.
    int qtVersion = 0;
    QString qtNamespace;
};

} // namespace Debugger::Internal
