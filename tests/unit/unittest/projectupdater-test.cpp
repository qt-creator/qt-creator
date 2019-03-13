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

#include "mockpchmanagerclient.h"
#include "mockpchmanagernotifier.h"
#include "mockpchmanagerserver.h"
#include "mockprecompiledheaderstorage.h"
#include "mockprogressmanager.h"
#include "mockprojectpartsstorage.h"
#include "mocksqlitetransactionbackend.h"

#include <pchmanagerprojectupdater.h>

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

#include <projectexplorer/projectexplorerconstants.h>
#include <cpptools/compileroptionsbuilder.h>
#include <cpptools/projectpart.h>

#include <utils/algorithm.h>

namespace {

using testing::_;
using testing::ElementsAre;
using testing::SizeIs;
using testing::NiceMock;
using testing::AnyNumber;

using ClangBackEnd::CompilerMacro;
using ClangBackEnd::IncludeSearchPath;
using ClangBackEnd::IncludeSearchPathType;
using ClangBackEnd::V2::FileContainer;
using ClangBackEnd::ProjectPartContainer;
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
        projectPart.files.push_back(header1ProjectFile);
        projectPart.files.push_back(header2ProjectFile);
        projectPart.files.push_back(source1ProjectFile);
        projectPart.files.push_back(source2ProjectFile);
        projectPart.files.push_back(nonActiveProjectFile);
        projectPart.displayName = "projectb";
        projectPart.projectMacros = {{"FOO", "2"}, {"BAR", "1"}};
        projectPartId = projectPartsStorage.fetchProjectPartId(Utils::SmallString{projectPart.id()});

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
                             {{CLANG_RESOURCE_DIR, 1, ClangBackEnd::IncludeSearchPathType::BuiltIn}},
                             {},
                             {filePathId(headerPaths[1])},
                             {filePathIds(sourcePaths)},
                             Utils::Language::Cxx,
                             Utils::LanguageVersion::LatestCxx,
                             Utils::LanguageExtension::None};
        expectedContainer2 = {projectPartId2,
                              arguments2.clone(),
                              Utils::clone(compilerMacros),
                              {{CLANG_RESOURCE_DIR, 1, ClangBackEnd::IncludeSearchPathType::BuiltIn}},
                              {},
                              {filePathId(headerPaths[1])},
                              {filePathIds(sourcePaths)},
                              Utils::Language::Cxx,
                              Utils::LanguageVersion::LatestCxx,
                              Utils::LanguageExtension::None};
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
    ClangPchManager::ProjectUpdater updater{mockPchManagerServer, filePathCache, projectPartsStorage};
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
    CppTools::ProjectPart projectPart;
    CppTools::ProjectPart projectPart2;
    CppTools::ProjectPart nonBuildingProjectPart;
    ProjectPartContainer expectedContainer;
    ProjectPartContainer expectedContainer2;
    FileContainer generatedFile{{"/path/to", "header1.h"}, "content", {}};
    FileContainer generatedFile2{{"/path/to2", "header1.h"}, "content", {}};
    FileContainer generatedFile3{{"/path/to", "header2.h"}, "content", {}};
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
    ClangPchManager::PchManagerProjectUpdater pchUpdater{mockPchManagerServer,
                                                         pchManagerClient,
                                                         filePathCache,
                                                         projectPartsStorage};
    ClangBackEnd::RemoveProjectPartsMessage message{{projectPartId, projectPartId2}};

    EXPECT_CALL(mockPchManagerNotifier, precompiledHeaderRemoved(projectPartId));
    EXPECT_CALL(mockPchManagerNotifier, precompiledHeaderRemoved(projectPartId2));

    pchUpdater.removeProjectParts({projectPartId, projectPartId2});
}

TEST_F(ProjectUpdater, ConvertProjectPartToProjectPartContainer)
{
    updater.setExcludedPaths({"/path/to/header1.h"});

    auto container = updater.toProjectPartContainer(&projectPart);

    ASSERT_THAT(container, expectedContainer);
}

TEST_F(ProjectUpdater, ConvertProjectPartToProjectPartContainersHaveSameSizeLikeProjectParts)
{
    auto containers = updater.toProjectPartContainers(
        {&projectPart, &projectPart, &nonBuildingProjectPart});

    ASSERT_THAT(containers, SizeIs(2));
}

TEST_F(ProjectUpdater, CallStorageInsideTransaction)
{
    InSequence s;
    CppTools::ProjectPart projectPart;
    projectPart.displayName = "project";
    Utils::SmallString projectPartName = projectPart.id();
    MockSqliteTransactionBackend mockSqliteTransactionBackend;
    MockProjectPartsStorage mockProjectPartsStorage;
    ON_CALL(mockProjectPartsStorage, transactionBackend())
        .WillByDefault(ReturnRef(mockSqliteTransactionBackend));
    ClangPchManager::ProjectUpdater updater{mockPchManagerServer,
                                            filePathCache,
                                            mockProjectPartsStorage};

    EXPECT_CALL(mockProjectPartsStorage, fetchProjectPartId(Eq(projectPartName)));

    updater.toProjectPartContainers({&projectPart});
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
    auto paths = updater.createCompilerMacros({{"DEFINE", "1"}, {"FOO", "2"}, {"BAR", "1"}});

    ASSERT_THAT(paths, ElementsAre(CompilerMacro{"BAR", "1", 1},
                                   CompilerMacro{"FOO", "2", 2},
                                   CompilerMacro{"DEFINE", "1", 3}));
}

TEST_F(ProjectUpdater, CreateSortedIncludeSearchPaths)
{
    CppTools::ProjectPart projectPart;
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

    ASSERT_THAT(paths.system,
                ElementsAre(Eq(IncludeSearchPath{systemPath.path, 1, IncludeSearchPathType::System}),
                            Eq(IncludeSearchPath{builtInPath.path, 4, IncludeSearchPathType::BuiltIn}),
                            Eq(IncludeSearchPath{frameworkPath.path, 2, IncludeSearchPathType::Framework}),
                            Eq(IncludeSearchPath{CLANG_RESOURCE_DIR,
                                                 3,
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
                ElementsAre("-m32", "-fPIC", "--target=target", "extraflags", "-include", "config.h"));
}

TEST_F(ProjectUpdater, ToolChainArgumentsMSVC)
{
    projectPart.toolChainTargetTriple = "target";
    projectPart.extraCodeModelFlags.push_back("extraflags");
    projectPart.toolchainType = ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID;
    projectPart.isMsvc2015Toolchain = true;

    auto arguments = updater.toolChainArguments(&projectPart);

    ASSERT_THAT(arguments,
                ElementsAre("-m32",
                            "--target=target",
                            "extraflags",
                            "-U__clang__",
                            "-U__clang_major__",
                            "-U__clang_minor__",
                            "-U__clang_patchlevel__",
                            "-U__clang_version__",
                            "-U__cpp_aggregate_bases",
                            "-U__cpp_aggregate_nsdmi",
                            "-U__cpp_alias_templates",
                            "-U__cpp_aligned_new",
                            "-U__cpp_attributes",
                            "-U__cpp_binary_literals",
                            "-U__cpp_capture_star_this",
                            "-U__cpp_constexpr",
                            "-U__cpp_decltype",
                            "-U__cpp_decltype_auto",
                            "-U__cpp_deduction_guides",
                            "-U__cpp_delegating_constructors",
                            "-U__cpp_digit_separators",
                            "-U__cpp_enumerator_attributes",
                            "-U__cpp_exceptions",
                            "-U__cpp_fold_expressions",
                            "-U__cpp_generic_lambdas",
                            "-U__cpp_guaranteed_copy_elision",
                            "-U__cpp_hex_float",
                            "-U__cpp_if_constexpr",
                            "-U__cpp_inheriting_constructors",
                            "-U__cpp_init_captures",
                            "-U__cpp_initializer_lists",
                            "-U__cpp_inline_variables",
                            "-U__cpp_lambdas",
                            "-U__cpp_namespace_attributes",
                            "-U__cpp_nested_namespace_definitions",
                            "-U__cpp_noexcept_function_type",
                            "-U__cpp_nontype_template_args",
                            "-U__cpp_nontype_template_parameter_auto",
                            "-U__cpp_nsdmi",
                            "-U__cpp_range_based_for",
                            "-U__cpp_raw_strings",
                            "-U__cpp_ref_qualifiers",
                            "-U__cpp_return_type_deduction",
                            "-U__cpp_rtti",
                            "-U__cpp_rvalue_references",
                            "-U__cpp_static_assert",
                            "-U__cpp_structured_bindings",
                            "-U__cpp_template_auto",
                            "-U__cpp_threadsafe_static_init",
                            "-U__cpp_unicode_characters",
                            "-U__cpp_unicode_literals",
                            "-U__cpp_user_defined_literals",
                            "-U__cpp_variable_templates",
                            "-U__cpp_variadic_templates",
                            "-U__cpp_variadic_using"));
}

TEST_F(ProjectUpdater, FetchProjectPartName)
{
    updater.updateProjectParts({&projectPart}, {});

    auto projectPartName = updater.fetchProjectPartName(1);

    ASSERT_THAT(projectPartName, Eq(" projectb"));
}

// test for update many time and get the same id

} // namespace
