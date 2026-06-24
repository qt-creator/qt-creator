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

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskhub.h>

#include <utils/async.h>
#include <utils/environment.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>

#include <QElapsedTimer>

#include <memory>
#include <ranges>

using namespace Core;
using namespace ProjectExplorer;
using namespace QtTaskTree;
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
    static const std::unordered_set<QStringView> srcs = {
        // C/C++
        u"c", u"C", u"cpp", u"cppm", u"cxx", u"cc", u"c++",

        // Objective-C/C++
        u"m", u"M", u"mm", u"mi",u"mii",

        // ASM
        u"s", u"S", u"asm",

        // rust
        u"rs",

        // swift
        u"swift",
    };
    return srcs.contains(file.suffixView());
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

    for (const auto &data : target.datas) {
        otherFiles << FilePath::fromString(data);
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
    for (const GNTarget &target : targets) {
        if (!target.sources.empty() || !target.datas.empty())
            root->addNestedNode(makeTargetNode(root, target));
    }

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

static QStringList processGnOutput(const QStringList &list, const FilePath &srcDir)
{
    static const QRegularExpression errorRegex{R"(^ERROR at (.*):(\d+):(\d+): (.*))"};
    QStringList res;
    for (const auto &line : list) {
        auto errors = errorRegex.match(line);
        if (errors.hasMatch()) {
            auto path = resolveGNPath(errors.captured(1), srcDir);
            auto task = BuildSystemTask(Task::TaskType::Error,
                                        line,
                                        FilePath::fromString(path),
                                        errors.captured(2).toInt());
            task.setColumn(errors.captured(3).toInt());
            TaskHub::addTask(task);
        }
        res << addGNPrefix(line);
    }
    return res;
}

static void addMissingTargets(QStringList *targetList)
{
    static const QString additionalTargets[]{
        Constants::Targets::all,
        Constants::Targets::clean,
    };

    for (const QString &target : additionalTargets) {
        if (!targetList->contains(target))
            targetList->append(target);
    }
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
    if (!gnExe.exists()) {
        TaskHub::addTask<BuildSystemTask>(
            Task::TaskType::Error,
            Tr::tr("GN executable does not exist: %1").arg(gnExe.toUserOutput()));
        return false;
    }

    const FilePath srcDir = sourcePath.canonicalPath();
    const FilePath buildDir = buildPath.canonicalPath();

    QStringList args = {"gen", "--ide=json"};
    args.append(extraArgs);
    args.append(buildDir.toFSPathString());

    ProcessRunData runData;
    runData.command = CommandLine{gnExe, args};
    runData.workingDirectory = srcDir;
    runData.environment = env;

    const Storage<QElapsedTimer> elapsed;
    const QString displayName = Tr::tr("Generating \"%1\"").arg(m_projectName);

    const auto onProcessSetup = [runData, elapsed, displayName, srcDir](Process &process) {
        MessageManager::writeFlashing(
            Tr::tr("Running %1 in %2.")
                .arg(runData.command.toUserOutput(), runData.workingDirectory.toUserOutput()));
        process.setRunData(runData);
        auto progress = new ProcessProgress(&process);
        progress->setDisplayName(displayName);
        elapsed->start();
        qCDebug(gnProcessLog()) << "Starting:" << runData.command.toUserOutput();
        QObject::connect(&process, &Process::readyReadStandardOutput, &process,
                         [processPtr = &process, srcDir] {
            const QByteArray data = processPtr->readAllRawStandardOutput();
            MessageManager::writeFlashing(
                processGnOutput(QString::fromLocal8Bit(data).split('\n'), srcDir));
        });
        QObject::connect(&process, &Process::readyReadStandardError, &process,
                         [processPtr = &process, srcDir] {
            const QByteArray data = processPtr->readAllRawStandardError();
            MessageManager::writeDisrupting(
                processGnOutput(QString::fromLocal8Bit(data).split('\n'), srcDir));
        });
    };
    const auto onProcessDone = [elapsed](const Process &process) {
        if (process.result() != ProcessResult::FinishedWithSuccess)
            TaskHub::addTask<BuildSystemTask>(Task::TaskType::Error, process.exitMessage());
        MessageManager::writeSilently(formatElapsedTime(elapsed->elapsed()));
    };

    const auto onParseSetup = [srcDir, buildDir](Async<ParserData> &async) {
        async.setThreadPool(ProjectExplorerPlugin::sharedThreadPool());
        async.setConcurrentCallData([srcDir, buildDir] {
            auto result = GNInfoParser::parse(buildDir.pathAppended(Constants::PROJECT_JSON));
            auto rootNode = buildTree(srcDir, result.targets, result.buildSystemFiles);
            return ParserData{std::move(result), std::move(rootNode)};
        });
    };
    const auto onParseDone = [this](const Async<ParserData> &async) {
        auto [data, rootNode] = async.takeResult();
        m_parserResult = std::move(data);
        m_rootNode = std::move(rootNode);
        m_targetsNames.clear();
        for (const GNTarget &target : m_parserResult.targets)
            m_targetsNames.push_back(target.displayName);
        addMissingTargets(&m_targetsNames);
        m_targetsNames.sort();
        emit completed(true);
    };

    emit started();
    m_taskTreeRunner.start({
        elapsed,
        ProcessTask(onProcessSetup, onProcessDone),
        AsyncTask<ParserData>(onParseSetup, onParseDone, CallDoneFlag::OnSuccess)
    }, {}, [this] { emit completed(false); }, CallDoneFlag::OnError);
    return true;
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
                = Utils::FilePath::fromString(resolveGNPath(target.path, m_parserResult.rootPath))
                      .pathAppended("BUILD.gn");
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
                             + FilePaths::resolvePaths(m_parserResult.buildDir, flags.includePaths));

        // Determine language from sources - if it has .c files use C, otherwise C++
        bool hasCFiles = false;
        bool hasCppFiles = false;
        for (const auto &src : sourceFiles) {
            if (src.endsWith(".c"))
                hasCFiles = true;
            if (src.endsWith(".cpp") || src.endsWith(".cc") || src.endsWith(".cxx") || src.endsWith(".cppm"))
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
