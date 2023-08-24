// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "versionhelper.h"

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/id.h>
#include <utils/qtcassert.h>
#include <utils/store.h>

#include <optional>

namespace MesonProjectManager {
namespace Internal {

class Command
{
    Utils::CommandLine m_cmd;
    Utils::FilePath m_workDir;

public:
    Command() = default;
    Command(const Utils::FilePath &exe, const Utils::FilePath &workDir, const QStringList &args)
        : m_cmd{exe, args}
        , m_workDir{workDir}
    {}
    const Utils::CommandLine &cmdLine() const { return m_cmd; }
    const Utils::FilePath &workDir() const { return m_workDir; }
    Utils::FilePath executable() const { return m_cmd.executable(); }
    QStringList arguments() const { return m_cmd.splitArguments(); }
    QString toUserOutput() const { return m_cmd.toUserOutput(); }
};

class ToolWrapper
{
public:
    virtual ~ToolWrapper() {}
    ToolWrapper() = delete;
    ToolWrapper(const QString &name, const Utils::FilePath &path, bool autoDetected = false);
    ToolWrapper(const QString &name,
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

    static std::optional<Utils::FilePath> findTool(const QStringList &exeNames);

    template<typename T>
    friend Utils::Store toVariantMap(const T &);
    template<typename T>
    friend T fromVariantMap(const Utils::Store &);

protected:
    Version m_version;
    bool m_isValid;
    bool m_autoDetected;
    Utils::Id m_id;
    Utils::FilePath m_exe;
    QString m_name;
};

template<typename T>
Utils::Store toVariantMap(const T &);
template<typename T>
T fromVariantMap(const Utils::Store &);

} // namespace Internal
} // namespace MesonProjectManager
