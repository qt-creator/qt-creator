/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "toolchainmanager.h"

#include "abi.h"
#include "profileinformation.h"
#include "toolchain.h"

#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/persistentsettings.h>

#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QMainWindow>

static const char TOOLCHAIN_DATA_KEY[] = "ToolChain.";
static const char TOOLCHAIN_COUNT_KEY[] = "ToolChain.Count";
static const char TOOLCHAIN_FILE_VERSION_KEY[] = "Version";
static const char DEFAULT_DEBUGGER_COUNT_KEY[] = "DefaultDebugger.Count";
static const char DEFAULT_DEBUGGER_ABI_KEY[] = "DefaultDebugger.Abi.";
static const char DEFAULT_DEBUGGER_PATH_KEY[] = "DefaultDebugger.Path.";
static const char TOOLCHAIN_FILENAME[] = "/qtcreator/toolchains.xml";
static const char LEGACY_TOOLCHAIN_FILENAME[] = "/toolChains.xml";

using Utils::PersistentSettingsWriter;
using Utils::PersistentSettingsReader;

static QString settingsFileName(const QString &path)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    QFileInfo settingsLocation(pm->settings()->fileName());
    return settingsLocation.absolutePath() + path;
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
    ToolChainManagerPrivate(ToolChainManager *parent);

    QList<ToolChain *> &toolChains();

    ToolChainManager *q;
    bool m_initialized;
    QMap<QString, Utils::FileName> m_abiToDebugger;

private:
    QList<ToolChain *> m_toolChains;
};

ToolChainManagerPrivate::ToolChainManagerPrivate(ToolChainManager *parent)
    : q(parent), m_initialized(false)
{ }

QList<ToolChain *> &ToolChainManagerPrivate::toolChains()
{
    if (!m_initialized) {
        m_initialized = true;
        q->restoreToolChains();
    }
    return m_toolChains;
}

} // namespace Internal

// --------------------------------------------------------------------------
// ToolChainManager
// --------------------------------------------------------------------------

ToolChainManager *ToolChainManager::instance()
{
    return m_instance;
}

ToolChainManager::ToolChainManager(QObject *parent) :
    QObject(parent),
    d(new Internal::ToolChainManagerPrivate(this))
{
    Q_ASSERT(!m_instance);
    m_instance = this;

    connect(Core::ICore::instance(), SIGNAL(saveSettingsRequested()),
            this, SLOT(saveToolChains()));
    connect(this, SIGNAL(toolChainAdded(ProjectExplorer::ToolChain*)),
            this, SIGNAL(toolChainsChanged()));
    connect(this, SIGNAL(toolChainRemoved(ProjectExplorer::ToolChain*)),
            this, SIGNAL(toolChainsChanged()));
    connect(this, SIGNAL(toolChainUpdated(ProjectExplorer::ToolChain*)),
            this, SIGNAL(toolChainsChanged()));
}

void ToolChainManager::restoreToolChains()
{
    QList<ToolChain *> tcsToRegister;
    QList<ToolChain *> tcsToCheck;

    // read all tool chains from SDK
    QFileInfo systemSettingsFile(Core::ICore::settings(QSettings::SystemScope)->fileName());
    QList<ToolChain *> readTcs =
            restoreToolChains(systemSettingsFile.absolutePath() + QLatin1String(LEGACY_TOOLCHAIN_FILENAME));
    // make sure we mark these as autodetected!
    foreach (ToolChain *tc, readTcs)
        tc->setAutoDetected(true);

    tcsToRegister = readTcs; // SDK TCs are always considered to be up-to-date, so no need to
                             // recheck them.

    // read all tool chains from user file.
    // Read legacy settings once and keep them around...
    QString fileName = settingsFileName(QLatin1String(TOOLCHAIN_FILENAME));
    if (!QFileInfo(fileName).exists())
        fileName = settingsFileName(QLatin1String(LEGACY_TOOLCHAIN_FILENAME));
    readTcs = restoreToolChains(fileName);

    foreach (ToolChain *tc, readTcs) {
        if (tc->isAutoDetected())
            tcsToCheck.append(tc);
        else
            tcsToRegister.append(tc);
    }
    readTcs.clear();

    // Then auto detect
    QList<ToolChain *> detectedTcs;
    QList<ToolChainFactory *> factories = ExtensionSystem::PluginManager::getObjects<ToolChainFactory>();
    foreach (ToolChainFactory *f, factories)
        detectedTcs.append(f->autoDetect());

    // Find/update autodetected tool chains:
    ToolChain *toStore = 0;
    foreach (ToolChain *currentDetected, detectedTcs) {
        toStore = currentDetected;

        // Check whether we had this TC stored and prefer the old one with the old id:
        for (int i = 0; i < tcsToCheck.count(); ++i) {
            if (*(tcsToCheck.at(i)) == *currentDetected) {
                toStore = tcsToCheck.at(i);
                tcsToCheck.removeAt(i);
                delete currentDetected;
                break;
            }
        }
        registerToolChain(toStore);
    }

    // Delete all loaded autodetected tool chains that were not rediscovered:
    qDeleteAll(tcsToCheck);

    // Store manual tool chains
    foreach (ToolChain *tc, tcsToRegister)
        registerToolChain(tc);
}

ToolChainManager::~ToolChainManager()
{
    // Deregister tool chains
    QList<ToolChain *> copy = d->toolChains();
    foreach (ToolChain *tc, copy)
        deregisterToolChain(tc);

    delete d;
    m_instance = 0;
}

void ToolChainManager::saveToolChains()
{
    PersistentSettingsWriter writer;
    writer.saveValue(QLatin1String(TOOLCHAIN_FILE_VERSION_KEY), 1);

    int count = 0;
    foreach (ToolChain *tc, d->toolChains()) {
        if (tc->isValid()) {
            QVariantMap tmp = tc->toMap();
            if (tmp.isEmpty())
                continue;
            writer.saveValue(QString::fromLatin1(TOOLCHAIN_DATA_KEY) + QString::number(count), tmp);
            ++count;
        }
    }
    writer.saveValue(QLatin1String(TOOLCHAIN_COUNT_KEY), count);
    writer.save(settingsFileName(QLatin1String(TOOLCHAIN_FILENAME)), QLatin1String("QtCreatorToolChains"), Core::ICore::mainWindow());

    // Do not save default debuggers! Those are set by the SDK!
}

QList<ToolChain *> ToolChainManager::restoreToolChains(const QString &fileName)
{
    QList<ToolChain *> result;

    PersistentSettingsReader reader;
    if (!reader.load(fileName))
        return result;
    QVariantMap data = reader.restoreValues();

    // Check version:
    int version = data.value(QLatin1String(TOOLCHAIN_FILE_VERSION_KEY), 0).toInt();
    if (version < 1)
        return result;

    // Read default debugger settings (if any)
    int count = data.value(QLatin1String(DEFAULT_DEBUGGER_COUNT_KEY)).toInt();
    for (int i = 0; i < count; ++i) {
        const QString abiKey = QString::fromLatin1(DEFAULT_DEBUGGER_ABI_KEY) + QString::number(i);
        if (!data.contains(abiKey))
            continue;
        const QString pathKey = QString::fromLatin1(DEFAULT_DEBUGGER_PATH_KEY) + QString::number(i);
        if (!data.contains(pathKey))
            continue;
        d->m_abiToDebugger.insert(data.value(abiKey).toString(),
                                  Utils::FileName::fromString(data.value(pathKey).toString()));
    }

    QList<ToolChainFactory *> factories = ExtensionSystem::PluginManager::getObjects<ToolChainFactory>();

    count = data.value(QLatin1String(TOOLCHAIN_COUNT_KEY), 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(TOOLCHAIN_DATA_KEY) + QString::number(i);
        if (!data.contains(key))
            break;

        const QVariantMap tcMap = data.value(key).toMap();

        bool restored = false;
        foreach (ToolChainFactory *f, factories) {
            if (f->canRestore(tcMap)) {
                if (ToolChain *tc = f->restore(tcMap)) {
                    result.append(tc);
                    restored = true;
                    break;
                }
            }
        }
        if (!restored)
            qWarning("Warning: Unable to restore tool chain '%s' stored in %s.",
                     qPrintable(ToolChainFactory::idFromMap(tcMap)),
                     qPrintable(QDir::toNativeSeparators(fileName)));
    }
    return result;
}

QList<ToolChain *> ToolChainManager::toolChains() const
{
    return d->toolChains();
}

QList<ToolChain *> ToolChainManager::findToolChains(const Abi &abi) const
{
    QList<ToolChain *> result;
    foreach (ToolChain *tc, toolChains()) {
        Abi targetAbi = tc->targetAbi();
        if (targetAbi.isCompatibleWith(abi))
            result.append(tc);
    }
    return result;
}

ToolChain *ToolChainManager::findToolChain(const QString &id) const
{
    if (id.isEmpty())
        return 0;

    foreach (ToolChain *tc, d->toolChains()) {
        if (tc->id() == id)
            return tc;
    }
    return 0;
}

Utils::FileName ToolChainManager::defaultDebugger(const Abi &abi) const
{
    return d->m_abiToDebugger.value(abi.toString());
}

void ToolChainManager::notifyAboutUpdate(ProjectExplorer::ToolChain *tc)
{
    if (!tc || !toolChains().contains(tc))
        return;
    emit toolChainUpdated(tc);
}

bool ToolChainManager::registerToolChain(ToolChain *tc)
{
    if (!tc || d->toolChains().contains(tc))
        return true;
    foreach (ToolChain *current, d->toolChains()) {
        if (*tc == *current)
            return false;
    }

    d->toolChains().append(tc);
    emit toolChainAdded(tc);
    return true;
}

void ToolChainManager::deregisterToolChain(ToolChain *tc)
{
    if (!tc || !d->toolChains().contains(tc))
        return;
    d->toolChains().removeOne(tc);
    emit toolChainRemoved(tc);
    delete tc;
}

} // namespace ProjectExplorer
