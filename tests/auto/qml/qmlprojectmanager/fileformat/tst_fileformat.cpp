// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmlprojectitem.h"
#include "filefilteritems.h"
#include "qmlprojectfileformat.h"

#include <utils/algorithm.h>

#include <QtTest>

//TESTED_COMPONENT=src/plugins/qmlprojectmanager/fileformat

using namespace QmlProjectManager;

#define COMPARE_AS_SETS(actual, expected) \
    do {\
        if (!QTest::qCompare(Utils::toSet(actual), Utils::toSet(expected), #actual, #expected, __FILE__, __LINE__)) {\
            qDebug() << actual << "\nvs." << expected; \
            return;\
        }\
    } while (false)


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

static QString testDataDir = QLatin1String(SRCDIR "/data");

static QmlProjectItem *loadQmlProject(QString name, QString *error)
{
    return QmlProjectFileFormat::parseProjectFile(
                Utils::FilePath::fromString(testDataDir).pathAppended(name + ".qmlproject"), error);
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
        COMPARE_AS_SETS(project->files(), expectedFiles);
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
        COMPARE_AS_SETS(project->files(), expectedFiles);
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
        COMPARE_AS_SETS(project->files(), expectedFiles);
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
        COMPARE_AS_SETS(project->files(), expectedFiles);
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
        COMPARE_AS_SETS(project->files(), expectedFiles);
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
        COMPARE_AS_SETS(project->files(), expectedFiles);
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
        COMPARE_AS_SETS(project->files(), expectedFiles);
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
        COMPARE_AS_SETS(project->files(), expectedFiles);
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

    const QDir base(testDataDir);
    const QStringList expectedPaths({base.relativeFilePath(SRCDIR "/otherLibrary"),
                                     base.relativeFilePath(SRCDIR "/data/library")});
    COMPARE_AS_SETS(project->importPaths(), expectedPaths);
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

QTEST_GUILESS_MAIN(tst_FileFormat);
#include "tst_fileformat.moc"
