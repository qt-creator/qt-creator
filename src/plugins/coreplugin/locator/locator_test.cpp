// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../coreplugin.h"
#include "../testdatadir.h"
#include "locatorfiltertest.h"

#include <utils/algorithm.h>
#include <utils/fileutils.h>

#include <QDir>
#include <QTest>

using namespace Core::Tests;
using namespace Utils;

namespace {

QTC_DECLARE_MYTESTDATADIR("../../../tests/locators/")

class ReferenceData
{
public:
    ReferenceData() = default;
    ReferenceData(const QString &searchText, const ResultDataList &results)
        : searchText(searchText), results(results) {}

    QString searchText;
    ResultDataList results;
};

} // anonymous namespace

Q_DECLARE_METATYPE(ReferenceData)
Q_DECLARE_METATYPE(QList<ReferenceData>)

void Core::Internal::CorePlugin::test_basefilefilter()
{
    QFETCH(FilePaths, testFiles);
    QFETCH(QList<ReferenceData>, referenceDataList);

    LocatorFileCache cache;
    cache.setFilePaths(testFiles);
    const LocatorMatcherTasks tasks = {cache.matcher()};
    for (const ReferenceData &reference : std::as_const(referenceDataList)) {
        const LocatorFilterEntries filterEntries = LocatorMatcher::runBlocking(
            tasks, reference.searchText);
        const ResultDataList results = ResultData::fromFilterEntryList(filterEntries);
        QCOMPARE(results, reference.results);
    }
}

void Core::Internal::CorePlugin::test_basefilefilter_data()
{
    QTest::addColumn<FilePaths>("testFiles");
    QTest::addColumn<QList<ReferenceData> >("referenceDataList");

    const QChar pathSeparator = QDir::separator();
    const MyTestDataDir testDir("testdata_basic");
    const FilePaths testFiles({testDir.filePath("file.cpp"),
                               testDir.filePath("main.cpp"),
                               testDir.filePath("subdir/main.cpp")});
    const QStringList testFilesShort = Utils::transform(testFiles, &FilePath::shortNativePath);

    QTest::newRow("BaseFileFilter-EmptyInput")
        << testFiles
        << (QList<ReferenceData>()
            << ReferenceData(
                QString(),
                (QList<ResultData>()
                    << ResultData("file.cpp", testFilesShort.at(0))
                    << ResultData("main.cpp", testFilesShort.at(1))
                    << ResultData("main.cpp", testFilesShort.at(2))))
            );

    QTest::newRow("BaseFileFilter-InputIsFileName")
        << testFiles
        << (QList<ReferenceData>()
            << ReferenceData(
                "main.cpp",
                (QList<ResultData>()
                    << ResultData("main.cpp", testFilesShort.at(1))
                    << ResultData("main.cpp", testFilesShort.at(2))))
           );

    QTest::newRow("BaseFileFilter-InputIsFilePath")
        << testFiles
        << (QList<ReferenceData>()
            << ReferenceData(
                QString("subdir" + pathSeparator + "main.cpp"),
                (QList<ResultData>()
                     << ResultData("main.cpp", testFilesShort.at(2))))
            );

    QTest::newRow("BaseFileFilter-InputIsDirIsPath")
        << testFiles
        << (QList<ReferenceData>()
            << ReferenceData( "subdir", QList<ResultData>())
            << ReferenceData(
                QString("subdir" + pathSeparator + "main.cpp"),
                (QList<ResultData>()
                     << ResultData("main.cpp", testFilesShort.at(2))))
            );

    QTest::newRow("BaseFileFilter-InputIsFileNameFilePathFileName")
        << testFiles
        << (QList<ReferenceData>()
            << ReferenceData(
                "main.cpp",
                (QList<ResultData>()
                    << ResultData("main.cpp", testFilesShort.at(1))
                    << ResultData("main.cpp", testFilesShort.at(2))))
            << ReferenceData(
                QString("subdir" + pathSeparator + "main.cpp"),
                (QList<ResultData>()
                     << ResultData("main.cpp", testFilesShort.at(2))))
            << ReferenceData(
                "main.cpp",
                (QList<ResultData>()
                    << ResultData("main.cpp", testFilesShort.at(1))
                    << ResultData("main.cpp", testFilesShort.at(2))))
            );

    const FilePaths priorityTestFiles({testDir.filePath("qmap.cpp"),
                                       testDir.filePath("mid_qcore_mac_p.h"),
                                       testDir.filePath("qcore_mac_p.h"),
                                       testDir.filePath("foo_qmap.h"),
                                       testDir.filePath("qmap.h"),
                                       testDir.filePath("bar.h")});
    const QStringList priorityTestFilesShort = Utils::transform(priorityTestFiles,
                                                                &FilePath::shortNativePath);

    QTest::newRow("BaseFileFilter-InputPriorizeFullOverFuzzy")
        << priorityTestFiles
        << (QList<ReferenceData>()
            << ReferenceData(
                "qmap.h",
                (QList<ResultData>()
                     << ResultData("qmap.h", priorityTestFilesShort.at(4))
                     << ResultData("foo_qmap.h", priorityTestFilesShort.at(3))
                     << ResultData("qcore_mac_p.h", priorityTestFilesShort.at(2))
                     << ResultData("mid_qcore_mac_p.h", priorityTestFilesShort.at(1))))
           );

    const FilePaths sortingTestFiles({testDir.filePath("aaa/zfile.cpp"),
                                      testDir.filePath("bbb/yfile.cpp"),
                                      testDir.filePath("ccc/xfile.cpp")});
    const QStringList sortingTestFilesShort = Utils::transform(sortingTestFiles,
                                                               &FilePath::shortNativePath);

    QTest::newRow("BaseFileFilter-SortByDisplayName")
        << sortingTestFiles
        << (QList<ReferenceData>()
            << ReferenceData(
                "file",
                (QList<ResultData>()
                    << ResultData("xfile.cpp", sortingTestFilesShort.at(2))
                    << ResultData("yfile.cpp", sortingTestFilesShort.at(1))
                    << ResultData("zfile.cpp", sortingTestFilesShort.at(0))))
            );
}
