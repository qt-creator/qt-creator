// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeproject.h"

#include "cmakekitinformation.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectimporter.h"
#include "cmakeprojectmanagertr.h"

#include <coreplugin/icontext.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <qtsupport/qtkitinformation.h>

using namespace ProjectExplorer;
using namespace Utils;
using namespace CMakeProjectManager::Internal;

namespace CMakeProjectManager {

/*!
  \class CMakeProject
*/
CMakeProject::CMakeProject(const FilePath &fileName)
    : Project(Constants::CMAKE_MIMETYPE, fileName)
{
    setId(CMakeProjectManager::Constants::CMAKE_PROJECT_ID);
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(projectDirectory().fileName());
    setCanBuildProducts();

    // This only influences whether 'Install into temporary host directory'
    // will show up by default enabled in some remote deploy configurations.
    // We rely on staging via the actual cmake build step.
    setHasMakeInstallEquivalent(false);

    readPresets();
}

CMakeProject::~CMakeProject()
{
    delete m_projectImporter;
}

Tasks CMakeProject::projectIssues(const Kit *k) const
{
    Tasks result = Project::projectIssues(k);

    if (!CMakeKitAspect::cmakeTool(k))
        result.append(createProjectTask(Task::TaskType::Error, Tr::tr("No cmake tool set.")));
    if (ToolChainKitAspect::toolChains(k).isEmpty())
        result.append(createProjectTask(Task::TaskType::Warning, Tr::tr("No compilers set in kit.")));

    result.append(m_issues);

    return result;
}


ProjectImporter *CMakeProject::projectImporter() const
{
    if (!m_projectImporter)
        m_projectImporter = new CMakeProjectImporter(projectFilePath(), m_presetsData);
    return m_projectImporter;
}

void CMakeProject::addIssue(IssueType type, const QString &text)
{
    m_issues.append(createProjectTask(type, text));
}

void CMakeProject::clearIssues()
{
    m_issues.clear();
}

PresetsData CMakeProject::presetsData() const
{
    return m_presetsData;
}

Internal::PresetsData CMakeProject::combinePresets(Internal::PresetsData &cmakePresetsData,
                                                   Internal::PresetsData &cmakeUserPresetsData)
{
    Internal::PresetsData result;
    result.version = cmakePresetsData.version;
    result.cmakeMinimimRequired = cmakePresetsData.cmakeMinimimRequired;

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
                if ((left.inherits && !right.inherits) || leftInheritsRight || sameInheritance)
                    return false;
                return true;
            });
            for (auto &p : presetsList) {
                if (!p.inherits)
                    continue;

                for (const QString &inheritFromName : p.inherits.value()) {
                    if (presetsHash.contains(inheritFromName)) {
                        p.inheritFrom(presetsHash[inheritFromName]);
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
                TaskHub::addTask(
                    BuildSystemTask(Task::TaskType::Error,
                                    Tr::tr("CMakeUserPresets.json cannot re-define the %1 preset: %2")
                                        .arg(presetType)
                                        .arg(p.name),
                                    "CMakeUserPresets.json"));
                TaskHub::requestPopup();
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

    result.configurePresets = combinePresetsInternal(configurePresetsHash,
                                                     cmakePresetsData.configurePresets,
                                                     cmakeUserPresetsData.configurePresets,
                                                     "configure");
    result.buildPresets = combinePresetsInternal(buildPresetsHash,
                                                 cmakePresetsData.buildPresets,
                                                 cmakeUserPresetsData.buildPresets,
                                                 "build");

    return result;
}

void CMakeProject::setupBuildPresets(Internal::PresetsData &presetsData)
{
    for (auto &buildPreset : presetsData.buildPresets) {
        if (buildPreset.inheritConfigureEnvironment) {
            if (!buildPreset.configurePreset) {
                TaskHub::addTask(BuildSystemTask(
                    Task::TaskType::Error,
                    Tr::tr("Build preset %1 is missing a corresponding configure preset.")
                        .arg(buildPreset.name)));
                TaskHub::requestPopup();
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

void CMakeProject::readPresets()
{
    auto parsePreset = [](const Utils::FilePath &presetFile) -> Internal::PresetsData {
        Internal::PresetsData data;
        Internal::PresetsParser parser;

        QString errorMessage;
        int errorLine = -1;

        if (presetFile.exists()) {
            if (parser.parse(presetFile, errorMessage, errorLine)) {
                data = parser.presetsData();
            } else {
                TaskHub::addTask(BuildSystemTask(Task::TaskType::Error,
                                                 Tr::tr("Failed to load %1: %2")
                                                     .arg(presetFile.fileName())
                                                     .arg(errorMessage),
                                                 presetFile,
                                                 errorLine));
                TaskHub::requestPopup();
            }
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
                            TaskHub::addTask(BuildSystemTask(
                                Task::TaskType::Warning,
                                Tr::tr("Attempt to include %1 which was already parsed.")
                                    .arg(includePath.path()),
                                Utils::FilePath(),
                                -1));
                            TaskHub::requestPopup();
                        } else {
                            resolveIncludes(includeData, includeStack);
                        }
                    }

                    presetData.configurePresets = includeData.configurePresets
                                                  + presetData.configurePresets;
                    presetData.buildPresets = includeData.buildPresets + presetData.buildPresets;

                    includeStack << includePath;
                }
            }
        };

    const Utils::FilePath cmakePresetsJson = projectDirectory().pathAppended("CMakePresets.json");
    const Utils::FilePath cmakeUserPresetsJson = projectDirectory().pathAppended("CMakeUserPresets.json");

    Internal::PresetsData cmakePresetsData = parsePreset(cmakePresetsJson);
    Internal::PresetsData cmakeUserPresetsData = parsePreset(cmakeUserPresetsJson);

    // resolve the include
    Utils::FilePaths includeStack = {cmakePresetsJson};
    resolveIncludes(cmakePresetsData, includeStack);

    includeStack = {cmakeUserPresetsJson};
    resolveIncludes(cmakeUserPresetsData, includeStack);

    m_presetsData = combinePresets(cmakePresetsData, cmakeUserPresetsData);
    setupBuildPresets(m_presetsData);
}

bool CMakeProject::setupTarget(Target *t)
{
    t->updateDefaultBuildConfigurations();
    if (t->buildConfigurations().isEmpty())
        return false;
    t->updateDefaultDeployConfigurations();
    return true;
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

} // namespace CMakeProjectManager
