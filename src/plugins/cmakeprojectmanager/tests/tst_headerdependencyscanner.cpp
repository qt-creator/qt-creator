// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QDir>
#include <QTemporaryDir>
#include <QtTest>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/temporarydirectory.h>

#include "../headerdependencyscanner.h"

using namespace CMakeProjectManager::Internal;
using namespace QtTaskTree;
using namespace Utils;

class HeaderDependencyScannerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QVERIFY(m_temp.isValid());
        TemporaryDirectory::setMasterTemporaryDirectory(QDir::tempPath()
                                                        + "/qtc-headerdeps-XXXXXX");
    }

    void dialectFollowsToolchain()
    {
        using namespace ProjectExplorer::Constants;

        QCOMPARE(dependencyDialect(MSVC_TOOLCHAIN_TYPEID), DependencyDialect::Msvc);
        QCOMPARE(dependencyDialect(CLANG_CL_TOOLCHAIN_TYPEID), DependencyDialect::Msvc);
        QCOMPARE(dependencyDialect(GCC_TOOLCHAIN_TYPEID), DependencyDialect::Gcc);
        QCOMPARE(dependencyDialect(CLANG_TOOLCHAIN_TYPEID), DependencyDialect::Gcc);
        QCOMPARE(dependencyDialect(MINGW_TOOLCHAIN_TYPEID), DependencyDialect::Gcc);
    }

    void commandHashDoesNotDependOnTheProcess()
    {
        const FilePath compiler = FilePath::fromString("/usr/bin/clang++");
        QCOMPARE(commandHash(compiler, {"-DFOO", "-I/tmp"}), Q_UINT64_C(0xc6b9ea16e3f5ef6d));
    }

    void commandHashSeparatesArguments()
    {
        const FilePath compiler = FilePath::fromString("/usr/bin/clang++");

        QVERIFY(commandHash(compiler, {"-DA", "-DB"}) != commandHash(compiler, {"-DA-DB"}));
        QVERIFY(commandHash(compiler, {"-DA"})
                != commandHash(FilePath::fromString("/usr/bin/g++"), {"-DA"}));
    }

    void laundersGccArguments()
    {
        const QStringList arguments{"-DQT_CORE_LIB",
                                    "-I/inc",
                                    "-c",
                                    "-o",
                                    "main.cpp.o",
                                    "-MD",
                                    "-MF",
                                    "main.cpp.o.d",
                                    "-Xclang",
                                    "-include-pch",
                                    "-Xclang",
                                    "/build/cmake_pch.hxx.gch",
                                    "-std=gnu++17"};

        QCOMPARE(launderScanArguments(arguments, DependencyDialect::Gcc),
                 QStringList({"-DQT_CORE_LIB", "-I/inc", "-std=gnu++17"}));
    }

    void laundersMsvcArguments()
    {
        const QStringList arguments{"-DQT_CORE_LIB",
                                    "/I",
                                    "inc",
                                    "/c",
                                    "/Fomain.cpp.obj",
                                    "/Yucmake_pch.hxx",
                                    "/Fpcmake_pch.cxx.pch",
                                    "/showIncludes",
                                    "/std:c++17"};

        QCOMPARE(launderScanArguments(arguments, DependencyDialect::Msvc),
                 QStringList({"-DQT_CORE_LIB", "/I", "inc", "/std:c++17"}));
    }

    void scanCommandAsksForDependencies()
    {
        HeaderScanUnit unit;
        unit.compiler = FilePath::fromString("/usr/bin/clang++");
        unit.source = FilePath::fromString("/src/main.cpp");
        unit.arguments = {"-I/inc", "-c"};

        const QString nativeSource = unit.source.nativePath();

        const CommandLine gcc = dependencyScanCommand(unit, DependencyDialect::Gcc);
        QCOMPARE(gcc.executable(), unit.compiler);
        QCOMPARE(gcc.splitArguments(), QStringList({"-I/inc", "-M", "-MG", nativeSource}));

        const CommandLine msvc = dependencyScanCommand(unit, DependencyDialect::Msvc);
        QCOMPARE(msvc.splitArguments(),
                 QStringList({"-I/inc", "/nologo", "/Zs", "/showIncludes", nativeSource}));
    }

    void parsesMakeRule()
    {
        const QString output = "main.cpp.o: /src/main.cpp /src/main.h \\\n"
                               "  /usr/include/stdio.h\n";

        QCOMPARE(parseMakeDependencies(output, FilePath::fromString("/build")),
                 FilePaths({FilePath::fromString("/src/main.cpp"),
                            FilePath::fromString("/src/main.h"),
                            FilePath::fromString("/usr/include/stdio.h")}));
    }

    void resolvesRelativeDependencies()
    {
        const QString output = "main.cpp.o: ../src/main.cpp\n";

        QCOMPARE(parseMakeDependencies(output, FilePath::fromString("/project/build")),
                 FilePaths{FilePath::fromString("/project/src/main.cpp")});
    }

    void parsesEscapedSpaces()
    {
        const QString output = "main.cpp.o: /src/with\\ space.h /src/plain.h\n";

        QCOMPARE(parseMakeDependencies(output, FilePath::fromString("/build")),
                 FilePaths({FilePath::fromString("/src/with space.h"),
                            FilePath::fromString("/src/plain.h")}));
    }

    void parsesEscapedHashAndDollar()
    {
        const QString output = "main.cpp.o: /src/ha\\#sh.h /src/dollar$$.h\n";

        QCOMPARE(parseMakeDependencies(output, FilePath::fromString("/build")),
                 FilePaths({FilePath::fromString("/src/ha#sh.h"),
                            FilePath::fromString("/src/dollar$.h")}));
    }

    void skipsTargetsAndDeduplicates()
    {
        const QString output = "a.o b.o: /src/shared.h /src/shared.h /src/other.h\n";

        QCOMPARE(parseMakeDependencies(output, FilePath::fromString("/build")),
                 FilePaths({FilePath::fromString("/src/shared.h"),
                            FilePath::fromString("/src/other.h")}));
    }

    void treatsEachRuleSeparately()
    {
        const QString output = "a.o: /src/a.h\nb.o: /src/b.h\n";

        QCOMPARE(parseMakeDependencies(output, FilePath::fromString("/build")),
                 FilePaths({FilePath::fromString("/src/a.h"),
                            FilePath::fromString("/src/b.h")}));
    }

    void ignoresOutputWithoutARule()
    {
        QVERIFY(parseMakeDependencies("", FilePath::fromString("/build")).isEmpty());
        QVERIFY(parseMakeDependencies("/src/a.h /src/b.h\n", FilePath::fromString("/build"))
                    .isEmpty());
    }

    void parsesShowIncludesOutput()
    {
        const QString prefix = "Note: including file:";
        const QString output = "main.cpp\n"
                               "Note: including file: C:/src/main.h\n"
                               "Note: including file:  C:/inc/nested.h\n"
                               "Note: including file:  C:/inc/nested.h\n";

        QCOMPARE(parseShowIncludes(output, prefix, FilePath::fromString("C:/build")),
                 FilePaths({FilePath::fromString("C:/src/main.h"),
                            FilePath::fromString("C:/inc/nested.h")}));
    }

    void parsesLocalizedShowIncludesPrefix()
    {
        const QString prefix = "Hinweis: Einlesen der Datei:";
        const QString output = "Hinweis: Einlesen der Datei:  C:/src/main.h\n";

        QCOMPARE(parseShowIncludes(output, prefix, FilePath::fromString("C:/build")),
                 FilePaths{FilePath::fromString("C:/src/main.h")});
    }

    void findsNothingWithoutAShowIncludesPrefix()
    {
        const QString output = "main.c\n"
                               "Note: including file:  C:/src/main.h\n"
                               "fatal error C1083: Cannot open include file\n";

        QVERIFY(parseShowIncludes(output, {}, FilePath::fromString("C:/build")).isEmpty());
    }

    void pinsTheMessageLanguageWithoutADetectedPrefix()
    {
        HeaderScanSettings settings;
        settings.dialect = DependencyDialect::Msvc;
        settings.environment = Environment(OsTypeWindows);

        const HeaderScanSettings resolved = withShowIncludesPrefix(settings);

        QCOMPARE(resolved.showIncludesPrefix, "Note: including file:");
        QCOMPARE(resolved.environment.value("VSLANG"), "1033");
    }

    void keepsADetectedShowIncludesPrefix()
    {
        HeaderScanSettings settings;
        settings.dialect = DependencyDialect::Msvc;
        settings.environment = Environment(OsTypeWindows);
        settings.showIncludesPrefix = "Hinweis: Einlesen der Datei:";

        const HeaderScanSettings resolved = withShowIncludesPrefix(settings);

        QCOMPARE(resolved.showIncludesPrefix, settings.showIncludesPrefix);
        QVERIFY(!resolved.environment.hasKey("VSLANG"));
    }

    void leavesGccScansAlone()
    {
        HeaderScanSettings settings;
        settings.dialect = DependencyDialect::Gcc;
        settings.environment = Environment(OsTypeWindows);

        const HeaderScanSettings resolved = withShowIncludesPrefix(settings);

        QVERIFY(resolved.showIncludesPrefix.isEmpty());
        QVERIFY(!resolved.environment.hasKey("VSLANG"));
    }

    void scansWithARealCompiler()
    {
        const FilePath compiler = findCompiler();
        if (compiler.isEmpty())
            QSKIP("Neither clang++ nor g++ was found in PATH.");

        const FilePath directory = FilePath::fromString(m_temp.path());
        const FilePath header = directory / "generated.h";
        const FilePath source = directory / "unit.cpp";
        QVERIFY(header.writeFileContents("#pragma once\nint answer();\n"));
        QVERIFY(source.writeFileContents("#include \"generated.h\"\nint answer() "
                                         "{ return 42; }\n"));

        HeaderScanUnit unit;
        unit.compiler = compiler;
        unit.source = source;
        unit.arguments = {"-c", "-o", "unit.o"};

        HeaderScanSettings settings;
        settings.dialect = DependencyDialect::Gcc;
        settings.workingDirectory = directory;
        settings.environment = Environment::systemEnvironment();

        SourceScans scans;
        FilePaths dependencies;
        const auto handler = [&](const SourceScan &scan, const FilePaths &found) {
            scans.append(scan);
            dependencies = found;
        };

        const Group recipe{headerDependencyScanRecipe({unit}, settings, handler)};
        QCOMPARE(QTaskTree::runBlocking(recipe), DoneWith::Success);

        QCOMPARE(scans, SourceScans{unit.toScan()});
        QVERIFY2(dependencies.contains(header), qPrintable(pathList(dependencies)));
        QVERIFY(dependencies.contains(source));
    }

    void reportsHeadersThatAreNotGeneratedYet()
    {
        const FilePath compiler = findCompiler();
        if (compiler.isEmpty())
            QSKIP("Neither clang++ nor g++ was found in PATH.");

        const FilePath directory = FilePath::fromString(m_temp.path());
        const FilePath source = directory / "usesgenerated.cpp";
        QVERIFY(source.writeFileContents("#include \"ui_mainwindow.h\"\n"));

        HeaderScanUnit unit;
        unit.compiler = compiler;
        unit.source = source;

        FilePaths dependencies;
        QCOMPARE(runScan({unit}, directory, &dependencies), DoneWith::Success);

        QVERIFY2(Utils::contains(dependencies,
                                 [](const FilePath &path) {
                                     return path.fileName() == "ui_mainwindow.h";
                                 }),
                 qPrintable(pathList(dependencies)));
    }

    void survivesASourceThatCannotBePreprocessed()
    {
        const FilePath compiler = findCompiler();
        if (compiler.isEmpty())
            QSKIP("Neither clang++ nor g++ was found in PATH.");

        const FilePath directory = FilePath::fromString(m_temp.path());
        const FilePath good = directory / "good.cpp";
        const FilePath bad = directory / "bad.cpp";
        QVERIFY(good.writeFileContents("#include \"generated.h\"\n"));
        QVERIFY(bad.writeFileContents("#error deliberately broken\n"));

        const auto makeUnit = [&compiler](const FilePath &source) {
            HeaderScanUnit unit;
            unit.compiler = compiler;
            unit.source = source;
            return unit;
        };

        HeaderScanSettings settings;
        settings.dialect = DependencyDialect::Gcc;
        settings.workingDirectory = directory;
        settings.environment = Environment::systemEnvironment();

        SourceScans scans;
        const auto handler = [&scans](const SourceScan &scan, const FilePaths &) {
            scans.append(scan);
        };

        const Group recipe{
            headerDependencyScanRecipe({makeUnit(bad), makeUnit(good)}, settings, handler)};
        QCOMPARE(QTaskTree::runBlocking(recipe), DoneWith::Success);

        QVERIFY(Utils::contains(scans, [&good](const SourceScan &scan) {
            return scan.source == good;
        }));
        QVERIFY(!Utils::contains(scans, [&bad](const SourceScan &scan) {
            return scan.source == bad;
        }));
    }

private:
    static FilePath findCompiler()
    {
        const Environment environment = Environment::systemEnvironment();
        const FilePath clang = environment.searchInPath("clang++");
        return clang.isEmpty() ? environment.searchInPath("g++") : clang;
    }

    static QString pathList(const FilePaths &paths)
    {
        return "Dependencies were: "
               + Utils::transform(paths, &FilePath::toUserOutput).join(", ");
    }

    static DoneWith runScan(const HeaderScanUnits &units,
                            const FilePath &directory,
                            FilePaths *dependencies)
    {
        HeaderScanSettings settings;
        settings.dialect = DependencyDialect::Gcc;
        settings.workingDirectory = directory;
        settings.environment = Environment::systemEnvironment();

        const auto handler = [dependencies](const SourceScan &, const FilePaths &found) {
            *dependencies = found;
        };

        const Group recipe{headerDependencyScanRecipe(units, settings, handler)};
        return QTaskTree::runBlocking(recipe);
    }

    QTemporaryDir m_temp;
};

QTEST_GUILESS_MAIN(HeaderDependencyScannerTest)
#include "tst_headerdependencyscanner.moc"
