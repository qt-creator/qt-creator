// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "extensionsystem_global.h"

#include <QHash>
#include <QStaticPlugin>
#include <QString>
#include <QVector>

QT_BEGIN_NAMESPACE
class QRegularExpression;
QT_END_NAMESPACE

namespace ExtensionSystem {

namespace Internal {

class OptionsParser;
class PluginSpecPrivate;
class PluginManagerPrivate;

} // Internal

class IPlugin;
class PluginView;

struct EXTENSIONSYSTEM_EXPORT PluginDependency
{
    enum Type {
        Required,
        Optional,
        Test
    };

    PluginDependency() : type(Required) {}

    friend size_t qHash(const PluginDependency &value);

    QString name;
    QString version;
    Type type;
    bool operator==(const PluginDependency &other) const;
    QString toString() const;
};

struct EXTENSIONSYSTEM_EXPORT PluginArgumentDescription
{
    QString name;
    QString parameter;
    QString description;
};

struct EXTENSIONSYSTEM_EXPORT PerformanceData
{
    qint64 load = 0;
    qint64 initialize = 0;
    qint64 extensionsInitialized = 0;
    qint64 delayedInitialize = 0;

    qint64 total() const { return load + initialize + extensionsInitialized + delayedInitialize; }
    QString summary() const
    {
        return QString("l: %1ms, i: %2ms, x: %3ms, d: %4ms")
            .arg(load, 3)
            .arg(initialize, 3)
            .arg(extensionsInitialized, 3)
            .arg(delayedInitialize, 3);
    }
};

class EXTENSIONSYSTEM_EXPORT PluginSpec
{
public:
    enum State { Invalid, Read, Resolved, Loaded, Initialized, Running, Stopped, Deleted};

    ~PluginSpec();

    // information from the xml file, valid after 'Read' state is reached
    QString name() const;
    QString version() const;
    QString compatVersion() const;
    QString vendor() const;
    QString copyright() const;
    QString license() const;
    QString description() const;
    QString longDescription() const;
    QString url() const;
    QString category() const;
    QString revision() const;
    QRegularExpression platformSpecification() const;
    bool isAvailableForHostPlatform() const;
    bool isRequired() const;
    bool isExperimental() const;
    bool isEnabledByDefault() const;
    bool isEnabledBySettings() const;
    bool isEffectivelyEnabled() const;
    bool isEnabledIndirectly() const;
    bool isForceEnabled() const;
    bool isForceDisabled() const;
    QVector<PluginDependency> dependencies() const;
    QJsonObject metaData() const;
    const PerformanceData &performanceData() const;

    using PluginArgumentDescriptions = QVector<PluginArgumentDescription>;
    PluginArgumentDescriptions argumentDescriptions() const;

    // other information, valid after 'Read' state is reached
    QString location() const;
    QString filePath() const;

    QStringList arguments() const;
    void setArguments(const QStringList &arguments);
    void addArgument(const QString &argument);

    bool provides(const QString &pluginName, const QString &version) const;

    // dependency specs, valid after 'Resolved' state is reached
    QHash<PluginDependency, PluginSpec *> dependencySpecs() const;
    bool requiresAny(const QSet<PluginSpec *> &plugins) const;

    // linked plugin instance, valid after 'Loaded' state is reached
    IPlugin *plugin() const;

    // state
    State state() const;
    bool hasError() const;
    QString errorString() const;

    void setEnabledBySettings(bool value);

    static PluginSpec *read(const QString &filePath);
    static PluginSpec *read(const QStaticPlugin &plugin);

private:
    PluginSpec();

    Internal::PluginSpecPrivate *d;
    friend class PluginView;
    friend class Internal::OptionsParser;
    friend class Internal::PluginManagerPrivate;
    friend class Internal::PluginSpecPrivate;
};

} // namespace ExtensionSystem
