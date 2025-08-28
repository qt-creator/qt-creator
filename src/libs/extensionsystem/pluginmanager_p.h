// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "pluginspec.h"
#include "pluginmanager.h"

#include <utils/algorithm.h>

#include <QElapsedTimer>
#include <QMutex>
#include <QObject>
#include <QReadWriteLock>
#include <QScopedPointer>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <QWaitCondition>

#include <queue>

QT_BEGIN_NAMESPACE
class QTime;
class QEventLoop;
QT_END_NAMESPACE

namespace Utils {
class FutureSynchronizer;
class QtcSettings;
}

namespace ExtensionSystem {

class PluginManager;

namespace Internal {

class EXTENSIONSYSTEM_TEST_EXPORT PluginManagerPrivate : public QObject
{
public:
    PluginManagerPrivate(PluginManager *pluginManager);
    ~PluginManagerPrivate() override;

    // Object pool operations
    void addObject(QObject *obj);
    void removeObject(QObject *obj);

    // Plugin operations
    void checkForProblematicPlugins();
    void loadPlugins();
    void loadPluginsAtRuntime(const QSet<PluginSpec *> &plugins);
    void addPlugins(const QList<PluginSpec *> &specs);

    void shutdown();
    void setPluginPaths(const Utils::FilePaths &paths);
    const QList<ExtensionSystem::PluginSpec *> loadQueue();
    void loadPlugin(PluginSpec *spec, PluginSpec::State destState);
    void resolveDependencies();
    void enableDependenciesIndirectly();
    void increaseProfilingVerbosity();
    void enableTracing(const QString &filePath);
    QString profilingSummary(qint64 *totalOut = nullptr) const;
    void printProfilingSummary() const;
    void profilingReport(const char *what, const PluginSpec *spec, qint64 *target = nullptr);
    void setSettings(Utils::QtcSettings *settings);
    void setGlobalSettings(Utils::QtcSettings *settings);
    void readSettings();
    void writeSettings();

    bool acceptTermsAndConditions(PluginSpec *spec);
    void setAcceptTermsAndConditionsCallback(const std::function<bool(PluginSpec *)> &callback);
    void readPluginPaths();

    void removePluginsAfterRestart();
    void installPluginsAfterRestart();

    Utils::Result<> removePluginOnRestart(const QString &pluginId);
    void installPluginOnRestart(const Utils::FilePath &src, const Utils::FilePath &dest);

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

    QHash<QString, QList<PluginSpec *>> pluginCategories;
    QList<PluginSpec *> pluginSpecs;
    std::vector<TestSpec> testSpecs;
    Utils::FilePaths pluginPaths;
    QString pluginIID;
    QObjectList allObjects;      // ### make this a QList<QPointer<QObject> > > ?
    QStringList defaultDisabledPlugins; // Plugins/Ignored from install settings
    QStringList defaultEnabledPlugins; // Plugins/ForceEnabled from install settings
    QStringList disabledPlugins;
    QStringList forceEnabledPlugins;
    QStringList pluginsWithAcceptedTermsAndConditions;
    // delayed initialization
    QTimer delayedInitializeTimer;
    std::queue<PluginSpec *> delayedInitializeQueue;
    // ansynchronous shutdown
    QSet<PluginSpec *> asynchronousPlugins;  // plugins that have requested async shutdown
    QEventLoop *shutdownEventLoop = nullptr; // used for async shutdown

    QStringList arguments;
    QStringList argumentsForRestart;
    QScopedPointer<QElapsedTimer> m_profileTimer;
    qint64 m_profileElapsedMS = 0;
    qint64 m_totalUntilDelayedInitialize = 0;
    qint64 m_totalStartupMS = 0;
    unsigned m_profilingVerbosity = 0;
    Utils::QtcSettings *settings = nullptr;
    Utils::QtcSettings *globalSettings = nullptr;

    std::function<bool(PluginSpec *)> acceptTermsAndConditionsCallback;

    // Look in argument descriptions of the specs for the option.
    PluginSpec *pluginForOption(const QString &option, bool *requiresArgument) const;
    PluginSpec *pluginById(const QString &id) const;

    static void addTestCreator(IPlugin *plugin, const std::function<QObject *()> &testCreator);

    mutable QReadWriteLock m_lock;

    bool m_isInitializationDone = false;
    bool enableCrashCheck = true;
    bool m_isShuttingDown = false;

    QHash<QString, std::function<bool()>> m_scenarios;
    QString m_requestedScenario;
    std::atomic_bool m_isScenarioRunning = false; // if it's running, the running one is m_requestedScenario
    std::atomic_bool m_isScenarioFinished = false; // if it's running, the running one is m_requestedScenario
    bool m_scenarioFullyInitialized = false;
    QMutex m_scenarioMutex;
    QWaitCondition m_scenarioWaitCondition;

    PluginManager::ProcessData m_creatorProcessData;

private:
    PluginManager *q;

    void startDelayedInitialize();

    bool loadQueue(PluginSpec *spec,
                   QList<ExtensionSystem::PluginSpec *> &queue,
                   QList<ExtensionSystem::PluginSpec *> &circularityCheckQueue);
    void stopAll();
    void deleteAll();
    void checkForDuplicatePlugins();

#ifdef EXTENSIONSYSTEM_WITH_TESTOPTION
    void startTests();
#endif
};

} // namespace Internal
} // namespace ExtensionSystem
