// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QRandomGenerator>
#include <QtTest>

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
    void writeAndReadArchive(archive *a)
    {
        // Create test zip using libarchive
        ScopedFilePath zipFile
            = *FilePath::fromString(tempDir.path() + "/test-archive").createTempFile();

        ScopedFilePath testFile1 = FilePath::fromString(tempDir.path() + "/test1.txt");
        testFile1.writeFileContents("Hello World!");

        ScopedFilePath testFile2 = FilePath::fromString(tempDir.path() + "/test2.txt");
        testFile2.writeFileContents("Hello World again!");

        write_archive(a, zipFile, {testFile1, testFile2});

        QVERIFY(zipFile.isFile() && zipFile.fileSize() > 0);

        // Unarchive using Utils::Unarchiver
        Unarchiver unarchiver;
        unarchiver.setArchive(zipFile);
        unarchiver.setDestination(FilePath::fromString(tempDir.path() + "/unarchived"));
        unarchiver.start();
        const Result r = unarchiver.result();

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

private slots:
    void initTestCase() { QVERIFY(tempDir.isValid()); }

    void tst_tar_gz()
    {
        struct archive *a = archive_write_new();
        archive_write_add_filter_gzip(a);
        archive_write_set_format_pax_restricted(a);
        writeAndReadArchive(a);
    }

    void tst_tar_bz2()
    {
        struct archive *a = archive_write_new();
        archive_write_add_filter_bzip2(a);
        archive_write_set_format_pax_restricted(a);
        writeAndReadArchive(a);
    }

    void tst_7z()
    {
        struct archive *a = archive_write_new();
        archive_write_add_filter_none(a);
        archive_write_set_format_7zip(a);
        writeAndReadArchive(a);
    }

    void tst_zip()
    {
        struct archive *a = archive_write_new();
        archive_write_add_filter_none(a);
        archive_write_set_format_zip(a);
        writeAndReadArchive(a);
    }

private:
    QTemporaryDir tempDir;
};

} // namespace Utils

QTEST_GUILESS_MAIN(Utils::tst_unarchiver)

#include "tst_unarchiver.moc"
