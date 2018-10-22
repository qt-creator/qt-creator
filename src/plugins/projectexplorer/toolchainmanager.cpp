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

#include "toolchainmanager.h"

#include "abi.h"
#include "kitinformation.h"
#include "msvctoolchain.h"
#include "toolchain.h"
#include "toolchainsettingsaccessor.h"

#include <coreplugin/icore.h>

#include <utils/fileutils.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>
#include <utils/algorithm.h>

#include <QDir>
#include <QSettings>

#include <tuple>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// ToolChainManagerPrivate
// --------------------------------------------------------------------------

struct LanguageDisplayPair
{
    Core::Id id;
    QString displayName;
};

class ToolChainManagerPrivate
{
public:
    ~ToolChainManagerPrivate();

    QMap<QString, FileName> m_abiToDebugger;
    std::unique_ptr<ToolChainSettingsAccessor> m_accessor;

    QList<ToolChain *> m_toolChains; // prioritized List
    QVector<LanguageDisplayPair> m_languages;
};

ToolChainManagerPrivate::~ToolChainManagerPrivate()
{
    qDeleteAll(m_toolChains);
    m_toolChains.clear();
}

static ToolChainManager *m_instance = nullptr;
static ToolChainManagerPrivate *d = nullptr;

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

    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
            this, &ToolChainManager::saveToolChains);
    connect(this, &ToolChainManager::toolChainAdded, this, &ToolChainManager::toolChainsChanged);
    connect(this, &ToolChainManager::toolChainRemoved, this, &ToolChainManager::toolChainsChanged);
    connect(this, &ToolChainManager::toolChainUpdated, this, &ToolChainManager::toolChainsChanged);
}

ToolChainManager::~ToolChainManager()
{
    m_instance = nullptr;
    delete d;
    d = nullptr;
}

ToolChainManager *ToolChainManager::instance()
{
    return m_instance;
}

void ToolChainManager::restoreToolChains()
{
    QTC_ASSERT(!d->m_accessor, return);
    d->m_accessor = std::make_unique<Internal::ToolChainSettingsAccessor>();

    for (ToolChain *tc : d->m_accessor->restoreToolChains(Core::ICore::dialogParent()))
        registerToolChain(tc);

    emit m_instance->toolChainsLoaded();
}

void ToolChainManager::saveToolChains()
{
    QTC_ASSERT(d->m_accessor, return);

    d->m_accessor->saveToolChains(d->m_toolChains, Core::ICore::dialogParent());
}

QList<ToolChain *> ToolChainManager::toolChains(const ToolChain::Predicate &predicate)
{
    if (predicate)
        return Utils::filtered(d->m_toolChains, predicate);
    return d->m_toolChains;
}

ToolChain *ToolChainManager::toolChain(const ToolChain::Predicate &predicate)
{
    return Utils::findOrDefault(d->m_toolChains, predicate);
}

QList<ToolChain *> ToolChainManager::findToolChains(const Abi &abi)
{
    QList<ToolChain *> result;
    foreach (ToolChain *tc, d->m_toolChains) {
        bool isCompatible = Utils::anyOf(tc->supportedAbis(), [abi](const Abi &supportedAbi) {
            return supportedAbi.isCompatibleWith(abi);
        });

        if (isCompatible)
            result.append(tc);
    }
    return result;
}

ToolChain *ToolChainManager::findToolChain(const QByteArray &id)
{
    if (id.isEmpty())
        return nullptr;

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
    return bool(d->m_accessor);
}

void ToolChainManager::notifyAboutUpdate(ToolChain *tc)
{
    if (!tc || !d->m_toolChains.contains(tc))
        return;
    emit m_instance->toolChainUpdated(tc);
}

bool ToolChainManager::registerToolChain(ToolChain *tc)
{
    QTC_ASSERT(tc, return false);
    QTC_ASSERT(isLanguageSupported(tc->language()), return false);
    QTC_ASSERT(d->m_accessor, return false);

    if (d->m_toolChains.contains(tc))
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

QSet<Core::Id> ToolChainManager::allLanguages()
{
    return Utils::transform<QSet>(d->m_languages, &LanguageDisplayPair::id);
}

bool ToolChainManager::registerLanguage(const Core::Id &language, const QString &displayName)
{
    QTC_ASSERT(language.isValid(), return false);
    QTC_ASSERT(!isLanguageSupported(language), return false);
    QTC_ASSERT(!displayName.isEmpty(), return false);
    d->m_languages.push_back({language, displayName});
    return true;
}

QString ToolChainManager::displayNameOfLanguageId(const Core::Id &id)
{
    QTC_ASSERT(id.isValid(), return tr("None"));
    auto entry = Utils::findOrDefault(d->m_languages, Utils::equal(&LanguageDisplayPair::id, id));
    QTC_ASSERT(entry.id.isValid(), return tr("None"));
    return entry.displayName;
}

bool ToolChainManager::isLanguageSupported(const Core::Id &id)
{
    return Utils::contains(d->m_languages, Utils::equal(&LanguageDisplayPair::id, id));
}

void ToolChainManager::aboutToShutdown()
{
#ifdef Q_OS_WIN
    MsvcToolChain::cancelMsvcToolChainDetection();
#endif
}

} // namespace ProjectExplorer
