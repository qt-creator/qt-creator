// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/filepath.h>

#include <k7zip.h>
#include <karchive.h>
#include <ktar.h>
#include <kzip.h>

#include <QObject>
#include <QtTest>

using namespace Utils;

static const QByteArray testData = R"(Hello, World!
Compress me please!
Thank you!

Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut
labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores
et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet.

Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut
labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores
et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet.

Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut
labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores
et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet.

Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut
labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores
et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet.

Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut
labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores
et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet.
)";

class tst_Archive : public QObject
{
    Q_OBJECT

    void testArchive(KArchive &&archive, const FilePath &testFile)
    {
        QVERIFY(testFile.writeFileContents(testData));

        QVERIFY(archive.open(QIODevice::WriteOnly));
        QVERIFY(archive.addLocalFile(testFile.toFSPathString(), "data/test.txt"));
        QVERIFY(archive.close());
        QVERIFY(testFile.removeFile());
        QVERIFY(!testFile.exists());
    }

    void testUnarchive(KArchive &&archive, const FilePath &testFile)
    {
        QVERIFY(!testFile.exists());

        QVERIFY(archive.open(QIODevice::ReadOnly));
        QVERIFY(archive.directory()->entries().contains("data"));
        QVERIFY(archive.directory()->entry("data"));
        QVERIFY(archive.directory()->entry("data")->isDirectory());
        auto dataDir = static_cast<const KArchiveDirectory *>(archive.directory()->entry("data"));
        QVERIFY(dataDir->entries().contains("test.txt"));
        QVERIFY(dataDir->entry("test.txt")->isFile());
        QVERIFY(dataDir->file("test.txt")->copyTo("."));

        QVERIFY(testFile.exists());
        auto contents = testFile.fileContents();
        QVERIFY(contents);
        QCOMPARE(*contents, QByteArray(testData));
        QVERIFY(testFile.removeFile());
    }

    template<class ARCHIVE>
    void unArchive(const FilePath &archivePath, bool testSize = true)
    {
        FilePath p = FilePath::fromString("test.txt");

        testArchive(ARCHIVE(archivePath.toFSPathString()), p);
        QVERIFY(archivePath.exists());
        qDebug() << "Archive size:" << archivePath.fileSize() << "(expected roughly less than"
                 << testData.size() << ")";
        if (testSize)
            QVERIFY(archivePath.fileSize() < testData.size());
        testUnarchive(ARCHIVE(archivePath.toFSPathString()), p);
        QVERIFY(archivePath.removeFile());
    }

private slots:
    void initTestCase() {}

    void cleanupTestCase() {}

    void test7zip() { unArchive<K7Zip>(FilePath::fromString("test.7z")); }
    void testZip() { unArchive<KZip>(FilePath::fromString("test.zip")); }
    void testTar() { unArchive<KTar>(FilePath::fromString("test.tar"), false); }
    void testXZ() { unArchive<KTar>(FilePath::fromString("test.tar.xz")); }
    void testGZ() { unArchive<KTar>(FilePath::fromString("test.tar.gz")); }
    void testBZip2() { unArchive<KTar>(FilePath::fromString("test.tar.bz2")); }
};

QTEST_GUILESS_MAIN(tst_Archive)

#include "tst_archive.moc"
