// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mesonpluginconstants.h"
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

class ToolWrapper
{
public:
    virtual ~ToolWrapper() {}
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
    ToolWrapper(const ToolWrapper &other) = default;
    ToolWrapper(ToolWrapper &&other) = default;
    ToolWrapper &operator=(const ToolWrapper &other) = default;
    ToolWrapper &operator=(ToolWrapper &&other) = default;

    const Version &version() const noexcept { return m_version; }
    bool isValid() const noexcept { return m_isValid; }
    bool autoDetected() const noexcept { return m_autoDetected; }
    Utils::Id id() const noexcept { return m_id; }
    Utils::FilePath exe() const noexcept { return m_exe; }
    QString name() const noexcept { return m_name; }

    inline void setName(const QString &newName) { m_name = newName; }
    virtual void setExe(const Utils::FilePath &newExe);

    static Version read_version(const Utils::FilePath &toolPath);

    Utils::Store toVariantMap() const;
    static ToolWrapper *fromVariantMap(const Utils::Store &, ToolType toolType);

    ToolType toolType() const { return m_toolType; }
    void setToolType(ToolType newToolType) { m_toolType = newToolType; }

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

class MesonWrapper final : public ToolWrapper
{
public:
    using ToolWrapper::ToolWrapper;

    Command setup(const Utils::FilePath &sourceDirectory,
                  const Utils::FilePath &buildDirectory,
                  const QStringList &options = {}) const;
    Command configure(const Utils::FilePath &sourceDirectory,
                      const Utils::FilePath &buildDirectory,
                      const QStringList &options = {}) const;

    Command regenerate(const Utils::FilePath &sourceDirectory,
                       const Utils::FilePath &buildDirectory) const;

    Command introspect(const Utils::FilePath &sourceDirectory) const;
};

class NinjaWrapper final : public ToolWrapper
{
public:
    using ToolWrapper::ToolWrapper;
};

std::optional<Utils::FilePath> findMesonTool();
std::optional<Utils::FilePath> findNinjaTool();

} // namespace Internal
} // namespace MesonProjectManager
