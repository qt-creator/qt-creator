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

#include "cpptools_global.h"

#include "cpprawprojectpart.h"
#include "projectpart.h"

#include <projectexplorer/project.h>
#include <projectexplorer/toolchain.h>

#include <QHash>
#include <QPointer>
#include <QSet>
#include <QVector>

namespace CppTools {

class ToolChainInfo
{
public:
    ToolChainInfo() = default;
    ToolChainInfo(const ProjectExplorer::ToolChain *toolChain,
                  const ProjectExplorer::Kit *kit);

    bool isValid() const { return type.isValid(); }

public:
    Core::Id type;
    bool isMsvc2015ToolChain = false;
    unsigned wordWidth = 0;
    QString targetTriple;

    QString sysRoothPath; // For headerPathsRunner.
    ProjectExplorer::ToolChain::SystemHeaderPathsRunner headerPathsRunner;
    ProjectExplorer::ToolChain::PredefinedMacrosRunner predefinedMacrosRunner;
};

class CPPTOOLS_EXPORT ProjectUpdateInfo
{
public:
    ProjectUpdateInfo() = default;
    ProjectUpdateInfo(ProjectExplorer::Project *project,
                      const ProjectExplorer::ToolChain *cToolChain,
                      const ProjectExplorer::ToolChain *cxxToolChain,
                      const ProjectExplorer::Kit *kit,
                      const RawProjectParts &rawProjectParts);
    bool isValid() const { return project && !rawProjectParts.isEmpty(); }

public:
    QPointer<ProjectExplorer::Project> project;
    QVector<RawProjectPart> rawProjectParts;

    const ProjectExplorer::ToolChain *cToolChain = nullptr;
    const ProjectExplorer::ToolChain *cxxToolChain = nullptr;

    ToolChainInfo cToolChainInfo;
    ToolChainInfo cxxToolChainInfo;
};

class CPPTOOLS_EXPORT ProjectInfo
{
public:
    ProjectInfo() = default;
    ProjectInfo(QPointer<ProjectExplorer::Project> project);

    bool isValid() const;

    QPointer<ProjectExplorer::Project> project() const;
    const QVector<ProjectPart::Ptr> projectParts() const;
    const QSet<QString> sourceFiles() const;

    struct CompilerCallGroup {
        using CallsPerSourceFile = QHash<QString, QList<QStringList>>;

        QString groupId;
        CallsPerSourceFile callsPerSourceFile;
    };
    using CompilerCallData = QVector<CompilerCallGroup>;
    void setCompilerCallData(const CompilerCallData &data);
    CompilerCallData compilerCallData() const;

    // Comparisons
    bool operator ==(const ProjectInfo &other) const;
    bool operator !=(const ProjectInfo &other) const;
    bool definesChanged(const ProjectInfo &other) const;
    bool configurationChanged(const ProjectInfo &other) const;
    bool configurationOrFilesChanged(const ProjectInfo &other) const;

    // Construction
    void appendProjectPart(const ProjectPart::Ptr &projectPart);
    void finish();

private:
    QPointer<ProjectExplorer::Project> m_project;
    QVector<ProjectPart::Ptr> m_projectParts;
    CompilerCallData m_compilerCallData;

    // The members below are (re)calculated from the project parts with finish()
    ProjectPartHeaderPaths m_headerPaths;
    QSet<QString> m_sourceFiles;
    QByteArray m_defines;
};

} // namespace CppTools
