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

#include "../coreplugin.h"

#include "basefilefilter.h"
#include "locatorfiltertest.h"

#include <coreplugin/testdatadir.h>
#include <utils/fileutils.h>

#include <QDir>
#include <QTextStream>
#include <QtTest>

using namespace Core::Tests;

namespace {

QTC_DECLARE_MYTESTDATADIR("../../../../tests/locators/")

class MyBaseFileFilter : public Core::BaseFileFilter
{
public:
    MyBaseFileFilter(const QStringList &theFiles)
    {
        setFileIterator(new BaseFileFilter::ListIterator(theFiles));
    }

    void refresh(QFutureInterface<void> &) override {}
};

class ReferenceData
{
public:
    ReferenceData() {}
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

    MyBaseFileFilter filter(testFiles);
    BasicLocatorFilterTest test(&filter);

    for (const ReferenceData &reference : qAsConst(referenceDataList)) {
        const QList<LocatorFilterEntry> filterEntries = test.matchesFor(reference.searchText);
        const ResultDataList results = ResultData::fromFilterEntryList(filterEntries);
//        QTextStream(stdout) << "----" << endl;
//        ResultData::printFilterEntries(results);
        QCOMPARE(results, reference.results);
    }
}

void Core::Internal::CorePlugin::test_basefilefilter_data()
{
    QTest::addColumn<QStringList>("testFiles");
    QTest::addColumn<QList<ReferenceData> >("referenceDataList");

    const QChar pathSeparator = QDir::separator();
    const MyTestDataDir testDir("testdata_basic");
    const QStringList testFiles({QDir::fromNativeSeparators(testDir.file("file.cpp")),
                                 QDir::fromNativeSeparators(testDir.file("main.cpp")),
                                 QDir::fromNativeSeparators(testDir.file("subdir/main.cpp"))});
    QStringList testFilesShort;
    for (const QString &file : testFiles)
        testFilesShort << Utils::FileUtils::shortNativePath(Utils::FileName::fromString(file));

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
    QStringList priorityTestFilesShort;
    for (const QString &file : priorityTestFiles)
        priorityTestFilesShort << Utils::FileUtils::shortNativePath(Utils::FileName::fromString(file));

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
}
