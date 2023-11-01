// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../coreplugin.h"
#include "../testdatadir.h"
#include "locatorfiltertest.h"

#include <utils/algorithm.h>
#include <utils/fileutils.h>

#include <QDir>
#include <QtTest>

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
    QFETCH(QStringList, testFiles);
    QFETCH(QList<ReferenceData>, referenceDataList);

    LocatorFileCache cache;
    cache.setFilePaths(FileUtils::toFilePathList(testFiles));
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
    auto shortNativePath = [](const QString &file) {
        return FilePath::fromString(file).shortNativePath();
    };

    QTest::addColumn<QStringList>("testFiles");
    QTest::addColumn<QList<ReferenceData> >("referenceDataList");

    const QChar pathSeparator = QDir::separator();
    const MyTestDataDir testDir("testdata_basic");
    const QStringList testFiles({QDir::fromNativeSeparators(testDir.file("file.cpp")),
                                 QDir::fromNativeSeparators(testDir.file("main.cpp")),
                                 QDir::fromNativeSeparators(testDir.file("subdir/main.cpp"))});
    const QStringList testFilesShort = Utils::transform(testFiles, shortNativePath);

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

    const QStringList priorityTestFiles({testDir.file("qmap.cpp"),
                                         testDir.file("mid_qcore_mac_p.h"),
                                         testDir.file("qcore_mac_p.h"),
                                         testDir.file("foo_qmap.h"),
                                         testDir.file("qmap.h"),
                                         testDir.file("bar.h")});
    const QStringList priorityTestFilesShort = Utils::transform(priorityTestFiles, shortNativePath);

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

    const QStringList sortingTestFiles({QDir::fromNativeSeparators(testDir.file("aaa/zfile.cpp")),
                                        QDir::fromNativeSeparators(testDir.file("bbb/yfile.cpp")),
                                        QDir::fromNativeSeparators(testDir.file("ccc/xfile.cpp"))});
    const QStringList sortingTestFilesShort = Utils::transform(sortingTestFiles, shortNativePath);

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
