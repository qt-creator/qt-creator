/****************************************************************************
**
** Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
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

#include <QList>
#include <QObject>

#include <utils/fileutils.h>

namespace Utils { class PersistentSettingsWriter; }

namespace BareMetal {
namespace Internal {

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

} // namespace Internal
} // namespace BareMetal
