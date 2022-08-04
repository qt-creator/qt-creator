// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include <QtTest>
#include <QDebug>
#include <QRandomGenerator>

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/link.h>

//TESTED_COMPONENT=src/libs/utils
using namespace Utils;

namespace QTest {
    template<>
    char *toString(const FilePath &filePath)
    {
        return qstrdup(filePath.toString().toLocal8Bit().constData());
    }
}

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
    void fromString_data();
    void fromString();
    void toString_data();
    void toString();
    void comparison_data();
    void comparison();
    void linkFromString_data();
    void linkFromString();
    void pathAppended_data();
    void pathAppended();
    void commonPath_data();
    void commonPath();
    void resolvePath_data();
    void resolvePath();
    void relativeChildPath_data();
    void relativeChildPath();
    void asyncLocalCopy();
    void startsWithDriveLetter();
    void startsWithDriveLetter_data();
    void onDevice();
    void onDevice_data();
    void plus();
    void plus_data();
    void url();
    void url_data();

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
    QTest::newRow("//./com1") << "//./com1" << "//./" << "";
    QTest::newRow("//?/path") << "//?/path" << "/" << "Qt 4 cannot handle this path.";
    QTest::newRow("/Global?\?/UNC/host") << "/Global?\?/UNC/host" << "/Global?\?/UNC/host"
                                        << "Qt 4 cannot handle this path.";
    QTest::newRow("//server/directory/file")
            << "//server/directory/file" << "//server/directory" << "";
    QTest::newRow("//server/directory") << "//server/directory" << "//server/" << "";
    QTest::newRow("//server") << "//server/" << "" << "";
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
    QTest::newRow("//./com1") << "/" << "//./com1" << false;
    QTest::newRow("//?/path") << "/" << "//?/path" << false;
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

    const FilePath child = FilePath::fromString(childPath);
    const FilePath parent = FilePath::fromString(path);

    QCOMPARE(child.isChildOf(parent), result);
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

void tst_fileutils::toString_data()
{
    QTest::addColumn<QString>("scheme");
    QTest::addColumn<QString>("host");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("result");
    QTest::addColumn<QString>("userResult");

    QTest::newRow("empty") << "" << "" << "" << "" << "";
    QTest::newRow("scheme") << "http" << "" << "" << QDir::rootPath() + "__qtc_devices__/http//./" << "http:///./";
    QTest::newRow("scheme-and-host") << "http" << "127.0.0.1" << "" << QDir::rootPath() + "__qtc_devices__/http/127.0.0.1/./" << "http://127.0.0.1/./";
    QTest::newRow("root") << "http" << "127.0.0.1" << "/" << QDir::rootPath() + "__qtc_devices__/http/127.0.0.1/" << "http://127.0.0.1/";

    QTest::newRow("root-folder") << "" << "" << "/" << "/" << "/";
    QTest::newRow("qtc-dev-root-folder") << "" << "" << QDir::rootPath() + "__qtc_devices__" << QDir::rootPath() + "__qtc_devices__" << QDir::rootPath() + "__qtc_devices__";
    QTest::newRow("qtc-dev-type-root-folder") << "" << "" << QDir::rootPath() + "__qtc_devices__/docker" << QDir::rootPath() + "__qtc_devices__/docker" << QDir::rootPath() + "__qtc_devices__/docker";
    QTest::newRow("qtc-root-folder") << "docker" << "alpine:latest" << "/" << QDir::rootPath() + "__qtc_devices__/docker/alpine:latest/" << "docker://alpine:latest/";
    QTest::newRow("qtc-root-folder-rel") << "docker" << "alpine:latest" << "" << QDir::rootPath() + "__qtc_devices__/docker/alpine:latest/./" << "docker://alpine:latest/./";
}

void tst_fileutils::toString()
{
    QFETCH(QString, scheme);
    QFETCH(QString, host);
    QFETCH(QString, path);
    QFETCH(QString, result);
    QFETCH(QString, userResult);

    FilePath filePath = FilePath::fromParts(scheme, host, path);
    QCOMPARE(filePath.toString(), result);
    QString cleanedOutput = filePath.needsDevice() ? filePath.toUserOutput() : QDir::cleanPath(filePath.toUserOutput());
    QCOMPARE(cleanedOutput, userResult);
}

void tst_fileutils::fromString_data()
{
    QTest::addColumn<QString>("input");

    QTest::addColumn<QString>("scheme");
    QTest::addColumn<QString>("host");
    QTest::addColumn<QString>("path");
    QTest::addColumn<OsType>("osType");

    QTest::newRow("empty") << "" << "" << "" << "" << HostOsInfo::hostOs();
    QTest::newRow("single-slash") << ":" << "" << "" << ":" << HostOsInfo::hostOs();
    QTest::newRow("single-colon") << "/" << "" << "" << "/" << HostOsInfo::hostOs();
    QTest::newRow("single-char") << "a" << "" << "" << "a" << HostOsInfo::hostOs();
    QTest::newRow("qrc-no-slash") << ":test.txt" << "" << "" << ":test.txt" << HostOsInfo::hostOs();
    QTest::newRow("qrc") << ":/test.txt" << "" << "" << ":/test.txt" << HostOsInfo::hostOs();
    QTest::newRow("unc-incomplete") << "//" << "" << "" << "" << OsType::OsTypeWindows;
    QTest::newRow("unc-incomplete-only-server") << "//server" << "" << "" << "//server/" << OsType::OsTypeWindows;
    QTest::newRow("unc-incomplete-only-server-2") << "//server/" << "" << "" << "//server/" << OsType::OsTypeWindows;
    QTest::newRow("unc-server-and-share") << "//server/share" << "" << "" << "//server/share" << OsType::OsTypeWindows;
    QTest::newRow("unc-server-and-share-2") << "//server/share/" << "" << "" << "//server/share/" << OsType::OsTypeWindows;
    QTest::newRow("unc-full") << "//server/share/test.txt" << "" << "" << "//server/share/test.txt" << OsType::OsTypeWindows;

    QTest::newRow("unix-root") << "/" << "" << "" << "/"  << OsType::OsTypeLinux;
    QTest::newRow("unix-folder") << "/tmp" << "" << "" << "/tmp" << OsType::OsTypeLinux;
    QTest::newRow("unix-folder-with-trailing-slash") << "/tmp/" << "" << "" << "/tmp/" << OsType::OsTypeLinux;

    QTest::newRow("windows-root") << "c:" << "" << "" << "c:/" << OsType::OsTypeWindows;
    QTest::newRow("windows-folder") << "c:\\Windows" << "" << "" << "c:/Windows" << OsType::OsTypeWindows;
    QTest::newRow("windows-folder-with-trailing-slash") << "c:\\Windows\\" << "" << "" << "c:/Windows\\" << OsType::OsTypeWindows;
    QTest::newRow("windows-folder-slash") << "C:/Windows" << "" << "" << "C:/Windows" << OsType::OsTypeWindows;

    QTest::newRow("docker-root-url") << "docker://1234/" << "docker" << "1234" << "/" << OsType::OsTypeLinux;
    QTest::newRow("docker-root-url-special") << QDir::rootPath() + "__qtc_devices__/docker/1234/" << "docker" << "1234" << "/" << OsType::OsTypeLinux;

    QTest::newRow("qtc-dev") << QDir::rootPath() + "__qtc_devices__" << "" << "" << QDir::rootPath() + "__qtc_devices__" << HostOsInfo::hostOs();
    QTest::newRow("qtc-dev-type") << QDir::rootPath() + "__qtc_devices__/docker" << "" << "" << QDir::rootPath() + "__qtc_devices__/docker" << HostOsInfo::hostOs();
    QTest::newRow("qtc-dev-type-dev") << QDir::rootPath() + "__qtc_devices__/docker/1234" << "docker" << "1234" << "/" << OsType::OsTypeLinux;

    QTest::newRow("cross-os") << QDir::rootPath() + "__qtc_devices__/docker/1234/c:/test.txt" << "docker" << "1234" << "c:/test.txt" << OsType::OsTypeWindows;
    QTest::newRow("cross-os-unclean") << QDir::rootPath() + "__qtc_devices__/docker/1234/c:\\test.txt" << "docker" << "1234" << "c:/test.txt" << OsType::OsTypeWindows;
    QTest::newRow("unc-full-in-docker") << QDir::rootPath() + "__qtc_devices__/docker/1234//server/share/test.txt" << "docker" << "1234" << "//server/share/test.txt" << OsType::OsTypeWindows;

    QTest::newRow("unc-dos-1") << "//?/c:" << "" << "" << "//?/c:" << OsType::OsTypeWindows;
    QTest::newRow("unc-dos-com") << "//./com1" << "" << "" << "//./com1" << OsType::OsTypeWindows;
}

void tst_fileutils::fromString()
{
    QFETCH(QString, input);
    QFETCH(QString, scheme);
    QFETCH(QString, host);
    QFETCH(QString, path);
    QFETCH(OsType, osType);

    FilePath filePath = FilePath::fromString(input, osType);
    QCOMPARE(filePath.scheme(), scheme);
    QCOMPARE(filePath.host(), host);
    QCOMPARE(filePath.path(), path);
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
        << "docker" << "1234abcdef" << "/bin/ls" << QDir::rootPath() + "__qtc_devices__/docker/1234abcdef/bin/ls";

    QTest::newRow("s5")
        << "docker" << "1234" << "/bin/ls" << QDir::rootPath() + "__qtc_devices__/docker/1234/bin/ls";

    // This is not a proper URL.
    QTest::newRow("s6")
        << "docker" << "1234" << "somefile" << QDir::rootPath() + "__qtc_devices__/docker/1234/./somefile";

    // Local Windows paths:
    QTest::newRow("w1") << "" << "" << "C:/data" << "C:/data";
    QTest::newRow("w2") << "" << "" << "C:/" << "C:/";
    QTest::newRow("w3") << "" << "" << "/Global?\?/UNC/host" << "/Global?\?/UNC/host";
    QTest::newRow("w4") << "" << "" << "//server/dir/file" << "//server/dir/file";
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

    FilePath copy = FilePath::fromParts(scheme, host, path);
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

    if (HostOsInfo::isWindowsHost()) {
        QTest::newRow("win-1") << "c:" << "/a/b" << "c:/a/b";
        QTest::newRow("win-2") << "c:/" << "/a/b" << "c:/a/b";
        QTest::newRow("win-3") << "c:/" << "a/b" << "c:/a/b";
    }
}

void tst_fileutils::resolvePath_data()
{
    QTest::addColumn<FilePath>("left");
    QTest::addColumn<FilePath>("right");
    QTest::addColumn<FilePath>("expected");

    QTest::newRow("empty") << FilePath() << FilePath() << FilePath();
    QTest::newRow("s0") << FilePath("/") << FilePath("b") << FilePath("/b");
    QTest::newRow("s1") << FilePath() << FilePath("b") << FilePath("b");
    QTest::newRow("s2") << FilePath("a") << FilePath() << FilePath("a");
    QTest::newRow("s3") << FilePath("a") << FilePath("b") << FilePath("a/b");
    QTest::newRow("s4") << FilePath("/a") << FilePath("/b") << FilePath("/b");
    QTest::newRow("s5") << FilePath("a") << FilePath("/b") << FilePath("/b");
    QTest::newRow("s6") << FilePath("/a") << FilePath("b") << FilePath("/a/b");
}

void tst_fileutils::resolvePath()
{
    QFETCH(FilePath, left);
    QFETCH(FilePath, right);
    QFETCH(FilePath, expected);

    const FilePath result = left.resolvePath(right);

    QCOMPARE(result, expected);
}

void tst_fileutils::relativeChildPath_data()
{
    QTest::addColumn<FilePath>("parent");
    QTest::addColumn<FilePath>("child");
    QTest::addColumn<FilePath>("expected");

    QTest::newRow("empty") << FilePath() << FilePath() << FilePath();

    QTest::newRow("simple-0") << FilePath("/a") << FilePath("/a/b") << FilePath("b");
    QTest::newRow("simple-1") << FilePath("/a/") << FilePath("/a/b") << FilePath("b");
    QTest::newRow("simple-2") << FilePath("/a") << FilePath("/a/b/c/d/e/f") << FilePath("b/c/d/e/f");

    QTest::newRow("not-0") << FilePath("/x") << FilePath("/a/b") << FilePath();
    QTest::newRow("not-1") << FilePath("/a/b/c") << FilePath("/a/b") << FilePath();

}

void tst_fileutils::relativeChildPath()
{
    QFETCH(FilePath, parent);
    QFETCH(FilePath, child);
    QFETCH(FilePath, expected);

    const FilePath result = child.relativeChildPath(parent);

    QCOMPARE(result, expected);
}

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
    QTest::newRow("Single path") << FilePaths{ p1 } << p1;
    QTest::newRow("3 identical paths") << FilePaths{ p1, p1, p1 } << p1;
    QTest::newRow("3 paths, common path") << FilePaths{ p1, p2, p3 } << p1;
    QTest::newRow("3 paths, no common path") << FilePaths{ p1, p2, p4 } << p5;
    QTest::newRow("3 paths, first is part of second") << FilePaths{ p4, p1, p3 } << p5;
    QTest::newRow("Common scheme") << FilePaths{ url1, url3 } << url6;
    QTest::newRow("Different scheme") << FilePaths{ url1, url2 } << FilePath();
    QTest::newRow("Common host") << FilePaths{ url4, url5 } << url4;
    QTest::newRow("Different host") << FilePaths{ url1, url3 } << url6;
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

void tst_fileutils::startsWithDriveLetter_data()
{
    if (!HostOsInfo::isWindowsHost()) {
        QSKIP("This test is only relevant on Windows");
    }

    QTest::addColumn<FilePath>("path");
    QTest::addColumn<bool>("expected");

    QTest::newRow("empty") << FilePath() << false;
    QTest::newRow("simple-win") << FilePath("c:/a", OsType::OsTypeWindows) << true;
    QTest::newRow("simple-linux") << FilePath("/c:/a", OsType::OsTypeLinux) << false;
    QTest::newRow("relative") << FilePath("a/b") << false;
}

void tst_fileutils::startsWithDriveLetter()
{
    QFETCH(FilePath, path);
    QFETCH(bool, expected);

    QCOMPARE(path.startsWithDriveLetter(), expected);
}

void tst_fileutils::onDevice_data() {
    QTest::addColumn<FilePath>("path");
    QTest::addColumn<FilePath>("templatePath");
    QTest::addColumn<FilePath>("expected");

    QTest::newRow("empty") << FilePath() << FilePath() << FilePath();
    QTest::newRow("same-local") << FilePath("/a/b") << FilePath("/a/b") << FilePath("/a/b");
    QTest::newRow("same-docker") << FilePath("docker://1234/a/b") << FilePath("docker://1234/e") << FilePath("docker://1234/a/b");

    QTest::newRow("docker-to-local") << FilePath("docker://1234/a/b") << FilePath("/c/d") << FilePath("/a/b");
    QTest::newRow("local-to-docker") << FilePath("/a/b") << FilePath("docker://1234/c/d") << FilePath("docker://1234/a/b");

}

void tst_fileutils::onDevice() {
    QFETCH(FilePath, path);
    QFETCH(FilePath, templatePath);
    QFETCH(FilePath, expected);

    QCOMPARE(path.onDevice(templatePath), expected);
}

void tst_fileutils::plus_data() {
    QTest::addColumn<FilePath>("left");
    QTest::addColumn<QString>("right");
    QTest::addColumn<FilePath>("expected");

    QTest::newRow("empty") << FilePath() << QString() << FilePath();
    QTest::newRow("empty-left") << FilePath() << "a" << FilePath("a");
    QTest::newRow("empty-right") << FilePath("a") << QString() << FilePath("a");
    QTest::newRow("add-root") << FilePath() << QString("/") << FilePath("/");
    QTest::newRow("add-root-and-more") << FilePath() << QString("/test/blah") << FilePath("/test/blah");
    QTest::newRow("add-extension") << FilePath::fromString("/a") << QString(".txt") << FilePath("/a.txt");
    QTest::newRow("trailing-slash") << FilePath::fromString("/a") << QString("b/") << FilePath("/ab/");
    QTest::newRow("slash-trailing-slash") << FilePath::fromString("/a/") << QString("b/") << FilePath("/a/b/");
}

void tst_fileutils::plus()
{
    QFETCH(FilePath, left);
    QFETCH(QString, right);
    QFETCH(FilePath, expected);

    const FilePath result = left + right;

    QCOMPARE(expected, result);
}

void tst_fileutils::url()
{
    QFETCH(QString, url);
    QFETCH(QString, expectedScheme);
    QFETCH(QString, expectedHost);
    QFETCH(QString, expectedPath);

    const FilePath result = FilePath::fromString(url);
    QCOMPARE(result.scheme(), expectedScheme);
    QCOMPARE(result.host(), expectedHost);
    QCOMPARE(result.path(), expectedPath);
}

void tst_fileutils::url_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("expectedScheme");
    QTest::addColumn<QString>("expectedHost");
    QTest::addColumn<QString>("expectedPath");
    QTest::newRow("empty") << QString() << QString() << QString() << QString();
    QTest::newRow("simple-file") << QString("file:///a/b") << QString("file") << QString() << QString("/a/b");
    QTest::newRow("simple-file-root") << QString("file:///") << QString("file") << QString() << QString("/");
    QTest::newRow("simple-docker") << QString("docker://1234/a/b") << QString("docker") << QString("1234") << QString("/a/b");
    QTest::newRow("simple-ssh") << QString("ssh://user@host/a/b") << QString("ssh") << QString("user@host") << QString("/a/b");
    QTest::newRow("simple-ssh-with-port") << QString("ssh://user@host:1234/a/b") << QString("ssh") << QString("user@host:1234") << QString("/a/b");
    QTest::newRow("http-qt.io") << QString("http://qt.io") << QString("http") << QString("qt.io") << QString();
    QTest::newRow("http-qt.io-index.html") << QString("http://qt.io/index.html") << QString("http") << QString("qt.io") << QString("/index.html");
}

QTEST_GUILESS_MAIN(tst_fileutils)
#include "tst_fileutils.moc"
