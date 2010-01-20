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
    void testQmlFileFilter();
};

TestProject::TestProject()
{

}

QString testDataDir = QLatin1String(SRCDIR "/data");

void TestProject::testQmlFileFilter()
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
        QCOMPARE(project->qmlFiles().toSet(), expectedFiles.toSet());
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
        QCOMPARE(project->qmlFiles().toSet(), expectedFiles.toSet());
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
        QCOMPARE(project->qmlFiles().toSet(), expectedFiles.toSet());
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
        QCOMPARE(project->qmlFiles().size(), 3);
        QCOMPARE(project->qmlFiles().toSet(), expectedFiles.toSet());
    }
}


QTEST_MAIN(TestProject);
#include "tst_fileformat.moc"
