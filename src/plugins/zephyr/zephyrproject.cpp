// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "zephyrproject.h"

#include "zephyrconstants.h"
#include "zephyrtr.h"

#include <coreplugin/icontext.h>
#include <coreplugin/messagemanager.h>

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
#include <utils/qtcassert.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include <QtTaskTree/QSingleTaskTreeRunner>

#ifdef WITH_TESTS
#include <QFile>
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
        ParseGuard guard = guardParsingRun();
        const FilePath root = projectDirectory();

        static const QStringList skipDirs = {".git", ".west", "build"};

        FilePaths files;
        root.iterateDirectory(
            [&](const FilePath &path) {
                const QString pathStr = path.path();
                for (const QString &skip : skipDirs) {
                    if (pathStr.contains('/' + skip + '/') || pathStr.endsWith('/' + skip))
                        return IterationPolicy::Continue;
                }
                files.append(path);
                return IterationPolicy::Continue;
            },
            {{"*.c", "*.h", "*.cpp", "*.hpp", "*.cmake", "*.conf", "*.yml", "*.yaml",
              "CMakeLists.txt"},
             DirFilterFlag::Files,
             DirIteratorFlag::Subdirectories});

        auto newRoot = std::make_unique<ProjectNode>(root);
        for (const FilePath &f : files)
            newRoot->addNestedNode(std::make_unique<FileNode>(f, FileNode::fileTypeForFileName(f)));
        setRootProjectNode(std::move(newRoot));

        updateCppCodeModel();

        guard.markAsSuccess();
        emitBuildSystemUpdated();
    }

private:
    FilePath compileDatabaseFile() const
    {
        return Internal::compileDatabaseFile(buildConfiguration()->buildDirectory(),
                                             projectDirectory().fileName());
    }

    // A Zephyr database is several megabytes, so it is read in a worker.
    void updateCppCodeModel()
    {
        // CppModelManager keeps one project info per project.
        if (!m_cppCodeModelUpdater || project()->activeBuildSystem() != this)
            return;

        const FilePath buildDir = buildConfiguration()->buildDirectory();
        const FilePath dbFile = compileDatabaseFile();
        m_compileDatabaseStamp.update(dbFile);
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

#endif // WITH_TESTS

} // namespace Zephyr::Internal

#ifdef WITH_TESTS
#include "zephyrproject.moc"
#endif
