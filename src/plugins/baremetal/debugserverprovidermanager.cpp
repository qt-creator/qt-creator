// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debugserverprovidermanager.h"
#include "idebugserverprovider.h"

// GDB debug servers.
#include "debugservers/gdb/genericgdbserverprovider.h"
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

using namespace Utils;

namespace BareMetal::Internal {

const char dataKeyC[] = "DebugServerProvider.";
const char countKeyC[] = "DebugServerProvider.Count";
const char fileVersionKeyC[] = "Version";
const char fileNameKeyC[] = "debugserverproviders.xml";

static DebugServerProviderManager *m_instance = nullptr;

// DebugServerProviderManager

DebugServerProviderManager::DebugServerProviderManager()
    : m_configFile(Core::ICore::userResourcePath(fileNameKeyC))
    , m_factories({new GenericGdbServerProviderFactory,
                   new JLinkGdbServerProviderFactory,
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
    PersistentSettingsReader reader;
    if (!reader.load(m_configFile))
        return;

    const Store data = reader.restoreValues();
    const int version = data.value(fileVersionKeyC, 0).toInt();
    if (version < 1)
        return;

    const int count = data.value(countKeyC, 0).toInt();
    for (int i = 0; i < count; ++i) {
        const Key key = numberedKey(dataKeyC, i);
        if (!data.contains(key))
            break;

        Store map = storeFromVariant(data.value(key));
        const KeyList keys = map.keys();
        for (const Key &key : keys) {
            const int lastDot = key.view().lastIndexOf('.');
            if (lastDot != -1)
                map[key.toByteArray().mid(lastDot + 1)] = map[key];
        }
        bool restored = false;
        for (IDebugServerProviderFactory *f : std::as_const(m_factories)) {
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
    Store data;
    data.insert(fileVersionKeyC, 1);

    int count = 0;
    for (const IDebugServerProvider *p : std::as_const(m_providers)) {
        if (p->isValid()) {
            Store tmp;
            p->toMap(tmp);
            if (tmp.isEmpty())
                continue;
            const Key key = numberedKey(dataKeyC, count);
            data.insert(key, variantFromStore(tmp));
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
    for (const IDebugServerProvider *current : std::as_const(m_instance->m_providers)) {
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

} // BareMetal::Internal
