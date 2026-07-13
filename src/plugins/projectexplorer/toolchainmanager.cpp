// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolchainmanager.h"

#include "abi.h"
#include "devicesupport/devicemanager.h"
#include "msvctoolchain.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "toolchain.h"
#include "toolchainsettingsaccessor.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <nanotrace/nanotrace.h>

#include <QLoggingCategory>

#ifdef WITH_TESTS
#include "kit.h"
#include "kitmanager.h"
#include "toolchainconfigwidget.h"
#include "toolchainkitaspect.h"

#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>
#endif // WITH_TESTS

using namespace Utils;

namespace ProjectExplorer {

static Q_LOGGING_CATEGORY(tcLog, "qtc.projectexplorer.toolchains", QtWarningMsg)
namespace Internal {

// --------------------------------------------------------------------------
// ToolchainManagerPrivate
// --------------------------------------------------------------------------

class ToolchainManagerPrivate
{
public:
    ~ToolchainManagerPrivate();

    std::unique_ptr<ToolchainSettingsAccessor> m_accessor;

    Toolchains m_toolChains; // prioritized List
    BadToolchains m_badToolchains;   // to be skipped when auto-detecting

    QList<Id> m_languages;
    QHash<Id, QString> m_displayNameForLanguage;

    QList<LanguageCategory> m_languageCategories;
    QHash<LanguageCategory, QString> m_displayNameForCategory;

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
    connect(this, &ToolchainManager::toolchainsRegistered,
            this, &ToolchainManager::toolchainsChanged);
    connect(this, &ToolchainManager::toolchainsDeregistered, this, &ToolchainManager::toolchainsChanged);
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

    registerToolchains(d->m_accessor->restoreToolchains());

    d->m_loaded = true;
    emit m_instance->toolchainsLoaded();

    DeviceManager::addToolDetector(m_instance, Tr::tr("Searching for toolchains..."),
                                   [](const ToolDetectionRequest &request,
                                      const std::function<void()> &done) {
        const ToolDetectionLogger &logger = request.logger;
        ToolchainDetector detector(m_instance->toolchains(), request.device, request.searchPaths);
        Toolchains toRegister;
        for (ToolchainFactory *f : ToolchainFactory::allToolchainFactories()) {
            for (Toolchain * const tc : f->autoDetect(detector)) {
                if (!m_instance->toolchains().contains(tc)) {
                    toRegister << tc;
                    tc->setDetectionSource(request.source);
                }
            }
        }
        if (logger) {
            if (toRegister.isEmpty())
                logger.logItem(Tr::tr("No new toolchains found."));
            for (const Toolchain *tc : std::as_const(toRegister))
                logger.logItem(Tr::tr("Found toolchain: %1").arg(tc->displayName()));
        }
        registerToolchains(toRegister);

        // A toolchain that is not fully known yet - MSVC learns its compiler only once its
        // vcvars environment capture has run - keeps this detection open, so the kits, created
        // when the last detection task is deregistered, wait for it.
        for (Toolchain * const tc : std::as_const(toRegister))
            tc->holdToolDetection(request.device, request.token);

        done();
    });

    connect(DeviceManager::instance(), &DeviceManager::deviceUpdated, m_instance, [](Id deviceId) {
        // A device changed, e.g. reconnected. Retry toolchains on it that were invalid, so
        // they heal (e.g. get recognized as valid again) without requiring a manual re-detect.
        for (Toolchain *tc : std::as_const(d->m_toolChains)) {
            if (tc->isValid())
                continue;
            const IDeviceConstPtr dev = DeviceManager::deviceForPath(tc->compilerCommand());
            if (dev && dev->id() == deviceId && tc->refreshValid())
                notifyAboutUpdate(tc);
        }
    });
}

void ToolchainManager::saveToolchains()
{
    QTC_ASSERT(d->m_accessor, return);

    d->m_accessor->saveToolchains(d->m_toolChains);
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

Toolchain *ToolchainManager::findToolchain(Id id)
{
    QTC_CHECK(d->m_loaded);
    if (!id.isValid())
        return nullptr;

    Toolchain *tc = Utils::findOrDefault(d->m_toolChains, Utils::equal(&Toolchain::id, id));

    // Compatibility with versions 3.5 and earlier.
    // Also still used by the installer, e.g. see QTCREATORBUG-34681.
    if (!tc) {
        const int pos = id.name().indexOf(':');
        if (pos < 0)
            return tc;

        const Id shortId = Id::fromName(id.name().mid(pos + 1));

        tc = Utils::findOrDefault(d->m_toolChains, Utils::equal(&Toolchain::id, shortId));
    }
    return tc;
}

bool ToolchainManager::isLoaded()
{
    // May be called from a ToolchainFactory constructor before the manager itself exists.
    return d && d->m_loaded;
}

static void registerRecoveredToolchains(const Toolchains &recovered)
{
    const Toolchains rejected = ToolchainManager::registerToolchains(recovered);

    QList<std::pair<Toolchain *, Toolchain *>> replaced;
    for (Toolchain * const tc : ToolchainManager::toolchains()) {
        if (!tc->detectionSource().isAutoDetected() || recovered.contains(tc))
            continue;
        Toolchain * const replacement = Utils::findOrDefault(recovered, [&](const Toolchain *r) {
            return !rejected.contains(r) && *r == *tc;
        });
        if (replacement)
            replaced.append(std::make_pair(tc, replacement));
    }
    Toolchains redetected;
    for (const auto &[tc, replacement] : std::as_const(replaced)) {
        emit ToolchainManager::instance()->toolchainReplaced(tc, replacement);
        redetected << tc;
    }
    ToolchainManager::deregisterToolchains(redetected);
    qDeleteAll(rejected);
}

void ToolchainManager::toolchainFactoryAdded()
{
    // A factory was registered after the toolchains were loaded (e.g. by a soft-loaded plugin).
    // This runs from the ToolchainFactory base-class constructor, before the derived class has set
    // its supported type, and several factories may be added in one batch. Defer the retry to the
    // event loop so all newly registered factories are fully constructed first.
    static bool scheduled = false;
    if (!isLoaded() || scheduled)
        return;
    scheduled = true;
    QMetaObject::invokeMethod(
        m_instance,
        [] {
            scheduled = false;
            if (!d->m_accessor)
                return;
            const Toolchains recovered = d->m_accessor->retryDeferredToolchains();
            if (!recovered.isEmpty()) {
                qCInfo(tcLog) << "Restored" << recovered.size()
                              << "previously unrestorable toolchain(s)"
                              << "after a factory was registered";
                registerRecoveredToolchains(recovered);
            }
        },
        Qt::QueuedConnection);
}

bool ToolchainManager::isToolchainWithUnknownType(Id id)
{
    return d && d->m_accessor && d->m_accessor->isDeferred(id);
}

#ifdef WITH_TESTS
// Makes isToolchainWithUnknownType() report id, as for a toolchain stored with a type
// whose plugin is not loaded.
void ToolchainManager::deferToolchainForTest(Id id)
{
    QTC_ASSERT(d && d->m_accessor, return);
    d->m_accessor->deferForTest(id);
}

void ToolchainManager::removeDeferredToolchainForTest(Id id)
{
    QTC_ASSERT(d && d->m_accessor, return);
    d->m_accessor->removeDeferredForTest(id);
}
#endif // WITH_TESTS

void ToolchainManager::notifyAboutUpdate(Toolchain *tc)
{
    if (!tc || !d->m_toolChains.contains(tc))
        return;
    emit m_instance->toolchainUpdated(tc);
}

Toolchains ToolchainManager::registerToolchains(const Toolchains &toolchains)
{
    Toolchains registered;
    Toolchains notRegistered;

    for (Toolchain * const tc : toolchains) {
        QTC_ASSERT(tc, notRegistered << tc; continue);
        QTC_ASSERT(isLanguageSupported(tc->language()),
                   qDebug() << qPrintable("language \"" + tc->language().toString()
                                          + "\" unknown while registering \""
                                          + tc->compilerCommand().toUrlishString() + "\"");
                   notRegistered << tc;
                   continue);
        QTC_ASSERT(d->m_accessor, notRegistered << tc; continue);
        QTC_ASSERT(!d->m_toolChains.contains(tc), continue);
        QTC_ASSERT(!Utils::contains(d->m_toolChains, Utils::equal(&Toolchain::id, tc->id())),
                   notRegistered << tc;
                   continue);
        if (!tc->detectionSource().isAutoDetected()
            && Utils::contains(d->m_toolChains, [tc](const Toolchain *existing) {
                   return *tc == *existing;
               })) {
            notRegistered << tc;
            continue;
        }
        d->m_toolChains << tc;
        registered << tc;
    }

    if (!registered.isEmpty())
        emit m_instance->toolchainsRegistered(registered);
    return notRegistered;
}

void ToolchainManager::deregisterToolchains(const Toolchains &toolchains)
{
    QTC_CHECK(d->m_loaded);
    Toolchains deregistered;
    for (Toolchain * const tc : toolchains) {
        QTC_ASSERT(tc, continue);
        const bool removed = d->m_toolChains.removeOne(tc);
        QTC_ASSERT(removed, continue);
        deregistered << tc;
    }

    if (!deregistered.isEmpty())
        emit m_instance->toolchainsDeregistered(deregistered);
    qDeleteAll(toolchains);
}

QList<Id> ToolchainManager::allLanguages()
{
    return d->m_languages;
}

bool ToolchainManager::registerLanguage(const Utils::Id &language, const QString &displayName)
{
    QTC_ASSERT(language.isValid(), return false);
    QTC_ASSERT(!isLanguageSupported(language), return false);
    QTC_ASSERT(!displayName.isEmpty(), return false);
    d->m_languages.push_back(language);
    d->m_displayNameForLanguage.insert(language, displayName);
    return true;
}

void ToolchainManager::registerLanguageCategory(const LanguageCategory &languages, const QString &displayName)
{
    d->m_languageCategories.push_back(languages);
    d->m_displayNameForCategory.insert(languages, displayName);
}

QString ToolchainManager::displayNameOfLanguageId(const Utils::Id &id)
{
    QTC_ASSERT(id.isValid(), return Tr::tr("None", "No compiler language"));
    QString display = d->m_displayNameForLanguage.value(id);
    QTC_ASSERT(!display.isEmpty(), return Tr::tr("None", "No compiler language"));
    return display;
}

QString ToolchainManager::displayNameOfLanguageCategory(const LanguageCategory &category)
{
    if (int(category.size()) == 1)
        return displayNameOfLanguageId(*category.begin());
    QString name = d->m_displayNameForCategory.value(category);
    QTC_ASSERT(!name.isEmpty(), return Tr::tr("None", "No compiler category"));
    return name;
}

const QList<LanguageCategory> ToolchainManager::languageCategories()
{
    QList<LanguageCategory> categories = d->m_languageCategories;
    const QList<Utils::Id> languages = allLanguages();
    for (const Utils::Id &l : languages) {
        if (Utils::contains(categories, [l](const LanguageCategory &lc) {
                return lc.contains(l);
            })) {
            continue;
        }
        categories.push_back({l});
    }

    return categories;
}

bool ToolchainManager::isLanguageSupported(const Utils::Id &id)
{
    return d->m_languages.contains(id);
}

void ToolchainManager::aboutToShutdown()
{
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

// Use as a tie-breaker for toolchains that match the strong requirements like toolchain type
// and ABI.
// For toolchains with the same priority, gives precedence to icecc and ccache,
// prefers the higher version and otherwise simply chooses the one with the shortest path.
bool ToolchainManager::isBetterToolchain(
    const ToolchainBundle &bundle1, const ToolchainBundle &bundle2)
{
    if (const ToolchainBundle::Valid valid1 = bundle1.validity(), valid2 = bundle2.validity();
        valid1 != valid2) {
        return valid1 == ToolchainBundle::Valid::All || valid2 == ToolchainBundle::Valid::None;
    }

    const int priority1 = bundle1.get(&Toolchain::priority);
    const int priority2 = bundle2.get(&Toolchain::priority);
    if (priority1 > priority2)
        return true;
    if (priority1 < priority2)
        return false;

    const FilePath path1 = bundle1.get(&Toolchain::compilerCommand);
    const FilePath path2 = bundle2.get(&Toolchain::compilerCommand);
    const QString pathString1 = path1.path();
    const QString pathString2 = path2.path();

    const bool b1IsIcecc = pathString1.contains("icecc");
    const bool b2IsIcecc = pathString2.contains("icecc");
    if (b1IsIcecc)
        return !b2IsIcecc;
    if (b2IsIcecc)
        return false;

    const bool b1IsCCache = pathString1.contains("ccache");
    const bool b2IsCcache = pathString2.contains("ccache");
    if (b1IsCCache)
        return !b2IsCcache;
    if (b2IsCcache)
        return false;

    // Hack to prefer a tool chain from PATH (e.g. autodetected) over other matches.
    // This improves the situation a bit if a cross-compilation tool chain has the
    // same ABI as the host.
    if (bundle1.get(&Toolchain::compilerCommand).isLocal()) {
        const FilePaths envPathVar = Environment::systemEnvironment().path();
        const auto toolchainIsInPath = [&envPathVar](const ToolchainBundle &b) {
            return Utils::contains(b.toolchains(), [&envPathVar](const Toolchain *tc) {
                return envPathVar.contains(tc->compilerCommand().parentDir());
            });
        };
        const bool tc1IsInPath = toolchainIsInPath(bundle1);
        const bool tc2IsInPath = toolchainIsInPath(bundle2);
        if (tc1IsInPath) {
            if (!tc2IsInPath)
                return true;
        } else if (tc2IsInPath) {
            return false;
        }
    }

    if (path1.isLocal() && path2.isLocal()) {
        const QVersionNumber v1 = bundle1.get(&Toolchain::version);
        const QVersionNumber v2 = bundle2.get(&Toolchain::version);
        if (!v1.isNull() && !v2.isNull()) {
            if (v1 > v2)
                return true;
            if (v1 < v2)
                return false;
        }
    }

    return pathString1.size() < pathString2.size();
}

#ifdef WITH_TESTS
namespace Internal {

class ValidityTestToolchain : public Toolchain
{
public:
    ValidityTestToolchain()
        : Toolchain("ProjectExplorer.ToolChain.ValidityTest")
    {
        setLanguage(Constants::CXX_LANGUAGE_ID);
        setTypeDisplayName("Validity Test Toolchain");
        setTargetAbiNoSignal(Abi::hostAbi());
    }

    MacroInspectionRunner createMacroInspectionRunner() const override { return {}; }
    LanguageExtensions languageExtensions(const QStringList &) const override
    { return LanguageExtension::None; }
    WarningFlags warningFlags(const QStringList &) const override
    { return WarningFlag::NoWarnings; }
    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner(const Environment &) const override
    { return {}; }
    void addToEnvironment(Environment &) const override {}
    FilePath makeCommand(const Environment &) const override { return "make"; }
    QList<OutputLineParser *> createOutputParsers() const override { return {}; }
};

class ToolchainManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void testRevalidateOnDeviceUpdate()
    {
        const IDevice::ConstPtr desktop = DeviceManager::defaultDesktopDevice();
        QVERIFY(desktop);

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath compiler
            = FilePath::fromString(dir.path())
                  .pathAppended(HostOsInfo::withExecutableSuffix("fake-compiler"));

        auto tc = new ValidityTestToolchain;
        const QScopeGuard cleanup([tc] { ToolchainManager::deregisterToolchains({tc}); });
        tc->setCompilerCommand(compiler);
        QVERIFY(!tc->isValid());
        QVERIFY(ToolchainManager::registerToolchains({tc}).isEmpty());
        QCOMPARE(DeviceManager::deviceForPath(compiler), desktop);

        int updates = 0;
        connect(ToolchainManager::instance(), &ToolchainManager::toolchainUpdated,
                tc, [tc, &updates](Toolchain *updated) {
            if (updated == tc)
                ++updates;
        });

        // The contents do not matter, only that an executable file exists.
        QVERIFY(compiler.writeFileContents({}));
        QVERIFY(compiler.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        QVERIFY(!tc->isValid()); // The negative result is cached until a device update.

        // Any state change announces a device update. Toggle and restore.
        const IDevice::DeviceState state = DeviceManager::deviceState(desktop->id());
        const IDevice::DeviceState otherState = state == IDevice::DeviceReadyToUse
                                                    ? IDevice::DeviceStateUnknown
                                                    : IDevice::DeviceReadyToUse;
        DeviceManager::setDeviceState(desktop->id(), otherState);
        DeviceManager::setDeviceState(desktop->id(), state);

        QVERIFY(tc->isValid());
        QCOMPARE(updates, 1);
    }

    void testRecoveredReplacesRedetected()
    {
        const auto validityTcs = [] {
            return ToolchainManager::toolchains([](const Toolchain *tc) {
                return tc->typeId() == "ProjectExplorer.ToolChain.ValidityTest";
            });
        };
        const QScopeGuard cleanup([&] { ToolchainManager::deregisterToolchains(validityTcs()); });

        Kit * const kit = KitManager::registerKit([](Kit *k) {
            k->setUnexpandedDisplayName("Recovered Toolchain Kit");
        });
        QVERIFY(kit);
        const QScopeGuard deregisterKit([kit] { KitManager::deregisterKit(kit); });

        Kit * const redetectedKit = KitManager::registerKit([](Kit *k) {
            k->setUnexpandedDisplayName("Redetected Toolchain Kit");
        });
        QVERIFY(redetectedKit);
        const QScopeGuard deregisterRedetectedKit([redetectedKit] {
            KitManager::deregisterKit(redetectedKit);
        });

        auto redetected = new ValidityTestToolchain;
        redetected->setDetectionSource(DetectionSource::FromSystem);
        QVERIFY(ToolchainManager::registerToolchains({redetected}).isEmpty());
        ToolchainKitAspect::setToolchain(redetectedKit, redetected);

        auto recovered = new ValidityTestToolchain;
        recovered->setDetectionSource(DetectionSource::FromSystem);
        ToolchainKitAspect::setToolchain(kit, recovered);
        registerRecoveredToolchains({recovered});

        const Id cxx = Constants::CXX_LANGUAGE_ID;
        QCOMPARE(validityTcs(), Toolchains{recovered});
        QCOMPARE(ToolchainKitAspect::toolchain(kit, cxx), recovered);
        QCOMPARE(ToolchainKitAspect::toolchain(redetectedKit, cxx), recovered);
    }

    void testDeferredToolchainInKit()
    {
        class ValidityTestFactory final : public ToolchainFactory
        {
        public:
            ValidityTestFactory()
            {
                setSupportedToolchainType("ProjectExplorer.ToolChain.ValidityTest");
                setToolchainConstructor([] { return new ValidityTestToolchain; });
            }
            std::unique_ptr<ToolchainConfigWidget> createConfigurationWidget(
                const ToolchainBundle &) const override
            {
                return nullptr;
            }
        };

        const auto validityTcs = [] {
            return ToolchainManager::toolchains([](const Toolchain *tc) {
                return tc->typeId() == "ProjectExplorer.ToolChain.ValidityTest";
            });
        };
        const QScopeGuard cleanup([&] { ToolchainManager::deregisterToolchains(validityTcs()); });

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath file = FilePath::fromString(dir.filePath("toolchains.xml"));
        const std::unique_ptr<Toolchain> saved(new ValidityTestToolchain);
        const std::unique_ptr<Toolchain> savedC(new ValidityTestToolchain);
        savedC->setLanguage(Constants::C_LANGUAGE_ID);
        {
            ToolchainSettingsAccessor accessor;
            accessor.setBaseFilePath(file);
            accessor.saveToolchains({saved.get(), savedC.get()});
        }
        auto accessor = std::make_unique<ToolchainSettingsAccessor>();
        accessor->setBaseFilePath(file);
        qDeleteAll(accessor->restoreToolchains());
        QVERIFY(accessor->isDeferred(saved->id()));
        QVERIFY(accessor->isDeferred(savedC->id()));
        std::swap(d->m_accessor, accessor);
        const QScopeGuard restoreAccessor([&] { std::swap(d->m_accessor, accessor); });

        const auto registerKit = [&](const QString &name, const DetectionSource &source) {
            return KitManager::registerKit([&](Kit *k) {
                k->setUnexpandedDisplayName(name);
                k->setDetectionSource(source);
                ToolchainKitAspect::setToolchain(k, saved.get());
                ToolchainKitAspect::setToolchain(k, savedC.get());
            });
        };
        Kit * const kit = registerKit("Deferred Toolchain Kit", {});
        QVERIFY(kit);
        const QScopeGuard deregisterKit([kit] { KitManager::deregisterKit(kit); });
        Kit * const sdkKit = registerKit("Deferred Toolchain SDK Kit", DetectionSource::FromSdk);
        QVERIFY(sdkKit);
        const QScopeGuard deregisterSdkKit([sdkKit] { KitManager::deregisterKit(sdkKit); });
        const Id cxx = Constants::CXX_LANGUAGE_ID;
        const Id c = Constants::C_LANGUAGE_ID;
        QCOMPARE(ToolchainKitAspect::toolchainId(kit, cxx), saved->id());
        kit->setup();
        kit->fix();
        QCOMPARE(ToolchainKitAspect::toolchainId(kit, cxx), saved->id());
        QCOMPARE(ToolchainKitAspect::toolchainId(kit, c), savedC->id());
        QVERIFY(!ToolchainKitAspect::toolchain(kit, cxx));
        sdkKit->setup();
        QCOMPARE(ToolchainKitAspect::toolchainId(sdkKit, cxx), saved->id());
        QCOMPARE(ToolchainKitAspect::toolchainId(sdkKit, c), savedC->id());
        QVERIFY(sdkKit->isSticky(ToolchainKitAspect::id()));

        int updates = 0;
        QObject guard;
        connect(KitManager::instance(), &KitManager::kitUpdated, &guard, [kit, &updates](Kit *k) {
            if (k == kit)
                ++updates;
        });
        const ValidityTestFactory factory;
        QTRY_VERIFY(ToolchainKitAspect::toolchain(kit, cxx));
        QCOMPARE(ToolchainKitAspect::toolchainId(kit, cxx), saved->id());
        QCOMPARE(updates, 1);
    }
};

QObject *createToolchainManagerTest()
{
    return new ToolchainManagerTest;
}

} // namespace Internal
#endif // WITH_TESTS

} // namespace ProjectExplorer

#ifdef WITH_TESTS
#include <toolchainmanager.moc>
#endif
