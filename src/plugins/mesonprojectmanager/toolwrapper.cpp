// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolwrapper.h"

#include "mesonpluginconstants.h"

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>

using namespace Utils;

namespace MesonProjectManager {
namespace Internal {

static ToolType typeFromId(const QString &id)
{
    if (id == Constants::ToolsSettings::TOOL_TYPE_NINJA)
        return ToolType::Ninja;
    if (id == Constants::ToolsSettings::TOOL_TYPE_MESON)
        return ToolType::Meson;
    QTC_CHECK(false);
    return ToolType::Meson;
}

ToolWrapper::ToolWrapper(const Store &data)
{
    m_toolType = typeFromId(data.value(Constants::ToolsSettings::TOOL_TYPE_KEY).toString());
    m_name = data[Constants::ToolsSettings::NAME_KEY].toString();
    m_exe = FilePath::fromSettings(data[Constants::ToolsSettings::EXE_KEY]);
    m_id = Id::fromSetting(data[Constants::ToolsSettings::ID_KEY]);
    m_autoDetected =  data[Constants::ToolsSettings::AUTO_DETECTED_KEY].toBool();
}

ToolWrapper::ToolWrapper(ToolType toolType,
                         const QString &name,
                         const FilePath &path,
                         bool autoDetected)
    : m_toolType(toolType)
    , m_version(read_version(path))
    , m_isValid{path.exists() && m_version.isValid}
    , m_autoDetected{autoDetected}
    , m_id{Utils::Id::generate()}
    , m_exe{path}
    , m_name{name}
{}

ToolWrapper::ToolWrapper(ToolType toolType,
                         const QString &name,
                         const FilePath &path,
                         const Id &id,
                         bool autoDetected)
    : m_toolType(toolType)
    , m_version(read_version(path))
    , m_isValid{path.exists() && m_version.isValid}
    , m_autoDetected{autoDetected}
    , m_id{id}
    , m_exe{path}
    , m_name{name}
{
    QTC_ASSERT(m_id.isValid(), m_id = Utils::Id::generate());
}

ToolWrapper::~ToolWrapper() = default;

void ToolWrapper::setExe(const Utils::FilePath &newExe)
{
    m_exe = newExe;
    m_version = read_version(m_exe);
}

Version ToolWrapper::read_version(const Utils::FilePath &toolPath)
{
    if (toolPath.toFileInfo().isExecutable()) {
        Utils::Process process;
        process.setCommand({ toolPath, { "--version" } });
        process.start();
        if (process.waitForFinished())
            return Version::fromString(process.cleanedStdOut());
    }
    return {};
}

Store ToolWrapper::toVariantMap() const
{
    Utils::Store data;
    data.insert(Constants::ToolsSettings::NAME_KEY, m_name);
    data.insert(Constants::ToolsSettings::EXE_KEY, m_exe.toSettings());
    data.insert(Constants::ToolsSettings::AUTO_DETECTED_KEY, m_autoDetected);
    data.insert(Constants::ToolsSettings::ID_KEY, m_id.toSetting());
    if (m_toolType == ToolType::Meson)
        data.insert(Constants::ToolsSettings::TOOL_TYPE_KEY, Constants::ToolsSettings::TOOL_TYPE_MESON);
    else
        data.insert(Constants::ToolsSettings::TOOL_TYPE_KEY, Constants::ToolsSettings::TOOL_TYPE_NINJA);
    return data;
}

static std::optional<FilePath> findTool(const QStringList &exeNames)
{
    Environment systemEnvironment = Environment::systemEnvironment();
    for (const auto &exe : exeNames) {
        const FilePath exe_path = systemEnvironment.searchInPath(exe);
        if (exe_path.exists())
            return exe_path;
    }
    return std::nullopt;
}

template<typename First>
void impl_option_cat(QStringList &list, const First &first)
{
    list.append(first);
}

template<typename First, typename... T>
void impl_option_cat(QStringList &list, const First &first, const T &...args)
{
    impl_option_cat(list, first);
    impl_option_cat(list, args...);
}

template<typename... T>
QStringList options_cat(const T &...args)
{
    QStringList result;
    impl_option_cat(result, args...);
    return result;
}

Command ToolWrapper::setup(const FilePath &sourceDirectory,
                           const FilePath &buildDirectory,
                           const QStringList &options) const
{
    return {{m_exe, options_cat("setup", options, sourceDirectory.path(), buildDirectory.path())},
            sourceDirectory};
}

Command ToolWrapper::configure(const FilePath &sourceDirectory,
                               const FilePath &buildDirectory,
                               const QStringList &options) const
{
    if (!isSetup(buildDirectory))
        return setup(sourceDirectory, buildDirectory, options);
    return {{m_exe, options_cat("configure", options, buildDirectory.path())},
            buildDirectory};
}

Command ToolWrapper::regenerate(const FilePath &sourceDirectory,
                                const FilePath &buildDirectory) const
{
    return {{m_exe,
            options_cat("--internal",
                        "regenerate",
                        sourceDirectory.path(),
                        buildDirectory.path(),
                        "--backend",
                        "ninja")},
            buildDirectory};
}

Command ToolWrapper::introspect(const Utils::FilePath &sourceDirectory) const
{
    return {{m_exe,
            {"introspect", "--all", QString("%1/meson.build").arg(sourceDirectory.path())}},
            sourceDirectory};
}

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

bool run_meson(const Command &command, QIODevice *output)
{
    Utils::Process process;
    process.setWorkingDirectory(command.workDir);
    process.setCommand(command.cmdLine);
    process.start();
    if (!process.waitForFinished())
        return false;
    if (output) {
        output->write(process.rawStdOut());
    }
    return process.exitCode() == 0;
}

bool isSetup(const Utils::FilePath &buildPath)
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

std::optional<FilePath> findMesonTool()
{
    return findTool({"meson.py", "meson"});
}

std::optional<FilePath> findNinjaTool()
{
    return findTool({"ninja", "ninja-build"});
}

} // namespace Internal
} // namespace MesonProjectManager
