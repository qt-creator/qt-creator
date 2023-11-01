// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QObject>

#include <utils/filepath.h>

namespace Utils { class PersistentSettingsWriter; }

namespace BareMetal::Internal {

class BareMetalPlugin;
class BareMetalPluginPrivate;
class IDebugServerProvider;
class IDebugServerProviderFactory;

// DebugServerProviderManager

class DebugServerProviderManager final : public QObject
{
    Q_OBJECT

public:
    static DebugServerProviderManager *instance();
    ~DebugServerProviderManager() final;

    static QList<IDebugServerProvider *> providers();
    static QList<IDebugServerProviderFactory *> factories();
    static IDebugServerProvider *findProvider(const QString &id);
    static IDebugServerProvider *findByDisplayName(const QString &displayName);
    static bool registerProvider(IDebugServerProvider *provider);
    static void deregisterProvider(IDebugServerProvider *provider);

signals:
    void providerAdded(IDebugServerProvider *provider);
    void providerRemoved(IDebugServerProvider *provider);
    void providerUpdated(IDebugServerProvider *provider);
    void providersChanged();
    void providersLoaded();

private:
    void saveProviders();
    DebugServerProviderManager();

    void restoreProviders();
    static void notifyAboutUpdate(IDebugServerProvider *provider);

    Utils::PersistentSettingsWriter *m_writer = nullptr;
    QList<IDebugServerProvider *> m_providers;
    const Utils::FilePath m_configFile;
    const QList<IDebugServerProviderFactory *> m_factories;

    friend class BareMetalPlugin; // for restoreProviders
    friend class BareMetalPluginPrivate; // for constructor
    friend class IDebugServerProvider;
};

} // BareMetal::Internal
