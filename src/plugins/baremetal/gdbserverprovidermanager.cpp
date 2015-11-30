/****************************************************************************
**
** Copyright (C) 2014 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "gdbserverprovidermanager.h"
#include "gdbserverprovider.h"

#include "openocdgdbserverprovider.h"
#include "defaultgdbserverprovider.h"
#include "stlinkutilgdbserverprovider.h"

#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>
#include <utils/algorithm.h>

#include <QDir>

namespace BareMetal {
namespace Internal {

const char dataKeyC[] = "GdbServerProvider.";
const char countKeyC[] = "GdbServerProvider.Count";
const char fileVersionKeyC[] = "Version";
const char fileNameKeyC[] = "/qtcreator/gdbserverproviders.xml";

static Utils::FileName settingsFileName(const QString &path)
{
    const QFileInfo settingsLocation(Core::ICore::settings()->fileName());
    return Utils::FileName::fromString(settingsLocation.absolutePath() + path);
}

static GdbServerProviderManager *m_instance = 0;

GdbServerProviderManager::GdbServerProviderManager(QObject *parent)
    : QObject(parent)
    , m_configFile(settingsFileName(QLatin1String(fileNameKeyC)))
    , m_factories({ new DefaultGdbServerProviderFactory,
                    new OpenOcdGdbServerProviderFactory,
                    new StLinkUtilGdbServerProviderFactory })
{
    m_writer = new Utils::PersistentSettingsWriter(
                m_configFile, QLatin1String("QtCreatorGdbServerProviders"));

    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
            this, &GdbServerProviderManager::saveProviders);

    connect(this, &GdbServerProviderManager::providerAdded,
            this, &GdbServerProviderManager::providersChanged);
    connect(this, &GdbServerProviderManager::providerRemoved,
            this, &GdbServerProviderManager::providersChanged);
    connect(this, &GdbServerProviderManager::providerUpdated,
            this, &GdbServerProviderManager::providersChanged);
}

GdbServerProviderManager::~GdbServerProviderManager()
{
    qDeleteAll(m_providers);
    m_providers.clear();
    delete m_writer;
    m_instance = 0;
}

GdbServerProviderManager *GdbServerProviderManager::instance()
{
    if (!m_instance)
        m_instance = new GdbServerProviderManager;
    return m_instance;
}

void GdbServerProviderManager::restoreProviders()
{
    Utils::PersistentSettingsReader reader;
    if (!reader.load(m_configFile))
        return;

    const QVariantMap data = reader.restoreValues();
    const int version = data.value(QLatin1String(fileVersionKeyC), 0).toInt();
    if (version < 1)
        return;

    const int count = data.value(QLatin1String(countKeyC), 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(dataKeyC) + QString::number(i);
        if (!data.contains(key))
            break;

        const QVariantMap map = data.value(key).toMap();
        bool restored = false;
        foreach (GdbServerProviderFactory *f, m_factories) {
            if (f->canRestore(map)) {
                if (GdbServerProvider *p = f->restore(map)) {
                    registerProvider(p);
                    restored = true;
                    break;
                }
            }
        }
        if (!restored)
            qWarning("Warning: Unable to restore provider '%s' stored in %s.",
                     qPrintable(GdbServerProviderFactory::idFromMap(map)),
                     qPrintable(m_configFile.toUserOutput()));
    }

    emit providersLoaded();
}

void GdbServerProviderManager::saveProviders()
{
    QVariantMap data;
    data.insert(QLatin1String(fileVersionKeyC), 1);

    int count = 0;
    foreach (const GdbServerProvider *p, m_providers) {
        if (p->isValid()) {
            const QVariantMap tmp = p->toMap();
            if (tmp.isEmpty())
                continue;
            const QString key = QString::fromLatin1(dataKeyC) + QString::number(count);
            data.insert(key, tmp);
            ++count;
        }
    }
    data.insert(QLatin1String(countKeyC), count);
    m_writer->save(data, Core::ICore::mainWindow());
}

QList<GdbServerProvider *> GdbServerProviderManager::providers() const
{
    return m_providers;
}

QList<GdbServerProviderFactory *> GdbServerProviderManager::factories() const
{
    return m_factories;
}

GdbServerProvider *GdbServerProviderManager::findProvider(const QString &id) const
{
    if (id.isEmpty())
        return 0;

    return Utils::findOrDefault(m_providers, Utils::equal(&GdbServerProvider::id, id));
}

GdbServerProvider *GdbServerProviderManager::findByDisplayName(const QString &displayName) const
{
    if (displayName.isEmpty())
        return 0;

    return Utils::findOrDefault(m_providers, Utils::equal(&GdbServerProvider::displayName, displayName));
}

void GdbServerProviderManager::notifyAboutUpdate(GdbServerProvider *provider)
{
    if (!provider || !m_providers.contains(provider))
        return;
    emit providerUpdated(provider);
}

bool GdbServerProviderManager::registerProvider(GdbServerProvider *provider)
{
    if (!provider || m_providers.contains(provider))
        return true;
    foreach (const GdbServerProvider *current, m_providers) {
        if (*provider == *current)
            return false;
        QTC_ASSERT(current->id() != provider->id(), return false);
    }

    m_providers.append(provider);
    emit providerAdded(provider);
    return true;
}

void GdbServerProviderManager::deregisterProvider(GdbServerProvider *provider)
{
    if (!provider || !m_providers.contains(provider))
        return;
    m_providers.removeOne(provider);
    emit providerRemoved(provider);
    delete provider;
}

} // namespace Internal
} // namespace BareMetal
