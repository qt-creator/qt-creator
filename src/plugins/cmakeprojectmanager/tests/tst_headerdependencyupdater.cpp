// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTemporaryDir>
#include <QtTest>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/rawprojectpart.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>

#include "../headerdependencyupdater.h"

using namespace CMakeProjectManager::Internal;
using namespace ProjectExplorer;
using namespace Utils;

class HeaderDependencyUpdaterTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() { QVERIFY(m_temp.isValid()); }

    void argumentsCarryMacrosAndIncludePaths()
    {
        RawProjectPartFlags flags;
        flags.commandLineFlags = {"-std=gnu++17"};

        const Macros macros{Macro("QT_CORE_LIB", "1"),
                            Macro("VERSION", "\"2.0\""),
                            Macro("NDEBUG", MacroType::Undefine)};

        const FilePath user = FilePath::fromString("/project/inc");
        const FilePath system = FilePath::fromString("/qt/include");
        const FilePath framework = FilePath::fromString("/frameworks");

        const HeaderPaths headerPaths{HeaderPath::makeUser(user),
                                      HeaderPath::makeSystem(system),
                                      HeaderPath::makeBuiltIn(FilePath::fromString("/usr/include")),
                                      HeaderPath::makeFramework(framework)};

        QCOMPARE(scanArguments(flags, macros, headerPaths, DependencyDialect::Gcc),
                 QStringList({"-std=gnu++17",
                              "-DQT_CORE_LIB",
                              "-DVERSION=\"2.0\"",
                              "-UNDEBUG",
                              "-I" + user.nativePath(),
                              "-isystem",
                              system.nativePath(),
                              "-F" + framework.nativePath()}));
    }

    void argumentsLeaveOutBuiltInHeaderPaths()
    {
        const HeaderPaths headerPaths{
            HeaderPath::makeBuiltIn(FilePath::fromString("/toolchain/include"))};

        QVERIFY(scanArguments({}, {}, headerPaths, DependencyDialect::Gcc).isEmpty());
        QVERIFY(scanArguments({}, {}, headerPaths, DependencyDialect::Msvc).isEmpty());
    }

    void argumentsUseSlashesForMsvc()
    {
        const Macros macros{Macro("QT_CORE_LIB", "1")};
        const HeaderPaths headerPaths{HeaderPath::makeUser(FilePath::fromString("C:/project/inc"))};

        const QStringList arguments = scanArguments({}, macros, headerPaths,
                                                    DependencyDialect::Msvc);
        QCOMPARE(arguments.size(), 2);
        QCOMPARE(arguments.at(0), "/DQT_CORE_LIB");
        QVERIFY(arguments.at(1).startsWith("/I"));
    }

    void planCoversSourcesButNotHeaders()
    {
        RawProjectPart part;
        part.setFiles({source("main.cpp"), source("main.h"), source("widget.cpp")});

        const HeaderScanUnits units = planHeaderScan({part}, {}, cxxToolchain());

        QCOMPARE(Utils::transform(units, &HeaderScanUnit::source),
                 FilePaths({source("main.cpp"), source("widget.cpp")}));
    }

    void planUsesTheCCompilerForCSources()
    {
        RawProjectPart part;
        part.setFiles({source("a.c"), source("b.cpp")});
        part.setFlagsForC(cFlags());
        part.setFlagsForCxx(cxxFlags());

        const HeaderScanUnits units = planHeaderScan({part}, cToolchain(), cxxToolchain());

        QCOMPARE(units.size(), 2);
        const HeaderScanUnit cUnit = unitFor(units, source("a.c"));
        const HeaderScanUnit cxxUnit = unitFor(units, source("b.cpp"));
        QCOMPARE(cUnit.compiler, FilePath::fromString("/usr/bin/cc"));
        QCOMPARE(cxxUnit.compiler, FilePath::fromString("/usr/bin/c++"));
        QVERIFY(cUnit.arguments.contains("-std=c11"));
        QVERIFY(cxxUnit.arguments.contains("-std=gnu++17"));
    }

    void planSkipsLanguagesWithoutAToolchain()
    {
        RawProjectPart part;
        part.setFiles({source("a.c"), source("b.cpp")});

        const HeaderScanUnits units = planHeaderScan({part}, {}, cxxToolchain());

        QCOMPARE(Utils::transform(units, &HeaderScanUnit::source), FilePaths{source("b.cpp")});
    }

    void planScansASharedSourceOnce()
    {
        RawProjectPart library;
        library.setDisplayName("library");
        library.setFiles({source("shared.cpp")});

        RawProjectPart test;
        test.setDisplayName("test");
        test.setFiles({source("shared.cpp"), source("test.cpp")});

        const HeaderScanUnits units = planHeaderScan({library, test}, {}, cxxToolchain());

        QCOMPARE(Utils::transform(units, &HeaderScanUnit::source),
                 FilePaths({source("shared.cpp"), source("test.cpp")}));
    }

    void discoveredHeadersReachEveryPartThatCompilesTheSource()
    {
        RawProjectPart library;
        library.setFiles({source("shared.cpp")});
        RawProjectPart test;
        test.setFiles({source("shared.cpp")});

        HeaderDependencyStore store;
        store.insert({source("shared.cpp"), 0}, {source("shared.h")}, 1);

        const RawProjectParts enriched
            = withDiscoveredHeaders({library, test}, store, sourceDir(), buildDir());

        QCOMPARE(enriched.size(), 2);
        for (const RawProjectPart &part : enriched) {
            QCOMPARE(Utils::toSet(part.files),
                     Utils::toSet(FilePaths{source("shared.cpp"), source("shared.h")}));
        }
    }

    void discoveredHeadersExcludeSystemHeaders()
    {
        RawProjectPart part;
        part.setFiles({source("main.cpp")});

        HeaderDependencyStore store;
        store.insert({source("main.cpp"), 0},
                     {source("main.cpp"),
                      source("main.h"),
                      buildDir() / "ui_mainwindow.h",
                      FilePath::fromString("/usr/include/stdio.h"),
                      FilePath::fromString("/opt/qt/include/QtCore/qobject.h")},
                     1);

        const RawProjectParts enriched = withDiscoveredHeaders({part}, store, sourceDir(),
                                                               buildDir());

        QCOMPARE(Utils::toSet(enriched.first().files),
                 Utils::toSet(FilePaths{source("main.cpp"),
                                        source("main.h"),
                                        buildDir() / "ui_mainwindow.h"}));
    }

    void discoveredHeadersKeepPartsWithoutResultsUntouched()
    {
        RawProjectPart part;
        part.setFiles({source("main.cpp")});

        const HeaderDependencyStore store;
        const RawProjectParts enriched = withDiscoveredHeaders({part}, store, sourceDir(),
                                                               buildDir());

        QCOMPARE(enriched.first().files, FilePaths{source("main.cpp")});
    }

    void headerMapReadsBackWhatWasStored()
    {
        const FilePath storeFile = FilePath::fromString(m_temp.path()) / "map/header-deps";

        HeaderDependencyStore store;
        store.insert({source("main.cpp"), 0},
                     {source("main.h"), FilePath::fromString("/usr/include/stdio.h")},
                     1);
        store.insert({source("lonely.cpp"), 0},
                     {FilePath::fromString("/usr/include/stdio.h")},
                     1);
        QVERIFY(store.save(storeFile));

        HeaderDependencyUpdater updater;
        updater.setStoreFile(storeFile);
        updater.setProjectDirectories(sourceDir(), buildDir());

        const QHash<FilePath, FilePaths> map = updater.projectHeaderMap();

        QCOMPARE(map.keys(), FilePaths{source("main.cpp")});
        QCOMPARE(map.value(source("main.cpp")), FilePaths{source("main.h")});
    }

    void headerMapIsEmptyWithoutAStore()
    {
        HeaderDependencyUpdater updater;
        updater.setStoreFile(FilePath::fromString(m_temp.path()) / "absent");
        updater.setProjectDirectories(sourceDir(), buildDir());

        QVERIFY(updater.projectHeaderMap().isEmpty());
    }

private:
    FilePath sourceDir() const { return FilePath::fromString(m_temp.path()) / "project"; }
    FilePath buildDir() const { return FilePath::fromString(m_temp.path()) / "build"; }
    FilePath source(const QString &name) const { return sourceDir() / name; }

    static RawProjectPartFlags cFlags()
    {
        RawProjectPartFlags flags;
        flags.commandLineFlags = {"-std=c11"};
        return flags;
    }

    static RawProjectPartFlags cxxFlags()
    {
        RawProjectPartFlags flags;
        flags.commandLineFlags = {"-std=gnu++17"};
        return flags;
    }

    static ToolchainInfo cToolchain()
    {
        ToolchainInfo info;
        info.type = Constants::GCC_TOOLCHAIN_TYPEID;
        info.compilerFilePath = FilePath::fromString("/usr/bin/cc");
        return info;
    }

    static ToolchainInfo cxxToolchain()
    {
        ToolchainInfo info;
        info.type = Constants::GCC_TOOLCHAIN_TYPEID;
        info.compilerFilePath = FilePath::fromString("/usr/bin/c++");
        return info;
    }

    static HeaderScanUnit unitFor(const HeaderScanUnits &units, const FilePath &source)
    {
        return Utils::findOrDefault(units, [&source](const HeaderScanUnit &unit) {
            return unit.source == source;
        });
    }

    QTemporaryDir m_temp;
};

QTEST_GUILESS_MAIN(HeaderDependencyUpdaterTest)
#include "tst_headerdependencyupdater.moc"
