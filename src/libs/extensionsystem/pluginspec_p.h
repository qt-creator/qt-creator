// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "pluginspec.h"
#include "iplugin.h"

#include <QJsonObject>
#include <QObject>
#include <QPluginLoader>
#include <QRegularExpression>
#include <QStringList>
#include <QVector>
#include <QXmlStreamReader>

#include <optional>

namespace ExtensionSystem {

class IPlugin;

namespace Internal {

class EXTENSIONSYSTEM_EXPORT PluginSpecPrivate : public QObject
{
    Q_OBJECT

public:
    PluginSpecPrivate(PluginSpec *spec);

    void reset();
    bool read(const QString &fileName);
    bool read(const QStaticPlugin &plugin);
    bool provides(const QString &pluginName, const QString &version) const;
    bool resolveDependencies(const QVector<PluginSpec *> &specs);
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

    std::optional<QPluginLoader> loader;
    std::optional<QStaticPlugin> staticPlugin;

    QString name;
    QString version;
    QString compatVersion;
    bool required = false;
    bool experimental = false;
    bool enabledByDefault = true;
    QString vendor;
    QString copyright;
    QString license;
    QString description;
    QString longDescription;
    QString url;
    QString category;
    QRegularExpression platformSpecification;
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

    QList<TestCreator> registeredPluginTests;

    PluginSpec::State state = PluginSpec::Invalid;
    bool hasError = false;
    QString errorString;

    static bool isValidVersion(const QString &version);
    static int versionCompare(const QString &version1, const QString &version2);

    QVector<PluginSpec *> enableDependenciesIndirectly(bool enableTestDependencies = false);

    bool readMetaData(const QJsonObject &pluginMetaData);

private:
    PluginSpec *q;

    bool reportError(const QString &err);
    static const QRegularExpression &versionRegExp();
};

} // namespace Internal
} // namespace ExtensionSystem
