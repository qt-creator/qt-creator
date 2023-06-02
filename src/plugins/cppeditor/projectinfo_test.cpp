// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectinfo_test.h"

#include "cppprojectfilecategorizer.h"
#include "cppprojectinfogenerator.h"
#include "cppprojectpartchooser.h"
#include "headerpathfilter.h"
#include "projectinfo.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchainconfigwidget.h>
#include <utils/algorithm.h>

#include <QtTest>

using namespace ProjectExplorer;

namespace CppEditor::Internal {

namespace {
class ProjectPartChooserTestHelper
{
public:
    ProjectPartChooserTestHelper()
    {
        chooser.setFallbackProjectPart([&]() {
            return fallbackProjectPart;
        });
        chooser.setProjectPartsForFile([&](const QString &) {
            return projectPartsForFile;
        });
        chooser.setProjectPartsFromDependenciesForFile([&](const QString &) {
            return projectPartsFromDependenciesForFile;
        });
    }

    const ProjectPartInfo choose()
    {
        const Project * const project = projectMap.value(activeProject).get();
        const Utils::FilePath projectFilePath = project ? project->projectFilePath()
                                                        : Utils::FilePath();
        return chooser.choose(filePath, currentProjectPartInfo, preferredProjectPartId,
                              projectFilePath,
                              languagePreference, projectsChanged);
    }

    static QList<ProjectPart::ConstPtr> createProjectPartsWithDifferentProjects()
    {
        QList<ProjectPart::ConstPtr> projectParts;

        const auto p1 = std::make_shared<Project>(
                    QString(), Utils::FilePath::fromString("p1.pro"));
        projectMap.insert(p1->projectFilePath(), p1);
        projectParts.append(ProjectPart::create(p1->projectFilePath()));
        const auto p2 = std::make_shared<Project>(
                    QString(), Utils::FilePath::fromString("p2.pro"));
        projectMap.insert(p2->projectFilePath(), p2);
        projectParts.append(ProjectPart::create(p2->projectFilePath()));

        return projectParts;
    }

    static QList<ProjectPart::ConstPtr> createCAndCxxProjectParts()
    {
        QList<ProjectPart::ConstPtr> projectParts;
        ToolChainInfo tcInfo;

        // Create project part for C
        tcInfo.macroInspectionRunner = [](const QStringList &) {
            return ToolChain::MacroInspectionReport{{}, Utils::LanguageVersion::C11};
        };
        const ProjectPart::ConstPtr cprojectpart = ProjectPart::create({}, {}, {}, {}, {}, {}, {},
                                                                  tcInfo);
        projectParts.append(cprojectpart);

        // Create project part for CXX
        tcInfo.macroInspectionRunner = [](const QStringList &) {
            return ToolChain::MacroInspectionReport{{}, Utils::LanguageVersion::CXX98};
        };
        const ProjectPart::ConstPtr cxxprojectpart = ProjectPart::create({}, {}, {}, {}, {}, {}, {},
                                                                    tcInfo);
        projectParts.append(cxxprojectpart);

        return projectParts;
    }

    QString filePath;
    ProjectPart::ConstPtr currentProjectPart = ProjectPart::create({});
    ProjectPartInfo currentProjectPartInfo{currentProjectPart,
                                           {currentProjectPart},
                                           ProjectPartInfo::NoHint};
    QString preferredProjectPartId;
    Utils::FilePath activeProject;
    Utils::Language languagePreference = Utils::Language::Cxx;
    bool projectsChanged = false;
    ProjectPartChooser chooser;

    QList<ProjectPart::ConstPtr> projectPartsForFile;
    QList<ProjectPart::ConstPtr> projectPartsFromDependenciesForFile;
    ProjectPart::ConstPtr fallbackProjectPart;

    static QHash<Utils::FilePath, std::shared_ptr<Project>> projectMap;
};

QHash<Utils::FilePath, std::shared_ptr<Project>>
ProjectPartChooserTestHelper::projectMap;
}

void ProjectPartChooserTest::testChooseManuallySet()
{
    ProjectPart::ConstPtr p1 = ProjectPart::create({});
    RawProjectPart rpp2;
    rpp2.setProjectFileLocation("someId");
    ProjectPart::ConstPtr p2 = ProjectPart::create({}, rpp2);
    ProjectPartChooserTestHelper t;
    t.preferredProjectPartId = p2->projectFile;
    t.projectPartsForFile += {p1, p2};

    QCOMPARE(t.choose().projectPart, p2);
}

void ProjectPartChooserTest::testIndicateManuallySet()
{
    ProjectPart::ConstPtr p1 = ProjectPart::create({});
    RawProjectPart rpp2;
    rpp2.setProjectFileLocation("someId");
    ProjectPart::ConstPtr p2 = ProjectPart::create({}, rpp2);
    ProjectPartChooserTestHelper t;
    t.preferredProjectPartId = p2->projectFile;
    t.projectPartsForFile += {p1, p2};

    QVERIFY(t.choose().hints & ProjectPartInfo::IsPreferredMatch);
}

void ProjectPartChooserTest::testIndicateManuallySetForFallbackToProjectPartFromDependencies()
{
    ProjectPart::ConstPtr p1 = ProjectPart::create({});
    RawProjectPart rpp2;
    rpp2.setProjectFileLocation("someId");
    ProjectPart::ConstPtr p2 = ProjectPart::create({}, rpp2);
    ProjectPartChooserTestHelper t;
    t.preferredProjectPartId = p2->projectFile;
    t.projectPartsFromDependenciesForFile += {p1, p2};

    QVERIFY(t.choose().hints & ProjectPartInfo::IsPreferredMatch);
}

void ProjectPartChooserTest::testDoNotIndicateNotManuallySet()
{
    QVERIFY(!(ProjectPartChooserTestHelper().choose().hints & ProjectPartInfo::IsPreferredMatch));
}

void ProjectPartChooserTest::testForMultipleChooseFromActiveProject()
{
    ProjectPartChooserTestHelper t;
    const QList<ProjectPart::ConstPtr> projectParts = t.createProjectPartsWithDifferentProjects();
    const ProjectPart::ConstPtr secondProjectPart = projectParts.at(1);
    t.projectPartsForFile += projectParts;
    t.activeProject = secondProjectPart->topLevelProject;

    QCOMPARE(t.choose().projectPart, secondProjectPart);
}

void ProjectPartChooserTest::testForMultiplePreferSelectedForBuilding()
{
    RawProjectPart rpp1;
    rpp1.setSelectedForBuilding(false);
    RawProjectPart rpp2;
    rpp2.setSelectedForBuilding(true);
    const ProjectPart::ConstPtr firstProjectPart = ProjectPart::create({}, rpp1);
    const ProjectPart::ConstPtr secondProjectPart = ProjectPart::create({}, rpp2);
    ProjectPartChooserTestHelper t;
    t.projectPartsForFile += firstProjectPart;
    t.projectPartsForFile += secondProjectPart;

    QCOMPARE(t.choose().projectPart, secondProjectPart);
}

void ProjectPartChooserTest::testForMultipleFromDependenciesChooseFromActiveProject()
{
    ProjectPartChooserTestHelper t;
    const QList<ProjectPart::ConstPtr> projectParts = t.createProjectPartsWithDifferentProjects();
    const ProjectPart::ConstPtr secondProjectPart = projectParts.at(1);
    t.projectPartsFromDependenciesForFile += projectParts;
    t.activeProject = secondProjectPart->topLevelProject;

    QCOMPARE(t.choose().projectPart, secondProjectPart);
}

void ProjectPartChooserTest::testForMultipleCheckIfActiveProjectChanged()
{
    ProjectPartChooserTestHelper t;
    const QList<ProjectPart::ConstPtr> projectParts = t.createProjectPartsWithDifferentProjects();
    const ProjectPart::ConstPtr firstProjectPart = projectParts.at(0);
    const ProjectPart::ConstPtr secondProjectPart = projectParts.at(1);
    t.projectPartsForFile += projectParts;
    t.currentProjectPartInfo.projectPart = firstProjectPart;
    t.activeProject = secondProjectPart->topLevelProject;

    QCOMPARE(t.choose().projectPart, secondProjectPart);
}

void ProjectPartChooserTest::testForMultipleAndAmbigiousHeaderPreferCProjectPart()
{
    ProjectPartChooserTestHelper t;
    t.languagePreference = Utils::Language::C;
    t.projectPartsForFile = t.createCAndCxxProjectParts();
    const ProjectPart::ConstPtr cProjectPart = t.projectPartsForFile.at(0);

    QCOMPARE(t.choose().projectPart, cProjectPart);
}

void ProjectPartChooserTest::testForMultipleAndAmbigiousHeaderPreferCxxProjectPart()
{
    ProjectPartChooserTestHelper t;
    t.languagePreference = Utils::Language::Cxx;
    t.projectPartsForFile = t.createCAndCxxProjectParts();
    const ProjectPart::ConstPtr cxxProjectPart = t.projectPartsForFile.at(1);

    QCOMPARE(t.choose().projectPart, cxxProjectPart);
}

void ProjectPartChooserTest::testIndicateMultiple()
{
    const ProjectPart::ConstPtr p1 = ProjectPart::create({});
    const ProjectPart::ConstPtr p2 = ProjectPart::create({});
    ProjectPartChooserTestHelper t;
    t.projectPartsForFile += {p1, p2};

    QVERIFY(t.choose().hints & ProjectPartInfo::IsAmbiguousMatch);
}

void ProjectPartChooserTest::testIndicateMultipleForFallbackToProjectPartFromDependencies()
{
    const ProjectPart::ConstPtr p1 = ProjectPart::create({});
    const ProjectPart::ConstPtr p2 = ProjectPart::create({});
    ProjectPartChooserTestHelper t;
    t.projectPartsFromDependenciesForFile += {p1, p2};

    QVERIFY(t.choose().hints & ProjectPartInfo::IsAmbiguousMatch);
}

void ProjectPartChooserTest::testForMultipleChooseNewIfPreviousIsGone()
{
    const ProjectPart::ConstPtr newProjectPart = ProjectPart::create({});
    ProjectPartChooserTestHelper t;
    t.projectPartsForFile += newProjectPart;

    QCOMPARE(t.choose().projectPart, newProjectPart);
}

void ProjectPartChooserTest::testFallbackToProjectPartFromDependencies()
{
    const ProjectPart::ConstPtr fromDependencies = ProjectPart::create({});
    ProjectPartChooserTestHelper t;
    t.projectPartsFromDependenciesForFile += fromDependencies;

    QCOMPARE(t.choose().projectPart, fromDependencies);
}

void ProjectPartChooserTest::testFallbackToProjectPartFromModelManager()
{
    ProjectPartChooserTestHelper t;
    t.fallbackProjectPart = ProjectPart::create({});

    QCOMPARE(t.choose().projectPart, t.fallbackProjectPart);
}

void ProjectPartChooserTest::testContinueUsingFallbackFromModelManagerIfProjectDoesNotChange()
{
    // ...without re-calculating the dependency table.
    ProjectPartChooserTestHelper t;
    t.fallbackProjectPart = ProjectPart::create({});
    t.currentProjectPartInfo.projectPart = t.fallbackProjectPart;
    t.currentProjectPartInfo.hints |= ProjectPartInfo::IsFallbackMatch;
    t.projectPartsFromDependenciesForFile += ProjectPart::create({});

    QCOMPARE(t.choose().projectPart, t.fallbackProjectPart);
}

void ProjectPartChooserTest::testStopUsingFallbackFromModelManagerIfProjectChanges1()
{
    ProjectPartChooserTestHelper t;
    t.fallbackProjectPart = ProjectPart::create({});
    t.currentProjectPartInfo.projectPart = t.fallbackProjectPart;
    t.currentProjectPartInfo.hints |= ProjectPartInfo::IsFallbackMatch;
    const ProjectPart::ConstPtr addedProject = ProjectPart::create({});
    t.projectPartsForFile += addedProject;

    QCOMPARE(t.choose().projectPart, addedProject);
}

void ProjectPartChooserTest::testStopUsingFallbackFromModelManagerIfProjectChanges2()
{
    ProjectPartChooserTestHelper t;
    t.fallbackProjectPart = ProjectPart::create({});
    t.currentProjectPartInfo.projectPart = t.fallbackProjectPart;
    t.currentProjectPartInfo.hints |= ProjectPartInfo::IsFallbackMatch;
    const ProjectPart::ConstPtr addedProject = ProjectPart::create({});
    t.projectPartsFromDependenciesForFile += addedProject;
    t.projectsChanged = true;

    QCOMPARE(t.choose().projectPart, addedProject);
}

void ProjectPartChooserTest::testIndicateFallbacktoProjectPartFromModelManager()
{
    ProjectPartChooserTestHelper t;
    t.fallbackProjectPart = ProjectPart::create({});

    QVERIFY(t.choose().hints & ProjectPartInfo::IsFallbackMatch);
}

void ProjectPartChooserTest::testIndicateFromDependencies()
{
    ProjectPartChooserTestHelper t;
    t.projectPartsFromDependenciesForFile += ProjectPart::create({});

    QVERIFY(t.choose().hints & ProjectPartInfo::IsFromDependenciesMatch);
}

void ProjectPartChooserTest::testDoNotIndicateFromDependencies()
{
    ProjectPartChooserTestHelper t;
    t.projectPartsForFile += ProjectPart::create({});

    QVERIFY(!(t.choose().hints & ProjectPartInfo::IsFromDependenciesMatch));
}

namespace {
class TestToolchain : public ToolChain
{
public:
    TestToolchain() : ToolChain("dummy") {}

private:
    MacroInspectionRunner createMacroInspectionRunner() const override { return {}; }
    Utils::LanguageExtensions languageExtensions(const QStringList &) const override { return {}; }
    Utils::WarningFlags warningFlags(const QStringList &) const override { return {}; }
    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner(
            const Utils::Environment &) const override { return {}; }
    void addToEnvironment(Utils::Environment &) const override {}
    Utils::FilePath makeCommand(const Utils::Environment &) const override { return {}; }
    QList<Utils::OutputLineParser *> createOutputParsers() const override { return {}; }
    std::unique_ptr<ToolChainConfigWidget> createConfigurationWidget() override
    {
        return {};
    };
};

class ProjectInfoGeneratorTestHelper
{
public:
    ProjectInfoGeneratorTestHelper()
    {
        TestToolchain aToolChain;
        projectUpdateInfo.cxxToolChainInfo = {&aToolChain, {}, {}};
        projectUpdateInfo.cToolChainInfo = {&aToolChain, {}, {}};
    }

    ProjectInfo::ConstPtr generate()
    {
        QPromise<ProjectInfo::ConstPtr> promise;

        projectUpdateInfo.rawProjectParts += rawProjectPart;
        ProjectInfoGenerator generator(projectUpdateInfo);

        return generator.generate(promise);
    }

    ProjectUpdateInfo projectUpdateInfo;
    RawProjectPart rawProjectPart;
};
}

void ProjectInfoGeneratorTest::testCreateNoProjectPartsForEmptyFileList()
{
    ProjectInfoGeneratorTestHelper t;
    const ProjectInfo::ConstPtr projectInfo = t.generate();

    QVERIFY(projectInfo->projectParts().isEmpty());
}

void ProjectInfoGeneratorTest::testCreateSingleProjectPart()
{
    ProjectInfoGeneratorTestHelper t;
    t.rawProjectPart.files = QStringList{ "foo.cpp", "foo.h"};
    const ProjectInfo::ConstPtr projectInfo = t.generate();

    QCOMPARE(projectInfo->projectParts().size(), 1);
}

void ProjectInfoGeneratorTest::testCreateMultipleProjectParts()
{
    ProjectInfoGeneratorTestHelper t;
    t.rawProjectPart.files = QStringList{ "foo.cpp", "foo.h", "bar.c", "bar.h" };
    const ProjectInfo::ConstPtr projectInfo = t.generate();

    QCOMPARE(projectInfo->projectParts().size(), 2);
}

void ProjectInfoGeneratorTest::testProjectPartIndicatesObjectiveCExtensionsByDefault()
{
    ProjectInfoGeneratorTestHelper t;
    t.rawProjectPart.files = QStringList{ "foo.mm" };
    const ProjectInfo::ConstPtr projectInfo = t.generate();
    QCOMPARE(projectInfo->projectParts().size(), 1);

    const ProjectPart &projectPart = *projectInfo->projectParts().at(0);
    QVERIFY(projectPart.languageExtensions & Utils::LanguageExtension::ObjectiveC);
}

void ProjectInfoGeneratorTest::testProjectPartHasLatestLanguageVersionByDefault()
{
    ProjectInfoGeneratorTestHelper t;
    t.rawProjectPart.files = QStringList{ "foo.cpp" };
    const ProjectInfo::ConstPtr projectInfo = t.generate();
    QCOMPARE(projectInfo->projectParts().size(), 1);

    const ProjectPart &projectPart = *projectInfo->projectParts().at(0);
    QCOMPARE(projectPart.languageVersion, Utils::LanguageVersion::LatestCxx);
}

void ProjectInfoGeneratorTest::testUseMacroInspectionReportForLanguageVersion()
{
    ProjectInfoGeneratorTestHelper t;
    t.projectUpdateInfo.cxxToolChainInfo.macroInspectionRunner = [](const QStringList &) {
        return TestToolchain::MacroInspectionReport{Macros(), Utils::LanguageVersion::CXX17};
    };
    t.rawProjectPart.files = QStringList{ "foo.cpp" };
    const ProjectInfo::ConstPtr projectInfo = t.generate();

    QCOMPARE(projectInfo->projectParts().size(), 1);

    const ProjectPart &projectPart = *projectInfo->projectParts().at(0);
    QCOMPARE(projectPart.languageVersion, Utils::LanguageVersion::CXX17);
}

void ProjectInfoGeneratorTest::testUseCompilerFlagsForLanguageExtensions()
{
    ProjectInfoGeneratorTestHelper t;
    t.rawProjectPart.files = QStringList{ "foo.cpp" };
    t.rawProjectPart.flagsForCxx.languageExtensions = Utils::LanguageExtension::Microsoft;
    const ProjectInfo::ConstPtr projectInfo = t.generate();

    QCOMPARE(projectInfo->projectParts().size(), 1);

    const ProjectPart &projectPart = *projectInfo->projectParts().at(0);
    QVERIFY(projectPart.languageExtensions & Utils::LanguageExtension::Microsoft);
}

void ProjectInfoGeneratorTest::testProjectFileKindsMatchProjectPartVersion()
{
    ProjectInfoGeneratorTestHelper t;
    t.rawProjectPart.files = QStringList{ "foo.h" };
    const ProjectInfo::ConstPtr projectInfo = t.generate();

    QCOMPARE(projectInfo->projectParts().size(), 4);
    QVERIFY(Utils::contains(projectInfo->projectParts(), [](const ProjectPart::ConstPtr &p) {
        return p->languageVersion == Utils::LanguageVersion::LatestC
                && p->files.first().kind == ProjectFile::CHeader;
    }));
    QVERIFY(Utils::contains(projectInfo->projectParts(), [](const ProjectPart::ConstPtr &p) {
        return p->languageVersion == Utils::LanguageVersion::LatestC
                && p->files.first().kind == ProjectFile::ObjCHeader;
    }));
    QVERIFY(Utils::contains(projectInfo->projectParts(), [](const ProjectPart::ConstPtr &p) {
        return p->languageVersion == Utils::LanguageVersion::LatestCxx
                && p->files.first().kind == ProjectFile::CXXHeader;
    }));
    QVERIFY(Utils::contains(projectInfo->projectParts(), [](const ProjectPart::ConstPtr &p) {
        return p->languageVersion == Utils::LanguageVersion::LatestCxx
                && p->files.first().kind == ProjectFile::ObjCXXHeader;
    }));
}

namespace {
class HeaderPathFilterTestHelper
{
public:
    const ProjectPart &finalize()
    {
        RawProjectPart rpp;
        rpp.setHeaderPaths(headerPaths);
        ToolChainInfo tcInfo;
        tcInfo.type = toolchainType;
        tcInfo.targetTriple = targetTriple;
        tcInfo.installDir = toolchainInstallDir;
        projectPart = ProjectPart::create({}, rpp, {}, {}, {}, {}, {}, tcInfo);
        filter.emplace(HeaderPathFilter(*projectPart, UseTweakedHeaderPaths::No, {},
                                        "/project", "/build"));
        return *projectPart;
    }

    static HeaderPath user(const QString &path) { return HeaderPath::makeUser(path); }
    static HeaderPath builtIn(const QString &path) { return HeaderPath::makeBuiltIn(path); }
    static HeaderPath system(const QString &path) { return HeaderPath::makeSystem(path); }
    static HeaderPath framework(const QString &path) { return HeaderPath::makeFramework(path); }

    QString targetTriple;
    Utils::Id toolchainType;
    Utils::FilePath toolchainInstallDir;
    HeaderPaths headerPaths = {
        builtIn(""),
        builtIn("/builtin_path"),
        system("/system_path"),
        framework("/framework_path"),
        user("/outside_project_user_path"),
        user("/build/user_path"),
        user("/buildb/user_path"),
        user("/projectb/user_path"),
        user("/project/user_path")};

    std::optional<HeaderPathFilter> filter;

private:
    ProjectPart::ConstPtr projectPart;
};
}

void HeaderPathFilterTest::testBuiltin()
{
    HeaderPathFilterTestHelper t;
    t.finalize();
    t.filter->process();

    QCOMPARE(t.filter->builtInHeaderPaths, (HeaderPaths{t.builtIn("/builtin_path")}));
}

void HeaderPathFilterTest::testSystem()
{
    HeaderPathFilterTestHelper t;
    t.finalize();
    t.filter->process();

    QCOMPARE(t.filter->systemHeaderPaths, (HeaderPaths{
        t.system("/project/.pre_includes"), t.system("/system_path"),
        t.framework("/framework_path"), t.user("/outside_project_user_path"),
        t.user("/buildb/user_path"), t.user("/projectb/user_path")}));
}

void HeaderPathFilterTest::testUser()
{
    HeaderPathFilterTestHelper t;
    t.finalize();
    t.filter->process();

    QCOMPARE(t.filter->userHeaderPaths, (HeaderPaths{t.user("/build/user_path"),
                                                     t.user("/project/user_path")}));
}

void HeaderPathFilterTest::testNoProjectPathSet()
{
    HeaderPathFilterTestHelper t;
    HeaderPathFilter filter{t.finalize(), UseTweakedHeaderPaths::No};
    filter.process();

    QCOMPARE(filter.userHeaderPaths, (HeaderPaths{
        t.user("/outside_project_user_path"), t.user("/build/user_path"),
        t.user("/buildb/user_path"), t.user("/projectb/user_path"),
        t.user("/project/user_path")}));
}

void HeaderPathFilterTest::testDontAddInvalidPath()
{
    HeaderPathFilterTestHelper t;
    t.finalize();
    t.filter->process();
    QCOMPARE(t.filter->builtInHeaderPaths, (HeaderPaths{t.builtIn("/builtin_path")}));
    QCOMPARE(t.filter->systemHeaderPaths, HeaderPaths({
        t.system("/project/.pre_includes"), t.system("/system_path"),
        t.framework("/framework_path"), t.user("/outside_project_user_path"),
        t.user("/buildb/user_path"), t.user("/projectb/user_path")}));
    QCOMPARE(t.filter->userHeaderPaths, HeaderPaths({t.user("/build/user_path"),
                                                     t.user("/project/user_path")}));
}

void HeaderPathFilterTest::testClangHeadersPath()
{
    HeaderPathFilterTestHelper t;
    HeaderPathFilter filter(t.finalize(), UseTweakedHeaderPaths::Yes, "clang_dir");
    filter.process();

    QCOMPARE(filter.builtInHeaderPaths, (HeaderPaths{t.builtIn("clang_dir"),
                                                     t.builtIn("/builtin_path")}));
}

void HeaderPathFilterTest::testClangHeadersPathWitoutClangVersion()
{
    HeaderPathFilterTestHelper t;
    HeaderPathFilter filter(t.finalize(), UseTweakedHeaderPaths::Yes);
    filter.process();

    QCOMPARE(filter.builtInHeaderPaths, (HeaderPaths{t.builtIn("/builtin_path")}));
}

void HeaderPathFilterTest::testClangHeadersAndCppIncludesPathsOrderMacOs()
{
    HeaderPathFilterTestHelper t;
    t.targetTriple = "x86_64-apple-darwin10";
    const auto builtIns = {
        t.builtIn("/usr/include/c++/4.2.1"), t.builtIn("/usr/include/c++/4.2.1/backward"),
        t.builtIn("/usr/local/include"),
        t.builtIn("/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/6.0/include"),
        t.builtIn("/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include"),
        t.builtIn("/usr/include")
    };
    std::copy(builtIns.begin(), builtIns.end(),
              std::inserter(t.headerPaths, t.headerPaths.begin()));
    HeaderPathFilter filter(t.finalize(), UseTweakedHeaderPaths::Yes, "clang_dir");
    filter.process();

    QCOMPARE(filter.builtInHeaderPaths, (HeaderPaths{
        t.builtIn("/usr/include/c++/4.2.1"), t.builtIn("/usr/include/c++/4.2.1/backward"),
        t.builtIn("/usr/local/include"), t.builtIn("clang_dir"),
        t.builtIn("/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include"),
        t.builtIn("/usr/include"),
        t.builtIn("/builtin_path")}));
}

void HeaderPathFilterTest::testClangHeadersAndCppIncludesPathsOrderLinux()
{
    HeaderPathFilterTestHelper t;
    t.targetTriple = "x86_64-linux-gnu";
    const auto builtIns = {
        t.builtIn("/usr/include/c++/4.8"), t.builtIn("/usr/include/c++/4.8/backward"),
        t.builtIn("/usr/include/x86_64-linux-gnu/c++/4.8"),
        t.builtIn("/usr/local/include"), t.builtIn("/usr/lib/gcc/x86_64-linux-gnu/4.8/include"),
        t.builtIn("/usr/include/x86_64-linux-gnu"), t.builtIn("/usr/include")};
    std::copy(builtIns.begin(), builtIns.end(),
              std::inserter(t.headerPaths, t.headerPaths.begin()));
    HeaderPathFilter filter(t.finalize(), UseTweakedHeaderPaths::Yes, "clang_dir");
    filter.process();

    QCOMPARE(filter.builtInHeaderPaths, (HeaderPaths{
        t.builtIn("/usr/include/c++/4.8"), t.builtIn("/usr/include/c++/4.8/backward"),
        t.builtIn("/usr/include/x86_64-linux-gnu/c++/4.8"), t.builtIn("/usr/local/include"),
        t.builtIn("clang_dir"), t.builtIn("/usr/lib/gcc/x86_64-linux-gnu/4.8/include"),
        t.builtIn("/usr/include/x86_64-linux-gnu"), t.builtIn("/usr/include"),
        t.builtIn("/builtin_path")}));
}

// GCC-internal include paths like <installdir>/include and <installdir/include-next> might confuse
// clang and should be filtered out. clang on the command line filters them out, too.
void HeaderPathFilterTest::testRemoveGccInternalPaths()
{
    HeaderPathFilterTestHelper t;
    t.toolchainInstallDir = Utils::FilePath::fromUtf8("/usr/lib/gcc/x86_64-linux-gnu/7");
    t.toolchainType = Constants::GCC_TOOLCHAIN_TYPEID;
    t.headerPaths = {
        t.builtIn("/usr/lib/gcc/x86_64-linux-gnu/7/include"),
        t.builtIn("/usr/lib/gcc/x86_64-linux-gnu/7/include-fixed"),
    };
    HeaderPathFilter filter(t.finalize(), UseTweakedHeaderPaths::Yes, "clang_dir");
    filter.process();

    QCOMPARE(filter.builtInHeaderPaths, (HeaderPaths{t.builtIn("clang_dir")}));
}

// Some distributions ship the standard library headers in "<installdir>/include/c++" (MinGW)
// or e.g. "<installdir>/include/g++-v8" (Gentoo).
// Ensure that we do not remove include paths pointing there.
void HeaderPathFilterTest::testRemoveGccInternalPathsExceptForStandardPaths()
{
    HeaderPathFilterTestHelper t;
    t.toolchainInstallDir = Utils::FilePath::fromUtf8("c:/mingw/lib/gcc/x86_64-w64-mingw32/7.3.0");
    t.toolchainType = Constants::MINGW_TOOLCHAIN_TYPEID;
    t.headerPaths = {
        t.builtIn("c:/mingw/lib/gcc/x86_64-w64-mingw32/7.3.0/include/c++"),
        t.builtIn("c:/mingw/lib/gcc/x86_64-w64-mingw32/7.3.0/include/c++/x86_64-w64-mingw32"),
        t.builtIn("c:/mingw/lib/gcc/x86_64-w64-mingw32/7.3.0/include/c++/backward"),
    };

    HeaderPaths expected = t.headerPaths;
    expected.append(t.builtIn("clang_dir"));
    HeaderPathFilter filter(t.finalize(), UseTweakedHeaderPaths::Yes, "clang_dir");
    filter.process();

    QCOMPARE(filter.builtInHeaderPaths, expected);
}

void HeaderPathFilterTest::testClangHeadersAndCppIncludesPathsOrderNoVersion()
{
    HeaderPathFilterTestHelper t;
    t.headerPaths = {
        t.builtIn("C:/mingw/i686-w64-mingw32/include"),
        t.builtIn("C:/mingw/i686-w64-mingw32/include/c++"),
        t.builtIn("C:/mingw/i686-w64-mingw32/include/c++/i686-w64-mingw32"),
        t.builtIn("C:/mingw/i686-w64-mingw32/include/c++/backward"),
    };
    t.targetTriple = "x86_64-w64-windows-gnu";
    HeaderPathFilter filter(t.finalize(), UseTweakedHeaderPaths::Yes, "clang_dir");
    filter.process();

    QCOMPARE(filter.builtInHeaderPaths, (HeaderPaths{
        t.builtIn("C:/mingw/i686-w64-mingw32/include/c++"),
        t.builtIn("C:/mingw/i686-w64-mingw32/include/c++/i686-w64-mingw32"),
        t.builtIn("C:/mingw/i686-w64-mingw32/include/c++/backward"),
        t.builtIn("clang_dir"),
        t.builtIn("C:/mingw/i686-w64-mingw32/include")}));
}

void HeaderPathFilterTest::testClangHeadersAndCppIncludesPathsOrderAndroidClang()
{
    HeaderPathFilterTestHelper t;
    t.headerPaths = {
        t.builtIn("C:/Android/sdk/ndk-bundle/sysroot/usr/include/i686-linux-android"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sources/cxx-stl/llvm-libc++/include"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sources/android/support/include"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sources/cxx-stl/llvm-libc++abi/include"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sysroot/usr/include")
    };
    t.targetTriple = "i686-linux-android";
    HeaderPathFilter filter(t.finalize(), UseTweakedHeaderPaths::Yes, "clang_dir");
    filter.process();

    QCOMPARE(filter.builtInHeaderPaths, (HeaderPaths{
        t.builtIn("C:/Android/sdk/ndk-bundle/sources/cxx-stl/llvm-libc++/include"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sources/cxx-stl/llvm-libc++abi/include"),
        t.builtIn("clang_dir"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sysroot/usr/include/i686-linux-android"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sources/android/support/include"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sysroot/usr/include")}));
}

void ProjectFileCategorizerTest::testC()
{
    const ProjectFileCategorizer categorizer({}, {"foo.c", "foo.h"});
    const ProjectFiles expected {
        ProjectFile("foo.c", ProjectFile::CSource),
        ProjectFile("foo.h", ProjectFile::CHeader),
    };

    QCOMPARE(categorizer.cSources(), expected);
    QVERIFY(categorizer.cxxSources().isEmpty());
    QVERIFY(categorizer.objcSources().isEmpty());
    QVERIFY(categorizer.objcxxSources().isEmpty());
}

void ProjectFileCategorizerTest::testCxxWithUnambiguousHeaderSuffix()
{
    const ProjectFileCategorizer categorizer({}, {"foo.cpp", "foo.hpp"});
    const ProjectFiles expected {
        ProjectFile("foo.cpp", ProjectFile::CXXSource),
        ProjectFile("foo.hpp", ProjectFile::CXXHeader),
    };

    QCOMPARE(categorizer.cxxSources(), expected);
    QVERIFY(categorizer.cSources().isEmpty());
    QVERIFY(categorizer.objcSources().isEmpty());
    QVERIFY(categorizer.objcxxSources().isEmpty());
}

void ProjectFileCategorizerTest::testCxxWithAmbiguousHeaderSuffix()
{
    const ProjectFiles expected {
        ProjectFile("foo.cpp", ProjectFile::CXXSource),
        ProjectFile("foo.h", ProjectFile::CXXHeader),
    };

    const ProjectFileCategorizer categorizer({}, {"foo.cpp", "foo.h"});

    QCOMPARE(categorizer.cxxSources(), expected);
    QVERIFY(categorizer.cSources().isEmpty());
    QVERIFY(categorizer.objcSources().isEmpty());
    QVERIFY(categorizer.objcxxSources().isEmpty());
}

void ProjectFileCategorizerTest::testObjectiveC()
{
    const ProjectFiles expected {
        ProjectFile("foo.m", ProjectFile::ObjCSource),
        ProjectFile("foo.h", ProjectFile::ObjCHeader),
    };

    const ProjectFileCategorizer categorizer({}, {"foo.m", "foo.h"});

    QCOMPARE(categorizer.objcSources(), expected);
    QVERIFY(categorizer.cxxSources().isEmpty());
    QVERIFY(categorizer.cSources().isEmpty());
    QVERIFY(categorizer.objcxxSources().isEmpty());
}

void ProjectFileCategorizerTest::testObjectiveCxx()
{
    const ProjectFiles expected {
        ProjectFile("foo.mm", ProjectFile::ObjCXXSource),
        ProjectFile("foo.h", ProjectFile::ObjCXXHeader),
    };

    const ProjectFileCategorizer categorizer({}, {"foo.mm", "foo.h"});

    QCOMPARE(categorizer.objcxxSources(), expected);
    QVERIFY(categorizer.objcSources().isEmpty());
    QVERIFY(categorizer.cSources().isEmpty());
    QVERIFY(categorizer.cxxSources().isEmpty());
}

void ProjectFileCategorizerTest::testMixedCAndCxx()
{
    const ProjectFiles expectedCxxSources {
        ProjectFile("foo.cpp", ProjectFile::CXXSource),
        ProjectFile("foo.h", ProjectFile::CXXHeader),
        ProjectFile("bar.h", ProjectFile::CXXHeader),
    };
    const ProjectFiles expectedCSources {
        ProjectFile("bar.c", ProjectFile::CSource),
        ProjectFile("foo.h", ProjectFile::CHeader),
        ProjectFile("bar.h", ProjectFile::CHeader),
    };

    const ProjectFileCategorizer categorizer({}, {"foo.cpp", "foo.h", "bar.c", "bar.h"});

    QCOMPARE(categorizer.cxxSources(), expectedCxxSources);
    QCOMPARE(categorizer.cSources(), expectedCSources);
    QVERIFY(categorizer.objcSources().isEmpty());
    QVERIFY(categorizer.objcxxSources().isEmpty());
}

void ProjectFileCategorizerTest::testAmbiguousHeaderOnly()
{
    const ProjectFileCategorizer categorizer({}, {"foo.h"});

    QCOMPARE(categorizer.cSources(), {ProjectFile("foo.h", ProjectFile::CHeader)});
    QCOMPARE(categorizer.cxxSources(), {ProjectFile("foo.h", ProjectFile::CXXHeader)});
    QCOMPARE(categorizer.objcSources(), {ProjectFile("foo.h", ProjectFile::ObjCHeader)});
    QCOMPARE(categorizer.objcxxSources(), {ProjectFile("foo.h", ProjectFile::ObjCXXHeader)});
}

} // namespace CppEditor::Internal
