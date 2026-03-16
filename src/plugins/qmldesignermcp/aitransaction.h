// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QHash>
#include <QJsonObject>
#include <QString>

namespace QmlDesigner {

/**
 * @brief Tracks all file mutations made by one agentic request loop against the qml_server MCP tools.
 *
 * Files are snapshotted (before-state captured) the first time a tool
 * touches them. Rollback restores every snapshotted file; commit is a no-op
 * since the MCP tools write directly to disk.
 */
class AiTransaction
{
public:
    AiTransaction();

    // Opens the transaction. Must be called before the agentic loop starts.
    void begin(const Utils::FilePath &projectRoot);

    // Call from the toolCallStarted handler.
    // snapshots the affected file(s) before the tool runs.
    void snapshotForTool(const QString &toolName, const QJsonObject &args);

    // Restores all snapshotted files to their before-state.
    void rollback();

    // No-op: files are already on disk. Marks the transaction closed.
    void commit();

    // Discard snapshot state without any file I/O — used on session reset.
    void reset();

    bool isActive() const { return m_active; }
    bool hasChanges() const { return !m_snapshots.isEmpty(); }

private:
    struct Snapshot {
        bool existed = false;
        QByteArray content; // empty when existed == false
    };

    void snapshotFile(const Utils::FilePath &absPath);
    Utils::FilePath resolve(const QString &relativePath) const;


    Utils::FilePath m_projectRoot;
    QHash<QString, Snapshot> m_snapshots; // key: absolute path string
    bool m_active = false;
};

} // namespace QmlDesigner
