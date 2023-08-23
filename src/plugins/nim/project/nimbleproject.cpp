// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimbleproject.h"
#include "nimconstants.h"
#include "nimblebuildsystem.h"

#include <coreplugin/icontext.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>

#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

NimbleProject::NimbleProject(const Utils::FilePath &fileName)
    : ProjectExplorer::Project(Constants::C_NIMBLE_MIMETYPE, fileName)
{
    setId(Constants::C_NIMBLEPROJECT_ID);
    setDisplayName(fileName.completeBaseName());
    // ensure debugging is enabled (Nim plugin translates nim code to C code)
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setBuildSystemCreator([] (Target *t) { return new NimbleBuildSystem(t); });
}

void NimbleProject::toMap(Store &map) const
{
    Project::toMap(map);
    map[Constants::C_NIMPROJECT_EXCLUDEDFILES] = m_excludedFiles;
}

Project::RestoreResult NimbleProject::fromMap(const Store &map, QString *errorMessage)
{
    auto result = Project::fromMap(map, errorMessage);
    m_excludedFiles = map.value(Constants::C_NIMPROJECT_EXCLUDEDFILES).toStringList();
    return result;
}

QStringList NimbleProject::excludedFiles() const
{
    return m_excludedFiles;
}

void NimbleProject::setExcludedFiles(const QStringList &excludedFiles)
{
    m_excludedFiles = excludedFiles;
}

// Factory

NimbleProjectFactory::NimbleProjectFactory()
{
    ProjectManager::registerProjectType<NimbleProject>(Constants::C_NIMBLE_MIMETYPE);
}

} // Nim
