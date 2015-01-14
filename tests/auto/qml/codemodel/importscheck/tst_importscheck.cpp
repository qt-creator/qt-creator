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

    void initTestCase();
private:
    QStringList m_basePaths;
};

void scanDir(const QString &dir)
{
    QFutureInterface<void> result;
    PathsAndLanguages paths;
    paths.maybeInsert(Utils::FileName::fromString(dir), Dialect::Qml);
    ModelManagerInterface::importScan(result, ModelManagerInterface::workingCopy(), paths,
                                      ModelManagerInterface::instance(), false);
    ViewerContext vCtx = ViewerContext(QStringList(), QStringList(dir));
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
    QLoggingCategory::setFilterRules(QLatin1String("qtc.*.debug=false"));
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
    QTest::newRow("base") << QStringList(QLatin1String(TESTSRCDIR "/base"))
                          << (QStringList() << QLatin1String("QML 1.0")
                              << QLatin1String("QtQml 2.2")
                              << QLatin1String("QtQml 2.1")
                              << QLatin1String("QtQuick 2.0")
                              << QLatin1String("QtQml 2.0")
                              << QLatin1String("QtQuick 2.-1")
                              << QLatin1String("QtQuick 2.1")
                              << QLatin1String("QtQuick 2.2")
                              << QLatin1String("<cpp>"))
                          << QStringList();
    QTest::newRow("001_flatQmlOnly") << QStringList(QLatin1String(TESTSRCDIR "/001_flatQmlOnly"))
                          << QStringList()
                          << QStringList();
    QTest::newRow("002_nestedQmlOnly") << QStringList(QLatin1String(TESTSRCDIR "/002_nestedQmlOnly"))
                          << QStringList()
                          << QStringList();
    /*QTest::newRow("003_packageQmlOnly") << QStringList(QLatin1String(TESTSRCDIR "/003_packageQmlOnly"))
                          << (QStringList() << QLatin1String("QtGraphicalEffects"))
                          << (QStringList()
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/ZoomBlur.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/private/FastGlow.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/private/GaussianInnerShadow.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/private/SourceProxy.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/GammaAdjust.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/HueSaturation.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/Colorize.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/RadialBlur.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/ColorOverlay.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/MaskedBlur.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/RectangularGlow.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/Displace.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/private/FastMaskedBlur.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/Desaturate.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/private/GaussianDirectionalBlur.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/GaussianBlur.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/InnerShadow.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/LinearGradient.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/Blend.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/Glow.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/RecursiveBlur.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/private/GaussianMaskedBlur.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/DropShadow.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/DirectionalBlur.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/OpacityMask.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/BrightnessContrast.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/private/GaussianGlow.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/RadialGradient.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/private/FastInnerShadow.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/ConicalGradient.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/ThresholdMask.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/LevelAdjust.qml")
                              << QLatin1String(TESTSRCDIR "/003_packageQmlOnly/QtGraphicalEffects/FastBlur.qml"));
    QTest::newRow("004_cppOnly copy") << QStringList(QLatin1String(TESTSRCDIR "004_cppOnly copy"))
                          << (QStringList() << QLatin1String("QML 1.0")
                              << QLatin1String("QtQml 2.2")
                              << QLatin1String("QtQml 2.1")
                              << QLatin1String("QtQuick 2.0")
                              << QLatin1String("QtQml 2.0")
                              << QLatin1String("QtQuick 2.-1")
                              << QLatin1String("QtQuick 2.1")
                              << QLatin1String("QtQuick 2.2")
                              << QLatin1String("<cpp>"))
                          << QStringList();*/
}

void tst_ImportCheck::test()
{
    QFETCH(QStringList, paths);
    QFETCH(QStringList, expectedLibraries);
    QFETCH(QStringList, expectedFiles);

    QFutureInterface<void> result;
    PathsAndLanguages lPaths;
    foreach (const QString &path, paths)
        lPaths.maybeInsert(Utils::FileName::fromString(path), Dialect::Qml);
    ModelManagerInterface::importScan(result, ModelManagerInterface::workingCopy(), lPaths,
                                      ModelManagerInterface::instance(), false);
    ViewerContext vCtx(QStringList(), paths);
    Snapshot snap = ModelManagerInterface::instance()->snapshot();

    ImportDependencies *iDeps = snap.importDependencies();
    QStringList detectedLibraries;
    QStringList detectedFiles;
    foreach (const ImportKey &importK, iDeps->libraryImports(vCtx))
        detectedLibraries << importK.toString();
    foreach (const QString &path, paths)
        foreach (const ImportKey &importK, iDeps->subdirImports(ImportKey(ImportType::Directory,
                                                                          path), vCtx))
        detectedFiles << QFileInfo(importK.toString()).canonicalFilePath();

    expectedLibraries.sort();
    expectedFiles.sort();
    detectedLibraries.sort();
    detectedFiles.sort();
    QCOMPARE(expectedLibraries, detectedLibraries);
    QCOMPARE(expectedFiles, detectedFiles);
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
