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
    void getObjects();
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
    QLoggingCategory::setFilterRules(QLatin1String("qtc.*.debug=false"));
    m_pm = new PluginManager;
    m_pm->setPluginIID(QLatin1String("plugin"));
    m_objectAdded = new QSignalSpy(m_pm, SIGNAL(objectAdded(QObject*)));
    m_aboutToRemoveObject = new QSignalSpy(m_pm, SIGNAL(aboutToRemoveObject(QObject*)));
    m_pluginsChanged = new QSignalSpy(m_pm, SIGNAL(pluginsChanged()));
}

void tst_PluginManager::cleanup()
{
    m_pm->shutdown();
    delete m_pm;
    delete m_objectAdded;
    delete m_aboutToRemoveObject;
    delete m_pluginsChanged;
}

void tst_PluginManager::addRemoveObjects()
{
    QObject *object1 = new QObject;
    QObject *object2 = new QObject;
    QCOMPARE(m_pm->allObjects().size(), 0);
    m_pm->addObject(object1);
    QCOMPARE(m_objectAdded->count(), 1);
    QCOMPARE(m_objectAdded->at(0).first().value<QObject *>(), object1);
    QCOMPARE(m_aboutToRemoveObject->count(), 0);
    QVERIFY(m_pm->allObjects().contains(object1));
    QVERIFY(!m_pm->allObjects().contains(object2));
    QCOMPARE(m_pm->allObjects().size(), 1);
    m_pm->addObject(object2);
    QCOMPARE(m_objectAdded->count(), 2);
    QCOMPARE(m_objectAdded->at(1).first().value<QObject *>(), object2);
    QCOMPARE(m_aboutToRemoveObject->count(), 0);
    QVERIFY(m_pm->allObjects().contains(object1));
    QVERIFY(m_pm->allObjects().contains(object2));
    QCOMPARE(m_pm->allObjects().size(), 2);
    m_pm->removeObject(object1);
    QCOMPARE(m_objectAdded->count(), 2);
    QCOMPARE(m_aboutToRemoveObject->count(), 1);
    QCOMPARE(m_aboutToRemoveObject->at(0).first().value<QObject *>(), object1);
    QVERIFY(!m_pm->allObjects().contains(object1));
    QVERIFY(m_pm->allObjects().contains(object2));
    QCOMPARE(m_pm->allObjects().size(), 1);
    m_pm->removeObject(object2);
    QCOMPARE(m_objectAdded->count(), 2);
    QCOMPARE(m_aboutToRemoveObject->count(), 2);
    QCOMPARE(m_aboutToRemoveObject->at(1).first().value<QObject *>(), object2);
    QVERIFY(!m_pm->allObjects().contains(object1));
    QVERIFY(!m_pm->allObjects().contains(object2));
    QCOMPARE(m_pm->allObjects().size(), 0);
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
    m_pm->addObject(object2);
    QCOMPARE(m_pm->getObject<MyClass11>(), static_cast<MyClass11 *>(0));
    QCOMPARE(m_pm->getObjectByClassName("MyClass11"), static_cast<QObject*>(0));
    QCOMPARE(m_pm->getObject<MyClass1>(), static_cast<MyClass1 *>(0));
    QCOMPARE(m_pm->getObject<MyClass2>(), object2);
    QCOMPARE(m_pm->getObjectByClassName("MyClass2"), object2);
    m_pm->addObject(object11);
    QCOMPARE(m_pm->getObject<MyClass11>(), object11);
    QCOMPARE(m_pm->getObject<MyClass1>(), qobject_cast<MyClass1 *>(object11));
    QCOMPARE(m_pm->getObject<MyClass2>(), object2);
    QCOMPARE(m_pm->getObjectByName(objectName), static_cast<QObject *>(0));
    m_pm->addObject(object2b);
    QCOMPARE(m_pm->getObjectByName(objectName), object2b);
    QCOMPARE(m_pm->getObject<MyClass2>(
                    [&objectName](MyClass2 *obj) { return obj->objectName() == objectName;}), object2b);
    m_pm->removeObject(object2);
    m_pm->removeObject(object11);
    m_pm->removeObject(object2b);
    delete object2;
    delete object11;
    delete object2b;
}

void tst_PluginManager::getObjects()
{
    MyClass1 *object1 = new MyClass1;
    MyClass2 *object2 = new MyClass2;
    MyClass11 *object11 = new MyClass11;
    m_pm->addObject(object2);
    QCOMPARE(m_pm->getObjects<MyClass11>(), QList<MyClass11*>());
    QCOMPARE(m_pm->getObjects<MyClass1>(), QList<MyClass1*>());
    QCOMPARE(m_pm->getObjects<MyClass2>(), QList<MyClass2*>() << object2);
    QCOMPARE(m_pm->allObjects(), QList<QObject*>() << object2);
    m_pm->addObject(object11);
    QCOMPARE(m_pm->getObjects<MyClass11>(), QList<MyClass11*>() << object11);
    QCOMPARE(m_pm->getObjects<MyClass1>(), QList<MyClass1*>() << object11);
    QCOMPARE(m_pm->getObjects<MyClass2>(), QList<MyClass2*>() << object2);
    QCOMPARE(m_pm->allObjects(), QList<QObject*>() << object2 << object11);
    m_pm->addObject(object1);
    QCOMPARE(m_pm->getObjects<MyClass11>(), QList<MyClass11*>() << object11);
    QCOMPARE(m_pm->getObjects<MyClass1>(), QList<MyClass1*>() << object11 << object1);
    QCOMPARE(m_pm->getObjects<MyClass2>(), QList<MyClass2*>() << object2);
    QCOMPARE(m_pm->allObjects(), QList<QObject*>() << object2 << object11 << object1);

    QCOMPARE(m_pm->getObjects<MyClass1>(
                 [](MyClass1 *o){
                    return !qobject_cast<MyClass11 *>(o);} ),
             QList<MyClass1 *>() << object1);

    m_pm->removeObject(object2);
    m_pm->removeObject(object11);
    m_pm->removeObject(object1);
    delete object1;
    delete object2;
    delete object11;
}

void tst_PluginManager::circularPlugins()
{
    m_pm->setPluginPaths(QStringList() << pluginFolder(QLatin1String("circularplugins")));
    m_pm->loadPlugins();
    QList<PluginSpec *> plugins = m_pm->plugins();
    QCOMPARE(plugins.count(), 3);
    foreach (PluginSpec *spec, plugins) {
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
    m_pm->setPluginPaths(QStringList() << pluginFolder(QLatin1String("correctplugins1")));
    m_pm->loadPlugins();
    bool specError = false;
    bool runError = false;
    foreach (PluginSpec *spec, m_pm->plugins()) {
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
    foreach (QObject *obj, m_pm->allObjects()) {
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

QTEST_MAIN(tst_PluginManager)

#include "tst_pluginmanager.moc"

