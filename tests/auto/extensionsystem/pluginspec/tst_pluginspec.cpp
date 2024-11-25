// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginmanager_p.h>
#include <extensionsystem/pluginspec.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QMetaObject>
#include <QtTest>
#include <QDir>

using namespace ExtensionSystem;

static const Utils::FilePath PLUGIN_DIR_PATH = Utils::FilePath::fromUserInput(PLUGIN_DIR);

static QJsonObject metaData(const QString &fileName)
{
    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open" << fileName;
        return {};
    }
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Could not parse" << fileName << ":" << error.errorString();
        return {};
    }
    return doc.object();
}

static QString libraryName(const QString &basename)
{
#if defined(Q_OS_MACOS)
    return QLatin1String("lib") + basename + QLatin1String("_debug.dylib");
#elif defined(Q_OS_UNIX)
    return QLatin1String("lib") + basename + QLatin1String(".so");
#else
    return basename + QLatin1String(DLL_INFIX ".dll");
#endif
}

class tst_PluginSpec : public QObject
{
    Q_OBJECT

private slots:
    void read();
    void readError();
    void isValidVersion();
    void versionCompare();
    void disabledByDefault();
    void provides();
    void experimental();
    void locationAndPath();
    void resolveDependencies();
    void loadLibrary();
    void initializePlugin();
    void initializeExtensions();
    void init();
    void initTestCase();
    void cleanupTestCase();

private:
    PluginManager *pm;
};

void tst_PluginSpec::init()
{
    QVERIFY(QDir::setCurrent(QLatin1String(PLUGINSPEC_DIR)));
}

void tst_PluginSpec::initTestCase()
{
    pm = new PluginManager;
    PluginManager::setPluginIID(QLatin1String("plugin"));
}

void tst_PluginSpec::cleanupTestCase()
{
    delete pm;
    pm = 0;
}

void tst_PluginSpec::read()
{
    CppPluginSpec spec;
    QCOMPARE(spec.state(), PluginSpec::Invalid);
    QVERIFY(spec.readMetaData(metaData("testspecs/spec1.json")));
    QCOMPARE(spec.errorString(), QString());
    QVERIFY(spec.errorString().isEmpty());
    QCOMPARE(spec.name(), QString("test"));
    QCOMPARE(spec.version(), QString("1.0.1"));
    QCOMPARE(spec.compatVersion(), QString("1.0.0"));
    QCOMPARE(spec.isRequired(), false);
    QCOMPARE(spec.isExperimental(), false);
    QCOMPARE(spec.isEnabledBySettings(), true);
    QCOMPARE(spec.vendor(), QString("The Qt Company Ltd"));
    QCOMPARE(spec.copyright(), QString("(C) 2015 The Qt Company Ltd"));
    QCOMPARE(spec.license(), QString("This is a default license bla\nblubbblubb\nend of terms"));
    QCOMPARE(spec.description(), QString("This plugin is just a test."));
    QCOMPARE(
        spec.longDescription(),
        QString(
            "This plugin is just a test.\n    it demonstrates the great use of the plugin spec."));
    QCOMPARE(spec.url(), QString("http://www.qt.io"));
    PluginDependency dep1;
    dep1.id = QString("SomeOtherPlugin");
    dep1.version = QString("2.3.0_2");
    PluginDependency dep2;
    dep2.id = QString("EvenOther");
    dep2.version = QString("1.0.0");
    QCOMPARE(spec.dependencies(), QVector<PluginDependency>() << dep1 << dep2);

    // test missing compatVersion behavior
    // and 'required' attribute
    QVERIFY(spec.readMetaData(metaData("testspecs/spec2.json")));
    QCOMPARE(spec.errorString(), QString());
    QCOMPARE(spec.version(), QString("3.1.4_10"));
    QCOMPARE(spec.compatVersion(), QString("3.1.4_10"));
    QCOMPARE(spec.isRequired(), true);
}

void tst_PluginSpec::readError()
{
    CppPluginSpec spec;
    QCOMPARE(spec.state(), PluginSpec::Invalid);
    QVERIFY(!spec.readMetaData(metaData("non-existing-file.json")));
    QCOMPARE(spec.state(), PluginSpec::Invalid);
    QCOMPARE(spec.errorString(), QString());
    QVERIFY(!spec.hasError());
    QVERIFY(spec.readMetaData(metaData("testspecs/spec_wrong2.json")));
    QCOMPARE(spec.state(), PluginSpec::Invalid);
    QVERIFY(spec.hasError());
    QVERIFY(!spec.errorString().isEmpty());
    QVERIFY(spec.readMetaData(metaData("testspecs/spec_wrong3.json")));
    QCOMPARE(spec.state(), PluginSpec::Invalid);
    QVERIFY(spec.hasError());
    QVERIFY(!spec.errorString().isEmpty());
    QVERIFY(spec.readMetaData(metaData("testspecs/spec_wrong4.json")));
    QCOMPARE(spec.state(), PluginSpec::Invalid);
    QVERIFY(spec.hasError());
    QVERIFY(!spec.errorString().isEmpty());
    QVERIFY(spec.readMetaData(metaData("testspecs/spec_wrong5.json")));
    QCOMPARE(spec.state(), PluginSpec::Invalid);
    QVERIFY(spec.hasError());
    QVERIFY(!spec.errorString().isEmpty());
}

void tst_PluginSpec::isValidVersion()
{
    QVERIFY(PluginSpec::isValidVersion("2"));
    QVERIFY(PluginSpec::isValidVersion("53"));
    QVERIFY(PluginSpec::isValidVersion("52_1"));
    QVERIFY(PluginSpec::isValidVersion("3.12"));
    QVERIFY(PluginSpec::isValidVersion("31.1_12"));
    QVERIFY(PluginSpec::isValidVersion("31.1.0"));
    QVERIFY(PluginSpec::isValidVersion("1.0.2_1"));

    QVERIFY(!PluginSpec::isValidVersion(""));
    QVERIFY(!PluginSpec::isValidVersion("1..0"));
    QVERIFY(!PluginSpec::isValidVersion("1.0_"));
    QVERIFY(!PluginSpec::isValidVersion("1.0.0.0"));
}

void tst_PluginSpec::versionCompare()
{
    QVERIFY(PluginSpec::versionCompare("3", "3") == 0);
    QVERIFY(PluginSpec::versionCompare("3.0.0", "3") == 0);
    QVERIFY(PluginSpec::versionCompare("3.0", "3") == 0);
    QVERIFY(PluginSpec::versionCompare("3.0.0_1", "3_1") == 0);
    QVERIFY(PluginSpec::versionCompare("3.0_21", "3_21") == 0);

    QVERIFY(PluginSpec::versionCompare("3", "1") > 0);
    QVERIFY(PluginSpec::versionCompare("3", "1.0_12") > 0);
    QVERIFY(PluginSpec::versionCompare("3_1", "3") > 0);
    QVERIFY(PluginSpec::versionCompare("3.1.0_23", "3.1") > 0);
    QVERIFY(PluginSpec::versionCompare("3.1_23", "3.1_12") > 0);

    QVERIFY(PluginSpec::versionCompare("1", "3") < 0);
    QVERIFY(PluginSpec::versionCompare("1.0_12", "3") < 0);
    QVERIFY(PluginSpec::versionCompare("3", "3_1") < 0);
    QVERIFY(PluginSpec::versionCompare("3.1", "3.1.0_23") < 0);
    QVERIFY(PluginSpec::versionCompare("3.1_12", "3.1_23") < 0);
}

void tst_PluginSpec::disabledByDefault()
{
    CppPluginSpec spec;
    QVERIFY(spec.readMetaData(metaData("testdir/disabledbydefault.json")));
    QCOMPARE(spec.errorString(), QString());

    QCOMPARE(spec.isEnabledBySettings(), false);
    QCOMPARE(spec.isEnabledByDefault(), false);
}

void tst_PluginSpec::provides()
{
    CppPluginSpec spec;
    QVERIFY(spec.readMetaData(metaData("testspecs/simplespec.json")));

    QVERIFY(!spec.provides(&spec, {"SomeOtherPlugin", "2.2.3_9"}));
    QVERIFY(!spec.provides(&spec, {"MyPlugin", "2.2.3_10"}));
    QVERIFY(!spec.provides(&spec, {"MyPlugin", "2.2.4"}));
    QVERIFY(!spec.provides(&spec, {"MyPlugin", "2.3.11_1"}));
    QVERIFY(!spec.provides(&spec, {"MyPlugin", "2.3"}));
    QVERIFY(!spec.provides(&spec, {"MyPlugin", "3.0"}));
    QVERIFY(!spec.provides(&spec, {"MyPlugin", "1.9.9_99"}));
    QVERIFY(!spec.provides(&spec, {"MyPlugin", "1.9"}));
    QVERIFY(!spec.provides(&spec, {"MyPlugin", "0.9"}));
    QVERIFY(!spec.provides(&spec, {"MyPlugin", "1"}));

    QVERIFY(spec.provides(&spec, {"myplugin", "2.2.3_9"}));
    QVERIFY(spec.provides(&spec, {"MyPlugin", "2.2.3_9"}));
    QVERIFY(spec.provides(&spec, {"MyPlugin", "2.2.3_8"}));
    QVERIFY(spec.provides(&spec, {"MyPlugin", "2.2.3"}));
    QVERIFY(spec.provides(&spec, {"MyPlugin", "2.2.2"}));
    QVERIFY(spec.provides(&spec, {"MyPlugin", "2.1.2_10"}));
    QVERIFY(spec.provides(&spec, {"MyPlugin", "2.0_10"}));
    QVERIFY(spec.provides(&spec, {"MyPlugin", "2"}));
}

void tst_PluginSpec::experimental()
{
    CppPluginSpec spec;
    QVERIFY(spec.readMetaData(metaData("testspecs/simplespec_experimental.json")));

    QCOMPARE(spec.isExperimental(), true);
    QCOMPARE(spec.isEnabledBySettings(), false);
}

void tst_PluginSpec::locationAndPath()
{
    Utils::expected_str<std::unique_ptr<PluginSpec>> ps = readCppPluginSpec(
        PLUGIN_DIR_PATH / "testplugin" / libraryName(QLatin1String("test")));
    QVERIFY_EXPECTED(ps);
    CppPluginSpec *spec = static_cast<CppPluginSpec *>(ps->get());
    QCOMPARE(spec->location(), PLUGIN_DIR_PATH / "testplugin");
    QCOMPARE(spec->filePath(), PLUGIN_DIR_PATH / "testplugin" / libraryName(QLatin1String("test")));
}

void tst_PluginSpec::resolveDependencies()
{
    PluginSpecs specs;
    PluginSpec *spec1 = new CppPluginSpec();
    specs.append(spec1);
    spec1->readMetaData(metaData("testdependencies/spec1.json"));
    spec1->setState(PluginSpec::Read); // fake read state for plugin resolving

    PluginSpec *spec2 = new CppPluginSpec();
    specs.append(spec2);
    spec2->readMetaData(metaData("testdependencies/spec2.json"));
    spec2->setState(PluginSpec::Read); // fake read state for plugin resolving

    PluginSpec *spec3 = new CppPluginSpec();
    specs.append(spec3);
    spec3->readMetaData(metaData("testdependencies/spec3.json"));
    spec3->setState(PluginSpec::Read); // fake read state for plugin resolving

    PluginSpec *spec4 = new CppPluginSpec();
    specs.append(spec4);
    spec4->readMetaData(metaData("testdependencies/spec4.json"));
    spec4->setState(PluginSpec::Read); // fake read state for plugin resolving

    PluginSpec *spec5 = new CppPluginSpec();
    specs.append(spec5);
    spec5->readMetaData(metaData("testdependencies/spec5.json"));
    spec5->setState(PluginSpec::Read); // fake read state for plugin resolving

    QCOMPARE(spec1->errorString(), QString());
    QCOMPARE(spec2->errorString(), QString());
    QCOMPARE(spec3->errorString(), QString());
    QCOMPARE(spec4->errorString(), QString());
    QCOMPARE(spec5->errorString(), QString());

    QVERIFY(spec1->resolveDependencies(specs));
    QCOMPARE(spec1->dependencySpecs().size(), 2);
    QVERIFY(!spec1->dependencySpecs().key(spec2).id.isEmpty());
    QVERIFY(!spec1->dependencySpecs().key(spec3).id.isEmpty());
    QCOMPARE(spec1->state(), PluginSpec::Resolved);
    QVERIFY(!spec4->resolveDependencies(specs));
    QVERIFY(spec4->hasError());
    QCOMPARE(spec4->state(), PluginSpec::Read);
}

void tst_PluginSpec::loadLibrary()
{
    Utils::expected_str<std::unique_ptr<PluginSpec>> ps = readCppPluginSpec(
        PLUGIN_DIR_PATH / "testplugin" / libraryName(QLatin1String("test")));

    QVERIFY_EXPECTED(ps);
    CppPluginSpec *spec = static_cast<CppPluginSpec *>(ps->get());

    QCOMPARE(spec->errorString(), QString());
    QVERIFY(spec->resolveDependencies({}));
    QVERIFY2(spec->loadLibrary(), qPrintable(spec->errorString()));
    QVERIFY(spec->plugin() != 0);
    QVERIFY(QLatin1String(spec->plugin()->metaObject()->className())
            == QLatin1String("MyPlugin::MyPluginImpl"));
    QCOMPARE(spec->state(), PluginSpec::Loaded);
    QVERIFY(!spec->hasError());
}

void tst_PluginSpec::initializePlugin()
{
    Utils::expected_str<std::unique_ptr<PluginSpec>> ps = readCppPluginSpec(
        PLUGIN_DIR_PATH / "testplugin" / libraryName(QLatin1String("test")));
    QVERIFY_EXPECTED(ps);
    CppPluginSpec *spec = static_cast<CppPluginSpec *>(ps->get());
    QVERIFY(spec->resolveDependencies({}));
    QVERIFY2(spec->loadLibrary(), qPrintable(spec->errorString()));
    bool isInitialized;
    QMetaObject::invokeMethod(spec->plugin(),
                              "isInitialized",
                              Qt::DirectConnection,
                              Q_RETURN_ARG(bool, isInitialized));
    QVERIFY(!isInitialized);
    QVERIFY(spec->initializePlugin());
    QCOMPARE(spec->state(), PluginSpec::Initialized);
    QVERIFY(!spec->hasError());
    QMetaObject::invokeMethod(spec->plugin(),
                              "isInitialized",
                              Qt::DirectConnection,
                              Q_RETURN_ARG(bool, isInitialized));
    QVERIFY(isInitialized);
}

void tst_PluginSpec::initializeExtensions()
{
    Utils::expected_str<std::unique_ptr<PluginSpec>> ps = readCppPluginSpec(
        PLUGIN_DIR_PATH / "testplugin" / libraryName(QLatin1String("test")));
    QVERIFY_EXPECTED(ps);
    CppPluginSpec *spec = static_cast<CppPluginSpec *>(ps->get());
    QVERIFY(spec->resolveDependencies({}));
    QVERIFY2(spec->loadLibrary(), qPrintable(spec->errorString()));
    bool isExtensionsInitialized;
    QVERIFY(spec->initializePlugin());
    QMetaObject::invokeMethod(spec->plugin(),
                              "isExtensionsInitialized",
                              Qt::DirectConnection,
                              Q_RETURN_ARG(bool, isExtensionsInitialized));
    QVERIFY(!isExtensionsInitialized);
    QVERIFY(spec->initializeExtensions());
    QCOMPARE(spec->state(), PluginSpec::Running);
    QVERIFY(!spec->hasError());
    QMetaObject::invokeMethod(spec->plugin(),
                              "isExtensionsInitialized",
                              Qt::DirectConnection,
                              Q_RETURN_ARG(bool, isExtensionsInitialized));
    QVERIFY(isExtensionsInitialized);
}

QTEST_GUILESS_MAIN(tst_PluginSpec)

#include "tst_pluginspec.moc"
