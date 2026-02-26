// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>
#include <utils/id.h>
#include <utils/store.h>

#include <QVersionNumber>

#include <memory>
#include <vector>

namespace GNProjectManager::Internal {

class GNTool final
{
public:
    GNTool() = delete;
    explicit GNTool(const Utils::Store &data);
    GNTool(const QString &name,
           const Utils::FilePath &path,
           const Utils::Id &id = {},
           bool autoDetected = false);

    ~GNTool();

    const QVersionNumber &version() const noexcept { return m_version; }
    bool isValid() const noexcept { return m_isValid; }
    bool autoDetected() const noexcept { return m_autoDetected; }
    Utils::Id id() const noexcept { return m_id; }
    Utils::FilePath exe() const noexcept { return m_exe; }
    QString name() const noexcept { return m_name; }

    void setName(const QString &newName) { m_name = newName; }
    void setExe(const Utils::FilePath &newExe);

    static QVersionNumber readVersion(const Utils::FilePath &toolPath);

    Utils::Store toVariantMap() const;

private:
    QVersionNumber m_version;
    bool m_isValid;
    bool m_autoDetected;
    Utils::Id m_id;
    Utils::FilePath m_exe;
    QString m_name;
};

std::optional<Utils::FilePath> findGN();

class GNTools : public QObject
{
    Q_OBJECT

    GNTools() {}
    ~GNTools() {}

public:
    using Tool_t = std::shared_ptr<GNTool>;

    static void setTools(std::vector<Tool_t> &&tools);
    static const std::vector<Tool_t> &tools();

    static void updateTool(const Utils::Id &itemId, const QString &name, const Utils::FilePath &exe);
    static void removeTool(const Utils::Id &id);

    static std::shared_ptr<GNTool> toolById(const Utils::Id &id);
    static std::shared_ptr<GNTool> autoDetectedTool();

    static GNTools *instance();

signals:
    void toolAdded(const Tool_t &tool);
    void toolRemoved(const Tool_t &tool);
};

} // namespace GNProjectManager::Internal
