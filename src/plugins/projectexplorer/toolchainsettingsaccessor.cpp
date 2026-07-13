// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolchainsettingsaccessor.h"

#include "devicesupport/devicemanager.h"
#include "projectexplorerconstants.h"
#include "toolchain.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <nanotrace/nanotrace.h>

#include <QElapsedTimer>
#include <QGuiApplication>
#include <QLoggingCategory>

#ifdef WITH_TESTS
#include "abi.h"
#include "toolchainconfigwidget.h"

#include <QSet>
#include <QTemporaryDir>
#include <QTest>
#endif // WITH_TESTS

using namespace Utils;

namespace ProjectExplorer::Internal {

static Q_LOGGING_CATEGORY(Log, "qtc.projectexplorer.toolchain.autodetection", QtWarningMsg)

// --------------------------------------------------------------------
// ToolChainSettingsUpgraders:
// --------------------------------------------------------------------

class ToolChainSettingsUpgraderV0 : public Utils::VersionUpgrader
{
    // Necessary to make Version 1 supported.
public:
    ToolChainSettingsUpgraderV0() : Utils::VersionUpgrader(0, "4.6") { }

    // NOOP
    Store upgrade(const Store &data) final { return data; }
};

// --------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------

const char TOOLCHAIN_DATA_KEY[] = "ToolChain.";
const char TOOLCHAIN_COUNT_KEY[] = "ToolChain.Count";
const char TOOLCHAIN_FILENAME[] = "toolchains.xml";

struct ToolChainOperations
{
    Toolchains toDemote;
    Toolchains toRegister;
    Toolchains toDelete;
};

static Toolchains autoDetectToolchains(const ToolchainDetector &detector)
{
    Toolchains result;
    for (ToolchainFactory *f : ToolchainFactory::allToolchainFactories()) {
        NANOTRACE_SCOPE_ARGS("ProjectExplorer",
                             "ToolchainSettingsAccessor::autoDetectToolchains",
                             {"factory", f->displayName().toStdString()});
        QElapsedTimer et;
        et.start();
        result.append(f->autoDetect(detector));
        qCDebug(Log) << f->displayName() << "auto detection took: " << et.elapsed() << "ms";
    }

    // We only use this for Desktop, so "FromSystem" detection is fine
    for (Toolchain *tc : result) {
        // but do not override the ID
        DetectionSource ds = tc->detectionSource();
        ds.type = DetectionSource::FromSystem;
        tc->setDetectionSource(ds);
    }

    // Remove invalid toolchains that might have sneaked in.
    return Utils::filtered(result, [](const Toolchain *tc) { return tc->isValid(); });
}

static Toolchains makeUniqueByEqual(const Toolchains &a)
{
    Toolchains result;
    for (Toolchain *tc : a) {
        if (!Utils::contains(result, [tc](Toolchain *rtc) { return *tc == *rtc; }))
            result.append(tc);
    }
    return result;
}

static Toolchains makeUniqueByPointerEqual(const Toolchains &a)
{
    return Utils::toList(Utils::toSet(a));
}

static Toolchains subtractById(const Toolchains &a, const Toolchains &b)
{
    return Utils::filtered(a, [&b](Toolchain *atc) {
                                  return !Utils::anyOf(b, Utils::equal(&Toolchain::id, atc->id()));
                              });
}

static bool containsByEqual(const Toolchains &a, const Toolchain *atc)
{
    return Utils::anyOf(a, [atc](Toolchain *btc) { return *atc == *btc; });
}

static Toolchains subtractByEqual(const Toolchains &a, const Toolchains &b)
{
    return Utils::filtered(a, [&b](Toolchain *atc) {
                                  return !Utils::anyOf(b, [atc](Toolchain *btc) { return *atc == *btc; });
                              });
}

static Toolchains subtractByPointerEqual(const Toolchains &a, const Toolchains &b)
{
    return Utils::filtered(a, [&b](Toolchain *atc) { return !b.contains(atc); });
}

static Toolchains stabilizeOrder(const Toolchains &toRegister,
                                         const Toolchains &userFileTcs)
{
    // Keep the toolchains in their position in the user toolchain file to minimize diff!
    Toolchains result;
    result.reserve(toRegister.size());
    Toolchains toHandle = toRegister;

    for (int i = 0; i < userFileTcs.count(); ++i) {
        const Id userId = userFileTcs.at(i)->id();
        const int handlePos = Utils::indexOf(toHandle,
                                             [&userId](const Toolchain *htc) { return htc->id() == userId; });
        if (handlePos < 0)
            continue;

        result.append(toHandle.at(handlePos));
        toHandle.removeAt(handlePos);
    }
    result.append(toHandle);
    return result;
}

static ToolChainOperations mergeToolChainLists(const Toolchains &systemFileTcs,
                                               const Toolchains &userFileTcs,
                                               const Toolchains &autodetectedTcs)
{
    const Toolchains uniqueUserFileTcs = makeUniqueByEqual(userFileTcs);
    Toolchains manualUserFileTcs;
    Toolchains autodetectedUserFileTcs;
    std::tie(autodetectedUserFileTcs, manualUserFileTcs)
            = Utils::partition(uniqueUserFileTcs,[](Toolchain *tc) {
                return tc->detectionSource().isAutoDetected();
              });
    const Toolchains autodetectedUserTcs = subtractById(autodetectedUserFileTcs, systemFileTcs);

    // Calculate a set of Tcs that were detected before (and saved to userFile) and that
    // got re-detected again. Take the userTcs (to keep Ids) over the same in autodetectedTcs.
    Toolchains redetectedUserTcs;
    Toolchains notRedetectedUserTcs;
    std::tie(redetectedUserTcs, notRedetectedUserTcs)
            = Utils::partition(autodetectedUserTcs,
                               [&autodetectedTcs](Toolchain *tc) { return containsByEqual(autodetectedTcs, tc); });

    // Remove redetected tcs from autodetectedTcs:
    const Toolchains newlyAutodetectedTcs
            = subtractByEqual(autodetectedTcs, redetectedUserTcs);

    const Toolchains notRedetectedButValidLocalUserTcs
        = Utils::filtered(notRedetectedUserTcs, [](const Toolchain *tc) {
              return tc->isValid() && tc->compilerCommand().isLocal();
          });

    const Toolchains notRedetectedRemoteUserTcs
        = Utils::filtered(notRedetectedUserTcs, [](const Toolchain *tc) {
              return tc->isDeviceDetected();
          });

    ToolChainOperations result;
    result.toDemote = notRedetectedButValidLocalUserTcs;
    result.toRegister = stabilizeOrder(
        // manual TCs
        systemFileTcs + manualUserFileTcs + result.toDemote

        // auto TCs
            + redetectedUserTcs + notRedetectedRemoteUserTcs
            + newlyAutodetectedTcs,
        userFileTcs);

    result.toDelete = makeUniqueByPointerEqual(subtractByPointerEqual(systemFileTcs + userFileTcs + autodetectedTcs,
                                                                      result.toRegister));
    return result;
}

// --------------------------------------------------------------------
// ToolChainSettingsAccessor:
// --------------------------------------------------------------------

ToolchainSettingsAccessor::ToolchainSettingsAccessor()
{
    setDocType("QtCreatorToolChains");
    setApplicationDisplayName(QGuiApplication::applicationDisplayName());
    setBaseFilePath(Core::ICore::userResourcePath(TOOLCHAIN_FILENAME));

    addVersionUpgrader(std::make_unique<ToolChainSettingsUpgraderV0>());
}

static void warnUnrestorable(const Store &tcMap)
{
    qWarning("Warning: Unable to restore compiler type '%s' for toolchain %s.",
             qPrintable(ToolchainFactory::typeIdFromMap(tcMap).toString()),
             qPrintable(ToolchainFactory::idFromMap(tcMap).toString()));
}

static bool containsId(const QList<Store> &tcMaps, Id id)
{
    return Utils::anyOf(tcMaps, [id](const Store &tcMap) {
        return ToolchainFactory::idFromMap(tcMap) == id;
    });
}

Toolchains ToolchainSettingsAccessor::restoreToolchains()
{
    NANOTRACE_SCOPE("ProjectExplorer", "ToolChainSettingsAccessor::restoreToolChains");
    // read all tool chains from SDK
    m_deferredSdkToolchains.clear();
    const Toolchains systemFileTcs = toolChains(
        restoreSettings(Core::ICore::installerResourcePath(TOOLCHAIN_FILENAME)),
        m_deferredSdkToolchains);
    for (Toolchain * const systemTc : systemFileTcs)
        systemTc->setDetectionSource(DetectionSource::FromSdk);

    // read all tool chains from user file.
    QList<Store> deferredUserTcs;
    const Toolchains userFileTcs = toolChains(restoreSettings(), deferredUserTcs);
    m_deferredUserToolchains = Utils::filtered(deferredUserTcs, [this](const Store &tcMap) {
        return !containsId(m_deferredSdkToolchains, ToolchainFactory::idFromMap(tcMap));
    });

    // Autodetect: Pass autodetected toolchains from user file so the information can be reused:
    const Toolchains autodetectedUserFileTcs
            = Utils::filtered(userFileTcs, [](Toolchain *tc) {
                return tc->detectionSource().isAutoDetected();
              });

    // Autodect from system paths on the desktop device.
    // The restriction is intentional to keep startup and automatic validation a limited effort
    ToolchainDetector detector(autodetectedUserFileTcs, DeviceManager::defaultDesktopDevice(), {});
    const Toolchains autodetectedTcs = autoDetectToolchains(detector);

    // merge tool chains and register those that we need to keep:
    const ToolChainOperations ops = mergeToolChainLists(systemFileTcs, userFileTcs, autodetectedTcs);

    // Process ops:
    for (Toolchain *tc : ops.toDemote) {
        if (tc->detectionSource().id.isEmpty())
            tc->setDetectionSource(DetectionSource::Manual);
    }

    qDeleteAll(ops.toDelete);

    return ops.toRegister;
}

void ToolchainSettingsAccessor::saveToolchains(const Toolchains &toolchains)
{
    Store data;

    int count = 0;
    for (const Toolchain *tc : toolchains) {
        QTC_ASSERT(tc, continue);
        if (tc->detectionSource().isTemporary())
            continue;

        // A device might just be unreachable at the moment.
        if (!tc->isValid() && tc->detectionSource().isAutoDetected()
            && !tc->isDeviceDetected()) {
            continue;
        }
        Store tmp;
        tc->toMap(tmp);
        if (tmp.isEmpty())
            continue;
        data.insert(numberedKey(TOOLCHAIN_DATA_KEY, count), variantFromStore(tmp));
        ++count;
    }
    // Keep the ones whose factory is not registered yet. The SDK ones come back from the
    // installer file.
    for (const Store &deferred : std::as_const(m_deferredUserToolchains)) {
        data.insert(numberedKey(TOOLCHAIN_DATA_KEY, count), variantFromStore(deferred));
        ++count;
    }
    data.insert(TOOLCHAIN_COUNT_KEY, count);

    // Do not save default debuggers! Those are set by the SDK!

    saveSettings(data);
}

Toolchains ToolchainSettingsAccessor::toolChains(const Store &data, QList<Store> &deferred) const
{
    Toolchains result;

    const int count = data.value(TOOLCHAIN_COUNT_KEY, 0).toInt();
    for (int i = 0; i < count; ++i) {
        const Key key = numberedKey(TOOLCHAIN_DATA_KEY, i);
        if (!data.contains(key))
            break;

        const Store tcMap = storeFromVariant(data.value(key));

        const Utils::Id tcType = ToolchainFactory::typeIdFromMap(tcMap);
        ToolchainFactory * const f = tcType.isValid() ? ToolchainFactory::factoryForType(tcType)
                                                      : nullptr;
        if (!f) {
            // No factory (yet). Keep the raw data so a factory registered later (e.g. by a
            // soft-loaded plugin) can restore it.
            deferred.append(tcMap);
            qCDebug(Log, "Deferring restore of toolchain of unknown compiler type '%s'.",
                    qPrintable(tcType.toString()));
        } else if (Toolchain *tc = f->restore(tcMap)) {
            result.append(tc);
        } else {
            warnUnrestorable(tcMap);
        }
    }

    return result;
}

static Toolchains restoreDeferred(QList<Store> &tcMaps)
{
    Toolchains result;
    for (auto it = tcMaps.begin(); it != tcMaps.end();) {
        const Utils::Id tcType = ToolchainFactory::typeIdFromMap(*it);
        ToolchainFactory * const f = tcType.isValid() ? ToolchainFactory::factoryForType(tcType)
                                                      : nullptr;
        if (!f) {
            ++it;
            continue;
        }
        if (Toolchain * const tc = f->restore(*it))
            result.append(tc);
        else
            warnUnrestorable(*it);
        it = tcMaps.erase(it);
    }
    return result;
}

Toolchains ToolchainSettingsAccessor::retryDeferredToolchains()
{
    const Toolchains sdkTcs = restoreDeferred(m_deferredSdkToolchains);
    for (Toolchain * const tc : sdkTcs)
        tc->setDetectionSource(DetectionSource::FromSdk);
    return sdkTcs + restoreDeferred(m_deferredUserToolchains);
}

bool ToolchainSettingsAccessor::isDeferred(Id toolchainId) const
{
    const auto contains = [this](Id id) {
        return containsId(m_deferredSdkToolchains, id) || containsId(m_deferredUserToolchains, id);
    };
    if (contains(toolchainId))
        return true;
    // Installer-written kits refer to toolchains as "<type>:<id>", see findToolchain().
    const int pos = toolchainId.name().indexOf(':');
    return pos >= 0 && contains(Id::fromName(toolchainId.name().mid(pos + 1)));
}

#ifdef WITH_TESTS

void ToolchainSettingsAccessor::deferForTest(Id toolchainId)
{
    Store tcMap;
    tcMap.insert("ProjectExplorer.ToolChain.Id",
                 QString("ProjectExplorer.ToolChain.NoFactory:" + toolchainId.toString()));
    m_deferredUserToolchains.append(tcMap);
}

void ToolchainSettingsAccessor::removeDeferredForTest(Id toolchainId)
{
    Utils::eraseOne(m_deferredUserToolchains, [toolchainId](const Store &tcMap) {
        return ToolchainFactory::idFromMap(tcMap) == toolchainId;
    });
}

const char TestTokenKey[] = "TestTokenKey";
const char TestToolChainType[] = "TestToolChainType";

class TTC : public Toolchain
{
public:
    TTC(const QByteArray &t = {}, bool v = true) :
        Toolchain(TestToolChainType),
        token(t),
        m_valid(v)
    {
        m_toolChains.append(this);
        setLanguage(Constants::CXX_LANGUAGE_ID);
        setTypeDisplayName("Test Tool Chain");
        setTargetAbiNoSignal(Abi::hostAbi());
        setCompilerCommandKey("TestCompilerCommand");
        setCompilerCommand("/tmp/test/gcc");
    }

    static QList<TTC *> toolChains() { return m_toolChains; }
    static bool hasToolChains() { return !m_toolChains.isEmpty(); }

    bool isValid() const override { return m_valid; }
    MacroInspectionRunner createMacroInspectionRunner() const override { return {}; }
    LanguageExtensions languageExtensions(const QStringList &cxxflags) const override { Q_UNUSED(cxxflags) return LanguageExtension::None; }
    WarningFlags warningFlags(const QStringList &cflags) const override { Q_UNUSED(cflags) return WarningFlag::NoWarnings; }
    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner(const Utils::Environment &) const override { return {}; }
    void addToEnvironment(Environment &env) const override { Q_UNUSED(env) }
    FilePath makeCommand(const Environment &) const override { return "make"; }
    QList<OutputLineParser *> createOutputParsers() const override { return {}; }
    bool operator ==(const Toolchain &other) const override {
        if (!Toolchain::operator==(other))
            return false;
        return static_cast<const TTC *>(&other)->token == token;
    }
    bool canShareBundleImpl(const Toolchain &) const override { return false; }

    void fromMap(const Store &data) final
    {
        Toolchain::fromMap(data);
        token = data.value(TestTokenKey).toByteArray();
    }

    void toMap(Store &data) const final
    {
        Toolchain::toMap(data);
        data[TestTokenKey] = token;
    }

    QByteArray token;

private:
    bool m_valid = false;

    static inline QList<TTC *> m_toolChains;
};

class ToolchainSettingsTest : public QObject
{
    Q_OBJECT

private slots:
    void testMerging_data()
    {
        class TestToolchainFactory : ToolchainFactory
        {
        public:
            TestToolchainFactory() {
                setSupportedToolchainType(TestToolChainType);
                setToolchainConstructor([] { return new TTC; });
            }
            std::unique_ptr<ToolchainConfigWidget> createConfigurationWidget(
                const ToolchainBundle &) const override
            {
                return nullptr;
            }
        };

        TestToolchainFactory factory;

        QTest::addColumn<Toolchains>("system");
        QTest::addColumn<Toolchains>("user");
        QTest::addColumn<Toolchains>("autodetect");
        QTest::addColumn<Toolchains>("toDemote");
        QTest::addColumn<Toolchains>("toRegister");

        TTC *system1 = nullptr;
        Toolchain *system1c = nullptr;
        TTC *system2 = nullptr;
        TTC *system3i = nullptr;
        TTC *user1 = nullptr;
        Toolchain *user1c = nullptr;
        TTC *user3i = nullptr;
        TTC *user2 = nullptr;
        TTC *auto1 = nullptr;
        Toolchain *auto1c = nullptr;
        TTC *auto1_2 = nullptr;
        TTC *auto2 = nullptr;
        TTC *auto3i = nullptr;

        if (!TTC::hasToolChains()) {
            system1 = new TTC("system1");
            system1->setDetectionSource(DetectionSource::FromSystem);
            system1c = system1->clone(); Q_UNUSED(system1c)
            system2 = new TTC("system2");
            system2->setDetectionSource(DetectionSource::FromSystem);
            system3i = new TTC("system3", false);
            system3i->setDetectionSource(DetectionSource::FromSystem);
            user1 = new TTC("user1");
            user1->setDetectionSource(DetectionSource::Manual);
            user1c = user1->clone(); Q_UNUSED(user1c)
            user2 = new TTC("user2");
            user2->setDetectionSource(DetectionSource::Manual);
            user3i = new TTC("user3", false);
            user3i->setDetectionSource(DetectionSource::Manual);
            auto1 = new TTC("auto1");
            auto1->setDetectionSource(DetectionSource::FromSystem);
            auto1c = auto1->clone();
            auto1_2 = new TTC("auto1");
            auto1_2->setDetectionSource(DetectionSource::FromSystem);
            auto2 = new TTC("auto2");
            auto2->setDetectionSource(DetectionSource::FromSystem);
            auto3i = new TTC("auto3", false);
            auto3i->setDetectionSource(DetectionSource::FromSystem);
        }

        QTest::newRow("no toolchains")
            << (Toolchains()) << (Toolchains()) << (Toolchains())
            << (Toolchains()) << (Toolchains());

        QTest::newRow("System: system, no user")
            << (Toolchains() << system1) << (Toolchains()) << (Toolchains())
            << (Toolchains()) << (Toolchains() << system1);
        QTest::newRow("System: system, user")
            << (Toolchains() << system1) << (Toolchains() << system1) << (Toolchains())
            << (Toolchains()) << (Toolchains() << system1);
        QTest::newRow("System: no system, user") // keep, the user tool chain as it is still found
            << (Toolchains()) << (Toolchains() << system1) << (Toolchains())
            << (Toolchains() << system1) << (Toolchains() << system1);
        QTest::newRow("System: no system, invalid user")
            << (Toolchains()) << (Toolchains() << system3i) << (Toolchains())
            << (Toolchains()) << (Toolchains());

        QTest::newRow("Auto: no auto, user")
            << (Toolchains()) << (Toolchains() << auto1) << (Toolchains())
            << (Toolchains() << auto1) << (Toolchains() << auto1);
        QTest::newRow("Auto: auto, no user")
            << (Toolchains()) << (Toolchains()) << (Toolchains() << auto1)
            << (Toolchains()) << (Toolchains() << auto1);
        QTest::newRow("Auto: auto, user")
            << (Toolchains()) << (Toolchains() << auto1) << (Toolchains() << auto1)
            << (Toolchains()) << (Toolchains() << auto1);
        QTest::newRow("Auto: auto-redetect, user")
            << (Toolchains()) << (Toolchains() << auto1) << (Toolchains() << auto1_2)
            << (Toolchains()) << (Toolchains() << auto1);
        QTest::newRow("Auto: auto-redetect, duplicate users")
            << (Toolchains()) << (Toolchains() << auto1 << auto1c) << (Toolchains() << auto1_2)
            << (Toolchains()) << (Toolchains() << auto1);
        QTest::newRow("Auto: (no) auto, invalid user")
            << (Toolchains()) << (Toolchains() << auto3i) << (Toolchains())
            << (Toolchains()) << (Toolchains());

        QTest::newRow("invalid user")
            << (Toolchains()) << (Toolchains() << user3i) << (Toolchains())
            << (Toolchains()) << (Toolchains{user3i});

        QTest::newRow("one of everything")
            << (Toolchains() << system1) << (Toolchains() << user1) << (Toolchains() << auto1)
            << (Toolchains()) << (Toolchains() << system1 << user1 << auto1);
    }

    void testMerging()
    {
        QFETCH(Toolchains, system);
        QFETCH(Toolchains, user);
        QFETCH(Toolchains, autodetect);
        QFETCH(Toolchains, toRegister);
        QFETCH(Toolchains, toDemote);

        Internal::ToolChainOperations ops = Internal::mergeToolChainLists(system, user, autodetect);

        QSet<Toolchain *> expToRegister = Utils::toSet(toRegister);
        QSet<Toolchain *> expToDemote = Utils::toSet(toDemote);

        QSet<Toolchain *> actToRegister = Utils::toSet(ops.toRegister);
        QSet<Toolchain *> actToDemote = Utils::toSet(ops.toDemote);
        QSet<Toolchain *> actToDelete = Utils::toSet(ops.toDelete);

        QCOMPARE(actToRegister.count(), ops.toRegister.count()); // no dups!
        QCOMPARE(actToDemote.count(), ops.toDemote.count()); // no dups!
        QCOMPARE(actToDelete.count(), ops.toDelete.count()); // no dups!

        QSet<Toolchain *> tmp = actToRegister;
        tmp.intersect(actToDemote);
        QCOMPARE(tmp, actToDemote); // all toDemote are in toRegister

        tmp = actToRegister;
        tmp.intersect(actToDelete);
        QVERIFY(tmp.isEmpty()); // Nothing that needs to be registered is to be deleted

        tmp = actToRegister;
        tmp.unite(actToDelete);
        QCOMPARE(tmp, Utils::toSet(system + user + autodetect)); // All input is accounted for

        QCOMPARE(expToRegister, actToRegister);
        QCOMPARE(expToDemote, actToDemote);
        QCOMPARE(Utils::toSet(system + user + autodetect),
                 Utils::toSet(ops.toRegister + ops.toDemote + ops.toDelete));
    }

    // A device that is not reachable when the settings are saved, e.g. a Docker daemon
    // that has not started yet, must not cost the kits their compilers.
    void testKeepUnreachableDeviceToolchain()
    {
        class TestToolchainFactory : ToolchainFactory
        {
        public:
            TestToolchainFactory() {
                setSupportedToolchainType(TestToolChainType);
                setToolchainConstructor([] { return new TTC; });
            }
            std::unique_ptr<ToolchainConfigWidget> createConfigurationWidget(
                const ToolchainBundle &) const override
            {
                return nullptr;
            }
        };

        TestToolchainFactory factory;

        auto onDevice = new TTC("onDevice", false);
        onDevice->setCompilerCommand("docker://4711/usr/bin/gcc");
        onDevice->setDetectionSource(DetectionSource::FromSystem);

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ToolchainSettingsAccessor accessor;
        accessor.setBaseFilePath(FilePath::fromString(dir.filePath("toolchains.xml")));
        accessor.saveToolchains({onDevice});
        const Toolchains restored = accessor.restoreToolchains();

        const Toolchain * const found = Utils::findOrDefault(restored, [](const Toolchain *tc) {
            return tc->typeId() == TestToolChainType;
        });
        QVERIFY(found);
        QCOMPARE(found->id(), onDevice->id());
        QCOMPARE(found->compilerCommand(), onDevice->compilerCommand());

        qDeleteAll(Utils::filtered(restored, [](const Toolchain *tc) {
            return tc->typeId() != TestToolChainType;
        }));
    }

    // A toolchain whose factory is not registered yet must survive a save and come back
    // once the factory is there.
    void testDeferredToolchain()
    {
        class TestToolchainFactory : ToolchainFactory
        {
        public:
            TestToolchainFactory() {
                setSupportedToolchainType(TestToolChainType);
                setToolchainConstructor([] { return new TTC; });
            }
            std::unique_ptr<ToolchainConfigWidget> createConfigurationWidget(
                const ToolchainBundle &) const override
            {
                return nullptr;
            }
        };

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath file = FilePath::fromString(dir.filePath("toolchains.xml"));
        const auto hasId = [](const Toolchains &tcs, Id id) {
            return Utils::contains(tcs, Utils::equal(&Toolchain::id, id));
        };

        auto deferred = new TTC("deferred");
        {
            ToolchainSettingsAccessor accessor;
            accessor.setBaseFilePath(file);
            accessor.saveToolchains({deferred});
        }

        {
            ToolchainSettingsAccessor accessor;
            accessor.setBaseFilePath(file);
            const Toolchains restored = accessor.restoreToolchains();
            QVERIFY(!hasId(restored, deferred->id()));
            QVERIFY(accessor.isDeferred(deferred->id()));
            accessor.saveToolchains({});
            qDeleteAll(restored);
        }

        ToolchainSettingsAccessor accessor;
        accessor.setBaseFilePath(file);
        qDeleteAll(accessor.restoreToolchains());
        QVERIFY(accessor.isDeferred(deferred->id()));
        const QString longId = QLatin1String(TestToolChainType) + ':' + deferred->id().toString();
        QVERIFY(accessor.isDeferred(Id::fromString(longId)));

        TestToolchainFactory factory;
        const Toolchains recovered = accessor.retryDeferredToolchains();
        QVERIFY(hasId(recovered, deferred->id()));
        QVERIFY(!accessor.isDeferred(deferred->id()));
    }

    void cleanupTestCase()
    {
        qDeleteAll(TTC::toolChains());
    }
};

QObject *createToolchainSettingsTest()
{
    return new ToolchainSettingsTest;
}

#endif // WITH_TESTS

} // ProjectExplorer::Internal

#ifdef WITH_TESTS
#include <toolchainsettingsaccessor.moc>
#endif
