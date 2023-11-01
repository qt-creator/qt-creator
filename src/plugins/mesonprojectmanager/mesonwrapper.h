// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mesonpluginconstants.h"
#include "toolwrapper.h"

#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/id.h>
#include <utils/process.h>

#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>

#include <optional>
#include <tuple>

namespace MesonProjectManager {
namespace Internal {

template<typename File_t>
bool containsFiles(const QString &path, const File_t &file)
{
    return QFileInfo::exists(QString("%1/%2").arg(path).arg(file));
}

template<typename File_t, typename... T>
bool containsFiles(const QString &path, const File_t &file, const T &...files)
{
    return containsFiles(path, file) && containsFiles(path, files...);
}

inline bool run_meson(const Command &command, QIODevice *output = nullptr)
{
    Utils::Process process;
    process.setWorkingDirectory(command.workDir());
    process.setCommand(command.cmdLine());
    process.start();
    if (!process.waitForFinished())
        return false;
    if (output) {
        output->write(process.readAllRawStandardOutput());
    }
    return process.exitCode() == 0;
}

inline bool isSetup(const Utils::FilePath &buildPath)
{
    using namespace Utils;
    return containsFiles(buildPath.pathAppended(Constants::MESON_INFO_DIR).toString(),
                         Constants::MESON_INTRO_TESTS,
                         Constants::MESON_INTRO_TARGETS,
                         Constants::MESON_INTRO_INSTALLED,
                         Constants::MESON_INTRO_BENCHMARKS,
                         Constants::MESON_INTRO_BUIDOPTIONS,
                         Constants::MESON_INTRO_PROJECTINFO,
                         Constants::MESON_INTRO_DEPENDENCIES,
                         Constants::MESON_INTRO_BUILDSYSTEM_FILES);
}

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

    static std::optional<Utils::FilePath> find()
    {
        return ToolWrapper::findTool({"meson.py", "meson"});
    }

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

} // namespace Internal
} // namespace MesonProjectManager
