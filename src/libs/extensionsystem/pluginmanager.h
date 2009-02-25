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

#ifndef EXTENSIONSYSTEM_PLUGINMANAGER_H
#define EXTENSIONSYSTEM_PLUGINMANAGER_H

#include "extensionsystem_global.h"
#include <aggregation/aggregate.h>

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtCore/QStringList>
#include <QtCore/QWriteLocker>
#include <QtCore/QReadWriteLock>

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace ExtensionSystem {

namespace Internal {
    class PluginManagerPrivate;
}

class IPlugin;
class PluginSpec;

class EXTENSIONSYSTEM_EXPORT PluginManager : public QObject
{
    Q_DISABLE_COPY(PluginManager)
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

    // Plugin operations
    void loadPlugins();
    QStringList pluginPaths() const;
    void setPluginPaths(const QStringList &paths);
    QList<PluginSpec *> plugins() const;
    void setFileExtension(const QString &extension);
    QString fileExtension() const;

    // command line arguments
    QStringList arguments() const;
    bool parseOptions(const QStringList &args,
        const QMap<QString, bool> &appOptions,
        QMap<QString, QString> *foundAppOptions,
        QString *errorString);
    static void formatOptions(QTextStream &str, int optionIndentation, int descriptionIndentation);
    void formatPluginOptions(QTextStream &str, int optionIndentation, int descriptionIndentation) const;
    void formatPluginVersions(QTextStream &str) const;

    bool runningTests() const;
    QString testDataDirectory() const;

signals:
    void objectAdded(QObject *obj);
    void aboutToRemoveObject(QObject *obj);

    void pluginsChanged();
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
