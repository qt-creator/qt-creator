/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "toolchainmanager.h"

#include "toolchain.h"

#include <coreplugin/icore.h>
#include <projectexplorer/persistentsettings.h>

#include <extensionsystem/pluginmanager.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QSettings>

static const char *const TOOLCHAIN_DATA_KEY = "ToolChain.";
static const char *const TOOLCHAIN_COUNT_KEY = "ToolChain.Count";
static const char *const TOOLCHAIN_FILE_VERSION_KEY = "Version";
static const char *const TOOLCHAIN_FILENAME = "/toolChains.xml";

static QString settingsFileName()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    QFileInfo settingsLocation(pm->settings()->fileName());
    return settingsLocation.absolutePath() + QLatin1String(TOOLCHAIN_FILENAME);
}

namespace ProjectExplorer {

ToolChainManager *ToolChainManager::m_instance = 0;

namespace Internal {

// --------------------------------------------------------------------------
// ToolChainManagerPrivate
// --------------------------------------------------------------------------

class ToolChainManagerPrivate
{
public:
    QList<ToolChain *> m_toolChains;
};

} // namespace Internal

// --------------------------------------------------------------------------
// ToolChainManager
// --------------------------------------------------------------------------

ToolChainManager *ToolChainManager::instance()
{
    Q_ASSERT(m_instance);
    return m_instance;
}

ToolChainManager::ToolChainManager(QObject *parent) :
    QObject(parent),
    m_d(new Internal::ToolChainManagerPrivate)
{
    Q_ASSERT(!m_instance);
    m_instance = this;
    connect(Core::ICore::instance(), SIGNAL(saveSettingsRequested()),
            this, SLOT(saveToolChains()));
}

void ToolChainManager::restoreToolChains()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    QList<ToolChainFactory *> factories = pm->getObjects<ToolChainFactory>();
    // Autodetect tool chains:
    foreach (ToolChainFactory *f, factories) {
        QList<ToolChain *> tcs = f->autoDetect();
        foreach (ToolChain *tc, tcs)
            registerToolChain(tc);
    }

    restoreToolChains(settingsFileName(), false);
    restoreToolChains(Core::ICore::instance()->resourcePath()
                      + QLatin1String("/Nokia") + QLatin1String(TOOLCHAIN_FILENAME), true);
}

ToolChainManager::~ToolChainManager()
{
    // Deregister tool chains
    QList<ToolChain *> copy = m_d->m_toolChains;
    foreach (ToolChain *tc, copy)
        deregisterToolChain(tc);

    delete m_d;
    m_instance = 0;
}

void ToolChainManager::saveToolChains()
{
    PersistentSettingsWriter writer;
    writer.saveValue(QLatin1String(TOOLCHAIN_FILE_VERSION_KEY), 1);

    int count = 0;
    foreach (ToolChain *tc, m_d->m_toolChains) {
        if (!tc->isAutoDetected() && tc->isValid()) {
            QVariantMap tmp = tc->toMap();
            if (tmp.isEmpty())
                continue;
            writer.saveValue(QString::fromLatin1(TOOLCHAIN_DATA_KEY) + QString::number(count), tmp);
            ++count;
        }
    }
    writer.saveValue(QLatin1String(TOOLCHAIN_COUNT_KEY), count);
    writer.save(settingsFileName(), "QtCreatorToolChains");
}

void ToolChainManager::restoreToolChains(const QString &fileName, bool autoDetected)
{
    PersistentSettingsReader reader;
    if (!reader.load(fileName))
        return;
    QVariantMap data = reader.restoreValues();

    // Check version:
    int version = data.value(QLatin1String(TOOLCHAIN_FILE_VERSION_KEY), 0).toInt();
    if (version < 1)
        return;

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    QList<ToolChainFactory *> factories = pm->getObjects<ToolChainFactory>();

    int count = data.value(QLatin1String(TOOLCHAIN_COUNT_KEY), 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(TOOLCHAIN_DATA_KEY) + QString::number(i);
        if (!data.contains(key))
            break;

        const QVariantMap tcMap = data.value(key).toMap();

        bool restored = false;
        foreach (ToolChainFactory *f, factories) {
            if (f->canRestore(tcMap)) {
                if (ToolChain *tc = f->restore(tcMap)) {
                    tc->setAutoDetected(autoDetected);

                    registerToolChain(tc);
                    restored = true;
                    break;
                }
            }
        }
        if (!restored)
            qWarning("Warning: Unable to restore manual tool chain '%s' stored in %s.",
                     qPrintable(ToolChainFactory::idFromMap(tcMap)),
                     qPrintable(QDir::toNativeSeparators(fileName)));
    }
}

QList<ToolChain *> ToolChainManager::toolChains() const
{
    return m_d->m_toolChains;
}

QList<ToolChain *> ToolChainManager::findToolChains(const Abi &abi) const
{
    QList<ToolChain *> result;
    foreach (ToolChain *tc, m_d->m_toolChains) {
        Abi targetAbi = tc->targetAbi();
        if (targetAbi.isCompatibleWith(abi))
            result.append(tc);
    }
    return result;
}

ToolChain *ToolChainManager::findToolChain(const QString &id) const
{
    foreach (ToolChain *tc, m_d->m_toolChains) {
        if (tc->id() == id)
            return tc;
    }
    return 0;
}

void ToolChainManager::registerToolChain(ToolChain *tc)
{
    if (!tc || m_d->m_toolChains.contains(tc))
        return;
    foreach (ToolChain *current, m_d->m_toolChains) {
        if (*tc == *current)
            return;
    }

    m_d->m_toolChains.append(tc);
    emit toolChainAdded(tc);
}

void ToolChainManager::deregisterToolChain(ToolChain *tc)
{
    if (!tc || !m_d->m_toolChains.contains(tc))
        return;
    m_d->m_toolChains.removeOne(tc);
    emit toolChainRemoved(tc);
    delete tc;
}

} // namespace ProjectExplorer
