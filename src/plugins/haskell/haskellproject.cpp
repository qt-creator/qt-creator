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

#include "haskellproject.h"

#include "haskellconstants.h"

#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QFile>
#include <QTextStream>

using namespace ProjectExplorer;
using namespace Utils;

namespace Haskell {
namespace Internal {

static QVector<QString> parseExecutableNames(const FilePath &projectFilePath)
{
    static const QString EXECUTABLE = "executable";
    static const int EXECUTABLE_LEN = EXECUTABLE.length();
    QVector<QString> result;
    QFile file(projectFilePath.toString());
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            const QString line = stream.readLine().trimmed();
            if (line.length() > EXECUTABLE_LEN && line.startsWith(EXECUTABLE)
                    && line.at(EXECUTABLE_LEN).isSpace())
                result.append(line.mid(EXECUTABLE_LEN + 1).trimmed());
        }
    }
    return result;
}

HaskellProject::HaskellProject(const Utils::FilePath &fileName)
    : Project(Constants::C_HASKELL_PROJECT_MIMETYPE, fileName)
{
    setId(Constants::C_HASKELL_PROJECT_ID);
    setDisplayName(fileName.toFileInfo().completeBaseName());
    setBuildSystemCreator([](Target *t) { return new HaskellBuildSystem(t); });
}

bool HaskellProject::isHaskellProject(Project *project)
{
    return project && project->id() == Constants::C_HASKELL_PROJECT_ID;
}

HaskellBuildSystem::HaskellBuildSystem(Target *t)
    : BuildSystem(t)
{
    connect(&m_scanner, &TreeScanner::finished, this, [this] {
        auto root = std::make_unique<ProjectNode>(projectDirectory());
        root->setDisplayName(target()->project()->displayName());
        std::vector<std::unique_ptr<FileNode>> nodePtrs
            = Utils::transform<std::vector>(m_scanner.release().allFiles, [](FileNode *fn) {
                  return std::unique_ptr<FileNode>(fn);
              });
        root->addNestedNodes(std::move(nodePtrs));
        setRootProjectNode(std::move(root));

        updateApplicationTargets();

        m_parseGuard.markAsSuccess();
        m_parseGuard = {};

        emitBuildSystemUpdated();
    });

    connect(target()->project(),
            &Project::projectFileIsDirty,
            this,
            &BuildSystem::requestDelayedParse);

    requestDelayedParse();
}

void HaskellBuildSystem::triggerParsing()
{
    m_parseGuard = guardParsingRun();
    m_scanner.asyncScanForFiles(target()->project()->projectDirectory());
}

void HaskellBuildSystem::updateApplicationTargets()
{
    const QVector<QString> executables = parseExecutableNames(projectFilePath());
    const Utils::FilePath projFilePath = projectFilePath();
    const QList<BuildTargetInfo> appTargets
        = Utils::transform<QList>(executables, [projFilePath](const QString &executable) {
              BuildTargetInfo bti;
              bti.displayName = executable;
              bti.buildKey = executable;
              bti.targetFilePath = FilePath::fromString(executable);
              bti.projectFilePath = projFilePath;
              bti.isQtcRunnable = true;
              return bti;
          });
    setApplicationTargets(appTargets);
    target()->updateDefaultRunConfigurations();
}

} // namespace Internal
} // namespace Haskell
