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

#include "qmlprojectitem.h"
#include "filefilteritems.h"
#include "qmlprojectfileformat.h"
#include <QDeclarativeComponent>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
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
    QmlProjectFileFormat::registerDeclarativeTypes();
}

QString testDataDir = QLatin1String(SRCDIR "/data");

void tst_FileFormat::testFileFilter()
{
    //
    // Search for qml files in directory + subdirectories
    //
    QString projectFile = QLatin1String(
            "import QmlProject 1.0\n"
            "Project {\n"
            "  QmlFiles {"
            "  }"
            "}\n");

    {
        QDeclarativeEngine engine;
        QDeclarativeComponent component(&engine);
        component.setData(projectFile.toUtf8(), QUrl());
        if (!component.isReady())
            qDebug() << component.errorString();
        QVERIFY(component.isReady());

        QmlProjectItem *project = qobject_cast<QmlProjectItem*>(component.create());
        QVERIFY(project);

        project->setSourceDirectory(testDataDir);

        QStringList expectedFiles(QStringList() << testDataDir + "/file1.qml"
                                                << testDataDir + "/file2.qml"
                                                << testDataDir + "/subdir/file3.qml");
        QCOMPARE(project->files().toSet(), expectedFiles.toSet());
    }

    //
    // search for all qml files in directory
    //
    projectFile = QLatin1String(
            "import QmlProject 1.0\n"
            "Project {\n"
            "  QmlFiles {\n"
            "    recursive: false\n"
            "  }\n"
            "}\n");

    {
        QDeclarativeEngine engine;
        QDeclarativeComponent component(&engine);
        component.setData(projectFile.toUtf8(), QUrl());
        if (!component.isReady())
            qDebug() << component.errorString();
        QVERIFY(component.isReady());

        QmlProjectItem *project = qobject_cast<QmlProjectItem*>(component.create());
        QVERIFY(project);

        project->setSourceDirectory(testDataDir);

        QStringList expectedFiles(QStringList() << testDataDir + "/file1.qml"
                                                << testDataDir + "/file2.qml");
        QCOMPARE(project->files().toSet(), expectedFiles.toSet());
    }

    //
    // search for all qml files in subdirectory
    //
    projectFile = QLatin1String(
            "import QmlProject 1.0\n"
            "Project {\n"
            "  QmlFiles {\n"
            "    directory: \"subdir\"\n"
            "  }\n"
            "}\n");

    {
        QDeclarativeEngine engine;
        QDeclarativeComponent component(&engine);
        component.setData(projectFile.toUtf8(), QUrl());
        QVERIFY(component.isReady());

        QmlProjectItem *project = qobject_cast<QmlProjectItem*>(component.create());
        QVERIFY(project);

        project->setSourceDirectory(testDataDir);

        QStringList expectedFiles(QStringList() <<  testDataDir + "/subdir/file3.qml");
        QCOMPARE(project->files().toSet(), expectedFiles.toSet());
    }

    //
    // multiple entries
    //
    projectFile = QLatin1String(
            "import QmlProject 1.0\n"
            "Project {\n"
            "  QmlFiles {\n"
            "    directory: \".\"\n"
            "    recursive: false\n"
            "  }"
            "  QmlFiles {\n"
            "    directory: \"subdir\"\n"
            "  }\n"
            "}\n");

    {
        QDeclarativeEngine engine;
        QDeclarativeComponent component(&engine);
        component.setData(projectFile.toUtf8(), QUrl());
        if (!component.isReady())
            qDebug() << component.errorString();
        QVERIFY(component.isReady());

        QmlProjectItem *project = qobject_cast<QmlProjectItem*>(component.create());
        QVERIFY(project);

        project->setSourceDirectory(testDataDir);

        QStringList expectedFiles(QStringList() << testDataDir + "/file1.qml"
                                                << testDataDir + "/file2.qml"
                                                << testDataDir + "/subdir/file3.qml");
        QCOMPARE(project->files().size(), 3);
        QCOMPARE(project->files().toSet(), expectedFiles.toSet());
    }

    //
    // include specific list
    //
    projectFile = QLatin1String(
            "import QmlProject 1.0\n"
            "Project {\n"
            "  QmlFiles {\n"
            "    paths: [ \"file1.qml\",\n"
            "\"file2.qml\" ]\n"
            "  }\n"
            "}\n");

    {
        QDeclarativeEngine engine;
        QDeclarativeComponent component(&engine);
        component.setData(projectFile.toUtf8(), QUrl());
        if (!component.isReady())
            qDebug() << component.errorString();
        QVERIFY(component.isReady());

        QmlProjectItem *project = qobject_cast<QmlProjectItem*>(component.create());
        QVERIFY(project);

        project->setSourceDirectory(testDataDir);

        QStringList expectedFiles(QStringList() << testDataDir + "/file1.qml"
                                                << testDataDir + "/file2.qml");
        QCOMPARE(project->files().toSet(), expectedFiles.toSet());
    }

    //
    // include specific list
    //
    projectFile = QLatin1String(
            "import QmlProject 1.0\n"
            "Project {\n"
            "  ImageFiles {\n"
            "    directory: \".\"\n"
            "  }\n"
            "}\n");

    {
        QDeclarativeEngine engine;
        QDeclarativeComponent component(&engine);
        component.setData(projectFile.toUtf8(), QUrl());
        if (!component.isReady())
            qDebug() << component.errorString();
        QVERIFY(component.isReady());

        QmlProjectItem *project = qobject_cast<QmlProjectItem*>(component.create());
        QVERIFY(project);

        project->setSourceDirectory(testDataDir);

        QStringList expectedFiles(QStringList() << testDataDir + "/image.gif");
        qDebug() << project->files().toSet() << expectedFiles.toSet();
        QCOMPARE(project->files().toSet(), expectedFiles.toSet());
    }

    //
    // use wildcards
    //
    projectFile = QLatin1String(
            "import QmlProject 1.0\n"
            "Project {\n"
            "  ImageFiles {\n"
            "    filter: \"?mage.[gf]if\"\n"
            "  }\n"
            "}\n");

    {
        QDeclarativeEngine engine;
        QDeclarativeComponent component(&engine);
        component.setData(projectFile.toUtf8(), QUrl());
        if (!component.isReady())
            qDebug() << component.errorString();
        QVERIFY(component.isReady());

        QmlProjectItem *project = qobject_cast<QmlProjectItem*>(component.create());
        QVERIFY(project);

        project->setSourceDirectory(testDataDir);

        QStringList expectedFiles(QStringList() << testDataDir + "/image.gif");
        qDebug() << project->files().toSet() << expectedFiles.toSet();
        QCOMPARE(project->files().toSet(), expectedFiles.toSet());
    }

    //
    // use Files element (1.1)
    //
    projectFile = QLatin1String(
            "import QmlProject 1.1\n"
            "Project {\n"
            "  Files {\n"
            "    filter: \"image.gif\"\n"
            "  }\n"
            "}\n");

    {
        QDeclarativeEngine engine;
        QDeclarativeComponent component(&engine);
        component.setData(projectFile.toUtf8(), QUrl());
        if (!component.isReady())
            qDebug() << component.errorString();
        QVERIFY(component.isReady());

        QmlProjectItem *project = qobject_cast<QmlProjectItem*>(component.create());
        QVERIFY(project);

        project->setSourceDirectory(testDataDir);

        QStringList expectedFiles(QStringList() << testDataDir + "/image.gif");
        qDebug() << project->files().toSet() << expectedFiles.toSet();
        QCOMPARE(project->files().toSet(), expectedFiles.toSet());
    }
}

void tst_FileFormat::testMatchesFile()
{
    //
    // search for qml files in local directory
    //
    QString projectFile = QLatin1String(
            "import QmlProject 1.0\n"
            "Project {\n"
            "  QmlFiles {"
            "    recursive: true"
            "  }"
            "  JavaScriptFiles {"
            "    paths: [\"script.js\"]"
            "  }"
            "}\n");

    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData(projectFile.toUtf8(), QUrl());
    if (!component.isReady())
        qDebug() << component.errorString();
    QVERIFY(component.isReady());

    QmlProjectItem *project = qobject_cast<QmlProjectItem*>(component.create());
    QVERIFY(project);

    project->setSourceDirectory(testDataDir);

    QVERIFY(project->matchesFile(testDataDir + "/file1.qml"));
    QVERIFY(project->matchesFile(testDataDir + "/notyetexistingfile.qml"));
    QVERIFY(project->matchesFile(testDataDir + "/subdir/notyetexistingfile.qml"));
    QVERIFY(project->matchesFile(testDataDir + "/script.js"));
    QVERIFY(!project->matchesFile(testDataDir + "/script.css"));
}

void tst_FileFormat::testLibraryPaths()
{
    //
    // search for qml files in local directory
    //
    QString projectFile = QLatin1String(
            "import QmlProject 1.0\n"
            "Project {\n"
            "  importPaths: [ \"../otherLibrary\", \"library\" ]\n"
            "}\n");

    {
        QDeclarativeEngine engine;
        QDeclarativeComponent component(&engine);
        component.setData(projectFile.toUtf8(), QUrl());
        if (!component.isReady())
            qDebug() << component.errorString();
        QVERIFY(component.isReady());

        QmlProjectItem *project = qobject_cast<QmlProjectItem*>(component.create());
        QVERIFY(project);

        project->setSourceDirectory(testDataDir);

        QStringList expectedPaths(QStringList() << SRCDIR "/otherLibrary"
                                                << SRCDIR "/data/library");
        qDebug() << expectedPaths << project->importPaths();
        QCOMPARE(project->importPaths().toSet(), expectedPaths.toSet());
    }
}

void tst_FileFormat::testMainFile()
{
    //
    // search for qml files in local directory
    //
    QString projectFile = QLatin1String(
            "import QmlProject 1.1\n"
            "Project {\n"
            "  mainFile: \"file1.qml\"\n"
            "}\n");

    {
        QDeclarativeEngine engine;
        QDeclarativeComponent component(&engine);
        component.setData(projectFile.toUtf8(), QUrl());
        if (!component.isReady())
            qDebug() << component.errorString();
        QVERIFY(component.isReady());

        QmlProjectItem *project = qobject_cast<QmlProjectItem*>(component.create());
        QVERIFY(project);

        QCOMPARE(project->mainFile(), QString("file1.qml"));
    }
}

QTEST_MAIN(tst_FileFormat);
#include "tst_fileformat.moc"
