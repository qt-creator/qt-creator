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

#include <QSettings>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// ToolChainManagerPrivate
// --------------------------------------------------------------------------

struct LanguageDisplayPair
{
    Utils::Id id;
    QString displayName;
};

class ToolChainManagerPrivate
{
public:
    ~ToolChainManagerPrivate();

    std::unique_ptr<ToolChainSettingsAccessor> m_accessor;

    Toolchains m_toolChains; // prioritized List
    BadToolchains m_badToolchains;   // to be skipped when auto-detecting
    QVector<LanguageDisplayPair> m_languages;
    ToolchainDetectionSettings m_detectionSettings;
    bool m_loaded = false;
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

const char DETECT_X64_AS_X32_KEY[] = "ProjectExplorer/Toolchains/DetectX64AsX32";

static QString badToolchainsKey() { return {"BadToolChains"}; }

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

    QSettings * const s = Core::ICore::settings();
    d->m_detectionSettings.detectX64AsX32
        = s->value(DETECT_X64_AS_X32_KEY, ToolchainDetectionSettings().detectX64AsX32).toBool();
    d->m_badToolchains = BadToolchains::fromVariant(s->value(badToolchainsKey()));
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

    d->m_loaded = true;
    emit m_instance->toolChainsLoaded();
}

void ToolChainManager::saveToolChains()
{
    QTC_ASSERT(d->m_accessor, return);

    d->m_accessor->saveToolChains(d->m_toolChains, Core::ICore::dialogParent());
    QtcSettings *const s = Core::ICore::settings();
    s->setValueWithDefault(DETECT_X64_AS_X32_KEY,
                           d->m_detectionSettings.detectX64AsX32,
                           ToolchainDetectionSettings().detectX64AsX32);
    s->setValue(badToolchainsKey(), d->m_badToolchains.toVariant());
}

const Toolchains &ToolChainManager::toolchains()
{
    return d->m_toolChains;
}

Toolchains ToolChainManager::toolchains(const ToolChain::Predicate &predicate)
{
    QTC_ASSERT(predicate, return {});
    return Utils::filtered(d->m_toolChains, predicate);
}

ToolChain *ToolChainManager::toolChain(const ToolChain::Predicate &predicate)
{
    return Utils::findOrDefault(d->m_toolChains, predicate);
}

Toolchains ToolChainManager::findToolChains(const Abi &abi)
{
    Toolchains result;
    for (ToolChain *tc : qAsConst(d->m_toolChains)) {
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

bool ToolChainManager::isLoaded()
{
    return d->m_loaded;
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
    QTC_ASSERT(isLanguageSupported(tc->language()),
               qDebug() << qPrintable("language \"" + tc->language().toString()
                                      + "\" unknown while registering \""
                                      + tc->compilerCommand().toString() + "\"");
               return false);
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

QList<Id> ToolChainManager::allLanguages()
{
    return Utils::transform<QList>(d->m_languages, &LanguageDisplayPair::id);
}

bool ToolChainManager::registerLanguage(const Utils::Id &language, const QString &displayName)
{
    QTC_ASSERT(language.isValid(), return false);
    QTC_ASSERT(!isLanguageSupported(language), return false);
    QTC_ASSERT(!displayName.isEmpty(), return false);
    d->m_languages.push_back({language, displayName});
    return true;
}

QString ToolChainManager::displayNameOfLanguageId(const Utils::Id &id)
{
    QTC_ASSERT(id.isValid(), return tr("None"));
    auto entry = Utils::findOrDefault(d->m_languages, Utils::equal(&LanguageDisplayPair::id, id));
    QTC_ASSERT(entry.id.isValid(), return tr("None"));
    return entry.displayName;
}

bool ToolChainManager::isLanguageSupported(const Utils::Id &id)
{
    return Utils::contains(d->m_languages, Utils::equal(&LanguageDisplayPair::id, id));
}

void ToolChainManager::aboutToShutdown()
{
    if (HostOsInfo::isWindowsHost())
        MsvcToolChain::cancelMsvcToolChainDetection();
}

ToolchainDetectionSettings ToolChainManager::detectionSettings()
{
    return d->m_detectionSettings;
}

void ToolChainManager::setDetectionSettings(const ToolchainDetectionSettings &settings)
{
    d->m_detectionSettings = settings;
}

void ToolChainManager::resetBadToolchains()
{
    d->m_badToolchains.toolchains.clear();
}

bool ToolChainManager::isBadToolchain(const Utils::FilePath &toolchain)
{
    return d->m_badToolchains.isBadToolchain(toolchain);
}

void ToolChainManager::addBadToolchain(const Utils::FilePath &toolchain)
{
    d->m_badToolchains.toolchains << toolchain;
}

} // namespace ProjectExplorer
