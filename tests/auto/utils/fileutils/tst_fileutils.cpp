// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest>

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>

//TESTED_COMPONENT=src/libs/utils

QT_BEGIN_NAMESPACE
namespace QTest {

template<>
char *toString(const Utils::FilePath &filePath)
{
    return qstrdup(filePath.toString().toLocal8Bit().constData());
}

template<>
char *toString(const Utils::FilePathInfo &filePathInfo)
{
    QByteArray ba = "FilePathInfo(";
    ba += QByteArray::number(filePathInfo.fileSize);
    ba += ", ";
    ba += QByteArray::number(filePathInfo.fileFlags, 16);
    ba += ", ";
    ba += filePathInfo.lastModified.toString().toUtf8();
    ba += ")";
    return qstrdup(ba.constData());
}

} // namespace QTest
QT_END_NAMESPACE

using namespace Utils;

class tst_fileutils : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void commonPath_data();
    void commonPath();

    void bytesAvailableFromDF_data();
    void bytesAvailableFromDF();

    void filePathInfoFromTriple_data();
    void filePathInfoFromTriple();
};

void tst_fileutils::initTestCase() {}

void tst_fileutils::commonPath()
{
    QFETCH(FilePaths, list);
    QFETCH(FilePath, expected);

    const FilePath result = FileUtils::commonPath(list);

    QCOMPARE(expected.toString(), result.toString());
}

void tst_fileutils::commonPath_data()
{
    QTest::addColumn<FilePaths>("list");
    QTest::addColumn<FilePath>("expected");

    const FilePath p1 = FilePath::fromString("c:/Program Files(x86)");
    const FilePath p2 = FilePath::fromString("c:/Program Files(x86)/Ide");
    const FilePath p3 = FilePath::fromString("c:/Program Files(x86)/Ide/Qt Creator");
    const FilePath p4 = FilePath::fromString("c:/Program Files");
    const FilePath p5 = FilePath::fromString("c:");

    const FilePath url1 = FilePath::fromString("http://127.0.0.1/./");
    const FilePath url2 = FilePath::fromString("https://127.0.0.1/./");
    const FilePath url3 = FilePath::fromString("http://www.qt.io/./");
    const FilePath url4 = FilePath::fromString("https://www.qt.io/./");
    const FilePath url5 = FilePath::fromString("https://www.qt.io/ide/");
    const FilePath url6 = FilePath::fromString("http:///./");

    QTest::newRow("Zero paths") << FilePaths{} << FilePath();
    QTest::newRow("Single path") << FilePaths{p1} << p1;
    QTest::newRow("3 identical paths") << FilePaths{p1, p1, p1} << p1;
    QTest::newRow("3 paths, common path") << FilePaths{p1, p2, p3} << p1;
    QTest::newRow("3 paths, no common path") << FilePaths{p1, p2, p4} << p5;
    QTest::newRow("3 paths, first is part of second") << FilePaths{p4, p1, p3} << p5;
    QTest::newRow("Common scheme") << FilePaths{url1, url3} << url6;
    QTest::newRow("Different scheme") << FilePaths{url1, url2} << FilePath();
    QTest::newRow("Common host") << FilePaths{url4, url5} << url4;
    QTest::newRow("Different host") << FilePaths{url1, url3} << url6;
}

void tst_fileutils::bytesAvailableFromDF_data()
{
    QTest::addColumn<QByteArray>("dfOutput");
    QTest::addColumn<qint64>("expected");

    QTest::newRow("empty") << QByteArray("") << qint64(-1);
    QTest::newRow("mac") << QByteArray(
        "Filesystem   1024-blocks      Used Available Capacity iused      ifree %iused  Mounted "
        "on\n/dev/disk3s5   971350180 610014564 342672532    65% 4246780 3426725320    0%   "
        "/System/Volumes/Data\n")
                         << qint64(342672532);
    QTest::newRow("alpine") << QByteArray(
        "Filesystem           1K-blocks      Used Available Use% Mounted on\noverlay              "
        "569466448 163526072 376983360  30% /\n")
                            << qint64(376983360);
    QTest::newRow("alpine-no-trailing-br")
        << QByteArray("Filesystem           1K-blocks      Used Available Use% Mounted on\noverlay "
                      "             569466448 163526072 376983360  30% /")
        << qint64(376983360);
    QTest::newRow("alpine-missing-line")
        << QByteArray("Filesystem           1K-blocks      Used Available Use% Mounted on\n")
        << qint64(-1);
    QTest::newRow("wrong-header") << QByteArray(
        "Filesystem           1K-blocks      Used avail Use% Mounted on\noverlay              "
        "569466448 163526072 376983360  30% /\n")
                                  << qint64(-1);
    QTest::newRow("not-enough-fields") << QByteArray(
        "Filesystem 1K-blocks Used avail Use% Mounted on\noverlay              569466448\n")
                                       << qint64(-1);
    QTest::newRow("not-enough-header-fields")
        << QByteArray("Filesystem           1K-blocks      Used \noverlay              569466448 "
                      "163526072 376983360  30% /\n")
        << qint64(-1);
}

void tst_fileutils::bytesAvailableFromDF()
{
    if (HostOsInfo::isWindowsHost())
        QSKIP("Test only valid on unix-ish systems");

    QFETCH(QByteArray, dfOutput);
    QFETCH(qint64, expected);

    const auto result = FileUtils::bytesAvailableFromDFOutput(dfOutput);
    QCOMPARE(result, expected);
}

void tst_fileutils::filePathInfoFromTriple_data()
{
    QTest::addColumn<QString>("statoutput");
    QTest::addColumn<FilePathInfo>("expected");

    QTest::newRow("empty") << QString() << FilePathInfo();

    QTest::newRow("linux-root") << QString("41ed 1676354359 4096")
                                << FilePathInfo{4096,
                                                FilePathInfo::FileFlags(
                                                    FilePathInfo::ReadOwnerPerm
                                                    | FilePathInfo::WriteOwnerPerm
                                                    | FilePathInfo::ExeOwnerPerm
                                                    | FilePathInfo::ReadUserPerm
                                                    | FilePathInfo::WriteUserPerm
                                                    | FilePathInfo::ExeUserPerm
                                                    | FilePathInfo::ReadGroupPerm
                                                    | FilePathInfo::ExeGroupPerm
                                                    | FilePathInfo::ReadOtherPerm
                                                    | FilePathInfo::ExeOtherPerm
                                                    | FilePathInfo::DirectoryType
                                                    | FilePathInfo::ExistsFlag),
                                                QDateTime::fromSecsSinceEpoch(1676354359)};

    QTest::newRow("linux-grep-bin")
        << QString("81ed 1668852790 808104")
        << FilePathInfo{808104,
                        FilePathInfo::FileFlags(
                            FilePathInfo::ReadOwnerPerm | FilePathInfo::WriteOwnerPerm
                            | FilePathInfo::ExeOwnerPerm | FilePathInfo::ReadUserPerm
                            | FilePathInfo::WriteUserPerm | FilePathInfo::ExeUserPerm
                            | FilePathInfo::ReadGroupPerm | FilePathInfo::ExeGroupPerm
                            | FilePathInfo::ReadOtherPerm | FilePathInfo::ExeOtherPerm
                            | FilePathInfo::FileType | FilePathInfo::ExistsFlag),
                        QDateTime::fromSecsSinceEpoch(1668852790)};

    QTest::newRow("linux-disk") << QString("61b0 1651167746 0")
                                << FilePathInfo{0,
                                                FilePathInfo::FileFlags(
                                                    FilePathInfo::ReadOwnerPerm
                                                    | FilePathInfo::WriteOwnerPerm
                                                    | FilePathInfo::ReadUserPerm
                                                    | FilePathInfo::WriteUserPerm
                                                    | FilePathInfo::ReadGroupPerm
                                                    | FilePathInfo::WriteGroupPerm
                                                    | FilePathInfo::LocalDiskFlag
                                                    | FilePathInfo::ExistsFlag),
                                                QDateTime::fromSecsSinceEpoch(1651167746)};
}

void tst_fileutils::filePathInfoFromTriple()
{
    QFETCH(QString, statoutput);
    QFETCH(FilePathInfo, expected);

    const FilePathInfo result = FileUtils::filePathInfoFromTriple(statoutput, 16);

    QCOMPARE(result, expected);
}

QTEST_GUILESS_MAIN(tst_fileutils)

#include "tst_fileutils.moc"
