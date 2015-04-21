/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CMAKEPROJECTMANAGER_INTERNAL_CMAKECBPPARSER_H
#define CMAKEPROJECTMANAGER_INTERNAL_CMAKECBPPARSER_H

#include "cmakeproject.h"

#include <QXmlStreamReader>


namespace ProjectExplorer {
class FileNode;
class Kit;
}

namespace CMakeProjectManager {
namespace Internal {

class CMakeCbpParser : public QXmlStreamReader
{
public:
    bool parseCbpFile(ProjectExplorer::Kit *kit, const QString &fileName, const QString &sourceDirectory);
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

    QMap<Utils::FileName, QString> m_unitTargetMap;
    ProjectExplorer::Kit *m_kit;
    QList<ProjectExplorer::FileNode *> m_fileList;
    QList<ProjectExplorer::FileNode *> m_cmakeFileList;
    QSet<Utils::FileName> m_processedUnits;
    bool m_parsingCmakeUnit;

    CMakeBuildTarget m_buildTarget;
    QList<CMakeBuildTarget> m_buildTargets;
    QString m_projectName;
    QString m_compiler;
    QString m_sourceDirectory;
    QString m_buildDirectory;
    QString m_unitTarget;
};

} // namespace Internal
} // namespace CMakeProjectManager

#endif // CMAKEPROJECTMANAGER_INTERNAL_CMAKECBPPARSER_H
