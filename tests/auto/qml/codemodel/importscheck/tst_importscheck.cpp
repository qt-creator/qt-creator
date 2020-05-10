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

#include <QFileInfo>
#include <QGraphicsObject>
#include <QGuiApplication>
#include <QLatin1String>
#include <QLoggingCategory>
#include <QScopedPointer>
#include <QSettings>
#include <QStringList>
#include <QtTest>

#include <qmljs/qmljsbind.h>
#include <qmljs/qmljscheck.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsimportdependencies.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljslink.h>
#include <qmljs/qmljsmodelmanagerinterface.h>

#include <algorithm>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJS::StaticAnalysis;

class tst_ImportCheck : public QObject
{
    Q_OBJECT
public:
    tst_ImportCheck();

private slots:
    void test();
    void test_data();

    void importTypes_data();
    void importTypes();

    void initTestCase();
private:
    QStringList m_basePaths;
};

void scanDir(const QString &dir)
{
    QFutureInterface<void> result;
    PathsAndLanguages paths;
    paths.maybeInsert(Utils::FilePath::fromString(dir), Dialect::Qml);
    ModelManagerInterface::importScan(result, ModelManagerInterface::workingCopy(), paths,
                                      ModelManagerInterface::instance(), false);
    ViewerContext vCtx;
    vCtx.paths.append(dir);
    Snapshot snap = ModelManagerInterface::instance()->snapshot();

    ImportDependencies *iDeps = snap.importDependencies();
    qDebug() << "libs:";
    foreach (const ImportKey &importK, iDeps->libraryImports(vCtx))
        qDebug() << "libImport: " << importK.toString();
    qDebug() << "qml files:";
    foreach (const ImportKey &importK, iDeps->subdirImports(ImportKey(ImportType::Directory, dir),
                                                           vCtx))
        qDebug() << importK.toString();
}

tst_ImportCheck::tst_ImportCheck()
{
}

#ifdef Q_OS_MAC
#  define SHARE_PATH "/Resources"
#else
#  define SHARE_PATH "/share/qtcreator"
#endif

QString resourcePath()
{
    return QDir::cleanPath(QTCREATORDIR + QLatin1String(SHARE_PATH));
}

void tst_ImportCheck::initTestCase()
{
    if (!ModelManagerInterface::instance())
        new ModelManagerInterface;

    // the resource path is wrong, have to load things manually
    QFileInfo builtins(QString::fromLocal8Bit(TESTSRCDIR) + QLatin1String("/base"));
    if (builtins.exists())
        m_basePaths.append(builtins.filePath());
}

#define QCOMPARE_NOEXIT(actual, expected) \
    QTest::qCompare(actual, expected, #actual, #expected, __FILE__, __LINE__)

void tst_ImportCheck::test_data()
{
    QTest::addColumn<QStringList>("paths");
    QTest::addColumn<QStringList>("expectedLibraries");
    QTest::addColumn<QStringList>("expectedFiles");
    QTest::newRow("base") << QStringList(QString(TESTSRCDIR "/base"))
                          << QStringList({"QML 1.0", "QtQml 2.2", "QtQml 2.1", "QtQuick 2.0",
                                          "QtQml 2.0", "QtQuick 2.1", "QtQuick 2.2", "<cpp>"})
                          << QStringList();
    QTest::newRow("001_flatQmlOnly") << QStringList(QString(TESTSRCDIR "/001_flatQmlOnly"))
                          << QStringList()
                          << QStringList();
    QTest::newRow("002_nestedQmlOnly") << QStringList(QString(TESTSRCDIR "/002_nestedQmlOnly"))
                          << QStringList()
                          << QStringList();
    QTest::newRow("003_packageQmlOnly") << QStringList(QString(TESTSRCDIR "/003_packageQmlOnly"))
                          << QStringList("QtGraphicalEffects 1.0")
                          << (QStringList()
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/ZoomBlur.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/private/FastGlow.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/private/GaussianInnerShadow.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/private/SourceProxy.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/GammaAdjust.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/HueSaturation.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/Colorize.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/RadialBlur.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/ColorOverlay.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/MaskedBlur.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/RectangularGlow.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/Displace.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/private/FastMaskedBlur.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/Desaturate.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/private/GaussianDirectionalBlur.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/GaussianBlur.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/InnerShadow.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/LinearGradient.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/Blend.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/Glow.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/RecursiveBlur.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/private/GaussianMaskedBlur.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/DropShadow.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/DirectionalBlur.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/OpacityMask.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/BrightnessContrast.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/private/GaussianGlow.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/RadialGradient.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/private/FastInnerShadow.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/ConicalGradient.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/ThresholdMask.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/LevelAdjust.qml")
                              << QString(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/FastBlur.qml"));
    QTest::newRow("004_cppOnly copy") << QStringList(QString(TESTSRCDIR "/004_cppOnly copy"))
                          << QStringList({ "QML 1.0", "QtQml 2.2", "QtQml 2.1", "QtQuick 2.0",
                                           "QtQml 2.0", "QtQuick 2.1", "QtQuick 2.2", "<cpp>" })
                          << QStringList();
}

void tst_ImportCheck::test()
{
    QFETCH(QStringList, paths);
    QFETCH(QStringList, expectedLibraries);
    QFETCH(QStringList, expectedFiles);

    QFutureInterface<void> result;
    PathsAndLanguages lPaths;
    foreach (const QString &path, paths)
        lPaths.maybeInsert(Utils::FilePath::fromString(path), Dialect::Qml);
    ModelManagerInterface::importScan(result, ModelManagerInterface::workingCopy(), lPaths,
                                      ModelManagerInterface::instance(), false);
    ViewerContext vCtx;
    vCtx.paths.append(paths);
    Snapshot snap = ModelManagerInterface::instance()->snapshot();

    ImportDependencies *iDeps = snap.importDependencies();
    QStringList detectedLibraries;
    QStringList detectedFiles;
    foreach (const ImportKey &importK, iDeps->libraryImports(vCtx))
        detectedLibraries << importK.toString();
    foreach (const QString &path, paths) {
        foreach (const ImportKey &importK, iDeps->subdirImports(ImportKey(ImportType::Directory,
                                                                          path), vCtx)) {
            detectedFiles << QFileInfo(importK.toString()).canonicalFilePath();
        }
    }

    expectedLibraries.sort();
    expectedFiles.sort();
    detectedLibraries.sort();
    detectedFiles.sort();
    QCOMPARE(expectedLibraries, detectedLibraries);
    QCOMPARE(expectedFiles, detectedFiles);
}

class MyModelManager : public ModelManagerInterface
{
public:
    using ModelManagerInterface::setDefaultProject;
    using ModelManagerInterface::updateImportPaths;
};

void tst_ImportCheck::importTypes_data()
{
    QTest::addColumn<QString>("qmlFile");
    QTest::addColumn<QString>("importPath");
    QTest::addColumn<QStringList>("expectedTypes");

    QTest::newRow("base")
            << QString(TESTSRCDIR "/importTypes/importQtQuick.qml")
            << QString(TESTSRCDIR "/base")
            << QStringList({ "Item", "Rectangle", "QtObject" });

    // simple case, with everything in QtQuick/plugins.qmltypes
    QTest::newRow("QtQuick-simple")
            << QString(TESTSRCDIR "/importTypes/importQtQuick.qml")
            << QString(TESTSRCDIR "/importTypes/imports-QtQuick-simple")
            << QStringList({ "Item", "QtObject", "IsQtQuickSimple" });

    // QtQuick/ and QtQml/ with an implicit dependency
    // Seen in Qt 5.15.0.
    QTest::newRow("QtQuick-workaround-QtQml")
            << QString(TESTSRCDIR "/importTypes/importQtQuick.qml")
            << QString(TESTSRCDIR "/importTypes/imports-QtQuick-workaround-QtQml")
            << QStringList({ "Item", "QtObject", "IsQtQuickWorkaround" });

    // QtQuick/ and QtQml/ with an "import" in the qmldir file
    // Seen in Qt 6.
    QTest::newRow("QtQuick-qmldir-import")
            << QString(TESTSRCDIR "/importTypes/importQtQuick.qml")
            << QString(TESTSRCDIR "/importTypes/imports-QtQuick-qmldir-import")
            << QStringList({ "Item", "QtObject", "IsQtQuickQmldirImport" });
}

void tst_ImportCheck::importTypes()
{
    QFETCH(QString, qmlFile);
    QFETCH(QString, importPath);
    QFETCH(QStringList, expectedTypes);

    // full reset
    delete ModelManagerInterface::instance();
    MyModelManager *modelManager = new MyModelManager;

    // the default qtQmlPath is based on the Qt version in use otherwise
    ModelManagerInterface::ProjectInfo defaultProject;
    defaultProject.qtQmlPath = importPath;
    modelManager->setDefaultProject(defaultProject, nullptr);
    modelManager->activateScan();

    modelManager->updateSourceFiles(QStringList(qmlFile), false);
    modelManager->test_joinAllThreads();

    Snapshot snapshot = modelManager->newestSnapshot();
    Document::Ptr doc = snapshot.document(qmlFile);

    // It's unfortunate, but nowadays linking can trigger async module loads,
    // so do it once to start the process, then do it again for real once the
    // dependencies are available.
    const auto getContext = [&]() {
        Link link(snapshot, modelManager->defaultVContext(doc->language(), doc),
                  modelManager->builtins(doc));
        return link();
    };
    getContext();
    modelManager->test_joinAllThreads();
    snapshot = modelManager->newestSnapshot();
    doc = snapshot.document(qmlFile);

    ContextPtr context = getContext();

    bool allFound = true;
    for (const auto &expected : expectedTypes) {
        if (!context->lookupType(doc.data(), QStringList(expected))) {
            allFound = false;
            qWarning() << "Type '" << expected << "' not found";
        }
    }
    QVERIFY(allFound);
}

#ifdef MANUAL_IMPORT_SCANNER

int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);
    new ModelManagerInterface;
    if (argc != 2 || !QFileInfo(QString::fromLocal8Bit(argv[1])).isDir()) {
        printf("usage: %s dir/to/scan\n", ((argc > 0) ? argv[0] : "importScan"));
        exit(1);
    }
    tst_ImportCheck checker;
    QTimer::singleShot(1, &checker, SLOT(doScan()));
    a.exec();
    return 0;
}

#else

QTEST_MAIN(tst_ImportCheck)

#endif // MANUAL_IMPORT_SCANNER

#include "tst_importscheck.moc"
