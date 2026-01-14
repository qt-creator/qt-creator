// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QMap>
#include <QObject>
#include <QStringList>

namespace Mcp::Internal {

class IssuesManager;

class McpCommands : public QObject
{
    Q_OBJECT

public:
    explicit McpCommands(QObject *parent = nullptr);

    // Core MCP commands
    bool build();
    QString debug();
    QString stopDebug();
    bool openFile(const QString &path);
    QStringList listProjects();
    QStringList listBuildConfigs();
    bool switchToBuildConfig(const QString &name);
    bool quit();
    QString getVersion();
    QString getBuildStatus();

    // Additional useful commands
    QString getCurrentProject();
    QString getCurrentBuildConfig();
    bool cleanProject();
    QStringList listOpenFiles();

    // Session management commands
    QStringList listSessions();
    QString getCurrentSession();
    bool loadSession(const QString &sessionName);
    bool saveSession();

    // Issue management commands
    QStringList listIssues();

    // Method metadata management
    QString getMethodMetadata();
    QString setMethodMetadata(const QString &method, int timeoutSeconds);
    int getMethodTimeout(const QString &method) const;

    // Debugging management helpers
    bool isDebuggingActive();
    QString abortDebug();
    bool killDebuggedProcesses();
    void performDebuggingCleanup();
    bool performDebuggingCleanupSync();

private:
    bool hasValidProject() const;

    // Method timeout storage
    QMap<QString, int> m_methodTimeouts;

    // Issues management
    IssuesManager *m_issuesManager;
};

} // namespace Mcp::Internal
