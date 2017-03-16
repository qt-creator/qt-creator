/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#include "cpprawprojectpart.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/kitinformation.h>

namespace CppTools {

RawProjectPartFlags::RawProjectPartFlags(const ProjectExplorer::ToolChain *toolChain,
                                         const QStringList &commandLineFlags)
{
    // Keep the following cheap/non-blocking for the ui thread. Expensive
    // operations are encapsulated in ToolChainInfo as "runners".
    if (toolChain) {
        this->commandLineFlags = commandLineFlags;
        warningFlags = toolChain->warningFlags(commandLineFlags);
        compilerFlags = toolChain->compilerFlags(commandLineFlags);
    }
}

void RawProjectPart::setDisplayName(const QString &displayName)
{
    this->displayName = displayName;
}

void RawProjectPart::setFiles(const QStringList &files, FileClassifier fileClassifier)
{
    this->files = files;
    this->fileClassifier = fileClassifier;
}

void RawProjectPart::setProjectFileLocation(const QString &projectFile, int line, int column)
{
    this->projectFile = projectFile;
    projectFileLine = line;
    projectFileColumn = column;
}

void RawProjectPart::setConfigFileName(const QString &configFileName)
{
    this->projectConfigFile = configFileName;
}

void RawProjectPart::setBuildSystemTarget(const QString &target)
{
    buildSystemTarget = target;
}

void RawProjectPart::setCallGroupId(const QString &id)
{
    callGroupId = id;
}

void RawProjectPart::setQtVersion(ProjectPart::QtVersion qtVersion)
{
    this->qtVersion = qtVersion;
}

void RawProjectPart::setDefines(const QByteArray &defines)
{
    this->projectDefines = defines;
}

void RawProjectPart::setHeaderPaths(const ProjectPartHeaderPaths &headerPaths)
{
    this->headerPaths = headerPaths;
}

void RawProjectPart::setIncludePaths(const QStringList &includePaths)
{
    headerPaths.clear();

    foreach (const QString &includeFile, includePaths) {
        ProjectPartHeaderPath hp(includeFile, ProjectPartHeaderPath::IncludePath);

        // The simple project managers are utterly ignorant of frameworks on macOS, and won't report
        // framework paths. The work-around is to check if the include path ends in ".framework",
        // and if so, add the parent directory as framework path.
        if (includeFile.endsWith(QLatin1String(".framework"))) {
            const int slashIdx = includeFile.lastIndexOf(QLatin1Char('/'));
            if (slashIdx != -1) {
                hp = ProjectPartHeaderPath(includeFile.left(slashIdx),
                                             ProjectPartHeaderPath::FrameworkPath);
            }
        }

        headerPaths += hp;
    }
}

void RawProjectPart::setPreCompiledHeaders(const QStringList &preCompiledHeaders)
{
    this->precompiledHeaders = preCompiledHeaders;
}

void RawProjectPart::setSelectedForBuilding(bool yesno)
{
    this->selectedForBuilding = yesno;
}

void RawProjectPart::setFlagsForC(const RawProjectPartFlags &flags)
{
    flagsForC = flags;
}

void RawProjectPart::setFlagsForCxx(const RawProjectPartFlags &flags)
{
    flagsForCxx = flags;
}

} // namespace CppTools
