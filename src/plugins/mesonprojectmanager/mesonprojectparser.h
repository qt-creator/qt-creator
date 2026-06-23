// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "kitdata.h"
#include "mesoninfoparser.h"
#include "mesonoutputparser.h"
#include "mesonprojectnodes.h"
#include "mesontools.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/rawprojectpart.h>

#include <utils/processinterface.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

namespace MesonProjectManager::Internal {

class MesonProjectParser : public QObject
{
    Q_OBJECT

    struct ParserData
    {
        MesonInfoParser::Result data;
        std::unique_ptr<MesonProjectNode> rootNode;
    };

public:
    MesonProjectParser(const Utils::Id &meson,
                       const Utils::Environment &env,
                       ProjectExplorer::Project *project);

    bool configure(const Utils::FilePath &sourcePath,
                   const Utils::FilePath &buildPath,
                   const QStringList &args);
    bool wipe(const Utils::FilePath &sourcePath,
              const Utils::FilePath &buildPath,
              const QStringList &args);
    bool setup(const Utils::FilePath &sourcePath,
               const Utils::FilePath &buildPath,
               const QStringList &args,
               bool forceWipe = false);
    bool parse(const Utils::FilePath &sourcePath, const Utils::FilePath &buildPath);
    bool parse(const Utils::FilePath &sourcePath);

    std::unique_ptr<MesonProjectNode> takeProjectNode() { return std::move(m_rootNode); }

    const BuildOptionsList &buildOptions() const { return m_parserResult.buildOptions; };
    const TargetsList &targets() const { return m_parserResult.targets; }
    const QStringList &targetsNames() const { return m_targetsNames; }

    QList<ProjectExplorer::BuildTargetInfo> appsTargets() const;

    ProjectExplorer::RawProjectParts buildProjectParts(
        const Utils::FilePath &buildDir,
        const ProjectExplorer::Toolchain *cxxToolchain,
        const ProjectExplorer::Toolchain *cToolchain);

    void setEnvironment(const Utils::Environment &environment) { m_env = environment; }

    void setQtVersion(Utils::QtMajorVersion v) { m_qtVersion = v; }

    bool matchesKit(const KitData &kit);

    bool usesSameMesonVersion(const Utils::FilePath &buildPath);

signals:
    void parsingCompleted(bool success);

private:
    void startParsing(const QtTaskTree::Group &recipe);

    QtTaskTree::ExecutableItem streamingProcessTask(const Utils::ProcessRunData &runData);
    QtTaskTree::ExecutableItem fileParserTask(const Utils::FilePath &srcDir,
                                              const Utils::FilePath &buildDir);

    void applyParserResults(const Utils::FilePath &srcDir, ParserData &&parserData);

    static ParserData extractParserResults(const Utils::FilePath &srcDir,
                                           MesonInfoParser::Result &&parserResult);
    ProjectExplorer::RawProjectPart buildRawPart(const Utils::FilePath &buildDir,
                                                 const Target &target,
                                                 const Target::SourceGroup &sources,
                                                 const ProjectExplorer::Toolchain *cxxToolchain,
                                                 const ProjectExplorer::Toolchain *cToolchain);

    MesonOutputParser m_outputParser;
    Utils::Environment m_env;
    Utils::Id m_meson;
    MesonInfoParser::Result m_parserResult;
    QStringList m_targetsNames;
    Utils::QtMajorVersion m_qtVersion = Utils::QtMajorVersion::Unknown;
    std::unique_ptr<MesonProjectNode> m_rootNode;
    QString m_projectName;
    QtTaskTree::QSingleTaskTreeRunner m_taskTreeRunner;
};

} // namespace MesonProjectManager::Internal
