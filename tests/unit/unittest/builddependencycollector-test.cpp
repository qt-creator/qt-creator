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

#include <refactoringdatabaseinitializer.h>
#include <filepathcaching.h>
#include <builddependencycollector.h>

#include <sqlitedatabase.h>

#include <QDateTime>
#include <QDir>

using testing::AllOf;
using testing::Contains;
using testing::Not;
using testing::ElementsAre;
using testing::UnorderedElementsAre;

using ClangBackEnd::BuildDependency;
using ClangBackEnd::FilePathId;
using ClangBackEnd::FilePathIds;
using ClangBackEnd::FilePathView;
using ClangBackEnd::SourceDependency;
using ClangBackEnd::SourceType;
using ClangBackEnd::UsedMacro;

namespace {

MATCHER_P2(HasInclude, sourceId, sourceType,
           std::string(negation ? "hasn't " : "has ")
               + PrintToString(ClangBackEnd::SourceEntry(sourceId, sourceType, -1)))
{
    const ClangBackEnd::SourceEntry &entry = arg;

    return entry.sourceId == sourceId && entry.sourceType == sourceType;
}

class BuildDependencyCollector : public ::testing::Test
{
protected:
    BuildDependencyCollector()
    {
        setFilePathCache(&filePathCache);

        collector.addFile(id(TESTDATA_DIR "/builddependencycollector/project/main.cpp"), {"cc", "-I", TESTDATA_DIR "/builddependencycollector/external", "-I", TESTDATA_DIR "/builddependencycollector/project", "-isystem", TESTDATA_DIR "/builddependencycollector/system"});
        collector.addFile(id(TESTDATA_DIR "/builddependencycollector/project/main2.cpp"), {"cc", "-I", TESTDATA_DIR "/builddependencycollector/external", "-I", TESTDATA_DIR "/builddependencycollector/project", "-isystem", TESTDATA_DIR "/builddependencycollector/system"});

        collector.addUnsavedFiles({{{TESTDATA_DIR, "BuildDependencyCollector/project/generated_file.h"}, "#pragma once", {}}});

        collector.setExcludedFilePaths(Utils::clone(excludePaths));
        emptyCollector.setExcludedFilePaths(Utils::clone(excludePaths));
    }

    ~BuildDependencyCollector()
    {
        setFilePathCache(nullptr);
    }

    FilePathId id(const Utils::SmallStringView &path) const
    {
        return filePathCache.filePathId(FilePathView{path});
    }

    static off_t fileSize(Utils::SmallStringView filePath)
    {
        return QFileInfo(QString(filePath)).size();
    }

   static std::time_t lastModified(Utils::SmallStringView filePath)
    {
        return QFileInfo(QString(filePath)).lastModified().toTime_t();
    }

    ClangBackEnd::FileStatus fileStatus(Utils::SmallStringView filePath) const
    {
        return {id(filePath), fileSize(filePath), lastModified(filePath), false};
    }

    static
    FilePathIds filteredIncludes(const ClangBackEnd::SourceEntries &includes,
                                 ClangBackEnd::SourceType includeType)
    {
        FilePathIds filteredIncludes;

        for (const ClangBackEnd::SourceEntry &include : includes) {
            if (include.sourceType == includeType)
                filteredIncludes.push_back(include.sourceId);
        }

        return filteredIncludes;
    }

    static
    FilePathIds topIncludes(const ClangBackEnd::SourceEntries &includes)
    {
        return filteredIncludes(includes, ClangBackEnd::SourceType::TopInclude);
    }

    static
    FilePathIds systemTopIncludes(const ClangBackEnd::SourceEntries &includes)
    {
        return filteredIncludes(includes, ClangBackEnd::SourceType::TopSystemInclude);
    }

    static
    FilePathIds userIncludes(const ClangBackEnd::SourceEntries &includes)
    {
        return filteredIncludes(includes, ClangBackEnd::SourceType::UserInclude);
    }

    static
    FilePathIds allIncludes(const ClangBackEnd::SourceEntries &includes)
    {
        FilePathIds filteredIncludes;

        for (const ClangBackEnd::SourceEntry &include : includes)
                filteredIncludes.push_back(include.sourceId);

        return filteredIncludes;
    }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> databaseInitializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    ClangBackEnd::BuildDependencyCollector collector{filePathCache};
    ClangBackEnd::BuildDependencyCollector emptyCollector{filePathCache};
    ClangBackEnd::FilePaths excludePaths = {TESTDATA_DIR "/builddependencycollector/project/main.cpp",
                                            TESTDATA_DIR "/builddependencycollector/project/main2.cpp",
                                            TESTDATA_DIR "/builddependencycollector/project/header1.h",
                                            TESTDATA_DIR "/builddependencycollector/project/header2.h",
                                            TESTDATA_DIR "/builddependencycollector/project/generated_file.h"};
};

TEST_F(BuildDependencyCollector, IncludesExternalHeader)
{
    collector.collect();

    ASSERT_THAT(allIncludes(collector.includeIds()),
                AllOf(Contains(id(TESTDATA_DIR "/builddependencycollector/external/external1.h")),
                      Contains(id(TESTDATA_DIR "/builddependencycollector/external/external2.h")),
                      Contains(id(TESTDATA_DIR "/builddependencycollector/external/indirect_external.h")),
                      Contains(id(TESTDATA_DIR "/builddependencycollector/external/indirect_external2.h"))));
}

TEST_F(BuildDependencyCollector, InternalHeaderAreUserIncludes)
{
    collector.collect();

    ASSERT_THAT(userIncludes(collector.includeIds()), Contains(id(TESTDATA_DIR "/builddependencycollector/project/header1.h")));
}

TEST_F(BuildDependencyCollector, NoDuplicate)
{
    collector.collect();

    ASSERT_THAT(allIncludes(collector.includeIds()),
                UnorderedElementsAre(
                    id(TESTDATA_DIR "/builddependencycollector/project/header1.h"),
                    id(TESTDATA_DIR "/builddependencycollector/project/header2.h"),
                    id(TESTDATA_DIR "/builddependencycollector/external/external1.h"),
                    id(TESTDATA_DIR "/builddependencycollector/external/external2.h"),
                    id(TESTDATA_DIR "/builddependencycollector/external/external3.h"),
                    id(TESTDATA_DIR "/builddependencycollector/external/indirect_external.h"),
                    id(TESTDATA_DIR "/builddependencycollector/external/indirect_external2.h")));
}

TEST_F(BuildDependencyCollector, IncludesAreSorted)
{
    collector.collect();

    ASSERT_THAT(allIncludes(collector.includeIds()),
                ElementsAre(
                    id(TESTDATA_DIR "/builddependencycollector/project/header1.h"),
                    id(TESTDATA_DIR "/builddependencycollector/project/header2.h"),
                    id(TESTDATA_DIR "/builddependencycollector/external/external3.h"),
                    id(TESTDATA_DIR "/builddependencycollector/external/external1.h"),
                    id(TESTDATA_DIR "/builddependencycollector/external/indirect_external.h"),
                    id(TESTDATA_DIR "/builddependencycollector/external/indirect_external2.h"),
                    id(TESTDATA_DIR "/builddependencycollector/external/external2.h")));
}

TEST_F(BuildDependencyCollector, If)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/builddependencycollector/project/if.cpp"),  {"cc", "-I", TESTDATA_DIR "/builddependencycollector/external", "-I", TESTDATA_DIR "/builddependencycollector/project", "-isystem", TESTDATA_DIR "/builddependencycollector/system"});

    emptyCollector.collect();

    ASSERT_THAT(allIncludes(emptyCollector.includeIds()),
                ElementsAre(id(TESTDATA_DIR "/builddependencycollector/project/true.h")));
}

TEST_F(BuildDependencyCollector, LocalPath)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/builddependencycollector/project/main.cpp"),  {"cc", "-I", TESTDATA_DIR "/builddependencycollector/external", "-I", TESTDATA_DIR "/builddependencycollector/project", "-isystem", TESTDATA_DIR "/builddependencycollector/system"});

    emptyCollector.collect();

    ASSERT_THAT(allIncludes(emptyCollector.includeIds()),
                UnorderedElementsAre(
                    id(TESTDATA_DIR "/builddependencycollector/project/header1.h"),
                    id(TESTDATA_DIR "/builddependencycollector/project/header2.h"),
                    id(TESTDATA_DIR "/builddependencycollector/external/external1.h"),
                    id(TESTDATA_DIR "/builddependencycollector/external/external2.h"),
                    id(TESTDATA_DIR "/builddependencycollector/external/external3.h"),
                    id(TESTDATA_DIR "/builddependencycollector/external/indirect_external.h"),
                    id(TESTDATA_DIR "/builddependencycollector/external/indirect_external2.h")));
}

TEST_F(BuildDependencyCollector, IgnoreMissingFile)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/builddependencycollector/project/missingfile.cpp"),  {"cc", "-I", TESTDATA_DIR "/builddependencycollector/external", "-I", TESTDATA_DIR "/builddependencycollector/project", "-isystem", TESTDATA_DIR "/builddependencycollector/system"});

    emptyCollector.collect();

    ASSERT_THAT(allIncludes(emptyCollector.includeIds()),
                UnorderedElementsAre(id(TESTDATA_DIR "/builddependencycollector/external/external1.h"),
                                     id(TESTDATA_DIR "/builddependencycollector/external/indirect_external.h"),
                                     id(TESTDATA_DIR "/builddependencycollector/external/indirect_external2.h")));
}

TEST_F(BuildDependencyCollector, IncludesOnlyTopExternalHeader)
{
    collector.collect();

    ASSERT_THAT(topIncludes(collector.includeIds()),
                UnorderedElementsAre(id(TESTDATA_DIR "/builddependencycollector/external/external1.h"),
                                     id(TESTDATA_DIR "/builddependencycollector/external/external2.h"),
                                     id(TESTDATA_DIR "/builddependencycollector/external/external3.h")));
}

TEST_F(BuildDependencyCollector, TopIncludeInIfMacro)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/builddependencycollector/project/if.cpp"),  {"cc", "-I", TESTDATA_DIR "/builddependencycollector/external", "-I", TESTDATA_DIR "/builddependencycollector/project", "-isystem", TESTDATA_DIR "/builddependencycollector/system"});
    emptyCollector.setExcludedFilePaths({TESTDATA_DIR "/builddependencycollector/project/if.cpp"});

    emptyCollector.collect();

    ASSERT_THAT(topIncludes(emptyCollector.includeIds()),
                ElementsAre(id(TESTDATA_DIR "/builddependencycollector/project/true.h")));
}

TEST_F(BuildDependencyCollector, TopIncludeWithLocalPath)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/builddependencycollector/project/main.cpp"),  {"cc", "-I", TESTDATA_DIR "/builddependencycollector/external", "-I", TESTDATA_DIR "/builddependencycollector/project", "-isystem", TESTDATA_DIR "/builddependencycollector/system"});

    emptyCollector.collect();

    ASSERT_THAT(topIncludes(emptyCollector.includeIds()),
                UnorderedElementsAre(id(TESTDATA_DIR "/builddependencycollector/external/external1.h"),
                                     id(TESTDATA_DIR "/builddependencycollector/external/external2.h"),
                                     id(TESTDATA_DIR "/builddependencycollector/external/external3.h")));
}

TEST_F(BuildDependencyCollector, TopIncludesIgnoreMissingFile)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/builddependencycollector/project/missingfile.cpp"),  {"cc", "-I", TESTDATA_DIR "/builddependencycollector/external", "-I", TESTDATA_DIR "/builddependencycollector/project", "-isystem", TESTDATA_DIR "/builddependencycollector/system"});
    emptyCollector.setExcludedFilePaths({TESTDATA_DIR "/builddependencycollector/project/missingfile.cpp"});

    emptyCollector.collect();

    ASSERT_THAT(topIncludes(emptyCollector.includeIds()),
                UnorderedElementsAre(id(TESTDATA_DIR "/builddependencycollector/external/external1.h")));
}

TEST_F(BuildDependencyCollector, SourceFiles)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc", "-I", TESTDATA_DIR "/builddependencycollector/external", "-I", TESTDATA_DIR "/builddependencycollector/project", "-isystem", TESTDATA_DIR "/builddependencycollector/system"});

    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.sourceFiles(),
                UnorderedElementsAre(id(TESTDATA_DIR "/symbolscollector_main.cpp"),
                                     id(TESTDATA_DIR "/symbolscollector_header1.h"),
                                     id(TESTDATA_DIR "/symbolscollector_header2.h")));
}

TEST_F(BuildDependencyCollector, MainFileInSourceFiles)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc", "-I", TESTDATA_DIR "/builddependencycollector/external", "-I", TESTDATA_DIR "/builddependencycollector/project", "-isystem", TESTDATA_DIR "/builddependencycollector/system"});

    ASSERT_THAT(emptyCollector.sourceFiles(),
                ElementsAre(id(TESTDATA_DIR "/symbolscollector_main.cpp")));
}

TEST_F(BuildDependencyCollector, ResetMainFileInSourceFiles)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc", "-I", TESTDATA_DIR "/builddependencycollector/external", "-I", TESTDATA_DIR "/builddependencycollector/project", "-isystem", TESTDATA_DIR "/builddependencycollector/system"});

    ASSERT_THAT(emptyCollector.sourceFiles(),
                ElementsAre(id(TESTDATA_DIR "/symbolscollector_main.cpp")));
}

TEST_F(BuildDependencyCollector, DontDuplicateSourceFiles)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc", "-I", TESTDATA_DIR "/builddependencycollector/external", "-I", TESTDATA_DIR "/builddependencycollector/project", "-isystem", TESTDATA_DIR "/builddependencycollector/system"});
    emptyCollector.collect();

    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.sourceFiles(),
                UnorderedElementsAre(id(TESTDATA_DIR "/symbolscollector_main.cpp"),
                                     id(TESTDATA_DIR "/symbolscollector_header1.h"),
                                     id(TESTDATA_DIR "/symbolscollector_header2.h")));
}

TEST_F(BuildDependencyCollector, ClearSourceFiles)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc", "-I", TESTDATA_DIR "/builddependencycollector/external", "-I", TESTDATA_DIR "/builddependencycollector/project", "-isystem", TESTDATA_DIR "/builddependencycollector/system"});

    emptyCollector.clear();

    ASSERT_THAT(emptyCollector.sourceFiles(), IsEmpty());
}

TEST_F(BuildDependencyCollector, ClearFileStatus)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc", "-I", TESTDATA_DIR "/builddependencycollector/external", "-I", TESTDATA_DIR "/builddependencycollector/project", "-isystem", TESTDATA_DIR "/builddependencycollector/system"});
    emptyCollector.collect();

    emptyCollector.clear();

    ASSERT_THAT(emptyCollector.fileStatuses(), IsEmpty());
}

TEST_F(BuildDependencyCollector, ClearUsedMacros)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_defines.h"), {"cc", "-I", TESTDATA_DIR "/builddependencycollector/external", "-I", TESTDATA_DIR "/builddependencycollector/project", "-isystem", TESTDATA_DIR "/builddependencycollector/system"});
    emptyCollector.collect();

    emptyCollector.clear();

    ASSERT_THAT(emptyCollector.usedMacros(), IsEmpty());
}

TEST_F(BuildDependencyCollector, ClearSourceDependencies)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main2.cpp"), {"cc", "-I" TESTDATA_DIR});
    emptyCollector.collect();

    emptyCollector.clear();

    ASSERT_THAT(emptyCollector.sourceDependencies(), IsEmpty());
}

TEST_F(BuildDependencyCollector, DontCollectSourceFilesAfterFilesAreCleared)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc", "-I", TESTDATA_DIR "/builddependencycollector/external", "-I", TESTDATA_DIR "/builddependencycollector/project", "-isystem", TESTDATA_DIR "/builddependencycollector/system"});

    emptyCollector.clear();
    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.sourceFiles(), IsEmpty());
}

TEST_F(BuildDependencyCollector, DontCollectFileStatusAfterFilesAreCleared)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc", "-I", TESTDATA_DIR "/builddependencycollector/external", "-I", TESTDATA_DIR "/builddependencycollector/project", "-isystem", TESTDATA_DIR "/builddependencycollector/system"});

    emptyCollector.clear();
    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.fileStatuses(), IsEmpty());
}

TEST_F(BuildDependencyCollector, DontCollectUsedMacrosAfterFilesAreCleared)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc", "-I", TESTDATA_DIR "/builddependencycollector/external", "-I", TESTDATA_DIR "/builddependencycollector/project", "-isystem", TESTDATA_DIR "/builddependencycollector/system"});

    emptyCollector.clear();
    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.usedMacros(), IsEmpty());
}


TEST_F(BuildDependencyCollector, DontCollectSourceDependenciesAfterFilesAreCleared)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc", "-I", TESTDATA_DIR "/builddependencycollector/external", "-I", TESTDATA_DIR "/builddependencycollector/project", "-isystem", TESTDATA_DIR "/builddependencycollector/system"});

    emptyCollector.clear();
    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.sourceDependencies(), IsEmpty());
}

TEST_F(BuildDependencyCollector, CollectUsedMacrosWithExternalDefine)
{
    auto fileId = id(TESTDATA_DIR "/symbolscollector_defines.h");
    emptyCollector.addFile(fileId, {"cc", "-DCOMPILER_ARGUMENT"});

    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.usedMacros(),
                ElementsAre(Eq(UsedMacro{"DEFINED", fileId}),
                            Eq(UsedMacro{"IF_DEFINE", fileId}),
                            Eq(UsedMacro{"__clang__", fileId}),
                            Eq(UsedMacro{"CLASS_EXPORT", fileId}),
                            Eq(UsedMacro{"IF_NOT_DEFINE", fileId}),
                            Eq(UsedMacro{"MACRO_EXPANSION", fileId}),
                            Eq(UsedMacro{"COMPILER_ARGUMENT", fileId})));
}

TEST_F(BuildDependencyCollector, CollectUsedMacrosWithoutExternalDefine)
{
    auto fileId = id(TESTDATA_DIR "/symbolscollector_defines.h");
    emptyCollector.addFile(fileId, {"cc", "-I", TESTDATA_DIR "/builddependencycollector/external", "-I", TESTDATA_DIR "/builddependencycollector/project", "-isystem", TESTDATA_DIR "/builddependencycollector/system"});

    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.usedMacros(),
                ElementsAre(Eq(UsedMacro{"DEFINED", fileId}),
                            Eq(UsedMacro{"IF_DEFINE", fileId}),
                            Eq(UsedMacro{"__clang__", fileId}),
                            Eq(UsedMacro{"CLASS_EXPORT", fileId}),
                            Eq(UsedMacro{"IF_NOT_DEFINE", fileId}),
                            Eq(UsedMacro{"MACRO_EXPANSION", fileId}),
                            Eq(UsedMacro{"COMPILER_ARGUMENT", fileId})));
}

TEST_F(BuildDependencyCollector, DontCollectHeaderGuards)
{
    auto fileId = id(TESTDATA_DIR "/symbolscollector_defines.h");
    emptyCollector.addFile(fileId, {"cc", "-I", TESTDATA_DIR "/builddependencycollector/external", "-I", TESTDATA_DIR "/builddependencycollector/project", "-isystem", TESTDATA_DIR "/builddependencycollector/system"});

    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.usedMacros(),
                Not(Contains(Eq(UsedMacro{"SYMBOLSCOLLECTOR_DEFINES_H", fileId}))));
}

TEST_F(BuildDependencyCollector, DISABLED_DontCollectDynamicLibraryExports)
{
    auto fileId = id(TESTDATA_DIR "/symbolscollector_defines.h");
    emptyCollector.addFile(fileId, {"cc", "-I", TESTDATA_DIR "/builddependencycollector/external", "-I", TESTDATA_DIR "/builddependencycollector/project", "-isystem", TESTDATA_DIR "/builddependencycollector/system"});

    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.usedMacros(),
                Not(Contains(Eq(UsedMacro{"CLASS_EXPORT", fileId}))));
}

TEST_F(BuildDependencyCollector, CollectFileStatuses)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc", "-I", TESTDATA_DIR "/builddependencycollector/external", "-I", TESTDATA_DIR "/builddependencycollector/project", "-isystem", TESTDATA_DIR "/builddependencycollector/system"});

    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.fileStatuses(),
                ElementsAre(
                    fileStatus(TESTDATA_DIR "/symbolscollector_main.cpp"),
                    fileStatus(TESTDATA_DIR "/symbolscollector_header1.h"),
                    fileStatus(TESTDATA_DIR "/symbolscollector_header2.h")));
}

TEST_F(BuildDependencyCollector, CollectSourceDependencies)
{
    auto mainFileId = id(TESTDATA_DIR "/symbolscollector_main2.cpp");
    auto header1FileId = id(TESTDATA_DIR "/symbolscollector_header1.h");
    auto header2FileId = id(TESTDATA_DIR "/symbolscollector_header2.h");
    auto header3FileId = id(TESTDATA_DIR "/symbolscollector_header3.h");
    emptyCollector.addFile(mainFileId, {"cc", "-I" TESTDATA_DIR});

    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.sourceDependencies(),
                UnorderedElementsAre(SourceDependency(mainFileId, header1FileId),
                                     SourceDependency(mainFileId, header3FileId),
                                     SourceDependency(header3FileId, header2FileId),
                                     SourceDependency(header1FileId, header2FileId)));
}

TEST_F(BuildDependencyCollector, MissingInclude)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/builddependencycollector/project/main5.cpp"),
                           {"cc",
                            "-I",
                            TESTDATA_DIR "/builddependencycollector/external",
                            "-I",
                            TESTDATA_DIR "/builddependencycollector/project",
                            "-isystem",
                            TESTDATA_DIR "/builddependencycollector/system"});

    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.includeIds(),
                ElementsAre(
                    HasInclude(id(TESTDATA_DIR "/builddependencycollector/project/header1.h"),
                               SourceType::UserInclude)));
}

TEST_F(BuildDependencyCollector, Create)
{
    ClangBackEnd::BuildDependencyCollector collector{filePathCache};
    ClangBackEnd::V2::ProjectPartContainer
        projectPart{"project1",
                    {"cc",
                     "-I",
                     TESTDATA_DIR "/builddependencycollector/external",
                     "-I",
                     TESTDATA_DIR "/builddependencycollector/project",
                     "-isystem",
                     TESTDATA_DIR "/builddependencycollector/system"},
                    {},
                    {},
                    {
                        id(TESTDATA_DIR "/builddependencycollector/project/header1.h"),
                        id(TESTDATA_DIR "/builddependencycollector/project/header2.h"),
                        id(TESTDATA_DIR "/builddependencycollector/project/missingfile.h"),
                        id(TESTDATA_DIR "/builddependencycollector/project/macros.h"),
                    },
                    {id(TESTDATA_DIR "/builddependencycollector/project/main4.cpp")}};

    auto buildDependency = collector.create(projectPart);

    ASSERT_THAT(
        buildDependency,
        AllOf(
            Field(&BuildDependency::fileStatuses,
                  UnorderedElementsAre(
                      fileStatus(TESTDATA_DIR "/builddependencycollector/project/main4.cpp"),
                      fileStatus(TESTDATA_DIR "/builddependencycollector/project/header1.h"),
                      fileStatus(TESTDATA_DIR "/builddependencycollector/project/header2.h"),
                      fileStatus(TESTDATA_DIR "/builddependencycollector/external/external3.h"),
                      fileStatus(TESTDATA_DIR "/builddependencycollector/external/external1.h"),
                      fileStatus(TESTDATA_DIR
                                 "/builddependencycollector/external/indirect_external.h"),
                      fileStatus(TESTDATA_DIR
                                 "/builddependencycollector/external/indirect_external2.h"),
                      fileStatus(TESTDATA_DIR "/builddependencycollector/external/external2.h"),
                      fileStatus(TESTDATA_DIR "/builddependencycollector/system/system1.h"),
                      fileStatus(TESTDATA_DIR "/builddependencycollector/system/indirect_system.h"),
                      fileStatus(TESTDATA_DIR "/builddependencycollector/system/indirect_system2.h"),
                      fileStatus(TESTDATA_DIR "/builddependencycollector/project/missingfile.h"),
                      fileStatus(TESTDATA_DIR "/builddependencycollector/project/macros.h"))),
            Field(&BuildDependency::includes,
                  UnorderedElementsAre(
                      HasInclude(id(TESTDATA_DIR "/builddependencycollector/project/header1.h"),
                                 SourceType::UserInclude),
                      HasInclude(id(TESTDATA_DIR "/builddependencycollector/project/header2.h"),
                                 SourceType::UserInclude),
                      HasInclude(id(TESTDATA_DIR "/builddependencycollector/external/external3.h"),
                                 SourceType::TopInclude),
                      HasInclude(id(TESTDATA_DIR "/builddependencycollector/external/external1.h"),
                                 SourceType::TopInclude),
                      HasInclude(id(TESTDATA_DIR
                                    "/builddependencycollector/external/indirect_external.h"),
                                 SourceType::UserInclude),
                      HasInclude(id(TESTDATA_DIR
                                    "/builddependencycollector/external/indirect_external2.h"),
                                 SourceType::UserInclude),
                      HasInclude(id(TESTDATA_DIR "/builddependencycollector/external/external2.h"),
                                 SourceType::TopInclude),
                      HasInclude(id(TESTDATA_DIR "/builddependencycollector/system/system1.h"),
                                 SourceType::TopSystemInclude),
                      HasInclude(id(TESTDATA_DIR
                                    "/builddependencycollector/system/indirect_system.h"),
                                 SourceType::SystemInclude),
                      HasInclude(id(TESTDATA_DIR
                                    "/builddependencycollector/system/indirect_system2.h"),
                                 SourceType::SystemInclude),
                      HasInclude(id(TESTDATA_DIR "/builddependencycollector/project/macros.h"),
                                 SourceType::UserInclude))),
            Field(&BuildDependency::usedMacros,
                  UnorderedElementsAre(
                      UsedMacro{"DEFINE",
                                id(TESTDATA_DIR "/builddependencycollector/project/macros.h")},
                      UsedMacro{"IFDEF",
                                id(TESTDATA_DIR "/builddependencycollector/project/macros.h")},
                      UsedMacro{"DEFINED",
                                id(TESTDATA_DIR "/builddependencycollector/project/macros.h")})),
            Field(&BuildDependency::sourceFiles,
                  UnorderedElementsAre(
                      id(TESTDATA_DIR "/builddependencycollector/project/main4.cpp"),
                      id(TESTDATA_DIR "/builddependencycollector/project/header1.h"),
                      id(TESTDATA_DIR "/builddependencycollector/project/header2.h"),
                      id(TESTDATA_DIR "/builddependencycollector/external/external3.h"),
                      id(TESTDATA_DIR "/builddependencycollector/external/external1.h"),
                      id(TESTDATA_DIR "/builddependencycollector/external/indirect_external.h"),
                      id(TESTDATA_DIR "/builddependencycollector/external/indirect_external2.h"),
                      id(TESTDATA_DIR "/builddependencycollector/external/external2.h"),
                      id(TESTDATA_DIR "/builddependencycollector/system/system1.h"),
                      id(TESTDATA_DIR "/builddependencycollector/system/indirect_system.h"),
                      id(TESTDATA_DIR "/builddependencycollector/system/indirect_system2.h"),
                      id(TESTDATA_DIR "/builddependencycollector/project/missingfile.h"),
                      id(TESTDATA_DIR "/builddependencycollector/project/macros.h"))),
            Field(
                &BuildDependency::sourceDependencies,
                UnorderedElementsAre(
                    SourceDependency(id(TESTDATA_DIR "/builddependencycollector/project/main4.cpp"),
                                     id(TESTDATA_DIR "/builddependencycollector/project/header1.h")),
                    SourceDependency(id(TESTDATA_DIR "/builddependencycollector/project/main4.cpp"),
                                     id(TESTDATA_DIR "/builddependencycollector/project/header2.h")),
                    SourceDependency(id(TESTDATA_DIR "/builddependencycollector/project/main4.cpp"),
                                     id(TESTDATA_DIR
                                        "/builddependencycollector/project/missingfile.h")),
                    SourceDependency(id(TESTDATA_DIR "/builddependencycollector/project/main4.cpp"),
                                     id(TESTDATA_DIR
                                        "/builddependencycollector/external/external1.h")),
                    SourceDependency(id(TESTDATA_DIR "/builddependencycollector/project/main4.cpp"),
                                     id(TESTDATA_DIR
                                        "/builddependencycollector/external/external2.h")),
                    SourceDependency(id(TESTDATA_DIR "/builddependencycollector/project/main4.cpp"),
                                     id(TESTDATA_DIR "/builddependencycollector/system/system1.h")),
                    SourceDependency(id(TESTDATA_DIR "/builddependencycollector/project/main4.cpp"),
                                     id(TESTDATA_DIR "/builddependencycollector/project/macros.h")),
                    SourceDependency(id(TESTDATA_DIR "/builddependencycollector/project/header2.h"),
                                     id(TESTDATA_DIR
                                        "/builddependencycollector/external/external3.h")),
                    SourceDependency(id(TESTDATA_DIR
                                        "/builddependencycollector/project/missingfile.h"),
                                     id(TESTDATA_DIR
                                        "/builddependencycollector/external/external1.h")),
                    SourceDependency(id(TESTDATA_DIR
                                        "/builddependencycollector/external/external1.h"),
                                     id(TESTDATA_DIR
                                        "/builddependencycollector/external/indirect_external.h")),
                    SourceDependency(id(TESTDATA_DIR
                                        "/builddependencycollector/external/indirect_external.h"),
                                     id(TESTDATA_DIR
                                        "/builddependencycollector/external/indirect_external2.h")),
                    SourceDependency(id(TESTDATA_DIR "/builddependencycollector/system/system1.h"),
                                     id(TESTDATA_DIR
                                        "/builddependencycollector/system/indirect_system.h")),
                    SourceDependency(id(TESTDATA_DIR
                                        "/builddependencycollector/system/indirect_system.h"),
                                     id(TESTDATA_DIR
                                        "/builddependencycollector/system/indirect_system2.h"))))));
}
} // namespace
