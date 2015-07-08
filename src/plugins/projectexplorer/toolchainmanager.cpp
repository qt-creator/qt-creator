/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "toolchainmanager.h"

#include "abi.h"
#include "kitinformation.h"
#include "toolchain.h"

#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/fileutils.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>
#include <utils/algorithm.h>

#include <QDir>
#include <QSettings>

static const char TOOLCHAIN_DATA_KEY[] = "ToolChain.";
static const char TOOLCHAIN_COUNT_KEY[] = "ToolChain.Count";
static const char TOOLCHAIN_FILE_VERSION_KEY[] = "Version";
static const char TOOLCHAIN_FILENAME[] = "/qtcreator/toolchains.xml";
static const char LEGACY_TOOLCHAIN_FILENAME[] = "/toolChains.xml";

using namespace Utils;

static FileName settingsFileName(const QString &path)
{
    QFileInfo settingsLocation(Core::ICore::settings()->fileName());
    return FileName::fromString(settingsLocation.absolutePath() + path);
}

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// ToolChainManagerPrivate
// --------------------------------------------------------------------------

class ToolChainManagerPrivate
{
public:
    ToolChainManagerPrivate() : m_writer(0) {}
    ~ToolChainManagerPrivate();

    QMap<QString, FileName> m_abiToDebugger;
    PersistentSettingsWriter *m_writer;

    QList<ToolChain *> m_toolChains; // prioritized List
};

ToolChainManagerPrivate::~ToolChainManagerPrivate()
{
    qDeleteAll(m_toolChains);
    m_toolChains.clear();
    delete m_writer;
}

static ToolChainManager *m_instance = 0;
static ToolChainManagerPrivate *d;

} // namespace Internal

using namespace Internal;

// --------------------------------------------------------------------------
// ToolChainManager
// --------------------------------------------------------------------------

ToolChainManager::ToolChainManager(QObject *parent) :
    QObject(parent)
{
    Q_ASSERT(!m_instance);
    m_instance = this;

    d = new ToolChainManagerPrivate;

    connect(Core::ICore::instance(), SIGNAL(saveSettingsRequested()),
            this, SLOT(saveToolChains()));
    connect(this, SIGNAL(toolChainAdded(ProjectExplorer::ToolChain*)),
            this, SIGNAL(toolChainsChanged()));
    connect(this, SIGNAL(toolChainRemoved(ProjectExplorer::ToolChain*)),
            this, SIGNAL(toolChainsChanged()));
    connect(this, SIGNAL(toolChainUpdated(ProjectExplorer::ToolChain*)),
            this, SIGNAL(toolChainsChanged()));
}

ToolChainManager::~ToolChainManager()
{
    delete d;
    m_instance = 0;
}

ToolChainManager *ToolChainManager::instance()
{
    return m_instance;
}

static QList<ToolChain *> restoreFromFile(const FileName &fileName)
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

    QList<ToolChainFactory *> factories = ExtensionSystem::PluginManager::getObjects<ToolChainFactory>();

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
                    result.append(tc);
                    restored = true;
                    break;
                }
            }
        }
        if (!restored)
            qWarning("Warning: '%s': Unable to restore compiler type '%s' for tool chain %s.",
                     qPrintable(fileName.toUserOutput()),
                     qPrintable(ToolChainFactory::typeIdFromMap(tcMap).toString()),
                     qPrintable(QString::fromUtf8(ToolChainFactory::idFromMap(tcMap))));
    }

    return result;
}

void ToolChainManager::restoreToolChains()
{
    QTC_ASSERT(!d->m_writer, return);
    d->m_writer =
            new PersistentSettingsWriter(settingsFileName(QLatin1String(TOOLCHAIN_FILENAME)), QLatin1String("QtCreatorToolChains"));

    QList<ToolChain *> tcsToRegister;
    QList<ToolChain *> tcsToCheck;

    // read all tool chains from SDK
    QFileInfo systemSettingsFile(Core::ICore::settings(QSettings::SystemScope)->fileName());
    QList<ToolChain *> readTcs =
            restoreFromFile(FileName::fromString(systemSettingsFile.absolutePath() + QLatin1String(TOOLCHAIN_FILENAME)));
    // make sure we mark these as autodetected!
    foreach (ToolChain *tc, readTcs)
        tc->setDetection(ToolChain::AutoDetection);

    tcsToRegister = readTcs; // SDK TCs are always considered to be up-to-date, so no need to
                             // recheck them.

    // read all tool chains from user file.
    // Read legacy settings once and keep them around...
    FileName fileName = settingsFileName(QLatin1String(TOOLCHAIN_FILENAME));
    if (!fileName.exists())
        fileName = settingsFileName(QLatin1String(LEGACY_TOOLCHAIN_FILENAME));
    readTcs = restoreFromFile(fileName);

    foreach (ToolChain *tc, readTcs) {
        if (tc->isAutoDetected())
            tcsToCheck.append(tc);
        else
            tcsToRegister.append(tc);
    }
    readTcs.clear();

    // Remove TCs configured by the SDK:
    foreach (ToolChain *tc, tcsToRegister) {
        for (int i = tcsToCheck.count() - 1; i >= 0; --i) {
            if (tcsToCheck.at(i)->id() == tc->id()) {
                delete tcsToCheck.at(i);
                tcsToCheck.removeAt(i);
            }
        }
    }

    // Then auto detect
    QList<ToolChain *> detectedTcs;
    QList<ToolChainFactory *> factories = ExtensionSystem::PluginManager::getObjects<ToolChainFactory>();
    foreach (ToolChainFactory *f, factories)
        detectedTcs.append(f->autoDetect());

    // Find/update autodetected tool chains:
    ToolChain *toStore = 0;
    foreach (ToolChain *currentDetected, detectedTcs) {
        toStore = currentDetected;

        // Check whether we had this TC stored and prefer the old one with the old id, marked
        // as auto-detection.
        for (int i = 0; i < tcsToCheck.count(); ++i) {
            if (*(tcsToCheck.at(i)) == *currentDetected) {
                toStore = tcsToCheck.at(i);
                toStore->setDetection(ToolChain::AutoDetection);
                tcsToCheck.removeAt(i);
                delete currentDetected;
                break;
            }
        }
        tcsToRegister += toStore;
    }

    // Keep toolchains that were not rediscovered but are still executable and delete the rest
    foreach (ToolChain *tc, tcsToCheck) {
        if (!tc->isValid()) {
            qWarning() << QString::fromLatin1("ToolChain \"%1\" (%2) dropped since it is not valid")
                          .arg(tc->displayName()).arg(QString::fromUtf8(tc->id()));
            delete tc;
        } else {
            tc->setDetection(ToolChain::ManualDetection); // "demote" to manual toolchain
            tcsToRegister += tc;
        }
    }

    // Store manual tool chains
    foreach (ToolChain *tc, tcsToRegister)
        registerToolChain(tc);

    emit m_instance->toolChainsLoaded();
}

void ToolChainManager::saveToolChains()
{
    QVariantMap data;
    data.insert(QLatin1String(TOOLCHAIN_FILE_VERSION_KEY), 1);

    int count = 0;
    foreach (ToolChain *tc, d->m_toolChains) {
        if (tc->isValid()) {
            QVariantMap tmp = tc->toMap();
            if (tmp.isEmpty())
                continue;
            data.insert(QString::fromLatin1(TOOLCHAIN_DATA_KEY) + QString::number(count), tmp);
            ++count;
        }
    }
    data.insert(QLatin1String(TOOLCHAIN_COUNT_KEY), count);
    d->m_writer->save(data, Core::ICore::mainWindow());

    // Do not save default debuggers! Those are set by the SDK!
}

QList<ToolChain *> ToolChainManager::toolChains()
{
    return d->m_toolChains;
}

QList<ToolChain *> ToolChainManager::findToolChains(const Abi &abi)
{
    QList<ToolChain *> result;
    foreach (ToolChain *tc, d->m_toolChains) {
        Abi targetAbi = tc->targetAbi();
        if (targetAbi.isCompatibleWith(abi))
            result.append(tc);
    }
    return result;
}

ToolChain *ToolChainManager::findToolChain(const QByteArray &id)
{
    if (id.isEmpty())
        return 0;

    ToolChain *tc = Utils::findOrDefault(d->m_toolChains, Utils::equal(&ToolChain::id, id));

    // Compatibility with versions 3.5 and earlier:
    if (!tc) {
        const int pos = id.indexOf(':');
        if (pos < 0)
            return tc;

        const QByteArray shortId = id.mid(pos + 1);

        tc = Utils::findOrDefault(d->m_toolChains, Utils::equal(&ToolChain::id, shortId));
    }
    return tc;
}

FileName ToolChainManager::defaultDebugger(const Abi &abi)
{
    return d->m_abiToDebugger.value(abi.toString());
}

bool ToolChainManager::isLoaded()
{
    return d->m_writer;
}

void ToolChainManager::notifyAboutUpdate(ToolChain *tc)
{
    if (!tc || !d->m_toolChains.contains(tc))
        return;
    emit m_instance->toolChainUpdated(tc);
}

bool ToolChainManager::registerToolChain(ToolChain *tc)
{
    QTC_ASSERT(d->m_writer, return false);

    if (!tc || d->m_toolChains.contains(tc))
        return true;
    foreach (ToolChain *current, d->m_toolChains) {
        if (*tc == *current && !tc->isAutoDetected())
            return false;
        QTC_ASSERT(current->id() != tc->id(), return false);
    }

    d->m_toolChains.append(tc);
    emit m_instance->toolChainAdded(tc);
    return true;
}

void ToolChainManager::deregisterToolChain(ToolChain *tc)
{
    if (!tc || !d->m_toolChains.contains(tc))
        return;
    d->m_toolChains.removeOne(tc);
    emit m_instance->toolChainRemoved(tc);
    delete tc;
}

} // namespace ProjectExplorer
