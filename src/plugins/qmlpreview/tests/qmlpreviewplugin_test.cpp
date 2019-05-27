/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qmlpreviewplugin_test.h"
#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <QtTest>
#include <QVariant>

Q_DECLARE_METATYPE(QmlPreview::Internal::TestFileLoader)
Q_DECLARE_METATYPE(QmlPreview::Internal::TestFpsHandler)

namespace QmlPreview {
namespace Internal {

QmlPreviewPluginTest::QmlPreviewPluginTest(QObject *parent) : QObject(parent)
{
}

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

void QmlPreviewPluginTest::testLocaleProperty()
{
    ExtensionSystem::IPlugin *plugin = getPlugin();
    QVERIFY(plugin);

    QSignalSpy spy(plugin, SIGNAL(localeChanged(QString)));

    QCOMPARE(plugin->property("locale").toString(), QString());
    plugin->setProperty("locale", "de_DE");
    QCOMPARE(plugin->property("locale").toString(), QString("de_DE"));
    plugin->setProperty("locale", "qt_QT");
    QCOMPARE(plugin->property("locale").toString(), QString("qt_QT"));
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

} // namespace Internal
} // namespace QmlPreview
