// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "versionhelper.h"

#include <utils/commandline.h>
#include <utils/id.h>
#include <utils/store.h>

#include <optional>

namespace MesonProjectManager {
namespace Internal {

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
    static ToolWrapper *fromVariantMap(const Utils::Store &, ToolType toolType);

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

protected:
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

std::optional<Utils::FilePath> findMesonTool();
std::optional<Utils::FilePath> findNinjaTool();

} // namespace Internal
} // namespace MesonProjectManager
