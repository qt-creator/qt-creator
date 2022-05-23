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

#include <QtTest>
#include <QDebug>
#include <QRandomGenerator>

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/link.h>

//TESTED_COMPONENT=src/libs/utils
using namespace Utils;

class tst_fileutils : public QObject
{
    Q_OBJECT

signals:
    void asyncTestDone(); // test internal helper signal

private slots:
    void initTestCase();
    void parentDir_data();
    void parentDir();
    void isChildOf_data();
    void isChildOf();
    void fileName_data();
    void fileName();
    void calcRelativePath_data();
    void calcRelativePath();
    void relativePath_specials();
    void relativePath_data();
    void relativePath();
    void fromToString_data();
    void fromToString();
    void comparison_data();
    void comparison();
    void linkFromString_data();
    void linkFromString();
    void pathAppended_data();
    void pathAppended();

    void asyncLocalCopy();

private:
    QTemporaryDir tempDir;
    QString rootPath;
};

static void touch(const QDir &dir, const QString &filename, bool fill)
{
    QFile file(dir.absoluteFilePath(filename));
    file.open(QIODevice::WriteOnly);
    if (fill) {
        QRandomGenerator *random = QRandomGenerator::global();
        for (int i = 0; i < 10; ++i)
            file.write(QString::number(random->generate(), 16).toUtf8());
    }
    file.close();
}

void tst_fileutils::initTestCase()
{
    // initialize test for tst_fileutiles::relativePath*()
    QVERIFY(tempDir.isValid());
    rootPath = tempDir.path();
    QDir dir(rootPath);
    dir.mkpath("a/b/c/d");
    dir.mkpath("a/x/y/z");
    dir.mkpath("a/b/x/y/z");
    dir.mkpath("x/y/z");
    touch(dir, "a/b/c/d/file1.txt", false);
    touch(dir, "a/x/y/z/file2.txt", false);
    touch(dir, "a/file3.txt", false);
    touch(dir, "x/y/file4.txt", false);

    // initialize test for tst_fileutils::asyncLocalCopy()
    touch(dir, "x/y/fileToCopy.txt", true);
}

void tst_fileutils::parentDir_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("parentPath");
    QTest::addColumn<QString>("expectFailMessage");

    QTest::newRow("empty path") << "" << "" << "";
    QTest::newRow("root only") << "/" << "" << "";
    QTest::newRow("//") << "//" << "" << "";
    QTest::newRow("/tmp/dir") << "/tmp/dir" << "/tmp" << "";
    QTest::newRow("relative/path") << "relative/path" << "relative" << "";
    QTest::newRow("relativepath") << "relativepath" << "." << "";

    // Windows stuff:
#ifdef Q_OS_WIN
    QTest::newRow("C:/data") << "C:/data" << "C:/" << "";
    QTest::newRow("C:/") << "C:/" << "" << "";
    QTest::newRow("//./com1") << "//./com1" << "//." << "";
    QTest::newRow("//?/path") << "//?/path" << "/" << "Qt 4 cannot handle this path.";
    QTest::newRow("/Global?\?/UNC/host") << "/Global?\?/UNC/host" << "/Global?\?/UNC/host"
                                        << "Qt 4 cannot handle this path.";
    QTest::newRow("//server/directory/file")
            << "//server/directory/file" << "//server/directory" << "";
    QTest::newRow("//server/directory") << "//server/directory" << "//server" << "";
    QTest::newRow("//server") << "//server" << "" << "";
#endif
}

void tst_fileutils::parentDir()
{
    QFETCH(QString, path);
    QFETCH(QString, parentPath);
    QFETCH(QString, expectFailMessage);

    FilePath result = FilePath::fromString(path).parentDir();
    if (!expectFailMessage.isEmpty())
        QEXPECT_FAIL("", expectFailMessage.toUtf8().constData(), Continue);
    QCOMPARE(result.toString(), parentPath);
}

void tst_fileutils::isChildOf_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("childPath");
    QTest::addColumn<bool>("result");

    QTest::newRow("empty path") << "" << "/tmp" << false;
    QTest::newRow("root only") << "/" << "/tmp" << true;
    QTest::newRow("/tmp/dir") << "/tmp" << "/tmp/dir" << true;
    QTest::newRow("relative/path") << "relative" << "relative/path" << true;
    QTest::newRow("/tmpdir") << "/tmp" << "/tmpdir" << false;
    QTest::newRow("same") << "/tmp/dir" << "/tmp/dir" << false;

    // Windows stuff:
#ifdef Q_OS_WIN
    QTest::newRow("C:/data") << "C:/" << "C:/data" << true;
    QTest::newRow("C:/") << "" << "C:/" << false;
    QTest::newRow("//./com1") << "/" << "//./com1" << true;
    QTest::newRow("//?/path") << "/" << "//?/path" << true;
    QTest::newRow("/Global?\?/UNC/host") << "/Global?\?/UNC/host"
                                        << "/Global?\?/UNC/host/file" << true;
    QTest::newRow("//server/directory/file")
            << "//server/directory" << "//server/directory/file" << true;
    QTest::newRow("//server/directory")
            << "//server" << "//server/directory" << true;
#endif
}

void tst_fileutils::isChildOf()
{
    QFETCH(QString, path);
    QFETCH(QString, childPath);
    QFETCH(bool, result);

    bool res = FilePath::fromString(childPath).isChildOf(FilePath::fromString(path));
    QCOMPARE(res, result);
}

void tst_fileutils::fileName_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<int>("components");
    QTest::addColumn<QString>("result");

    QTest::newRow("empty 1") << "" << 0 << "";
    QTest::newRow("empty 2") << "" << 1 << "";
    QTest::newRow("basic") << "/foo/bar/baz" << 0 << "baz";
    QTest::newRow("2 parts") << "/foo/bar/baz" << 1 << "bar/baz";
    QTest::newRow("root no depth") << "/foo" << 0 << "foo";
    QTest::newRow("root full") << "/foo" << 1 << "/foo";
    QTest::newRow("root included") << "/foo/bar/baz" << 2 << "/foo/bar/baz";
    QTest::newRow("too many parts") << "/foo/bar/baz" << 5 << "/foo/bar/baz";
    QTest::newRow("windows root") << "C:/foo/bar/baz" << 2 << "C:/foo/bar/baz";
    QTest::newRow("smb share") << "//server/share/file" << 2 << "//server/share/file";
    QTest::newRow("no slashes") << "foobar" << 0 << "foobar";
    QTest::newRow("no slashes with depth") << "foobar" << 1 << "foobar";
    QTest::newRow("multiple slashes 1") << "/foo/bar////baz" << 0 << "baz";
    QTest::newRow("multiple slashes 2") << "/foo/bar////baz" << 1 << "bar////baz";
    QTest::newRow("multiple slashes 3") << "/foo////bar/baz" << 2 << "/foo////bar/baz";
    QTest::newRow("single char 1") << "/a/b/c" << 0 << "c";
    QTest::newRow("single char 2") << "/a/b/c" << 1 << "b/c";
    QTest::newRow("single char 3") << "/a/b/c" << 2 << "/a/b/c";
    QTest::newRow("slash at end 1") << "/a/b/" << 0 << "";
    QTest::newRow("slash at end 2") << "/a/b/" << 1 << "b/";
    QTest::newRow("slashes at end 1") << "/a/b//" << 0 << "";
    QTest::newRow("slashes at end 2") << "/a/b//" << 1 << "b//";
    QTest::newRow("root only 1") << "/" << 0 << "";
    QTest::newRow("root only 2") << "/" << 1 << "/";
}

void tst_fileutils::fileName()
{
    QFETCH(QString, path);
    QFETCH(int, components);
    QFETCH(QString, result);
    QCOMPARE(FilePath::fromString(path).fileNameWithPathComponents(components), result);
}

void tst_fileutils::calcRelativePath_data()
{
    QTest::addColumn<QString>("absolutePath");
    QTest::addColumn<QString>("anchorPath");
    QTest::addColumn<QString>("result");

    QTest::newRow("empty") << "" << "" << "";
    QTest::newRow("leftempty") << "" << "/" << "";
    QTest::newRow("rightempty") << "/" << "" << "";
    QTest::newRow("root") << "/" << "/" << ".";
    QTest::newRow("simple1") << "/a" << "/" << "a";
    QTest::newRow("simple2") << "/" << "/a" << "..";
    QTest::newRow("simple3") << "/a" << "/a" << ".";
    QTest::newRow("extraslash1") << "/a/b/c" << "/a/b/c" << ".";
    QTest::newRow("extraslash2") << "/a/b/c" << "/a/b/c/" << ".";
    QTest::newRow("extraslash3") << "/a/b/c/" << "/a/b/c" << ".";
    QTest::newRow("normal1") << "/a/b/c" << "/a/x" << "../b/c";
    QTest::newRow("normal2") << "/a/b/c" << "/a/x/y" << "../../b/c";
    QTest::newRow("normal3") << "/a/b/c" << "/x/y" << "../../a/b/c";
}

void tst_fileutils::calcRelativePath()
{
    QFETCH(QString, absolutePath);
    QFETCH(QString, anchorPath);
    QFETCH(QString, result);
    QString relativePath = Utils::FilePath::calcRelativePath(absolutePath, anchorPath);
    QCOMPARE(relativePath, result);
}

void tst_fileutils::relativePath_specials()
{
    QString path = FilePath("").relativePath("").toString();
    QCOMPARE(path, "");
}

void tst_fileutils::relativePath_data()
{
    QTest::addColumn<QString>("relative");
    QTest::addColumn<QString>("anchor");
    QTest::addColumn<QString>("result");

    QTest::newRow("samedir") << "/" << "/" << ".";
    QTest::newRow("samedir_but_file") << "a/b/c/d/file1.txt" << "a/b/c/d" << "file1.txt";
    QTest::newRow("samedir_but_file2") << "a/b/c/d" << "a/b/c/d/file1.txt" << ".";
    QTest::newRow("dir2dir_1") << "a/b/c/d" << "a/x/y/z" << "../../../b/c/d";
    QTest::newRow("dir2dir_2") << "a/b" <<"a/b/c" << "..";
    QTest::newRow("file2file_1") << "a/b/c/d/file1.txt" << "a/file3.txt" << "b/c/d/file1.txt";
    QTest::newRow("dir2file_1") << "a/b/c" << "a/x/y/z/file2.txt" << "../../../b/c";
    QTest::newRow("file2dir_1") << "a/b/c/d/file1.txt" << "x/y" << "../../a/b/c/d/file1.txt";
}

void tst_fileutils::relativePath()
{
    QFETCH(QString, relative);
    QFETCH(QString, anchor);
    QFETCH(QString, result);
    FilePath actualPath = FilePath::fromString(rootPath + "/" + relative)
                              .relativePath(FilePath::fromString(rootPath + "/" + anchor));
    QCOMPARE(actualPath.toString(), result);
}

void tst_fileutils::fromToString_data()
{
    QTest::addColumn<QString>("scheme");
    QTest::addColumn<QString>("host");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("full");

    QTest::newRow("s0") << "" << "" << "" << "";
    QTest::newRow("s1") << "" << "" << "/" << "/";
    QTest::newRow("s2") << "" << "" << "a/b/c/d" << "a/b/c/d";
    QTest::newRow("s3") << "" << "" << "/a/b" << "/a/b";

    QTest::newRow("s4")
        << "docker" << "1234abcdef" << "/bin/ls" << "docker://1234abcdef/bin/ls";

    QTest::newRow("s5")
        << "docker" << "1234" << "/bin/ls" << "docker://1234/bin/ls";

    // This is not a proper URL.
    QTest::newRow("s6")
        << "docker" << "1234" << "somefile" << "docker://1234/./somefile";

    // Local Windows paths:
    QTest::newRow("w1") << "" << "" << "C:/data" << "C:/data";
    QTest::newRow("w2") << "" << "" << "C:/" << "C:/";
    QTest::newRow("w3") << "" << "" << "//./com1" << "//./com1";
    QTest::newRow("w4") << "" << "" << "//?/path" << "//?/path";
    QTest::newRow("w5") << "" << "" << "/Global?\?/UNC/host" << "/Global?\?/UNC/host";
    QTest::newRow("w6") << "" << "" << "//server/dir/file" << "//server/dir/file";
    QTest::newRow("w7") << "" << "" << "//server/dir" << "//server/dir";
    QTest::newRow("w8") << "" << "" << "//server" << "//server";

    // Not supported yet: "Remote" windows. Would require use of e.g.
    // FileUtils::isRelativePath with support from the remote device
    // identifying itself as Windows.

    //  Actual   (filePath.path()): "/C:/data"
    //  Expected (path)           : "C:/data"

    //QTest::newRow("w9") << "scheme" << "server" << "C:/data"
    //    << "scheme://server/C:/data";
}

void tst_fileutils::fromToString()
{
    QFETCH(QString, full);
    QFETCH(QString, scheme);
    QFETCH(QString, host);
    QFETCH(QString, path);

    FilePath filePath = FilePath::fromString(full);

    QCOMPARE(filePath.toString(), full);

    QCOMPARE(filePath.scheme(), scheme);
    QCOMPARE(filePath.host(), host);
    QCOMPARE(filePath.path(), path);


    FilePath copy = filePath;
    copy.setHost(host);
    QCOMPARE(copy.toString(), full);

    copy.setScheme(scheme);
    QCOMPARE(copy.toString(), full);

    copy.setPath(path);
    QCOMPARE(copy.toString(), full);
}

void tst_fileutils::comparison()
{
    QFETCH(QString, left);
    QFETCH(QString, right);
    QFETCH(bool, hostSensitive);
    QFETCH(bool, expected);

    HostOsInfo::setOverrideFileNameCaseSensitivity(
        hostSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);

    FilePath l = FilePath::fromString(left);
    FilePath r = FilePath::fromString(right);
    QCOMPARE(l == r, expected);
}

void tst_fileutils::comparison_data()
{
    QTest::addColumn<QString>("left");
    QTest::addColumn<QString>("right");
    QTest::addColumn<bool>("hostSensitive");
    QTest::addColumn<bool>("expected");

    QTest::newRow("r1") << "Abc" << "abc" << true << false;
    QTest::newRow("r2") << "Abc" << "abc" << false << true;
    QTest::newRow("r3") << "x://y/Abc" << "x://y/abc" << true << false;
    QTest::newRow("r4") << "x://y/Abc" << "x://y/abc" << false << false;

    QTest::newRow("s1") << "abc" << "abc" << true << true;
    QTest::newRow("s2") << "abc" << "abc" << false << true;
    QTest::newRow("s3") << "x://y/abc" << "x://y/abc" << true << true;
    QTest::newRow("s4") << "x://y/abc" << "x://y/abc" << false << true;
}

void tst_fileutils::linkFromString()
{
    QFETCH(QString, testFile);
    QFETCH(Utils::FilePath, filePath);
    QFETCH(QString, postfix);
    QFETCH(int, line);
    QFETCH(int, column);
    QString extractedPostfix;
    Link link = Link::fromString(testFile, true, &extractedPostfix);
    QCOMPARE(link.targetFilePath, filePath);
    QCOMPARE(extractedPostfix, postfix);
    QCOMPARE(link.targetLine, line);
    QCOMPARE(link.targetColumn, column);
}

void tst_fileutils::linkFromString_data()
{
    QTest::addColumn<QString>("testFile");
    QTest::addColumn<Utils::FilePath>("filePath");
    QTest::addColumn<QString>("postfix");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("column");

    QTest::newRow("no-line-no-column")
        << QString("someFile.txt") << FilePath("someFile.txt")
        << QString() << -1 << -1;
    QTest::newRow(": at end") << QString::fromLatin1("someFile.txt:")
                              << FilePath("someFile.txt")
                              << QString::fromLatin1(":") << 0 << -1;
    QTest::newRow("+ at end") << QString::fromLatin1("someFile.txt+")
                              << FilePath("someFile.txt")
                              << QString::fromLatin1("+") << 0 << -1;
    QTest::newRow(": for column") << QString::fromLatin1("someFile.txt:10:")
                                  << FilePath("someFile.txt")
                                  << QString::fromLatin1(":10:") << 10 << -1;
    QTest::newRow("+ for column") << QString::fromLatin1("someFile.txt:10+")
                                  << FilePath("someFile.txt")
                                  << QString::fromLatin1(":10+") << 10 << -1;
    QTest::newRow(": and + at end")
        << QString::fromLatin1("someFile.txt:+") << FilePath("someFile.txt")
        << QString::fromLatin1(":+") << 0 << -1;
    QTest::newRow("empty line") << QString::fromLatin1("someFile.txt:+10")
                                << FilePath("someFile.txt")
                                << QString::fromLatin1(":+10") << 0 << 9;
    QTest::newRow(":line-no-column") << QString::fromLatin1("/some/path/file.txt:42")
                                     << FilePath("/some/path/file.txt")
                                     << QString::fromLatin1(":42") << 42 << -1;
    QTest::newRow("+line-no-column") << QString::fromLatin1("/some/path/file.txt+42")
                                     << FilePath("/some/path/file.txt")
                                     << QString::fromLatin1("+42") << 42 << -1;
    QTest::newRow(":line-:column") << QString::fromLatin1("/some/path/file.txt:42:3")
                                   << FilePath("/some/path/file.txt")
                                   << QString::fromLatin1(":42:3") << 42 << 2;
    QTest::newRow(":line-+column") << QString::fromLatin1("/some/path/file.txt:42+33")
                                   << FilePath("/some/path/file.txt")
                                   << QString::fromLatin1(":42+33") << 42 << 32;
    QTest::newRow("+line-:column") << QString::fromLatin1("/some/path/file.txt+142:30")
                                   << FilePath("/some/path/file.txt")
                                   << QString::fromLatin1("+142:30") << 142 << 29;
    QTest::newRow("+line-+column") << QString::fromLatin1("/some/path/file.txt+142+33")
                                   << FilePath("/some/path/file.txt")
                                   << QString::fromLatin1("+142+33") << 142 << 32;
    QTest::newRow("( at end") << QString::fromLatin1("/some/path/file.txt(")
                              << FilePath("/some/path/file.txt")
                              << QString::fromLatin1("(") << -1 << -1;
    QTest::newRow("(42 at end") << QString::fromLatin1("/some/path/file.txt(42")
                                << FilePath("/some/path/file.txt")
                                << QString::fromLatin1("(42") << 42 << -1;
    QTest::newRow("(42) at end") << QString::fromLatin1("/some/path/file.txt(42)")
                                 << FilePath("/some/path/file.txt")
                                 << QString::fromLatin1("(42)") << 42 << -1;
}

void tst_fileutils::pathAppended()
{
    QFETCH(QString, left);
    QFETCH(QString, right);
    QFETCH(QString, expected);

    const FilePath fleft = FilePath::fromString(left);
    const FilePath fexpected = FilePath::fromString(expected);

    const FilePath result = fleft.pathAppended(right);

    QCOMPARE(fexpected, result);
}

void tst_fileutils::pathAppended_data()
{
    QTest::addColumn<QString>("left");
    QTest::addColumn<QString>("right");
    QTest::addColumn<QString>("expected");

    QTest::newRow("p0") <<   ""  <<  ""   << "";
    QTest::newRow("p1") <<   ""  <<  "/"  << "/";
    QTest::newRow("p2") <<   ""  << "c/"  << "c/";
    QTest::newRow("p3") <<   ""  <<  "/d" << "/d";
    QTest::newRow("p4") <<   ""  << "c/d" << "c/d";

    QTest::newRow("r0") <<  "/"  <<  ""   << "/";
    QTest::newRow("r1") <<  "/"  <<  "/"  << "/";
    QTest::newRow("r2") <<  "/"  << "c/"  << "/c/";
    QTest::newRow("r3") <<  "/"  <<  "/d" << "/d";
    QTest::newRow("r4") <<  "/"  << "c/d" << "/c/d";

    QTest::newRow("s0") <<  "/b" <<  ""   << "/b";
    QTest::newRow("s1") <<  "/b" <<  "/"  << "/b/";
    QTest::newRow("s2") <<  "/b" << "c/"  << "/b/c/";
    QTest::newRow("s3") <<  "/b" <<  "/d" << "/b/d";
    QTest::newRow("s4") <<  "/b" << "c/d" << "/b/c/d";

    QTest::newRow("t0") << "a/"  <<  ""   << "a/";
    QTest::newRow("t1") << "a/"  <<  "/"  << "a/";
    QTest::newRow("t2") << "a/"  << "c/"  << "a/c/";
    QTest::newRow("t3") << "a/"  <<  "/d" << "a/d";
    QTest::newRow("t4") << "a/"  << "c/d" << "a/c/d";

    QTest::newRow("u0") << "a/b" <<  ""   << "a/b";
    QTest::newRow("u1") << "a/b" <<  "/"  << "a/b/";
    QTest::newRow("u2") << "a/b" << "c/"  << "a/b/c/";
    QTest::newRow("u3") << "a/b" <<  "/d" << "a/b/d";
    QTest::newRow("u4") << "a/b" << "c/d" << "a/b/c/d";
}

void tst_fileutils::asyncLocalCopy()
{
    const FilePath orig = FilePath::fromString(rootPath).pathAppended("x/y/fileToCopy.txt");
    QVERIFY(orig.exists());
    const FilePath dest = FilePath::fromString(rootPath).pathAppended("x/fileToCopyDest.txt");
    auto afterCopy = [&orig, &dest, this] (bool result) {
        QVERIFY(result);
        // check existence, size and content
        QVERIFY(dest.exists());
        QCOMPARE(dest.fileSize(), orig.fileSize());
        QCOMPARE(dest.fileContents(), orig.fileContents());
        emit asyncTestDone();
    };
    QSignalSpy spy(this, &tst_fileutils::asyncTestDone);
    orig.asyncCopyFile(afterCopy, dest);
    // we usually have already received the signal, but if it fails wait 3s
    QVERIFY(spy.count() == 1 || spy.wait(3000));
}

QTEST_MAIN(tst_fileutils)
#include "tst_fileutils.moc"
