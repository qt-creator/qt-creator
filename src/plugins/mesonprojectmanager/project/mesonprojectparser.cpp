/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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
#include "mesonprojectparser.h"
#include "projecttree/mesonprojectnodes.h"
#include "projecttree/projecttree.h"
#include <exewrappers/mesontools.h>
#include <mesoninfoparser/mesoninfoparser.h>

#include <coreplugin/messagemanager.h>
#include <projectexplorer/projectexplorer.h>
#include <utils/fileinprojectfinder.h>
#include <utils/optional.h>
#include <utils/runextensions.h>

#include <QStringList>
#include <QTextStream>

namespace MesonProjectManager {
namespace Internal {

struct CompilerArgs
{
    QStringList args;
    QStringList includePaths;
    ProjectExplorer::Macros macros;
};

inline Utils::optional<QString> extractValueIfMatches(const QString &arg,
                                                      const QStringList &candidates)
{
    for (const auto &flag : candidates) {
        if (arg.startsWith(flag))
            return arg.mid(flag.length());
    }
    return Utils::nullopt;
}

inline Utils::optional<QString> extractInclude(const QString &arg)
{
    return extractValueIfMatches(arg, {"-I", "/I", "-isystem", "-imsvc", "/imsvc"});
}
inline Utils::optional<ProjectExplorer::Macro> extractMacro(const QString &arg)
{
    auto define = extractValueIfMatches(arg, {"-D", "/D"});
    if (define)
        return ProjectExplorer::Macro::fromKeyValue(define->toLatin1());
    auto undef = extractValueIfMatches(arg, {"-U", "/U"});
    if (undef)
        return ProjectExplorer::Macro(undef->toLatin1(), ProjectExplorer::MacroType::Undefine);
    return Utils::nullopt;
}

CompilerArgs splitArgs(const QStringList &args)
{
    CompilerArgs splited;
    std::for_each(std::cbegin(args), std::cend(args), [&splited](const QString &arg) {
        auto inc = extractInclude(arg);
        if (inc) {
            splited.includePaths << *inc;
        } else {
            auto macro = extractMacro(arg);
            if (macro) {
                splited.macros << *macro;
            } else {
                splited.args << arg;
            }
        }
    });
    return splited;
}

QStringList toAbsolutePath(const Utils::FilePath &refPath, QStringList &pathList)
{
    QStringList allAbs;
    std::transform(std::cbegin(pathList),
                   std::cend(pathList),
                   std::back_inserter(allAbs),
                   [refPath](const QString &path) {
                       if (Utils::FileUtils::isAbsolutePath(path))
                           return path;
                       return refPath.pathAppended(path).toString();
                   });
    return allAbs;
}

MesonProjectParser::MesonProjectParser(const Utils::Id &meson,
                                       Utils::Environment env,
                                       ProjectExplorer::Project *project)
    : m_env{env}
    , m_meson{meson}
    , m_projectName{project->displayName()}
{
    connect(&m_process, &MesonProcess::finished, this, &MesonProjectParser::processFinished);
    connect(&m_process,
            &MesonProcess::readyReadStandardOutput,
            &m_outputParser,
            &MesonOutputParser::readStdo);

    // TODO re-think the way all BuildSystem/ProjectParser are tied
    // I take project info here, I also take build and src dir later from
    // functions args.
    auto fileFinder = new Utils::FileInProjectFinder;
    fileFinder->setProjectDirectory(project->projectDirectory());
    fileFinder->setProjectFiles(project->files(ProjectExplorer::Project::AllFiles));
    m_outputParser.setFileFinder(fileFinder);
}

void MesonProjectParser::setMesonTool(const Utils::Id &meson)
{
    m_meson = meson;
}

bool MesonProjectParser::configure(const Utils::FilePath &sourcePath,
                                   const Utils::FilePath &buildPath,
                                   const QStringList &args)
{
    m_introType = IntroDataType::file;
    m_srcDir = sourcePath;
    m_buildDir = buildPath;
    m_outputParser.setSourceDirectory(sourcePath);
    auto cmd = MesonTools::mesonWrapper(m_meson)->configure(sourcePath, buildPath, args);
    // see comment near m_pendingCommands declaration
    m_pendingCommands.enqueue(
        std::make_tuple(MesonTools::mesonWrapper(m_meson)->regenerate(sourcePath, buildPath),
                        false));
    return m_process.run(cmd, m_env, m_projectName);
}

bool MesonProjectParser::wipe(const Utils::FilePath &sourcePath,
                              const Utils::FilePath &buildPath,
                              const QStringList &args)
{
    return setup(sourcePath, buildPath, args, true);
}

bool MesonProjectParser::setup(const Utils::FilePath &sourcePath,
                               const Utils::FilePath &buildPath,
                               const QStringList &args,
                               bool forceWipe)
{
    m_introType = IntroDataType::file;
    m_srcDir = sourcePath;
    m_buildDir = buildPath;
    m_outputParser.setSourceDirectory(sourcePath);
    auto cmdArgs = args;
    if (forceWipe || isSetup(buildPath))
        cmdArgs << "--wipe";
    auto cmd = MesonTools::mesonWrapper(m_meson)->setup(sourcePath, buildPath, cmdArgs);
    return m_process.run(cmd, m_env, m_projectName);
}

bool MesonProjectParser::parse(const Utils::FilePath &sourcePath, const Utils::FilePath &buildPath)
{
    m_srcDir = sourcePath;
    m_buildDir = buildPath;
    m_outputParser.setSourceDirectory(sourcePath);
    if (!isSetup(buildPath)) {
        return parse(sourcePath);
    } else {
        m_introType = IntroDataType::file;
        return startParser();
    }
}

bool MesonProjectParser::parse(const Utils::FilePath &sourcePath)
{
    m_srcDir = sourcePath;
    m_introType = IntroDataType::stdo;
    m_outputParser.setSourceDirectory(sourcePath);
    return m_process.run(MesonTools::mesonWrapper(m_meson)->introspect(sourcePath),
                         m_env,
                         m_projectName,
                         true);
}

QList<ProjectExplorer::BuildTargetInfo> MesonProjectParser::appsTargets() const
{
    QList<ProjectExplorer::BuildTargetInfo> apps;
    std::for_each(std::cbegin(m_parserResult.targets),
                  std::cend(m_parserResult.targets),
                  [&apps, srcDir = m_srcDir](const Target &target) {
                      if (target.type == Target::Type::executable) {
                          ProjectExplorer::BuildTargetInfo bti;
                          bti.displayName = target.name;
                          bti.buildKey = Target::fullName(srcDir, target);
                          bti.displayNameUniquifier = bti.buildKey;
                          bti.targetFilePath = Utils::FilePath::fromString(target.fileName.first());
                          bti.workingDirectory
                              = Utils::FilePath::fromString(target.fileName.first()).absolutePath();
                          bti.projectFilePath = Utils::FilePath::fromString(target.definedIn);
                          bti.usesTerminal = true;
                          apps.append(bti);
                      }
                  });
    return apps;
}
bool MesonProjectParser::startParser()
{
    m_parserFutureResult = Utils::runAsync(
        ProjectExplorer::ProjectExplorerPlugin::sharedThreadPool(),
        [process = &m_process,
         introType = m_introType,
         buildDir = m_buildDir.toString(),
         srcDir = m_srcDir]() {
            if (introType == IntroDataType::file) {
                return extractParserResults(srcDir, MesonInfoParser::parse(buildDir));
            } else {
                return extractParserResults(srcDir, MesonInfoParser::parse(process->stdOut()));
            }
        });

    Utils::onFinished(m_parserFutureResult, this, &MesonProjectParser::update);
    return true;
}

MesonProjectParser::ParserData *MesonProjectParser::extractParserResults(
    const Utils::FilePath &srcDir, MesonInfoParser::Result &&parserResult)
{
    auto rootNode = ProjectTree::buildTree(srcDir,
                                           parserResult.targets,
                                           parserResult.buildSystemFiles);
    return new ParserData{std::move(parserResult), std::move(rootNode)};
}

void MesonProjectParser::addMissingTargets(QStringList &targetList)
{
    // Not all targets are listed in introspection data
    for (const auto &target : additionalTargets()) {
        if (!targetList.contains(target)) {
            targetList.append(target);
        }
    }
}

void MesonProjectParser::update(const QFuture<MesonProjectParser::ParserData *> &data)
{
    auto parserData = data.result();
    m_parserResult = std::move(parserData->data);
    m_rootNode = std::move(parserData->rootNode);
    m_targetsNames.clear();
    for (const Target &target : m_parserResult.targets) {
        m_targetsNames.push_back(Target::fullName(m_srcDir, target));
    }
    addMissingTargets(m_targetsNames);
    m_targetsNames.sort();
    delete data;
    emit parsingCompleted(true);
}

ProjectExplorer::RawProjectPart MesonProjectParser::buildRawPart(
    const Target &target,
    const Target::SourceGroup &sources,
    const ProjectExplorer::ToolChain *cxxToolChain,
    const ProjectExplorer::ToolChain *cToolChain)
{
    ProjectExplorer::RawProjectPart part;
    part.setDisplayName(target.name);
    part.setBuildSystemTarget(Target::fullName(m_srcDir, target));
    part.setFiles(sources.sources + sources.generatedSources);
    auto flags = splitArgs(sources.parameters);
    part.setMacros(flags.macros);
    part.setIncludePaths(toAbsolutePath(m_buildDir, flags.includePaths));
    part.setProjectFileLocation(target.definedIn);
    if (sources.language == "cpp")
        part.setFlagsForCxx({cxxToolChain, flags.args});
    else if (sources.language == "c")
        part.setFlagsForC({cToolChain, flags.args});
    part.setQtVersion(m_qtVersion);
    return part;
}

void MesonProjectParser::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
        if (m_pendingCommands.isEmpty())
            startParser();
        else {
            // see comment near m_pendingCommands declaration
            std::tuple<Command, bool> args = m_pendingCommands.dequeue();
            m_process.run(std::get<0>(args), m_env, m_projectName, std::get<1>(args));
        }
    } else {
        if (m_introType == IntroDataType::stdo) {
            auto data = m_process.stdErr();
            Core::MessageManager::write(QString::fromLocal8Bit(data));
            m_outputParser.readStdo(data);
        }
        emit parsingCompleted(false);
    }
}

ProjectExplorer::RawProjectParts MesonProjectParser::buildProjectParts(
    const ProjectExplorer::ToolChain *cxxToolChain, const ProjectExplorer::ToolChain *cToolChain)
{
    ProjectExplorer::RawProjectParts parts;
    for_each_source_group(m_parserResult.targets,
                          [&parts,
                           &cxxToolChain,
                           &cToolChain,
                           this](const Target &target, const Target::SourceGroup &sourceList) {
                              parts.push_back(
                                  buildRawPart(target, sourceList, cxxToolChain, cToolChain));
                          });
    return parts;
}

bool sourceGroupMatchesKit(const KitData &kit, const Target::SourceGroup &group)
{
    if (group.language == "c")
        return kit.cCompilerPath == group.compiler[0];
    if (group.language == "cpp")
        return kit.cxxCompilerPath == group.compiler[0];
    return true;
}

bool MesonProjectParser::matchesKit(const KitData &kit)
{
    bool matches = true;
    for_each_source_group(m_parserResult.targets,
                          [&matches, &kit](const Target &, const Target::SourceGroup &sourceGroup) {
                              matches = matches && sourceGroupMatchesKit(kit, sourceGroup);
                          });
    return matches;
}

bool MesonProjectParser::usesSameMesonVersion(const Utils::FilePath &buildPath)
{
    auto info = MesonInfoParser::mesonInfo(buildPath.toString());
    auto meson = MesonTools::mesonWrapper(m_meson);
    return info && meson && info->mesonVersion == meson->version();
}
} // namespace Internal
} // namespace MesonProjectManager
