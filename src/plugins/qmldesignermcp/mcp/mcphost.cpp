// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "mcphost.h"

namespace QmlDesigner {

McpHost::McpHost(
    const QString &protocolVersion,
    const QString &hostName,
    const QString &hostVersion,
    QObject *parent)
    : QObject(parent)
    , m_protocolVersion(protocolVersion)
    , m_hostName(hostName)
    , m_hostVersion(hostVersion)
{}

void McpHost::addServer(const McpServerConfig &cfg)
{
    if (cfg.name.isEmpty()) {
        qWarning() << __FUNCTION__ << "server name is empty";
        return;
    }
    m_serverConfigs.insert(cfg.name, cfg);
}

bool McpHost::hasServer(const QString &serverName) const
{
    return m_serverConfigs.contains(serverName);
}

void McpHost::removeServer(const QString &serverName)
{
    stopServer(serverName);
    m_serverConfigs.remove(serverName);
    m_allowedTools.remove(serverName);
    m_toolsRequireConsent.remove(serverName);
    cancelRestart(serverName);
}

QStringList McpHost::servers() const
{
    return m_serverConfigs.keys();
}

void McpHost::setAllowedTools(const QString &serverName, const QSet<QString> &tools)
{
    m_allowedTools.insert(serverName, tools);
}

QSet<QString> McpHost::allowedTools(const QString &serverName) const
{
    return m_allowedTools.value(serverName);
}

void McpHost::setConsentHook(ConsentHook hook)
{
    m_consentHook = std::move(hook);
}

void McpHost::setRequiredConsent(const QString &serverName, const QSet<QString> &tools)
{
    m_toolsRequireConsent.insert(serverName, tools);
}

QSet<QString> McpHost::requiredConsent(const QString &serverName) const
{
    return m_toolsRequireConsent.value(serverName);
}

void McpHost::startAll()
{
    const QStringList keys = m_serverConfigs.keys();
    for (const QString &name : keys)
        startServer(name);
}

void McpHost::stopAll()
{
    const QStringList keys = m_clients.keys();
    for (const QString &name : keys)
        stopServer(name);
}

bool McpHost::startServer(const QString &serverName)
{
    const auto it = m_serverConfigs.find(serverName);
    if (it == m_serverConfigs.end()) {
        emit errorOccurred(serverName, tr("No configuration for server '%1'").arg(serverName));
        return false;
    }

    // Prevent duplicate starts
    if (auto client = m_clients.value(serverName); !client.isNull()) {
        if (client->isRunning())
            return true;
        m_clients.remove(serverName);
    }

    const McpServerConfig &cfg = it.value();
    emit serverStarting(serverName);

    // Start a client and wire it to the server
    auto client = QSharedPointer<McpClient>::create();
    m_clients.insert(serverName, client);
    wireClientSignals(serverName, client);

    bool ok = false;
    switch (cfg.transport) {
    case McpServerConfig::Stdio: {
        ok = client->startProcess(cfg.command, cfg.args, cfg.workingDir, cfg.env);
        break;
    }
    case McpServerConfig::Http: {
        // TODO
        emit errorOccurred(serverName, tr("HTTP transport not implemented for '%1'").arg(serverName));
        ok = false;
        break;
    }
    }

    if (!ok) {
        emit errorOccurred(serverName, tr("Failed to start server '%1'").arg(serverName));
        return false;
    }

    // Initialize session (sends initialized on success)
    client->initialize(m_protocolVersion, m_hostName, m_hostVersion);
    return true;
}

void McpHost::stopServer(const QString &serverName)
{
    cancelRestart(serverName);

    auto it = m_clients.find(serverName);
    if (it == m_clients.end())
        return;

    it.value()->stop();
    m_clients.erase(it);
}

qint64 McpHost::listTools(const QString &serverName)
{
    QSharedPointer<McpClient> client = this->client(serverName);
    if (!client) {
        emit errorOccurred(
            serverName, QStringLiteral("listTools: no client for '%1'").arg(serverName));
        return -1;
    }
    return client->listTools();
}

qint64 McpHost::callTool(
    const QString &serverName, const QString &toolName, const QJsonObject &arguments)
{
    if (!isToolAllowed(serverName, toolName)) {
        emit errorOccurred(
            serverName, QStringLiteral("Tool '%1' not allowed on '%2'").arg(toolName, serverName));
        return -1;
    }

    if (!askConsent(serverName, toolName, arguments)) {
        emit errorOccurred(
            serverName,
            QStringLiteral("Consent denied for tool '%1' on '%2'").arg(toolName, serverName));
        return -1;
    }

    QSharedPointer<McpClient> client = this->client(serverName);
    if (!client) {
        emit errorOccurred(serverName, QStringLiteral("callTool: no client for '%1'").arg(serverName));
        return -1;
    }
    return client->callTool(toolName, arguments);
}

qint64 McpHost::sendRequest(
    const QString &serverName,
    const QString &method,
    const QJsonObject &params,
    McpClient::ResponseHandler callback)
{
    QSharedPointer<McpClient> client = this->client(serverName);
    if (!client) {
        emit errorOccurred(
            serverName, QStringLiteral("sendRequest: no client for '%1'").arg(serverName));
        return -1;
    }
    return client->sendRequest(method, params, std::move(callback));
}

QSharedPointer<McpClient> McpHost::client(const QString &serverName) const
{
    auto it = m_clients.find(serverName);
    if (it == m_clients.end())
        return {};
    return it.value();
}

void McpHost::wireClientSignals(const QString &name, const QSharedPointer<McpClient> &c)
{
    connect(c.get(), &McpClient::connected, [&](const McpServerInfo &info) {
        emit serverStarted(name, info);
    });

    connect(c.get(), &McpClient::exited, [&](int ec, QProcess::ExitStatus st) {
        emit serverExited(name, ec, st);
        // schedule restart if it crashed
        if (st == QProcess::CrashExit)
            scheduleRestart(name);
    });

    connect(c.get(), &McpClient::errorOccurred, [&](const QString &msg) {
        emit errorOccurred(name, msg);
    });

    connect(c.get(), &McpClient::logMessage, [&](const QString &line) {
        emit logMessage(name, line);
    });

    connect(c.get(), &McpClient::toolsListed, [&](const QList<McpTool> &tools, qint64 reqId) {
        emit toolsListed(name, tools, reqId);
    });

    connect(c.get(), &McpClient::toolCallSucceeded, [&](const QJsonObject &res, qint64 id) {
        emit toolCallSucceeded(name, res, id);
    });

    connect(
        c.get(),
        &McpClient::toolCallFailed,
        [&](const QString &msg, const QJsonObject &err, qint64 id) {
            emit toolCallFailed(name, msg, err, id);
        });

    connect(
        c.get(),
        &McpClient::notificationReceived,
        [&](const QString &method, const QJsonObject &params) {
            emit notificationReceived(name, method, params);
        });
}

void McpHost::scheduleRestart(const QString &name)
{
    QTimer *t = m_restartTimers.value(name, nullptr);
    if (!t) {
        t = new QTimer(this);
        t->setSingleShot(true);
        m_restartTimers.insert(name, t);
        connect(t, &QTimer::timeout, [this, name] {
            // Ensure client isn't already running
            if (auto client = this->client(name); !client.isNull() && client->isRunning())
                return;
            startServer(name);
        });
    }
    if (!t->isActive())
        t->start(2000);
}

void McpHost::cancelRestart(const QString &name)
{
    if (QTimer *t = m_restartTimers.value(name, nullptr))
        t->stop();
}

bool McpHost::isToolAllowed(const QString &serverName, const QString &toolName) const
{
    const QSet<QString> set = m_allowedTools.value(serverName);
    if (set.isEmpty())
        return true; // no list means allow all
    return set.contains(toolName);
}

bool McpHost::askConsent(
    const QString &serverName, const QString &toolName, const QJsonObject &args) const
{
    const QSet<QString> req = requiredConsent(serverName);
    if (bool needsConsent = req.contains(toolName); !needsConsent)
        return true;

    if (!m_consentHook)
        return false;

    return m_consentHook(serverName, toolName, args);
}

} // namespace QmlDesigner
