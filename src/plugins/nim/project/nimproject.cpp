// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimproject.h"

#include "../nimconstants.h"
#include "../nimtr.h"
#include "nimbuildsystem.h"

#include <coreplugin/icontext.h>

#include <projectexplorer/kitaspects.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/toolchain.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

class NimProject : public Project
{
public:
    explicit NimProject(const FilePath &filePath);

    Tasks projectIssues(const Kit *k) const final;

    // Keep for compatibility with Qt Creator 4.10
    void toMap(Store &map) const final;

    QStringList excludedFiles() const;
    void setExcludedFiles(const QStringList &excludedFiles);

protected:
    // Keep for compatibility with Qt Creator 4.10
    RestoreResult fromMap(const Store &map, QString *errorMessage) final;

    QStringList m_excludedFiles;
};

NimProject::NimProject(const FilePath &filePath) : Project(Constants::C_NIM_MIMETYPE, filePath)
{
    setId(Constants::C_NIMPROJECT_ID);
    setDisplayName(filePath.completeBaseName());
    // ensure debugging is enabled (Nim plugin translates nim code to C code)
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));

    setBuildSystemCreator([](Target *t) { return new NimBuildSystem(t); });
}

Tasks NimProject::projectIssues(const Kit *k) const
{
    Tasks result = Project::projectIssues(k);
    auto tc = ToolChainKitAspect::toolChain(k, Constants::C_NIMLANGUAGE_ID);
    if (!tc) {
        result.append(createProjectTask(Task::TaskType::Error, Tr::tr("No Nim compiler set.")));
        return result;
    }
    if (!tc->compilerCommand().exists())
        result.append(createProjectTask(Task::TaskType::Error, Tr::tr("Nim compiler does not exist.")));

    return result;
}

void NimProject::toMap(Store &map) const
{
    Project::toMap(map);
    map[Constants::C_NIMPROJECT_EXCLUDEDFILES] = m_excludedFiles;
}

Project::RestoreResult NimProject::fromMap(const Store &map, QString *errorMessage)
{
    auto result = Project::fromMap(map, errorMessage);
    m_excludedFiles = map.value(Constants::C_NIMPROJECT_EXCLUDEDFILES).toStringList();
    return result;
}

QStringList NimProject::excludedFiles() const
{
    return m_excludedFiles;
}

void NimProject::setExcludedFiles(const QStringList &excludedFiles)
{
    m_excludedFiles = excludedFiles;
}

// Factory

NimProjectFactory::NimProjectFactory()
{
    ProjectManager::registerProjectType<NimProject>(Constants::C_NIM_PROJECT_MIMETYPE);
}

} // Nim
