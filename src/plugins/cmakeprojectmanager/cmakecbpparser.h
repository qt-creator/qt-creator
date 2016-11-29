/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include "cmakeproject.h"
#include "cmaketool.h"

#include <utils/fileutils.h>

#include <QList>
#include <QMap>
#include <QSet>
#include <QString>
#include <QXmlStreamReader>

namespace ProjectExplorer {
class FileNode;
class Kit;
} // namespace ProjectExplorer

namespace CMakeProjectManager {
namespace Internal {

class CMakeCbpParser : public QXmlStreamReader
{
public:
    bool parseCbpFile(CMakeTool::PathMapper mapper, const Utils::FileName &fileName,
                      const Utils::FileName &sourceDirectory);
    QList<ProjectExplorer::FileNode *> fileList();
    QList<ProjectExplorer::FileNode *> cmakeFileList();
    QList<CMakeBuildTarget> buildTargets();
    QString projectName() const;
    QString compilerName() const;
    bool hasCMakeFiles();

private:
    void parseCodeBlocks_project_file();
    void parseProject();
    void parseBuild();
    void parseOption();
    void parseBuildTarget();
    void parseBuildTargetOption();
    void parseMakeCommands();
    void parseBuildTargetBuild();
    void parseBuildTargetClean();
    void parseCompiler();
    void parseAdd();
    void parseUnit();
    void parseUnitOption();
    void parseUnknownElement();
    void sortFiles();

    QMap<Utils::FileName, QStringList> m_unitTargetMap;
    CMakeTool::PathMapper m_pathMapper;
    QList<ProjectExplorer::FileNode *> m_fileList;
    QList<ProjectExplorer::FileNode *> m_cmakeFileList;
    QSet<Utils::FileName> m_processedUnits;
    bool m_parsingCMakeUnit = false;

    CMakeBuildTarget m_buildTarget;
    QList<CMakeBuildTarget> m_buildTargets;
    QString m_projectName;
    QString m_compiler;
    Utils::FileName m_sourceDirectory;
    Utils::FileName m_buildDirectory;
    QStringList m_unitTargets;
};

} // namespace Internal
} // namespace CMakeProjectManager
