// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "kitdata.h"
#include "mesoninfoparser.h"
#include "mesonoutputparser.h"
#include "mesonprocess.h"
#include "mesonprojectnodes.h"
#include "mesonwrapper.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/rawprojectpart.h>

#include <utils/environment.h>
#include <utils/fileutils.h>

#include <QFuture>
#include <QQueue>

namespace MesonProjectManager {
namespace Internal {

class MesonProjectParser : public QObject
{
    Q_OBJECT
    enum class IntroDataType { file, stdo };
    struct ParserData
    {
        MesonInfoParser::Result data;
        std::unique_ptr<MesonProjectNode> rootNode;
    };

public:
    MesonProjectParser(const Utils::Id &meson, Utils::Environment env, ProjectExplorer::Project* project);
    void setMesonTool(const Utils::Id &meson);
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

    Q_SIGNAL void parsingCompleted(bool success);

    std::unique_ptr<MesonProjectNode> takeProjectNode() { return std::move(m_rootNode); }

    inline const BuildOptionsList &buildOptions() const { return m_parserResult.buildOptions; };
    inline const TargetsList &targets() const { return m_parserResult.targets; }
    inline const QStringList &targetsNames() const { return m_targetsNames; }

    static inline QStringList additionalTargets()
    {
        return QStringList{Constants::Targets::all,
                           Constants::Targets::clean,
                           Constants::Targets::install,
                           Constants::Targets::benchmark,
                           Constants::Targets::scan_build};
    }

    QList<ProjectExplorer::BuildTargetInfo> appsTargets() const;

    ProjectExplorer::RawProjectParts buildProjectParts(
        const ProjectExplorer::ToolChain *cxxToolChain,
        const ProjectExplorer::ToolChain *cToolChain);

    inline void setEnvironment(const Utils::Environment &environment) { m_env = environment; }

    inline void setQtVersion(Utils::QtMajorVersion v) { m_qtVersion = v; }

    bool matchesKit(const KitData &kit);

    bool usesSameMesonVersion(const Utils::FilePath &buildPath);

private:
    bool startParser();
    static ParserData *extractParserResults(const Utils::FilePath &srcDir,
                                            MesonInfoParser::Result &&parserResult);
    static void addMissingTargets(QStringList &targetList);
    void update(const QFuture<ParserData *> &data);
    ProjectExplorer::RawProjectPart buildRawPart(const Target &target,
                                                 const Target::SourceGroup &sources,
                                                 const ProjectExplorer::ToolChain *cxxToolChain,
                                                 const ProjectExplorer::ToolChain *cToolChain);
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    MesonProcess m_process;
    MesonOutputParser m_outputParser;
    Utils::Environment m_env;
    Utils::Id m_meson;
    Utils::FilePath m_buildDir;
    Utils::FilePath m_srcDir;
    QFuture<ParserData *> m_parserFutureResult;
    bool m_configuring = false;
    IntroDataType m_introType;
    MesonInfoParser::Result m_parserResult;
    QStringList m_targetsNames;
    Utils::QtMajorVersion m_qtVersion = Utils::QtMajorVersion::Unknown;
    std::unique_ptr<MesonProjectNode> m_rootNode; // <- project tree root node
    QString m_projectName;
    // maybe moving meson to build step could make this class simpler
    // also this should ease command dependencies
    QQueue<std::tuple<Command, bool>> m_pendingCommands;
};

} // namespace Internal
} // namespace MesonProjectManager
