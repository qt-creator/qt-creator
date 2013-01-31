/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
};

void tst_fileutils::parentDir_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("parentPath");
    QTest::addColumn<QString>("expectFailMessage");

    QTest::newRow("empty path") << QString() << QString() << QString();
    QTest::newRow("root only") << QString::fromLatin1("/") << QString() << QString();
    QTest::newRow("//") << QString::fromLatin1("//") << QString() << QString();
    QTest::newRow("/tmp/dir") << QString::fromLatin1("/tmp/dir") << QString::fromLatin1("/tmp") << QString();
    QTest::newRow("relative/path") << QString::fromLatin1("relative/path") << QString::fromLatin1("relative") << QString();
    QTest::newRow("relativepath") << QString::fromLatin1("relativepath") << QString::fromLatin1(".")
                                  << QString::fromLatin1("see QTBUG-23892");

    // Windows stuff:
#ifdef Q_OS_WIN
    QTest::newRow("C:/data") << QString::fromLatin1("C:/data") << QString::fromLatin1("C:/") << QString();
    QTest::newRow("C:/") << QString::fromLatin1("C:/") << QString() << QString();
    QTest::newRow("//./com1") << QString::fromLatin1("//./com1") << QString::fromLatin1("/") << QString();
    QTest::newRow("//?/path") << QString::fromLatin1("//?/path") << QString::fromLatin1("/")
                              << QString::fromLatin1("Qt 4 can not handle this path.");
    QTest::newRow("/Global??/UNC/host") << QString::fromLatin1("/Global??/UNC/host")
                                        << QString::fromLatin1("/Global??/UNC/host")
                                        << QString::fromLatin1("Qt 4 can not handle this path.");
    QTest::newRow("//server/directory/file")
            << QString::fromLatin1("//server/directory/file") << QString::fromLatin1("//server/directory") << QString();
    QTest::newRow("//server/directory")
            << QString::fromLatin1("//server/directory") << QString::fromLatin1("//server") << QString();
    QTest::newRow("//server")
            << QString::fromLatin1("//server") << QString() << QString();
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

    QTest::newRow("empty path") << QString() << QString::fromLatin1("/tmp") << false;
    QTest::newRow("root only") << QString::fromLatin1("/") << QString::fromLatin1("/tmp") << true;
    QTest::newRow("/tmp/dir") << QString::fromLatin1("/tmp") << QString::fromLatin1("/tmp/dir") << true;
    QTest::newRow("relative/path") << QString::fromLatin1("relative") << QString::fromLatin1("relative/path") << true;
    QTest::newRow("/tmpdir") << QString::fromLatin1("/tmp") << QString::fromLatin1("/tmpdir") << false;
    QTest::newRow("same") << QString::fromLatin1("/tmp/dir") << QString::fromLatin1("/tmp/dir") << false;

    // Windows stuff:
#ifdef Q_OS_WIN
    QTest::newRow("C:/data") << QString::fromLatin1("C:/") << QString::fromLatin1("C:/data") << true;
    QTest::newRow("C:/") << QString() << QString::fromLatin1("C:/") << false;
    QTest::newRow("//./com1") << QString::fromLatin1("/") << QString::fromLatin1("//./com1") << true;
    QTest::newRow("//?/path") << QString::fromLatin1("/") << QString::fromLatin1("//?/path") << true;
    QTest::newRow("/Global??/UNC/host") << QString::fromLatin1("/Global??/UNC/host")
                                        << QString::fromLatin1("/Global??/UNC/host/file") << true;
    QTest::newRow("//server/directory/file")
            << QString::fromLatin1("//server/directory") << QString::fromLatin1("//server/directory/file") << true;
    QTest::newRow("//server/directory")
            << QString::fromLatin1("//server") << QString::fromLatin1("//server/directory") << true;
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

QTEST_APPLESS_MAIN(tst_fileutils)
#include "tst_fileutils.moc"
