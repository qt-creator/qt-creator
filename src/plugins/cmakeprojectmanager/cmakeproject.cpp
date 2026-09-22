// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS
#include <QtTest>
#endif

#include "cmakeproject.h"

#include "cmakebuildsystem.h"
#include "cmakekitaspect.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectimporter.h"
#include "cmakeprojectmanager.h"
#include "cmakeprojectmanagertr.h"
#include "cmakeutils.h"
#include "presetsmacros.h"

#include <coreplugin/icontext.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchainkitaspect.h>

#include <utils/mimeconstants.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;
using namespace CMakeProjectManager::Internal;

namespace CMakeProjectManager {

namespace Internal {

QString packageManagerDir()
{
    return QString(ProjectExplorer::Constants::PROJECT_QTC_DIR) + "/cmake-helper";
}

} // CMakeProjectManager::Internal

static FilePath cmakeListTxtFromFilePath(const FilePath &filepath)
{
    if (filepath.endsWith(Constants::CMAKE_CACHE_TXT)) {
        QString errorMessage;
        const CMakeConfig config = CMakeConfig::fromFile(filepath, &errorMessage);
        const FilePath cmakeListsTxt = config.filePathValueOf("CMAKE_HOME_DIRECTORY")
                                           .pathAppended(Constants::CMAKE_LISTS_TXT);
        if (cmakeListsTxt.exists())
            return cmakeListsTxt;
    }
    return filepath;
}

/*!
  \class CMakeProject
*/
CMakeProject::CMakeProject(const FilePath &fileName)
    : Project(Utils::Constants::CMAKE_MIMETYPE, cmakeListTxtFromFilePath(fileName))
    , m_settings(this, true)
{
    setType(CMakeProjectManager::Constants::CMAKE_PROJECT_ID);
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(projectDisplayName(projectFilePath()));
    setCanBuildProducts();
    setCanBuildFiles();
    setBuildSystemCreator<CMakeBuildSystem>();

    // Allow presets to check if being run under Qt Creator
    Environment::modifySystemEnvironment({{"QTC_RUN", "1"}});

    // This only influences whether 'Install into temporary host directory'
    // will show up by default enabled in some remote deploy configurations.
    // We rely on staging via the actual cmake build step.
    setHasMakeInstallEquivalent(false);

    readPresets();

    if (fileName.endsWith(Constants::CMAKE_CACHE_TXT))
        m_buildDirToImport = fileName.parentDir();

    CMakeProjectImporter *importer = new CMakeProjectImporter(projectFilePath(), this);
    setProjectImporter(importer);
    if (m_presetsData.havePresets)
        importer->createKitsFromPresets();
}

Tasks CMakeProject::projectIssues(const Kit *k) const
{
    Tasks result = Project::projectIssues(k);
    // MCU project issue as it is a CMake project that accept specific kits
    // FIXME remove when the MCUProject is implemented
    // MCUSupport is not a dependency for CMakePlugin
    // This works for projects in installations only, looking at the file content might be costly compared to file path
    static const QRegularExpression mcuRE("QtMCUs/[0-9]+\\.[0-9]+\\.[0-9]+/(demos|examples)");
    const FilePath projectPath = projectFilePath().absoluteFilePath();

    if (k && mcuRE.match(projectPath.path()).hasMatch() && !k->hasFeatures({"MCU"}))
        result << BuildSystemTask(Task::Error, "Kit is not suitable for MCU projects.");

    const CMakeConfigItem presetItem = CMakeConfigurationKitAspect::cmakePresetConfigItem(k);
    if (!presetItem.isNull() && !m_presetsData.havePresets)
        result << BuildSystemTask(Task::Error, "Kit is not suitable for CMake projects that don't use presets.");
    else if (!presetItem.isNull() && m_presetsData.havePresets) {
        if (!presetKitIds().contains(k->id()))
            result << BuildSystemTask(Task::Error, "Kit was created for a different CMake project.");
    }

    result.append(m_issues);
    return result;
}

void CMakeProject::addIssue(IssueType type, const QString &text)
{
    m_issues.append(createTask(type, text));
}

void CMakeProject::clearIssues()
{
    m_issues.clear();
}

PresetsData CMakeProject::presetsData() const
{
    return m_presetsData;
}

QString CMakeProject::projectDisplayName(const Utils::FilePath &projectFilePath)
{
    const QString fallbackDisplayName = projectFilePath.absolutePath().fileName();

    const CMakeLang::DocumentPtr document = parseCMakeFile(projectFilePath);
    if (!document->isValid())
        return fallbackDisplayName;

    QHash<QString, QString> setVariables;
    for (CMakeLang::CommandAST *command : document->commands()) {
        const CMakeLang::ListView<CMakeLang::ArgumentAST *> arguments = command->arguments();

        if (command->isNamed("set") && arguments.size() == 2)
            setVariables.insert(arguments.at(0)->value(), arguments.at(1)->value());

        if (command->isNamed("project") && !arguments.isEmpty()) {
            const QString projectName = arguments.at(0)->value();
            if (projectName.startsWith("${") && projectName.endsWith("}")) {
                const QString projectVar = projectName.mid(2, projectName.size() - 3);
                if (setVariables.contains(projectVar))
                    return setVariables.value(projectVar);
                else
                    return fallbackDisplayName;
            }
            return projectName;
        }
    }

    return fallbackDisplayName;
}

Internal::CMakeSpecificSettings &CMakeProject::settings()
{
    return m_settings;
}

void CMakeProject::readPresets()
{
    auto parsePreset = [](const Utils::FilePath &presetFile) -> Internal::PresetsData {
        Internal::PresetsData data;
        Internal::PresetsParser parser;

        QString errorMessage;
        int errorLine = -1;

        if (parser.parse(presetFile, errorMessage, errorLine)) {
            data = parser.presetsData();
        } else {
            TaskHub::addTask<BuildSystemTask>(
                Task::TaskType::DisruptingError, errorMessage, presetFile, errorLine);
        }
        return data;
    };

    // A file that several files include is read only once, like CMake does. Only a file that
    // includes itself through its own include chain is an error.
    Utils::FilePaths readFiles;
    std::function<void(Internal::PresetsData & presetData, Utils::FilePaths & includeChain)>
        resolveIncludes = [&](Internal::PresetsData &presetData, Utils::FilePaths &includeChain) {
            if (!presetData.include)
                return;

            for (const QString &path : *presetData.include) {
                QString expandedPath = path;
                CMakePresets::Macros::expandFileMacros(projectDirectory(),
                                                       presetData.fileDir,
                                                       expandedPath);

                Utils::FilePath includePath = Utils::FilePath::fromUserInput(expandedPath);
                if (!includePath.isAbsolutePath())
                    includePath = presetData.fileDir.resolvePath(expandedPath);
                includePath = includePath.cleanPath();

                if (includeChain.contains(includePath)) {
                    TaskHub::addTask<BuildSystemTask>(
                        Task::TaskType::DisruptingError,
                        Tr::tr("Attempt to include \"%1\" which was already parsed.")
                            .arg(includePath.path()));
                    presetData.hasValidPresets = false;
                    continue;
                }
                if (readFiles.contains(includePath))
                    continue;
                readFiles << includePath;

                Internal::PresetsData includeData = parsePreset(includePath);

                includeChain << includePath;
                resolveIncludes(includeData, includeChain);
                includeChain.removeLast();

                presetData.configurePresets = includeData.configurePresets
                                              + presetData.configurePresets;
                presetData.buildPresets = includeData.buildPresets + presetData.buildPresets;
                presetData.testPresets = includeData.testPresets + presetData.testPresets;
                presetData.hasValidPresets = includeData.hasValidPresets
                                             && presetData.hasValidPresets;
            }
        };

    const Utils::FilePath cmakePresetsJson = projectDirectory().pathAppended("CMakePresets.json");
    const Utils::FilePath cmakeUserPresetsJson = projectDirectory().pathAppended("CMakeUserPresets.json");

    Internal::PresetsData cmakePresetsData;
    if (cmakePresetsJson.exists())
        cmakePresetsData = parsePreset(cmakePresetsJson);
    Internal::PresetsData cmakeUserPresetsData;
    if (cmakeUserPresetsJson.exists())
        cmakeUserPresetsData = parsePreset(cmakeUserPresetsJson);

    // Both presets are optional, but at least one needs to be present
    if (!cmakePresetsJson.exists() && !cmakeUserPresetsJson.exists())
        return;

    // resolve the include
    readFiles = {cmakePresetsJson, cmakeUserPresetsJson};

    Utils::FilePaths includeChain = {cmakePresetsJson};
    resolveIncludes(cmakePresetsData, includeChain);

    includeChain = {cmakeUserPresetsJson};
    resolveIncludes(cmakeUserPresetsData, includeChain);

    // Watch before the presets are validated, so that correcting a rejected file reloads it.
    m_includeFilesWatcher = FilePath::watch(readFiles);
    for (Result<std::unique_ptr<FilePathWatcher>> &watcher : m_includeFilesWatcher) {
        if (watcher) {
            connect(watcher->get(), &FilePathWatcher::pathChanged, this, [this] {
                // A change reaches several watchers at once, and reloading replaces all of
                // them, so reload once, after they have all finished emitting. Reloading asks
                // a question and creates kits, both of which run a nested event loop, so the
                // flag only drops once that is over: a notification arriving in the meantime
                // would otherwise reload the same project from inside its own reload.
                if (m_presetsReloadPending)
                    return;
                m_presetsReloadPending = true;
                QMetaObject::invokeMethod(
                    this,
                    [this] {
                        Internal::reloadCMakePresets(this);
                        m_presetsReloadPending = false;
                    },
                    Qt::QueuedConnection);
            });
        }
    }

    m_presetsData = Internal::combinePresets(cmakePresetsData, cmakeUserPresetsData);
    Internal::setupBuildPresets(m_presetsData);
    Internal::setupTestPresets(m_presetsData);

    for (const Internal::PresetsError &error : std::as_const(m_presetsData.errors)) {
        TaskHub::addTask<BuildSystemTask>(Task::TaskType::DisruptingError,
                                          error.message,
                                          error.filePath);
    }

    if (!m_presetsData.hasValidPresets) {
        m_presetsData = {};
        m_presetKitIds.clear();
        return;
    }

    m_presetKitIds = Utils::transform(
        m_presetsData.configurePresets,
        [project = projectFilePath().toSettings().toString()](
            const Internal::PresetsDetails::ConfigurePreset &preset) {
            return CMakeConfigurationKitAspect::cmakePresetKitId(project, preset.name);
        });

    for (const auto &configPreset : std::as_const(m_presetsData.configurePresets)) {
        if (configPreset.hidden)
            continue;

        if (configPreset.condition) {
            if (!CMakePresets::Macros::evaluatePresetCondition(configPreset, projectDirectory()))
                continue;
        }
        m_presetsData.havePresets = true;
        break;
    }
}

void CMakeProject::createKitsFromPresets() const
{
    static_cast<CMakeProjectImporter *>(projectImporter())->createKitsFromPresets();
}

const QList<Id> &CMakeProject::presetKitIds() const
{
    return m_presetKitIds;
}

FilePath CMakeProject::buildDirectoryToImport() const
{
    return m_buildDirToImport;
}

#ifdef WITH_TESTS

class TestPresetsInheritance final : public QObject
{
    Q_OBJECT
private slots:

    // QTCREATORBUG-32853
    void testConfigurePresetInheritanceOrder()
    {
        const QByteArray content = R"(
            {
                "version": 3,
                "configurePresets": [
                    {
                        "name": "says-a",
                        "cacheVariables": {
                            "VARIABLE": "a"
                        }
                    },
                    {
                        "name": "says-b",
                        "cacheVariables": {
                            "VARIABLE": "b"
                        }
                    },
                    {
                        "name": "should-say-a",
                        "inherits": [
                            "says-a",
                            "says-b"
                        ]
                    }
                ]
            }
        )";
        const FilePath presetFiles = FilePath::fromUserInput(QDir::tempPath() + "/CMakePresets.json");
        QVERIFY(presetFiles.writeFileContents(content));

        // create a CMakeProject – this will automatically read & combine presets
        CMakeProject project(presetFiles);
        const PresetsData &pd = project.presetsData();

        // locate the child preset
        auto it = std::find_if(
            pd.configurePresets.begin(),
            pd.configurePresets.end(),
            [](const PresetsDetails::ConfigurePreset &p) { return p.name == "should-say-a"; });
        QVERIFY(it != pd.configurePresets.end());

        const PresetsDetails::ConfigurePreset &childPreset = *it;
        QVERIFY(childPreset.cacheVariables);
        const auto &cache = *childPreset.cacheVariables;

        // The value should come from the first preset ("says-a")
        bool found = false;
        for (const auto &item : cache) {
            if (item.key == "VARIABLE") {
                found = true;
                QCOMPARE(QString::fromUtf8(item.value), QString("a"));
            }
        }
        QVERIFY(found);
    }

    void testCyclicInheritance()
    {
        const QByteArray content = R"(
            {
                "version": 3,
                "configurePresets": [
                    {
                        "name": "a",
                        "inherits": "b",
                        "binaryDir": "${sourceDir}/build"
                    },
                    {
                        "name": "b",
                        "inherits": "a",
                        "binaryDir": "${sourceDir}/build"
                    }
                ]
            }
        )";
        const FilePath presetFiles = FilePath::fromUserInput(QDir::tempPath()
                                                             + "/CMakePresets.json");
        QVERIFY(presetFiles.writeFileContents(content));

        // Reading the presets reports the cycle instead of recursing until the stack is gone
        CMakeProject project(presetFiles);
        QVERIFY(!project.presetsData().havePresets);
    }

    void testDiamondInheritanceIsNotCyclic()
    {
        const QByteArray content = R"(
            {
                "version": 3,
                "configurePresets": [
                    {
                        "name": "base",
                        "hidden": true,
                        "binaryDir": "${sourceDir}/build",
                        "cacheVariables": {
                            "FROM_BASE": "yes"
                        }
                    },
                    {
                        "name": "ninja",
                        "hidden": true,
                        "inherits": "base",
                        "generator": "Ninja"
                    },
                    {
                        "name": "vcpkg",
                        "hidden": true,
                        "inherits": "base"
                    },
                    {
                        "name": "dev",
                        "inherits": [
                            "ninja",
                            "vcpkg"
                        ]
                    }
                ]
            }
        )";
        const FilePath presetFiles = FilePath::fromUserInput(QDir::tempPath()
                                                             + "/CMakePresets.json");
        QVERIFY(presetFiles.writeFileContents(content));

        // "base" is reached both through "ninja" and through "vcpkg". That is a diamond, which
        // CMake allows, and not a cycle, so the presets survive and "dev" inherits from "base".
        CMakeProject project(presetFiles);
        const PresetsData &pd = project.presetsData();
        QVERIFY(pd.havePresets);

        auto it = std::find_if(pd.configurePresets.begin(),
                               pd.configurePresets.end(),
                               [](const PresetsDetails::ConfigurePreset &p) {
                                   return p.name == "dev";
                               });
        QVERIFY(it != pd.configurePresets.end());
        QVERIFY(it->cacheVariables);
        QCOMPARE(it->cacheVariables->valueOf("FROM_BASE"), QByteArray("yes"));
    }

    void testFileIncludedTwiceIsReadOnce()
    {
        const QByteArray root = R"(
            {
                "version": 4,
                "include": [
                    "${sourceDir}/include/first.json",
                    "include/second.json"
                ]
            }
        )";
        const QByteArray includer = R"(
            {
                "version": 4,
                "include": [ "common.json" ]
            }
        )";
        const QByteArray common = R"(
            {
                "version": 4,
                "configurePresets": [
                    {
                        "name": "common",
                        "binaryDir": "${sourceDir}/build",
                        "generator": "Ninja"
                    }
                ]
            }
        )";

        const QString tempPath = QDir::tempPath();
        const FilePath presetFiles = FilePath::fromUserInput(tempPath + "/CMakePresets.json");
        QVERIFY(presetFiles.writeFileContents(root));
        for (const QString &name : {QString("first"), QString("second")}) {
            const FilePath file = FilePath::fromUserInput(
                tempPath + "/include/" + name + ".json");
            QVERIFY(file.parentDir().ensureWritableDir());
            QVERIFY(file.writeFileContents(includer));
        }
        const FilePath commonFile = FilePath::fromUserInput(tempPath + "/include/common.json");
        QVERIFY(commonFile.writeFileContents(common));

        CMakeProject project(presetFiles);
        const PresetsData &pd = project.presetsData();

        // Both included files include the same file, whose presets appear only once
        const int commonPresets = Utils::count(pd.configurePresets,
                                               [](const PresetsDetails::ConfigurePreset &p) {
                                                   return p.name == "common";
                                               });
        QCOMPARE(commonPresets, 1);
        QVERIFY(pd.havePresets);
    }

    // QTCREATORBUG-30288
    void testInheritanceFromBasePresets()
    {
        const QByteArray presets = R"(
            {
                "version": 4,
                "include": [
                    "CMake/Platform/Linux/CMakePresets.json",
                    "CMake/Platform/Windows/CMakePresets.json",
                    "CMake/Platform/Mac/CMakePresets.json"
                ],
                "configurePresets": []
            }
        )";

        const QByteArray common = R"(
            {
                "version": 4,
                "configurePresets": [
                    {
                        "name": "default",
                        "description": "Placeholder configuration that buildPresets and testPresets can inherit from",
                        "hidden": true
                    },
                    {
                        "name": "release",
                        "description": "Specifies build type for single-configuration generators: release",
                        "hidden": true,
                        "cacheVariables": {
                            "CMAKE_BUILD_TYPE": {
                                "type": "STRING",
                                "value": "Release"
                            }
                        }
                    },
                    {
                        "name": "compile-commands-json",
                        "description": "Generate compile_commands.json file when used with a Makefile or Ninja Generator",
                        "hidden": true,
                        "cacheVariables": {
                            "CMAKE_EXPORT_COMPILE_COMMANDS": {
                                "type": "BOOL",
                                "value": "ON"
                            }
                        }
                    },
                    {
                        "name": "config-develop",
                        "description": "CMake flags for the deploy version",
                        "hidden": true,
                        "cacheVariables": {
                        }
                    },
                    {
                        "name": "ninja",
                        "displayName": "Ninja",
                        "description": "Configure using Ninja generator",
                        "binaryDir": "${sourceDir}/../build_NestedCMakePresets",
                        "hidden": true,
                        "generator": "Ninja",
                        "inherits": [
                            "compile-commands-json"
                        ]
                    },
                    {
                        "name": "host-windows",
                        "displayName": "Host OS - Windows",
                        "description": "Specifies Windows host condition for configure preset",
                        "hidden": true,
                        "condition": {
                            "type": "equals",
                            "lhs": "${hostSystemName}",
                            "rhs": "Windows"
                        }
                    },
                    {
                        "name": "host-linux",
                        "displayName": "Host OS - Linux",
                        "description": "Specifies Linux host condition for configure preset",
                        "hidden": true,
                        "condition": {
                            "type": "equals",
                            "lhs": "${hostSystemName}",
                            "rhs": "Linux"
                        }
                    },
                    {
                        "name": "host-mac",
                        "displayName": "Host OS - Mac",
                        "description": "Specifies Mac host condition for configure preset",
                        "hidden": true,
                        "condition": {
                            "type": "equals",
                            "lhs": "${hostSystemName}",
                            "rhs": "Darwin"
                        }
                    }
                ]
            }
        )";
        const QByteArray windows = R"(
            {
                "version": 4,
                "include": [
                    "../Common/CMakePresets.json"
                ],
                "configurePresets": [
                    {
                        "name": "windows-base",
                        "hidden": true,
                        "inherits": [
                            "default",
                            "ninja",
                            "host-windows"
                        ],
                        "toolset": {
                            "value": "host=x64",
                            "strategy": "external"
                        },
                        "architecture": {
                            "value": "x64",
                            "strategy": "external"
                        }
                    },
                    {
                        "name": "windows-msvc-base",
                        "hidden": true,
                        "inherits": "windows-base",
                        "cacheVariables": {
                            "CMAKE_C_COMPILER": "cl.exe",
                            "CMAKE_CXX_COMPILER": "cl.exe"
                        }
                    },
                    {
                        "name": "windows-msvc-release",
                        "binaryDir": "${sourceDir}/../build_NestedCMakePresets_MSVC_Release",
                        "inherits": [
                            "config-develop",
                            "windows-msvc-base",
                            "release"
                        ]
                    }
                ]
            }
        )";
        const QByteArray linux = R"(
            {
                "version": 4,
                "include": [
                    "../Common/CMakePresets.json"
                ],
                "configurePresets": [
                    {
                        "name": "linux-base",
                        "hidden": true,
                        "inherits": [
                            "default",
                            "ninja",
                            "host-linux"
                        ],
                        "cacheVariables": {
                            "VCPKG_TARGET_TRIPLET": "x64-linux"
                        }
                    },
                    {
                        "name": "linux-gcc-base",
                        "hidden": true,
                        "description": "Use gold linker to fix linking",
                        "inherits": "linux-base",
                        "cacheVariables": {
                            "CMAKE_CXX_COMPILER": "g++",
                            "CMAKE_C_COMPILER": "gcc",
                            "CMAKE_EXE_LINKER_FLAGS": "-fuse-ld=gold",
                            "CMAKE_CXX_FLAGS": "-fuse-ld=gold"
                        }
                    },
                    {
                        "name": "linux-gcc-release",
                        "binaryDir": "${sourceDir}/../build_NestedCMakePresets_Gcc_Release",
                        "inherits": [
                            "config-develop",
                            "linux-gcc-base",
                            "release"
                        ]
                    }
                ]
            }
        )";
        const QByteArray mac = R"(
            {
                "version": 4,
                "include": [
                    "../Common/CMakePresets.json"
                ],
                "configurePresets": [
                    {
                        "name": "mac-base",
                        "hidden": true,
                        "inherits": [
                            "default",
                            "ninja",
                            "host-mac"
                        ],
                        "cacheVariables": {
                            "VCPKG_OSX_ARCHITECTURES": "arm64;x86_64",
                            "VCPKG_TARGET_TRIPLET": "64-osx-universal"
                        },
                        "condition": {
                            "type": "equals",
                            "lhs": "${hostSystemName}",
                            "rhs": "Darwin"
                        }
                    },
                    {
                        "name": "mac-clang-base",
                        "hidden": true,
                        "inherits": "mac-base",
                        "cacheVariables": {
                            "CMAKE_CXX_COMPILER": "clang++",
                            "CMAKE_C_COMPILER": "clang"
                        }
                    },
                    {
                        "name": "mac-clang-release",
                        "binaryDir": "${sourceDir}/../build_NestedCMakePresets_Clang_Release",
                        "inherits": [
                            "config-develop",
                            "mac-clang-base",
                            "release"
                        ]
                    }
                ]
            }
        )";

        QString tempStringPath = QDir::tempPath();

        const FilePath presetsFiles = FilePath::fromUserInput(tempStringPath + "/CMakePresets.json");
        QVERIFY(presetsFiles.parentDir().ensureWritableDir());
        QVERIFY(presetsFiles.writeFileContents(presets));
        const FilePath commonPresets = FilePath::fromUserInput(
            tempStringPath + "/CMake/Platform/Common/CMakePresets.json");
        QVERIFY(commonPresets.parentDir().ensureWritableDir());
        QVERIFY(commonPresets.writeFileContents(common));
        const FilePath linuxPresets = FilePath::fromUserInput(
            tempStringPath + "/CMake/Platform/Linux/CMakePresets.json");
        QVERIFY(linuxPresets.parentDir().ensureWritableDir());
        QVERIFY(linuxPresets.writeFileContents(linux));
        const FilePath macPresets = FilePath::fromUserInput(
            tempStringPath + "/CMake/Platform/Mac/CMakePresets.json");
        QVERIFY(macPresets.parentDir().ensureWritableDir());
        QVERIFY(macPresets.writeFileContents(mac));
        const FilePath windowsPresets = FilePath::fromUserInput(
            tempStringPath + "/CMake/Platform/Windows/CMakePresets.json");
        QVERIFY(windowsPresets.parentDir().ensureWritableDir());
        QVERIFY(windowsPresets.writeFileContents(windows));

        // create a CMakeProject – this will automatically read & combine presets
        CMakeProject project(presetsFiles);
        const PresetsData &pd = project.presetsData();

        // locate the loaded preset
        QMap<OsType, QPair<QString, bool>> visibleOsMap;
        visibleOsMap[OsType::OsTypeMac] = {"mac-clang-release", false};
        visibleOsMap[OsType::OsTypeLinux] = {"linux-gcc-release", false};
        visibleOsMap[OsType::OsTypeWindows] = {"windows-msvc-release", false};

        visibleOsMap[HostOsInfo::hostOs()].second = true;

        // Only the host os preset should evaluate the "condition" and not be hidden
        for (const QPair<QString, bool> &os : std::as_const(visibleOsMap)) {
            const QString presetName = os.first;

            auto it = std::find_if(
                pd.configurePresets.begin(),
                pd.configurePresets.end(),
                [presetName](const PresetsDetails::ConfigurePreset &p) { return p.name == presetName; });
            QVERIFY(it != pd.configurePresets.end());

            bool visible = false;
            if (it->condition)
                visible
                    = !it->hidden
                      && CMakePresets::Macros::evaluatePresetCondition(*it, presetsFiles.parentDir());
            QCOMPARE(os.second, visible);
        }
    }
};

QObject *createTestPresetsInheritanceTest()
{
    return new TestPresetsInheritance();
}
#include "cmakeproject.moc"

#endif

} // namespace CMakeProjectManager
