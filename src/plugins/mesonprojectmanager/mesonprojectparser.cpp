// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesonprojectparser.h"

#include "common.h"
#include "mesoninfoparser.h"
#include "mesonpluginconstants.h"
#include "mesonprojectmanagertr.h"
#include "mesonprojectnodes.h"
#include "mesontools.h"

#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/processprogress.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskhub.h>

#include <utils/async.h>
#include <utils/environment.h>
#include <utils/fileinprojectfinder.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/futuresynchronizer.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

#include <QElapsedTimer>

#include <memory>
#include <optional>

using namespace Core;
using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace MesonProjectManager::Internal {

static Q_LOGGING_CATEGORY(mesonProcessLog, "qtc.meson.buildsystem", QtWarningMsg);

struct CompilerArgs
{
    QStringList args;
    QStringList includePaths;
    Macros macros;
};

struct TargetFiles
{
    FilePaths c_cpp_sources;
    FilePaths headers;
    FilePaths py_sources;
    FilePaths ui_files;
    FilePaths qrc_files;
    FilePaths qml_files;
    FilePaths local_others;
    FilePaths global_others;
};


static bool isHeader(const Utils::FilePath &file)
{
    return file.endsWith(".h")
           || file.endsWith(".hpp")
           || file.endsWith(".hxx");
}

static bool isSource(const Utils::FilePath &file)
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

static std::unique_ptr<MesonTargetNode> makeTargetNodes(std::unique_ptr<MesonProjectNode> &root,
                                                        const Target &target,
                                                        const TargetFiles &targetFiles)
{
    auto targetNode = std::make_unique<MesonTargetNode>(
        FilePath::fromString(target.definedIn).absolutePath().pathAppended(target.name),
        Target::unique_name(target,root->directory()),
        Target::typeToString(target.type),
        target.fileName,
        target.buildByDefault
        );
    targetNode->setDisplayName(target.name);
    if (!targetFiles.c_cpp_sources.isEmpty()) {
        auto sourcesGroup = createVFolder(targetNode->path(), Tr::tr("Source Files"), true, ProjectExplorer::Constants::FILEOVERLAY_CPP);
        for (const auto &file : targetFiles.c_cpp_sources) {
            sourcesGroup->addNestedNode(std::make_unique<FileNode>(file, FileType::Source));
        }
        targetNode->addNode(std::move(sourcesGroup));
    }
    if (!targetFiles.headers.isEmpty()) {
        auto headersGroup = createVFolder(targetNode->path(), Tr::tr("Header Files"), true, ProjectExplorer::Constants::FILEOVERLAY_H);
        for (const auto &file : targetFiles.headers) {
            headersGroup->addNestedNode(std::make_unique<FileNode>(file, FileType::Header));
        }
        targetNode->addNode(std::move(headersGroup));
    }
    if (!targetFiles.py_sources.isEmpty()) {
        auto pyGroup = createVFolder(targetNode->path(), Tr::tr("Python Files"), true, ProjectExplorer::Constants::FILEOVERLAY_PY);
        for (const auto &file : targetFiles.py_sources) {
            pyGroup->addNestedNode(std::make_unique<FileNode>(file, FileType::Source));
        }
        targetNode->addNode(std::move(pyGroup));
    }
    if (!targetFiles.ui_files.isEmpty()) {
        auto uiGroup = createVFolder(
            targetNode->path(),
            Tr::tr("Qt Widgets Designer Files"),
            true,
            ProjectExplorer::Constants::FILEOVERLAY_UI);
        for (const auto &file : targetFiles.ui_files) {
            uiGroup->addNestedNode(std::make_unique<FileNode>(file, FileType::Form));
        }
        targetNode->addNode(std::move(uiGroup));
    }
    if (!targetFiles.qrc_files.isEmpty()) {
        auto qrcGroup = createVFolder(targetNode->path(), Tr::tr("Qt Resource Files"), true, ProjectExplorer::Constants::FILEOVERLAY_QRC);
        for (const auto &file : targetFiles.qrc_files) {
            qrcGroup->addNestedNode(std::make_unique<FileNode>(file, FileType::Resource));
        }
        targetNode->addNode(std::move(qrcGroup));
    }
    if (!targetFiles.qml_files.isEmpty()) {
        auto qmlGroup = createVFolder(targetNode->path(), Tr::tr("QML Files"), true, ProjectExplorer::Constants::FILEOVERLAY_QML);
        for (const auto &file : targetFiles.qml_files) {
            qmlGroup->addNestedNode(std::make_unique<FileNode>(file, FileType::QML));
        }
        targetNode->addNode(std::move(qmlGroup));
    }
    if (!targetFiles.local_others.isEmpty()) {
        auto otherGroup = createVFolder(targetNode->path(), Tr::tr("Other Files"), false, ProjectExplorer::Constants::FILEOVERLAY_UNKNOWN);
        for (const auto &file : targetFiles.local_others) {
            otherGroup->addNestedNode(std::make_unique<FileNode>(file, FileType::Unknown));
        }
        targetNode->addNode(std::move(otherGroup));
    }
    if (!targetFiles.global_others.isEmpty()) {
        for (const auto &file : targetFiles.global_others) {
            root->addNestedNode(std::make_unique<FileNode>(file, FileType::Unknown));
        }
    }
    return targetNode;
}

static void filterFile(const Utils::FilePath &file, const Utils::FilePath &targetPath, TargetFiles &targetFiles)
{
    if (isHeader(file))
        targetFiles.headers << file;
    else if (isSource(file))
        targetFiles.c_cpp_sources << file;
    else if (file.endsWith(".py"))
        targetFiles.py_sources << file;
    else if (file.endsWith(".ui"))
        targetFiles.ui_files << file;
    else if (file.endsWith(".qrc"))
        targetFiles.qrc_files << file;
    else if (file.endsWith(".qml"))
        targetFiles.qml_files << file;
    else if (file.isChildOf(targetPath))
        targetFiles.local_others << file;
    else
        targetFiles.global_others << file;
}

static void buildTargetTree(std::unique_ptr<MesonProjectNode> &root, const Target &target)
{
    const auto project_file_path = FilePath::fromString(target.definedIn).canonicalPath();
    const auto project_file_dir = project_file_path.absolutePath();
    TargetFiles targetFiles;
    if (auto node = root->findNode([absPath=project_file_dir](Node* node){return node->path()==absPath;}); node) {
        if (auto folder = node->asFolderNode(); folder) {
            for (const auto &group : target.sources) {
                for (const auto &file : group.sources) {
                    filterFile(FilePath::fromString(file).canonicalPath(), project_file_dir, targetFiles);
                }
            }
            for (const auto &extraFile : target.extraFiles) {
                filterFile(FilePath::fromString(extraFile).canonicalPath(), project_file_dir, targetFiles);
            }
            folder->addNode(makeTargetNodes(root, target, targetFiles));
        }
    }
}

static std::unique_ptr<MesonProjectNode> buildTree(
    const FilePath &srcDir, const TargetsList &targets, const FilePaths &bsFiles)
{
    auto root = std::make_unique<MesonProjectNode>(srcDir);
    //Populating all build system files ensures all folders are created
    for (FilePath bsFile : bsFiles) {
        bsFile = srcDir.resolvePath(bsFile);
        // For some reason Meson also list files outside of the project directory
        // like the python3 or qmake path
        if (bsFile.canonicalPath().isChildOf(srcDir))
            root->addNestedNode(std::make_unique<FileNode>(bsFile, FileType::Project));
    }

    for (const Target &target : targets) {
        buildTargetTree(root, target);
    }

    return root;
}

static std::optional<QString> extractValueIfMatches(const QString &arg, const QStringList &candidates)
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

static void addMissingTargets(QStringList *targetList)
{
    // Not all targets are listed in introspection data
    static const QString additionalTargets[]{Constants::Targets::all,
                                             Constants::Targets::tests,
                                             Constants::Targets::benchmark,
                                             Constants::Targets::clean,
                                             Constants::Targets::install};

    for (const QString &target : additionalTargets) {
        if (!targetList->contains(target))
            targetList->append(target);
    }
}

static void initProcess(Process &process, const ProcessRunData &runData,
                        const Storage<QElapsedTimer> &elapsed, const QString &displayName)
{
    MessageManager::writeFlashing(
        Tr::tr("Running %1 in %2.")
            .arg(runData.command.toUserOutput(), runData.workingDirectory.toUserOutput()));
    process.setRunData(runData);
    auto *progress = new ProcessProgress(&process);
    progress->setDisplayName(displayName);
    elapsed->start();
    qCDebug(mesonProcessLog()) << "Starting:" << runData.command.toUserOutput();
}

static bool sanityCheck(const ProcessRunData &runData)
{
    const auto &exe = runData.command.executable();
    if (!exe.exists()) {
        //Should only reach this point if Meson exe is removed while a Meson project is opened
        TaskHub::addTask<BuildSystemTask>(
            Task::TaskType::Error, Tr::tr("Executable does not exist: %1").arg(exe.toUserOutput()));
        return false;
    }
    if (!exe.toFileInfo().isExecutable()) {
        TaskHub::addTask<BuildSystemTask>(
            Task::TaskType::Error, Tr::tr("Command is not executable: %1").arg(exe.toUserOutput()));
        return false;
    }
    return true;
}

static void finalizeProcess(const Process &process, const Storage<QElapsedTimer> &elapsed)
{
    if (process.result() != ProcessResult::FinishedWithSuccess)
        TaskHub::addTask<BuildSystemTask>(Task::TaskType::Error, process.exitMessage());
    MessageManager::writeSilently(formatElapsedTime(elapsed->elapsed()));
}

MesonProjectParser::MesonProjectParser(const Id &meson, const Environment &env, Project *project)
    : m_env{env}
    , m_meson{meson}
    , m_projectName{project->displayName()}
{
    // TODO re-think the way all BuildSystem/ProjectParser are tied
    // I take project info here, I also take build and src dir later from
    // functions args.
    auto fileFinder = new FileInProjectFinder;
    fileFinder->setProjectDirectory(project->projectDirectory());
    fileFinder->setProjectFiles(project->files(Project::AllFiles));
    m_outputParser.setFileFinder(fileFinder);
}

bool MesonProjectParser::configure(
    const FilePath &sourcePath, const FilePath &buildPath, const QStringList &args)
{
    const FilePath srcDir = sourcePath.canonicalPath();
    const FilePath buildDir = buildPath.canonicalPath();
    m_outputParser.setSourceDirectory(srcDir);
    m_outputParser.setBuildDirectory(buildDir);

    auto configureCmd = MesonTools::toolById(m_meson)->configure(srcDir, buildDir, args);
    configureCmd.environment = m_env;
    auto regenerateCmd = MesonTools::toolById(m_meson)->regenerate(srcDir, buildDir);
    regenerateCmd.environment = m_env;

    if (!sanityCheck(configureCmd))
        return false;

    TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

    startParsing({
        streamingProcessTask(configureCmd),
        streamingProcessTask(regenerateCmd),
        fileParserTask(srcDir, buildDir)
    });
    return true;
}

bool MesonProjectParser::wipe(
    const FilePath &sourcePath, const FilePath &buildPath, const QStringList &args)
{
    return setup(sourcePath, buildPath, args, true);
}

bool MesonProjectParser::setup(
    const FilePath &sourcePath, const FilePath &buildPath, const QStringList &args, bool forceWipe)
{
    const FilePath srcDir = sourcePath.canonicalPath();
    const FilePath buildDir = buildPath.canonicalPath();
    m_outputParser.setSourceDirectory(srcDir);
    m_outputParser.setBuildDirectory(buildDir);

    auto cmdArgs = args;
    if (forceWipe || isSetup(buildDir))
        cmdArgs << "--wipe";
    auto cmd = MesonTools::toolById(m_meson)->setup(srcDir, buildDir, cmdArgs);
    cmd.environment = m_env;

    if (!sanityCheck(cmd))
        return false;

    TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

    startParsing({
        streamingProcessTask(cmd),
        fileParserTask(srcDir, buildDir)
    });
    return true;
}

bool MesonProjectParser::parse(const FilePath &sourcePath, const FilePath &buildPath)
{
    const FilePath srcDir = sourcePath.canonicalPath();
    const FilePath buildDir = buildPath.canonicalPath();
    m_outputParser.setSourceDirectory(srcDir);
    m_outputParser.setBuildDirectory(buildDir);

    if (!isSetup(buildDir))
        return parse(srcDir);

    startParsing({fileParserTask(srcDir, buildDir)});
    return true;
}

bool MesonProjectParser::parse(const FilePath &sourcePath)
{
    const FilePath srcDir = sourcePath.canonicalPath();
    m_outputParser.setSourceDirectory(srcDir);

    auto cmd = MesonTools::toolById(m_meson)->introspect(srcDir);
    cmd.environment = m_env;

    if (!sanityCheck(cmd))
        return false;

    TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

    const Storage<QElapsedTimer> elapsed;
    const Storage<QByteArray> stdoStorage;

    const QString displayName = Tr::tr("Configuring \"%1\".").arg(m_projectName);

    const auto onCaptureSetup = [cmd, elapsed, displayName](Process &process) {
        initProcess(process, cmd, elapsed, displayName);
    };
    const auto onCaptureDone = [this, elapsed, stdoStorage](const Process &process,
                                                             DoneWith result) {
        finalizeProcess(process, elapsed);
        if (result == DoneWith::Success) {
            *stdoStorage = process.rawStdOut();
        } else {
            const QByteArray stde = process.rawStdErr();
            MessageManager::writeSilently(QString::fromLocal8Bit(stde));
            m_outputParser.readStdo(stde);
        }
    };

    const auto onParseSetup = [srcDir, stdoStorage](Async<ParserData> &async) {
        async.setThreadPool(ProjectExplorerPlugin::sharedThreadPool());
        async.setConcurrentCallData([srcDir, stdo = *stdoStorage] {
            return extractParserResults(srcDir, MesonInfoParser::parse(stdo));
        });
    };
    const auto onParseDone = [this, srcDir](const Async<ParserData> &async) {
        applyParserResults(srcDir, async.takeResult());
    };

    startParsing({
        elapsed,
        stdoStorage,
        ProcessTask(onCaptureSetup, onCaptureDone, CallDoneFlag::Always),
        AsyncTask<ParserData>(onParseSetup, onParseDone, CallDoneFlag::OnSuccess)
    });
    return true;
}

void MesonProjectParser::startParsing(const Group &recipe)
{
    m_taskTreeRunner.start(recipe, {}, [this] {
        emit parsingCompleted(false);
    }, CallDoneFlag::OnError);
}

ExecutableItem MesonProjectParser::streamingProcessTask(const ProcessRunData &runData)
{
    const Storage<QElapsedTimer> elapsed;
    const QString displayName = Tr::tr("Configuring \"%1\".").arg(m_projectName);

    const auto onSetup = [this, runData, elapsed, displayName](Process &process) {
        initProcess(process, runData, elapsed, displayName);
        QObject::connect(&process, &Process::readyReadStandardOutput, this,
                         [processPtr = &process, this] {
            const QByteArray data = processPtr->readAllRawStandardOutput();
            MessageManager::writeSilently(QString::fromLocal8Bit(data));
            m_outputParser.readStdo(data);
        });
        QObject::connect(&process, &Process::readyReadStandardError, &process,
                         [processPtr = &process] {
            MessageManager::writeSilently(
                QString::fromLocal8Bit(processPtr->readAllRawStandardError()));
        });
    };
    const auto onDone = [elapsed](const Process &process) {
        finalizeProcess(process, elapsed);
    };
    return Group {elapsed, ProcessTask(onSetup, onDone)};
}

ExecutableItem MesonProjectParser::fileParserTask(const FilePath &srcDir, const FilePath &buildDir)
{
    return AsyncTask<ParserData>([srcDir, buildDir](Async<ParserData> &async) {
        async.setThreadPool(ProjectExplorerPlugin::sharedThreadPool());
        async.setConcurrentCallData([srcDir, buildDir] {
            return extractParserResults(srcDir, MesonInfoParser::parse(buildDir));
        });
    }, [this, srcDir](const Async<ParserData> &async) {
        applyParserResults(srcDir, async.takeResult());
    }, CallDoneFlag::OnSuccess);
}

void MesonProjectParser::applyParserResults(const FilePath &srcDir, ParserData &&parserData)
{
    m_parserResult = std::move(parserData.data);
    m_rootNode = std::move(parserData.rootNode);
    m_targetsNames.clear();
    for (const Target &target : m_parserResult.targets)
        m_targetsNames.push_back(Target::unique_name(target, srcDir));
    addMissingTargets(&m_targetsNames);
    m_targetsNames.sort();
    emit parsingCompleted(true);
}

MesonProjectParser::ParserData MesonProjectParser::extractParserResults(
    const FilePath &srcDir, MesonInfoParser::Result &&parserResult)
{
    auto rootNode = buildTree(srcDir, parserResult.targets, parserResult.buildSystemFiles);
    return ParserData{std::move(parserResult), std::move(rootNode)};
}

QList<BuildTargetInfo> MesonProjectParser::appsTargets() const
{
    QList<BuildTargetInfo> apps;
    for (const Target &target : m_parserResult.targets) {
        if (target.type == Target::Type::executable) {
            BuildTargetInfo bti;
            bti.displayName = target.name;
            bti.buildKey = target.name;
            bti.displayNameUniquifier = bti.buildKey;
            bti.targetFilePath = FilePath::fromString(target.fileName.first());
            bti.projectFilePath = FilePath::fromString(target.definedIn);
            bti.usesTerminal = true;
            apps.append(bti);
        }
    }
    return apps;
}

RawProjectPart MesonProjectParser::buildRawPart(
    const FilePath &buildDir,
    const Target &target,
    const Target::SourceGroup &sources,
    const Toolchain *cxxToolchain,
    const Toolchain *cToolchain)
{
    RawProjectPart part;
    part.setDisplayName(target.name);
    part.setBuildSystemTarget(target.name);
    part.setFiles(FilePaths::fromStrings(sources.sources + sources.generatedSources));
    CompilerArgs flags = splitArgs(sources.parameters);
    part.setMacros(flags.macros);
    part.setIncludePaths(FilePaths::resolvePaths(buildDir, flags.includePaths));
    part.setProjectFileLocation(FilePath::fromUserInput(target.definedIn));
    if (sources.language == "cpp")
        part.setFlagsForCxx({cxxToolchain, flags.args, {}});
    else if (sources.language == "c")
        part.setFlagsForC({cToolchain, flags.args, {}});
    part.setQtVersion(m_qtVersion);
    return part;
}

RawProjectParts MesonProjectParser::buildProjectParts(
    const FilePath &buildDir, const Toolchain *cxxToolchain, const Toolchain *cToolchain)
{
    const FilePath canonicalBuildDir = buildDir.canonicalPath();
    RawProjectParts parts;
    for_each_source_group(
        m_parserResult.targets,
        [&parts, &canonicalBuildDir, &cxxToolchain, &cToolchain,
         this](const Target &target, const Target::SourceGroup &sourceList) {
            parts.push_back(buildRawPart(canonicalBuildDir, target, sourceList, cxxToolchain, cToolchain));
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
    for_each_source_group(
        m_parserResult.targets,
        [&matches, &kit](const Target &, const Target::SourceGroup &sourceGroup) {
            matches = matches && sourceGroupMatchesKit(kit, sourceGroup);
        });
    return matches;
}

static QVersionNumber versionNumber(const FilePath &buildDir)
{
    const FilePath jsonFile = buildDir / Constants::MESON_INFO_DIR / Constants::MESON_INFO;
    auto obj = load<QJsonObject>(jsonFile.toFSPathString());
    if (!obj)
        return {};
    auto version = obj->value("meson_version").toObject();
    return {version["major"].toInt(), version["minor"].toInt(), version["patch"].toInt()};
}

bool MesonProjectParser::usesSameMesonVersion(const FilePath &buildPath)
{
    auto version = versionNumber(buildPath);
    auto meson = MesonTools::toolById(m_meson);
    return !version.isNull() && meson && version == meson->version();
}



} // namespace MesonProjectManager::Internal
