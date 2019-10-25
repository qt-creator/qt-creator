/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "nimproject.h"

#include "../nimconstants.h"
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
    setDisplayName(fileName.toFileInfo().completeBaseName());
    // ensure debugging is enabled (Nim plugin translates nim code to C code)
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));

    setBuildSystemCreator([](Target *t) { return new NimBuildSystem(t); });
}

Tasks NimProject::projectIssues(const Kit *k) const
{
    Tasks result = Project::projectIssues(k);
    auto tc = dynamic_cast<NimToolChain *>(ToolChainKitAspect::toolChain(k, Constants::C_NIMLANGUAGE_ID));
    if (!tc) {
        result.append(createProjectTask(Task::TaskType::Error, tr("No Nim compiler set.")));
        return result;
    }
    if (!tc->compilerCommand().exists())
        result.append(createProjectTask(Task::TaskType::Error, tr("Nim compiler does not exist.")));

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

} // namespace Nim
