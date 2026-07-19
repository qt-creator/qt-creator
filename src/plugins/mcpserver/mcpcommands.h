// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <mcp/server/mcpserver.h>

#include <QObject>
#include <QStringList>

namespace Mcp::Internal {

class McpCommands : public QObject
{
    Q_OBJECT

public:
    explicit McpCommands(QObject *parent = nullptr);

    static void registerCommands();

    using ResponseCallback = std::function<void(const QJsonObject &response)>;

    // Core Mcp commands
    bool quit();
    QString getVersion();

    // document management commands
    bool openFile(const QString &path, int line = 0, int column = 0);
    QString getFilePlainText(const QString &path, int startLine = 0, int endLine = 0);
    QJsonObject setFilePlainText(const QString &path, const QString &contents);
    QJsonObject writeFileContentsBase64(const QString &path, const QString &base64);
    bool saveFile(const QString &path);
    bool closeFile(const QString &path);
    bool reformatFile(const QString &path);
    void searchInFile(
        const QString &path,
        const QString &pattern,
        bool regex,
        bool caseSensitive,
        const ResponseCallback &callback);
    void searchInDirectory(
        const QString directory,
        const QString &pattern,
        bool regex,
        bool caseSensitive,
        const ResponseCallback &callback);

    static Mcp::Schema::Tool::OutputSchema searchResultsSchema();

    void replaceInFile(
        const QString &path,
        const QString &pattern,
        const QString &replacement,
        bool regex,
        bool caseSensitive,
        const ResponseCallback &callback);
    void replaceInDirectory(
        const QString directory,
        const QString &pattern,
        const QString &replacement,
        bool regex,
        bool caseSensitive,
        const ResponseCallback &callback);

    // Additional useful commands
    QStringList listOpenFiles();
    QStringList listVisibleFiles();
    bool createNewFile(const QString &path, const QString &text);

    // Session management commands
    QStringList listSessions();
    QString getCurrentSession();
    bool loadSession(const QString &sessionName);
    bool saveSession();

    void executeCommand(
        const QString &command,
        const QString &arguments,
        const QString &workingDirectory,
        const ResponseCallback &callback);
};

} // namespace Mcp::Internal
