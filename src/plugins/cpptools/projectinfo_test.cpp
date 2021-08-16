/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "cppprojectfilecategorizer.h"
#include "cppprojectinfogenerator.h"
#include "cppprojectpartchooser.h"
#include "cpptoolsplugin.h"
#include "headerpathfilter.h"
#include "projectinfo.h"

#include <projectexplorer/toolchainconfigwidget.h>
#include <utils/algorithm.h>

#include <QtTest>

using namespace ProjectExplorer;

namespace CppTools {
namespace Internal {

namespace {
class ProjectPartChooserTest
{
public:
    ProjectPartChooserTest()
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
        return chooser.choose(filePath, currentProjectPartInfo, preferredProjectPartId,
                              projectMap.value(activeProject).get(),
                              languagePreference, projectsChanged);
    }

    static QList<ProjectPart::Ptr> createProjectPartsWithDifferentProjects()
    {
        QList<ProjectPart::Ptr> projectParts;

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

    static QList<ProjectPart::Ptr> createCAndCxxProjectParts()
    {
        QList<ProjectPart::Ptr> projectParts;
        ToolChainInfo tcInfo;

        // Create project part for C
        tcInfo.macroInspectionRunner = [](const QStringList &) {
            return ToolChain::MacroInspectionReport{{}, Utils::LanguageVersion::C11};
        };
        const ProjectPart::Ptr cprojectpart = ProjectPart::create({}, {}, {}, {}, {}, {}, {},
                                                                  tcInfo);
        projectParts.append(cprojectpart);

        // Create project part for CXX
        tcInfo.macroInspectionRunner = [](const QStringList &) {
            return ToolChain::MacroInspectionReport{{}, Utils::LanguageVersion::CXX98};
        };
        const ProjectPart::Ptr cxxprojectpart = ProjectPart::create({}, {}, {}, {}, {}, {}, {},
                                                                    tcInfo);
        projectParts.append(cxxprojectpart);

        return projectParts;
    }

    QString filePath;
    ProjectPart::Ptr currentProjectPart = ProjectPart::create({});
    ProjectPartInfo currentProjectPartInfo{currentProjectPart,
                                           {currentProjectPart},
                                           ProjectPartInfo::NoHint};
    QString preferredProjectPartId;
    Utils::FilePath activeProject;
    Language languagePreference = Language::Cxx;
    bool projectsChanged = false;
    ProjectPartChooser chooser;

    QList<ProjectPart::Ptr> projectPartsForFile;
    QList<ProjectPart::Ptr> projectPartsFromDependenciesForFile;
    ProjectPart::Ptr fallbackProjectPart;

    static QHash<Utils::FilePath, std::shared_ptr<Project>> projectMap;
};

QHash<Utils::FilePath, std::shared_ptr<Project>>
ProjectPartChooserTest::projectMap;
}

void CppToolsPlugin::test_projectPartChooser_chooseManuallySet()
{
    ProjectPart::Ptr p1 = ProjectPart::create({});
    RawProjectPart rpp2;
    rpp2.setProjectFileLocation("someId");
    ProjectPart::Ptr p2 = ProjectPart::create({}, rpp2);
    ProjectPartChooserTest t;
    t.preferredProjectPartId = p2->projectFile;
    t.projectPartsForFile += {p1, p2};

    QCOMPARE(t.choose().projectPart, p2);
}

void CppToolsPlugin::test_projectPartChooser_indicateManuallySet()
{
    ProjectPart::Ptr p1 = ProjectPart::create({});
    RawProjectPart rpp2;
    rpp2.setProjectFileLocation("someId");
    ProjectPart::Ptr p2 = ProjectPart::create({}, rpp2);
    ProjectPartChooserTest t;
    t.preferredProjectPartId = p2->projectFile;
    t.projectPartsForFile += {p1, p2};

    QVERIFY(t.choose().hints & ProjectPartInfo::IsPreferredMatch);
}

void CppToolsPlugin::test_projectPartChooser_indicateManuallySetForFallbackToProjectPartFromDependencies()
{
    ProjectPart::Ptr p1 = ProjectPart::create({});
    RawProjectPart rpp2;
    rpp2.setProjectFileLocation("someId");
    ProjectPart::Ptr p2 = ProjectPart::create({}, rpp2);
    ProjectPartChooserTest t;
    t.preferredProjectPartId = p2->projectFile;
    t.projectPartsFromDependenciesForFile += {p1, p2};

    QVERIFY(t.choose().hints & ProjectPartInfo::IsPreferredMatch);
}

void CppToolsPlugin::test_projectPartChooser_doNotIndicateNotManuallySet()
{
    QVERIFY(!(ProjectPartChooserTest().choose().hints & ProjectPartInfo::IsPreferredMatch));
}

void CppToolsPlugin::test_projectPartChooser_forMultipleChooseFromActiveProject()
{
    ProjectPartChooserTest t;
    const QList<ProjectPart::Ptr> projectParts = t.createProjectPartsWithDifferentProjects();
    const ProjectPart::Ptr secondProjectPart = projectParts.at(1);
    t.projectPartsForFile += projectParts;
    t.activeProject = secondProjectPart->topLevelProject;

    QCOMPARE(t.choose().projectPart, secondProjectPart);
}

void CppToolsPlugin::test_projectPartChooser_forMultiplePreferSelectedForBuilding()
{
    RawProjectPart rpp1;
    rpp1.setSelectedForBuilding(false);
    RawProjectPart rpp2;
    rpp2.setSelectedForBuilding(true);
    const ProjectPart::Ptr firstProjectPart = ProjectPart::create({}, rpp1);
    const ProjectPart::Ptr secondProjectPart = ProjectPart::create({}, rpp2);
    ProjectPartChooserTest t;
    t.projectPartsForFile += firstProjectPart;
    t.projectPartsForFile += secondProjectPart;

    QCOMPARE(t.choose().projectPart, secondProjectPart);
}

void CppToolsPlugin::test_projectPartChooser_forMultipleFromDependenciesChooseFromActiveProject()
{
    ProjectPartChooserTest t;
    const QList<ProjectPart::Ptr> projectParts = t.createProjectPartsWithDifferentProjects();
    const ProjectPart::Ptr secondProjectPart = projectParts.at(1);
    t.projectPartsFromDependenciesForFile += projectParts;
    t.activeProject = secondProjectPart->topLevelProject;

    QCOMPARE(t.choose().projectPart, secondProjectPart);
}

void CppToolsPlugin::test_projectPartChooser_forMultipleCheckIfActiveProjectChanged()
{
    ProjectPartChooserTest t;
    const QList<ProjectPart::Ptr> projectParts = t.createProjectPartsWithDifferentProjects();
    const ProjectPart::Ptr firstProjectPart = projectParts.at(0);
    const ProjectPart::Ptr secondProjectPart = projectParts.at(1);
    t.projectPartsForFile += projectParts;
    t.currentProjectPartInfo.projectPart = firstProjectPart;
    t.activeProject = secondProjectPart->topLevelProject;

    QCOMPARE(t.choose().projectPart, secondProjectPart);
}

void CppToolsPlugin::test_projectPartChooser_forMultipleAndAmbigiousHeaderPreferCProjectPart()
{
    ProjectPartChooserTest t;
    t.languagePreference = Language::C;
    t.projectPartsForFile = t.createCAndCxxProjectParts();
    const ProjectPart::Ptr cProjectPart = t.projectPartsForFile.at(0);

    QCOMPARE(t.choose().projectPart, cProjectPart);
}

void CppToolsPlugin::test_projectPartChooser_forMultipleAndAmbigiousHeaderPreferCxxProjectPart()
{
    ProjectPartChooserTest t;
    t.languagePreference = Language::Cxx;
    t.projectPartsForFile = t.createCAndCxxProjectParts();
    const ProjectPart::Ptr cxxProjectPart = t.projectPartsForFile.at(1);

    QCOMPARE(t.choose().projectPart, cxxProjectPart);
}

void CppToolsPlugin::test_projectPartChooser_indicateMultiple()
{
    const ProjectPart::Ptr p1 = ProjectPart::create({});
    const ProjectPart::Ptr p2 = ProjectPart::create({});
    ProjectPartChooserTest t;
    t.projectPartsForFile += {p1, p2};

    QVERIFY(t.choose().hints & ProjectPartInfo::IsAmbiguousMatch);
}

void CppToolsPlugin::test_projectPartChooser_indicateMultipleForFallbackToProjectPartFromDependencies()
{
    const ProjectPart::Ptr p1 = ProjectPart::create({});
    const ProjectPart::Ptr p2 = ProjectPart::create({});
    ProjectPartChooserTest t;
    t.projectPartsFromDependenciesForFile += {p1, p2};

    QVERIFY(t.choose().hints & ProjectPartInfo::IsAmbiguousMatch);
}

void CppToolsPlugin::test_projectPartChooser_forMultipleChooseNewIfPreviousIsGone()
{
    const ProjectPart::Ptr newProjectPart = ProjectPart::create({});
    ProjectPartChooserTest t;
    t.projectPartsForFile += newProjectPart;

    QCOMPARE(t.choose().projectPart, newProjectPart);
}

void CppToolsPlugin::test_projectPartChooser_fallbackToProjectPartFromDependencies()
{
    const ProjectPart::Ptr fromDependencies = ProjectPart::create({});
    ProjectPartChooserTest t;
    t.projectPartsFromDependenciesForFile += fromDependencies;

    QCOMPARE(t.choose().projectPart, fromDependencies);
}

void CppToolsPlugin::test_projectPartChooser_fallbackToProjectPartFromModelManager()
{
    ProjectPartChooserTest t;
    t.fallbackProjectPart = ProjectPart::create({});

    QCOMPARE(t.choose().projectPart, t.fallbackProjectPart);
}

void CppToolsPlugin::test_projectPartChooser_continueUsingFallbackFromModelManagerIfProjectDoesNotChange()
{
    // ...without re-calculating the dependency table.
    ProjectPartChooserTest t;
    t.fallbackProjectPart = ProjectPart::create({});
    t.currentProjectPartInfo.projectPart = t.fallbackProjectPart;
    t.currentProjectPartInfo.hints |= ProjectPartInfo::IsFallbackMatch;
    t.projectPartsFromDependenciesForFile += ProjectPart::create({});

    QCOMPARE(t.choose().projectPart, t.fallbackProjectPart);
}

void CppToolsPlugin::test_projectPartChooser_stopUsingFallbackFromModelManagerIfProjectChanges1()
{
    ProjectPartChooserTest t;
    t.fallbackProjectPart = ProjectPart::create({});
    t.currentProjectPartInfo.projectPart = t.fallbackProjectPart;
    t.currentProjectPartInfo.hints |= ProjectPartInfo::IsFallbackMatch;
    const ProjectPart::Ptr addedProject = ProjectPart::create({});
    t.projectPartsForFile += addedProject;

    QCOMPARE(t.choose().projectPart, addedProject);
}

void CppToolsPlugin::test_projectPartChooser_stopUsingFallbackFromModelManagerIfProjectChanges2()
{
    ProjectPartChooserTest t;
    t.fallbackProjectPart = ProjectPart::create({});
    t.currentProjectPartInfo.projectPart = t.fallbackProjectPart;
    t.currentProjectPartInfo.hints |= ProjectPartInfo::IsFallbackMatch;
    const ProjectPart::Ptr addedProject = ProjectPart::create({});
    t.projectPartsFromDependenciesForFile += addedProject;
    t.projectsChanged = true;

    QCOMPARE(t.choose().projectPart, addedProject);
}

void CppToolsPlugin::test_projectPartChooser_indicateFallbacktoProjectPartFromModelManager()
{
    ProjectPartChooserTest t;
    t.fallbackProjectPart = ProjectPart::create({});

    QVERIFY(t.choose().hints & ProjectPartInfo::IsFallbackMatch);
}

void CppToolsPlugin::test_projectPartChooser_indicateFromDependencies()
{
    ProjectPartChooserTest t;
    t.projectPartsFromDependenciesForFile += ProjectPart::create({});

    QVERIFY(t.choose().hints & ProjectPartInfo::IsFromDependenciesMatch);
}

void CppToolsPlugin::test_projectPartChooser_doNotIndicateFromDependencies()
{
    ProjectPartChooserTest t;
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

class ProjectInfoGeneratorTest
{
public:
    ProjectInfoGeneratorTest()
    {
        TestToolchain aToolChain;
        projectUpdateInfo.cxxToolChainInfo = {&aToolChain, {}, {}};
        projectUpdateInfo.cToolChainInfo = {&aToolChain, {}, {}};
    }

    ProjectInfo::Ptr generate()
    {
        QFutureInterface<ProjectInfo::Ptr> fi;

        projectUpdateInfo.rawProjectParts += rawProjectPart;
        ProjectInfoGenerator generator(fi, projectUpdateInfo);

        return generator.generate();
    }

    ProjectUpdateInfo projectUpdateInfo;
    RawProjectPart rawProjectPart;
};
}

void CppToolsPlugin::test_projectInfoGenerator_createNoProjectPartsForEmptyFileList()
{
    ProjectInfoGeneratorTest t;
    const ProjectInfo::Ptr projectInfo = t.generate();

    QVERIFY(projectInfo->projectParts().isEmpty());
}

void CppToolsPlugin::test_projectInfoGenerator_createSingleProjectPart()
{
    ProjectInfoGeneratorTest t;
    t.rawProjectPart.files = QStringList{ "foo.cpp", "foo.h"};
    const ProjectInfo::Ptr projectInfo = t.generate();

    QCOMPARE(projectInfo->projectParts().size(), 1);
}

void CppToolsPlugin::test_projectInfoGenerator_createMultipleProjectParts()
{
    ProjectInfoGeneratorTest t;
    t.rawProjectPart.files = QStringList{ "foo.cpp", "foo.h", "bar.c", "bar.h" };
    const ProjectInfo::Ptr projectInfo = t.generate();

    QCOMPARE(projectInfo->projectParts().size(), 2);
}

void CppToolsPlugin::test_projectInfoGenerator_projectPartIndicatesObjectiveCExtensionsByDefault()
{
    ProjectInfoGeneratorTest t;
    t.rawProjectPart.files = QStringList{ "foo.mm" };
    const ProjectInfo::Ptr projectInfo = t.generate();
    QCOMPARE(projectInfo->projectParts().size(), 1);

    const ProjectPart &projectPart = *projectInfo->projectParts().at(0);
    QVERIFY(projectPart.languageExtensions & Utils::LanguageExtension::ObjectiveC);
}

void CppToolsPlugin::test_projectInfoGenerator_projectPartHasLatestLanguageVersionByDefault()
{
    ProjectInfoGeneratorTest t;
    t.rawProjectPart.files = QStringList{ "foo.cpp" };
    const ProjectInfo::Ptr projectInfo = t.generate();
    QCOMPARE(projectInfo->projectParts().size(), 1);

    const ProjectPart &projectPart = *projectInfo->projectParts().at(0);
    QCOMPARE(projectPart.languageVersion, Utils::LanguageVersion::LatestCxx);
}

void CppToolsPlugin::test_projectInfoGenerator_useMacroInspectionReportForLanguageVersion()
{
    ProjectInfoGeneratorTest t;
    t.projectUpdateInfo.cxxToolChainInfo.macroInspectionRunner = [](const QStringList &) {
        return TestToolchain::MacroInspectionReport{Macros(), Utils::LanguageVersion::CXX17};
    };
    t.rawProjectPart.files = QStringList{ "foo.cpp" };
    const ProjectInfo::Ptr projectInfo = t.generate();

    QCOMPARE(projectInfo->projectParts().size(), 1);

    const ProjectPart &projectPart = *projectInfo->projectParts().at(0);
    QCOMPARE(projectPart.languageVersion, Utils::LanguageVersion::CXX17);
}

void CppToolsPlugin::test_projectInfoGenerator_useCompilerFlagsForLanguageExtensions()
{
    ProjectInfoGeneratorTest t;
    t.rawProjectPart.files = QStringList{ "foo.cpp" };
    t.rawProjectPart.flagsForCxx.languageExtensions = Utils::LanguageExtension::Microsoft;
    const ProjectInfo::Ptr projectInfo = t.generate();

    QCOMPARE(projectInfo->projectParts().size(), 1);

    const ProjectPart &projectPart = *projectInfo->projectParts().at(0);
    QVERIFY(projectPart.languageExtensions & Utils::LanguageExtension::Microsoft);
}

void CppToolsPlugin::test_projectInfoGenerator_projectFileKindsMatchProjectPartVersion()
{
    ProjectInfoGeneratorTest t;
    t.rawProjectPart.files = QStringList{ "foo.h" };
    const ProjectInfo::Ptr projectInfo = t.generate();

    QCOMPARE(projectInfo->projectParts().size(), 4);
    QVERIFY(Utils::contains(projectInfo->projectParts(), [](const ProjectPart::Ptr &p) {
        return p->languageVersion == Utils::LanguageVersion::LatestC
                && p->files.first().kind == ProjectFile::CHeader;
    }));
    QVERIFY(Utils::contains(projectInfo->projectParts(), [](const ProjectPart::Ptr &p) {
        return p->languageVersion == Utils::LanguageVersion::LatestC
                && p->files.first().kind == ProjectFile::ObjCHeader;
    }));
    QVERIFY(Utils::contains(projectInfo->projectParts(), [](const ProjectPart::Ptr &p) {
        return p->languageVersion == Utils::LanguageVersion::LatestCxx
                && p->files.first().kind == ProjectFile::CXXHeader;
    }));
    QVERIFY(Utils::contains(projectInfo->projectParts(), [](const ProjectPart::Ptr &p) {
        return p->languageVersion == Utils::LanguageVersion::LatestCxx
                && p->files.first().kind == ProjectFile::ObjCXXHeader;
    }));
}

namespace {
class HeaderPathFilterTest
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
        filter.emplace(HeaderPathFilter(*projectPart, UseTweakedHeaderPaths::No, {}, {},
                                        "/project", "/build"));
        return *projectPart;
    }

    static HeaderPath builtIn(const QString &path)
    {
        return HeaderPath{path, HeaderPathType::BuiltIn};
    }
    static HeaderPath system(const QString &path)
    {
        return HeaderPath{path, HeaderPathType::System};
    }
    static HeaderPath framework(const QString &path)
    {
        return HeaderPath{path, HeaderPathType::Framework};
    }
    static HeaderPath user(const QString &path)
    {
        return HeaderPath{path, HeaderPathType::User};
    }

    QString targetTriple;
    Utils::Id toolchainType;
    Utils::FilePath toolchainInstallDir;
    HeaderPaths headerPaths = {
        HeaderPath{"", HeaderPathType::BuiltIn},
        HeaderPath{"/builtin_path", HeaderPathType::BuiltIn},
        HeaderPath{"/system_path", HeaderPathType::System},
        HeaderPath{"/framework_path", HeaderPathType::Framework},
        HeaderPath{"/outside_project_user_path", HeaderPathType::User},
        HeaderPath{"/build/user_path", HeaderPathType::User},
        HeaderPath{"/buildb/user_path", HeaderPathType::User},
        HeaderPath{"/projectb/user_path", HeaderPathType::User},
        HeaderPath{"/project/user_path", HeaderPathType::User}};

    Utils::optional<HeaderPathFilter> filter;

private:
    ProjectPart::Ptr projectPart;
};
}

void CppToolsPlugin::test_headerPathFilter_builtin()
{
    HeaderPathFilterTest t;
    t.finalize();
    t.filter->process();

    QCOMPARE(t.filter->builtInHeaderPaths, (HeaderPaths{t.builtIn("/builtin_path")}));
}

void CppToolsPlugin::test_headerPathFilter_system()
{
    HeaderPathFilterTest t;
    t.finalize();
    t.filter->process();

    QCOMPARE(t.filter->systemHeaderPaths, (HeaderPaths{
        t.system("/project/.pre_includes"), t.system("/system_path"),
        t.framework("/framework_path"), t.user("/outside_project_user_path"),
        t.user("/buildb/user_path"), t.user("/projectb/user_path")}));
}

void CppToolsPlugin::test_headerPathFilter_user()
{
    HeaderPathFilterTest t;
    t.finalize();
    t.filter->process();

    QCOMPARE(t.filter->userHeaderPaths, (HeaderPaths{t.user("/build/user_path"),
                                                     t.user("/project/user_path")}));
}

void CppToolsPlugin::test_headerPathFilter_noProjectPathSet()
{
    HeaderPathFilterTest t;
    HeaderPathFilter filter{t.finalize(), UseTweakedHeaderPaths::No};
    filter.process();

    QCOMPARE(filter.userHeaderPaths, (HeaderPaths{
        t.user("/outside_project_user_path"), t.user("/build/user_path"),
        t.user("/buildb/user_path"), t.user("/projectb/user_path"),
        t.user("/project/user_path")}));
}

void CppToolsPlugin::test_headerPathFilter_dontAddInvalidPath()
{
    HeaderPathFilterTest t;
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

void CppToolsPlugin::test_headerPathFilter_clangHeadersPath()
{
    HeaderPathFilterTest t;
    HeaderPathFilter filter(t.finalize(), UseTweakedHeaderPaths::Yes, "6.0", "clang_dir");
    filter.process();

    QCOMPARE(filter.builtInHeaderPaths, (HeaderPaths{t.builtIn("clang_dir"),
                                                     t.builtIn("/builtin_path")}));
}

void CppToolsPlugin::test_headerPathFilter_clangHeadersPathWitoutClangVersion()
{
    HeaderPathFilterTest t;
    HeaderPathFilter filter(t.finalize(), UseTweakedHeaderPaths::Yes);
    filter.process();

    QCOMPARE(filter.builtInHeaderPaths, (HeaderPaths{t.builtIn("/builtin_path")}));
}

void CppToolsPlugin::test_headerPathFilter_clangHeadersAndCppIncludesPathsOrderMacOs()
{
    HeaderPathFilterTest t;
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
    HeaderPathFilter filter(t.finalize(), UseTweakedHeaderPaths::Yes, "6.0", "clang_dir");
    filter.process();

    QCOMPARE(filter.builtInHeaderPaths, (HeaderPaths{
        t.builtIn("/usr/include/c++/4.2.1"), t.builtIn("/usr/include/c++/4.2.1/backward"),
        t.builtIn("/usr/local/include"), t.builtIn("clang_dir"),
        t.builtIn("/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include"),
        t.builtIn("/usr/include"),
        t.builtIn("/builtin_path")}));
}

void CppToolsPlugin::test_headerPathFilter_clangHeadersAndCppIncludesPathsOrderLinux()
{
    HeaderPathFilterTest t;
    t.targetTriple = "x86_64-linux-gnu";
    const auto builtIns = {
        t.builtIn("/usr/include/c++/4.8"), t.builtIn("/usr/include/c++/4.8/backward"),
        t.builtIn("/usr/include/x86_64-linux-gnu/c++/4.8"),
        t.builtIn("/usr/local/include"), t.builtIn("/usr/lib/gcc/x86_64-linux-gnu/4.8/include"),
        t.builtIn("/usr/include/x86_64-linux-gnu"), t.builtIn("/usr/include")};
    std::copy(builtIns.begin(), builtIns.end(),
              std::inserter(t.headerPaths, t.headerPaths.begin()));
    HeaderPathFilter filter(t.finalize(), UseTweakedHeaderPaths::Yes, "6.0", "clang_dir");
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
void CppToolsPlugin::test_headerPathFilter_removeGccInternalPaths()
{
    HeaderPathFilterTest t;
    t.toolchainInstallDir = Utils::FilePath::fromUtf8("/usr/lib/gcc/x86_64-linux-gnu/7");
    t.toolchainType = Constants::GCC_TOOLCHAIN_TYPEID;
    t.headerPaths = {
        t.builtIn("/usr/lib/gcc/x86_64-linux-gnu/7/include"),
        t.builtIn("/usr/lib/gcc/x86_64-linux-gnu/7/include-fixed"),
    };
    HeaderPathFilter filter(t.finalize(), UseTweakedHeaderPaths::Yes, "6.0", "clang_dir");
    filter.process();

    QCOMPARE(filter.builtInHeaderPaths, (HeaderPaths{t.builtIn("clang_dir")}));
}

// Some distributions ship the standard library headers in "<installdir>/include/c++" (MinGW)
// or e.g. "<installdir>/include/g++-v8" (Gentoo).
// Ensure that we do not remove include paths pointing there.
void CppToolsPlugin::test_headerPathFilter_removeGccInternalPathsExceptForStandardPaths()
{
    HeaderPathFilterTest t;
    t.toolchainInstallDir = Utils::FilePath::fromUtf8("c:/mingw/lib/gcc/x86_64-w64-mingw32/7.3.0");
    t.toolchainType = Constants::MINGW_TOOLCHAIN_TYPEID;
    t.headerPaths = {
        t.builtIn("c:/mingw/lib/gcc/x86_64-w64-mingw32/7.3.0/include/c++"),
        t.builtIn("c:/mingw/lib/gcc/x86_64-w64-mingw32/7.3.0/include/c++/x86_64-w64-mingw32"),
        t.builtIn("c:/mingw/lib/gcc/x86_64-w64-mingw32/7.3.0/include/c++/backward"),
    };

    HeaderPaths expected = t.headerPaths;
    expected.append(t.builtIn("clang_dir"));
    HeaderPathFilter filter(t.finalize(), UseTweakedHeaderPaths::Yes, "6.0", "clang_dir");
    filter.process();

    QCOMPARE(filter.builtInHeaderPaths, expected);
}

void CppToolsPlugin::test_headerPathFilter_clangHeadersAndCppIncludesPathsOrderNoVersion()
{
    HeaderPathFilterTest t;
    t.headerPaths = {
        t.builtIn("C:/mingw/i686-w64-mingw32/include"),
        t.builtIn("C:/mingw/i686-w64-mingw32/include/c++"),
        t.builtIn("C:/mingw/i686-w64-mingw32/include/c++/i686-w64-mingw32"),
        t.builtIn("C:/mingw/i686-w64-mingw32/include/c++/backward"),
    };
    t.targetTriple = "x86_64-w64-windows-gnu";
    HeaderPathFilter filter(t.finalize(), UseTweakedHeaderPaths::Yes, "6.0", "clang_dir");
    filter.process();

    QCOMPARE(filter.builtInHeaderPaths, (HeaderPaths{
        t.builtIn("C:/mingw/i686-w64-mingw32/include/c++"),
        t.builtIn("C:/mingw/i686-w64-mingw32/include/c++/i686-w64-mingw32"),
        t.builtIn("C:/mingw/i686-w64-mingw32/include/c++/backward"),
        t.builtIn("clang_dir"),
        t.builtIn("C:/mingw/i686-w64-mingw32/include")}));
}

void CppToolsPlugin::test_headerPathFilter_clangHeadersAndCppIncludesPathsOrderAndroidClang()
{
    HeaderPathFilterTest t;
    t.headerPaths = {
        t.builtIn("C:/Android/sdk/ndk-bundle/sysroot/usr/include/i686-linux-android"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sources/cxx-stl/llvm-libc++/include"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sources/android/support/include"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sources/cxx-stl/llvm-libc++abi/include"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sysroot/usr/include")
    };
    t.targetTriple = "i686-linux-android";
    HeaderPathFilter filter(t.finalize(), UseTweakedHeaderPaths::Yes, "6.0", "clang_dir");
    filter.process();

    QCOMPARE(filter.builtInHeaderPaths, (HeaderPaths{
        t.builtIn("C:/Android/sdk/ndk-bundle/sources/cxx-stl/llvm-libc++/include"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sources/cxx-stl/llvm-libc++abi/include"),
        t.builtIn("clang_dir"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sysroot/usr/include/i686-linux-android"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sources/android/support/include"),
        t.builtIn("C:/Android/sdk/ndk-bundle/sysroot/usr/include")}));
}

void CppToolsPlugin::test_projectFileCategorizer_c()
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

void CppToolsPlugin::test_projectFileCategorizer_cxxWithUnambiguousHeaderSuffix()
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

void CppToolsPlugin::test_projectFileCategorizer_cxxWithAmbiguousHeaderSuffix()
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

void CppToolsPlugin::test_projectFileCategorizer_objectiveC()
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

void CppToolsPlugin::test_projectFileCategorizer_objectiveCxx()
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

void CppToolsPlugin::test_projectFileCategorizer_mixedCAndCxx()
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

void CppToolsPlugin::test_projectFileCategorizer_ambiguousHeaderOnly()
{
    const ProjectFileCategorizer categorizer({}, {"foo.h"});

    QCOMPARE(categorizer.cSources(), {ProjectFile("foo.h", ProjectFile::CHeader)});
    QCOMPARE(categorizer.cxxSources(), {ProjectFile("foo.h", ProjectFile::CXXHeader)});
    QCOMPARE(categorizer.objcSources(), {ProjectFile("foo.h", ProjectFile::ObjCHeader)});
    QCOMPARE(categorizer.objcxxSources(), {ProjectFile("foo.h", ProjectFile::ObjCXXHeader)});
}

} // namespace Internal
} // namespace CppTools
