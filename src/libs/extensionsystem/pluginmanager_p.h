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

#pragma once

#include "pluginspec.h"

#include <utils/algorithm.h>

#include <QElapsedTimer>
#include <QObject>
#include <QReadWriteLock>
#include <QScopedPointer>
#include <QSet>
#include <QStringList>

#include <queue>

QT_BEGIN_NAMESPACE
class QTime;
class QTimer;
class QSettings;
class QEventLoop;
QT_END_NAMESPACE

namespace ExtensionSystem {

class PluginManager;

namespace Internal {

class PluginSpecPrivate;

class EXTENSIONSYSTEM_EXPORT PluginManagerPrivate : public QObject
{
    Q_OBJECT
public:
    PluginManagerPrivate(PluginManager *pluginManager);
    ~PluginManagerPrivate() override;

    // Object pool operations
    void addObject(QObject *obj);
    void removeObject(QObject *obj);

    // Plugin operations
    void checkForProblematicPlugins();
    void loadPlugins();
    void shutdown();
    void setPluginPaths(const QStringList &paths);
    const QVector<ExtensionSystem::PluginSpec *> loadQueue();
    void loadPlugin(PluginSpec *spec, PluginSpec::State destState);
    void resolveDependencies();
    void enableDependenciesIndirectly();
    void initProfiling();
    void profilingSummary() const;
    void profilingReport(const char *what, const PluginSpec *spec = nullptr);
    void setSettings(QSettings *settings);
    void setGlobalSettings(QSettings *settings);
    void readSettings();
    void writeSettings();

    class TestSpec {
    public:
        TestSpec(PluginSpec *pluginSpec, const QStringList &testFunctionsOrObjects = QStringList())
            : pluginSpec(pluginSpec)
            , testFunctionsOrObjects(testFunctionsOrObjects)
        {}
        PluginSpec *pluginSpec = nullptr;
        QStringList testFunctionsOrObjects;
    };

    bool containsTestSpec(PluginSpec *pluginSpec) const
    {
        return Utils::contains(testSpecs, [pluginSpec](const TestSpec &s) { return s.pluginSpec == pluginSpec; });
    }

    void removeTestSpec(PluginSpec *pluginSpec)
    {
        testSpecs = Utils::filtered(testSpecs, [pluginSpec](const TestSpec &s) { return s.pluginSpec != pluginSpec; });
    }

    QHash<QString, QVector<PluginSpec *>> pluginCategories;
    QVector<PluginSpec *> pluginSpecs;
    std::vector<TestSpec> testSpecs;
    QStringList pluginPaths;
    QString pluginIID;
    QVector<QObject *> allObjects;      // ### make this a QVector<QPointer<QObject> > > ?
    QStringList defaultDisabledPlugins; // Plugins/Ignored from install settings
    QStringList defaultEnabledPlugins; // Plugins/ForceEnabled from install settings
    QStringList disabledPlugins;
    QStringList forceEnabledPlugins;
    // delayed initialization
    QTimer *delayedInitializeTimer = nullptr;
    std::queue<PluginSpec *> delayedInitializeQueue;
    // ansynchronous shutdown
    QSet<PluginSpec *> asynchronousPlugins;  // plugins that have requested async shutdown
    QEventLoop *shutdownEventLoop = nullptr; // used for async shutdown

    QStringList arguments;
    QStringList argumentsForRestart;
    QScopedPointer<QElapsedTimer> m_profileTimer;
    QHash<const PluginSpec *, int> m_profileTotal;
    int m_profileElapsedMS = 0;
    unsigned m_profilingVerbosity = 0;
    QSettings *settings = nullptr;
    QSettings *globalSettings = nullptr;

    // Look in argument descriptions of the specs for the option.
    PluginSpec *pluginForOption(const QString &option, bool *requiresArgument) const;
    PluginSpec *pluginByName(const QString &name) const;

    // used by tests
    static PluginSpec *createSpec();
    static PluginSpecPrivate *privateSpec(PluginSpec *spec);

    mutable QReadWriteLock m_lock;

    bool m_isInitializationDone = false;

private:
    PluginManager *q;

    void nextDelayedInitialize();
    void asyncShutdownFinished();

    void readPluginPaths();
    bool loadQueue(PluginSpec *spec,
                   QVector<ExtensionSystem::PluginSpec *> &queue,
                   QVector<ExtensionSystem::PluginSpec *> &circularityCheckQueue);
    void stopAll();
    void deleteAll();

#ifdef WITH_TESTS
    void startTests();
#endif
};

} // namespace Internal
} // namespace ExtensionSystem
