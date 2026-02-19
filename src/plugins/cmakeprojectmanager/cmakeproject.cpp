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
#include "cmakeprojectmanagertr.h"
#include "presetsmacros.h"

#include <coreplugin/icontext.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchainkitaspect.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/mimeconstants.h>
#include <utils/textfileformat.h>

using namespace ProjectExplorer;
using namespace Utils;
using namespace CMakeProjectManager::Internal;

namespace CMakeProjectManager {

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
}

CMakeProject::~CMakeProject()
{
    delete m_projectImporter;
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

    result.append(m_issues);
    return result;
}

ProjectImporter *CMakeProject::projectImporter() const
{
    if (!m_projectImporter)
        m_projectImporter = new CMakeProjectImporter(projectFilePath(), this);
    return m_projectImporter;
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

template<typename T>
static QStringList recursiveInheritsList(const T &presetsHash, const QStringList &inheritsList)
{
    QStringList result;
    for (const QString &inheritFrom : inheritsList) {
        result << inheritFrom;
        if (presetsHash.contains(inheritFrom)) {
            auto item = presetsHash[inheritFrom];
            if (item.inherits)
                result << recursiveInheritsList(presetsHash, item.inherits.value());
        }
    }
    return result;
}

Internal::PresetsData CMakeProject::combinePresets(Internal::PresetsData &cmakePresetsData,
                                                   Internal::PresetsData &cmakeUserPresetsData)
{
    Internal::PresetsData result;
    result.version = cmakePresetsData.version;
    result.cmakeMinimimRequired = cmakePresetsData.cmakeMinimimRequired;

    result.include = cmakePresetsData.include;
    if (result.include) {
        if (cmakeUserPresetsData.include)
            result.include->append(cmakeUserPresetsData.include.value());
    } else {
        result.include = cmakeUserPresetsData.include;
    }

    result.vendor = cmakePresetsData.vendor;
    if (result.vendor) {
        if (cmakeUserPresetsData.vendor)
            result.vendor->insert(cmakeUserPresetsData.vendor.value());
    } else {
        result.vendor = cmakeUserPresetsData.vendor;
    }

    result.hasValidPresets = cmakePresetsData.hasValidPresets && cmakeUserPresetsData.hasValidPresets;

    auto combinePresetsInternal = [](auto &presetsHash,
                                     auto &presets,
                                     auto &userPresets,
                                     const QString &presetType) {
        // Populate the hash map with the CMakePresets
        for (const auto &p : presets)
            presetsHash.insert(p.name, p);

        auto resolveInherits = [](auto &presetsHash, auto &presetsList) {
            Utils::sort(presetsList, [](const auto &left, const auto &right) {
                const bool sameInheritance = left.inherits && right.inherits
                                             && left.inherits.value() == right.inherits.value();
                const bool leftInheritsRight = left.inherits
                                               && left.inherits.value().contains(right.name);

                const bool inheritsGreater = left.inherits && right.inherits
                                             && !left.inherits.value().isEmpty()
                                             && !right.inherits.value().isEmpty()
                                             && left.inherits.value().first()
                                                    > right.inherits.value().first();

                const bool noInheritsGreaterEqual = !left.inherits &&
                                                    !right.inherits &&
                                                    left.name >= right.name;

                if ((left.inherits && !right.inherits) || leftInheritsRight || sameInheritance
                    || inheritsGreater || noInheritsGreaterEqual)
                    return false;
                return true;
            });
            for (auto &p : presetsList) {
                if (!p.inherits)
                    continue;

                const QStringList inheritsList = recursiveInheritsList(presetsHash,
                                                                       p.inherits.value());
                for (const QString &inheritFrom : inheritsList) {
                    if (presetsHash.contains(inheritFrom)) {
                        p.inheritFrom(presetsHash[inheritFrom]);
                        presetsHash[p.name] = p;
                    }
                }
            }
        };

        // First resolve the CMakePresets
        resolveInherits(presetsHash, presets);

        // Add the CMakeUserPresets to the resolve hash map
        for (const auto &p : userPresets) {
            if (presetsHash.contains(p.name)) {
                TaskHub::addTask<BuildSystemTask>(
                    Task::TaskType::DisruptingError,
                    Tr::tr("CMakeUserPresets.json cannot re-define the %1 preset: %2")
                        .arg(presetType)
                        .arg(p.name),
                    FilePath::fromString("CMakeUserPresets.json"));
            } else {
                presetsHash.insert(p.name, p);
            }
        }

        // Then resolve the CMakeUserPresets
        resolveInherits(presetsHash, userPresets);

        // Get both CMakePresets and CMakeUserPresets into the result
        auto result = presets;

        // std::vector doesn't have append
        std::copy(userPresets.begin(), userPresets.end(), std::back_inserter(result));
        return result;
    };

    QHash<QString, PresetsDetails::ConfigurePreset> configurePresetsHash;
    QHash<QString, PresetsDetails::BuildPreset> buildPresetsHash;
    QHash<QString, PresetsDetails::TestPreset> testPresetsHash;

    result.configurePresets = combinePresetsInternal(configurePresetsHash,
                                                     cmakePresetsData.configurePresets,
                                                     cmakeUserPresetsData.configurePresets,
                                                     "configure");
    result.buildPresets = combinePresetsInternal(buildPresetsHash,
                                                 cmakePresetsData.buildPresets,
                                                 cmakeUserPresetsData.buildPresets,
                                                 "build");
    result.testPresets = combinePresetsInternal(
        testPresetsHash, cmakePresetsData.testPresets, cmakeUserPresetsData.testPresets, "test");

    return result;
}

void CMakeProject::setupBuildPresets(Internal::PresetsData &presetsData)
{
    for (auto &buildPreset : presetsData.buildPresets) {
        if (buildPreset.inheritConfigureEnvironment) {
            if (!buildPreset.configurePreset && !buildPreset.hidden) {
                TaskHub::addTask<BuildSystemTask>(
                    Task::TaskType::DisruptingError,
                    Tr::tr("Build preset %1 is missing a corresponding configure preset.")
                        .arg(buildPreset.name));
                presetsData.hasValidPresets = false;
            }

            const QString &configurePresetName = buildPreset.configurePreset.value_or(QString());
            buildPreset.environment
                = Utils::findOrDefault(presetsData.configurePresets,
                                       [configurePresetName](
                                           const PresetsDetails::ConfigurePreset &configurePreset) {
                                           return configurePresetName == configurePreset.name;
                                       })
                      .environment;
        }
    }
}

void CMakeProject::setupTestPresets(Internal::PresetsData &presetsData)
{
    for (auto &testPreset : presetsData.testPresets) {
        if (testPreset.inheritConfigureEnvironment) {
            if (!testPreset.configurePreset && !testPreset.hidden) {
                TaskHub::addTask<BuildSystemTask>(
                    Task::TaskType::DisruptingError,
                    Tr::tr("Test preset %1 is missing a corresponding configure preset.")
                        .arg(testPreset.name));
                presetsData.hasValidPresets = false;
            }

            const QString &configurePresetName = testPreset.configurePreset.value_or(QString());
            testPreset.environment
                = Utils::findOrDefault(presetsData.configurePresets,
                                       [configurePresetName](
                                           const PresetsDetails::ConfigurePreset &configurePreset) {
                                           return configurePresetName == configurePreset.name;
                                       })
                      .environment;
        }
    }
}


QString CMakeProject::projectDisplayName(const Utils::FilePath &projectFilePath)
{
    const QString fallbackDisplayName = projectFilePath.absolutePath().fileName();

    QByteArray fileContent;
    cmListFile cmakeListFile;
    std::string errorString;
    if (TextFileFormat::readFileUtf8(projectFilePath, TextEncoding::Utf8, &fileContent)) {
        if (!cmakeListFile.ParseString(
                fileContent.toStdString(), projectFilePath.fileName().toStdString(), errorString)) {
            return fallbackDisplayName;
        }
    }

    QHash<QString, QString> setVariables;
    for (const auto &func : cmakeListFile.Functions) {
        if (func.LowerCaseName() == "set" && func.Arguments().size() == 2)
            setVariables.insert(
                QString::fromUtf8(func.Arguments()[0].Value),
                QString::fromUtf8(func.Arguments()[1].Value));

        if (func.LowerCaseName() == "project" && func.Arguments().size() > 0) {
            const QString projectName = QString::fromUtf8(func.Arguments()[0].Value);
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

    std::function<void(Internal::PresetsData & presetData, Utils::FilePaths & inclueStack)>
        resolveIncludes = [&](Internal::PresetsData &presetData, Utils::FilePaths &includeStack) {
            if (presetData.include) {
                for (const QString &path : presetData.include.value()) {
                    Utils::FilePath includePath = Utils::FilePath::fromUserInput(path);
                    if (!includePath.isAbsolutePath())
                        includePath = presetData.fileDir.resolvePath(path);

                    Internal::PresetsData includeData = parsePreset(includePath);
                    if (includeData.include) {
                        if (includeStack.contains(includePath)) {
                            TaskHub::addTask<BuildSystemTask>(
                                Task::TaskType::Warning,
                                Tr::tr("Attempt to include \"%1\" which was already parsed.")
                                    .arg(includePath.path()),
                                Utils::FilePath(),
                                -1);
                            TaskHub::requestPopup();
                        } else {
                            resolveIncludes(includeData, includeStack);
                        }
                    }

                    presetData.configurePresets = includeData.configurePresets
                                                  + presetData.configurePresets;
                    presetData.buildPresets = includeData.buildPresets + presetData.buildPresets;
                    presetData.testPresets = includeData.testPresets + presetData.testPresets;
                    presetData.hasValidPresets = includeData.hasValidPresets && presetData.hasValidPresets;

                    includeStack << includePath;
                }
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
    Utils::FilePaths includeStack = {cmakePresetsJson};
    resolveIncludes(cmakePresetsData, includeStack);

    includeStack = {cmakeUserPresetsJson};
    resolveIncludes(cmakeUserPresetsData, includeStack);

    m_presetsData = combinePresets(cmakePresetsData, cmakeUserPresetsData);
    setupBuildPresets(m_presetsData);
    setupTestPresets(m_presetsData);

    if (!m_presetsData.hasValidPresets) {
        m_presetsData = {};
        return;
    }

    for (const auto &configPreset : std::as_const(m_presetsData.configurePresets)) {
        if (configPreset.hidden)
            continue;

        if (configPreset.condition) {
            if (!CMakePresets::Macros::evaluatePresetCondition(configPreset, projectFilePath()))
                continue;
        }
        m_presetsData.havePresets = true;
        break;
    }
}

FilePath CMakeProject::buildDirectoryToImport() const
{
    return m_buildDirToImport;
}

ProjectExplorer::DeploymentKnowledge CMakeProject::deploymentKnowledge() const
{
    return !files([](const ProjectExplorer::Node *n) {
                return n->filePath().fileName() == "QtCreatorDeployment.txt";
            })
                   .isEmpty()
               ? DeploymentKnowledge::Approximative
               : DeploymentKnowledge::Bad;
}

void CMakeProject::configureAsExampleProject(ProjectExplorer::Kit *kit)
{
    QList<BuildInfo> infoList;
    const QList<Kit *> kits(kit != nullptr ? QList<Kit *>({kit}) : KitManager::kits());
    for (Kit *k : kits) {
        if (QtSupport::QtKitAspect::qtVersion(k) != nullptr) {
            if (auto factory = BuildConfigurationFactory::find(k, projectFilePath()))
                infoList << factory->allAvailableSetups(k, projectFilePath());
        }
    }
    setup(infoList);
}

void CMakeProjectManager::CMakeProject::setOldPresetKits(
    const QList<ProjectExplorer::Kit *> &presetKits) const
{
    m_oldPresetKits = presetKits;
}

QList<Kit *> CMakeProject::oldPresetKits() const
{
    return m_oldPresetKits;
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
        const auto &cache = childPreset.cacheVariables.value();

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
        for (const QPair<QString, bool> &os : visibleOsMap) {
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
