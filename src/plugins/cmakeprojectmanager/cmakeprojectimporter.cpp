// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "cmakeprojectimporter.h"

#include "cmakebuildconfiguration.h"
#include "cmakekitinformation.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"
#include "cmaketoolmanager.h"
#include "presetsmacros.h"

#include <coreplugin/messagemanager.h>

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchainmanager.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/temporarydirectory.h>

#include <QApplication>
#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

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
    QString extraGenerator;
    QString platform;
    QString toolset;
    FilePath sysroot;
    QtProjectImporter::QtVersionData qt;
    QVector<ToolChainDescription> toolChains;
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

CMakeProjectImporter::CMakeProjectImporter(const FilePath &path, const PresetsData &presetsData)
    : QtProjectImporter(path)
    , m_presetsData(presetsData)
    , m_presetsTempDir("qtc-cmake-presets-XXXXXXXX")
{
    useTemporaryKitAspect(CMakeKitAspect::id(),
                               [this](Kit *k, const QVariantList &vl) { cleanupTemporaryCMake(k, vl); },
                               [this](Kit *k, const QVariantList &vl) { persistTemporaryCMake(k, vl); });

}

FilePaths CMakeProjectImporter::importCandidates()
{
    FilePaths candidates;

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

    for (const auto &configPreset : m_presetsData.configurePresets) {
        if (configPreset.hidden.value())
            continue;

        if (configPreset.condition) {
            if (!CMakePresets::Macros::evaluatePresetCondition(configPreset, projectFilePath()))
                continue;
        }

        const FilePath configPresetDir = m_presetsTempDir.filePath(configPreset.name);
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
            candidates.removeIf(
                [&binaryFilePath] (const FilePath &path) { return path == binaryFilePath; });
        }
    }

    const FilePaths finalists = Utils::filteredUnique(candidates);
    qCInfo(cmInputLog) << "import candidates:" << finalists;
    return finalists;
}

static CMakeConfig configurationFromPresetProbe(
    const FilePath &importPath, const PresetsDetails::ConfigurePreset &configurePreset)
{
    const FilePath cmakeListTxt = importPath / "CMakeLists.txt";
    cmakeListTxt.writeFileContents(QByteArray("cmake_minimum_required(VERSION 3.15)\n"
                                              "\n"
                                              "project(preset-probe)\n"
                                              "\n"));

    QtcProcess cmake;
    cmake.setTimeoutS(30);
    cmake.setDisableUnixTerminal();

    const FilePath cmakeExecutable = FilePath::fromString(configurePreset.cmakeExecutable.value());

    Environment env = cmakeExecutable.deviceEnvironment();
    CMakePresets::Macros::expand(configurePreset, env, importPath);

    env.setupEnglishOutput();
    cmake.setEnvironment(env);
    cmake.setTimeOutMessageBoxEnabled(false);

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
        args.emplace_back("-A");
        args.emplace_back(configurePreset.architecture.value().value.value());
    }
    if (configurePreset.toolset && configurePreset.toolset.value().value) {
        args.emplace_back("-T");
        args.emplace_back(configurePreset.toolset.value().value.value());
    }

    if (configurePreset.cacheVariables) {
        const CMakeConfig cache = configurePreset.cacheVariables
                                      ? configurePreset.cacheVariables.value()
                                      : CMakeConfig();
        const FilePath cmakeMakeProgram = cache.filePathValueOf(QByteArray("CMAKE_MAKE_PROGRAM"));
        const FilePath toolchainFile = cache.filePathValueOf(QByteArray("CMAKE_TOOLCHAIN_FILE"));
        const QString prefixPath = cache.stringValueOf(QByteArray("CMAKE_PREFIX_PATH"));
        const QString findRootPath = cache.stringValueOf(QByteArray("CMAKE_FIND_ROOT_PATH"));
        const QString qtHostPath = cache.stringValueOf(QByteArray("QT_HOST_PATH"));

        if (!cmakeMakeProgram.isEmpty()) {
            args.emplace_back(
                QStringLiteral("-DCMAKE_MAKE_PROGRAM=%1").arg(cmakeMakeProgram.toString()));
        }
        if (!toolchainFile.isEmpty()) {
            args.emplace_back(
                QStringLiteral("-DCMAKE_TOOLCHAIN_FILE=%1").arg(toolchainFile.toString()));
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
    }

    qCDebug(cmInputLog) << "CMake probing for compilers: " << cmakeExecutable.toUserOutput()
                        << args;
    cmake.setCommand({cmakeExecutable, args});
    cmake.runBlocking();

    QString errorMessage;
    const CMakeConfig config = CMakeConfig::fromFile(importPath.pathAppended(
                                                         "build/CMakeCache.txt"),
                                                     &errorMessage);

    return config;
}

static FilePath qmakeFromCMakeCache(const CMakeConfig &config)
{
    // Qt4 way to define things (more convenient for us, so try this first;-)
    const FilePath qmake = config.filePathValueOf("QT_QMAKE_EXECUTABLE");
    qCDebug(cmInputLog) << "QT_QMAKE_EXECUTABLE=" << qmake.toUserOutput();
    if (!qmake.isEmpty())
        return qmake;

    // Check Qt5 settings: oh, the horror!
    const FilePath qtCMakeDir = [config] {
        FilePath tmp = config.filePathValueOf("Qt5Core_DIR");
        if (tmp.isEmpty())
            tmp = config.filePathValueOf("Qt6Core_DIR");
        return tmp;
    }();
    qCDebug(cmInputLog) << "QtXCore_DIR=" << qtCMakeDir.toUserOutput();
    const FilePath canQtCMakeDir = FilePath::fromString(qtCMakeDir.toFileInfo().canonicalFilePath());
    qCInfo(cmInputLog) << "QtXCore_DIR (canonical)=" << canQtCMakeDir.toUserOutput();
    QString prefixPath;
    if (!qtCMakeDir.isEmpty()) {
        prefixPath = canQtCMakeDir.parentDir().parentDir().parentDir().toString(); // Up 3 levels...
    } else {
        prefixPath = config.stringValueOf("CMAKE_PREFIX_PATH");
    }
    qCDebug(cmInputLog) << "PrefixPath:" << prefixPath;

    FilePath toolchainFile = config.filePathValueOf(QByteArray("CMAKE_TOOLCHAIN_FILE"));
    if (prefixPath.isEmpty() && toolchainFile.isEmpty())
        return FilePath();

    // Run a CMake project that would do qmake probing
    TemporaryDirectory qtcQMakeProbeDir("qtc-cmake-qmake-probe-XXXXXXXX");

    QFile cmakeListTxt(qtcQMakeProbeDir.filePath("CMakeLists.txt").toString());
    if (!cmakeListTxt.open(QIODevice::WriteOnly)) {
        return FilePath();
    }
    // FIXME replace by raw string when gcc 8+ is minimum
    cmakeListTxt.write(QByteArray(
"cmake_minimum_required(VERSION 3.15)\n"
"\n"
"project(qmake-probe LANGUAGES NONE)\n"
"\n"
"# Bypass Qt6's usage of find_dependency, which would require compiler\n"
"# and source code probing, which slows things unnecessarily\n"
"file(WRITE \"${CMAKE_SOURCE_DIR}/CMakeFindDependencyMacro.cmake\"\n"
"[=["
"    macro(find_dependency dep)\n"
"    endmacro()\n"
"]=])\n"
"set(CMAKE_MODULE_PATH \"${CMAKE_SOURCE_DIR}\")\n"
"\n"
"find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)\n"
"find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)\n"
"\n"
"if (CMAKE_CROSSCOMPILING)\n"
"    find_program(qmake_binary\n"
"        NAMES qmake qmake.bat\n"
"        PATHS \"${Qt${QT_VERSION_MAJOR}_DIR}/../../../bin\"\n"
"        NO_DEFAULT_PATH)\n"
"    file(WRITE \"${CMAKE_SOURCE_DIR}/qmake-location.txt\" \"${qmake_binary}\")\n"
"else()\n"
"    file(GENERATE\n"
"         OUTPUT \"${CMAKE_SOURCE_DIR}/qmake-location.txt\"\n"
"         CONTENT \"$<TARGET_PROPERTY:Qt${QT_VERSION_MAJOR}::qmake,IMPORTED_LOCATION>\")\n"
"endif()\n"
));
    cmakeListTxt.close();

    QtcProcess cmake;
    cmake.setTimeoutS(5);
    cmake.setDisableUnixTerminal();
    Environment env = Environment::systemEnvironment();
    env.setupEnglishOutput();
    cmake.setEnvironment(env);
    cmake.setTimeOutMessageBoxEnabled(false);

    QString cmakeGenerator = config.stringValueOf(QByteArray("CMAKE_GENERATOR"));
    QString cmakeGeneratorPlatform = config.stringValueOf(QByteArray("CMAKE_GENERATOR_PLATFORM"));
    QString cmakeGeneratorToolset = config.stringValueOf(QByteArray("CMAKE_GENERATOR_TOOLSET"));
    FilePath cmakeExecutable = config.filePathValueOf(QByteArray("CMAKE_COMMAND"));
    FilePath cmakeMakeProgram = config.filePathValueOf(QByteArray("CMAKE_MAKE_PROGRAM"));
    FilePath hostPath = config.filePathValueOf(QByteArray("QT_HOST_PATH"));

    QStringList args;
    args.push_back("-S");
    args.push_back(qtcQMakeProbeDir.path().path());
    args.push_back("-B");
    args.push_back(qtcQMakeProbeDir.filePath("build").path());
    args.push_back("-G");
    args.push_back(cmakeGenerator);
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
    if (toolchainFile.isEmpty()) {
        args.push_back(QStringLiteral("-DCMAKE_PREFIX_PATH=%1").arg(prefixPath));
    } else {
        if (!prefixPath.isEmpty())
            args.push_back(QStringLiteral("-DCMAKE_FIND_ROOT_PATH=%1").arg(prefixPath));
        args.push_back(QStringLiteral("-DCMAKE_TOOLCHAIN_FILE=%1").arg(toolchainFile.toString()));
    }
    if (!hostPath.isEmpty()) {
        args.push_back(QStringLiteral("-DQT_HOST_PATH=%1").arg(hostPath.toString()));
    }

    qCDebug(cmInputLog) << "CMake probing for qmake path: " << cmakeExecutable.toUserOutput() << args;
    cmake.setCommand({cmakeExecutable, args});
    cmake.runBlocking();

    QFile qmakeLocationTxt(qtcQMakeProbeDir.filePath("qmake-location.txt").path());
    if (!qmakeLocationTxt.open(QIODevice::ReadOnly)) {
        return FilePath();
    }
    FilePath qmakeLocation = FilePath::fromUtf8(qmakeLocationTxt.readLine().constData());
    qCDebug(cmInputLog) << "qmake location: " << qmakeLocation.toUserOutput();

    return qmakeLocation;
}

static QVector<ToolChainDescription> extractToolChainsFromCache(const CMakeConfig &config)
{
    QVector<ToolChainDescription> result;
    bool haveCCxxCompiler = false;
    for (const CMakeConfigItem &i : config) {
        if (!i.key.startsWith("CMAKE_") || !i.key.endsWith("_COMPILER"))
            continue;
        const QByteArray language = i.key.mid(6, i.key.count() - 6 - 9); // skip "CMAKE_" and "_COMPILER"
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

QList<void *> CMakeProjectImporter::examineDirectory(const FilePath &importPath,
                                                     QString *warningMessage) const
{
    QList<void *> result;
    qCInfo(cmInputLog) << "Examining directory:" << importPath.toUserOutput();

    if (importPath.isChildOf(m_presetsTempDir.path())) {
        auto data = std::make_unique<DirectoryData>();

        const QString presetName = importPath.fileName();
        PresetsDetails::ConfigurePreset configurePreset
            = Utils::findOrDefault(m_presetsData.configurePresets,
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
            if (cmakeTool)
                configurePreset.cmakeExecutable = cmakeTool->cmakeExecutable().toString();
        }

        data->cmakeBinary = Utils::FilePath::fromString(configurePreset.cmakeExecutable.value());
        if (configurePreset.generator)
            data->generator = configurePreset.generator.value();

        if (configurePreset.architecture && configurePreset.architecture.value().value)
            data->platform = configurePreset.architecture.value().value.value();

        if (configurePreset.toolset && configurePreset.toolset.value().value)
            data->toolset = configurePreset.toolset.value().value.value();

        if (configurePreset.binaryDir) {
            QString binaryDir = configurePreset.binaryDir.value();
            CMakePresets::Macros::expand(configurePreset, env, projectDirectory(), binaryDir);
            data->buildDirectory = Utils::FilePath::fromString(binaryDir);
        }

        CMakePresets::Macros::updateToolchainFile(configurePreset,
                                                  env,
                                                  projectDirectory(),
                                                  data->buildDirectory);

        const CMakeConfig cache = configurePreset.cacheVariables
                                      ? configurePreset.cacheVariables.value()
                                      : CMakeConfig();

        data->sysroot = cache.filePathValueOf("CMAKE_SYSROOT");

        CMakeConfig config;
        if (cache.valueOf("CMAKE_C_COMPILER").isEmpty()
            && cache.valueOf("CMAKE_CXX_COMPILER").isEmpty()) {
            QApplication::setOverrideCursor(Qt::WaitCursor);
            config = configurationFromPresetProbe(importPath, configurePreset);
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
            config << CMakeConfigItem("CMAKE_COMMAND",
                                      CMakeConfigItem::PATH,
                                      configurePreset.cmakeExecutable.value().toUtf8());
            if (configurePreset.generator)
                config << CMakeConfigItem("CMAKE_GENERATOR",
                                          CMakeConfigItem::STRING,
                                          configurePreset.generator.value().toUtf8());
        }

        const FilePath qmake = qmakeFromCMakeCache(config);
        if (!qmake.isEmpty())
            data->qt = findOrCreateQtVersion(qmake);

        // ToolChains:
        data->toolChains = extractToolChainsFromCache(config);

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

            result.emplace_back(newData);
        }

        return result;
    }

    const FilePath cacheFile = importPath.pathAppended("CMakeCache.txt");

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
        data->extraGenerator = config.stringValueOf("CMAKE_EXTRA_GENERATOR");
        data->platform = config.stringValueOf("CMAKE_GENERATOR_PLATFORM");
        if (data->platform.isEmpty())
            data->platform = extractVisualStudioPlatformFromConfig(config);
        data->toolset = config.stringValueOf("CMAKE_GENERATOR_TOOLSET");
        data->sysroot = config.filePathValueOf("CMAKE_SYSROOT");

        // Qt:
        const FilePath qmake = qmakeFromCMakeCache(config);
        if (!qmake.isEmpty())
            data->qt = findOrCreateQtVersion(qmake);

        // ToolChains:
        data->toolChains = extractToolChainsFromCache(config);

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
            || CMakeGeneratorKitAspect::extraGenerator(k) != data->extraGenerator
            || CMakeGeneratorKitAspect::platform(k) != data->platform
            || CMakeGeneratorKitAspect::toolset(k) != data->toolset)
        return false;

    if (SysRootKitAspect::sysRoot(k) != data->sysroot)
        return false;

    if (data->qt.qt && QtSupport::QtKitAspect::qtVersionId(k) != data->qt.qt->uniqueId())
        return false;

    const QList<Id> allLanguages = ToolChainManager::allLanguages();
    for (const ToolChainDescription &tcd : data->toolChains) {
        if (!Utils::contains(allLanguages, [&tcd](const Id& language) {return language == tcd.language;}))
            continue;
        ToolChain *tc = ToolChainKitAspect::toolChain(k, tcd.language);
        if (!tc || !tc->matchesCompilerCommand(tcd.compilerPath)) {
            return false;
        }
    }

    if (!data->cmakePreset.isEmpty()) {
        auto presetConfigItem = CMakeConfigurationKitAspect::cmakePresetConfigItem(k);
        if (data->cmakePreset != presetConfigItem.expandedValue(k))
            return false;

        ensureBuildDirectory(*data, k);
    }

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
        CMakeGeneratorKitAspect::setExtraGenerator(k, data->extraGenerator);
        CMakeGeneratorKitAspect::setPlatform(k, data->platform);
        CMakeGeneratorKitAspect::setToolset(k, data->toolset);

        if (!data->cmakePresetDisplayname.isEmpty()) {
            k->setUnexpandedDisplayName(
                QString("%1 (CMake preset)").arg(data->cmakePresetDisplayname));

            CMakeConfigurationKitAspect::setCMakePreset(k, data->cmakePreset);
        }
        if (!data->cmakePreset.isEmpty())
            ensureBuildDirectory(*data, k);

        SysRootKitAspect::setSysRoot(k, data->sysroot);

        for (const ToolChainDescription &cmtcd : data->toolChains) {
            const ToolChainData tcd = findOrCreateToolChains(cmtcd);
            QTC_ASSERT(!tcd.tcs.isEmpty(), continue);

            if (tcd.areTemporary) {
                for (ToolChain *tc : tcd.tcs)
                    addTemporaryData(ToolChainKitAspect::id(), tc->id(), k);
            }

            ToolChainKitAspect::setToolChain(k, tcd.tcs.at(0));
        }

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
    config.insert(Constants::CMAKE_HOME_DIR, data->cmakeHomeDirectory.toString());
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
    const QVariant data = vl.at(0);
    CMakeTool *tmpCmake = CMakeToolManager::findById(Id::fromSetting(data));
    CMakeTool *actualCmake = CMakeKitAspect::cmakeTool(k);

    // User changed Kit away from temporary CMake that was set up:
    if (tmpCmake && actualCmake != tmpCmake)
        CMakeToolManager::deregisterCMakeTool(tmpCmake->id());

    qCDebug(cmInputLog) << "Temporary CMake tool made persistent.";
}

} // CMakeProjectManager::Internal

#ifdef WITH_TESTS

#include "cmakeprojectplugin.h"

#include <QTest>

namespace CMakeProjectManager {
namespace Internal {

void CMakeProjectPlugin::testCMakeProjectImporterQt_data()
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

void CMakeProjectPlugin::testCMakeProjectImporterQt()
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

    FilePath realQmake = qmakeFromCMakeCache(config);
    QCOMPARE(realQmake.toString(), expectedQmake);
}
void CMakeProjectPlugin::testCMakeProjectImporterToolChain_data()
{
    QTest::addColumn<QStringList>("cache");
    QTest::addColumn<QByteArrayList>("expectedLanguages");
    QTest::addColumn<QStringList>("expectedToolChains");

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

void CMakeProjectPlugin::testCMakeProjectImporterToolChain()
{
    QFETCH(QStringList, cache);
    QFETCH(QByteArrayList, expectedLanguages);
    QFETCH(QStringList, expectedToolChains);

    QCOMPARE(expectedLanguages.count(), expectedToolChains.count());

    CMakeConfig config;
    for (const QString &c : std::as_const(cache)) {
        const int pos = c.indexOf('=');
        Q_ASSERT(pos > 0);
        const QString key = c.left(pos);
        const QString value = c.mid(pos + 1);
        config.append(CMakeConfigItem(key.toUtf8(), value.toUtf8()));
    }

    const QVector<ToolChainDescription> tcs = extractToolChainsFromCache(config);
    QCOMPARE(tcs.count(), expectedLanguages.count());
    for (int i = 0; i < tcs.count(); ++i) {
        QCOMPARE(tcs.at(i).language, expectedLanguages.at(i));
        QCOMPARE(tcs.at(i).compilerPath.toString(), expectedToolChains.at(i));
    }
}

} // namespace Internal
} // namespace CMakeProjectManager

#endif
