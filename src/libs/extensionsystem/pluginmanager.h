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

#include "extensionsystem_global.h"

#include <aggregation/aggregate.h>
#include <utils/qtcsettings.h>

#include <QObject>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace ExtensionSystem {
class IPlugin;
class PluginSpec;

namespace Internal { class PluginManagerPrivate; }

class EXTENSIONSYSTEM_EXPORT PluginManager : public QObject
{
    Q_OBJECT

public:
    static PluginManager *instance();

    PluginManager();
    ~PluginManager() override;

    // Object pool operations
    static void addObject(QObject *obj);
    static void removeObject(QObject *obj);
    static QVector<QObject *> allObjects();
    static QReadWriteLock *listLock();

    // This is useful for soft dependencies using pure interfaces.
    template <typename T> static T *getObject()
    {
        QReadLocker lock(listLock());
        const QVector<QObject *> all = allObjects();
        for (QObject *obj : all) {
            if (T *result = qobject_cast<T *>(obj))
                return result;
        }
        return nullptr;
    }
    template <typename T, typename Predicate> static T *getObject(Predicate predicate)
    {
        QReadLocker lock(listLock());
        const QVector<QObject *> all = allObjects();
        for (QObject *obj : all) {
            if (T *result = qobject_cast<T *>(obj))
                if (predicate(result))
                    return result;
        }
        return 0;
    }

    static QObject *getObjectByName(const QString &name);

    // Plugin operations
    static QVector<PluginSpec *> loadQueue();
    static void loadPlugins();
    static QStringList pluginPaths();
    static void setPluginPaths(const QStringList &paths);
    static QString pluginIID();
    static void setPluginIID(const QString &iid);
    static const QVector<PluginSpec *> plugins();
    static QHash<QString, QVector<PluginSpec *>> pluginCollections();
    static bool hasError();
    static const QStringList allErrors();
    static const QSet<PluginSpec *> pluginsRequiringPlugin(PluginSpec *spec);
    static const QSet<PluginSpec *> pluginsRequiredByPlugin(PluginSpec *spec);
    static void checkForProblematicPlugins();

    // Settings
    static void setSettings(Utils::QtcSettings *settings);
    static Utils::QtcSettings *settings();
    static void setGlobalSettings(Utils::QtcSettings *settings);
    static Utils::QtcSettings *globalSettings();
    static void writeSettings();

    // command line arguments
    static QStringList arguments();
    static QStringList argumentsForRestart();
    static bool parseOptions(const QStringList &args,
        const QMap<QString, bool> &appOptions,
        QMap<QString, QString> *foundAppOptions,
        QString *errorString);
    static void formatOptions(QTextStream &str, int optionIndentation, int descriptionIndentation);
    static void formatPluginOptions(QTextStream &str, int optionIndentation, int descriptionIndentation);
    static void formatPluginVersions(QTextStream &str);

    static QString serializedArguments();

    static bool testRunRequested();

#ifdef WITH_TESTS
    static bool registerScenario(const QString &scenarioId, std::function<bool()> scenarioStarter);
    static bool isScenarioRequested();
    static bool runScenario();
    static bool isScenarioRunning(const QString &scenarioId);
    // static void triggerScenarioPoint(const QVariant pointData); // ?? called from scenario point
    static bool finishScenario();
    static void waitForScenarioFullyInitialized();
    // signals:
    // void scenarioPointTriggered(const QVariant pointData); // ?? e.g. in StringTable::GC() -> post a call to quit into main thread and sleep for 5 seconds in the GC thread
#endif

    struct ProcessData {
        QString m_executable;
        QStringList m_args;
        QString m_workingPath;
    };

    static void setCreatorProcessData(const ProcessData &data);
    static ProcessData creatorProcessData();

    static void profilingReport(const char *what, const PluginSpec *spec = nullptr);

    static QString platformName();

    static bool isInitializationDone();

    static void remoteArguments(const QString &serializedArguments, QObject *socket);
    static void shutdown();

    static QString systemInformation();

signals:
    void objectAdded(QObject *obj);
    void aboutToRemoveObject(QObject *obj);

    void pluginsChanged();
    void initializationDone();
    void testsFinished(int failedTests);
    void scenarioFinished(int exitCode);

    friend class Internal::PluginManagerPrivate;
};

} // namespace ExtensionSystem
