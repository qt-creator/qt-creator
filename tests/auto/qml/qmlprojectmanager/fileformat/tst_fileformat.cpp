#include "qmlprojectitem.h"
#include "filefilteritems.h"
#include <QmlComponent>
#include <QmlContext>
#include <QmlEngine>
#include <QtTest>

using namespace QmlProjectManager;

class TestProject : public QObject
{
    Q_OBJECT
public:
    TestProject();

private slots:
    void testFileFilter();
    void testMatchesFile();
    void testLibraryPaths();
};

TestProject::TestProject()
{

}

QString testDataDir = QLatin1String(SRCDIR "/data");

void TestProject::testFileFilter()
{
    //
    // search for qml files in local directory
    //
    QString projectFile = QLatin1String(
            "import QmlProject 1.0\n"
            "Project {\n"
            "  QmlFiles {"
            "  }"
            "}\n");

    {
        QmlEngine engine;
        QmlComponent component(&engine);
        component.setData(projectFile.toUtf8(), QUrl());
        if (!component.isReady())
            qDebug() << component.errorsString();
        QVERIFY(component.isReady());

        QmlProjectItem *project = qobject_cast<QmlProjectItem*>(component.create());
        QVERIFY(project);

        project->setSourceDirectory(testDataDir);

        QStringList expectedFiles(QStringList() << testDataDir + "/file1.qml"
                                                << testDataDir + "/file2.qml");
        QCOMPARE(project->files().toSet(), expectedFiles.toSet());
    }

    //
    // search for all qml files in all subdirectories
    //
    projectFile = QLatin1String(
            "import QmlProject 1.0\n"
            "Project {\n"
            "  QmlFiles {\n"
            "    recursive: true\n"
            "  }\n"
            "}\n");

    {
        QmlEngine engine;
        QmlComponent component(&engine);
        component.setData(projectFile.toUtf8(), QUrl());
        if (!component.isReady())
            qDebug() << component.errorsString();
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
        QmlEngine engine;
        QmlComponent component(&engine);
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
            "  }"
            "  QmlFiles {\n"
            "    directory: \"subdir\"\n"
            "  }\n"
            "}\n");

    {
        QmlEngine engine;
        QmlComponent component(&engine);
        component.setData(projectFile.toUtf8(), QUrl());
        if (!component.isReady())
            qDebug() << component.errorsString();
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
        QmlEngine engine;
        QmlComponent component(&engine);
        component.setData(projectFile.toUtf8(), QUrl());
        if (!component.isReady())
            qDebug() << component.errorsString();
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
        QmlEngine engine;
        QmlComponent component(&engine);
        component.setData(projectFile.toUtf8(), QUrl());
        if (!component.isReady())
            qDebug() << component.errorsString();
        QVERIFY(component.isReady());

        QmlProjectItem *project = qobject_cast<QmlProjectItem*>(component.create());
        QVERIFY(project);

        project->setSourceDirectory(testDataDir);

        QStringList expectedFiles(QStringList() << testDataDir + "/image.gif");
        qDebug() << project->files().toSet() << expectedFiles.toSet();
        QCOMPARE(project->files().toSet(), expectedFiles.toSet());
    }
}

void TestProject::testMatchesFile()
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

    QmlEngine engine;
    QmlComponent component(&engine);
    component.setData(projectFile.toUtf8(), QUrl());
    if (!component.isReady())
        qDebug() << component.errorsString();
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

void TestProject::testLibraryPaths()
{
    //
    // search for qml files in local directory
    //
    QString projectFile = QLatin1String(
            "import QmlProject 1.0\n"
            "Project {\n"
            "  libraryPaths: [ \"../otherLibrary\", \"library\" ]\n"
            "}\n");

    {
        QmlEngine engine;
        QmlComponent component(&engine);
        component.setData(projectFile.toUtf8(), QUrl());
        if (!component.isReady())
            qDebug() << component.errorsString();
        QVERIFY(component.isReady());

        QmlProjectItem *project = qobject_cast<QmlProjectItem*>(component.create());
        QVERIFY(project);

        project->setSourceDirectory(testDataDir);

        QStringList expectedPaths(QStringList() << "../otherLibrary"
                                                << "library");
        QCOMPARE(project->libraryPaths().toSet(), expectedPaths.toSet());
    }
}


QTEST_MAIN(TestProject);
#include "tst_fileformat.moc"
