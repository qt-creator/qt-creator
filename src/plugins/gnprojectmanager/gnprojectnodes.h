// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/projectnodes.h>

namespace GNProjectManager::Internal {

class GNProjectNode : public ProjectExplorer::ProjectNode
{
public:
    GNProjectNode(const Utils::FilePath &directory);
};

class GNTargetNode : public ProjectExplorer::ProjectNode
{
public:
    GNTargetNode(const Utils::FilePath &directory,
                 const QString &name,
                 const QString &targetType,
                 const QStringList &outputs);

    void build() override;
    QString tooltip() const final;
    QString buildKey() const final;

private:
    QString m_name;
    QString m_targetType;
    QStringList m_outputs;
};

} // namespace GNProjectManager::Internal
