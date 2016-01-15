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

#include <utils/fileutils.h>

//TESTED_COMPONENT=src/libs/utils
using namespace Utils;

class tst_fileutils : public QObject
{
    Q_OBJECT

public:

private slots:
    void parentDir_data();
    void parentDir();
    void isChildOf_data();
    void isChildOf();
    void fileName_data();
    void fileName();
};

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
    QTest::newRow("//./com1") << "//./com1" << "/" << "";
    QTest::newRow("//?/path") << "//?/path" << "/" << "Qt 4 can not handle this path.";
    QTest::newRow("/Global?\?/UNC/host") << "/Global?\?/UNC/host" << "/Global?\?/UNC/host"
                                        << "Qt 4 can not handle this path.";
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

    FileName result = FileName::fromString(path).parentDir();
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

    bool res = FileName::fromString(childPath).isChildOf(FileName::fromString(path));
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
    QCOMPARE(FileName::fromString(path).fileName(components), result);
}

QTEST_APPLESS_MAIN(tst_fileutils)
#include "tst_fileutils.moc"
