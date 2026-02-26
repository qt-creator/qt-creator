// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gnprojectparser.h"

#include "gninfoparser.h"
#include "gnpluginconstants.h"
#include "gnprojectmanagertr.h"
#include "gnprojectnodes.h"
#include "gnutils.h"

#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/processprogress.h>

#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskhub.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/environment.h>
#include <utils/fileinprojectfinder.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>

#include <memory>
#include <ranges>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace GNProjectManager::Internal {

static Q_LOGGING_CATEGORY(gnProcessLog, "qtc.gn.buildsystem", QtWarningMsg);

struct CompilerArgs
{
    QStringList args;
    QStringList includePaths;
    Macros macros;
};

static bool isHeader(const FilePath &file)
{
    return file.endsWith(".h") || file.endsWith(".hpp") || file.endsWith(".hxx");
}

static bool isSource(const FilePath &file)
{
    return file.endsWith(".c")
        || file.endsWith(".cpp")
        || file.endsWith(".cxx")
        || file.endsWith(".cc")
        || file.endsWith(".C");
}

static std::unique_ptr<VirtualFolderNode> createVFolder(const FilePath &basePath,
                                                        const QString &displayName,
                                                        bool isSourcesOrHeaders,
                                                        const QString &icon = {})
{
    auto newFolder = std::make_unique<VirtualFolderNode>(basePath);
    newFolder->setDisplayName(displayName);
    newFolder->setIsSourcesOrHeaders(isSourcesOrHeaders);
    if (icon.isEmpty())
        newFolder->setIcon(FileIconProvider::icon(QFileIconProvider::Folder));
    else
        newFolder->setIcon(FileIconProvider::directoryIcon(icon));
    return newFolder;
}

static std::unique_ptr<GNTargetNode> makeTargetNode(std::unique_ptr<GNProjectNode> &root,
                                                    const GNTarget &target)
{
    auto targetNode = std::
        make_unique<GNTargetNode>(Utils::FilePath::
                                  fromString(resolveGNPath(target.path, root->directory())),
                                  target.name,
                                  GNTarget::typeToString(target.type),
                                  target.outputs);
    targetNode->setDisplayName(target.displayName);

    FilePaths headers;
    FilePaths sources;
    FilePaths otherFiles;

    for (const auto &src : target.sources) {
        FilePath file = FilePath::fromString(src);
        if (isHeader(file))
            headers << file;
        else if (isSource(file))
            sources << file;
        else
            otherFiles << file;
    }

    if (!sources.isEmpty()) {
        auto sourcesGroup = createVFolder(targetNode->path(),
                                          Tr::tr("Source Files"),
                                          true,
                                          ProjectExplorer::Constants::FILEOVERLAY_CPP);
        for (const auto &file : sources)
            sourcesGroup->addNestedNode(std::make_unique<FileNode>(file, FileType::Source));
        targetNode->addNode(std::move(sourcesGroup));
    }
    if (!headers.isEmpty()) {
        auto headersGroup = createVFolder(targetNode->path(),
                                          Tr::tr("Header Files"),
                                          true,
                                          ProjectExplorer::Constants::FILEOVERLAY_H);
        for (const auto &file : headers)
            headersGroup->addNestedNode(std::make_unique<FileNode>(file, FileType::Header));
        targetNode->addNode(std::move(headersGroup));
    }
    if (!otherFiles.isEmpty()) {
        auto otherGroup = createVFolder(targetNode->path(),
                                        Tr::tr("Other Files"),
                                        false,
                                        ProjectExplorer::Constants::FILEOVERLAY_UNKNOWN);
        for (const auto &file : otherFiles)
            otherGroup->addNestedNode(std::make_unique<FileNode>(file, FileType::Unknown));
        targetNode->addNode(std::move(otherGroup));
    }

    return targetNode;
}

static std::unique_ptr<GNProjectNode> buildTree(const FilePath &srcDir,
                                                const GNTargetsList &targets,
                                                const FilePaths &bsFiles)
{
    auto root = std::make_unique<GNProjectNode>(srcDir);

    // Add build system files (e.g. .gn, BUILD.gn, args.gn, etc.)
    for (const FilePath &bsFile : bsFiles) {
        if (bsFile.exists())
            root->addNestedNode(std::make_unique<FileNode>(bsFile, FileType::Project));
    }

    // Add targets
    for (const GNTarget &target : targets)
        root->addNestedNode(makeTargetNode(root, target));

    return root;
}

static std::optional<QString> extractValueIfMatches(const QString &arg,
                                                    const QStringList &candidates)
{
    for (const auto &flag : candidates) {
        if (arg.startsWith(flag))
            return arg.mid(flag.size());
    }
    return std::nullopt;
}

static std::optional<QString> extractInclude(const QString &arg)
{
    return extractValueIfMatches(arg, {"-I", "/I", "-isystem", "-imsvc", "/imsvc"});
}

static std::optional<Macro> extractMacro(const QString &arg)
{
    auto define = extractValueIfMatches(arg, {"-D", "/D"});
    if (define)
        return Macro::fromKeyValue(define->toLatin1());
    auto undef = extractValueIfMatches(arg, {"-U", "/U"});
    if (undef)
        return Macro(undef->toLatin1(), MacroType::Undefine);
    return std::nullopt;
}

static CompilerArgs splitArgs(const QStringList &args)
{
    CompilerArgs splited;
    for (const QString &arg : args) {
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
    }
    return splited;
}

static QString addGNPrefix(const QString &str)
{
    static const QString prefix = ansiColoredText(Constants::OUTPUT_PREFIX,
                                                  creatorColor(Theme::Token_Text_Muted));
    return prefix + str;
}

// GNProjectParser

GNProjectParser::GNProjectParser(Project *project)
    : m_projectName{project->displayName()}
{}

bool GNProjectParser::generate(const FilePath &gnExe,
                               const FilePath &sourcePath,
                               const FilePath &buildPath,
                               const Environment &env,
                               const QStringList &extraArgs)
{
    m_srcDir = sourcePath.canonicalPath();
    m_buildDir = buildPath.canonicalPath();

    if (!gnExe.exists()) {
        TaskHub::addTask<BuildSystemTask>(
            Task::TaskType::Error,
            Tr::tr("GN executable does not exist: %1").arg(gnExe.toUserOutput()));
        return false;
    }

    // Run: gn gen --ide=json <build_dir>
    QStringList args = {"gen", "--ide=json"};
    args.append(extraArgs);
    args.append(m_buildDir.toFSPathString());

    if (m_process)
        m_process.release()->deleteLater();
    m_process = std::make_unique<Process>();

    connect(m_process.get(), &Process::done, this, &GNProjectParser::handleProcessDone);
    connect(m_process.get(), &Process::readyReadStandardOutput, this, [this] {
        const auto data = m_process->readAllRawStandardOutput();
        Core::MessageManager::
            writeFlashing(processGnOutput(QString::fromLocal8Bit(data).split('\n')));
    });
    connect(m_process.get(), &Process::readyReadStandardError, this, [this] {
        const auto data = m_process->readAllRawStandardError();
        Core::MessageManager::
            writeDisrupting(processGnOutput(QString::fromLocal8Bit(data).split('\n')));
    });

    ProcessRunData runData;
    runData.command = CommandLine{gnExe, args};
    runData.workingDirectory = m_srcDir;
    runData.environment = env;

    MessageManager::writeFlashing(
        Tr::tr("Running %1 in %2.").arg(runData.command.toUserOutput(), runData.workingDirectory.toUserOutput()));

    m_process->setRunData(runData);
    auto progress = new ProcessProgress(m_process.get());
    progress->setDisplayName(Tr::tr("Generating \"%1\".").arg(m_projectName));
    m_elapsed.start();
    m_process->start();
    emit started();
    qCDebug(gnProcessLog()) << "Starting:" << runData.command.toUserOutput();
    return true;
}

void GNProjectParser::handleProcessDone()
{
    if (m_process->result() != ProcessResult::FinishedWithSuccess) {
        TaskHub::addTask<BuildSystemTask>(Task::TaskType::Error, m_process->exitMessage());
        emit completed(false);
        return;
    }

    const QString elapsedTime = formatElapsedTime(m_elapsed.elapsed());
    MessageManager::writeSilently(elapsedTime);

    if (m_process->exitCode() == 0 && m_process->exitStatus() == QProcess::NormalExit) {
        startParser();
    } else {
        emit completed(false);
    }
}

bool GNProjectParser::parse(const FilePath &sourcePath, const FilePath &buildPath)
{
    m_srcDir = sourcePath.canonicalPath();
    m_buildDir = buildPath.canonicalPath();

    const FilePath projectJson = m_buildDir.pathAppended(Constants::PROJECT_JSON);
    if (!projectJson.exists())
        return false;

    startParser();
    return true;
}

QStringList GNProjectParser::processGnOutput(const QStringList &list)
{
    static const QRegularExpression errorRegex{R"(^ERROR at (.*):(\d+):(\d+): (.*))"};
    QStringList res;
    for (const auto &line : list) {
        auto errors = errorRegex.match(line);
        if (errors.hasMatch()) {
            Utils::OutputLineParser::LinkSpecs linkSpecs;
            auto path = resolveGNPath(errors.captured(1), m_srcDir);
            auto task = ProjectExplorer::BuildSystemTask(ProjectExplorer::Task::TaskType::Error,
                                                         line,
                                                         Utils::FilePath::fromString(path),
                                                         errors.captured(2).toInt());
            task.setColumn(errors.captured(3).toInt());
            ProjectExplorer::TaskHub::addTask(task);
        }
        res << addGNPrefix(line);
    }
    return res;
}

QList<BuildTargetInfo> GNProjectParser::appsTargets() const
{
    QList<BuildTargetInfo> apps;
    FilePaths libSearchPaths;
    std::ranges::filter_view libs{m_parserResult.targets, [](const GNTarget &target) {
                                      return target.type == GNTarget::Type::sharedLibrary
                                             && !target.outputs.isEmpty();
                                  }};
    std::ranges::transform(libs, std::back_inserter(libSearchPaths), [&](const GNTarget &target) {
        return FilePath::fromString(target.outputs.first()).parentDir();
    });
    for (const GNTarget &target : m_parserResult.targets) {
        if (target.type == GNTarget::Type::executable && !target.outputs.isEmpty()) {
            BuildTargetInfo bti;
            bti.displayName = target.displayName;
            bti.targetFilePath = FilePath::fromString(target.outputs.first());
            bti.projectFilePath
                = Utils::FilePath::fromString(resolveGNPath(target.path, m_srcDir)).pathAppended("BUILD.gn");
            bti.usesTerminal = true;
            bti.buildKey = bti.projectFilePath.toUrlishString();
            bti.displayNameUniquifier = bti.buildKey;
            bti.runEnvModifierHash = qHash(libSearchPaths);
            bti.runEnvModifier = [libSearchPaths](Environment &env, bool enabled) {
                if (enabled)
                    env.prependOrSetLibrarySearchPaths(libSearchPaths);
            };
            apps.append(bti);
        }
    }
    return apps;
}

static void addMissingTargets(QStringList &targetList)
{
    static const QString additionalTargets[]{
        Constants::Targets::all,
        Constants::Targets::clean,
    };

    for (const QString &target : additionalTargets) {
        if (!targetList.contains(target))
            targetList.append(target);
    }
}

void GNProjectParser::startParser()
{
    m_parserFutureResult = Utils::
        asyncRun(ProjectExplorerPlugin::sharedThreadPool(),
                 [buildDir = m_buildDir, srcDir = m_srcDir] {
                     const FilePath projectJson = buildDir.pathAppended(Constants::PROJECT_JSON);
                     return extractParserResults(srcDir, GNInfoParser::parse(projectJson));
                 });
    Utils::onFinished(m_parserFutureResult,
                      this,
                      [this](const QFuture<GNProjectParser::ParserData *> &data) {
                          auto parserData = data.result();
                          m_parserResult = std::move(parserData->data);
                          m_rootNode = std::move(parserData->rootNode);
                          m_targetsNames.clear();
                          for (const GNTarget &target : m_parserResult.targets)
                              m_targetsNames.push_back(target.displayName);
                          addMissingTargets(m_targetsNames);
                          m_targetsNames.sort();
                          delete parserData;
                          emit completed(true);
                      });
}

GNProjectParser::ParserData *GNProjectParser::
    extractParserResults(const FilePath &srcDir, GNInfoParser::Result &&parserResult)
{
    auto rootNode = buildTree(srcDir, parserResult.targets, parserResult.buildSystemFiles);
    return new ParserData{std::move(parserResult), std::move(rootNode)};
}

RawProjectParts GNProjectParser::
    buildProjectParts(const Toolchain *cxxToolchain, const Toolchain *cToolchain)
{
    RawProjectParts parts;
    for (const GNTarget &target : m_parserResult.targets) {
        // Only create project parts for targets that have sources
        if (target.sources.isEmpty())
            continue;

        RawProjectPart part;
        part.setDisplayName(target.displayName);
        part.setBuildSystemTarget(target.name);

        // Collect source files
        FilePaths sourceFiles;
        for (const QString &src : target.sources)
            sourceFiles << FilePath::fromString(src);
        part.setFiles(sourceFiles);

        // Build combined flags from target's cflags
        CompilerArgs flags = splitArgs(target.cflags);

        // Add defines from the target's defines list
        Macros macros = flags.macros;
        for (const QString &def : target.defines)
            macros << Macro::fromKeyValue(def.toLatin1());
        part.setMacros(macros);

        part.setIncludePaths(FilePaths::fromStrings(target.includes)
                             + FilePaths::resolvePaths(m_buildDir, flags.includePaths));

        // Determine language from sources - if it has .c files use C, otherwise C++
        bool hasCFiles = false;
        bool hasCppFiles = false;
        for (const auto &src : sourceFiles) {
            if (src.endsWith(".c"))
                hasCFiles = true;
            if (src.endsWith(".cpp") || src.endsWith(".cc") || src.endsWith(".cxx"))
                hasCppFiles = true;
        }

        if (hasCppFiles)
            part.setFlagsForCxx({cxxToolchain, flags.args, {}});
        if (hasCFiles)
            part.setFlagsForC({cToolchain, flags.args, {}});

        parts.push_back(part);
    }
    return parts;
}

} // namespace GNProjectManager::Internal
