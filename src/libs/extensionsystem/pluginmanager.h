/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef EXTENSIONSYSTEM_PLUGINMANAGER_H
#define EXTENSIONSYSTEM_PLUGINMANAGER_H

#include "extensionsystem_global.h"
#include <aggregation/aggregate.h>

#include <QtCore/QObject>
#include <QtCore/QReadWriteLock>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QTextStream;
class QSettings;
QT_END_NAMESPACE

namespace ExtensionSystem {
class PluginCollection;

namespace Internal {
class PluginManagerPrivate;
}

class IPlugin;
class PluginSpec;

class EXTENSIONSYSTEM_EXPORT PluginManager : public QObject
{
    Q_OBJECT

public:
    static PluginManager *instance();

    PluginManager();
    virtual ~PluginManager();

    // Object pool operations
    void addObject(QObject *obj);
    void removeObject(QObject *obj);
    QList<QObject *> allObjects() const;
    template <typename T> QList<T *> getObjects() const
    {
        QReadLocker lock(&m_lock);
        QList<T *> results;
        QList<QObject *> all = allObjects();
        QList<T *> result;
        foreach (QObject *obj, all) {
            result = Aggregation::query_all<T>(obj);
            if (!result.isEmpty())
                results += result;
        }
        return results;
    }
    template <typename T> T *getObject() const
    {
        QReadLocker lock(&m_lock);
        QList<QObject *> all = allObjects();
        T *result = 0;
        foreach (QObject *obj, all) {
            if ((result = Aggregation::query<T>(obj)) != 0)
                break;
        }
        return result;
    }

    QObject *getObjectByName(const QString &name) const;
    QObject *getObjectByClassName(const QString &className) const;

    // Plugin operations
    QList<PluginSpec *> loadQueue();
    void loadPlugins();
    QStringList pluginPaths() const;
    void setPluginPaths(const QStringList &paths);
    QList<PluginSpec *> plugins() const;
    QHash<QString, PluginCollection *> pluginCollections() const;
    void setFileExtension(const QString &extension);
    QString fileExtension() const;
    bool hasError() const;

    // Settings
    void setSettings(QSettings *settings);
    QSettings *settings() const;
    void readSettings();
    void writeSettings();

    // command line arguments
    QStringList arguments() const;
    bool parseOptions(const QStringList &args,
        const QMap<QString, bool> &appOptions,
        QMap<QString, QString> *foundAppOptions,
        QString *errorString);
    static void formatOptions(QTextStream &str, int optionIndentation, int descriptionIndentation);
    void formatPluginOptions(QTextStream &str, int optionIndentation, int descriptionIndentation) const;
    void formatPluginVersions(QTextStream &str) const;

    QString serializedArguments() const;

    bool runningTests() const;
    QString testDataDirectory() const;

    void profilingReport(const char *what, const PluginSpec *spec = 0);

signals:
    void objectAdded(QObject *obj);
    void aboutToRemoveObject(QObject *obj);

    void pluginsChanged();

public slots:
    void remoteArguments(const QString &serializedArguments);
    void shutdown();

private slots:
    void startTests();

private:
    Internal::PluginManagerPrivate *d;
    static PluginManager *m_instance;
    mutable QReadWriteLock m_lock;

    friend class Internal::PluginManagerPrivate;
};

} // namespace ExtensionSystem

#endif // EXTENSIONSYSTEM_PLUGINMANAGER_H
