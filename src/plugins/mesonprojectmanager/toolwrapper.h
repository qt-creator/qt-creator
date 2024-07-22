// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mesonpluginconstants.h"
#include "versionhelper.h"

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/id.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
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

    static std::optional<Utils::FilePath> find();

    static QString toolName() { return {"Meson"}; }
};

template<>
inline Utils::Store toVariantMap<MesonWrapper>(const MesonWrapper &meson)
{
    Utils::Store data;
    data.insert(Constants::ToolsSettings::NAME_KEY, meson.m_name);
    data.insert(Constants::ToolsSettings::EXE_KEY, meson.m_exe.toSettings());
    data.insert(Constants::ToolsSettings::AUTO_DETECTED_KEY, meson.m_autoDetected);
    data.insert(Constants::ToolsSettings::ID_KEY, meson.m_id.toSetting());
    data.insert(Constants::ToolsSettings::TOOL_TYPE_KEY, Constants::ToolsSettings::TOOL_TYPE_MESON);
    return data;
}
template<>
inline MesonWrapper *fromVariantMap<MesonWrapper *>(const Utils::Store &data)
{
    return new MesonWrapper(data[Constants::ToolsSettings::NAME_KEY].toString(),
                            Utils::FilePath::fromSettings(data[Constants::ToolsSettings::EXE_KEY]),
                            Utils::Id::fromSetting(data[Constants::ToolsSettings::ID_KEY]),
                            data[Constants::ToolsSettings::AUTO_DETECTED_KEY].toBool());
}

class NinjaWrapper final : public ToolWrapper
{
public:
    using ToolWrapper::ToolWrapper;

    static std::optional<Utils::FilePath> find();
    static QString toolName() { return {"Ninja"}; }
};

template<>
inline Utils::Store toVariantMap<NinjaWrapper>(const NinjaWrapper &meson)
{
    Utils::Store data;
    data.insert(Constants::ToolsSettings::NAME_KEY, meson.m_name);
    data.insert(Constants::ToolsSettings::EXE_KEY, meson.m_exe.toSettings());
    data.insert(Constants::ToolsSettings::AUTO_DETECTED_KEY, meson.m_autoDetected);
    data.insert(Constants::ToolsSettings::ID_KEY, meson.m_id.toSetting());
    data.insert(Constants::ToolsSettings::TOOL_TYPE_KEY, Constants::ToolsSettings::TOOL_TYPE_NINJA);
    return data;
}
template<>
inline NinjaWrapper *fromVariantMap<NinjaWrapper *>(const Utils::Store &data)
{
    return new NinjaWrapper(data[Constants::ToolsSettings::NAME_KEY].toString(),
                            Utils::FilePath::fromSettings(data[Constants::ToolsSettings::EXE_KEY]),
                            Utils::Id::fromSetting(data[Constants::ToolsSettings::ID_KEY]),
                            data[Constants::ToolsSettings::AUTO_DETECTED_KEY].toBool());
}

} // namespace Internal
} // namespace MesonProjectManager
