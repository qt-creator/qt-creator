// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aitransaction.h"

#include "aiassistantconstants.h"

#include <QSet>

namespace QmlDesigner {

AiTransaction::AiTransaction() = default;

void AiTransaction::begin(const Utils::FilePath &projectRoot)
{
    m_projectRoot = projectRoot;
    m_snapshots.clear();
    m_active = true;
}

void AiTransaction::snapshotForTool(const QString &toolName, const QJsonObject &args)
{
    if (!m_active)
        return;

    if (!Constants::qmlStructureMutatingTools.contains(toolName))
        return;

    if (toolName == "move_qml") {
        // Snapshot both ends: source disappears, destination is created.
        snapshotFile(m_projectRoot / args.value("source").toString());
        snapshotFile(m_projectRoot / args.value("destination").toString());
    } else {
        // create_qml, modify_qml, delete_qml all use "filepath"
        snapshotFile(m_projectRoot / args.value("filepath").toString());
    }
}

void AiTransaction::rollback()
{
    if (!hasChanges())
        return;

    for (auto it = m_snapshots.constBegin(); it != m_snapshots.constEnd(); ++it) {
        Utils::FilePath file = Utils::FilePath::fromString(it.key());
        const Snapshot &snap = it.value();

        if (snap.existed) {
            // Restore original content
            file.writeFileContents(snap.content);
        } else {
            // File was created by the agent — remove it
            file.removeFile();
        }
    }

    m_snapshots.clear();
    m_active = false;
}

void AiTransaction::commit()
{
    // m_snapshots not cleared in order for undo (rollback()) to work
    m_active = false;
}

void AiTransaction::snapshotFile(const Utils::FilePath &absPath)
{
    const QString key = absPath.toFSPathString();

    if (m_snapshots.contains(key))
        return; // already captured before-state, don't overwrite

    Snapshot snap;
    snap.existed = absPath.exists();
    if (snap.existed) {
        auto result = absPath.fileContents();
        if (result)
            snap.content = *result;
    }

    m_snapshots.insert(key, snap);
}

void AiTransaction::reset()
{
    m_snapshots.clear();
    m_active = false;
}

} // namespace QmlDesigner
