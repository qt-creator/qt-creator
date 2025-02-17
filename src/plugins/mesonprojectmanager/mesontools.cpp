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

MesonToolWrapper::MesonToolWrapper(const Store &data)
    : MesonToolWrapper(
          data[Constants::ToolsSettings::NAME_KEY].toString(),
          FilePath::fromSettings(data[Constants::ToolsSettings::EXE_KEY]),
          Id::fromSetting(data[Constants::ToolsSettings::ID_KEY]),
          data[Constants::ToolsSettings::AUTO_DETECTED_KEY].toBool())
{
}

MesonToolWrapper::MesonToolWrapper(const QString &name,
                         const FilePath &path,
                         const Id &id,
                         bool autoDetected)
    : m_version(read_version(path))
    , m_isValid{path.exists() && !m_version.isNull()}
    , m_autoDetected{autoDetected}
    , m_id{id.isValid() ? id : Id::generate()}
    , m_exe{path}
    , m_name{name}
{
    QTC_ASSERT(m_id.isValid(), m_id = Id::generate());
}

MesonToolWrapper::~MesonToolWrapper() = default;

void MesonToolWrapper::setExe(const FilePath &newExe)
{
    m_exe = newExe;
    m_version = read_version(m_exe);
}

QVersionNumber MesonToolWrapper::read_version(const FilePath &toolPath)
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


Store MesonToolWrapper::toVariantMap() const
{
    Store data;
    data.insert(Constants::ToolsSettings::NAME_KEY, m_name);
    data.insert(Constants::ToolsSettings::EXE_KEY, m_exe.toSettings());
    data.insert(Constants::ToolsSettings::AUTO_DETECTED_KEY, m_autoDetected);
    data.insert(Constants::ToolsSettings::ID_KEY, m_id.toSetting());
    data.insert(Constants::ToolsSettings::TOOL_TYPE_KEY, Constants::ToolsSettings::TOOL_TYPE_MESON);
    return data;
}

QStringList options_cat(const auto &...args)
{
    QStringList result;
    (result.append(args), ...);
    return result;
}

QStringList make_verbose(QStringList list, bool verbose)
{
    if (verbose)
        list.append("--verbose");
    return list;
}

QStringList make_quiet(QStringList list, bool quiet)
{
    if (quiet)
        list.append("--quiet");
    return list;
}

ProcessRunData MesonToolWrapper::setup(const FilePath &sourceDirectory,
                                  const FilePath &buildDirectory,
                                  const QStringList &options) const
{
    return {{m_exe, options_cat("setup", options, sourceDirectory.path(), buildDirectory.path())},
            sourceDirectory};
}

ProcessRunData MesonToolWrapper::configure(const FilePath &sourceDirectory,
                                      const FilePath &buildDirectory,
                                      const QStringList &options) const
{
    if (!isSetup(buildDirectory))
        return setup(sourceDirectory, buildDirectory, options);
    return {{m_exe, options_cat("configure", options, buildDirectory.path())},
            buildDirectory};
}

ProcessRunData MesonToolWrapper::regenerate(const FilePath &sourceDirectory,
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

ProcessRunData MesonToolWrapper::introspect(const FilePath &sourceDirectory) const
{
    return {{m_exe,
            {"introspect", "--all", QString("%1/meson.build").arg(sourceDirectory.path())}},
            sourceDirectory};
}

ProcessRunData MesonToolWrapper::compile(const FilePath &buildDirectory, const QString &target, bool verbose) const
{
    // Specific generic targets that needs to be handled differently
    // with meson compile
    if (target == Constants::Targets::clean)
        return clean(buildDirectory, verbose);
    if (target == Constants::Targets::all)
        return compile(buildDirectory, verbose);
    if (target == Constants::Targets::tests)
        return test(buildDirectory, "", verbose);
    if (target == Constants::Targets::benchmark)
        return benchmark(buildDirectory, "", verbose);
    if (target == Constants::Targets::install)
        return install(buildDirectory, verbose);
    return {{m_exe,
            make_verbose(QStringList{"compile"}, verbose) + QStringList{target}},
            buildDirectory};
}

Utils::ProcessRunData MesonProjectManager::Internal::MesonToolWrapper::compile(const Utils::FilePath &buildDirectory, bool verbose) const
{
    // implicit target is "all" and "all" doesn't exist as target with meson compile
    return {{m_exe,
            make_verbose(QStringList{"compile"}, verbose)},
            buildDirectory};
}

Utils::ProcessRunData MesonProjectManager::Internal::MesonToolWrapper::test(const Utils::FilePath &buildDirectory, const QString &testSuite, bool verbose) const
{
    if (testSuite.isEmpty())
        return {{m_exe,
                make_verbose(QStringList{"test"}, verbose)},
                buildDirectory};
    else
        return {{m_exe,
                make_verbose(QStringList{"test"}, verbose) + QStringList{"--suite", testSuite}},
                buildDirectory};
}

Utils::ProcessRunData MesonProjectManager::Internal::MesonToolWrapper::benchmark(const Utils::FilePath &buildDirectory, const QString &benchmark, bool verbose) const
{
    if (benchmark.isEmpty())
        return {{m_exe,
                make_verbose(QStringList{"test", "--benchmark"},  verbose)},
                buildDirectory};
    else
        return {{m_exe,
                make_verbose(QStringList{"test", "--benchmark"},  verbose ) + QStringList{"--suite", benchmark}},
                buildDirectory};
}

ProcessRunData MesonToolWrapper::clean(const Utils::FilePath &buildDirectory, bool verbose) const
{
    return {{m_exe,
            make_verbose({"compile", "--clean"}, verbose)},
            buildDirectory};
}

Utils::ProcessRunData MesonProjectManager::Internal::MesonToolWrapper::install(const Utils::FilePath &buildDirectory, bool verbose) const
{
    return {{m_exe,
            make_quiet({"install"},  !verbose)},
            buildDirectory};
}

bool containsFiles(const QString &path, const auto &...files)
{
    return (QFileInfo::exists(QString("%1/%2").arg(path).arg(files)) && ...);
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
    return containsFiles(buildPath.pathAppended(Constants::MESON_INFO_DIR).toUrlishString(),
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

std::optional<FilePath> findMeson()
{
    return findToolHelper({"meson.py", "meson"});
    QTC_CHECK(false);
    return {};
}


std::vector<MesonTools::Tool_t> s_tools;

MesonTools::Tool_t MesonTools::autoDetectedTool()
{
    for (const auto &tool : s_tools) {
        if (tool->autoDetected())
            return tool;
    }
    return nullptr;
}

static void ensureAutoDetected()
{
    if (MesonTools::autoDetectedTool())
        return;

    if (const std::optional<FilePath> path = findMeson()) {
        s_tools.emplace_back(std::make_shared<MesonToolWrapper>(
                QString("System %1 at %2").arg("Meson", path->toUrlishString()), *path, Id{}, true));

    }
}

void MesonTools::setTools(std::vector<MesonTools::Tool_t> &&tools)
{
    std::swap(s_tools, tools);
    ensureAutoDetected();
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
        s_tools.emplace_back(std::make_shared<MesonToolWrapper>( name, exe, itemId));
        emit instance()->toolAdded(s_tools.back());
    }
}

void MesonTools::removeTool(const Id &id)
{
    auto item = Utils::take(s_tools, [&id](const auto &item) { return item->id() == id; });
    QTC_ASSERT(item, return );
    emit instance()->toolRemoved(*item);
}

std::shared_ptr<MesonToolWrapper> MesonTools::toolById(const Id &id)
{
    const auto tool = std::find_if(std::cbegin(s_tools),
                                   std::cend(s_tools),
                                   [&id](const MesonTools::Tool_t &tool) {
                                       return tool->id() == id;
                                   });
    if (tool != std::cend(s_tools))
        return *tool;
    return nullptr;
}

MesonTools *MesonTools::instance()
{
    static MesonTools inst;
    return &inst;
}

} // MesonProjectManager::Internal
