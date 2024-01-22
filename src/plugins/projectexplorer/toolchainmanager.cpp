// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolchainmanager.h"

#include "abi.h"
#include "msvctoolchain.h"
#include "projectexplorertr.h"
#include "toolchain.h"
#include "toolchainsettingsaccessor.h"

#include <coreplugin/icore.h>

#include <utils/fileutils.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>
#include <utils/algorithm.h>

#include <nanotrace/nanotrace.h>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// ToolchainManagerPrivate
// --------------------------------------------------------------------------

struct LanguageDisplayPair
{
    Utils::Id id;
    QString displayName;
};

class ToolchainManagerPrivate
{
public:
    ~ToolchainManagerPrivate();

    std::unique_ptr<ToolchainSettingsAccessor> m_accessor;

    Toolchains m_toolChains; // prioritized List
    BadToolchains m_badToolchains;   // to be skipped when auto-detecting
    QVector<LanguageDisplayPair> m_languages;
    ToolchainDetectionSettings m_detectionSettings;
    bool m_loaded = false;
};

ToolchainManagerPrivate::~ToolchainManagerPrivate()
{
    qDeleteAll(m_toolChains);
    m_toolChains.clear();
}

static ToolchainManager *m_instance = nullptr;
static ToolchainManagerPrivate *d = nullptr;

} // namespace Internal

using namespace Internal;

const char DETECT_X64_AS_X32_KEY[] = "ProjectExplorer/Toolchains/DetectX64AsX32";

static Key badToolchainsKey() { return "BadToolChains"; }

// --------------------------------------------------------------------------
// ToolchainManager
// --------------------------------------------------------------------------

ToolchainManager::ToolchainManager(QObject *parent) :
    QObject(parent)
{
    Q_ASSERT(!m_instance);
    m_instance = this;

    d = new ToolchainManagerPrivate;

    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
            this, &ToolchainManager::saveToolchains);
    connect(this, &ToolchainManager::toolhainAdded, this, &ToolchainManager::toolchainsChanged);
    connect(this, &ToolchainManager::toolchainRemoved, this, &ToolchainManager::toolchainsChanged);
    connect(this, &ToolchainManager::toolchainUpdated, this, &ToolchainManager::toolchainsChanged);

    QtcSettings * const s = Core::ICore::settings();
    d->m_detectionSettings.detectX64AsX32
        = s->value(DETECT_X64_AS_X32_KEY, ToolchainDetectionSettings().detectX64AsX32).toBool();
    d->m_badToolchains = BadToolchains::fromVariant(s->value(badToolchainsKey()));
}

ToolchainManager::~ToolchainManager()
{
    m_instance = nullptr;
    delete d;
    d = nullptr;
}

ToolchainManager *ToolchainManager::instance()
{
    return m_instance;
}

void ToolchainManager::restoreToolchains()
{
    NANOTRACE_SCOPE("ProjectExplorer", "ToolchainManager::restoreToolChains");
    QTC_ASSERT(!d->m_accessor, return);
    d->m_accessor = std::make_unique<Internal::ToolchainSettingsAccessor>();

    for (Toolchain *tc : d->m_accessor->restoreToolchains(Core::ICore::dialogParent()))
        registerToolchain(tc);

    d->m_loaded = true;
    emit m_instance->toolchainsLoaded();
}

void ToolchainManager::saveToolchains()
{
    QTC_ASSERT(d->m_accessor, return);

    d->m_accessor->saveToolchains(d->m_toolChains, Core::ICore::dialogParent());
    QtcSettings *const s = Core::ICore::settings();
    s->setValueWithDefault(DETECT_X64_AS_X32_KEY,
                           d->m_detectionSettings.detectX64AsX32,
                           ToolchainDetectionSettings().detectX64AsX32);
    s->setValue(badToolchainsKey(), d->m_badToolchains.toVariant());
}

const Toolchains &ToolchainManager::toolchains()
{
    QTC_CHECK(d->m_loaded);
    return d->m_toolChains;
}

Toolchains ToolchainManager::toolchains(const Toolchain::Predicate &predicate)
{
    QTC_ASSERT(predicate, return {});
    return Utils::filtered(d->m_toolChains, predicate);
}

Toolchain *ToolchainManager::toolchain(const Toolchain::Predicate &predicate)
{
    QTC_CHECK(d->m_loaded);
    return Utils::findOrDefault(d->m_toolChains, predicate);
}

Toolchains ToolchainManager::findToolchains(const Abi &abi)
{
    QTC_CHECK(d->m_loaded);
    Toolchains result;
    for (Toolchain *tc : std::as_const(d->m_toolChains)) {
        bool isCompatible = Utils::anyOf(tc->supportedAbis(), [abi](const Abi &supportedAbi) {
            return supportedAbi.isCompatibleWith(abi);
        });

        if (isCompatible)
            result.append(tc);
    }
    return result;
}

Toolchain *ToolchainManager::findToolchain(const QByteArray &id)
{
    QTC_CHECK(d->m_loaded);
    if (id.isEmpty())
        return nullptr;

    Toolchain *tc = Utils::findOrDefault(d->m_toolChains, Utils::equal(&Toolchain::id, id));

    // Compatibility with versions 3.5 and earlier:
    if (!tc) {
        const int pos = id.indexOf(':');
        if (pos < 0)
            return tc;

        const QByteArray shortId = id.mid(pos + 1);

        tc = Utils::findOrDefault(d->m_toolChains, Utils::equal(&Toolchain::id, shortId));
    }
    return tc;
}

bool ToolchainManager::isLoaded()
{
    return d->m_loaded;
}

void ToolchainManager::notifyAboutUpdate(Toolchain *tc)
{
    if (!tc || !d->m_toolChains.contains(tc))
        return;
    emit m_instance->toolchainUpdated(tc);
}

bool ToolchainManager::registerToolchain(Toolchain *tc)
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
    for (const Toolchain *current : std::as_const(d->m_toolChains)) {
        if (*tc == *current && !tc->isAutoDetected())
            return false;
        QTC_ASSERT(current->id() != tc->id(), return false);
    }

    d->m_toolChains.append(tc);
    emit m_instance->toolhainAdded(tc);
    return true;
}

void ToolchainManager::deregisterToolchain(Toolchain *tc)
{
    QTC_CHECK(d->m_loaded);
    if (!tc || !d->m_toolChains.contains(tc))
        return;
    d->m_toolChains.removeOne(tc);
    emit m_instance->toolchainRemoved(tc);
    delete tc;
}

QList<Id> ToolchainManager::allLanguages()
{
    return Utils::transform<QList>(d->m_languages, &LanguageDisplayPair::id);
}

bool ToolchainManager::registerLanguage(const Utils::Id &language, const QString &displayName)
{
    QTC_ASSERT(language.isValid(), return false);
    QTC_ASSERT(!isLanguageSupported(language), return false);
    QTC_ASSERT(!displayName.isEmpty(), return false);
    d->m_languages.push_back({language, displayName});
    return true;
}

QString ToolchainManager::displayNameOfLanguageId(const Utils::Id &id)
{
    QTC_ASSERT(id.isValid(), return Tr::tr("None"));
    auto entry = Utils::findOrDefault(d->m_languages, Utils::equal(&LanguageDisplayPair::id, id));
    QTC_ASSERT(entry.id.isValid(), return Tr::tr("None"));
    return entry.displayName;
}

bool ToolchainManager::isLanguageSupported(const Utils::Id &id)
{
    return Utils::contains(d->m_languages, Utils::equal(&LanguageDisplayPair::id, id));
}

void ToolchainManager::aboutToShutdown()
{
    if (HostOsInfo::isWindowsHost())
        MsvcToolchain::cancelMsvcToolChainDetection();
}

ToolchainDetectionSettings ToolchainManager::detectionSettings()
{
    return d->m_detectionSettings;
}

void ToolchainManager::setDetectionSettings(const ToolchainDetectionSettings &settings)
{
    d->m_detectionSettings = settings;
}

void ToolchainManager::resetBadToolchains()
{
    d->m_badToolchains.toolchains.clear();
}

bool ToolchainManager::isBadToolchain(const Utils::FilePath &toolchain)
{
    return d->m_badToolchains.isBadToolchain(toolchain);
}

void ToolchainManager::addBadToolchain(const Utils::FilePath &toolchain)
{
    d->m_badToolchains.toolchains << toolchain;
}

} // namespace ProjectExplorer
