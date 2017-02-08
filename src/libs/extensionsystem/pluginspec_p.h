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
#include "iplugin.h"

#include <QJsonObject>
#include <QObject>
#include <QPluginLoader>
#include <QRegExp>
#include <QStringList>
#include <QVector>
#include <QXmlStreamReader>

namespace ExtensionSystem {

class IPlugin;
class PluginManager;

namespace Internal {

class EXTENSIONSYSTEM_EXPORT PluginSpecPrivate : public QObject
{
    Q_OBJECT

public:
    PluginSpecPrivate(PluginSpec *spec);

    bool read(const QString &fileName);
    bool provides(const QString &pluginName, const QString &version) const;
    bool resolveDependencies(const QList<PluginSpec *> &specs);
    bool loadLibrary();
    bool initializePlugin();
    bool initializeExtensions();
    bool delayedInitialize();
    IPlugin::ShutdownFlag stop();
    void kill();

    void setEnabledBySettings(bool value);
    void setEnabledByDefault(bool value);
    void setForceEnabled(bool value);
    void setForceDisabled(bool value);

    QPluginLoader loader;

    QString name;
    QString version;
    QString compatVersion;
    bool required = false;
    bool hiddenByDefault = false;
    bool experimental = false;
    bool enabledByDefault = true;
    QString vendor;
    QString copyright;
    QString license;
    QString description;
    QString url;
    QString category;
    QRegExp platformSpecification;
    QVector<PluginDependency> dependencies;
    QJsonObject metaData;
    bool enabledBySettings = true;
    bool enabledIndirectly = false;
    bool forceEnabled = false;
    bool forceDisabled = false;

    QString location;
    QString filePath;
    QStringList arguments;

    QHash<PluginDependency, PluginSpec *> dependencySpecs;
    PluginSpec::PluginArgumentDescriptions argumentDescriptions;
    IPlugin *plugin = nullptr;

    PluginSpec::State state = PluginSpec::Invalid;
    bool hasError = false;
    QString errorString;

    static bool isValidVersion(const QString &version);
    static int versionCompare(const QString &version1, const QString &version2);

    void enableDependenciesIndirectly();

    bool readMetaData(const QJsonObject &pluginMetaData);

private:
    PluginSpec *q;

    bool reportError(const QString &err);
    static QRegExp &versionRegExp();
};

} // namespace Internal
} // namespace ExtensionSystem
