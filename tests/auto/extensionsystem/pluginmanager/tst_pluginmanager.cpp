// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>
#include <extensionsystem/iplugin.h>

#include <QtTest>

#include <QObject>

using namespace ExtensionSystem;

class SignalReceiver;

class tst_PluginManager : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void addRemoveObjects();
    void getObject();
    void circularPlugins();
    void correctPlugins1();

private:
    PluginManager *m_pm;
    QSignalSpy *m_objectAdded;
    QSignalSpy *m_aboutToRemoveObject;
    QSignalSpy *m_pluginsChanged;
};

class MyClass1 : public QObject
{
    Q_OBJECT
};

class MyClass2 : public QObject
{
    Q_OBJECT
};

class MyClass11 : public MyClass1
{
    Q_OBJECT
};

static QString pluginFolder(const QLatin1String &folder)
{
    return QLatin1String(PLUGINMANAGER_TESTS_DIR) + QLatin1String("/") + folder;
}

void tst_PluginManager::init()
{
    m_pm = new PluginManager;
    PluginManager::setSettings(new Utils::QtcSettings);
    PluginManager::setPluginIID(QLatin1String("plugin"));
    m_objectAdded = new QSignalSpy(m_pm, &PluginManager::objectAdded);
    m_aboutToRemoveObject = new QSignalSpy(m_pm, &PluginManager::aboutToRemoveObject);
    m_pluginsChanged = new QSignalSpy(m_pm, &PluginManager::pluginsChanged);
}

void tst_PluginManager::cleanup()
{
    PluginManager::shutdown();
    delete m_pm;
    delete m_objectAdded;
    delete m_aboutToRemoveObject;
    delete m_pluginsChanged;
}

void tst_PluginManager::addRemoveObjects()
{
    QObject *object1 = new QObject;
    QObject *object2 = new QObject;
    QCOMPARE(PluginManager::allObjects().size(), 0);
    PluginManager::addObject(object1);
    QCOMPARE(m_objectAdded->count(), 1);
    QCOMPARE(m_objectAdded->at(0).first().value<QObject *>(), object1);
    QCOMPARE(m_aboutToRemoveObject->count(), 0);
    QVERIFY(PluginManager::allObjects().contains(object1));
    QVERIFY(!PluginManager::allObjects().contains(object2));
    QCOMPARE(PluginManager::allObjects().size(), 1);
    PluginManager::addObject(object2);
    QCOMPARE(m_objectAdded->count(), 2);
    QCOMPARE(m_objectAdded->at(1).first().value<QObject *>(), object2);
    QCOMPARE(m_aboutToRemoveObject->count(), 0);
    QVERIFY(PluginManager::allObjects().contains(object1));
    QVERIFY(PluginManager::allObjects().contains(object2));
    QCOMPARE(PluginManager::allObjects().size(), 2);
    PluginManager::removeObject(object1);
    QCOMPARE(m_objectAdded->count(), 2);
    QCOMPARE(m_aboutToRemoveObject->count(), 1);
    QCOMPARE(m_aboutToRemoveObject->at(0).first().value<QObject *>(), object1);
    QVERIFY(!PluginManager::allObjects().contains(object1));
    QVERIFY(PluginManager::allObjects().contains(object2));
    QCOMPARE(PluginManager::allObjects().size(), 1);
    PluginManager::removeObject(object2);
    QCOMPARE(m_objectAdded->count(), 2);
    QCOMPARE(m_aboutToRemoveObject->count(), 2);
    QCOMPARE(m_aboutToRemoveObject->at(1).first().value<QObject *>(), object2);
    QVERIFY(!PluginManager::allObjects().contains(object1));
    QVERIFY(!PluginManager::allObjects().contains(object2));
    QCOMPARE(PluginManager::allObjects().size(), 0);
    delete object1;
    delete object2;
}

void tst_PluginManager::getObject()
{
    MyClass2 *object2 = new MyClass2;
    MyClass11 *object11 = new MyClass11;
    MyClass2 *object2b = new MyClass2;
    const QString objectName = QLatin1String("OBJECTNAME");
    object2b->setObjectName(objectName);
    PluginManager::addObject(object2);
    QCOMPARE(PluginManager::getObject<MyClass11>(), static_cast<MyClass11 *>(0));
    QCOMPARE(PluginManager::getObject<MyClass1>(), static_cast<MyClass1 *>(0));
    QCOMPARE(PluginManager::getObject<MyClass2>(), object2);
    PluginManager::addObject(object11);
    QCOMPARE(PluginManager::getObject<MyClass11>(), object11);
    QCOMPARE(PluginManager::getObject<MyClass1>(), qobject_cast<MyClass1 *>(object11));
    QCOMPARE(PluginManager::getObject<MyClass2>(), object2);
    QCOMPARE(PluginManager::getObjectByName(objectName), static_cast<QObject *>(0));
    PluginManager::addObject(object2b);
    QCOMPARE(PluginManager::getObjectByName(objectName), object2b);
    QCOMPARE(PluginManager::getObject<MyClass2>(
                 [&objectName](MyClass2 *obj) { return obj->objectName() == objectName;}), object2b);
    PluginManager::removeObject(object2);
    PluginManager::removeObject(object11);
    PluginManager::removeObject(object2b);
    delete object2;
    delete object11;
    delete object2b;
}

void tst_PluginManager::circularPlugins()
{
    PluginManager::setPluginPaths(QStringList() << pluginFolder(QLatin1String("circularplugins")));
    PluginManager::loadPlugins();
    const QVector<PluginSpec *> plugins = PluginManager::plugins();
    QCOMPARE(plugins.count(), 3);
    for (PluginSpec *spec : plugins) {
        if (spec->name() == "plugin1") {
            QVERIFY(spec->hasError());
            QCOMPARE(spec->state(), PluginSpec::Resolved);
            QCOMPARE(spec->plugin(), static_cast<IPlugin *>(0));
        } else if (spec->name() == "plugin2") {
            QVERIFY2(!spec->hasError(), qPrintable(spec->errorString()));
            QCOMPARE(spec->state(), PluginSpec::Running);
        } else if (spec->name() == "plugin3") {
            QVERIFY(spec->hasError());
            QCOMPARE(spec->state(), PluginSpec::Resolved);
            QCOMPARE(spec->plugin(), static_cast<IPlugin *>(0));
        }
    }
}

void tst_PluginManager::correctPlugins1()
{
    PluginManager::setPluginPaths(QStringList() << pluginFolder(QLatin1String("correctplugins1")));
    PluginManager::loadPlugins();
    bool specError = false;
    bool runError = false;
    const QVector<PluginSpec *> plugins = PluginManager::plugins();
    for (PluginSpec *spec : plugins) {
        if (spec->hasError()) {
            qDebug() << spec->filePath();
            qDebug() << spec->errorString();
        }
        specError = specError || spec->hasError();
        runError = runError || (spec->state() != PluginSpec::Running);
    }
    QVERIFY(!specError);
    QVERIFY(!runError);
    bool plugin1running = false;
    bool plugin2running = false;
    bool plugin3running = false;
    const QVector<QObject *> objs = PluginManager::allObjects();
    for (QObject *obj : objs) {
        if (obj->objectName() == "MyPlugin1_running")
            plugin1running = true;
        else if (obj->objectName() == "MyPlugin2_running")
            plugin2running = true;
        else if (obj->objectName() == "MyPlugin3_running")
            plugin3running = true;
    }
    QVERIFY(plugin1running);
    QVERIFY(plugin2running);
    QVERIFY(plugin3running);
}

QTEST_GUILESS_MAIN(tst_PluginManager)

#include "tst_pluginmanager.moc"

