// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "zephyrproject.h"

#include "westbuildstep.h"
#include "zephyrconstants.h"
#include "zephyrtr.h"

#include <coreplugin/icontext.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/processprogress.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projectupdater.h>
#include <projectexplorer/rawprojectpart.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchainmanager.h>

#include <utils/async.h>
#include <utils/commandline.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include <QtTaskTree/QSingleTaskTreeRunner>

#include <optional>

#ifdef WITH_TESTS
#include "zephyrsettings.h"

#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>

#include <utils/algorithm.h>

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#endif

using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace PEConstants = ProjectExplorer::Constants;

namespace Zephyr::Internal {

const QLatin1StringView WEST_MANIFEST_MIMETYPE{"text/x-west-manifest"};

// Code model

static QStringList compileArguments(const QJsonObject &entry, OsType osType)
{
    const QJsonArray arguments = entry.value("arguments").toArray();
    if (arguments.isEmpty())
        return ProcessArgs::splitArgs(entry.value("command").toString(), osType);
    QStringList result;
    for (const QJsonValue &argument : arguments)
        result.append(argument.toString());
    return result;
}

static QString cmakeTargetFromOutput(const QString &output)
{
    // Ninja writes backslashes on Windows.
    static const QRegularExpression re(R"(CMakeFiles[/\\]([^/\\]+)\.dir[/\\])");
    return re.match(output).captured(1);
}

// "arm-zephyr-eabi-gcc" -> "arm-zephyr-eabi", "/usr/bin/gcc" -> ""
static QString targetTripleFromCompiler(const FilePath &compiler)
{
    static const QRegularExpression re(
        R"(^(.+-.+)-(?:gcc|g\+\+|cc|c\+\+|clang|clang\+\+)(?:-[0-9.]+)?(?:\.exe)?$)");
    return re.match(compiler.fileName()).captured(1);
}

using OwnedToolchains = std::vector<std::unique_ptr<Toolchain>>;

static Toolchain *toolchainForCompiler(const FilePath &compiler, Id language,
                                       OwnedToolchains &detected)
{
    if (compiler.isEmpty())
        return nullptr;
    const auto matches = [&](const Toolchain *tc) {
        return tc->isValid() && tc->language() == language && tc->matchesCompilerCommand(compiler);
    };
    if (Toolchain *tc = ToolchainManager::toolchain(matches))
        return tc;
    for (const std::unique_ptr<Toolchain> &tc : detected) {
        if (tc->language() == language && tc->compilerCommand() == compiler)
            return tc->refreshValid() ? tc.get() : nullptr;
    }

    // The kit's compiler rejects the target flags of a cross build, so the code
    // model needs the one the build used. It is not registered: kits and
    // imports would keep referring to it after it is gone. The compiler does not
    // run here, the code model asks it for macros and header paths in a worker.
    const QString triple = targetTripleFromCompiler(compiler);
    if (triple.isEmpty())
        return nullptr;
    Id type = PEConstants::GCC_TOOLCHAIN_TYPEID;
    if (compiler.fileName().contains("clang"))
        type = PEConstants::CLANG_TOOLCHAIN_TYPEID;
    Toolchain *tc = ToolchainFactory::createToolchain(type);
    QTC_ASSERT(tc, return nullptr);
    detected.emplace_back(tc);
    tc->setLanguage(language);
    tc->setCompilerCommand(compiler);
    tc->setTargetAbi(Abi::abiFromTargetTriplet(triple));
    if (GccToolchain *gcc = tc->asGccToolchain())
        gcc->setOriginalTargetTriple(triple);
    return tc->isValid() ? tc : nullptr;
}

class CompileGroup
{
public:
    QString target;
    bool isCxx = false;
    HeaderPaths headerPaths;
    Macros macros;
    FilePaths includedFiles;
    QStringList flags;
    FilePaths files;
};

class CompileDatabase
{
public:
    QList<CompileGroup> groups;
    FilePath cCompiler;
    FilePath cxxCompiler;
};

static bool isCxxSource(const FilePath &file)
{
    static const QStringList cxxSuffixes = {"cpp", "cxx", "cc", "c++", "C"};
    return cxxSuffixes.contains(file.suffix());
}

static CompileGroup decodeGroup(const QStringList &args,
                                const std::function<FilePath(const QString &)> &toPath)
{
    CompileGroup group;
    for (int i = 0; i < args.size(); ++i) {
        const QString &arg = args.at(i);
        const auto optionValue = [&](const QString &option) -> std::optional<QString> {
            if (arg == option)
                return i + 1 < args.size() ? args.at(++i) : QString();
            if (arg.startsWith(option))
                return arg.mid(option.size());
            return {};
        };

        if (const auto path = optionValue("-isystem")) {
            group.headerPaths.append(HeaderPath::makeSystem(toPath(*path)));
        } else if (const auto path = optionValue("-idirafter")) {
            group.headerPaths.append(HeaderPath::makeSystem(toPath(*path)));
        } else if (const auto path = optionValue("-iquote")) {
            group.headerPaths.append(HeaderPath::makeUser(toPath(*path)));
        } else if (const auto path = optionValue("-imacros")) {
            group.includedFiles.append(toPath(*path));
        } else if (const auto path = optionValue("-include")) {
            group.includedFiles.append(toPath(*path));
        } else if (const auto path = optionValue("-I")) {
            group.headerPaths.append(HeaderPath::makeUser(toPath(*path)));
        } else if (const auto macro = optionValue("-D")) {
            group.macros.append(Macro::fromKeyValue(*macro));
        } else if (const auto macro = optionValue("-U")) {
            group.macros.append(Macro(macro->toUtf8(), MacroType::Undefine));
        } else {
            group.flags.append(arg);
        }
    }
    return group;
}

// Entries compiled with the same flags end up in one group, which keeps a
// Zephyr build with a thousand sources down to a handful of project parts.
static Result<CompileDatabase> parseCompileDatabase(const QByteArray &contents,
                                                    const FilePath &buildDir)
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(contents, &error);
    if (error.error != QJsonParseError::NoError)
        return ResultError(error.errorString());
    if (!doc.isArray())
        return ResultError(Tr::tr("The top level is not an array."));

    CompileDatabase db;
    QMap<QString, CompileGroup> groups;

    const QJsonArray entries = doc.array();
    for (const QJsonValue &value : entries) {
        const QJsonObject entry = value.toObject();
        const FilePath dir = buildDir.withNewMappedPath(
            FilePath::fromUserInput(entry.value("directory").toString()));
        const auto toPath = [&dir](const QString &path) {
            return dir.resolvePath(FilePath::fromUserInput(path).path());
        };

        const QString fileName = entry.value("file").toString();
        const QStringList args = compileArguments(entry, buildDir.osType());
        if (args.isEmpty() || fileName.isEmpty())
            continue;
        const FilePath file = toPath(fileName);

        QStringList key = {cmakeTargetFromOutput(entry.value("output").toString()),
                           isCxxSource(file) ? QString("c++") : QString("c"), args.first()};
        for (int i = 1; i < args.size(); ++i) {
            const QString &arg = args.at(i);
            if (arg == "-c" || arg == "-MD" || arg == "-MMD" || arg == "-MP")
                continue;
            if (arg == "-o" || arg == "-MF" || arg == "-MT" || arg == "-MQ") {
                ++i;
                continue;
            }
            if (!arg.startsWith('-') && toPath(arg) == file)
                continue;
            key.append(arg);
        }

        const QString groupKey = key.join('\n');
        auto it = groups.find(groupKey);
        if (it == groups.end()) {
            CompileGroup group = decodeGroup(key.mid(3), toPath);
            group.target = key.at(0);
            group.isCxx = key.at(1) == "c++";
            FilePath &compiler = group.isCxx ? db.cxxCompiler : db.cCompiler;
            if (compiler.isEmpty())
                compiler = toPath(args.first());
            it = groups.insert(groupKey, group);
        }
        it->files.append(file);
    }

    db.groups = groups.values();
    return db;
}

static FilePath compileDatabaseFile(const FilePath &buildDir, const QString &appName)
{
    // A sysbuild puts the application into a subdirectory named after it.
    const FilePath sysbuildAppDb = buildDir / appName / "compile_commands.json";
    if (sysbuildAppDb.isReadableFile())
        return sysbuildAppDb;
    return buildDir / "compile_commands.json";
}

// Builds what source needs in the build directory that holds dbFile, which for
// a sysbuild is the one of the application and not the top level one.
static CommandLine generatedHeadersCommand(const FilePath &west,
                                           const FilePath &dbFile,
                                           const FilePath &source)
{
    return {west, {"build", "-d", dbFile.parentDir().nativePath(),
                   "-o", source.nativePath() + '^'}};
}

// A build rewrites the database only when CMake reconfigures.
class CompileDatabaseStamp
{
public:
    // Returns whether dbFile differs from the one recorded last, and records it.
    bool update(const FilePath &dbFile)
    {
        const QDateTime lastModified = dbFile.lastModified();
        if (dbFile == m_file && lastModified == m_lastModified)
            return false;
        m_file = dbFile;
        m_lastModified = lastModified;
        return true;
    }

private:
    FilePath m_file;
    QDateTime m_lastModified;
};

// Before the first build there is no database, which is not an error.
static Result<CompileDatabase> readCompileDatabase(const FilePath &dbFile, const FilePath &buildDir)
{
    if (!dbFile.exists())
        return CompileDatabase();
    const Result<QByteArray> contents = dbFile.fileContents();
    if (!contents)
        return ResultError(contents.error());
    const Result<CompileDatabase> db = parseCompileDatabase(*contents, buildDir);
    if (!db) {
        return ResultError(
            Tr::tr("Cannot parse \"%1\": %2").arg(dbFile.toUserOutput(), db.error()));
    }
    return db;
}

// The toolchains detected for the compilers of the build replace those of the kit.
static RawProjectParts projectPartsFromDatabase(KitInfo &kitInfo,
                                                const CompileDatabase &db,
                                                Toolchain *detectedC,
                                                Toolchain *detectedCxx,
                                                const QString &displayName,
                                                const FilePath &projectFile,
                                                const FilePath &buildDir)
{
    if (detectedC)
        kitInfo.cToolchain = detectedC;
    if (detectedCxx)
        kitInfo.cxxToolchain = detectedCxx;

    RawProjectParts rpps;
    for (const CompileGroup &group : db.groups) {
        RawProjectPart rpp;
        rpp.setDisplayName(group.target.isEmpty() ? displayName : group.target);
        rpp.setProjectFileLocation(projectFile);
        rpp.setBuildSystemTarget(group.target);
        rpp.setHeaderPaths(group.headerPaths);
        rpp.setMacros(group.macros);
        rpp.setIncludedFiles(group.includedFiles);
        rpp.setFiles(group.files);
        if (group.isCxx)
            rpp.setFlagsForCxx({kitInfo.cxxToolchain, group.flags, buildDir});
        else
            rpp.setFlagsForC({kitInfo.cToolchain, group.flags, buildDir});
        rpps.append(rpp);
    }
    return rpps;
}

// Project files

// Many Zephyr files have no suffix the code model would know, like Kconfig.
static FilePaths scanProjectFiles(const FilePath &root, const FilePath &buildDir)
{
    static const QStringList nameFilters = {
        "*.c", "*.h", "*.cpp", "*.hpp", "CMakeLists.txt", "*.cmake",
        "*.conf", "*.overlay", "*.dts", "*.dtsi", "*.yml", "*.yaml",
        "Kconfig", "Kconfig.*", "*_defconfig", "Dockerfile", "Dockerfile.*", "Makefile",
        "*.patch", "*.sh", "*.py", "*.rst", "*.md"};
    static const QStringList skipDirs = {".git", ".west", "build"};

    FilePaths files;
    root.iterateDirectory(
        [&](const FilePath &path) {
            if (buildDir.isChildOf(root) && path.isChildOf(buildDir))
                return IterationPolicy::Continue;
            const QStringList dirs = path.relativeChildPath(root).path().split('/');
            for (int i = 0; i < dirs.size() - 1; ++i) {
                if (skipDirs.contains(dirs.at(i)))
                    return IterationPolicy::Continue;
            }
            files.append(path);
            return IterationPolicy::Continue;
        },
        {nameFilters, DirFilterFlag::Files, DirIteratorFlag::Subdirectories});
    return files;
}

// ZephyrBuildSystem

class ZephyrBuildSystem final : public BuildSystem
{
public:
    explicit ZephyrBuildSystem(BuildConfiguration *bc)
        : BuildSystem(bc)
        , m_cppCodeModelUpdater(ProjectUpdaterFactory::createCppProjectUpdater())
    {
        const auto reparseIfActive = [this] {
            if (project()->activeBuildSystem() == this)
                requestDelayedParse();
        };
        connect(project(), &Project::projectFileIsDirty, this, reparseIfActive);
        connect(project(), &Project::activeBuildConfigurationChanged, this, reparseIfActive);
        connect(bc, &BuildConfiguration::buildDirectoryChanged, this, reparseIfActive);
        // A build does not change the source tree, only the compile database.
        connect(BuildManager::instance(), &BuildManager::buildStateChanged, this,
                [this](Project *p) {
                    if (p != project() || BuildManager::isBuilding(p)
                        || project()->activeBuildSystem() != this) {
                        return;
                    }
                    if (m_compileDatabaseStamp.update(compileDatabaseFile()))
                        updateCppCodeModel();
                });
        requestDelayedParse();
    }

    static QString name() { return "zephyr"; }

    void triggerParsing() final
    {
        if (m_configureProcess || m_configureReader.isRunning()) {
            m_reparseAfterConfigure = true;
            return;
        }

        ParseGuard guard = guardParsingRun();
        const FilePath root = projectDirectory();
        const FilePath buildDir = buildConfiguration()->buildDirectory();

        auto newRoot = std::make_unique<ProjectNode>(root);
        for (const FilePath &f : scanProjectFiles(root, buildDir))
            newRoot->addNestedNode(std::make_unique<FileNode>(f, FileNode::fileTypeForFileName(f)));
        setRootProjectNode(std::move(newRoot));

        if (project()->activeBuildSystem() == this && !compileDatabaseFile().exists()) {
            m_configureGuard = std::move(guard);
            if (startConfigure(buildDir))
                return;
            guard = std::move(m_configureGuard);
        }

        finishParsing(guard);
    }

private:
    void finishParsing(ParseGuard &guard,
                       const std::optional<Result<CompileDatabase>> &parsed = {})
    {
        updateCppCodeModel(parsed);
        guard.markAsSuccess();
        emitBuildSystemUpdated();
    }

    FilePath compileDatabaseFile() const
    {
        return Internal::compileDatabaseFile(buildConfiguration()->buildDirectory(),
                                             projectDirectory().fileName());
    }

    // Without a build there are no compile commands for the code model. The
    // CMake part of "west build" produces them.
    bool startConfigure(const FilePath &buildDir)
    {
        // Once per build directory, so a failing configuration is not repeated
        // on every reparse.
        if (m_configuredBuildDir == buildDir || BuildManager::isBuilding(project()))
            return false;
        const CommandLine configure = westConfigureCommand(buildConfiguration());
        if (configure.executable().isEmpty())
            return false;
        m_configuredBuildDir = buildDir;

        const std::function<void(bool)> recordConfigure
            = westConfigureRecorder(buildConfiguration());
        runConfigureStep(configure, [this, buildDir, recordConfigure,
                                     west = configure.executable()](bool success) {
            recordConfigure(success);
            if (!success) {
                finishConfigure();
                return;
            }
            // <zephyr/kernel.h> includes headers that the build generates, not the
            // configuration. Ninja's "<source>^" builds what one source needs.
            const FilePath dbFile
                = Internal::compileDatabaseFile(buildDir, projectDirectory().fileName());
            const auto onSetup = [dbFile, buildDir](Async<Result<CompileDatabase>> &task) {
                task.setConcurrentCallData(readCompileDatabase, dbFile, buildDir);
            };
            const auto onDone = [this, dbFile, west](const Async<Result<CompileDatabase>> &task) {
                if (task.isResultAvailable())
                    m_configuredDatabase = task.result();
                const FilePath source = m_configuredDatabase
                                            ? firstApplicationSource(*m_configuredDatabase)
                                            : FilePath();
                if (source.isEmpty()) {
                    finishConfigure();
                    return;
                }
                runConfigureStep(generatedHeadersCommand(west, dbFile, source),
                                 [this](bool) { finishConfigure(); });
            };
            m_configureReader.start({AsyncTask<Result<CompileDatabase>>(onSetup, onDone)});
        });
        return true;
    }

    static FilePath firstApplicationSource(const Result<CompileDatabase> &db)
    {
        if (!db)
            return {};
        for (const CompileGroup &group : db->groups) {
            if (group.target == "app" && !group.files.isEmpty())
                return group.files.first();
        }
        return {};
    }

    void runConfigureStep(const CommandLine &cmd, const std::function<void(bool)> &onDone)
    {
        m_configureProcess = new Process(this);
        m_configureProcess->setCommand(cmd);
        m_configureProcess->setWorkingDirectory(westWorkingDirectory(project()));
        m_configureProcess->setEnvironment(buildConfiguration()->environment());
        auto progress = new Core::ProcessProgress(m_configureProcess);
        progress->setDisplayName(Tr::tr("Configuring \"%1\"").arg(project()->displayName()));
        startNewBuildSystemOutput(Tr::tr("Running %1.").arg(cmd.toUserOutput()));
        connect(m_configureProcess, &Process::done, this, [this, onDone] {
            const bool success = m_configureProcess->result() == ProcessResult::FinishedWithSuccess;
            if (!success) {
                appendBuildSystemOutput(m_configureProcess->exitMessage() + '\n'
                                        + m_configureProcess->allOutput());
            }
            m_configureProcess->deleteLater();
            m_configureProcess = nullptr;
            onDone(success);
        });
        m_configureProcess->start();
    }

    void finishConfigure()
    {
        ParseGuard guard = std::move(m_configureGuard);
        finishParsing(guard, std::exchange(m_configuredDatabase, std::nullopt));
        if (m_reparseAfterConfigure) {
            m_reparseAfterConfigure = false;
            requestDelayedParse();
        }
    }

    // A Zephyr database is several megabytes, so it is read in a worker.
    void updateCppCodeModel(const std::optional<Result<CompileDatabase>> &parsed = {})
    {
        // CppModelManager keeps one project info per project.
        if (!m_cppCodeModelUpdater || project()->activeBuildSystem() != this)
            return;

        const FilePath buildDir = buildConfiguration()->buildDirectory();
        const FilePath dbFile = compileDatabaseFile();
        m_compileDatabaseStamp.update(dbFile);
        if (parsed) {
            applyCompileDatabase(*parsed, buildDir);
            return;
        }
        const auto onSetup = [dbFile, buildDir](Async<Result<CompileDatabase>> &task) {
            task.setConcurrentCallData(readCompileDatabase, dbFile, buildDir);
        };
        const auto onDone = [this, buildDir](const Async<Result<CompileDatabase>> &task) {
            if (task.isResultAvailable())
                applyCompileDatabase(task.result(), buildDir);
        };
        m_compileDatabaseReader.start({AsyncTask<Result<CompileDatabase>>(onSetup, onDone)});
    }

    void applyCompileDatabase(const Result<CompileDatabase> &db, const FilePath &buildDir)
    {
        if (!m_cppCodeModelUpdater || project()->activeBuildSystem() != this)
            return;
        KitInfo kitInfo(kit());
        QTC_ASSERT(kitInfo.isValid(), return);

        // Keep the previous project parts rather than replace them with nothing.
        if (!db) {
            Core::MessageManager::writeSilently(db.error());
            return;
        }

        Toolchain *detectedC = toolchainForCompiler(db->cCompiler, PEConstants::C_LANGUAGE_ID,
                                                    m_detectedToolchains);
        Toolchain *detectedCxx = toolchainForCompiler(db->cxxCompiler,
                                                      PEConstants::CXX_LANGUAGE_ID,
                                                      m_detectedToolchains);
        const RawProjectParts rpps = projectPartsFromDatabase(kitInfo, *db, detectedC,
                                                              detectedCxx,
                                                              project()->displayName(),
                                                              projectFilePath(), buildDir);
        m_cppCodeModelUpdater->update({project(), kitInfo, activeParseEnvironment(), rpps});
    }

    std::unique_ptr<ProjectUpdater> m_cppCodeModelUpdater;
    OwnedToolchains m_detectedToolchains;
    CompileDatabaseStamp m_compileDatabaseStamp;
    QSingleTaskTreeRunner m_compileDatabaseReader;
    Process *m_configureProcess = nullptr;
    QSingleTaskTreeRunner m_configureReader;
    ParseGuard m_configureGuard;
    std::optional<Result<CompileDatabase>> m_configuredDatabase;
    FilePath m_configuredBuildDir;
    bool m_reparseAfterConfigure = false;
};

// ZephyrBuildConfiguration

class ZephyrBuildConfiguration final : public BuildConfiguration
{
public:
    ZephyrBuildConfiguration(Target *target, Id id)
        : BuildConfiguration(target, id)
    {
        setBuildDirectoryHistoryCompleter("Zephyr.BuildDir.History");
        setConfigWidgetDisplayName(Tr::tr("Zephyr"));
        appendInitialBuildStep(Constants::WEST_BUILD_STEP_ID);
    }
};

class ZephyrBuildConfigurationFactory final : public BuildConfigurationFactory
{
public:
    ZephyrBuildConfigurationFactory()
    {
        registerBuildConfiguration<ZephyrBuildConfiguration>("Zephyr.ZephyrBuildConfiguration");
        setSupportedProjectType(Constants::ZEPHYR_PROJECT_ID);
        setSupportedProjectMimeTypeName(WEST_MANIFEST_MIMETYPE);
        setBuildGenerator([](const Kit *, const FilePath &projectPath, bool forSetup) {
            BuildInfo info;
            info.typeName = Tr::tr("Build");
            info.buildDirectory = forSetup ? projectPath.parentDir() / "build" : projectPath;
            info.buildSystemName = "zephyr";
            if (forSetup)
                info.displayName = Tr::tr("Default");
            return QList<BuildInfo>{info};
        });
    }
};

// ZephyrProject

class ZephyrProject final : public Project
{
public:
    explicit ZephyrProject(const FilePath &file)
        : Project(WEST_MANIFEST_MIMETYPE, file)
    {
        setType(Constants::ZEPHYR_PROJECT_ID);
        setProjectLanguages(Core::Context(ProjectExplorer::Constants::C_LANGUAGE_ID));
        setDisplayName(projectDirectory().fileName());
        setBuildSystemCreator<ZephyrBuildSystem>();
    }
};

void setupZephyrProject()
{
    static ZephyrBuildConfigurationFactory theBuildConfigurationFactory;
    ProjectManager::registerProjectType<ZephyrProject>(WEST_MANIFEST_MIMETYPE);
}

#ifdef WITH_TESTS

class ZephyrCompileDatabaseTest final : public QObject
{
    Q_OBJECT

private slots:
    void testGrouping();
    void testMalformed();
    void testTargetTriple_data();
    void testTargetTriple();
    void testToolchainNotRegistered();
    void testCMakeTarget_data();
    void testCMakeTarget();
    void testCompileDatabaseFile();
    void testCompileDatabaseStamp();
    void testProjectParts_data();
    void testProjectParts();
};

void ZephyrCompileDatabaseTest::testGrouping()
{
    const QByteArray json = R"([
      {"directory": "/b", "file": "/src/a.c", "output": "zephyr/CMakeFiles/zephyr.dir/a.c.obj",
       "command": "/sdk/arm-zephyr-eabi-gcc -DKERNEL -I/inc -isystem /sys -imacros /b/autoconf.h -mcpu=cortex-m3 -o zephyr/CMakeFiles/zephyr.dir/a.c.obj -c /src/a.c"},
      {"directory": "/b", "file": "/src/b.c", "output": "zephyr/CMakeFiles/zephyr.dir/b.c.obj",
       "command": "/sdk/arm-zephyr-eabi-gcc -DKERNEL -I/inc -isystem /sys -imacros /b/autoconf.h -mcpu=cortex-m3 -o zephyr/CMakeFiles/zephyr.dir/b.c.obj -c /src/b.c"},
      {"directory": "/b", "file": "/app/main.cpp", "output": "CMakeFiles/app.dir/main.cpp.obj",
       "arguments": ["/sdk/arm-zephyr-eabi-g++", "-I", "rel", "-UNDEBUG", "-c", "/app/main.cpp"]}
    ])";

    const Result<CompileDatabase> result = parseCompileDatabase(json, FilePath::fromString("/b"));
    QVERIFY_RESULT(result);
    const CompileDatabase &db = *result;
    QCOMPARE(db.cCompiler, FilePath::fromString("/sdk/arm-zephyr-eabi-gcc"));
    QCOMPARE(db.cxxCompiler, FilePath::fromString("/sdk/arm-zephyr-eabi-g++"));
    QCOMPARE(db.groups.size(), 2);

    const CompileGroup &app = db.groups.at(0).target == "app" ? db.groups.at(0) : db.groups.at(1);
    const CompileGroup &zephyr = db.groups.at(0).target == "app" ? db.groups.at(1) : db.groups.at(0);

    QCOMPARE(zephyr.target, "zephyr");
    QVERIFY(!zephyr.isCxx);
    QCOMPARE(zephyr.files, FilePaths({FilePath::fromString("/src/a.c"),
                                      FilePath::fromString("/src/b.c")}));
    QCOMPARE(zephyr.headerPaths, HeaderPaths({HeaderPath::makeUser(FilePath::fromString("/inc")),
                                              HeaderPath::makeSystem(FilePath::fromString("/sys"))}));
    QCOMPARE(zephyr.macros, Macros({Macro("KERNEL", "1")}));
    QCOMPARE(zephyr.includedFiles, FilePaths({FilePath::fromString("/b/autoconf.h")}));
    QCOMPARE(zephyr.flags, QStringList({"-mcpu=cortex-m3"}));

    QVERIFY(app.isCxx);
    QCOMPARE(app.headerPaths, HeaderPaths({HeaderPath::makeUser(FilePath::fromString("/b/rel"))}));
    QCOMPARE(app.macros, Macros({Macro("NDEBUG", MacroType::Undefine)}));
    QVERIFY(app.flags.isEmpty());
}

void ZephyrCompileDatabaseTest::testMalformed()
{
    const FilePath buildDir = FilePath::fromString("/b");
    QVERIFY(!parseCompileDatabase(R"([{"directory": "/b", "file": "/src/a.c")", buildDir));
    QVERIFY(!parseCompileDatabase("", buildDir));
    QVERIFY(!parseCompileDatabase(R"({"directory": "/b"})", buildDir));
    QVERIFY(parseCompileDatabase("[]", buildDir));

    const Result<CompileDatabase> noFile = parseCompileDatabase(
        R"([{"directory": "/b", "command": "gcc -c a.c"}])", buildDir);
    QVERIFY_RESULT(noFile);
    QVERIFY(noFile->groups.isEmpty());
}

void ZephyrCompileDatabaseTest::testTargetTriple_data()
{
    QTest::addColumn<QString>("compiler");
    QTest::addColumn<QString>("triple");

    QTest::newRow("sdk") << "/sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc" << "arm-zephyr-eabi";
    QTest::newRow("c++") << "x86_64-zephyr-elf-g++" << "x86_64-zephyr-elf";
    QTest::newRow("versioned") << "/usr/bin/arm-none-eabi-gcc-14.2" << "arm-none-eabi";
    QTest::newRow("windows") << "C:/sdk/riscv64-zephyr-elf-gcc.exe" << "riscv64-zephyr-elf";
    QTest::newRow("host") << "/usr/bin/gcc" << "";
    QTest::newRow("host versioned") << "/usr/bin/gcc-14" << "";
}

void ZephyrCompileDatabaseTest::testTargetTriple()
{
    QFETCH(QString, compiler);
    QFETCH(QString, triple);

    QCOMPARE(targetTripleFromCompiler(FilePath::fromUserInput(compiler)), triple);
}

void ZephyrCompileDatabaseTest::testToolchainNotRegistered()
{
    if (HostOsInfo::isWindowsHost())
        QSKIP("The fake compilers are shell scripts.");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath tmp = FilePath::fromString(dir.path());
    const FilePath ran = tmp / "ran";
    const auto fakeCompiler = [&](const QString &name) {
        const FilePath compiler = tmp / name;
        QTC_CHECK(compiler.writeFileContents("#!/bin/sh\ntouch " + ran.path().toUtf8() + "\n"));
        QTC_CHECK(compiler.setPermissions(QFile::ReadOwner | QFile::WriteOwner
                                          | QFile::ExeOwner));
        return compiler;
    };
    const FilePath compiler = fakeCompiler("x86_64-zephyrtest-elf-gcc");

    const Toolchains before = ToolchainManager::toolchains();
    OwnedToolchains detected;
    const Toolchain *tc = toolchainForCompiler(compiler, PEConstants::C_LANGUAGE_ID, detected);
    QVERIFY(tc);
    QCOMPARE(tc->typeId(), PEConstants::GCC_TOOLCHAIN_TYPEID);
    QCOMPARE(tc->targetAbi().architecture(), Abi::X86Architecture);
    QCOMPARE(tc->effectiveCodeModelTargetTriple(), "x86_64-zephyrtest-elf");
    QCOMPARE(ToolchainManager::toolchains(), before);
    QCOMPARE(toolchainForCompiler(compiler, PEConstants::C_LANGUAGE_ID, detected), tc);

    const FilePath clang = fakeCompiler("riscv64-zephyrtest-elf-clang");
    tc = toolchainForCompiler(clang, PEConstants::CXX_LANGUAGE_ID, detected);
    QVERIFY(tc);
    QCOMPARE(tc->typeId(), PEConstants::CLANG_TOOLCHAIN_TYPEID);

    // Without a triple there is nothing to tell the code model.
    QVERIFY(!toolchainForCompiler(fakeCompiler("gcc"), PEConstants::C_LANGUAGE_ID, detected));

    const FilePath gone = tmp / "arm-zephyrtest-eabi-gcc";
    const size_t count = detected.size();
    QVERIFY(!toolchainForCompiler(gone, PEConstants::C_LANGUAGE_ID, detected));
    QVERIFY(!toolchainForCompiler(gone, PEConstants::C_LANGUAGE_ID, detected));
    QCOMPARE(detected.size(), count + 1);
    fakeCompiler(gone.fileName());
    QVERIFY(toolchainForCompiler(gone, PEConstants::C_LANGUAGE_ID, detected));
    QCOMPARE(detected.size(), count + 1);
    QVERIFY(!ran.exists());
}

void ZephyrCompileDatabaseTest::testProjectParts_data()
{
    QTest::addColumn<bool>("detected");
    QTest::addColumn<QString>("expectedC");
    QTest::addColumn<QString>("expectedCxx");

    QTest::newRow("detected toolchain") << true << "detected C" << "detected C++";
    QTest::newRow("nothing detected") << false << "kit" << "kit";
}

void ZephyrCompileDatabaseTest::testProjectParts()
{
    QFETCH(bool, detected);
    QFETCH(QString, expectedC);
    QFETCH(QString, expectedCxx);

    std::map<QString, std::unique_ptr<Toolchain>> toolchains;
    const auto create = [&toolchains](const QString &name, Id type) {
        Toolchain *tc = ToolchainFactory::createToolchain(type);
        QTC_ASSERT(tc, return);
        toolchains[name].reset(tc);
    };
    create("kit", PEConstants::CLANG_TOOLCHAIN_TYPEID);
    create("detected C", PEConstants::GCC_TOOLCHAIN_TYPEID);
    create("detected C++", PEConstants::GCC_TOOLCHAIN_TYPEID);
    QCOMPARE(toolchains.size(), 3u);
    const auto nameOf = [&toolchains](const Toolchain *tc) {
        for (const auto &[name, owned] : toolchains) {
            if (owned.get() == tc)
                return name;
        }
        return QString();
    };

    CompileDatabase db;
    db.cCompiler = FilePath::fromString("/sdk/arm-zephyr-eabi-gcc");
    db.cxxCompiler = FilePath::fromString("/sdk/arm-zephyr-eabi-g++");
    CompileGroup cGroup;
    cGroup.target = "zephyr";
    cGroup.flags = {"-mcpu=cortex-m3"};
    CompileGroup cxxGroup;
    cxxGroup.target = "app";
    cxxGroup.isCxx = true;
    db.groups = {cGroup, cxxGroup};

    KitInfo kitInfo(nullptr);
    kitInfo.cToolchain = toolchains["kit"].get();
    kitInfo.cxxToolchain = toolchains["kit"].get();
    const RawProjectParts rpps = projectPartsFromDatabase(
        kitInfo, db,
        detected ? toolchains["detected C"].get() : nullptr,
        detected ? toolchains["detected C++"].get() : nullptr,
        "project", FilePath::fromString("/src/west.yml"), FilePath::fromString("/b"));

    QCOMPARE(nameOf(kitInfo.cToolchain), expectedC);
    QCOMPARE(nameOf(kitInfo.cxxToolchain), expectedCxx);
    QCOMPARE(rpps.size(), 2);
    QCOMPARE(rpps.at(0).buildSystemTarget, "zephyr");
    QCOMPARE(rpps.at(0).flagsForC.commandLineFlags, QStringList{"-mcpu=cortex-m3"});
    QCOMPARE(rpps.at(1).buildSystemTarget, "app");
}

void ZephyrCompileDatabaseTest::testCMakeTarget_data()
{
    QTest::addColumn<QString>("output");
    QTest::addColumn<QString>("target");

    QTest::newRow("unix") << "zephyr/CMakeFiles/zephyr.dir/lib/os/printk.c.obj" << "zephyr";
    QTest::newRow("windows") << "zephyr\\CMakeFiles\\zephyr.dir\\lib\\os\\printk.c.obj"
                             << "zephyr";
    QTest::newRow("app") << "CMakeFiles/app.dir/src/main.c.obj" << "app";
    QTest::newRow("no target") << "main.o" << "";
}

void ZephyrCompileDatabaseTest::testCMakeTarget()
{
    QFETCH(QString, output);
    QFETCH(QString, target);

    QCOMPARE(cmakeTargetFromOutput(output), target);
}

void ZephyrCompileDatabaseTest::testCompileDatabaseFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath buildDir = FilePath::fromString(dir.path());

    QCOMPARE(compileDatabaseFile(buildDir, "app"), buildDir / "compile_commands.json");
    QVERIFY((buildDir / "app").createDir());
    QVERIFY_RESULT((buildDir / "app/compile_commands.json").writeFileContents("[]"));
    QCOMPARE(compileDatabaseFile(buildDir, "app"), buildDir / "app/compile_commands.json");

    const FilePath west = FilePath::fromString("/bin/west");
    const FilePath source = FilePath::fromString("/src/main.c");
    const CommandLine expected(west, {"build", "-d", (buildDir / "app").nativePath(), "-o",
                                      source.nativePath() + '^'});
    QCOMPARE(generatedHeadersCommand(west, compileDatabaseFile(buildDir, "app"), source)
                 .toUserOutput(),
             expected.toUserOutput());
}

void ZephyrCompileDatabaseTest::testCompileDatabaseStamp()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath dbFile = FilePath::fromString(dir.path()) / "compile_commands.json";

    CompileDatabaseStamp stamp;
    QVERIFY(stamp.update(dbFile));
    QVERIFY(!stamp.update(dbFile));

    QVERIFY_RESULT(dbFile.writeFileContents("[]"));
    QVERIFY(stamp.update(dbFile));
    QVERIFY(!stamp.update(dbFile));

    QFile file(dbFile.toFSPathString());
    QVERIFY(file.open(QIODevice::ReadWrite));
    QVERIFY(file.setFileTime(dbFile.lastModified().addSecs(10),
                             QFileDevice::FileModificationTime));
    file.close();
    QVERIFY(stamp.update(dbFile));
    QVERIFY(!stamp.update(dbFile));

    const FilePath sysbuildDbFile = dbFile.parentDir() / "app/compile_commands.json";
    QVERIFY(stamp.update(sysbuildDbFile));
}

QObject *createZephyrCompileDatabaseTest()
{
    return new ZephyrCompileDatabaseTest;
}

class ZephyrParseFlowTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    void testReparseDuringConfigure();
    void testConfigureFails();
    void testNoApplicationSource();
    void testProjectFiles_data();
    void testProjectFiles();

private:
    void openProject();
    QStringList westCalls() const;

    Kit *m_kit = nullptr;
    QString m_previousWest;
    std::unique_ptr<QTemporaryDir> m_dir;
    FilePath m_tmp;
    FilePath m_source;
    Project *m_project = nullptr;
    BuildSystem *m_buildSystem = nullptr;
    std::unique_ptr<QSignalSpy> m_started;
    std::unique_ptr<QSignalSpy> m_finished;
};

void ZephyrParseFlowTest::initTestCase()
{
    m_previousWest = settings().westFilePath.value();
    if (HostOsInfo::isWindowsHost())
        QSKIP("The fake west is a shell script.");
    m_kit = KitManager::registerKit([](Kit *k) { k->setUnexpandedDisplayName("Zephyr Test"); });
    QVERIFY(m_kit);
}

void ZephyrParseFlowTest::cleanupTestCase()
{
    if (m_kit)
        KitManager::deregisterKit(m_kit);
    settings().westFilePath.setValue(m_previousWest);
}

// The fake west logs the arguments of "west build". Its configure step blocks
// while "hold" exists, fails if "fail" exists and otherwise copies "db.json"
// to the compile database.
void ZephyrParseFlowTest::init()
{
    m_dir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_dir->isValid());
    m_tmp = FilePath::fromString(m_dir->path());

    const FilePath west = m_tmp / "bin/west";
    QVERIFY(west.parentDir().createDir());
    QVERIFY_RESULT(west.writeFileContents(R"(#!/bin/sh
[ "$1" = build ] || exit 0
tmp=$(dirname "$0")/..
echo "$*" >> "$tmp/log"
case " $* " in
*" --cmake-only "*)
    while [ -e "$tmp/hold" ]; do sleep 0.05; done
    [ -e "$tmp/fail" ] && exit 1
    while [ $# -gt 0 ]; do
        [ "$1" = -d ] && build=$2
        shift
    done
    mkdir -p "$build" && cp "$tmp/db.json" "$build/compile_commands.json"
    ;;
esac
exit 0
)"));
    QVERIFY(west.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
    settings().westFilePath.setValue(west);

    const FilePath projectDir = m_tmp / "hello";
    m_source = projectDir / "src/main.c";
    QVERIFY(m_source.parentDir().createDir());
    QVERIFY_RESULT(m_source.writeFileContents("int main() { return 0; }\n"));
    QVERIFY_RESULT((projectDir / "CMakeLists.txt").writeFileContents("project(hello)\n"));
    QVERIFY_RESULT((projectDir / "west.yml").writeFileContents("manifest:\n  self:\n"));
    const QJsonObject entry{{"directory", (projectDir / "build").path()},
                            {"file", m_source.path()},
                            {"output", "CMakeFiles/app.dir/src/main.c.obj"},
                            {"command", QString("/sdk/arm-zephyr-eabi-gcc -c " + m_source.path())}};
    QVERIFY_RESULT((m_tmp / "db.json")
                       .writeFileContents(QJsonDocument(QJsonArray{entry}).toJson()));
}

void ZephyrParseFlowTest::cleanup()
{
    (m_tmp / "hold").removeFile();
    m_started.reset();
    m_finished.reset();
    if (m_project)
        ProjectExplorerPlugin::unloadProject(m_project);
    m_project = nullptr;
    m_buildSystem = nullptr;
    m_dir.reset();
}

void ZephyrParseFlowTest::openProject()
{
    const OpenProjectResult result = ProjectExplorerPlugin::openProject(m_tmp / "hello/west.yml");
    QVERIFY2(result && result.project(), qPrintable(result.errorMessage()));
    m_project = result.project();
    QVERIFY(m_project->configureAsExampleProject(m_kit));
    m_buildSystem = m_project->activeBuildSystem();
    QVERIFY(m_buildSystem);
    m_started = std::make_unique<QSignalSpy>(m_buildSystem, &BuildSystem::parsingStarted);
    m_finished = std::make_unique<QSignalSpy>(m_buildSystem, &BuildSystem::parsingFinished);
}

QStringList ZephyrParseFlowTest::westCalls() const
{
    const QByteArray log = (m_tmp / "log").fileContents().value_or(QByteArray());
    return QString::fromUtf8(log).split('\n', Qt::SkipEmptyParts);
}

void ZephyrParseFlowTest::testReparseDuringConfigure()
{
    QVERIFY_RESULT((m_tmp / "hold").writeFileContents({}));
    openProject();
    if (QTest::currentTestFailed())
        return;

    QTRY_COMPARE(westCalls().size(), 1);
    QVERIFY(westCalls().at(0).contains("--cmake-only"));
    QCOMPARE(m_started->size(), 1);

    m_buildSystem->requestParse();
    QTRY_VERIFY(!m_buildSystem->isWaitingForParse());
    QCOMPARE(m_started->size(), 1);
    QCOMPARE(m_finished->size(), 0);

    QVERIFY((m_tmp / "hold").removeFile());
    QTRY_COMPARE(m_finished->size(), 2);
    QCOMPARE(m_started->size(), 2);
    const QStringList calls = westCalls();
    QCOMPARE(calls.size(), 2);
    QVERIFY(calls.at(1).endsWith("-o " + m_source.nativePath() + '^'));

    m_buildSystem->requestParse();
    QTRY_COMPARE(m_finished->size(), 3);
    QCOMPARE(m_started->size(), 3);
    QCOMPARE(westCalls().size(), 2);
}

void ZephyrParseFlowTest::testConfigureFails()
{
    QVERIFY_RESULT((m_tmp / "fail").writeFileContents({}));
    openProject();
    if (QTest::currentTestFailed())
        return;

    QTRY_COMPARE(m_finished->size(), 1);
    QCOMPARE(m_started->size(), 1);
    QCOMPARE(westCalls().size(), 1);

    // Once per build directory.
    m_buildSystem->requestParse();
    QTRY_COMPARE(m_finished->size(), 2);
    QCOMPARE(westCalls().size(), 1);
}

void ZephyrParseFlowTest::testNoApplicationSource()
{
    const QJsonObject entry{{"directory", (m_tmp / "hello/build").path()},
                            {"file", "/zephyr/kernel/init.c"},
                            {"output", "zephyr/CMakeFiles/zephyr.dir/kernel/init.c.obj"},
                            {"command", "/sdk/arm-zephyr-eabi-gcc -c /zephyr/kernel/init.c"}};
    QVERIFY_RESULT((m_tmp / "db.json")
                       .writeFileContents(QJsonDocument(QJsonArray{entry}).toJson()));
    openProject();
    if (QTest::currentTestFailed())
        return;

    QTRY_COMPARE(m_finished->size(), 1);
    QCOMPARE(m_started->size(), 1);
    QCOMPARE(westCalls().size(), 1);
}

void ZephyrParseFlowTest::testProjectFiles_data()
{
    QTest::addColumn<QString>("buildSubdir");
    QTest::newRow("default build directory") << QString("build");
    QTest::newRow("in-source build") << QString();
    QTest::newRow("build directory not skipped by name") << QString("out");
}

void ZephyrParseFlowTest::testProjectFiles()
{
    QFETCH(QString, buildSubdir);
    const FilePath projectDir = m_tmp / "hello";
    const QStringList listed = {"CMakeLists.txt", "west.yml", "prj.conf", "Kconfig",
                                "Kconfig.app", "Dockerfile", "boards/disco.overlay",
                                "boards/disco.conf", "scripts/flash.sh",
                                "zephyr/patches/fix.patch", "cmake/extra.cmake", "src/main.c",
                                "build-other/prj.conf", "boards/disco/disco_defconfig"};
    const QStringList unlisted = {"build/zephyr/.config", "build/zephyr/dts.overlay",
                                  "notes.txt"};
    const QString outOverlay = "out/zephyr/dts.overlay";
    for (const QString &name : listed + unlisted + QStringList{outOverlay}) {
        const FilePath file = projectDir / name;
        QVERIFY(file.parentDir().ensureWritableDir());
        if (!file.exists())
            QVERIFY_RESULT(file.writeFileContents("\n"));
    }
    openProject();
    if (QTest::currentTestFailed())
        return;
    QTRY_COMPARE(m_finished->size(), 1);
    if (buildSubdir != "build") {
        m_project->activeBuildConfiguration()->setBuildDirectory(
            buildSubdir.isEmpty() ? projectDir : projectDir / buildSubdir);
        QTRY_COMPARE(m_finished->size(), 2);
    }

    FilePaths expected;
    for (const QString &name : listed)
        expected.append(projectDir / name);
    if (buildSubdir != "out")
        expected.append(projectDir / outOverlay);
    FilePaths files = m_project->files(Project::SourceFiles);
    files.removeAll(projectDir);
    Utils::sort(expected);
    Utils::sort(files);
    QCOMPARE(files, expected);
}

QObject *createZephyrParseFlowTest()
{
    return new ZephyrParseFlowTest;
}

#endif // WITH_TESTS

} // namespace Zephyr::Internal

#ifdef WITH_TESTS
#include "zephyrproject.moc"
#endif
