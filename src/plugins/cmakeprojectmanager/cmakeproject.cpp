// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "cmakeproject.h"

#include "cmakekitinformation.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectimporter.h"
#include "cmaketool.h"

#include <coreplugin/icontext.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {

using namespace Internal;

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
    setHasMakeInstallEquivalent(true);

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
        result.append(createProjectTask(Task::TaskType::Error, tr("No cmake tool set.")));
    if (ToolChainKitAspect::toolChains(k).isEmpty())
        result.append(createProjectTask(Task::TaskType::Warning, tr("No compilers set in kit.")));

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

    QHash<QString, PresetsDetails::ConfigurePreset> configurePresets;

    // Populate the hash map with the CMakePresets
    for (const PresetsDetails::ConfigurePreset &p: cmakePresetsData.configurePresets)
        configurePresets.insert(p.name, p);

    auto resolveInherits =
        [configurePresets](std::vector<PresetsDetails::ConfigurePreset> &configurePresetsList) {
            for (PresetsDetails::ConfigurePreset &cp : configurePresetsList) {
                if (!cp.inherits)
                    continue;

                for (const QString &inheritFromName : cp.inherits.value())
                    if (configurePresets.contains(inheritFromName))
                        cp.inheritFrom(configurePresets[inheritFromName]);
            }
        };

    // First resolve the CMakePresets
    resolveInherits(cmakePresetsData.configurePresets);

    // Add the CMakeUserPresets to the resolve hash map
    for (const PresetsDetails::ConfigurePreset &cp : cmakeUserPresetsData.configurePresets) {
        if (configurePresets.contains(cp.name)) {
            TaskHub::addTask(BuildSystemTask(
                Task::TaskType::Error,
                tr("CMakeUserPresets.json cannot re-define the configure preset: %1").arg(cp.name),
                "CMakeUserPresets.json"));
            TaskHub::requestPopup();
        } else {
            configurePresets.insert(cp.name, cp);
        }
    }

    // Then resolve the CMakeUserPresets
    resolveInherits(cmakeUserPresetsData.configurePresets);

    // Get both CMakePresets and CMakeUserPresets into the result
    result.configurePresets = cmakePresetsData.configurePresets;

    // std::vector doesn't have append
    std::copy(cmakeUserPresetsData.configurePresets.begin(),
              cmakeUserPresetsData.configurePresets.end(),
              std::back_inserter(result.configurePresets));

    return result;
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
                                                 tr("Failed to load %1: %2")
                                                     .arg(presetFile.fileName())
                                                     .arg(errorMessage),
                                                 presetFile,
                                                 errorLine));
                TaskHub::requestPopup();
            }
        }
        return data;
    };

    const Utils::FilePath cmakePresetsJson = projectDirectory().pathAppended("CMakePresets.json");
    const Utils::FilePath cmakeUserPresetsJson = projectDirectory().pathAppended("CMakeUserPresets.json");

    Internal::PresetsData cmakePresetsData = parsePreset(cmakePresetsJson);
    Internal::PresetsData cmakeUserPresetsData = parsePreset(cmakeUserPresetsJson);

    m_presetsData = combinePresets(cmakePresetsData, cmakeUserPresetsData);
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

} // namespace CMakeProjectManager
