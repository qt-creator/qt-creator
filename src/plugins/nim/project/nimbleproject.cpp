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

#include "nimbleproject.h"
#include "nimconstants.h"
#include "nimblebuildsystem.h"

#include <coreplugin/icontext.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcassert.h>

using namespace Nim;
using namespace ProjectExplorer;

NimbleProject::NimbleProject(const Utils::FilePath &fileName)
    : ProjectExplorer::Project(Constants::C_NIMBLE_MIMETYPE, fileName)
{
    setId(Constants::C_NIMBLEPROJECT_ID);
    setDisplayName(fileName.toFileInfo().completeBaseName());
    // ensure debugging is enabled (Nim plugin translates nim code to C code)
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setBuildSystemCreator([] (Target *t) { return new NimbleBuildSystem(t); });
}

QVariantMap NimbleProject::toMap() const
{
    QVariantMap result = Project::toMap();
    result[Constants::C_NIMPROJECT_EXCLUDEDFILES] = m_excludedFiles;
    return result;
}

Project::RestoreResult NimbleProject::fromMap(const QVariantMap &map, QString *errorMessage)
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


