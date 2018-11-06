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
#include <includecollector.h>

#include <sqlitedatabase.h>

#include <QDateTime>
#include <QDir>

using testing::AllOf;
using testing::Contains;
using testing::Not;
using testing::ElementsAre;
using testing::UnorderedElementsAre;

using ClangBackEnd::FilePathId;
using ClangBackEnd::FilePathIds;
using ClangBackEnd::FilePathView;
using ClangBackEnd::SourceDependency;
using ClangBackEnd::UsedMacro;

namespace {

class IncludeCollector : public ::testing::Test
{
protected:
    void SetUp()
    {
        collector.addFile(id(TESTDATA_DIR "/includecollector/project/main.cpp"), {"cc", "-I", TESTDATA_DIR "/includecollector/external", "-I", TESTDATA_DIR "/includecollector/project", "-isystem", TESTDATA_DIR "/includecollector/system"});
        collector.addFile(id(TESTDATA_DIR "/includecollector/project/main2.cpp"), {"cc", "-I", TESTDATA_DIR "/includecollector/external", "-I", TESTDATA_DIR "/includecollector/project", "-isystem", TESTDATA_DIR "/includecollector/system"});

        collector.addUnsavedFiles({{{TESTDATA_DIR, "includecollector/project/generated_file.h"}, "#pragma once", {}}});

        collector.setExcludedIncludes(excludePaths.clone());
        emptyCollector.setExcludedIncludes(excludePaths.clone());
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
protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> databaseInitializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    ClangBackEnd::IncludeCollector collector{filePathCache};
    ClangBackEnd::IncludeCollector emptyCollector{filePathCache};
    Utils::PathStringVector excludePaths = {TESTDATA_DIR "/includecollector/project/main.cpp",
                                            TESTDATA_DIR "/includecollector/project/main2.cpp",
                                            TESTDATA_DIR "/includecollector/project/header1.h",
                                            TESTDATA_DIR "/includecollector/project/header2.h",
                                            TESTDATA_DIR "/includecollector/project/generated_file.h"};
};

TEST_F(IncludeCollector, IncludesExternalHeader)
{
    collector.collect();

    ASSERT_THAT(collector.takeIncludeIds(),
                AllOf(Contains(id(TESTDATA_DIR "/includecollector/external/external1.h")),
                      Contains(id(TESTDATA_DIR "/includecollector/external/external2.h")),
                      Contains(id(TESTDATA_DIR "/includecollector/external/indirect_external.h")),
                      Contains(id(TESTDATA_DIR "/includecollector/external/indirect_external2.h"))));
}

TEST_F(IncludeCollector, DoesNotIncludesInternalHeader)
{
    collector.collect();

    ASSERT_THAT(collector.takeIncludeIds(), Not(Contains(id(TESTDATA_DIR "/includecollector/project/header1.h"))));
}

TEST_F(IncludeCollector, NoDuplicate)
{
    collector.collect();

    ASSERT_THAT(collector.takeIncludeIds(),
                UnorderedElementsAre(id(TESTDATA_DIR "/includecollector/external/external1.h"),
                                     id(TESTDATA_DIR "/includecollector/external/external2.h"),
                                     id(TESTDATA_DIR "/includecollector/external/external3.h"),
                                     id(TESTDATA_DIR "/includecollector/external/indirect_external.h"),
                                     id(TESTDATA_DIR "/includecollector/external/indirect_external2.h")));
}

TEST_F(IncludeCollector, IncludesAreSorted)
{
    collector.collect();

    ASSERT_THAT(collector.takeIncludeIds(),
                SizeIs(5));
}

TEST_F(IncludeCollector, If)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/includecollector/project/if.cpp"),  {"cc", "-I", TESTDATA_DIR "/includecollector/external", "-I", TESTDATA_DIR "/includecollector/project", "-isystem", TESTDATA_DIR "/includecollector/system"});

    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.takeIncludeIds(),
                ElementsAre(id(TESTDATA_DIR "/includecollector/project/true.h")));
}

TEST_F(IncludeCollector, LocalPath)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/includecollector/project/main.cpp"),  {"cc", "-I", TESTDATA_DIR "/includecollector/external", "-I", TESTDATA_DIR "/includecollector/project", "-isystem", TESTDATA_DIR "/includecollector/system"});

    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.takeIncludeIds(),
                UnorderedElementsAre(id(TESTDATA_DIR "/includecollector/external/external1.h"),
                                     id(TESTDATA_DIR "/includecollector/external/external2.h"),
                                     id(TESTDATA_DIR "/includecollector/external/external3.h"),
                                     id(TESTDATA_DIR "/includecollector/external/indirect_external.h"),
                                     id(TESTDATA_DIR "/includecollector/external/indirect_external2.h")));
}

TEST_F(IncludeCollector, IgnoreMissingFile)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/includecollector/project/missingfile.cpp"),  {"cc", "-I", TESTDATA_DIR "/includecollector/external", "-I", TESTDATA_DIR "/includecollector/project", "-isystem", TESTDATA_DIR "/includecollector/system"});

    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.takeIncludeIds(),
                UnorderedElementsAre(id(TESTDATA_DIR "/includecollector/external/external1.h"),
                                     id(TESTDATA_DIR "/includecollector/external/indirect_external.h"),
                                     id(TESTDATA_DIR "/includecollector/external/indirect_external2.h")));
}

TEST_F(IncludeCollector, IncludesOnlyTopExternalHeader)
{
    collector.collect();

    ASSERT_THAT(collector.takeTopIncludeIds(),
                UnorderedElementsAre(id(TESTDATA_DIR "/includecollector/external/external1.h"),
                                     id(TESTDATA_DIR "/includecollector/external/external2.h"),
                                     id(TESTDATA_DIR "/includecollector/external/external3.h")));
}

TEST_F(IncludeCollector, TopIncludeInIfMacro)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/includecollector/project/if.cpp"),  {"cc", "-I", TESTDATA_DIR "/includecollector/external", "-I", TESTDATA_DIR "/includecollector/project", "-isystem", TESTDATA_DIR "/includecollector/system"});
    emptyCollector.setExcludedIncludes({TESTDATA_DIR "/includecollector/project/if.cpp"});

    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.takeTopIncludeIds(),
                ElementsAre(id(TESTDATA_DIR "/includecollector/project/true.h")));
}

TEST_F(IncludeCollector, TopIncludeWithLocalPath)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/includecollector/project/main.cpp"),  {"cc", "-I", TESTDATA_DIR "/includecollector/external", "-I", TESTDATA_DIR "/includecollector/project", "-isystem", TESTDATA_DIR "/includecollector/system"});

    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.takeTopIncludeIds(),
                UnorderedElementsAre(id(TESTDATA_DIR "/includecollector/external/external1.h"),
                                     id(TESTDATA_DIR "/includecollector/external/external2.h"),
                                     id(TESTDATA_DIR "/includecollector/external/external3.h")));
}

TEST_F(IncludeCollector, TopIncludesIgnoreMissingFile)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/includecollector/project/missingfile.cpp"),  {"cc", "-I", TESTDATA_DIR "/includecollector/external", "-I", TESTDATA_DIR "/includecollector/project", "-isystem", TESTDATA_DIR "/includecollector/system"});
    emptyCollector.setExcludedIncludes({TESTDATA_DIR "/includecollector/project/missingfile.cpp"});

    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.takeTopIncludeIds(),
                UnorderedElementsAre(id(TESTDATA_DIR "/includecollector/external/external1.h")));
}

TEST_F(IncludeCollector, SourceFiles)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc", "-I", TESTDATA_DIR "/includecollector/external", "-I", TESTDATA_DIR "/includecollector/project", "-isystem", TESTDATA_DIR "/includecollector/system"});

    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.sourceFiles(),
                UnorderedElementsAre(id(TESTDATA_DIR "/symbolscollector_main.cpp"),
                                     id(TESTDATA_DIR "/symbolscollector_header1.h"),
                                     id(TESTDATA_DIR "/symbolscollector_header2.h")));
}

TEST_F(IncludeCollector, MainFileInSourceFiles)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc", "-I", TESTDATA_DIR "/includecollector/external", "-I", TESTDATA_DIR "/includecollector/project", "-isystem", TESTDATA_DIR "/includecollector/system"});

    ASSERT_THAT(emptyCollector.sourceFiles(),
                ElementsAre(id(TESTDATA_DIR "/symbolscollector_main.cpp")));
}

TEST_F(IncludeCollector, ResetMainFileInSourceFiles)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc", "-I", TESTDATA_DIR "/includecollector/external", "-I", TESTDATA_DIR "/includecollector/project", "-isystem", TESTDATA_DIR "/includecollector/system"});

    ASSERT_THAT(emptyCollector.sourceFiles(),
                ElementsAre(id(TESTDATA_DIR "/symbolscollector_main.cpp")));
}

TEST_F(IncludeCollector, DontDuplicateSourceFiles)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc", "-I", TESTDATA_DIR "/includecollector/external", "-I", TESTDATA_DIR "/includecollector/project", "-isystem", TESTDATA_DIR "/includecollector/system"});
    emptyCollector.collect();

    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.sourceFiles(),
                UnorderedElementsAre(id(TESTDATA_DIR "/symbolscollector_main.cpp"),
                                     id(TESTDATA_DIR "/symbolscollector_header1.h"),
                                     id(TESTDATA_DIR "/symbolscollector_header2.h")));
}

TEST_F(IncludeCollector, ClearSourceFiles)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc", "-I", TESTDATA_DIR "/includecollector/external", "-I", TESTDATA_DIR "/includecollector/project", "-isystem", TESTDATA_DIR "/includecollector/system"});

    emptyCollector.clear();

    ASSERT_THAT(emptyCollector.sourceFiles(), IsEmpty());
}

TEST_F(IncludeCollector, ClearFileStatus)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc", "-I", TESTDATA_DIR "/includecollector/external", "-I", TESTDATA_DIR "/includecollector/project", "-isystem", TESTDATA_DIR "/includecollector/system"});
    emptyCollector.collect();

    emptyCollector.clear();

    ASSERT_THAT(emptyCollector.fileStatuses(), IsEmpty());
}

TEST_F(IncludeCollector, ClearUsedMacros)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_defines.h"), {"cc", "-I", TESTDATA_DIR "/includecollector/external", "-I", TESTDATA_DIR "/includecollector/project", "-isystem", TESTDATA_DIR "/includecollector/system"});
    emptyCollector.collect();

    emptyCollector.clear();

    ASSERT_THAT(emptyCollector.usedMacros(), IsEmpty());
}

TEST_F(IncludeCollector, ClearSourceDependencies)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main2.cpp"), {"cc", "-I" TESTDATA_DIR});
    emptyCollector.collect();

    emptyCollector.clear();

    ASSERT_THAT(emptyCollector.sourceDependencies(), IsEmpty());
}

TEST_F(IncludeCollector, DontCollectSourceFilesAfterFilesAreCleared)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc", "-I", TESTDATA_DIR "/includecollector/external", "-I", TESTDATA_DIR "/includecollector/project", "-isystem", TESTDATA_DIR "/includecollector/system"});

    emptyCollector.clear();
    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.sourceFiles(), IsEmpty());
}

TEST_F(IncludeCollector, DontCollectFileStatusAfterFilesAreCleared)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc", "-I", TESTDATA_DIR "/includecollector/external", "-I", TESTDATA_DIR "/includecollector/project", "-isystem", TESTDATA_DIR "/includecollector/system"});

    emptyCollector.clear();
    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.fileStatuses(), IsEmpty());
}

TEST_F(IncludeCollector, DontCollectUsedMacrosAfterFilesAreCleared)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc", "-I", TESTDATA_DIR "/includecollector/external", "-I", TESTDATA_DIR "/includecollector/project", "-isystem", TESTDATA_DIR "/includecollector/system"});

    emptyCollector.clear();
    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.usedMacros(), IsEmpty());
}


TEST_F(IncludeCollector, DontCollectSourceDependenciesAfterFilesAreCleared)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc", "-I", TESTDATA_DIR "/includecollector/external", "-I", TESTDATA_DIR "/includecollector/project", "-isystem", TESTDATA_DIR "/includecollector/system"});

    emptyCollector.clear();
    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.sourceDependencies(), IsEmpty());
}

TEST_F(IncludeCollector, CollectUsedMacrosWithExternalDefine)
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

TEST_F(IncludeCollector, CollectUsedMacrosWithoutExternalDefine)
{
    auto fileId = id(TESTDATA_DIR "/symbolscollector_defines.h");
    emptyCollector.addFile(fileId, {"cc", "-I", TESTDATA_DIR "/includecollector/external", "-I", TESTDATA_DIR "/includecollector/project", "-isystem", TESTDATA_DIR "/includecollector/system"});

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

TEST_F(IncludeCollector, DontCollectHeaderGuards)
{
    auto fileId = id(TESTDATA_DIR "/symbolscollector_defines.h");
    emptyCollector.addFile(fileId, {"cc", "-I", TESTDATA_DIR "/includecollector/external", "-I", TESTDATA_DIR "/includecollector/project", "-isystem", TESTDATA_DIR "/includecollector/system"});

    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.usedMacros(),
                Not(Contains(Eq(UsedMacro{"SYMBOLSCOLLECTOR_DEFINES_H", fileId}))));
}

TEST_F(IncludeCollector, DISABLED_DontCollectDynamicLibraryExports)
{
    auto fileId = id(TESTDATA_DIR "/symbolscollector_defines.h");
    emptyCollector.addFile(fileId, {"cc", "-I", TESTDATA_DIR "/includecollector/external", "-I", TESTDATA_DIR "/includecollector/project", "-isystem", TESTDATA_DIR "/includecollector/system"});

    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.usedMacros(),
                Not(Contains(Eq(UsedMacro{"CLASS_EXPORT", fileId}))));
}

TEST_F(IncludeCollector, CollectFileStatuses)
{
    emptyCollector.addFile(id(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc", "-I", TESTDATA_DIR "/includecollector/external", "-I", TESTDATA_DIR "/includecollector/project", "-isystem", TESTDATA_DIR "/includecollector/system"});

    emptyCollector.collect();

    ASSERT_THAT(emptyCollector.fileStatuses(),
                ElementsAre(
                    fileStatus(TESTDATA_DIR "/symbolscollector_main.cpp"),
                    fileStatus(TESTDATA_DIR "/symbolscollector_header1.h"),
                    fileStatus(TESTDATA_DIR "/symbolscollector_header2.h")));
}

TEST_F(IncludeCollector, CollectSourceDependencies)
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

}
