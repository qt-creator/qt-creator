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

#include <extensionsystem/pluginspec.h>
#include <extensionsystem/pluginspec_p.h>
#include <extensionsystem/pluginmanager_p.h>
#include <extensionsystem/pluginmanager.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QMetaObject>
#include <QtTest>
#include <QDir>

using namespace ExtensionSystem;

static QJsonObject metaData(const QString &fileName)
{
    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open" << fileName;
        return QJsonObject();
    }
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Could not parse" << fileName << ":" << error.errorString();
        return QJsonObject();
    }
    return doc.object();
}

static QString libraryName(const QString &basename)
{
#if defined(Q_OS_OSX)
    return QLatin1String("lib") + basename + QLatin1String("_debug.dylib");
#elif defined(Q_OS_UNIX)
    return QLatin1String("lib") + basename + QLatin1String(".so");
#else
    return basename + QLatin1String("d4.dll");
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
    QLoggingCategory::setFilterRules(QLatin1String("qtc.*.debug=false"));
    QVERIFY(QDir::setCurrent(QLatin1String(PLUGINSPEC_DIR)));
}

void tst_PluginSpec::initTestCase()
{
    pm = new PluginManager;
    pm->setPluginIID(QLatin1String("plugin"));
}

void tst_PluginSpec::cleanupTestCase()
{
    delete pm;
    pm = 0;
}

void tst_PluginSpec::read()
{
    Internal::PluginSpecPrivate spec(0);
    QCOMPARE(spec.state, PluginSpec::Invalid);
    QVERIFY(spec.readMetaData(metaData("testspecs/spec1.json")));
    QVERIFY(!spec.hasError);
    QVERIFY(spec.errorString.isEmpty());
    QCOMPARE(spec.name, QString("test"));
    QCOMPARE(spec.version, QString("1.0.1"));
    QCOMPARE(spec.compatVersion, QString("1.0.0"));
    QCOMPARE(spec.required, false);
    QCOMPARE(spec.experimental, false);
    QCOMPARE(spec.enabledBySettings, true);
    QCOMPARE(spec.vendor, QString("The Qt Company Ltd"));
    QCOMPARE(spec.copyright, QString("(C) 2015 The Qt Company Ltd"));
    QCOMPARE(spec.license, QString("This is a default license bla\nblubbblubb\nend of terms"));
    QCOMPARE(spec.description, QString("This plugin is just a test.\n    it demonstrates the great use of the plugin spec."));
    QCOMPARE(spec.url, QString("http://www.qt.io"));
    PluginDependency dep1;
    dep1.name = QString("SomeOtherPlugin");
    dep1.version = QString("2.3.0_2");
    PluginDependency dep2;
    dep2.name = QString("EvenOther");
    dep2.version = QString("1.0.0");
    QCOMPARE(spec.dependencies, QVector<PluginDependency>() << dep1 << dep2);

    // test missing compatVersion behavior
    // and 'required' attribute
    QVERIFY(spec.readMetaData(metaData("testspecs/spec2.json")));
    QCOMPARE(spec.version, QString("3.1.4_10"));
    QCOMPARE(spec.compatVersion, QString("3.1.4_10"));
    QCOMPARE(spec.required, true);
}

void tst_PluginSpec::readError()
{
    Internal::PluginSpecPrivate spec(0);
    QCOMPARE(spec.state, PluginSpec::Invalid);
    QVERIFY(!spec.readMetaData(metaData("non-existing-file.json")));
    QCOMPARE(spec.state, PluginSpec::Invalid);
    QVERIFY(!spec.hasError);
    QVERIFY(spec.errorString.isEmpty());
    QVERIFY(spec.readMetaData(metaData("testspecs/spec_wrong2.json")));
    QCOMPARE(spec.state, PluginSpec::Invalid);
    QVERIFY(spec.hasError);
    QVERIFY(!spec.errorString.isEmpty());
    QVERIFY(spec.readMetaData(metaData("testspecs/spec_wrong3.json")));
    QCOMPARE(spec.state, PluginSpec::Invalid);
    QVERIFY(spec.hasError);
    QVERIFY(!spec.errorString.isEmpty());
    QVERIFY(spec.readMetaData(metaData("testspecs/spec_wrong4.json")));
    QCOMPARE(spec.state, PluginSpec::Invalid);
    QVERIFY(spec.hasError);
    QVERIFY(!spec.errorString.isEmpty());
    QVERIFY(spec.readMetaData(metaData("testspecs/spec_wrong5.json")));
    QCOMPARE(spec.state, PluginSpec::Invalid);
    QVERIFY(spec.hasError);
    QVERIFY(!spec.errorString.isEmpty());
}

void tst_PluginSpec::isValidVersion()
{
    QVERIFY(Internal::PluginSpecPrivate::isValidVersion("2"));
    QVERIFY(Internal::PluginSpecPrivate::isValidVersion("53"));
    QVERIFY(Internal::PluginSpecPrivate::isValidVersion("52_1"));
    QVERIFY(Internal::PluginSpecPrivate::isValidVersion("3.12"));
    QVERIFY(Internal::PluginSpecPrivate::isValidVersion("31.1_12"));
    QVERIFY(Internal::PluginSpecPrivate::isValidVersion("31.1.0"));
    QVERIFY(Internal::PluginSpecPrivate::isValidVersion("1.0.2_1"));

    QVERIFY(!Internal::PluginSpecPrivate::isValidVersion(""));
    QVERIFY(!Internal::PluginSpecPrivate::isValidVersion("1..0"));
    QVERIFY(!Internal::PluginSpecPrivate::isValidVersion("1.0_"));
    QVERIFY(!Internal::PluginSpecPrivate::isValidVersion("1.0.0.0"));
}

void tst_PluginSpec::versionCompare()
{
    QVERIFY(Internal::PluginSpecPrivate::versionCompare("3", "3") == 0);
    QVERIFY(Internal::PluginSpecPrivate::versionCompare("3.0.0", "3") == 0);
    QVERIFY(Internal::PluginSpecPrivate::versionCompare("3.0", "3") == 0);
    QVERIFY(Internal::PluginSpecPrivate::versionCompare("3.0.0_1", "3_1") == 0);
    QVERIFY(Internal::PluginSpecPrivate::versionCompare("3.0_21", "3_21") == 0);

    QVERIFY(Internal::PluginSpecPrivate::versionCompare("3", "1") > 0);
    QVERIFY(Internal::PluginSpecPrivate::versionCompare("3", "1.0_12") > 0);
    QVERIFY(Internal::PluginSpecPrivate::versionCompare("3_1", "3") > 0);
    QVERIFY(Internal::PluginSpecPrivate::versionCompare("3.1.0_23", "3.1") > 0);
    QVERIFY(Internal::PluginSpecPrivate::versionCompare("3.1_23", "3.1_12") > 0);

    QVERIFY(Internal::PluginSpecPrivate::versionCompare("1", "3") < 0);
    QVERIFY(Internal::PluginSpecPrivate::versionCompare("1.0_12", "3") < 0);
    QVERIFY(Internal::PluginSpecPrivate::versionCompare("3", "3_1") < 0);
    QVERIFY(Internal::PluginSpecPrivate::versionCompare("3.1", "3.1.0_23") < 0);
    QVERIFY(Internal::PluginSpecPrivate::versionCompare("3.1_12", "3.1_23") < 0);
}

void tst_PluginSpec::provides()
{
    Internal::PluginSpecPrivate spec(0);
    QVERIFY(spec.readMetaData(metaData("testspecs/simplespec.json")));
    QVERIFY(!spec.provides("SomeOtherPlugin", "2.2.3_9"));
    QVERIFY(!spec.provides("MyPlugin", "2.2.3_10"));
    QVERIFY(!spec.provides("MyPlugin", "2.2.4"));
    QVERIFY(!spec.provides("MyPlugin", "2.3.11_1"));
    QVERIFY(!spec.provides("MyPlugin", "2.3"));
    QVERIFY(!spec.provides("MyPlugin", "3.0"));
    QVERIFY(!spec.provides("MyPlugin", "1.9.9_99"));
    QVERIFY(!spec.provides("MyPlugin", "1.9"));
    QVERIFY(!spec.provides("MyPlugin", "0.9"));
    QVERIFY(!spec.provides("MyPlugin", "1"));

    QVERIFY(spec.provides("myplugin", "2.2.3_9"));
    QVERIFY(spec.provides("MyPlugin", "2.2.3_9"));
    QVERIFY(spec.provides("MyPlugin", "2.2.3_8"));
    QVERIFY(spec.provides("MyPlugin", "2.2.3"));
    QVERIFY(spec.provides("MyPlugin", "2.2.2"));
    QVERIFY(spec.provides("MyPlugin", "2.1.2_10"));
    QVERIFY(spec.provides("MyPlugin", "2.0_10"));
    QVERIFY(spec.provides("MyPlugin", "2"));
}

void tst_PluginSpec::experimental()
{
    Internal::PluginSpecPrivate spec(0);
    QVERIFY(spec.readMetaData(metaData("testspecs/simplespec_experimental.json")));
    QCOMPARE(spec.experimental, true);
    QCOMPARE(spec.enabledBySettings, false);
}

void tst_PluginSpec::locationAndPath()
{
    Internal::PluginSpecPrivate spec(0);
    QVERIFY(spec.read(QLatin1String(PLUGIN_DIR) + QLatin1String("/testplugin/") + libraryName(QLatin1String("test"))));
    QCOMPARE(spec.location, QString(QLatin1String(PLUGIN_DIR) + QLatin1String("/testplugin")));
    QCOMPARE(spec.filePath, QString(QLatin1String(PLUGIN_DIR) + QLatin1String("/testplugin/") + libraryName(QLatin1String("test"))));
}

void tst_PluginSpec::resolveDependencies()
{
    QList<PluginSpec *> specs;
    PluginSpec *spec1 = Internal::PluginManagerPrivate::createSpec();
    specs.append(spec1);
    Internal::PluginSpecPrivate *spec1Priv = Internal::PluginManagerPrivate::privateSpec(spec1);
    spec1Priv->readMetaData(metaData("testdependencies/spec1.json"));
    spec1Priv->state = PluginSpec::Read; // fake read state for plugin resolving

    PluginSpec *spec2 = Internal::PluginManagerPrivate::createSpec();
    specs.append(spec2);
    Internal::PluginSpecPrivate *spec2Priv = Internal::PluginManagerPrivate::privateSpec(spec2);
    spec2Priv->readMetaData(metaData("testdependencies/spec2.json"));
    spec2Priv->state = PluginSpec::Read; // fake read state for plugin resolving

    PluginSpec *spec3 = Internal::PluginManagerPrivate::createSpec();
    specs.append(spec3);
    Internal::PluginSpecPrivate *spec3Priv = Internal::PluginManagerPrivate::privateSpec(spec3);
    spec3Priv->readMetaData(metaData("testdependencies/spec3.json"));
    spec3Priv->state = PluginSpec::Read; // fake read state for plugin resolving

    PluginSpec *spec4 = Internal::PluginManagerPrivate::createSpec();
    specs.append(spec4);
    Internal::PluginSpecPrivate *spec4Priv = Internal::PluginManagerPrivate::privateSpec(spec4);
    spec4Priv->readMetaData(metaData("testdependencies/spec4.json"));
    spec4Priv->state = PluginSpec::Read; // fake read state for plugin resolving

    PluginSpec *spec5 = Internal::PluginManagerPrivate::createSpec();
    specs.append(spec5);
    Internal::PluginSpecPrivate *spec5Priv = Internal::PluginManagerPrivate::privateSpec(spec5);
    spec5Priv->readMetaData(metaData("testdependencies/spec5.json"));
    spec5Priv->state = PluginSpec::Read; // fake read state for plugin resolving

    QVERIFY(spec1Priv->resolveDependencies(specs));
    QCOMPARE(spec1Priv->dependencySpecs.size(), 2);
    QVERIFY(!spec1Priv->dependencySpecs.key(spec2).name.isEmpty());
    QVERIFY(!spec1Priv->dependencySpecs.key(spec3).name.isEmpty());
    QCOMPARE(spec1Priv->state, PluginSpec::Resolved);
    QVERIFY(!spec4Priv->resolveDependencies(specs));
    QVERIFY(spec4Priv->hasError);
    QCOMPARE(spec4Priv->state, PluginSpec::Read);
}

void tst_PluginSpec::loadLibrary()
{
    PluginSpec *ps = Internal::PluginManagerPrivate::createSpec();
    Internal::PluginSpecPrivate *spec = Internal::PluginManagerPrivate::privateSpec(ps);
    QVERIFY(spec->read(QLatin1String(PLUGIN_DIR) + QLatin1String("/testplugin/") + libraryName(QLatin1String("test"))));
    QVERIFY(spec->resolveDependencies(QList<PluginSpec *>()));
    QVERIFY2(spec->loadLibrary(), qPrintable(spec->errorString));
    QVERIFY(spec->plugin != 0);
    QVERIFY(QLatin1String(spec->plugin->metaObject()->className()) == QLatin1String("MyPlugin::MyPluginImpl"));
    QCOMPARE(spec->state, PluginSpec::Loaded);
    QVERIFY(!spec->hasError);
    QCOMPARE(spec->plugin->pluginSpec(), ps);
    delete ps;
}

void tst_PluginSpec::initializePlugin()
{
    Internal::PluginSpecPrivate spec(0);
    QVERIFY(spec.read(QLatin1String(PLUGIN_DIR) + QLatin1String("/testplugin/") + libraryName(QLatin1String("test"))));
    QVERIFY(spec.resolveDependencies(QList<PluginSpec *>()));
    QVERIFY2(spec.loadLibrary(), qPrintable(spec.errorString));
    bool isInitialized;
    QMetaObject::invokeMethod(spec.plugin, "isInitialized",
                              Qt::DirectConnection, Q_RETURN_ARG(bool, isInitialized));
    QVERIFY(!isInitialized);
    QVERIFY(spec.initializePlugin());
    QCOMPARE(spec.state, PluginSpec::Initialized);
    QVERIFY(!spec.hasError);
    QMetaObject::invokeMethod(spec.plugin, "isInitialized",
                              Qt::DirectConnection, Q_RETURN_ARG(bool, isInitialized));
    QVERIFY(isInitialized);
}

void tst_PluginSpec::initializeExtensions()
{
    Internal::PluginSpecPrivate spec(0);
    QVERIFY(spec.read(QLatin1String(PLUGIN_DIR) + QLatin1String("/testplugin/") + libraryName(QLatin1String("test"))));
    QVERIFY(spec.resolveDependencies(QList<PluginSpec *>()));
    QVERIFY2(spec.loadLibrary(), qPrintable(spec.errorString));
    bool isExtensionsInitialized;
    QVERIFY(spec.initializePlugin());
    QMetaObject::invokeMethod(spec.plugin, "isExtensionsInitialized",
                              Qt::DirectConnection, Q_RETURN_ARG(bool, isExtensionsInitialized));
    QVERIFY(!isExtensionsInitialized);
    QVERIFY(spec.initializeExtensions());
    QCOMPARE(spec.state, PluginSpec::Running);
    QVERIFY(!spec.hasError);
    QMetaObject::invokeMethod(spec.plugin, "isExtensionsInitialized",
                              Qt::DirectConnection, Q_RETURN_ARG(bool, isExtensionsInitialized));
    QVERIFY(isExtensionsInitialized);
}

QTEST_MAIN(tst_PluginSpec)

#include "tst_pluginspec.moc"
