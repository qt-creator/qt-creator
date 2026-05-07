// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gninfoparser.h"
#include "gnprojectnodes.h"
#include "gntarget.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/rawprojectpart.h>

#include <utils/processinterface.h>
#include <utils/qtcprocess.h>

#include <QElapsedTimer>

namespace GNProjectManager::Internal {

class GNProjectParser : public QObject
{
    Q_OBJECT

    struct ParserData
    {
        GNInfoParser::Result data;
        std::unique_ptr<GNProjectNode> rootNode;
    };

public:
    GNProjectParser(ProjectExplorer::Project *project);

    bool generate(const Utils::FilePath &gnExe,
                  const Utils::FilePath &sourcePath,
                  const Utils::FilePath &buildPath,
                  const Utils::Environment &env,
                  const QStringList &extraArgs = {});

    std::unique_ptr<GNProjectNode> takeProjectNode() { return std::move(m_rootNode); }

    const GNTargetsList &targets() const { return m_parserResult.targets; }
    const QStringList &targetsNames() const { return m_targetsNames; }
    const Utils::FilePaths &buildSystemFiles() const { return m_parserResult.buildSystemFiles; }

    QList<ProjectExplorer::BuildTargetInfo> appsTargets() const;

    ProjectExplorer::RawProjectParts buildProjectParts(const ProjectExplorer::
                                                       Toolchain *cxxToolchain,
                                                       const ProjectExplorer::
                                                       Toolchain *cToolchain);

signals:
    void started();
    void completed(bool success);

private:
    void startParser();
    void handleProcessDone();
    bool parse(const Utils::FilePath &sourcePath, const Utils::FilePath &buildPath);
    static ParserData extractParserResults(const Utils::FilePath &srcDir,
                                           GNInfoParser::Result &&parserResult);
    QStringList processGnOutput(const QStringList &list);

    Utils::FilePath m_buildDir;
    Utils::FilePath m_srcDir;
    GNInfoParser::Result m_parserResult;
    QStringList m_targetsNames;
    std::unique_ptr<GNProjectNode> m_rootNode;
    QString m_projectName;
    std::unique_ptr<Utils::Process> m_process;
    QElapsedTimer m_elapsed;
};

} // namespace GNProjectManager::Internal
