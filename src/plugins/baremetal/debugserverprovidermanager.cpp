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

#include "debugserverprovidermanager.h"
#include "idebugserverprovider.h"

// GDB debug servers.
#include "debugservers/gdb/openocdgdbserverprovider.h"
#include "debugservers/gdb/stlinkutilgdbserverprovider.h"
#include "debugservers/gdb/jlinkgdbserverprovider.h"
#include "debugservers/gdb/eblinkgdbserverprovider.h"

// UVSC debug servers.
#include "debugservers/uvsc/simulatoruvscserverprovider.h"
#include "debugservers/uvsc/stlinkuvscserverprovider.h"
#include "debugservers/uvsc/jlinkuvscserverprovider.h"

#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>

#include <QDir>

namespace BareMetal {
namespace Internal {

const char dataKeyC[] = "DebugServerProvider.";
const char countKeyC[] = "DebugServerProvider.Count";
const char fileVersionKeyC[] = "Version";
const char fileNameKeyC[] = "/debugserverproviders.xml";

static DebugServerProviderManager *m_instance = nullptr;

// DebugServerProviderManager

DebugServerProviderManager::DebugServerProviderManager()
    : m_configFile(Utils::FilePath::fromString(Core::ICore::userResourcePath() + fileNameKeyC))
    , m_factories({new JLinkGdbServerProviderFactory,
                   new OpenOcdGdbServerProviderFactory,
                   new StLinkUtilGdbServerProviderFactory,
                   new EBlinkGdbServerProviderFactory,
                   new SimulatorUvscServerProviderFactory,
                   new StLinkUvscServerProviderFactory,
                   new JLinkUvscServerProviderFactory})
{
    m_instance = this;
    m_writer = new Utils::PersistentSettingsWriter(
                m_configFile, "QtCreatorDebugServerProviders");

    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
            this, &DebugServerProviderManager::saveProviders);

    connect(this, &DebugServerProviderManager::providerAdded,
            this, &DebugServerProviderManager::providersChanged);
    connect(this, &DebugServerProviderManager::providerRemoved,
            this, &DebugServerProviderManager::providersChanged);
    connect(this, &DebugServerProviderManager::providerUpdated,
            this, &DebugServerProviderManager::providersChanged);
}

DebugServerProviderManager::~DebugServerProviderManager()
{
    qDeleteAll(m_providers);
    m_providers.clear();
    qDeleteAll(m_factories);
    delete m_writer;
    m_instance = nullptr;
}

DebugServerProviderManager *DebugServerProviderManager::instance()
{
    return m_instance;
}

void DebugServerProviderManager::restoreProviders()
{
    Utils::PersistentSettingsReader reader;
    if (!reader.load(m_configFile))
        return;

    const QVariantMap data = reader.restoreValues();
    const int version = data.value(fileVersionKeyC, 0).toInt();
    if (version < 1)
        return;

    const int count = data.value(countKeyC, 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(dataKeyC) + QString::number(i);
        if (!data.contains(key))
            break;

        const QVariantMap map = data.value(key).toMap();
        bool restored = false;
        for (IDebugServerProviderFactory *f : qAsConst(m_factories)) {
            if (f->canRestore(map)) {
                if (IDebugServerProvider *p = f->restore(map)) {
                    registerProvider(p);
                    restored = true;
                    break;
                }
            }
        }
        if (!restored)
            qWarning("Warning: Unable to restore provider '%s' stored in %s.",
                     qPrintable(IDebugServerProviderFactory::idFromMap(map)),
                     qPrintable(m_configFile.toUserOutput()));
    }

    emit providersLoaded();
}

void DebugServerProviderManager::saveProviders()
{
    QVariantMap data;
    data.insert(fileVersionKeyC, 1);

    int count = 0;
    for (const IDebugServerProvider *p : qAsConst(m_providers)) {
        if (p->isValid()) {
            const QVariantMap tmp = p->toMap();
            if (tmp.isEmpty())
                continue;
            const QString key = QString::fromLatin1(dataKeyC) + QString::number(count);
            data.insert(key, tmp);
            ++count;
        }
    }
    data.insert(countKeyC, count);
    m_writer->save(data, Core::ICore::dialogParent());
}

QList<IDebugServerProvider *> DebugServerProviderManager::providers()
{
    return m_instance->m_providers;
}

QList<IDebugServerProviderFactory *> DebugServerProviderManager::factories()
{
    return m_instance->m_factories;
}

IDebugServerProvider *DebugServerProviderManager::findProvider(const QString &id)
{
    if (id.isEmpty() || !m_instance)
        return nullptr;

    return Utils::findOrDefault(m_instance->m_providers, Utils::equal(&IDebugServerProvider::id, id));
}

IDebugServerProvider *DebugServerProviderManager::findByDisplayName(const QString &displayName)
{
    if (displayName.isEmpty())
        return nullptr;

    return Utils::findOrDefault(m_instance->m_providers,
                                Utils::equal(&IDebugServerProvider::displayName, displayName));
}

void DebugServerProviderManager::notifyAboutUpdate(IDebugServerProvider *provider)
{
    if (!provider || !m_instance->m_providers.contains(provider))
        return;
    emit m_instance->providerUpdated(provider);
}

bool DebugServerProviderManager::registerProvider(IDebugServerProvider *provider)
{
    if (!provider || m_instance->m_providers.contains(provider))
        return true;
    for (const IDebugServerProvider *current : qAsConst(m_instance->m_providers)) {
        if (*provider == *current)
            return false;
        QTC_ASSERT(current->id() != provider->id(), return false);
    }

    m_instance->m_providers.append(provider);
    emit m_instance->providerAdded(provider);
    return true;
}

void DebugServerProviderManager::deregisterProvider(IDebugServerProvider *provider)
{
    if (!provider || !m_instance->m_providers.contains(provider))
        return;
    m_instance->m_providers.removeOne(provider);
    emit m_instance->providerRemoved(provider);
    delete provider;
}

} // namespace Internal
} // namespace BareMetal
