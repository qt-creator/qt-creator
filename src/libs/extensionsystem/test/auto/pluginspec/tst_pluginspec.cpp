/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "testplugin/testplugin.h"

#include <extensionsystem/pluginspec.h>
#include <extensionsystem/pluginspec_p.h>
#include <extensionsystem/pluginmanager_p.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QObject>
#include <QtTest/QtTest>

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
    QCOMPARE(spec.vendor, QString("Trolltech"));
    QCOMPARE(spec.copyright, QString("(C) 2007 Trolltech ASA"));
    QCOMPARE(spec.license, QString("This is a default license bla\nblubbblubb\nend of terms"));
    QCOMPARE(spec.description, QString("This plugin is just a test.\n    it demonstrates the great use of the plugin spec."));
    QCOMPARE(spec.url, QString("http://www.trolltech.com"));
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

void tst_PluginSpec::locationAndPath()
{
    Internal::PluginSpecPrivate spec(0);
    QVERIFY(spec.read("testspecs/simplespec.xml"));
    QCOMPARE(spec.location, QDir::currentPath()+"/testspecs");
    QCOMPARE(spec.filePath, QDir::currentPath()+"/testspecs/simplespec.xml");
    QVERIFY(spec.read("testdir/../testspecs/simplespec.xml"));
    QCOMPARE(spec.location, QDir::currentPath()+"/testspecs");
    QCOMPARE(spec.filePath, QDir::currentPath()+"/testspecs/simplespec.xml");
    QVERIFY(spec.read("testdir/spec.xml"));
    QCOMPARE(spec.location, QDir::currentPath()+"/testdir");
    QCOMPARE(spec.filePath, QDir::currentPath()+"/testdir/spec.xml");
}

void tst_PluginSpec::resolveDependencies()
{
    QSet<PluginSpec *> specs;
    PluginSpec *spec1 = Internal::PluginManagerPrivate::createSpec();
    specs.insert(spec1);
    Internal::PluginSpecPrivate *spec1Priv = Internal::PluginManagerPrivate::privateSpec(spec1);
    spec1Priv->read("testdependencies/spec1.xml");
    PluginSpec *spec2 = Internal::PluginManagerPrivate::createSpec();
    specs.insert(spec2);
    Internal::PluginManagerPrivate::privateSpec(spec2)->read("testdependencies/spec2.xml");
    PluginSpec *spec3 = Internal::PluginManagerPrivate::createSpec();
    specs.insert(spec3);
    Internal::PluginManagerPrivate::privateSpec(spec3)->read("testdependencies/spec3.xml");
    PluginSpec *spec4 = Internal::PluginManagerPrivate::createSpec();
    specs.insert(spec4);
    Internal::PluginSpecPrivate *spec4Priv = Internal::PluginManagerPrivate::privateSpec(spec4);
    spec4Priv->read("testdependencies/spec4.xml");
    PluginSpec *spec5 = Internal::PluginManagerPrivate::createSpec();
    specs.insert(spec5);
    Internal::PluginManagerPrivate::privateSpec(spec5)->read("testdependencies/spec5.xml");
    QVERIFY(spec1Priv->resolveDependencies(specs));
    QCOMPARE(spec1Priv->dependencySpecs.size(), 2);
    QVERIFY(spec1Priv->dependencySpecs.contains(spec2));
    QVERIFY(spec1Priv->dependencySpecs.contains(spec3));
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
    QVERIFY(spec->resolveDependencies(QSet<PluginSpec *>()));
    QVERIFY(spec->loadLibrary());
    QVERIFY(qobject_cast<MyPlugin::MyPluginImpl*>(spec->plugin) != 0);
    QCOMPARE(spec->state, PluginSpec::Loaded);
    QVERIFY(!spec->hasError);
    QCOMPARE(spec->plugin->pluginSpec(), ps);
    delete manager;
    delete ps;
}

void tst_PluginSpec::initializePlugin()
{
    Internal::PluginSpecPrivate spec(0);
    MyPlugin::MyPluginImpl *impl;
    QVERIFY(spec.read("testplugin/testplugin.xml"));
    QVERIFY(spec.resolveDependencies(QSet<PluginSpec *>()));
    QVERIFY(spec.loadLibrary());
    impl = qobject_cast<MyPlugin::MyPluginImpl*>(spec.plugin);
    QVERIFY(impl != 0);
    QVERIFY(!impl->isInitialized());
    QVERIFY(spec.initializePlugin());
    QCOMPARE(spec.state, PluginSpec::Initialized);
    QVERIFY(!spec.hasError);
    QVERIFY(impl->isInitialized());
}

void tst_PluginSpec::initializeExtensions()
{
    Internal::PluginSpecPrivate spec(0);
    MyPlugin::MyPluginImpl *impl;
    QVERIFY(spec.read("testplugin/testplugin.xml"));
    QVERIFY(spec.resolveDependencies(QSet<PluginSpec *>()));
    QVERIFY(spec.loadLibrary());
    impl = qobject_cast<MyPlugin::MyPluginImpl*>(spec.plugin);
    QVERIFY(impl != 0);
    QVERIFY(spec.initializePlugin());
    QVERIFY(spec.initializeExtensions());
    QCOMPARE(spec.state, PluginSpec::Running);
    QVERIFY(!spec.hasError);
    QVERIFY(impl->isExtensionsInitialized());
}

QTEST_MAIN(tst_PluginSpec)

#include "tst_pluginspec.moc"
