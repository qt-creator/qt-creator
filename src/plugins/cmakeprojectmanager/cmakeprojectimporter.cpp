// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeprojectimporter.h"

#include "cmakebuildconfiguration.h"
#include "cmakekitaspect.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"
#include "cmaketoolmanager.h"
#include "presetsmacros.h"

#include <coreplugin/messagemanager.h>

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchainmanager.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/algorithm.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/temporarydirectory.h>

#include <QApplication>
#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

using namespace std::chrono_literals;

namespace CMakeProjectManager::Internal {

static Q_LOGGING_CATEGORY(cmInputLog, "qtc.cmake.import", QtWarningMsg);

struct DirectoryData
{
    // Project Stuff:
    QByteArray cmakeBuildType;
    FilePath buildDirectory;
    FilePath cmakeHomeDirectory;
    bool hasQmlDebugging = false;

    QString cmakePresetDisplayname;
    QString cmakePreset;

    // Kit Stuff
    FilePath cmakeBinary;
    QString generator;
    QString platform;
    QString toolset;
    FilePath sysroot;
    QtProjectImporter::QtVersionData qt;
    QVector<ToolchainDescription> toolchains;
};

static FilePaths scanDirectory(const FilePath &path, const QString &prefix)
{
    FilePaths result;
    qCDebug(cmInputLog) << "Scanning for directories matching" << prefix << "in" << path;

    const FilePaths entries = path.dirEntries({{prefix + "*"}, QDir::Dirs | QDir::NoDotAndDotDot});
    for (const FilePath &entry : entries) {
        QTC_ASSERT(entry.isDir(), continue);
        result.append(entry);
    }
    return result;
}

static QString baseCMakeToolDisplayName(CMakeTool &tool)
{
    if (!tool.isValid())
        return QString("CMake");

    CMakeTool::Version version = tool.version();
    return QString("CMake %1.%2.%3").arg(version.major).arg(version.minor).arg(version.patch);
}

static QString uniqueCMakeToolDisplayName(CMakeTool &tool)
{
    QString baseName = baseCMakeToolDisplayName(tool);

    QStringList existingNames;
    for (const CMakeTool *t : CMakeToolManager::cmakeTools())
        existingNames << t->displayName();
    return Utils::makeUniquelyNumbered(baseName, existingNames);
}

// CMakeProjectImporter

CMakeProjectImporter::CMakeProjectImporter(const FilePath &path, const CMakeProject *project)
    : QtProjectImporter(path)
    , m_project(project)
    , m_presetsTempDir("qtc-cmake-presets-XXXXXXXX")
{
    useTemporaryKitAspect(CMakeKitAspect::id(),
                               [this](Kit *k, const QVariantList &vl) { cleanupTemporaryCMake(k, vl); },
                               [this](Kit *k, const QVariantList &vl) { persistTemporaryCMake(k, vl); });

}

using CharToHexList = QList<QPair<QString, QString>>;
static const CharToHexList &charToHexList()
{
    static const CharToHexList list = {
        {"<", "{3C}"},
        {">", "{3E}"},
        {":", "{3A}"},
        {"\"", "{22}"},
        {"\\", "{5C}"},
        {"/", "{2F}"},
        {"|", "{7C}"},
        {"?", "{3F}"},
        {"*", "{2A}"},
    };

    return list;
}

static QString presetNameToFileName(const QString &name)
{
    QString fileName = name;
    for (const auto &p : charToHexList())
        fileName.replace(p.first, p.second);
    return fileName;
}

static QString fileNameToPresetName(const QString &fileName)
{
    QString name = fileName;
    for (const auto &p : charToHexList())
        name.replace(p.second, p.first);
    return name;
}

static QString displayPresetName(const QString &presetName)
{
    return QString("%1 (CMake preset)").arg(presetName);
}

FilePaths CMakeProjectImporter::importCandidates()
{
    FilePaths candidates = presetCandidates();

    if (candidates.isEmpty()) {
        candidates << scanDirectory(projectFilePath().absolutePath(), "build");

        const QList<Kit *> kits = KitManager::kits();
        for (const Kit *k : kits) {
            FilePath shadowBuildDirectory
                = CMakeBuildConfiguration::shadowBuildDirectory(projectFilePath(),
                                                                k,
                                                                QString(),
                                                                BuildConfiguration::Unknown);
            candidates << scanDirectory(shadowBuildDirectory.absolutePath(), QString());
        }
    }

    const FilePaths finalists = Utils::filteredUnique(candidates);
    qCInfo(cmInputLog) << "import candidates:" << finalists;
    return finalists;
}

FilePaths CMakeProjectImporter::presetCandidates()
{
    FilePaths candidates;

    for (const auto &configPreset : m_project->presetsData().configurePresets) {
        if (configPreset.hidden.value())
            continue;

        if (configPreset.condition) {
            if (!CMakePresets::Macros::evaluatePresetCondition(configPreset, projectFilePath()))
                continue;
        }

        const FilePath configPresetDir = m_presetsTempDir.filePath(
            presetNameToFileName(configPreset.name));
        configPresetDir.createDir();
        candidates << configPresetDir;

        // If the binaryFilePath exists, do not try to import the existing build, so that
        // we don't have duplicates, one from the preset and one from the previous configuration.
        if (configPreset.binaryDir) {
            Environment env = projectDirectory().deviceEnvironment();
            CMakePresets::Macros::expand(configPreset, env, projectDirectory());

            QString binaryDir = configPreset.binaryDir.value();
            CMakePresets::Macros::expand(configPreset, env, projectDirectory(), binaryDir);

            const FilePath binaryFilePath = FilePath::fromString(binaryDir);
            candidates.removeIf([&binaryFilePath](const FilePath &path)
                                { return path == binaryFilePath; });
        }
    }

    return candidates;
}

Target *CMakeProjectImporter::preferredTarget(const QList<Target *> &possibleTargets)
{
    for (Kit *kit : m_project->oldPresetKits()) {
        const bool haveKit = Utils::contains(possibleTargets, [kit](const auto &target) {
            return target->kit() == kit;
        });

        if (!haveKit)
            KitManager::deregisterKit(kit);
    }
    m_project->setOldPresetKits({});

    return ProjectImporter::preferredTarget(possibleTargets);
}

bool CMakeProjectImporter::filter(ProjectExplorer::Kit *k) const
{
    if (!m_project->presetsData().havePresets)
        return true;

    const auto presetConfigItem = CMakeConfigurationKitAspect::cmakePresetConfigItem(k);
    if (presetConfigItem.isNull())
        return false;

    const QString presetName = presetConfigItem.expandedValue(k);
    return std::find_if(m_project->presetsData().configurePresets.cbegin(),
                        m_project->presetsData().configurePresets.cend(),
                        [&presetName](const auto &preset) { return presetName == preset.name; })
           != m_project->presetsData().configurePresets.cend();
}

static CMakeConfig configurationFromPresetProbe(
    const FilePath &importPath,
    const FilePath &sourceDirectory,
    const PresetsDetails::ConfigurePreset &configurePreset)
{
    const FilePath cmakeListTxt = importPath / Constants::CMAKE_LISTS_TXT;
    cmakeListTxt.writeFileContents(QByteArray("cmake_minimum_required(VERSION 3.15)\n"
                                              "\n"
                                              "project(preset-probe)\n"
                                              "set(CMAKE_C_COMPILER \"${CMAKE_C_COMPILER}\" CACHE FILEPATH \"\" FORCE)\n"
                                              "set(CMAKE_CXX_COMPILER \"${CMAKE_CXX_COMPILER}\" CACHE FILEPATH \"\" FORCE)\n"
                                              "\n"));

    Process cmake;
    cmake.setDisableUnixTerminal();

    const FilePath cmakeExecutable = FilePath::fromString(configurePreset.cmakeExecutable.value());

    Environment env = cmakeExecutable.deviceEnvironment();
    CMakePresets::Macros::expand(configurePreset, env, sourceDirectory);

    env.setupEnglishOutput();
    cmake.setEnvironment(env);

    QStringList args;
    args.emplace_back("-S");
    args.emplace_back(importPath.path());
    args.emplace_back("-B");
    args.emplace_back(importPath.pathAppended("build/").path());

    if (configurePreset.generator) {
        args.emplace_back("-G");
        args.emplace_back(configurePreset.generator.value());
    }
    if (configurePreset.architecture && configurePreset.architecture.value().value) {
        if (!configurePreset.architecture->strategy
            || configurePreset.architecture->strategy
                   != PresetsDetails::ValueStrategyPair::Strategy::external) {
            args.emplace_back("-A");
            args.emplace_back(configurePreset.architecture.value().value.value());
        }
    }
    if (configurePreset.toolset && configurePreset.toolset.value().value) {
        if (!configurePreset.toolset->strategy
            || configurePreset.toolset->strategy
                   != PresetsDetails::ValueStrategyPair::Strategy::external) {
            args.emplace_back("-T");
            args.emplace_back(configurePreset.toolset.value().value.value());
        }
    }

    if (configurePreset.cacheVariables) {
        const CMakeConfig cache = configurePreset.cacheVariables
                                      ? configurePreset.cacheVariables.value()
                                      : CMakeConfig();

        const QString cmakeMakeProgram = cache.stringValueOf("CMAKE_MAKE_PROGRAM");
        const QString toolchainFile = cache.stringValueOf("CMAKE_TOOLCHAIN_FILE");
        const QString prefixPath = cache.stringValueOf("CMAKE_PREFIX_PATH");
        const QString findRootPath = cache.stringValueOf("CMAKE_FIND_ROOT_PATH");
        const QString qtHostPath = cache.stringValueOf("QT_HOST_PATH");
        const QString sysRoot = cache.stringValueOf("CMAKE_SYSROOT");

        if (!cmakeMakeProgram.isEmpty()) {
            args.emplace_back(
                QStringLiteral("-DCMAKE_MAKE_PROGRAM=%1").arg(cmakeMakeProgram));
        }
        if (!toolchainFile.isEmpty()) {
            args.emplace_back(
                QStringLiteral("-DCMAKE_TOOLCHAIN_FILE=%1").arg(toolchainFile));
        }
        if (!prefixPath.isEmpty()) {
            args.emplace_back(QStringLiteral("-DCMAKE_PREFIX_PATH=%1").arg(prefixPath));
        }
        if (!findRootPath.isEmpty()) {
            args.emplace_back(QStringLiteral("-DCMAKE_FIND_ROOT_PATH=%1").arg(findRootPath));
        }
        if (!qtHostPath.isEmpty()) {
            args.emplace_back(QStringLiteral("-DQT_HOST_PATH=%1").arg(qtHostPath));
        }
        if (!sysRoot.isEmpty()) {
            args.emplace_back(QStringLiteral("-DCMAKE_SYSROOT=%1").arg(sysRoot));
        }
    }

    qCDebug(cmInputLog) << "CMake probing for compilers: " << cmakeExecutable.toUserOutput()
                        << args;
    cmake.setCommand({cmakeExecutable, args});
    cmake.runBlocking(30s);

    QString errorMessage;
    const CMakeConfig config = CMakeConfig::fromFile(importPath.pathAppended(
                                                         "build/CMakeCache.txt"),
                                                     &errorMessage);

    return config;
}

struct QMakeAndCMakePrefixPath
{
    FilePath qmakePath;
    QString cmakePrefixPath; // can be a semicolon-separated list
};

static QMakeAndCMakePrefixPath qtInfoFromCMakeCache(const CMakeConfig &config,
                                                    const Environment &env)
{
    // Qt4 way to define things (more convenient for us, so try this first;-)
    const FilePath qmake = config.filePathValueOf("QT_QMAKE_EXECUTABLE");
    qCDebug(cmInputLog) << "QT_QMAKE_EXECUTABLE=" << qmake.toUserOutput();

    // Check Qt5 settings: oh, the horror!
    const FilePath qtCMakeDir = [config, env] {
        FilePath tmp;
        // Check the CMake "<package-name>_DIR" variable
        for (const auto &var : {"Qt6", "Qt6Core", "Qt5", "Qt5Core"}) {
            tmp = config.filePathValueOf(QByteArray(var) + "_DIR");
            if (!tmp.isEmpty())
                break;
        }
        return tmp;
    }();
    qCDebug(cmInputLog) << "QtXCore_DIR=" << qtCMakeDir.toUserOutput();
    const FilePath canQtCMakeDir = FilePath::fromString(qtCMakeDir.toFileInfo().canonicalFilePath());
    qCInfo(cmInputLog) << "QtXCore_DIR (canonical)=" << canQtCMakeDir.toUserOutput();

    const QString prefixPath = [qtCMakeDir, canQtCMakeDir, config, env] {
        QString result;
        if (!qtCMakeDir.isEmpty()) {
            result = canQtCMakeDir.parentDir().parentDir().parentDir().path(); // Up 3 levels...
        } else {
            // Check the CMAKE_PREFIX_PATH and "<package-name>_ROOT" CMake or environment variables
            // This can be a single value or a semicolon-separated list
            for (const auto &var : {"CMAKE_PREFIX_PATH", "Qt6_ROOT", "Qt5_ROOT"}) {
                result = config.stringValueOf(var);
                if (result.isEmpty())
                    result = env.value(QString::fromUtf8(var));
                if (!result.isEmpty())
                    break;
            }
        }
        return result;
    }();
    qCDebug(cmInputLog) << "PrefixPath:" << prefixPath;

    if (!qmake.isEmpty() && !prefixPath.isEmpty())
        return {qmake, prefixPath};

    FilePath toolchainFile = config.filePathValueOf(QByteArray("CMAKE_TOOLCHAIN_FILE"));
    if (prefixPath.isEmpty() && toolchainFile.isEmpty())
        return {qmake, QString()};

    // Run a CMake project that would do qmake probing
    TemporaryDirectory qtcQMakeProbeDir("qtc-cmake-qmake-probe-XXXXXXXX");

    FilePath cmakeListTxt(qtcQMakeProbeDir.filePath(Constants::CMAKE_LISTS_TXT));

    cmakeListTxt.writeFileContents(QByteArray(R"(
        cmake_minimum_required(VERSION 3.15)

        project(qmake-probe LANGUAGES NONE)

        # Bypass Qt6's usage of find_dependency, which would require compiler
        # and source code probing, which slows things unnecessarily
        file(WRITE "${CMAKE_SOURCE_DIR}/CMakeFindDependencyMacro.cmake"
        [=[
            macro(find_dependency dep)
            endmacro()
        ]=])
        set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")

        find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
        find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)

        if (CMAKE_CROSSCOMPILING)
            find_program(qmake_binary
                NAMES qmake qmake.bat
                PATHS "${Qt${QT_VERSION_MAJOR}_DIR}/../../../bin"
                NO_DEFAULT_PATH)
            file(WRITE "${CMAKE_SOURCE_DIR}/qmake-location.txt" "${qmake_binary}")
        else()
            file(GENERATE
                OUTPUT "${CMAKE_SOURCE_DIR}/qmake-location.txt"
                CONTENT "$<TARGET_PROPERTY:Qt${QT_VERSION_MAJOR}::qmake,IMPORTED_LOCATION>")
        endif()

        # Remove a Qt CMake hack that adds lib/cmake at the end of every path in CMAKE_PREFIX_PATH
        list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)
        list(TRANSFORM CMAKE_PREFIX_PATH REPLACE "/lib/cmake$" "")
        file(WRITE "${CMAKE_SOURCE_DIR}/cmake-prefix-path.txt" "${CMAKE_PREFIX_PATH}")
    )"));

    Process cmake;
    cmake.setDisableUnixTerminal();

    Environment cmakeEnv(env);
    cmakeEnv.setupEnglishOutput();
    cmake.setEnvironment(cmakeEnv);

    QString cmakeGenerator = config.stringValueOf(QByteArray("CMAKE_GENERATOR"));
    QString cmakeGeneratorPlatform = config.stringValueOf(QByteArray("CMAKE_GENERATOR_PLATFORM"));
    QString cmakeGeneratorToolset = config.stringValueOf(QByteArray("CMAKE_GENERATOR_TOOLSET"));
    FilePath cmakeExecutable = config.filePathValueOf(QByteArray("CMAKE_COMMAND"));
    FilePath cmakeMakeProgram = config.filePathValueOf(QByteArray("CMAKE_MAKE_PROGRAM"));
    FilePath hostPath = config.filePathValueOf(QByteArray("QT_HOST_PATH"));
    const QString findRootPath = config.stringValueOf("CMAKE_FIND_ROOT_PATH");

    QStringList args;
    args.push_back("-S");
    args.push_back(qtcQMakeProbeDir.path().path());
    args.push_back("-B");
    args.push_back(qtcQMakeProbeDir.filePath("build").path());
    if (!cmakeGenerator.isEmpty()) {
        args.push_back("-G");
        args.push_back(cmakeGenerator);
    }
    if (!cmakeGeneratorPlatform.isEmpty()) {
        args.push_back("-A");
        args.push_back(cmakeGeneratorPlatform);
    }
    if (!cmakeGeneratorToolset.isEmpty()) {
        args.push_back("-T");
        args.push_back(cmakeGeneratorToolset);
    }

    if (!cmakeMakeProgram.isEmpty()) {
        args.push_back(QStringLiteral("-DCMAKE_MAKE_PROGRAM=%1").arg(cmakeMakeProgram.toString()));
    }

    if (!toolchainFile.isEmpty()) {
        args.push_back(QStringLiteral("-DCMAKE_TOOLCHAIN_FILE=%1").arg(toolchainFile.toString()));
    }
    if (!prefixPath.isEmpty()) {
        args.push_back(QStringLiteral("-DCMAKE_PREFIX_PATH=%1").arg(prefixPath));
    }
    if (!findRootPath.isEmpty()) {
        args.push_back(QStringLiteral("-DCMAKE_FIND_ROOT_PATH=%1").arg(findRootPath));
    }
    if (!hostPath.isEmpty()) {
        args.push_back(QStringLiteral("-DQT_HOST_PATH=%1").arg(hostPath.toString()));
    }

    qCDebug(cmInputLog) << "CMake probing for qmake path: " << cmakeExecutable.toUserOutput() << args;
    cmake.setCommand({cmakeExecutable, args});
    cmake.runBlocking(5s);

    const FilePath qmakeLocationTxt = qtcQMakeProbeDir.filePath("qmake-location.txt");
    const FilePath qmakeLocation = FilePath::fromUtf8(
        qmakeLocationTxt.fileContents().value_or(QByteArray()));
    qCDebug(cmInputLog) << "qmake location: " << qmakeLocation.toUserOutput();

    const FilePath prefixPathTxt = qtcQMakeProbeDir.filePath("cmake-prefix-path.txt");
    const QString resultedPrefixPath = QString::fromUtf8(
        prefixPathTxt.fileContents().value_or(QByteArray()));
    qCDebug(cmInputLog) << "PrefixPath [after qmake probe]: " << resultedPrefixPath;

    return {qmakeLocation, resultedPrefixPath};
}

static QVector<ToolchainDescription> extractToolchainsFromCache(const CMakeConfig &config)
{
    QVector<ToolchainDescription> result;
    bool haveCCxxCompiler = false;
    for (const CMakeConfigItem &i : config) {
        if (!i.key.startsWith("CMAKE_") || !i.key.endsWith("_COMPILER"))
            continue;
        const QByteArray language = i.key.mid(6, i.key.size() - 6 - 9); // skip "CMAKE_" and "_COMPILER"
        Id languageId;
        if (language == "CXX") {
            haveCCxxCompiler = true;
            languageId = ProjectExplorer::Constants::CXX_LANGUAGE_ID;
        }
        else  if (language == "C") {
            haveCCxxCompiler = true;
            languageId = ProjectExplorer::Constants::C_LANGUAGE_ID;
        }
        else
            languageId = Id::fromName(language);
        result.append({FilePath::fromUtf8(i.value), languageId});
    }

    if (!haveCCxxCompiler) {
        const QByteArray generator = config.valueOf("CMAKE_GENERATOR");
        QString cCompilerName;
        QString cxxCompilerName;
        if (generator.contains("Visual Studio")) {
            cCompilerName = "cl.exe";
            cxxCompilerName = "cl.exe";
        } else if (generator.contains("Xcode")) {
            cCompilerName = "clang";
            cxxCompilerName = "clang++";
        }

        if (!cCompilerName.isEmpty() && !cxxCompilerName.isEmpty()) {
            const FilePath linker = config.filePathValueOf("CMAKE_LINKER");
            if (!linker.isEmpty()) {
                const FilePath compilerPath = linker.parentDir();
                result.append({compilerPath.pathAppended(cCompilerName),
                               ProjectExplorer::Constants::C_LANGUAGE_ID});
                result.append({compilerPath.pathAppended(cxxCompilerName),
                               ProjectExplorer::Constants::CXX_LANGUAGE_ID});
            }
        }
    }

    return result;
}

static QString extractVisualStudioPlatformFromConfig(const CMakeConfig &config)
{
    const QString cmakeGenerator = config.stringValueOf(QByteArray("CMAKE_GENERATOR"));
    QString platform;
    if (cmakeGenerator.contains("Visual Studio")) {
        const FilePath linker = config.filePathValueOf("CMAKE_LINKER");
        const QString toolsDir = linker.parentDir().fileName();
        if (toolsDir.compare("x64", Qt::CaseInsensitive) == 0) {
            platform = "x64";
        } else if (toolsDir.compare("x86", Qt::CaseInsensitive) == 0) {
            platform = "Win32";
        } else if (toolsDir.compare("arm64", Qt::CaseInsensitive) == 0) {
            platform = "ARM64";
        } else if (toolsDir.compare("arm", Qt::CaseInsensitive) == 0) {
            platform = "ARM";
        }
    }

    return platform;
}

void updateCompilerPaths(CMakeConfig &config, const Environment &env)
{
    auto updateRelativePath = [&config, env](const QByteArray &key) {
        FilePath pathValue = config.filePathValueOf(key);

        if (pathValue.isAbsolutePath() || pathValue.isEmpty())
            return;

        pathValue = env.searchInPath(pathValue.fileName());

        auto it = std::find_if(config.begin(), config.end(), [&key](const CMakeConfigItem &item) {
            return item.key == key;
        });
        QTC_ASSERT(it != config.end(), return);

        it->value = pathValue.path().toUtf8();
    };

    updateRelativePath("CMAKE_C_COMPILER");
    updateRelativePath("CMAKE_CXX_COMPILER");
}

void updateConfigWithDirectoryData(CMakeConfig &config, const std::unique_ptr<DirectoryData> &data)
{
    auto updateCompilerValue = [&config, &data](const QByteArray &key, const Utils::Id &language) {
        auto it = std::find_if(config.begin(), config.end(), [&key](const CMakeConfigItem &ci) {
            return ci.key == key;
        });

        auto tcd = Utils::findOrDefault(data->toolchains,
                                        [&language](const ToolchainDescription &t) {
                                            return t.language == language;
                                        });

        if (it != config.end() && it->value.isEmpty())
            it->value = tcd.compilerPath.toString().toUtf8();
        else
            config << CMakeConfigItem(key,
                                      CMakeConfigItem::FILEPATH,
                                      tcd.compilerPath.toString().toUtf8());
    };

    updateCompilerValue("CMAKE_C_COMPILER", ProjectExplorer::Constants::C_LANGUAGE_ID);
    updateCompilerValue("CMAKE_CXX_COMPILER", ProjectExplorer::Constants::CXX_LANGUAGE_ID);

    if (data->qt.qt)
        config << CMakeConfigItem("QT_QMAKE_EXECUTABLE",
                                  CMakeConfigItem::FILEPATH,
                                  data->qt.qt->qmakeFilePath().toString().toUtf8());
}

Toolchain *findExternalToolchain(const QString &presetArchitecture, const QString &presetToolset)
{
    // A compiler path example. Note that the compiler version is not the same version from MsvcToolchain
    // ... \MSVC\14.29.30133\bin\Hostx64\x64\cl.exe
    //
    // And the CMakePresets.json
    //
    // "toolset": {
    //      "value": "v142,host=x64,version=14.29.30133",
    //      "strategy": "external"
    //  },
    //  "architecture": {
    //      "value": "x64",
    //      "strategy": "external"
    //  }

    auto msvcToolchains = ToolchainManager::toolchains([](const Toolchain *tc) {
        return  tc->typeId() ==  ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID;
    });

    const QSet<Abi::OSFlavor> msvcFlavors = Utils::toSet(Utils::transform(msvcToolchains, [](const Toolchain *tc) {
        return tc->targetAbi().osFlavor();
    }));

    return ToolchainManager::toolchain(
        [presetArchitecture, presetToolset, msvcFlavors](const Toolchain *tc) -> bool {
            if (tc->typeId() != ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID)
                return false;

            const FilePath compilerPath = tc->compilerCommand();
            const QString architecture = compilerPath.parentDir().fileName().toLower();
            const QString host
                = compilerPath.parentDir().parentDir().fileName().toLower().replace("host", "host=");
            const QString version
                = QString("version=%1")
                      .arg(compilerPath.parentDir().parentDir().parentDir().parentDir().fileName());

            static std::pair<QString, Abi::OSFlavor> abiTable[] = {
                {QStringLiteral("v143"), Abi::WindowsMsvc2022Flavor},
                {QStringLiteral("v142"), Abi::WindowsMsvc2019Flavor},
                {QStringLiteral("v141"), Abi::WindowsMsvc2017Flavor},
            };

            Abi::OSFlavor toolsetAbi = Abi::UnknownFlavor;
            for (const auto &abiPair : abiTable) {
                if (presetToolset.contains(abiPair.first)) {
                    toolsetAbi = abiPair.second;
                    break;
                }
            }

            // User didn't specify any flavor, so pick the highest toolchain available
            if (toolsetAbi == Abi::UnknownFlavor) {
                for (const auto &abiPair : abiTable) {
                    if (msvcFlavors.contains(abiPair.second)) {
                        toolsetAbi = abiPair.second;
                        break;
                    }
                }
            }

            if (toolsetAbi != tc->targetAbi().osFlavor())
                return false;

            if (presetToolset.contains("host=") && !presetToolset.contains(host))
                return false;

            // Make sure we match also version=14.29
            auto versionIndex = presetToolset.indexOf("version=");
            if (versionIndex != -1 && !version.startsWith(presetToolset.mid(versionIndex)))
                return false;

            if (presetArchitecture != architecture)
                return false;

            qCDebug(cmInputLog) << "For external architecture" << presetArchitecture
                                << "and toolset" << presetToolset
                                << "the following toolchain was selected:\n"
                                << compilerPath.toString();
            return true;
        });
}

QList<void *> CMakeProjectImporter::examineDirectory(const FilePath &importPath,
                                                     QString *warningMessage) const
{
    QList<void *> result;
    qCInfo(cmInputLog) << "Examining directory:" << importPath.toUserOutput();

    if (importPath.isChildOf(m_presetsTempDir.path())) {
        auto data = std::make_unique<DirectoryData>();

        const QString presetName = fileNameToPresetName(importPath.fileName());
        PresetsDetails::ConfigurePreset configurePreset
            = Utils::findOrDefault(m_project->presetsData().configurePresets,
                                   [presetName](const PresetsDetails::ConfigurePreset &preset) {
                                       return preset.name == presetName;
                                   });

        Environment env = projectDirectory().deviceEnvironment();
        CMakePresets::Macros::expand(configurePreset, env, projectDirectory());

        if (configurePreset.displayName)
            data->cmakePresetDisplayname = configurePreset.displayName.value();
        else
            data->cmakePresetDisplayname = configurePreset.name;
        data->cmakePreset = configurePreset.name;

        if (!configurePreset.cmakeExecutable) {
            const CMakeTool *cmakeTool = CMakeToolManager::defaultCMakeTool();
            if (cmakeTool) {
                configurePreset.cmakeExecutable = cmakeTool->cmakeExecutable().toString();
            } else {
                configurePreset.cmakeExecutable = QString();
                TaskHub::addTask(
                    BuildSystemTask(Task::TaskType::Error, Tr::tr("<No CMake Tool available>")));
                TaskHub::requestPopup();
            }
        } else {
            QString cmakeExecutable = configurePreset.cmakeExecutable.value();
            CMakePresets::Macros::expand(configurePreset, env, projectDirectory(), cmakeExecutable);

            configurePreset.cmakeExecutable = FilePath::fromUserInput(cmakeExecutable).path();
        }

        data->cmakeBinary = Utils::FilePath::fromString(configurePreset.cmakeExecutable.value());
        if (configurePreset.generator)
            data->generator = configurePreset.generator.value();

        if (configurePreset.binaryDir) {
            QString binaryDir = configurePreset.binaryDir.value();
            CMakePresets::Macros::expand(configurePreset, env, projectDirectory(), binaryDir);
            data->buildDirectory = Utils::FilePath::fromString(binaryDir);
        }

        const bool architectureExternalStrategy
            = configurePreset.architecture && configurePreset.architecture->strategy
              && configurePreset.architecture->strategy
                     == PresetsDetails::ValueStrategyPair::Strategy::external;

        const bool toolsetExternalStrategy
            = configurePreset.toolset && configurePreset.toolset->strategy
              && configurePreset.toolset->strategy
                     == PresetsDetails::ValueStrategyPair::Strategy::external;

        if (!architectureExternalStrategy && configurePreset.architecture
            && configurePreset.architecture.value().value)
            data->platform = configurePreset.architecture.value().value.value();

        if (!toolsetExternalStrategy && configurePreset.toolset && configurePreset.toolset.value().value)
            data->toolset = configurePreset.toolset.value().value.value();

        if (architectureExternalStrategy && toolsetExternalStrategy) {
            const Toolchain *tc
                = findExternalToolchain(configurePreset.architecture->value.value_or(QString()),
                                        configurePreset.toolset->value.value_or(QString()));
            if (tc)
                tc->addToEnvironment(env);
        }

        CMakePresets::Macros::updateToolchainFile(configurePreset,
                                                  env,
                                                  projectDirectory(),
                                                  data->buildDirectory);

        CMakePresets::Macros::updateCacheVariables(configurePreset, env, projectDirectory());

        const CMakeConfig cache = configurePreset.cacheVariables
                                      ? configurePreset.cacheVariables.value()
                                      : CMakeConfig();
        CMakeConfig config;
        const bool noCompilers = cache.valueOf("CMAKE_C_COMPILER").isEmpty()
                                 && cache.valueOf("CMAKE_CXX_COMPILER").isEmpty();
        if (noCompilers || !configurePreset.generator) {
            QApplication::setOverrideCursor(Qt::WaitCursor);
            config = configurationFromPresetProbe(importPath, projectDirectory(), configurePreset);
            QApplication::restoreOverrideCursor();

            if (!configurePreset.generator) {
                QString cmakeGenerator = config.stringValueOf(QByteArray("CMAKE_GENERATOR"));
                configurePreset.generator = cmakeGenerator;
                data->generator = cmakeGenerator;
                data->platform = extractVisualStudioPlatformFromConfig(config);
                if (!data->platform.isEmpty()) {
                    configurePreset.architecture = PresetsDetails::ValueStrategyPair();
                    configurePreset.architecture->value = data->platform;
                }
            }
        } else {
            config = cache;
            updateCompilerPaths(config, env);
            config << CMakeConfigItem("CMAKE_COMMAND",
                                      CMakeConfigItem::PATH,
                                      configurePreset.cmakeExecutable.value().toUtf8());
            if (configurePreset.generator)
                config << CMakeConfigItem("CMAKE_GENERATOR",
                                          CMakeConfigItem::STRING,
                                          configurePreset.generator.value().toUtf8());
        }

        data->sysroot = config.filePathValueOf("CMAKE_SYSROOT");

        const auto [qmake, cmakePrefixPath] = qtInfoFromCMakeCache(config, env);
        if (!qmake.isEmpty())
            data->qt = findOrCreateQtVersion(qmake);

        if (!cmakePrefixPath.isEmpty() && config.valueOf("CMAKE_PREFIX_PATH").isEmpty())
            config << CMakeConfigItem("CMAKE_PREFIX_PATH",
                                      CMakeConfigItem::PATH,
                                      cmakePrefixPath.toUtf8());

        // Toolchains:
        data->toolchains = extractToolchainsFromCache(config);

        // Update QT_QMAKE_EXECUTABLE and CMAKE_C|XX_COMPILER config values
        updateConfigWithDirectoryData(config, data);

        data->hasQmlDebugging = CMakeBuildConfiguration::hasQmlDebugging(config);

        QByteArrayList buildConfigurationTypes = {cache.valueOf("CMAKE_BUILD_TYPE")};
        if (buildConfigurationTypes.front().isEmpty()) {
            buildConfigurationTypes.clear();
            QByteArray buildConfigurationTypesString = cache.valueOf("CMAKE_CONFIGURATION_TYPES");
            if (!buildConfigurationTypesString.isEmpty()) {
                buildConfigurationTypes = buildConfigurationTypesString.split(';');
            } else {
                for (int type = CMakeBuildConfigurationFactory::BuildTypeDebug;
                     type != CMakeBuildConfigurationFactory::BuildTypeLast;
                     ++type) {
                    BuildInfo info = CMakeBuildConfigurationFactory::createBuildInfo(
                        CMakeBuildConfigurationFactory::BuildType(type));
                    buildConfigurationTypes << info.typeName.toUtf8();
                }
            }
        }
        for (const auto &buildType : buildConfigurationTypes) {
            DirectoryData *newData = new DirectoryData(*data);
            newData->cmakeBuildType = buildType;

            // Handle QML Debugging
            auto type = CMakeBuildConfigurationFactory::buildTypeFromByteArray(
                newData->cmakeBuildType);
            if (type == CMakeBuildConfigurationFactory::BuildTypeDebug
                || type == CMakeBuildConfigurationFactory::BuildTypeProfile)
                newData->hasQmlDebugging = true;

            result.emplace_back(newData);
        }

        return result;
    }

    const FilePath cacheFile = importPath.pathAppended(Constants::CMAKE_CACHE_TXT);

    if (!cacheFile.exists()) {
        qCDebug(cmInputLog) << cacheFile.toUserOutput() << "does not exist, returning.";
        return result;
    }

    QString errorMessage;
    const CMakeConfig config = CMakeConfig::fromFile(cacheFile, &errorMessage);
    if (config.isEmpty() || !errorMessage.isEmpty()) {
        qCDebug(cmInputLog) << "Failed to read configuration from" << cacheFile << errorMessage;
        return result;
    }

    QByteArrayList buildConfigurationTypes = {config.valueOf("CMAKE_BUILD_TYPE")};
    if (buildConfigurationTypes.front().isEmpty()) {
        QByteArray buildConfigurationTypesString = config.valueOf("CMAKE_CONFIGURATION_TYPES");
        if (!buildConfigurationTypesString.isEmpty())
            buildConfigurationTypes = buildConfigurationTypesString.split(';');
    }

    const Environment env = projectDirectory().deviceEnvironment();

    for (auto const &buildType: std::as_const(buildConfigurationTypes)) {
        auto data = std::make_unique<DirectoryData>();

        data->cmakeHomeDirectory =
                FilePath::fromUserInput(config.stringValueOf("CMAKE_HOME_DIRECTORY"))
                    .canonicalPath();
        const FilePath canonicalProjectDirectory = projectDirectory().canonicalPath();
        if (data->cmakeHomeDirectory != canonicalProjectDirectory) {
            *warningMessage = Tr::tr("Unexpected source directory \"%1\", expected \"%2\". "
                                 "This can be correct in some situations, for example when "
                                 "importing a standalone Qt test, but usually this is an error. "
                                 "Import the build anyway?")
                                  .arg(data->cmakeHomeDirectory.toUserOutput(),
                                       canonicalProjectDirectory.toUserOutput());
        }

        data->hasQmlDebugging = CMakeBuildConfiguration::hasQmlDebugging(config);

        data->buildDirectory = importPath;
        data->cmakeBuildType = buildType;

        data->cmakeBinary = config.filePathValueOf("CMAKE_COMMAND");
        data->generator = config.stringValueOf("CMAKE_GENERATOR");
        data->platform = config.stringValueOf("CMAKE_GENERATOR_PLATFORM");
        if (data->platform.isEmpty())
            data->platform = extractVisualStudioPlatformFromConfig(config);
        data->toolset = config.stringValueOf("CMAKE_GENERATOR_TOOLSET");
        data->sysroot = config.filePathValueOf("CMAKE_SYSROOT");

        // Qt:
        const auto info = qtInfoFromCMakeCache(config, env);
        if (!info.qmakePath.isEmpty())
            data->qt = findOrCreateQtVersion(info.qmakePath);

        // Toolchains:
        data->toolchains = extractToolchainsFromCache(config);

        qCInfo(cmInputLog) << "Offering to import" << importPath.toUserOutput();
        result.push_back(static_cast<void *>(data.release()));
    }
    return result;
}

void CMakeProjectImporter::ensureBuildDirectory(DirectoryData &data, const Kit *k) const
{
    if (!data.buildDirectory.isEmpty())
        return;

    const auto cmakeBuildType = CMakeBuildConfigurationFactory::buildTypeFromByteArray(
        data.cmakeBuildType);
    auto buildInfo = CMakeBuildConfigurationFactory::createBuildInfo(cmakeBuildType);

    data.buildDirectory = CMakeBuildConfiguration::shadowBuildDirectory(projectFilePath(),
                                                                        k,
                                                                        buildInfo.typeName,
                                                                        buildInfo.buildType);
}

bool CMakeProjectImporter::matchKit(void *directoryData, const Kit *k) const
{
    DirectoryData *data = static_cast<DirectoryData *>(directoryData);

    CMakeTool *cm = CMakeKitAspect::cmakeTool(k);
    if (!cm || cm->cmakeExecutable() != data->cmakeBinary)
        return false;

    if (CMakeGeneratorKitAspect::generator(k) != data->generator
            || CMakeGeneratorKitAspect::platform(k) != data->platform
            || CMakeGeneratorKitAspect::toolset(k) != data->toolset)
        return false;

    if (SysRootKitAspect::sysRoot(k) != data->sysroot)
        return false;

    if (data->qt.qt && QtSupport::QtKitAspect::qtVersionId(k) != data->qt.qt->uniqueId())
        return false;

    const bool compilersMatch = [k, data] {
        const QList<Id> allLanguages = ToolchainManager::allLanguages();
        for (const ToolchainDescription &tcd : data->toolchains) {
            if (!Utils::contains(allLanguages,
                                 [&tcd](const Id &language) { return language == tcd.language; }))
                continue;
            Toolchain *tc = ToolchainKitAspect::toolchain(k, tcd.language);
            if ((!tc || !tc->matchesCompilerCommand(tcd.compilerPath))) {
                return false;
            }
        }
        return true;
    }();
    const bool noCompilers = [k, data] {
        const QList<Id> allLanguages = ToolchainManager::allLanguages();
        for (const ToolchainDescription &tcd : data->toolchains) {
            if (!Utils::contains(allLanguages,
                                 [&tcd](const Id &language) { return language == tcd.language; }))
                continue;
            Toolchain *tc = ToolchainKitAspect::toolchain(k, tcd.language);
            if (tc && tc->matchesCompilerCommand(tcd.compilerPath)) {
                return false;
            }
        }
        return true;
    }();

    bool haveCMakePreset = false;
    if (!data->cmakePreset.isEmpty()) {
        const auto presetConfigItem = CMakeConfigurationKitAspect::cmakePresetConfigItem(k);

        const QString presetName = presetConfigItem.expandedValue(k);
        if (data->cmakePreset != presetName)
            return false;

        if (!k->unexpandedDisplayName().contains(displayPresetName(data->cmakePresetDisplayname)))
            return false;

        ensureBuildDirectory(*data, k);
        haveCMakePreset = true;
    }

    if (!compilersMatch && !(haveCMakePreset && noCompilers))
        return false;

    qCDebug(cmInputLog) << k->displayName()
                        << "matches directoryData for" << data->buildDirectory.toUserOutput();
    return true;
}

Kit *CMakeProjectImporter::createKit(void *directoryData) const
{
    DirectoryData *data = static_cast<DirectoryData *>(directoryData);

    return QtProjectImporter::createTemporaryKit(data->qt, [&data, this](Kit *k) {
        const CMakeToolData cmtd = findOrCreateCMakeTool(data->cmakeBinary);
        QTC_ASSERT(cmtd.cmakeTool, return);
        if (cmtd.isTemporary)
            addTemporaryData(CMakeKitAspect::id(), cmtd.cmakeTool->id().toSetting(), k);
        CMakeKitAspect::setCMakeTool(k, cmtd.cmakeTool->id());

        CMakeGeneratorKitAspect::setGenerator(k, data->generator);
        CMakeGeneratorKitAspect::setPlatform(k, data->platform);
        CMakeGeneratorKitAspect::setToolset(k, data->toolset);

        SysRootKitAspect::setSysRoot(k, data->sysroot);

        for (const ToolchainDescription &cmtcd : data->toolchains) {
            const ToolchainData tcd = findOrCreateToolchains(cmtcd);
            QTC_ASSERT(!tcd.tcs.isEmpty(), continue);

            if (tcd.areTemporary) {
                for (Toolchain *tc : tcd.tcs)
                    addTemporaryData(ToolchainKitAspect::id(), tc->id(), k);
            }

            ToolchainKitAspect::setToolchain(k, tcd.tcs.at(0));
        }

        if (!data->cmakePresetDisplayname.isEmpty()) {
            k->setUnexpandedDisplayName(displayPresetName(data->cmakePresetDisplayname));

            CMakeConfigurationKitAspect::setCMakePreset(k, data->cmakePreset);
        }
        if (!data->cmakePreset.isEmpty())
            ensureBuildDirectory(*data, k);

        qCInfo(cmInputLog) << "Temporary Kit created.";
    });
}

const QList<BuildInfo> CMakeProjectImporter::buildInfoList(void *directoryData) const
{
    auto data = static_cast<const DirectoryData *>(directoryData);

    // create info:
    CMakeBuildConfigurationFactory::BuildType buildType
        = CMakeBuildConfigurationFactory::buildTypeFromByteArray(data->cmakeBuildType);
    // RelWithDebInfo + QML Debugging = Profile
    if (buildType == CMakeBuildConfigurationFactory::BuildTypeRelWithDebInfo
        && data->hasQmlDebugging)
        buildType = CMakeBuildConfigurationFactory::BuildTypeProfile;
    BuildInfo info = CMakeBuildConfigurationFactory::createBuildInfo(buildType);
    info.buildDirectory = data->buildDirectory;

    QVariantMap config = info.extraInfo.toMap(); // new empty, or existing one from createBuildInfo
    config.insert(Constants::CMAKE_HOME_DIR, data->cmakeHomeDirectory.toVariant());
    // Potentially overwrite the default QML Debugging settings for the build type as set by
    // createBuildInfo, in case we are importing a "Debug" CMake configuration without QML Debugging
    config.insert(Constants::QML_DEBUG_SETTING,
                  data->hasQmlDebugging ? TriState::Enabled.toVariant()
                                        : TriState::Default.toVariant());
    info.extraInfo = config;

    qCDebug(cmInputLog) << "BuildInfo configured.";
    return {info};
}

CMakeProjectImporter::CMakeToolData
CMakeProjectImporter::findOrCreateCMakeTool(const FilePath &cmakeToolPath) const
{
    CMakeToolData result;
    result.cmakeTool = CMakeToolManager::findByCommand(cmakeToolPath);
    if (!result.cmakeTool) {
        qCDebug(cmInputLog) << "Creating temporary CMakeTool for" << cmakeToolPath.toUserOutput();

        UpdateGuard guard(*this);

        auto newTool = std::make_unique<CMakeTool>(CMakeTool::ManualDetection, CMakeTool::createId());
        newTool->setFilePath(cmakeToolPath);
        newTool->setDisplayName(uniqueCMakeToolDisplayName(*newTool));

        result.cmakeTool = newTool.get();
        result.isTemporary = true;
        CMakeToolManager::registerCMakeTool(std::move(newTool));
    }
    return result;
}

void CMakeProjectImporter::deleteDirectoryData(void *directoryData) const
{
    delete static_cast<DirectoryData *>(directoryData);
}

void CMakeProjectImporter::cleanupTemporaryCMake(Kit *k, const QVariantList &vl)
{
    if (vl.isEmpty())
        return; // No temporary CMake
    QTC_ASSERT(vl.count() == 1, return);
    CMakeKitAspect::setCMakeTool(k, Id()); // Always mark Kit as not using this Qt
    CMakeToolManager::deregisterCMakeTool(Id::fromSetting(vl.at(0)));
    qCDebug(cmInputLog) << "Temporary CMake tool cleaned up.";
}

void CMakeProjectImporter::persistTemporaryCMake(Kit *k, const QVariantList &vl)
{
    if (vl.isEmpty())
        return; // No temporary CMake
    QTC_ASSERT(vl.count() == 1, return);
    const QVariant &data = vl.at(0);
    CMakeTool *tmpCmake = CMakeToolManager::findById(Id::fromSetting(data));
    CMakeTool *actualCmake = CMakeKitAspect::cmakeTool(k);

    // User changed Kit away from temporary CMake that was set up:
    if (tmpCmake && actualCmake != tmpCmake)
        CMakeToolManager::deregisterCMakeTool(tmpCmake->id());

    qCDebug(cmInputLog) << "Temporary CMake tool made persistent.";
}

} // CMakeProjectManager::Internal

#ifdef WITH_TESTS

#include <QTest>

namespace CMakeProjectManager::Internal {

class CMakeProjectImporterTest final : public QObject
{
    Q_OBJECT

private slots:
    void testCMakeProjectImporterQt_data();
    void testCMakeProjectImporterQt();

    void testCMakeProjectImporterToolchain_data();
    void testCMakeProjectImporterToolchain();
};

void CMakeProjectImporterTest::testCMakeProjectImporterQt_data()
{
    QTest::addColumn<QStringList>("cache");
    QTest::addColumn<QString>("expectedQmake");

    QTest::newRow("Empty input")
            << QStringList() << QString();

    QTest::newRow("Qt4")
            << QStringList({QString::fromLatin1("QT_QMAKE_EXECUTABLE=/usr/bin/xxx/qmake")})
            << "/usr/bin/xxx/qmake";

    // Everything else will require Qt installations!
}

void CMakeProjectImporterTest::testCMakeProjectImporterQt()
{
    QFETCH(QStringList, cache);
    QFETCH(QString, expectedQmake);

    CMakeConfig config;
    for (const QString &c : std::as_const(cache)) {
        const int pos = c.indexOf('=');
        Q_ASSERT(pos > 0);
        const QString key = c.left(pos);
        const QString value = c.mid(pos + 1);
        config.append(CMakeConfigItem(key.toUtf8(), value.toUtf8()));
    }

    auto [realQmake, cmakePrefixPath] = qtInfoFromCMakeCache(config,
                                                             Environment::systemEnvironment());
    QCOMPARE(realQmake.path(), expectedQmake);
}
void CMakeProjectImporterTest::testCMakeProjectImporterToolchain_data()
{
    QTest::addColumn<QStringList>("cache");
    QTest::addColumn<QByteArrayList>("expectedLanguages");
    QTest::addColumn<QStringList>("expectedToolchains");

    QTest::newRow("Empty input")
            << QStringList() << QByteArrayList() << QStringList();

    QTest::newRow("Unrelated input")
            << QStringList("CMAKE_SOMETHING_ELSE=/tmp") << QByteArrayList() << QStringList();
    QTest::newRow("CXX compiler")
            << QStringList({"CMAKE_CXX_COMPILER=/usr/bin/g++"})
            << QByteArrayList({"Cxx"})
            << QStringList({"/usr/bin/g++"});
    QTest::newRow("CXX compiler, C compiler")
            << QStringList({"CMAKE_CXX_COMPILER=/usr/bin/g++", "CMAKE_C_COMPILER=/usr/bin/clang"})
            << QByteArrayList({"Cxx", "C"})
            << QStringList({"/usr/bin/g++", "/usr/bin/clang"});
    QTest::newRow("CXX compiler, C compiler, strange compiler")
            << QStringList({"CMAKE_CXX_COMPILER=/usr/bin/g++",
                             "CMAKE_C_COMPILER=/usr/bin/clang",
                             "CMAKE_STRANGE_LANGUAGE_COMPILER=/tmp/strange/compiler"})
            << QByteArrayList({"Cxx", "C", "STRANGE_LANGUAGE"})
            << QStringList({"/usr/bin/g++", "/usr/bin/clang", "/tmp/strange/compiler"});
    QTest::newRow("CXX compiler, C compiler, strange compiler (with junk)")
            << QStringList({"FOO=test",
                             "CMAKE_CXX_COMPILER=/usr/bin/g++",
                             "CMAKE_BUILD_TYPE=debug",
                             "CMAKE_C_COMPILER=/usr/bin/clang",
                             "SOMETHING_COMPILER=/usr/bin/something",
                             "CMAKE_STRANGE_LANGUAGE_COMPILER=/tmp/strange/compiler",
                             "BAR=more test"})
            << QByteArrayList({"Cxx", "C", "STRANGE_LANGUAGE"})
            << QStringList({"/usr/bin/g++", "/usr/bin/clang", "/tmp/strange/compiler"});
}

void CMakeProjectImporterTest::testCMakeProjectImporterToolchain()
{
    QFETCH(QStringList, cache);
    QFETCH(QByteArrayList, expectedLanguages);
    QFETCH(QStringList, expectedToolchains);

    QCOMPARE(expectedLanguages.count(), expectedToolchains.count());

    CMakeConfig config;
    for (const QString &c : std::as_const(cache)) {
        const int pos = c.indexOf('=');
        Q_ASSERT(pos > 0);
        const QString key = c.left(pos);
        const QString value = c.mid(pos + 1);
        config.append(CMakeConfigItem(key.toUtf8(), value.toUtf8()));
    }

    const QVector<ToolchainDescription> tcs = extractToolchainsFromCache(config);
    QCOMPARE(tcs.count(), expectedLanguages.count());
    for (int i = 0; i < tcs.count(); ++i) {
        QCOMPARE(tcs.at(i).language, expectedLanguages.at(i));
        QCOMPARE(tcs.at(i).compilerPath.toString(), expectedToolchains.at(i));
    }
}

QObject *createCMakeProjectImporterTest()
{
    return new CMakeProjectImporterTest;
}

} // CMakeProjectManager::Internal

#endif

#include "cmakeprojectimporter.moc"
