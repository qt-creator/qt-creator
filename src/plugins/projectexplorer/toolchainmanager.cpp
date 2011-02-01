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

#include <extensionsystem/pluginmanager.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QSettings>


static const char *const ORGANIZATION_NAME = "Nokia";
static const char *const APPLICATION_NAME = "toolChains";
static const char *const ARRAY_NAME = "ToolChain";
static const char *const TOOLCHAIN_DATA_KEY = "Data";

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
}

void ToolChainManager::restoreToolChains()
{
    QList<ToolChainFactory *> factories =
            ExtensionSystem::PluginManager::instance()->getObjects<ToolChainFactory>();
    // Autodetect ToolChains:
    foreach (ToolChainFactory *f, factories) {
        QList<ToolChain *> tcs = f->autoDetect();
        foreach (ToolChain *tc, tcs)
            registerToolChain(tc);
    }

    // Restore user generated ToolChains:
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       ORGANIZATION_NAME, APPLICATION_NAME);
    int size = settings.beginReadArray(QLatin1String(ARRAY_NAME));
    if (size <= 0)
        return;

    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        QVariantMap tmp = settings.value(QLatin1String(TOOLCHAIN_DATA_KEY)).toMap();
        foreach (ToolChainFactory *f, factories) {
            if (!f->canRestore(tmp))
                continue;
            ToolChain *tc = f->restore(tmp);
            if (!tc)
                continue;
            registerToolChain(tc);
        }
    }
}

ToolChainManager::~ToolChainManager()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       ORGANIZATION_NAME, APPLICATION_NAME);
    settings.beginWriteArray(QLatin1String(ARRAY_NAME));
    int count = 0;
    foreach (ToolChain *tc, m_d->m_toolChains) {
        if (!tc->isAutoDetected() && tc->isValid()) {
            settings.setArrayIndex(count);
            ++count;

            QVariantMap tmp = tc->toMap();
            if (tmp.isEmpty())
                continue;
            settings.setValue(QLatin1String(TOOLCHAIN_DATA_KEY), tmp);
        }
    }
    settings.endArray();

    QList<ToolChain *> copy = m_d->m_toolChains;
    foreach (ToolChain *tc, copy)
        deregisterToolChain(tc);

    delete m_d;
    m_instance = 0;
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
