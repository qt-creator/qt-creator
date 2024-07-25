// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesontools.h"

#include "mesonpluginconstants.h"

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace Utils;

namespace MesonProjectManager::Internal {

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
                         const Id &id,
                         bool autoDetected)
    : m_toolType(toolType)
    , m_version(read_version(path))
    , m_isValid{path.exists() && !m_version.isNull()}
    , m_autoDetected{autoDetected}
    , m_id{id.isValid() ? id : Id::generate()}
    , m_exe{path}
    , m_name{name}
{
    QTC_ASSERT(m_id.isValid(), m_id = Id::generate());
}

ToolWrapper::~ToolWrapper() = default;

void ToolWrapper::setExe(const FilePath &newExe)
{
    m_exe = newExe;
    m_version = read_version(m_exe);
}

QVersionNumber ToolWrapper::read_version(const FilePath &toolPath)
{
    if (toolPath.isExecutableFile()) {
        Process process;
        process.setCommand({ toolPath, { "--version" } });
        process.start();
        if (process.waitForFinished())
            return QVersionNumber::fromString(process.cleanedStdOut());
    }
    return {};
}

Store ToolWrapper::toVariantMap() const
{
    Store data;
    data.insert(Constants::ToolsSettings::NAME_KEY, m_name);
    data.insert(Constants::ToolsSettings::EXE_KEY, m_exe.toSettings());
    data.insert(Constants::ToolsSettings::AUTO_DETECTED_KEY, m_autoDetected);
    data.insert(Constants::ToolsSettings::ID_KEY, m_id.toSetting());
    if (m_toolType == ToolType::Meson)
        data.insert(Constants::ToolsSettings::TOOL_TYPE_KEY, Constants::ToolsSettings::TOOL_TYPE_MESON);
    else
        data.insert(Constants::ToolsSettings::TOOL_TYPE_KEY, Constants::ToolsSettings::TOOL_TYPE_NINJA);
                  ;
    return data;
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

ProcessRunData ToolWrapper::setup(const FilePath &sourceDirectory,
                                  const FilePath &buildDirectory,
                                  const QStringList &options) const
{
    return {{m_exe, options_cat("setup", options, sourceDirectory.path(), buildDirectory.path())},
            sourceDirectory};
}

ProcessRunData ToolWrapper::configure(const FilePath &sourceDirectory,
                                      const FilePath &buildDirectory,
                                      const QStringList &options) const
{
    if (!isSetup(buildDirectory))
        return setup(sourceDirectory, buildDirectory, options);
    return {{m_exe, options_cat("configure", options, buildDirectory.path())},
            buildDirectory};
}

ProcessRunData ToolWrapper::regenerate(const FilePath &sourceDirectory,
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

ProcessRunData ToolWrapper::introspect(const FilePath &sourceDirectory) const
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

bool run_meson(const ProcessRunData &runData, QIODevice *output)
{
    Process process;
    process.setRunData(runData);
    process.start();
    if (!process.waitForFinished())
        return false;
    if (output) {
        output->write(process.rawStdOut());
    }
    return process.exitCode() == 0;
}

bool isSetup(const FilePath &buildPath)
{
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

static std::optional<FilePath> findToolHelper(const QStringList &exeNames)
{
    Environment systemEnvironment = Environment::systemEnvironment();
    for (const auto &exe : exeNames) {
        const FilePath exe_path = systemEnvironment.searchInPath(exe);
        if (exe_path.exists())
            return exe_path;
    }
    return std::nullopt;
}

std::optional<FilePath> findTool(ToolType toolType)
{
    if (toolType == ToolType::Meson)
        return findToolHelper({"meson.py", "meson"});
    if (toolType == ToolType::Ninja)
        return findToolHelper({"ninja", "ninja-build"});
    QTC_CHECK(false);
    return {};
}


std::vector<MesonTools::Tool_t> s_tools;

MesonTools::Tool_t MesonTools::autoDetectedTool(ToolType toolType)
{
    for (const auto &tool : s_tools) {
        if (tool->autoDetected() && tool->toolType() == toolType)
            return tool;
    }
    return nullptr;
}

static void fixAutoDetected(ToolType toolType)
{
    MesonTools::Tool_t autoDetected = MesonTools::autoDetectedTool(toolType);
    if (!autoDetected) {
        QStringList exeNames;
        QString toolName;
        if (toolType == ToolType::Meson) {
            if (std::optional<FilePath> path = findTool(toolType)) {
                s_tools.emplace_back(
                    std::make_shared<ToolWrapper>(toolType,
                        QString("System %1 at %2").arg("Meson").arg(path->toString()), *path, Id{}, true));
            }
        } else if (toolType == ToolType::Ninja) {
            if (std::optional<FilePath> path = findTool(toolType)) {
                s_tools.emplace_back(
                    std::make_shared<ToolWrapper>(toolType,
                        QString("System %1 at %2").arg("Ninja").arg(path->toString()), *path, Id{}, true));
            }
        }
    }
}

void MesonTools::setTools(std::vector<MesonTools::Tool_t> &&tools)
{
    std::swap(s_tools, tools);
    fixAutoDetected(ToolType::Meson);
    fixAutoDetected(ToolType::Ninja);
}

const std::vector<MesonTools::Tool_t> &MesonTools::tools()
{
    return s_tools;
}

void MesonTools::updateTool(const Id &itemId, const QString &name, const FilePath &exe)
{
    auto item = std::find_if(std::begin(s_tools),
                             std::end(s_tools),
                             [&itemId](const Tool_t &tool) { return tool->id() == itemId; });
    if (item != std::end(s_tools)) {
        (*item)->setExe(exe);
        (*item)->setName(name);
    } else {
        // TODO improve this
        const ToolType toolType = exe.fileName().contains("ninja") ? ToolType::Ninja : ToolType::Meson;
        s_tools.emplace_back(std::make_shared<ToolWrapper>(toolType, name, exe, itemId));
        emit instance()->toolAdded(s_tools.back());
    }
}

void MesonTools::removeTool(const Id &id)
{
    auto item = Utils::take(s_tools, [&id](const auto &item) { return item->id() == id; });
    QTC_ASSERT(item, return );
    emit instance()->toolRemoved(*item);
}

std::shared_ptr<ToolWrapper> MesonTools::toolById(const Id &id, ToolType toolType)
{
    const auto tool = std::find_if(std::cbegin(s_tools),
                                   std::cend(s_tools),
                                   [&id](const MesonTools::Tool_t &tool) {
                                       return tool->id() == id;
                                   });
    if (tool != std::cend(s_tools) && (*tool)->toolType() == toolType)
        return *tool;
    return nullptr;
}

MesonTools *MesonTools::instance()
{
    static MesonTools inst;
    return &inst;
}

} // MesonProjectManager::Internal
