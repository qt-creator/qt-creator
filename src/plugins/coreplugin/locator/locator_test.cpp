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

    void refresh(QFutureInterface<void> &) {}
};

inline QString _(const QByteArray &ba) { return QString::fromLatin1(ba, ba.size()); }

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

    foreach (const ReferenceData &reference, referenceDataList) {
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
    const MyTestDataDir testDir(QLatin1String("testdata_basic"));
    const QStringList testFiles({ QDir::fromNativeSeparators(testDir.file("file.cpp")),
                                  QDir::fromNativeSeparators(testDir.file("main.cpp")),
                                  QDir::fromNativeSeparators(testDir.file("subdir/main.cpp")) });
    QStringList testFilesShort;
    foreach (const QString &file, testFiles)
        testFilesShort << Utils::FileUtils::shortNativePath(Utils::FileName::fromString(file));

    QTest::newRow("BaseFileFilter-EmptyInput")
        << testFiles
        << (QList<ReferenceData>()
            << ReferenceData(
                QString(),
                (QList<ResultData>()
                    << ResultData(_("file.cpp"), testFilesShort.at(0))
                    << ResultData(_("main.cpp"), testFilesShort.at(1))
                    << ResultData(_("main.cpp"), testFilesShort.at(2))))
            );

    QTest::newRow("BaseFileFilter-InputIsFileName")
        << testFiles
        << (QList<ReferenceData>()
            << ReferenceData(
                _("main.cpp"),
                (QList<ResultData>()
                    << ResultData(_("main.cpp"), testFilesShort.at(1))
                    << ResultData(_("main.cpp"), testFilesShort.at(2))))
           );

    QTest::newRow("BaseFileFilter-InputIsFilePath")
        << testFiles
        << (QList<ReferenceData>()
            << ReferenceData(
                QString(_("subdir") + pathSeparator + _("main.cpp")),
                (QList<ResultData>()
                     << ResultData(_("main.cpp"), testFilesShort.at(2))))
            );

    QTest::newRow("BaseFileFilter-InputIsDirIsPath")
        << testFiles
        << (QList<ReferenceData>()
            << ReferenceData( _("subdir"), QList<ResultData>())
            << ReferenceData(
                QString(_("subdir") + pathSeparator + _("main.cpp")),
                (QList<ResultData>()
                     << ResultData(_("main.cpp"), testFilesShort.at(2))))
            );

    QTest::newRow("BaseFileFilter-InputIsFileNameFilePathFileName")
        << testFiles
        << (QList<ReferenceData>()
            << ReferenceData(
                _("main.cpp"),
                (QList<ResultData>()
                    << ResultData(_("main.cpp"), testFilesShort.at(1))
                    << ResultData(_("main.cpp"), testFilesShort.at(2))))
            << ReferenceData(
                QString(_("subdir") + pathSeparator + _("main.cpp")),
                (QList<ResultData>()
                     << ResultData(_("main.cpp"), testFilesShort.at(2))))
            << ReferenceData(
                _("main.cpp"),
                (QList<ResultData>()
                    << ResultData(_("main.cpp"), testFilesShort.at(1))
                    << ResultData(_("main.cpp"), testFilesShort.at(2))))
            );
}
