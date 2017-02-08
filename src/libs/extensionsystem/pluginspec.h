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

#include <QString>
#include <QHash>
#include <QVector>

QT_BEGIN_NAMESPACE
class QStringList;
class QRegExp;
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

    QString name;
    QString version;
    Type type;
    bool operator==(const PluginDependency &other) const;
    QString toString() const;
};

uint qHash(const ExtensionSystem::PluginDependency &value);

struct EXTENSIONSYSTEM_EXPORT PluginArgumentDescription
{
    QString name;
    QString parameter;
    QString description;
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
    QString url() const;
    QString category() const;
    QRegExp platformSpecification() const;
    bool isAvailableForHostPlatform() const;
    bool isRequired() const;
    bool isHiddenByDefault() const;
    bool isExperimental() const;
    bool isEnabledByDefault() const;
    bool isEnabledBySettings() const;
    bool isEffectivelyEnabled() const;
    bool isEnabledIndirectly() const;
    bool isForceEnabled() const;
    bool isForceDisabled() const;
    QVector<PluginDependency> dependencies() const;
    QJsonObject metaData() const;

    typedef QVector<PluginArgumentDescription> PluginArgumentDescriptions;
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

private:
    PluginSpec();

    Internal::PluginSpecPrivate *d;
    friend class PluginView;
    friend class Internal::OptionsParser;
    friend class Internal::PluginManagerPrivate;
    friend class Internal::PluginSpecPrivate;
};

} // namespace ExtensionSystem
