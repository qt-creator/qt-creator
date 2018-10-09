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
#include <usedmacrosandsourcescollector.h>

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

class UsedMacroAndSourcesCollector : public ::testing::Test
{
protected:
    FilePathId filePathId(Utils::SmallStringView filePath) const
    {
        return filePathCache.filePathId(ClangBackEnd::FilePathView{filePath});
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
        return {filePathId(filePath), fileSize(filePath), lastModified(filePath), false};
    }
protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> databaseInitializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    ClangBackEnd::UsedMacroAndSourcesCollector collector{filePathCache};
};

TEST_F(UsedMacroAndSourcesCollector, SourceFiles)
{
    collector.addFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});

    collector.collect();

    ASSERT_THAT(collector.sourceFiles(),
                UnorderedElementsAre(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"),
                                     filePathId(TESTDATA_DIR "/symbolscollector_header1.h"),
                                     filePathId(TESTDATA_DIR "/symbolscollector_header2.h")));
}

TEST_F(UsedMacroAndSourcesCollector, MainFileInSourceFiles)
{
    collector.addFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});

    ASSERT_THAT(collector.sourceFiles(),
                ElementsAre(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp")));
}

TEST_F(UsedMacroAndSourcesCollector, ResetMainFileInSourceFiles)
{
    collector.addFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});

    ASSERT_THAT(collector.sourceFiles(),
                ElementsAre(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp")));
}

TEST_F(UsedMacroAndSourcesCollector, DontDuplicateSourceFiles)
{
    collector.addFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});
    collector.collect();

    collector.collect();

    ASSERT_THAT(collector.sourceFiles(),
                UnorderedElementsAre(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"),
                                     filePathId(TESTDATA_DIR "/symbolscollector_header1.h"),
                                     filePathId(TESTDATA_DIR "/symbolscollector_header2.h")));
}

TEST_F(UsedMacroAndSourcesCollector, ClearSourceFiles)
{
    collector.addFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});

    collector.clear();

    ASSERT_THAT(collector.sourceFiles(), IsEmpty());
}

TEST_F(UsedMacroAndSourcesCollector, ClearFileStatus)
{
    collector.addFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});
    collector.collect();

    collector.clear();

    ASSERT_THAT(collector.fileStatuses(), IsEmpty());
}

TEST_F(UsedMacroAndSourcesCollector, ClearUsedMacros)
{
    collector.addFile(filePathId(TESTDATA_DIR "/symbolscollector_defines.h"), {"cc"});
    collector.collect();

    collector.clear();

    ASSERT_THAT(collector.usedMacros(), IsEmpty());
}

TEST_F(UsedMacroAndSourcesCollector, ClearSourceDependencies)
{
    collector.addFile(filePathId(TESTDATA_DIR "/symbolscollector_main2.cpp"), {"cc", "-I" TESTDATA_DIR});
    collector.collect();

    collector.clear();

    ASSERT_THAT(collector.sourceDependencies(), IsEmpty());
}

TEST_F(UsedMacroAndSourcesCollector, DontCollectSourceFilesAfterFilesAreCleared)
{
    collector.addFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});

    collector.clear();
    collector.collect();

    ASSERT_THAT(collector.sourceFiles(), IsEmpty());
}

TEST_F(UsedMacroAndSourcesCollector, DontCollectFileStatusAfterFilesAreCleared)
{
    collector.addFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});

    collector.clear();
    collector.collect();

    ASSERT_THAT(collector.fileStatuses(), IsEmpty());
}

TEST_F(UsedMacroAndSourcesCollector, DontCollectUsedMacrosAfterFilesAreCleared)
{
    collector.addFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});

    collector.clear();
    collector.collect();

    ASSERT_THAT(collector.usedMacros(), IsEmpty());
}


TEST_F(UsedMacroAndSourcesCollector, DontCollectSourceDependenciesAfterFilesAreCleared)
{
    collector.addFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});

    collector.clear();
    collector.collect();

    ASSERT_THAT(collector.sourceDependencies(), IsEmpty());
}

TEST_F(UsedMacroAndSourcesCollector, CollectUsedMacrosWithExternalDefine)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.addFile(fileId, {"cc", "-DCOMPILER_ARGUMENT"});

    collector.collect();

    ASSERT_THAT(collector.usedMacros(),
                ElementsAre(Eq(UsedMacro{"DEFINED", fileId}),
                            Eq(UsedMacro{"IF_DEFINE", fileId}),
                            Eq(UsedMacro{"__clang__", fileId}),
                            Eq(UsedMacro{"CLASS_EXPORT", fileId}),
                            Eq(UsedMacro{"IF_NOT_DEFINE", fileId}),
                            Eq(UsedMacro{"MACRO_EXPANSION", fileId}),
                            Eq(UsedMacro{"COMPILER_ARGUMENT", fileId})));
}

TEST_F(UsedMacroAndSourcesCollector, CollectUsedMacrosWithoutExternalDefine)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.addFile(fileId, {"cc"});

    collector.collect();

    ASSERT_THAT(collector.usedMacros(),
                ElementsAre(Eq(UsedMacro{"DEFINED", fileId}),
                            Eq(UsedMacro{"IF_DEFINE", fileId}),
                            Eq(UsedMacro{"__clang__", fileId}),
                            Eq(UsedMacro{"CLASS_EXPORT", fileId}),
                            Eq(UsedMacro{"IF_NOT_DEFINE", fileId}),
                            Eq(UsedMacro{"MACRO_EXPANSION", fileId}),
                            Eq(UsedMacro{"COMPILER_ARGUMENT", fileId})));
}

TEST_F(UsedMacroAndSourcesCollector, DontCollectHeaderGuards)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.addFile(fileId, {"cc"});

    collector.collect();

    ASSERT_THAT(collector.usedMacros(),
                Not(Contains(Eq(UsedMacro{"SYMBOLSCOLLECTOR_DEFINES_H", fileId}))));
}

TEST_F(UsedMacroAndSourcesCollector, DISABLED_DontCollectDynamicLibraryExports)
{
    auto fileId = filePathId(TESTDATA_DIR "/symbolscollector_defines.h");
    collector.addFile(fileId, {"cc"});

    collector.collect();

    ASSERT_THAT(collector.usedMacros(),
                Not(Contains(Eq(UsedMacro{"CLASS_EXPORT", fileId}))));
}

TEST_F(UsedMacroAndSourcesCollector, CollectFileStatuses)
{
    collector.addFile(filePathId(TESTDATA_DIR "/symbolscollector_main.cpp"), {"cc"});

    collector.collect();

    ASSERT_THAT(collector.fileStatuses(),
                ElementsAre(
                    fileStatus(TESTDATA_DIR "/symbolscollector_main.cpp"),
                    fileStatus(TESTDATA_DIR "/symbolscollector_header1.h"),
                    fileStatus(TESTDATA_DIR "/symbolscollector_header2.h")));
}

TEST_F(UsedMacroAndSourcesCollector, CollectSourceDependencies)
{
    auto mainFileId = filePathId(TESTDATA_DIR "/symbolscollector_main2.cpp");
    auto header1FileId = filePathId(TESTDATA_DIR "/symbolscollector_header1.h");
    auto header2FileId = filePathId(TESTDATA_DIR "/symbolscollector_header2.h");
    auto header3FileId = filePathId(TESTDATA_DIR "/symbolscollector_header3.h");
    collector.addFile(mainFileId, {"cc", "-I" TESTDATA_DIR});

    collector.collect();

    ASSERT_THAT(collector.sourceDependencies(),
                UnorderedElementsAre(SourceDependency(mainFileId, header1FileId),
                                     SourceDependency(mainFileId, header3FileId),
                                     SourceDependency(header3FileId, header2FileId),
                                     SourceDependency(header1FileId, header2FileId)));
}
}
