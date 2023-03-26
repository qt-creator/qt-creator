// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprojectitem.h"
#include "filefilteritems.h"
#include "qmlprojectfileformat.h"

#include <utils/algorithm.h>

#include <QtTest>

//TESTED_COMPONENT=src/plugins/qmlprojectmanager/fileformat

using namespace QmlProjectManager;
using namespace Utils;

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

const QString testDataDir = QLatin1String(SRCDIR "/data");
const FilePath testDataDirPath = FilePath::fromString(testDataDir);

static std::unique_ptr<QmlProjectItem> loadQmlProject(QString name, QString *error)
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
        auto project = loadQmlProject(QLatin1String("testFileFilter1"), &error);
        QVERIFY(project);
        QVERIFY(error.isEmpty());

        project->setSourceDirectory(testDataDirPath);

        QStringList expectedFiles(QStringList() << testDataDir + "/file1.qml"
                                                << testDataDir + "/file2.qml"
                                                << testDataDir + "/subdir/file3.qml");
        COMPARE_AS_SETS(project->files(), expectedFiles);
    }

    //
    // search for all qml files in directory
    //
    {
        auto project = loadQmlProject(QLatin1String("testFileFilter2"), &error);
        QVERIFY(project);
        QVERIFY(error.isEmpty());

        project->setSourceDirectory(testDataDirPath);

        QStringList expectedFiles(QStringList() << testDataDir + "/file1.qml"
                                                << testDataDir + "/file2.qml");
        COMPARE_AS_SETS(project->files(), expectedFiles);
    }

    //
    // search for all qml files in subdirectory
    //
    {
        auto project = loadQmlProject(QLatin1String("testFileFilter3"), &error);
        QVERIFY(project);
        QVERIFY(error.isEmpty());

        project->setSourceDirectory(testDataDirPath);

        QStringList expectedFiles(QStringList() <<  testDataDir + "/subdir/file3.qml");
        COMPARE_AS_SETS(project->files(), expectedFiles);
    }

    //
    // multiple entries
    //
    {
        auto project = loadQmlProject(QLatin1String("testFileFilter4"), &error);
        QVERIFY(project);
        QVERIFY(error.isEmpty());

        project->setSourceDirectory(testDataDirPath);

        QStringList expectedFiles(QStringList() << testDataDir + "/file1.qml"
                                                << testDataDir + "/file2.qml"
                                                << testDataDir + "/subdir/file3.qml");
        QCOMPARE(project->files().size(), 3);
        COMPARE_AS_SETS(project->files(), expectedFiles);
    }

    //
    // include specific list
    //
    {
        auto project = loadQmlProject(QLatin1String("testFileFilter5"), &error);
        QVERIFY(project);
        QVERIFY(error.isEmpty());

        project->setSourceDirectory(testDataDirPath);

        QStringList expectedFiles(QStringList() << testDataDir + "/file1.qml"
                                                << testDataDir + "/file2.qml");
        COMPARE_AS_SETS(project->files(), expectedFiles);
    }

    //
    // include specific list
    //
    {
        auto project = loadQmlProject(QLatin1String("testFileFilter6"), &error);
        QVERIFY(project);
        QVERIFY(error.isEmpty());

        project->setSourceDirectory(testDataDirPath);

        QStringList expectedFiles(QStringList() << testDataDir + "/image.gif");
        COMPARE_AS_SETS(project->files(), expectedFiles);
    }

    //
    // use wildcards
    //
    {
        auto project = loadQmlProject(QLatin1String("testFileFilter7"), &error);
        QVERIFY(project);
        QVERIFY(error.isEmpty());

        project->setSourceDirectory(testDataDirPath);

        QStringList expectedFiles(QStringList() << testDataDir + "/image.gif");
        COMPARE_AS_SETS(project->files(), expectedFiles);
    }

    //
    // use Files element (1.1)
    //
    {
        auto project = loadQmlProject(QLatin1String("testFileFilter8"), &error);
        QVERIFY(project);
        QVERIFY(error.isEmpty());

        project->setSourceDirectory(testDataDirPath);

        QStringList expectedFiles(QStringList() << testDataDir + "/image.gif");
        COMPARE_AS_SETS(project->files(), expectedFiles);
    }
}

void tst_FileFormat::testMatchesFile()
{
    QString error;
    //
    // search for qml files in local directory
    //
    auto project = loadQmlProject(QLatin1String("testMatchesFile"), &error);
    QVERIFY(project);
    QVERIFY(error.isEmpty());

    project->setSourceDirectory(testDataDirPath);

    QVERIFY(project->matchesFile(testDataDir + "/file1.qml"));
    QVERIFY(project->matchesFile(testDataDir + "/notyetexistingfile.qml"));
    QVERIFY(project->matchesFile(testDataDir + "/subdir/notyetexistingfile.qml"));
    QVERIFY(project->matchesFile(testDataDir + "/script.js"));
    QVERIFY(!project->matchesFile(testDataDir + "/script.css"));
}

void tst_FileFormat::testLibraryPaths()
{
    QString error;
    //
    // search for qml files in local directory
    //
    auto project = loadQmlProject(QLatin1String("testLibraryPaths"), &error);
    QVERIFY(project);
    QVERIFY(error.isEmpty());

    project->setSourceDirectory(testDataDirPath);

    const QDir base(testDataDir);
    const QStringList expectedPaths({base.relativeFilePath(SRCDIR "/otherLibrary"),
                                     base.relativeFilePath(SRCDIR "/data/library")});
    COMPARE_AS_SETS(project->importPaths(), expectedPaths);
}

void tst_FileFormat::testMainFile()
{
    QString error;
    //
    // search for qml files in local directory
    //
    auto project = loadQmlProject(QLatin1String("testMainFile"), &error);
    QVERIFY(project);
    QVERIFY(error.isEmpty());

    QCOMPARE(project->mainFile(), QString("file1.qml"));
}

QTEST_GUILESS_MAIN(tst_FileFormat);
#include "tst_fileformat.moc"
