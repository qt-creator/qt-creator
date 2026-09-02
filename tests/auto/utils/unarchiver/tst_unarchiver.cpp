// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QRandomGenerator>
#include <QTest>

#include <utils/fsengine/fsengine.h>
#include <utils/hostosinfo.h>
#include <utils/unarchiver.h>

#ifdef Q_OS_UNIX
#include <sys/stat.h>
#endif

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

// write_archive() derives entry names from real files, so it cannot express a
// hostile name or a link entry.
struct RawEntry
{
    QByteArray pathname;
    QByteArray content;
    __LA_MODE_T type = AE_IFREG;
    __LA_MODE_T perm = 0644;
    QByteArray symlinkTarget;
    QByteArray hardlinkTarget;
};

void write_raw_archive(struct archive *a, const FilePath &archive, const QList<RawEntry> &entries)
{
    QCOMPARE(archive_write_open_filename_w(a, archive.path().toStdWString().c_str()), ARCHIVE_OK);
    for (const RawEntry &e : entries) {
        struct archive_entry *entry = archive_entry_new();
        archive_entry_set_pathname_utf8(entry, e.pathname.constData());
        archive_entry_set_filetype(entry, e.type);
        archive_entry_set_perm(entry, e.perm);
        if (e.type == AE_IFLNK)
            archive_entry_set_symlink_utf8(entry, e.symlinkTarget.constData());
        else if (!e.hardlinkTarget.isEmpty())
            archive_entry_set_hardlink_utf8(entry, e.hardlinkTarget.constData());
        else
            archive_entry_set_size(entry, e.content.size());
        QCOMPARE(archive_write_header(a, entry), ARCHIVE_OK);
        if (e.type == AE_IFREG && !e.content.isEmpty()) {
            QCOMPARE(archive_write_data(a, e.content.constData(), e.content.size()),
                     e.content.size());
        }
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

    void tst_traversal_is_contained()
    {
        struct archive *a = archive_write_new();
        archive_write_add_filter_none(a);
        archive_write_set_format_pax_restricted(a);

        const FilePath archive = FilePath::fromString(tempDir.path() + "/traversal.tar");
        write_raw_archive(a, archive, {{"../escaped.txt", "pwned", AE_IFREG, 0644, {}}});

        const FilePath destination = FilePath::fromString(tempDir.path() + "/dest-traversal");
        Unarchiver unarchiver;
        unarchiver.setArchive(archive);
        unarchiver.setDestination(destination);
        unarchiver.start();
        const Result<> r = unarchiver.result();

        const FilePath escaped = FilePath::fromString(tempDir.path() + "/escaped.txt");
        QVERIFY2(!escaped.exists(),
                 qPrintable("entry wrote outside the destination: " + escaped.toUserOutput()));
        QVERIFY2(!r, "a traversing entry was accepted");
    }

    // Windows-shaped names are rejected on every platform: pathAppended()
    // rewrites a backslash to a separator everywhere, so they are traversals
    // wherever the archive is unpacked.
    void tst_windows_shaped_names_data()
    {
        QTest::addColumn<QByteArray>("entryName");

        QTest::newRow("backslash") << QByteArray("..\\escaped.txt");
        QTest::newRow("mixed separators") << QByteArray("sub/..\\..\\escaped.txt");
        QTest::newRow("drive letter") << QByteArray("C:\\escaped.txt");
        QTest::newRow("drive letter, forward slash") << QByteArray("C:/escaped.txt");
        QTest::newRow("UNC") << QByteArray("\\\\server\\share\\escaped.txt");
    }

    void tst_windows_shaped_names()
    {
        QFETCH(QByteArray, entryName);

        struct archive *a = archive_write_new();
        archive_write_add_filter_none(a);
        archive_write_set_format_pax_restricted(a);

        const FilePath archive = FilePath::fromString(tempDir.path() + "/windows-shaped.tar");
        write_raw_archive(a, archive, {{entryName, "pwned", AE_IFREG, 0644, {}}});

        Unarchiver unarchiver;
        unarchiver.setArchive(archive);
        unarchiver.setDestination(FilePath::fromString(tempDir.path() + "/dest-windows"));
        unarchiver.start();

        QVERIFY2(!unarchiver.result(), "a Windows-shaped traversal was accepted");
        QVERIFY(!FilePath::fromString(tempDir.path() + "/escaped.txt").exists());
    }

    void tst_symlink_does_not_escape()
    {
        if (HostOsInfo::isWindowsHost())
            QSKIP("Creating a symlink needs privileges there, so the check would be vacuous.");

        struct archive *a = archive_write_new();
        archive_write_add_filter_none(a);
        archive_write_set_format_pax_restricted(a);

        const FilePath outside = FilePath::fromString(tempDir.path() + "/outside");
        QVERIFY(outside.createDir());

        const FilePath archive = FilePath::fromString(tempDir.path() + "/symlink.tar");
        write_raw_archive(
            a,
            archive,
            {{"link", {}, AE_IFLNK, 0777, outside.toFSPathString().toUtf8()},
             {"link/pwned.txt", "pwned", AE_IFREG, 0644, {}}});

        Unarchiver unarchiver;
        unarchiver.setArchive(archive);
        unarchiver.setDestination(FilePath::fromString(tempDir.path() + "/dest-symlink"));
        unarchiver.start();

        QVERIFY2(!unarchiver.result(), "a link pointing out of the destination was accepted");

        const FilePath escaped = outside / "pwned.txt";
        QVERIFY2(!escaped.exists(),
                 qPrintable("wrote through a symlink out of the destination: "
                            + escaped.toUserOutput()));
    }

    void tst_hardlink_is_resolved_in_the_destination()
    {
        if (HostOsInfo::isWindowsHost())
            QSKIP("Creating a hard link needs privileges there, so the check would be vacuous.");

        struct archive *a = archive_write_new();
        archive_write_add_filter_none(a);
        archive_write_set_format_pax_restricted(a);

        const FilePath archive = FilePath::fromString(tempDir.path() + "/hardlink.tar");
        write_raw_archive(
            a,
            archive,
            {{"target.txt", "linked", AE_IFREG, 0644, {}, {}},
             {"link.txt", {}, AE_IFREG, 0644, {}, "target.txt"}});

        const FilePath destination = FilePath::fromString(tempDir.path() + "/dest-hardlink");
        Unarchiver unarchiver;
        unarchiver.setArchive(archive);
        unarchiver.setDestination(destination);
        unarchiver.start();
        const Result<> r = unarchiver.result();
        if (!r)
            QFAIL(qPrintable(r.error()));

        QCOMPARE((destination / "link.txt").fileContents(), "linked");
    }

    void tst_colon_in_name_is_not_a_drive_letter()
    {
        if (HostOsInfo::isWindowsHost())
            QSKIP("A colon cannot occur in a file name there.");

        struct archive *a = archive_write_new();
        archive_write_add_filter_none(a);
        archive_write_set_format_pax_restricted(a);

        const FilePath archive = FilePath::fromString(tempDir.path() + "/colon.tar");
        write_raw_archive(a, archive, {{"a:b", "kept", AE_IFREG, 0644, {}, {}}});

        const FilePath destination = FilePath::fromString(tempDir.path() + "/dest-colon");
        Unarchiver unarchiver;
        unarchiver.setArchive(archive);
        unarchiver.setDestination(destination);
        unarchiver.start();
        const Result<> r = unarchiver.result();
        if (!r)
            QFAIL(qPrintable(r.error()));

        QCOMPARE((destination / "a:b").fileContents(), "kept");
    }

    void tst_destination_reached_through_a_symlink()
    {
        if (HostOsInfo::isWindowsHost())
            QSKIP("Creating a symlink needs privileges there, so the check would be vacuous.");

        const FilePath real = FilePath::fromString(tempDir.path() + "/real-root");
        QVERIFY(real.createDir());
        const FilePath link = FilePath::fromString(tempDir.path() + "/linked-root");
        QVERIFY(real.createSymLink(link));

        struct archive *a = archive_write_new();
        archive_write_add_filter_none(a);
        archive_write_set_format_pax_restricted(a);

        const FilePath archive = FilePath::fromString(tempDir.path() + "/linked-dest.tar");
        write_raw_archive(a, archive, {{"sub/file.txt", "reached", AE_IFREG, 0644, {}, {}}});

        Unarchiver unarchiver;
        unarchiver.setArchive(archive);
        unarchiver.setDestination(link / "dest");
        unarchiver.start();
        const Result<> r = unarchiver.result();
        if (!r)
            QFAIL(qPrintable(r.error()));

        QCOMPARE((real / "dest/sub/file.txt").fileContents(), "reached");
    }

    void tst_setuid_bit_is_not_restored()
    {
#ifndef Q_OS_UNIX
        QSKIP("setuid is a Unix concept.");
#else
        struct archive *a = archive_write_new();
        archive_write_add_filter_none(a);
        archive_write_set_format_pax_restricted(a);

        const FilePath archive = FilePath::fromString(tempDir.path() + "/setuid.tar");
        write_raw_archive(a, archive, {{"suid", "x", AE_IFREG, 04755, {}}});

        const FilePath destination = FilePath::fromString(tempDir.path() + "/dest-setuid");
        Unarchiver unarchiver;
        unarchiver.setArchive(archive);
        unarchiver.setDestination(destination);
        unarchiver.start();
        const Result<> extraction = unarchiver.result();

        const FilePath extracted = destination / "suid";
        if (!extraction) {
            // Refusing the entry is an acceptable outcome - restoring the mode
            // is not permitted everywhere. A file carrying the bit is not.
            QVERIFY2(!extracted.exists(), qPrintable(extraction.error()));
            return;
        }

        QVERIFY(extracted.isFile());
        struct stat st;
        QCOMPARE(::stat(extracted.toFSPathString().toUtf8().constData(), &st), 0);
        QVERIFY2(!(st.st_mode & (S_ISUID | S_ISGID)),
                 qPrintable(QString("extracted file kept mode %1").arg(st.st_mode, 0, 8)));
#endif
    }

private:
    QTemporaryDir tempDir;
};

} // namespace Utils

QTEST_GUILESS_MAIN(Utils::tst_unarchiver)

#include "tst_unarchiver.moc"
