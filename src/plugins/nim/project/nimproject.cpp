// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimproject.h"

#include "../nimconstants.h"
#include "../nimtr.h"
#include "nimbuildsystem.h"
#include "nimtoolchain.h"

#include <coreplugin/icontext.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

NimProject::NimProject(const FilePath &fileName) : Project(Constants::C_NIM_MIMETYPE, fileName)
{
    setId(Constants::C_NIMPROJECT_ID);
    setDisplayName(fileName.completeBaseName());
    // ensure debugging is enabled (Nim plugin translates nim code to C code)
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));

    setBuildSystemCreator([](Target *t) { return new NimBuildSystem(t); });
}

Tasks NimProject::projectIssues(const Kit *k) const
{
    Tasks result = Project::projectIssues(k);
    auto tc = dynamic_cast<NimToolChain *>(ToolChainKitAspect::toolChain(k, Constants::C_NIMLANGUAGE_ID));
    if (!tc) {
        result.append(createProjectTask(Task::TaskType::Error, Tr::tr("No Nim compiler set.")));
        return result;
    }
    if (!tc->compilerCommand().exists())
        result.append(createProjectTask(Task::TaskType::Error, Tr::tr("Nim compiler does not exist.")));

    return result;
}

QVariantMap NimProject::toMap() const
{
    QVariantMap result = Project::toMap();
    result[Constants::C_NIMPROJECT_EXCLUDEDFILES] = m_excludedFiles;
    return result;
}

Project::RestoreResult NimProject::fromMap(const QVariantMap &map, QString *errorMessage)
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

} // Nim
