// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "versionhelper.h"

#include <utils/commandline.h>
#include <utils/id.h>
#include <utils/store.h>

#include <optional>
#include <memory>

namespace MesonProjectManager::Internal {

enum class ToolType { Meson, Ninja };

class Command
{
public:
    Utils::CommandLine cmdLine;
    Utils::FilePath workDir;
};

class ToolWrapper final
{
public:
    ToolWrapper() = delete;
    explicit ToolWrapper(const Utils::Store &data);
    ToolWrapper(ToolType toolType,
                const QString &name,
                const Utils::FilePath &path,
                bool autoDetected = false);
    ToolWrapper(ToolType toolType,
                const QString &name,
                const Utils::FilePath &path,
                const Utils::Id &id,
                bool autoDetected = false);

    ~ToolWrapper();

    const Version &version() const noexcept { return m_version; }
    bool isValid() const noexcept { return m_isValid; }
    bool autoDetected() const noexcept { return m_autoDetected; }
    Utils::Id id() const noexcept { return m_id; }
    Utils::FilePath exe() const noexcept { return m_exe; }
    QString name() const noexcept { return m_name; }

    void setName(const QString &newName) { m_name = newName; }
    void setExe(const Utils::FilePath &newExe);

    static Version read_version(const Utils::FilePath &toolPath);

    Utils::Store toVariantMap() const;

    ToolType toolType() const { return m_toolType; }
    void setToolType(ToolType newToolType) { m_toolType = newToolType; }

    Command setup(const Utils::FilePath &sourceDirectory,
                       const Utils::FilePath &buildDirectory,
                       const QStringList &options = {}) const;
    Command configure(const Utils::FilePath &sourceDirectory,
                           const Utils::FilePath &buildDirectory,
                           const QStringList &options = {}) const;
    Command regenerate(const Utils::FilePath &sourceDirectory,
                            const Utils::FilePath &buildDirectory) const;
    Command introspect(const Utils::FilePath &sourceDirectory) const;

private:
    ToolType m_toolType;
    Version m_version;
    bool m_isValid;
    bool m_autoDetected;
    Utils::Id m_id;
    Utils::FilePath m_exe;
    QString m_name;
};

bool run_meson(const Command &command, QIODevice *output = nullptr);

bool isSetup(const Utils::FilePath &buildPath);

std::optional<Utils::FilePath> findTool(ToolType toolType);

class MesonTools : public QObject
{
    Q_OBJECT
    MesonTools() {}
    ~MesonTools() {}

public:
    using Tool_t = std::shared_ptr<ToolWrapper>;

    static bool isMesonWrapper(const Tool_t &tool);
    static bool isNinjaWrapper(const Tool_t &tool);

    static void addTool(const Utils::Id &itemId,
                        const QString &name,
                        const Utils::FilePath &exe);

    static void addTool(Tool_t meson);

    static void setTools(std::vector<Tool_t> &&tools);

    static const std::vector<Tool_t> &tools();

    static void updateTool(const Utils::Id &itemId,
                           const QString &name,
                           const Utils::FilePath &exe);
    static void removeTool(const Utils::Id &id);

    static std::shared_ptr<ToolWrapper> toolById(const Utils::Id &id, ToolType toolType);

    static std::shared_ptr<ToolWrapper> autoDetectedTool(ToolType toolType);

    Q_SIGNAL void toolAdded(const Tool_t &tool);
    Q_SIGNAL void toolRemoved(const Tool_t &tool);

    static MesonTools *instance();
};

} // MesonProjectManager::Internal
