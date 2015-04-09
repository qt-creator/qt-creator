/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlprojectitem.h"
#include "filefilteritems.h"
#include "qmlprojectfileformat.h"
#include <QtTest>

//TESTED_COMPONENT=src/plugins/qmlprojectmanager/fileformat

using namespace QmlProjectManager;

class tst_FileFormat : public QObject
{
    Q_OBJECT
public:
    tst_FileFormat();

private slots:
    void testFileFilter();
    void testMatchesFile();
    void testLibraryPaths();
    void testMainFile();
};

tst_FileFormat::tst_FileFormat()
{
}

QString testDataDir = QLatin1String(SRCDIR "/data");

static QmlProjectItem *loadQmlProject(QString name, QString *error)
{
    return QmlProjectFileFormat::parseProjectFile(
                Utils::FileName::fromString(testDataDir).appendPath(name).appendString(".qmlproject"), error);
}

void tst_FileFormat::testFileFilter()
{
    QString error;
    //
    // Search for qml files in directory + subdirectories
    //
    {
        QmlProjectItem *project = loadQmlProject(QLatin1String("testFileFilter1"), &error);
        QVERIFY(project);
        QVERIFY(error.isEmpty());

        project->setSourceDirectory(testDataDir);

        QStringList expectedFiles(QStringList() << testDataDir + "/file1.qml"
                                                << testDataDir + "/file2.qml"
                                                << testDataDir + "/subdir/file3.qml");
        QCOMPARE(project->files().toSet(), expectedFiles.toSet());
        delete project;
    }

    //
    // search for all qml files in directory
    //
    {
        QmlProjectItem *project = loadQmlProject(QLatin1String("testFileFilter2"), &error);
        QVERIFY(project);
        QVERIFY(error.isEmpty());

        project->setSourceDirectory(testDataDir);

        QStringList expectedFiles(QStringList() << testDataDir + "/file1.qml"
                                                << testDataDir + "/file2.qml");
        QCOMPARE(project->files().toSet(), expectedFiles.toSet());
        delete project;
    }

    //
    // search for all qml files in subdirectory
    //
    {
        QmlProjectItem *project = loadQmlProject(QLatin1String("testFileFilter3"), &error);
        QVERIFY(project);
        QVERIFY(error.isEmpty());

        project->setSourceDirectory(testDataDir);

        QStringList expectedFiles(QStringList() <<  testDataDir + "/subdir/file3.qml");
        QCOMPARE(project->files().toSet(), expectedFiles.toSet());
        delete project;
    }

    //
    // multiple entries
    //
    {
        QmlProjectItem *project = loadQmlProject(QLatin1String("testFileFilter4"), &error);
        QVERIFY(project);
        QVERIFY(error.isEmpty());

        project->setSourceDirectory(testDataDir);

        QStringList expectedFiles(QStringList() << testDataDir + "/file1.qml"
                                                << testDataDir + "/file2.qml"
                                                << testDataDir + "/subdir/file3.qml");
        QCOMPARE(project->files().size(), 3);
        QCOMPARE(project->files().toSet(), expectedFiles.toSet());
        delete project;
    }

    //
    // include specific list
    //
    {
        QmlProjectItem *project = loadQmlProject(QLatin1String("testFileFilter5"), &error);
        QVERIFY(project);
        QVERIFY(error.isEmpty());

        project->setSourceDirectory(testDataDir);

        QStringList expectedFiles(QStringList() << testDataDir + "/file1.qml"
                                                << testDataDir + "/file2.qml");
        QCOMPARE(project->files().toSet(), expectedFiles.toSet());
        delete project;
    }

    //
    // include specific list
    //
    {
        QmlProjectItem *project = loadQmlProject(QLatin1String("testFileFilter6"), &error);
        QVERIFY(project);
        QVERIFY(error.isEmpty());

        project->setSourceDirectory(testDataDir);

        QStringList expectedFiles(QStringList() << testDataDir + "/image.gif");
        qDebug() << project->files().toSet() << expectedFiles.toSet();
        QCOMPARE(project->files().toSet(), expectedFiles.toSet());
        delete project;
    }

    //
    // use wildcards
    //
    {
        QmlProjectItem *project = loadQmlProject(QLatin1String("testFileFilter7"), &error);
        QVERIFY(project);
        QVERIFY(error.isEmpty());

        project->setSourceDirectory(testDataDir);

        QStringList expectedFiles(QStringList() << testDataDir + "/image.gif");
        qDebug() << project->files().toSet() << expectedFiles.toSet();
        QCOMPARE(project->files().toSet(), expectedFiles.toSet());
        delete project;
    }

    //
    // use Files element (1.1)
    //
    {
        QmlProjectItem *project = loadQmlProject(QLatin1String("testFileFilter8"), &error);
        QVERIFY(project);
        QVERIFY(error.isEmpty());

        project->setSourceDirectory(testDataDir);

        QStringList expectedFiles(QStringList() << testDataDir + "/image.gif");
        qDebug() << project->files().toSet() << expectedFiles.toSet();
        QCOMPARE(project->files().toSet(), expectedFiles.toSet());
        delete project;
    }
}

void tst_FileFormat::testMatchesFile()
{
    QString error;
    //
    // search for qml files in local directory
    //
    QmlProjectItem *project = loadQmlProject(QLatin1String("testMatchesFile"), &error);
    QVERIFY(project);
    QVERIFY(error.isEmpty());

    project->setSourceDirectory(testDataDir);

    QVERIFY(project->matchesFile(testDataDir + "/file1.qml"));
    QVERIFY(project->matchesFile(testDataDir + "/notyetexistingfile.qml"));
    QVERIFY(project->matchesFile(testDataDir + "/subdir/notyetexistingfile.qml"));
    QVERIFY(project->matchesFile(testDataDir + "/script.js"));
    QVERIFY(!project->matchesFile(testDataDir + "/script.css"));
    delete project;
}

void tst_FileFormat::testLibraryPaths()
{
    QString error;
    //
    // search for qml files in local directory
    //
    QmlProjectItem *project = loadQmlProject(QLatin1String("testLibraryPaths"), &error);
    QVERIFY(project);
    QVERIFY(error.isEmpty());

    project->setSourceDirectory(testDataDir);

    QStringList expectedPaths(QStringList() << SRCDIR "/otherLibrary"
                                            << SRCDIR "/data/library");
    qDebug() << expectedPaths << project->importPaths();
    QCOMPARE(project->importPaths().toSet(), expectedPaths.toSet());
    delete project;
}

void tst_FileFormat::testMainFile()
{
    QString error;
    //
    // search for qml files in local directory
    //
    QmlProjectItem *project = loadQmlProject(QLatin1String("testMainFile"), &error);
    QVERIFY(project);
    QVERIFY(error.isEmpty());

    QCOMPARE(project->mainFile(), QString("file1.qml"));
    delete project;
}

QTEST_MAIN(tst_FileFormat);
#include "tst_fileformat.moc"
