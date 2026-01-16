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
    QString stopDebug();

    QStringList listProjects();
    QStringList listBuildConfigs();
    bool switchToBuildConfig(const QString &name);
    bool quit();
    QString getVersion();
    QString getBuildStatus();

    // document management commands
    bool openFile(const QString &path);
    QString getFilePlainText(const QString &path);
    bool setFilePlainText(const QString &path, const QString &contents);
    bool saveFile(const QString &path);
    bool closeFile(const QString &path);

    // Additional useful commands
    QString getCurrentProject();
    QString getCurrentBuildConfig();
    QStringList listOpenFiles();

    // Session management commands
    QStringList listSessions();
    QString getCurrentSession();
    bool loadSession(const QString &sessionName);
    bool saveSession();

    // Issue management commands
    QStringList listIssues();

    // Debugging management helpers
    bool isDebuggingActive();
    QString abortDebug();
    bool killDebuggedProcesses();
    void performDebuggingCleanup();
    bool performDebuggingCleanupSync();

private:
    // Issues management
    IssuesManager *m_issuesManager;
};

} // namespace Mcp::Internal
