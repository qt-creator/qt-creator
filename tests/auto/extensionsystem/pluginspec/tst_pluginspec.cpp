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

#include <extensionsystem/pluginspec.h>
#include <extensionsystem/pluginspec_p.h>
#include <extensionsystem/pluginmanager_p.h>
#include <extensionsystem/pluginmanager.h>

#include <QObject>
#include <QMetaObject>
#include <QtTest>

using namespace ExtensionSystem;

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
};

void tst_PluginSpec::read()
{
    Internal::PluginSpecPrivate spec(0);
    QCOMPARE(spec.state, PluginSpec::Invalid);
    QVERIFY(spec.read("testspecs/spec1.xml"));
    QCOMPARE(spec.state, PluginSpec::Read);
    QVERIFY(!spec.hasError);
    QVERIFY(spec.errorString.isEmpty());
    QCOMPARE(spec.name, QString("test"));
    QCOMPARE(spec.version, QString("1.0.1"));
    QCOMPARE(spec.compatVersion, QString("1.0.0"));
    QCOMPARE(spec.experimental, false);
    QCOMPARE(spec.enabled, true);
    QCOMPARE(spec.vendor, QString("Digia Plc"));
    QCOMPARE(spec.copyright, QString("(C) 2013 Digia Plc"));
    QCOMPARE(spec.license, QString("This is a default license bla\nblubbblubb\nend of terms"));
    QCOMPARE(spec.description, QString("This plugin is just a test.\n    it demonstrates the great use of the plugin spec."));
    QCOMPARE(spec.url, QString("http://qt.digi.com"));
    PluginDependency dep1;
    dep1.name = QString("SomeOtherPlugin");
    dep1.version = QString("2.3.0_2");
    PluginDependency dep2;
    dep2.name = QString("EvenOther");
    dep2.version = QString("1.0.0");
    QCOMPARE(spec.dependencies, QList<PluginDependency>() << dep1 << dep2);

    // test missing compatVersion behavior
    QVERIFY(spec.read("testspecs/spec2.xml"));
    QCOMPARE(spec.version, QString("3.1.4_10"));
    QCOMPARE(spec.compatVersion, QString("3.1.4_10"));
}

void tst_PluginSpec::readError()
{
    Internal::PluginSpecPrivate spec(0);
    QCOMPARE(spec.state, PluginSpec::Invalid);
    QVERIFY(!spec.read("non-existing-file.xml"));
    QCOMPARE(spec.state, PluginSpec::Invalid);
    QVERIFY(spec.hasError);
    QVERIFY(!spec.errorString.isEmpty());
    QVERIFY(!spec.read("testspecs/spec_wrong1.xml"));
    QCOMPARE(spec.state, PluginSpec::Invalid);
    QVERIFY(spec.hasError);
    QVERIFY(!spec.errorString.isEmpty());
    QVERIFY(!spec.read("testspecs/spec_wrong2.xml"));
    QCOMPARE(spec.state, PluginSpec::Invalid);
    QVERIFY(spec.hasError);
    QVERIFY(!spec.errorString.isEmpty());
    QVERIFY(!spec.read("testspecs/spec_wrong3.xml"));
    QCOMPARE(spec.state, PluginSpec::Invalid);
    QVERIFY(spec.hasError);
    QVERIFY(!spec.errorString.isEmpty());
    QVERIFY(!spec.read("testspecs/spec_wrong4.xml"));
    QCOMPARE(spec.state, PluginSpec::Invalid);
    QVERIFY(spec.hasError);
    QVERIFY(!spec.errorString.isEmpty());
    QVERIFY(!spec.read("testspecs/spec_wrong5.xml"));
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
    QVERIFY(spec.read("testspecs/simplespec.xml"));
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
    QVERIFY(spec.read("testspecs/simplespec_experimental.xml"));
    QCOMPARE(spec.experimental, true);
    QCOMPARE(spec.enabled, false);
}

void tst_PluginSpec::locationAndPath()
{
    Internal::PluginSpecPrivate spec(0);
    QVERIFY(spec.read("testspecs/simplespec.xml"));
    QCOMPARE(spec.location, QString(QDir::currentPath()+"/testspecs"));
    QCOMPARE(spec.filePath, QString(QDir::currentPath()+"/testspecs/simplespec.xml"));
    QVERIFY(spec.read("testdir/../testspecs/simplespec.xml"));
    QCOMPARE(spec.location, QString(QDir::currentPath()+"/testspecs"));
    QCOMPARE(spec.filePath, QString(QDir::currentPath()+"/testspecs/simplespec.xml"));
    QVERIFY(spec.read("testdir/spec.xml"));
    QCOMPARE(spec.location, QString(QDir::currentPath()+"/testdir"));
    QCOMPARE(spec.filePath, QString(QDir::currentPath()+"/testdir/spec.xml"));
}

void tst_PluginSpec::resolveDependencies()
{
    QList<PluginSpec *> specs;
    PluginSpec *spec1 = Internal::PluginManagerPrivate::createSpec();
    specs.append(spec1);
    Internal::PluginSpecPrivate *spec1Priv = Internal::PluginManagerPrivate::privateSpec(spec1);
    spec1Priv->read("testdependencies/spec1.xml");
    PluginSpec *spec2 = Internal::PluginManagerPrivate::createSpec();
    specs.append(spec2);
    Internal::PluginManagerPrivate::privateSpec(spec2)->read("testdependencies/spec2.xml");
    PluginSpec *spec3 = Internal::PluginManagerPrivate::createSpec();
    specs.append(spec3);
    Internal::PluginManagerPrivate::privateSpec(spec3)->read("testdependencies/spec3.xml");
    PluginSpec *spec4 = Internal::PluginManagerPrivate::createSpec();
    specs.append(spec4);
    Internal::PluginSpecPrivate *spec4Priv = Internal::PluginManagerPrivate::privateSpec(spec4);
    spec4Priv->read("testdependencies/spec4.xml");
    PluginSpec *spec5 = Internal::PluginManagerPrivate::createSpec();
    specs.append(spec5);
    Internal::PluginManagerPrivate::privateSpec(spec5)->read("testdependencies/spec5.xml");
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
    PluginManager *manager = new PluginManager();
    QVERIFY(spec->read("testplugin/testplugin.xml"));
    QVERIFY(spec->resolveDependencies(QList<PluginSpec *>()));
    QVERIFY(spec->loadLibrary());
    QVERIFY(spec->plugin != 0);
    QVERIFY(QString::fromLocal8Bit(spec->plugin->metaObject()->className()) == QString::fromLocal8Bit("MyPlugin::MyPluginImpl"));
    QCOMPARE(spec->state, PluginSpec::Loaded);
    QVERIFY(!spec->hasError);
    QCOMPARE(spec->plugin->pluginSpec(), ps);
    delete manager;
    delete ps;
}

void tst_PluginSpec::initializePlugin()
{
    Internal::PluginSpecPrivate spec(0);
    QVERIFY(spec.read("testplugin/testplugin.xml"));
    QVERIFY(spec.resolveDependencies(QList<PluginSpec *>()));
    QVERIFY(spec.loadLibrary());
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
    QVERIFY(spec.read("testplugin/testplugin.xml"));
    QVERIFY(spec.resolveDependencies(QList<PluginSpec *>()));
    QVERIFY(spec.loadLibrary());
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
