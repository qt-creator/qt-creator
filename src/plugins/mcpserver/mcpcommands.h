// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "issuesmanager.h"

#include <utils/result.h>

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
    // TODO: find file for pattern
    // TODO: reformat file
    // TODO: search in File
    // TODO: replace text in file
    // TODO: get symbol info
    // TODO: rename symbol

    // Additional useful commands
    QString getCurrentProject();
    QString getCurrentBuildConfig();
    QStringList listOpenFiles();
    Utils::Result<QStringList> projectDependencies(const QString &projectName);
    // TODO: add a new File to the project
    // TODO: get repositories in project

    // Session management commands
    QStringList listSessions();
    QString getCurrentSession();
    bool loadSession(const QString &sessionName);
    bool saveSession();

    // Issue management commands
    QJsonObject listIssues();

    // TODO: list issues for a File

    // Debugging management helpers
    bool isDebuggingActive();
    QString abortDebug();
    bool killDebuggedProcesses();
    void performDebuggingCleanup();
    bool performDebuggingCleanupSync();

    void executeCommand(
        const QString &command,
        const QString &arguments,
        const QString &workingDirectory,
        std::function<void(const QJsonObject &response)> callback);

private:
    // Issues management
    IssuesManager m_issuesManager;
};

} // namespace Mcp::Internal
