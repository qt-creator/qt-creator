/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef PLUGINMANAGER_P_H
#define PLUGINMANAGER_P_H

#include "pluginspec.h"

#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtCore/QStringList>
#include <QtCore/QObject>
#include <QtCore/QScopedPointer>

QT_BEGIN_NAMESPACE
class QTime;
QT_END_NAMESPACE

namespace ExtensionSystem {

class PluginManager;
class PluginCollection;

namespace Internal {

class PluginSpecPrivate;

class EXTENSIONSYSTEM_EXPORT PluginManagerPrivate
{
public:
    PluginManagerPrivate(PluginManager *pluginManager);
    virtual ~PluginManagerPrivate();

    // Object pool operations
    void addObject(QObject *obj);
    void removeObject(QObject *obj);

    // Plugin operations
    void loadPlugins();
    void setPluginPaths(const QStringList &paths);
    QList<PluginSpec *> loadQueue();
    void loadPlugin(PluginSpec *spec, PluginSpec::State destState);
    void resolveDependencies();
    void initProfiling();
    void profilingReport(const char *what, const PluginSpec *spec = 0);
    void loadSettings();
    void writeSettings();
    void removePluginSpec(PluginSpec *spec);

    QHash<QString, PluginCollection *> pluginCategories;
    QList<PluginSpec *> pluginSpecs;
    QList<PluginSpec *> testSpecs;
    QStringList pluginPaths;
    QString extension;
    QList<QObject *> allObjects; // ### make this a QList<QPointer<QObject> > > ?
    QStringList disabledPlugins;

    QStringList arguments;
    QScopedPointer<QTime> m_profileTimer;
    int m_profileElapsedMS;

    // Look in argument descriptions of the specs for the option.
    PluginSpec *pluginForOption(const QString &option, bool *requiresArgument) const;
    PluginSpec *pluginByName(const QString &name) const;

    // used by tests
    static PluginSpec *createSpec();
    static PluginSpecPrivate *privateSpec(PluginSpec *spec);
private:
    PluginCollection *defaultCollection;
    PluginManager *q;

    void readPluginPaths();
    bool loadQueue(PluginSpec *spec,
            QList<PluginSpec *> &queue,
            QList<PluginSpec *> &circularityCheckQueue);
    void stopAll();
};

} // namespace Internal
} // namespace ExtensionSystem

#endif // PLUGINMANAGER_P_H
