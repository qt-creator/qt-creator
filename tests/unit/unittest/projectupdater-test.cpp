/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "googletest.h"

#include "mockfilepathcaching.h"
#include "mockpchmanagerclient.h"
#include "mockpchmanagernotifier.h"
#include "mockpchmanagerserver.h"
#include "mockprecompiledheaderstorage.h"
#include "mockprogressmanager.h"
#include "mockprojectpartsstorage.h"
#include "mocksqlitetransactionbackend.h"

#include <pchmanagerprojectupdater.h>

#include <clangindexingprojectsettings.h>
#include <clangindexingsettingsmanager.h>
#include <filepathcaching.h>
#include <pchmanagerclient.h>
#include <precompiledheaderstorage.h>
#include <precompiledheadersupdatedmessage.h>
#include <projectpartsstorage.h>
#include <refactoringdatabaseinitializer.h>
#include <removegeneratedfilesmessage.h>
#include <removeprojectpartsmessage.h>
#include <updategeneratedfilesmessage.h>
#include <updateprojectpartsmessage.h>

#include <cpptools/compileroptionsbuilder.h>
#include <cpptools/projectpart.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/namevalueitem.h>

namespace {

using testing::_;
using testing::ElementsAre;
using testing::SizeIs;
using testing::NiceMock;
using testing::AnyNumber;

using ClangBackEnd::CompilerMacro;
using ClangBackEnd::FilePath;
using ClangBackEnd::IncludeSearchPath;
using ClangBackEnd::IncludeSearchPathType;
using ClangBackEnd::ProjectPartContainer;
using ClangBackEnd::V2::FileContainer;
using CppTools::CompilerOptionsBuilder;
using ProjectExplorer::HeaderPath;

class ProjectUpdater : public testing::Test
{
protected:
    ClangBackEnd::FilePathId filePathId(Utils::SmallStringView path)
    {
        return filePathCache.filePathId(ClangBackEnd::FilePathView{path});
    }

    ClangBackEnd::FilePathIds filePathIds(const Utils::PathStringVector &paths)
    {
        return filePathCache.filePathIds(Utils::transform(paths, [] (const Utils::PathString &path) {
            return ClangBackEnd::FilePathView(path);
        }));
    }

    void SetUp() override
    {
        project.rootProjectDirectoryPath = Utils::FilePath::fromString("project");
        projectPart.project = &project;
        projectPart.files.push_back(header1ProjectFile);
        projectPart.files.push_back(header2ProjectFile);
        projectPart.files.push_back(source1ProjectFile);
        projectPart.files.push_back(source2ProjectFile);
        projectPart.files.push_back(nonActiveProjectFile);
        projectPart.displayName = "projectb";
        projectPart.projectMacros = {{"FOO", "2"}, {"BAR", "1"}, {"POO", "3"}};
        projectPartId = projectPartsStorage.fetchProjectPartId(Utils::SmallString{projectPart.id()});

        projectPart2.project = &project;
        projectPart2.files.push_back(header2ProjectFile);
        projectPart2.files.push_back(header1ProjectFile);
        projectPart2.files.push_back(source2ProjectFile);
        projectPart2.files.push_back(source1ProjectFile);
        projectPart2.files.push_back(nonActiveProjectFile);
        projectPart2.displayName = "projectaa";
        projectPart2.projectMacros = {{"BAR", "1"}, {"FOO", "2"}};
        projectPartId2 = projectPartsStorage.fetchProjectPartId(Utils::SmallString{projectPart2.id()});

        nonBuildingProjectPart.files.push_back(cannotBuildSourceProjectFile);
        nonBuildingProjectPart.displayName = "nonbuilding";
        nonBuildingProjectPart.selectedForBuilding = false;

        Utils::SmallStringVector arguments{
            ClangPchManager::ProjectUpdater::toolChainArguments(&projectPart)};
        Utils::SmallStringVector arguments2{
            ClangPchManager::ProjectUpdater::toolChainArguments(&projectPart2)};

        expectedContainer = {projectPartId,
                             arguments.clone(),
                             Utils::clone(compilerMacros),
                             {{"project/.pre_includes", 1, ClangBackEnd::IncludeSearchPathType::System},
                              {CLANG_RESOURCE_DIR, 2, ClangBackEnd::IncludeSearchPathType::BuiltIn}},
                             {},
                             {filePathId(headerPaths[1])},
                             {filePathIds(sourcePaths)},
                             Utils::Language::Cxx,
                             Utils::LanguageVersion::LatestCxx,
                             Utils::LanguageExtension::None};
        expectedContainer2 = {projectPartId2,
                              arguments2.clone(),
                              Utils::clone(compilerMacros),
                              {{"project/.pre_includes", 1, ClangBackEnd::IncludeSearchPathType::System},
                               {CLANG_RESOURCE_DIR, 2, ClangBackEnd::IncludeSearchPathType::BuiltIn}},
                              {},
                              {filePathId(headerPaths[1])},
                              {filePathIds(sourcePaths)},
                              Utils::Language::Cxx,
                              Utils::LanguageVersion::LatestCxx,
                              Utils::LanguageExtension::None};

        auto settings = settingsManager.settings(&project);
        settings->saveMacros({{"POO", "3", Utils::NameValueItem::Unset}});
    }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> initializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    NiceMock<MockProgressManager> mockPchCreationProgressManager;
    NiceMock<MockProgressManager> mockDependencyCreationProgressManager;
    ClangBackEnd::ProjectPartsStorage<Sqlite::Database> projectPartsStorage{database};
    ClangPchManager::PchManagerClient pchManagerClient{mockPchCreationProgressManager,
                                                       mockDependencyCreationProgressManager};
    MockPchManagerNotifier mockPchManagerNotifier{pchManagerClient};
    NiceMock<MockPchManagerServer> mockPchManagerServer;
    ClangPchManager::ClangIndexingSettingsManager settingsManager;
    ClangPchManager::ProjectUpdater updater{mockPchManagerServer,
                                            filePathCache,
                                            projectPartsStorage,
                                            settingsManager};
    ClangBackEnd::ProjectPartId projectPartId;
    ClangBackEnd::ProjectPartId projectPartId2;
    Utils::PathStringVector headerPaths = {"/path/to/header1.h", "/path/to/header2.h"};
    Utils::PathStringVector sourcePaths = {"/path/to/source1.cpp", "/path/to/source2.cpp"};
    ClangBackEnd::CompilerMacros compilerMacros = {{"BAR", "1", 1}, {"FOO", "2", 2}};
    CppTools::ProjectFile header1ProjectFile{QString(headerPaths[0]), CppTools::ProjectFile::CXXHeader};
    CppTools::ProjectFile header2ProjectFile{QString(headerPaths[1]), CppTools::ProjectFile::CXXHeader};
    CppTools::ProjectFile source1ProjectFile{QString(sourcePaths[0]), CppTools::ProjectFile::CXXSource};
    CppTools::ProjectFile source2ProjectFile{QString(sourcePaths[1]),
                                             CppTools::ProjectFile::CXXSource};
    CppTools::ProjectFile cannotBuildSourceProjectFile{QString("/cannot/build"),
                                                       CppTools::ProjectFile::CXXSource};
    CppTools::ProjectFile nonActiveProjectFile{QString("/foo"), CppTools::ProjectFile::CXXSource, false};
    ProjectExplorer::Project project;
    CppTools::ProjectPart projectPart;
    CppTools::ProjectPart projectPart2;
    CppTools::ProjectPart nonBuildingProjectPart;
    ProjectPartContainer expectedContainer;
    ProjectPartContainer expectedContainer2;
    FileContainer generatedFile{{"/path/to", "header1.h"},
                                filePathCache.filePathId(FilePath{"/path/to", "header1.h"}),
                                "content",
                                {}};
    FileContainer generatedFile2{{"/path/to2", "header1.h"},
                                 filePathCache.filePathId(FilePath{"/path/to2", "header1.h"}),
                                 "content",
                                 {}};
    FileContainer generatedFile3{{"/path/to", "header2.h"},
                                 filePathCache.filePathId(FilePath{"/path/to", "header2.h"}),
                                 "content",
                                 {}};
};

TEST_F(ProjectUpdater, CallUpdateProjectParts)
{
    std::vector<CppTools::ProjectPart*> projectParts = {&projectPart2, &projectPart};
    ClangBackEnd::UpdateProjectPartsMessage message{
        {expectedContainer.clone(), expectedContainer2.clone()}, {"toolChainArgument"}};
    updater.updateGeneratedFiles({generatedFile});

    EXPECT_CALL(mockPchManagerServer, updateProjectParts(message));

    updater.updateProjectParts(projectParts, {"toolChainArgument"});
}

TEST_F(ProjectUpdater, CallUpdateGeneratedFilesWithSortedEntries)
{
    ClangBackEnd::UpdateGeneratedFilesMessage message{{generatedFile, generatedFile3, generatedFile2}};

    EXPECT_CALL(mockPchManagerServer, updateGeneratedFiles(message));

    updater.updateGeneratedFiles({generatedFile2, generatedFile3, generatedFile});
}

TEST_F(ProjectUpdater, GeneratedFilesAddedAreSorted)
{
    updater.updateGeneratedFiles({generatedFile2, generatedFile3, generatedFile});

    ASSERT_THAT(updater.generatedFiles().fileContainers(),
                ElementsAre(generatedFile, generatedFile3, generatedFile2));
}

TEST_F(ProjectUpdater, GeneratedFilesAddedToExcludePaths)
{
    updater.updateGeneratedFiles({generatedFile2, generatedFile3, generatedFile});

    ASSERT_THAT(updater.excludedPaths(), ElementsAre(generatedFile.filePath,
                                                     generatedFile3.filePath,
                                                     generatedFile2.filePath));
}

TEST_F(ProjectUpdater, CallRemoveGeneratedFiles)
{
    ClangBackEnd::RemoveGeneratedFilesMessage message{{{"/path/to/header1.h"}}};

    EXPECT_CALL(mockPchManagerServer, removeGeneratedFiles(message));

    updater.removeGeneratedFiles({{"/path/to/header1.h"}});
}

TEST_F(ProjectUpdater, GeneratedFilesRemovedFromExcludePaths)
{
    updater.updateGeneratedFiles({generatedFile});

    updater.removeGeneratedFiles({generatedFile.filePath});

    ASSERT_THAT(updater.excludedPaths(), IsEmpty());
}

TEST_F(ProjectUpdater, CallRemoveProjectParts)
{
    ClangBackEnd::RemoveProjectPartsMessage message{{projectPartId, projectPartId2}};

    EXPECT_CALL(mockPchManagerServer, removeProjectParts(message));

    updater.removeProjectParts({projectPartId2, projectPartId});
}

TEST_F(ProjectUpdater, CallPrecompiledHeaderRemovedInPchManagerProjectUpdater)
{
    ClangPchManager::ClangIndexingSettingsManager settingManager;
    ClangPchManager::PchManagerProjectUpdater pchUpdater{mockPchManagerServer,
                                                         pchManagerClient,
                                                         filePathCache,
                                                         projectPartsStorage,
                                                         settingManager};
    ClangBackEnd::RemoveProjectPartsMessage message{{projectPartId, projectPartId2}};

    EXPECT_CALL(mockPchManagerNotifier, precompiledHeaderRemoved(projectPartId));
    EXPECT_CALL(mockPchManagerNotifier, precompiledHeaderRemoved(projectPartId2));

    pchUpdater.removeProjectParts({projectPartId, projectPartId2});
}

TEST_F(ProjectUpdater, ConvertProjectPartToProjectPartContainer)
{
    updater.setExcludedPaths({"/path/to/header1.h"});
    updater.fetchProjectPartIds({&projectPart});

    auto container = updater.toProjectPartContainer(&projectPart);

    ASSERT_THAT(container, expectedContainer);
}

TEST_F(ProjectUpdater, ConvertProjectPartToProjectPartContainersHaveSameSizeLikeProjectParts)
{
    updater.fetchProjectPartIds({&projectPart, &nonBuildingProjectPart});

    auto containers = updater.toProjectPartContainers(
        {&projectPart, &projectPart, &nonBuildingProjectPart});

    ASSERT_THAT(containers, SizeIs(2));
}

TEST_F(ProjectUpdater, ProjectPartIdsPrefetchingInsideTransaction)
{
    InSequence s;
    CppTools::ProjectPart projectPart;
    projectPart.project = &project;
    projectPart.displayName = "project";
    Utils::SmallString projectPartName = projectPart.id();
    NiceMock<MockSqliteTransactionBackend> mockSqliteTransactionBackend;
    NiceMock<MockProjectPartsStorage> mockProjectPartsStorage;
    ON_CALL(mockProjectPartsStorage, transactionBackend())
        .WillByDefault(ReturnRef(mockSqliteTransactionBackend));
    ClangPchManager::ClangIndexingSettingsManager settingsManager;
    ClangPchManager::ProjectUpdater updater{mockPchManagerServer,
                                            filePathCache,
                                            mockProjectPartsStorage,
                                            settingsManager};

    EXPECT_CALL(mockSqliteTransactionBackend, deferredBegin());
    EXPECT_CALL(mockProjectPartsStorage, fetchProjectPartIdUnguarded(Eq(projectPartName))).WillOnce(Return(1));
    EXPECT_CALL(mockSqliteTransactionBackend, commit());

    updater.fetchProjectPartIds({&projectPart});
}

TEST_F(ProjectUpdater, CreateSortedExcludedPaths)
{
    auto excludedPaths = updater.createExcludedPaths({generatedFile2, generatedFile3, generatedFile});

    ASSERT_THAT(excludedPaths, ElementsAre(ClangBackEnd::FilePath{"/path/to/header1.h"},
                                           ClangBackEnd::FilePath{"/path/to/header2.h"},
                                           ClangBackEnd::FilePath{"/path/to2/header1.h"}));
}

TEST_F(ProjectUpdater, CreateSortedCompilerMacros)
{
    auto paths = updater.createCompilerMacros({{"DEFINE", "1"}, {"FOO", "2"}, {"BAR", "1"}}, {});

    ASSERT_THAT(paths, ElementsAre(CompilerMacro{"BAR", "1", 1},
                                   CompilerMacro{"FOO", "2", 2},
                                   CompilerMacro{"DEFINE", "1", 3}));
}

TEST_F(ProjectUpdater, FilterCompilerMacros)
{
    auto paths = updater.createCompilerMacros({{"DEFINE", "1"},
                                               {"QT_TESTCASE_BUILDDIR", "2"},
                                               {"BAR", "1"}},
                                              {});

    ASSERT_THAT(paths, ElementsAre(CompilerMacro{"BAR", "1", 1}, CompilerMacro{"DEFINE", "1", 3}));
}

TEST_F(ProjectUpdater, FilterSettingsMacros)
{
    auto paths = updater.createCompilerMacros({{"YI", "1"}, {"SAN", "3"}, {"SE", "4"}, {"WU", "5"}},
                                              {{"SE", "44", Utils::NameValueItem::Unset},
                                               {"ER", "2", Utils::NameValueItem::SetEnabled},
                                               {"WU", "5", Utils::NameValueItem::Unset}});

    ASSERT_THAT(paths,
                ElementsAre(CompilerMacro{"ER", "2", 3},
                            CompilerMacro{"SE", "4", 3},
                            CompilerMacro{"YI", "1", 1},
                            CompilerMacro{"SAN", "3", 2}));
}

TEST_F(ProjectUpdater, CreateSortedIncludeSearchPaths)
{
    CppTools::ProjectPart projectPart;
    projectPart.project = &project;
    ProjectExplorer::HeaderPath includePath{"/to/path1", ProjectExplorer::HeaderPathType::User};
    ProjectExplorer::HeaderPath includePath2{"/to/path2", ProjectExplorer::HeaderPathType::User};
    ProjectExplorer::HeaderPath invalidPath;
    ProjectExplorer::HeaderPath frameworkPath{"/framework/path",
                                              ProjectExplorer::HeaderPathType::Framework};
    ProjectExplorer::HeaderPath builtInPath{"/builtin/path", ProjectExplorer::HeaderPathType::BuiltIn};
    ProjectExplorer::HeaderPath systemPath{"/system/path", ProjectExplorer::HeaderPathType::System};
    projectPart.headerPaths = {
        systemPath, builtInPath, frameworkPath, includePath2, includePath, invalidPath};

    auto paths = updater.createIncludeSearchPaths(projectPart);

    ASSERT_THAT(
        paths.system,
        ElementsAre(Eq(IncludeSearchPath{systemPath.path, 2, IncludeSearchPathType::System}),
                    Eq(IncludeSearchPath{builtInPath.path, 5, IncludeSearchPathType::BuiltIn}),
                    Eq(IncludeSearchPath{frameworkPath.path, 3, IncludeSearchPathType::Framework}),
                    Eq(IncludeSearchPath{"project/.pre_includes", 1, IncludeSearchPathType::System}),
                    Eq(IncludeSearchPath{CLANG_RESOURCE_DIR,
                                         4,
                                         ClangBackEnd::IncludeSearchPathType::BuiltIn})));
    ASSERT_THAT(paths.project,
                ElementsAre(Eq(IncludeSearchPath{includePath.path, 2, IncludeSearchPathType::User}),
                            Eq(IncludeSearchPath{includePath2.path, 1, IncludeSearchPathType::User})));
}

TEST_F(ProjectUpdater, ToolChainArguments)
{
    projectPart.toolChainTargetTriple = "target";
    projectPart.extraCodeModelFlags.push_back("extraflags");
    projectPart.compilerFlags.push_back("-fPIC");
    projectPart.projectConfigFile = "config.h";

    auto arguments = updater.toolChainArguments(&projectPart);

    ASSERT_THAT(arguments,
                ElementsAre(QString{"-m32"},
                            QString{"extraflags"},
                            QString{"-include"},
                            QString{"config.h"}));
}

TEST_F(ProjectUpdater, ToolChainArgumentsMSVC)
{
    projectPart.toolChainTargetTriple = "target";
    projectPart.extraCodeModelFlags.push_back("extraflags");
    projectPart.toolchainType = ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID;
    projectPart.isMsvc2015Toolchain = true;

    auto arguments = updater.toolChainArguments(&projectPart);

    ASSERT_THAT(arguments,
                ElementsAre(QString{"-m32"},
                            QString{"extraflags"},
                            QString{"-U__clang__"},
                            QString{"-U__clang_major__"},
                            QString{"-U__clang_minor__"},
                            QString{"-U__clang_patchlevel__"},
                            QString{"-U__clang_version__"},
                            QString{"-U__cpp_aggregate_bases"},
                            QString{"-U__cpp_aggregate_nsdmi"},
                            QString{"-U__cpp_alias_templates"},
                            QString{"-U__cpp_aligned_new"},
                            QString{"-U__cpp_attributes"},
                            QString{"-U__cpp_binary_literals"},
                            QString{"-U__cpp_capture_star_this"},
                            QString{"-U__cpp_constexpr"},
                            QString{"-U__cpp_decltype"},
                            QString{"-U__cpp_decltype_auto"},
                            QString{"-U__cpp_deduction_guides"},
                            QString{"-U__cpp_delegating_constructors"},
                            QString{"-U__cpp_digit_separators"},
                            QString{"-U__cpp_enumerator_attributes"},
                            QString{"-U__cpp_exceptions"},
                            QString{"-U__cpp_fold_expressions"},
                            QString{"-U__cpp_generic_lambdas"},
                            QString{"-U__cpp_guaranteed_copy_elision"},
                            QString{"-U__cpp_hex_float"},
                            QString{"-U__cpp_if_constexpr"},
                            QString{"-U__cpp_impl_destroying_delete"},
                            QString{"-U__cpp_inheriting_constructors"},
                            QString{"-U__cpp_init_captures"},
                            QString{"-U__cpp_initializer_lists"},
                            QString{"-U__cpp_inline_variables"},
                            QString{"-U__cpp_lambdas"},
                            QString{"-U__cpp_namespace_attributes"},
                            QString{"-U__cpp_nested_namespace_definitions"},
                            QString{"-U__cpp_noexcept_function_type"},
                            QString{"-U__cpp_nontype_template_args"},
                            QString{"-U__cpp_nontype_template_parameter_auto"},
                            QString{"-U__cpp_nsdmi"},
                            QString{"-U__cpp_range_based_for"},
                            QString{"-U__cpp_raw_strings"},
                            QString{"-U__cpp_ref_qualifiers"},
                            QString{"-U__cpp_return_type_deduction"},
                            QString{"-U__cpp_rtti"},
                            QString{"-U__cpp_rvalue_references"},
                            QString{"-U__cpp_static_assert"},
                            QString{"-U__cpp_structured_bindings"},
                            QString{"-U__cpp_template_auto"},
                            QString{"-U__cpp_threadsafe_static_init"},
                            QString{"-U__cpp_unicode_characters"},
                            QString{"-U__cpp_unicode_literals"},
                            QString{"-U__cpp_user_defined_literals"},
                            QString{"-U__cpp_variable_templates"},
                            QString{"-U__cpp_variadic_templates"},
                            QString{"-U__cpp_variadic_using"}));
}

TEST_F(ProjectUpdater, FetchProjectPartName)
{
    updater.updateProjectParts({&projectPart}, {});

    auto projectPartName = updater.fetchProjectPartName(1);

    ASSERT_THAT(projectPartName, Eq(QString{" projectb"}));
}

TEST_F(ProjectUpdater, AddProjectFilesToFilePathCache)
{
    NiceMock<MockFilePathCaching> mockFilePathCaching;
    ClangPchManager::ProjectUpdater updater{mockPchManagerServer,
                                            mockFilePathCaching,
                                            projectPartsStorage,
                                            settingsManager};

    EXPECT_CALL(mockFilePathCaching,
                addFilePaths(UnorderedElementsAre(Eq(headerPaths[0]),
                                                  Eq(headerPaths[1]),
                                                  Eq(sourcePaths[0]),
                                                  Eq(sourcePaths[1]))));

    updater.updateProjectParts({&projectPart}, {});
}

TEST_F(ProjectUpdater, FillProjectPartIdCacheAtCreation)
{
    NiceMock<MockProjectPartsStorage> mockProjectPartsStorage;

    EXPECT_CALL(mockProjectPartsStorage, fetchAllProjectPartNamesAndIds());

    ClangPchManager::ProjectUpdater updater{mockPchManagerServer,
                                            filePathCache,
                                            mockProjectPartsStorage,
                                            settingsManager};
}

TEST_F(ProjectUpdater, DontFetchProjectPartIdFromDatabaseIfItIsInCache)
{
    projectPart.files.clear();
    NiceMock<MockSqliteTransactionBackend> mockSqliteTransactionBackend;
    NiceMock<MockProjectPartsStorage> mockProjectPartsStorage;
    ON_CALL(mockProjectPartsStorage, transactionBackend())
        .WillByDefault(ReturnRef(mockSqliteTransactionBackend));
    ON_CALL(mockProjectPartsStorage, fetchAllProjectPartNamesAndIds())
        .WillByDefault(Return(
            ClangBackEnd::Internal::ProjectPartNameIds{{Utils::PathString(projectPart.id()), 55}}));
    ClangPchManager::ProjectUpdater updater{mockPchManagerServer,
                                            filePathCache,
                                            mockProjectPartsStorage,
                                            settingsManager};

    EXPECT_CALL(mockSqliteTransactionBackend, deferredBegin()).Times(0);
    EXPECT_CALL(mockProjectPartsStorage, fetchProjectPartId(_)).Times(0);
    EXPECT_CALL(mockProjectPartsStorage, fetchProjectPartIdUnguarded(_)).Times(0);
    EXPECT_CALL(mockSqliteTransactionBackend, commit()).Times(0);

    updater.updateProjectParts({&projectPart}, {});
}

} // namespace
