// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlpreviewplugin_test.h"
#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <QtTest>
#include <QVariant>

typedef QByteArray (*TestFileLoader)(const QString &, bool *);
typedef void (*TestFpsHandler)(quint16[8]);

Q_DECLARE_METATYPE(TestFileLoader)
Q_DECLARE_METATYPE(TestFpsHandler)

namespace QmlPreview {

class QmlPreviewPluginTest : public QObject
{
    Q_OBJECT

private slots:
    void testFileLoaderProperty();
    void testZoomFactorProperty();
    void testFpsHandlerProperty();
};

static ExtensionSystem::IPlugin *getPlugin()
{
    const QVector<ExtensionSystem::PluginSpec *> plugins = ExtensionSystem::PluginManager::plugins();
    auto it = std::find_if(plugins.begin(), plugins.end(), [](ExtensionSystem::PluginSpec *spec) {
        return spec->name() == "QmlPreview";
    });

    return (it == plugins.end()) ? nullptr : (*it)->plugin();
}

void QmlPreviewPluginTest::testFileLoaderProperty()
{
    ExtensionSystem::IPlugin *plugin = getPlugin();
    QVERIFY(plugin);

    QVariant var = plugin->property("fileLoader");
    TestFileLoader loader = qvariant_cast<TestFileLoader>(var);
    bool success = true;
    loader(QString("testzzzztestzzzztest"), &success);
    QVERIFY(!success);
}

void QmlPreviewPluginTest::testZoomFactorProperty()
{
    ExtensionSystem::IPlugin *plugin = getPlugin();
    QVERIFY(plugin);

    QSignalSpy spy(plugin, SIGNAL(zoomFactorChanged(float)));

    QCOMPARE(qvariant_cast<float>(plugin->property("zoomFactor")), -1.0f);
    plugin->setProperty("zoomFactor", 2.0f);
    QCOMPARE(qvariant_cast<float>(plugin->property("zoomFactor")), 2.0f);
    plugin->setProperty("zoomFactor", 1.0f);
    QCOMPARE(qvariant_cast<float>(plugin->property("zoomFactor")), 1.0f);
    QCOMPARE(spy.count(), 2);
}

void QmlPreviewPluginTest::testFpsHandlerProperty()
{
    ExtensionSystem::IPlugin *plugin = getPlugin();
    QVERIFY(plugin);

    QVariant var = plugin->property("fpsHandler");
    TestFpsHandler handler = qvariant_cast<TestFpsHandler>(var);
    QVERIFY(handler);
    quint16 stats[] = { 43, 44, 45, 46, 47, 48, 49, 50 };
    handler(stats);
}

QObject *createQmlPreviewPluginTest()
{
    return new QmlPreviewPluginTest;
}

} // namespace QmlPreview

#include "qmlpreviewplugin_test.moc"
