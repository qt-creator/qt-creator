// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QRandomGenerator>
#include <QtTest>

#include <utils/fsengine/fsengine.h>
#include <utils/hostosinfo.h>
#include <utils/unarchiver.h>

#include <archive.h>
#include <archive_entry.h>

namespace Utils {

void write_archive(struct archive *a, const FilePath &archive, const FilePaths &files)
{
    struct archive_entry *entry;
    QCOMPARE(archive_write_open_filename_w(a, archive.path().toStdWString().c_str()), ARCHIVE_OK);

    for (const auto &file : files) {
        entry = archive_entry_new();
        archive_entry_set_pathname_utf8(
            entry, file.relativeChildPath(archive.parentDir()).toFSPathString().toUtf8().data());
        archive_entry_set_size(entry, file.fileSize());
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);
        QCOMPARE(archive_write_header(a, entry), ARCHIVE_OK);
        const auto contents = file.fileContents();
        QCOMPARE(archive_write_data(a, contents->data(), contents->size()), contents->size());
        QCOMPARE(archive_write_finish_entry(a), ARCHIVE_OK);
        archive_entry_free(entry);
    }
    QCOMPARE(archive_write_close(a), ARCHIVE_OK);
    QCOMPARE(archive_write_free(a), ARCHIVE_OK);
}

class ScopedFilePath : public FilePath
{
public:
    ScopedFilePath(const FilePath &other)
        : FilePath(other)
    {}
    ~ScopedFilePath() { removeFile(); }
};

class tst_unarchiver : public QObject
{
    Q_OBJECT

public:
    ScopedFilePath writeArchive(archive *a)
    {
        ScopedFilePath zipFile
            = *FilePath::fromString(tempDir.path() + "/test-archive").createTempFile();

        ScopedFilePath testFile1 = FilePath::fromString(tempDir.path() + "/test1.txt");
        testFile1.writeFileContents("Hello World!");

        ScopedFilePath testFile2 = FilePath::fromString(tempDir.path() + "/test2.txt");
        testFile2.writeFileContents("Hello World again!");

        write_archive(a, zipFile, {testFile1, testFile2});

        return zipFile;
    }

    void readArchive(const FilePath &zipFile)
    {
        // Unarchive using Utils::Unarchiver
        Unarchiver unarchiver;
        unarchiver.setArchive(zipFile);
        unarchiver.setDestination(FilePath::fromString(tempDir.path() + "/unarchived"));
        unarchiver.start();
        const Result<> r = unarchiver.result();

        if (!r)
            qWarning() << "ERROR:" << r.error();

        QVERIFY(r);

        ScopedFilePath unarchivedFile = FilePath::fromString(
            tempDir.path() + "/unarchived/test1.txt");
        QVERIFY(unarchivedFile.isFile());
        QCOMPARE(unarchivedFile.fileContents(), "Hello World!");

        ScopedFilePath unarchivedFile2 = FilePath::fromString(
            tempDir.path() + "/unarchived/test2.txt");
        QVERIFY(unarchivedFile2.isFile());
        QCOMPARE(unarchivedFile2.fileContents(), "Hello World again!");
    }

    void writeAndReadArchive(archive *a)
    {
        ScopedFilePath zipFile = writeArchive(a);
        QVERIFY(!zipFile.isEmpty());
        QVERIFY(zipFile.isFile());
        QVERIFY(zipFile.fileSize() > 0);
        readArchive(zipFile);
    }

private slots:
    void initTestCase() { QVERIFY(tempDir.isValid()); }

    void tst_tar_gz()
    {
        struct archive *a = archive_write_new();
        archive_write_add_filter(a, ARCHIVE_FILTER_GZIP);
        archive_write_set_format(a, ARCHIVE_FORMAT_TAR);
        writeAndReadArchive(a);
    }

    void tst_tar_bz2()
    {
        struct archive *a = archive_write_new();
        archive_write_add_filter(a, ARCHIVE_FILTER_BZIP2);
        archive_write_set_format(a, ARCHIVE_FORMAT_TAR);
        writeAndReadArchive(a);
    }

    void tst_7z()
    {
        struct archive *a = archive_write_new();
        archive_write_add_filter(a, ARCHIVE_FILTER_NONE);
        archive_write_set_format(a, ARCHIVE_FORMAT_7ZIP);
        writeAndReadArchive(a);
    }

    void tst_zip()
    {
        struct archive *a = archive_write_new();
        archive_write_add_filter(a, ARCHIVE_FILTER_NONE);
        archive_write_set_format(a, ARCHIVE_FORMAT_ZIP);
        writeAndReadArchive(a);
    }

    void tst_remote()
    {
        if (!FSEngine::isAvailable())
            QSKIP("Utils was built without Filesystem Engine");

        if (HostOsInfo::isWindowsHost())
            QSKIP("The fsengine tests are not supported on Windows.");

        FSEngine fileSystemEngine;
        FSEngine::addDevice(FilePath::fromString("device://test"));

        struct archive *a = archive_write_new();
        archive_write_add_filter(a, ARCHIVE_FILTER_NONE);
        archive_write_set_format(a, ARCHIVE_FORMAT_ZIP);

        ScopedFilePath zipFile = writeArchive(a);

        FilePath p = FilePath::fromString("device://test/" + zipFile.path());

        readArchive(p);
    }

    void tst_raw()
    {
        ScopedFilePath rawFile = FilePath::fromString(tempDir.path() + "/test.raw");
        rawFile.writeFileContents("Hello World!");

        Unarchiver unarchiver;
        unarchiver.setArchive(rawFile);
        unarchiver.setDestination(FilePath::fromString(tempDir.path() + "/unarchived"));
        unarchiver.start();
        const Result<> r = unarchiver.result();

        // This should fail as the input file is not an archive
        QVERIFY(!r);
    }

    void tst_gzip_file()
    {
        struct archive *a = archive_write_new();
        archive_write_set_format_raw(a);
        archive_write_add_filter_gzip(a);

        ScopedFilePath gzFile
            = *FilePath::fromString(tempDir.path() + "/test-archive").createTempFile();

        ScopedFilePath testFile1 = FilePath::fromString(tempDir.path() + "/test1.txt");
        testFile1.writeFileContents("Hello World!");

        struct archive_entry *entry = archive_entry_new();
        QCOMPARE(archive_write_open_filename_w(a, gzFile.path().toStdWString().c_str()), ARCHIVE_OK);

        const auto contents = testFile1.fileContents();

        archive_entry_set_size(entry, testFile1.fileSize());
        archive_entry_set_filetype(entry, AE_IFREG);
        // Sadly, libarchive cannot set the filename in a gzip archive
        //archive_entry_set_pathname_utf8(entry, "test1.txt");

        QCOMPARE(archive_write_header(a, entry), ARCHIVE_OK);
        QCOMPARE(archive_write_data(a, contents->data(), contents->size()), contents->size());
        QCOMPARE(archive_write_close(a), ARCHIVE_OK);
        QCOMPARE(archive_write_free(a), ARCHIVE_OK);

        QVERIFY(gzFile.isFile() && gzFile.fileSize() > 0);

        // Unarchive using Utils::Unarchiver
        Unarchiver unarchiver;
        unarchiver.setArchive(gzFile);
        unarchiver.setDestination(FilePath::fromString(tempDir.path() + "/unarchived"));
        unarchiver.start();
        const Result<> r = unarchiver.result();

        if (!r)
            qWarning() << "ERROR:" << r.error();

        QVERIFY(r);

        ScopedFilePath unarchivedFile = FilePath::fromString(tempDir.path() + "/unarchived/data");
        QVERIFY(unarchivedFile.isFile());
        QCOMPARE(unarchivedFile.fileContents(), "Hello World!");
    }

    void tst_gzip_with_name()
    {
        // This is a gzip file with an uncompressed file name of "test-data.txt"
        // The content is "This file is called test-data.txt\n"
        static const unsigned char test_data_txt_gz[66]
            = {0x1f, 0x8b, 0x08, 0x08, 0xde, 0x40, 0xd9, 0x67, 0x00, 0x03, 0x74, 0x65, 0x73, 0x74,
               0x2d, 0x64, 0x61, 0x74, 0x61, 0x2e, 0x74, 0x78, 0x74, 0x00, 0x0b, 0xc9, 0xc8, 0x2c,
               0x56, 0x48, 0xcb, 0xcc, 0x49, 0x55, 0x00, 0xd2, 0xc9, 0x89, 0x39, 0x39, 0xa9, 0x29,
               0x0a, 0x25, 0xa9, 0xc5, 0x25, 0xba, 0x29, 0x89, 0x25, 0x89, 0x7a, 0x25, 0x15, 0x25,
               0x5c, 0x00, 0x66, 0x86, 0xd0, 0x48, 0x22, 0x00, 0x00, 0x00};

        ScopedFilePath gzFile = FilePath::fromString(tempDir.path() + "/test-archive.gz");
        QVERIFY(gzFile.writeFileContents(QByteArray((char *) test_data_txt_gz, 66)));

        Unarchiver unarchiver;
        unarchiver.setArchive(gzFile);
        unarchiver.setDestination(FilePath::fromString(tempDir.path() + "/unarchived"));
        unarchiver.start();
        const Result<> r = unarchiver.result();

        if (!r)
            qWarning() << "ERROR:" << r.error();
        QVERIFY(r);

        ScopedFilePath unarchivedFile = FilePath::fromString(
            tempDir.path() + "/unarchived/test-data.txt");
        QVERIFY(unarchivedFile.isFile());
        QCOMPARE(unarchivedFile.fileContents(), "This file is called test-data.txt\n");
    }

private:
    QTemporaryDir tempDir;
};

} // namespace Utils

QTEST_GUILESS_MAIN(Utils::tst_unarchiver)

#include "tst_unarchiver.moc"
