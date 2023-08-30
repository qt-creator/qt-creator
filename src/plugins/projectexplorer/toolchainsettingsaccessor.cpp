// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolchainsettingsaccessor.h"

#include "devicesupport/devicemanager.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "toolchain.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>

#include <nanotrace/nanotrace.h>

#include <QElapsedTimer>
#include <QGuiApplication>
#include <QLoggingCategory>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

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

static Toolchains autoDetectToolChains(const ToolchainDetector &detector)
{
    Toolchains result;
    for (ToolChainFactory *f : ToolChainFactory::allToolChainFactories()) {
        NANOTRACE_SCOPE_ARGS("ProjectExplorer",
                             "ToolChainSettingsAccessor::autoDetectToolChains",
                             {"factory", f->displayName().toStdString()});
        QElapsedTimer et;
        et.start();
        result.append(f->autoDetect(detector));
        qCDebug(Log) << f->displayName() << "auto detection took: " << et.elapsed() << "ms";
    }

    // Remove invalid toolchains that might have sneaked in.
    return Utils::filtered(result, [](const ToolChain *tc) { return tc->isValid(); });
}

static Toolchains makeUniqueByEqual(const Toolchains &a)
{
    Toolchains result;
    for (ToolChain *tc : a) {
        if (!Utils::contains(result, [tc](ToolChain *rtc) { return *tc == *rtc; }))
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
    return Utils::filtered(a, [&b](ToolChain *atc) {
                                  return !Utils::anyOf(b, Utils::equal(&ToolChain::id, atc->id()));
                              });
}

static bool containsByEqual(const Toolchains &a, const ToolChain *atc)
{
    return Utils::anyOf(a, [atc](ToolChain *btc) { return *atc == *btc; });
}

static Toolchains subtractByEqual(const Toolchains &a, const Toolchains &b)
{
    return Utils::filtered(a, [&b](ToolChain *atc) {
                                  return !Utils::anyOf(b, [atc](ToolChain *btc) { return *atc == *btc; });
                              });
}

static Toolchains subtractByPointerEqual(const Toolchains &a, const Toolchains &b)
{
    return Utils::filtered(a, [&b](ToolChain *atc) { return !b.contains(atc); });
}

static Toolchains stabilizeOrder(const Toolchains &toRegister,
                                         const Toolchains &userFileTcs)
{
    // Keep the toolchains in their position in the user toolchain file to minimize diff!
    Toolchains result;
    result.reserve(toRegister.size());
    Toolchains toHandle = toRegister;

    for (int i = 0; i < userFileTcs.count(); ++i) {
        const QByteArray userId = userFileTcs.at(i)->id();
        const int handlePos = Utils::indexOf(toHandle,
                                             [&userId](const ToolChain *htc) { return htc->id() == userId; });
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
            = Utils::partition(uniqueUserFileTcs, &ToolChain::isAutoDetected);
    const Toolchains autodetectedUserTcs = subtractById(autodetectedUserFileTcs, systemFileTcs);

    // Calculate a set of Tcs that were detected before (and saved to userFile) and that
    // got re-detected again. Take the userTcs (to keep Ids) over the same in autodetectedTcs.
    Toolchains redetectedUserTcs;
    Toolchains notRedetectedUserTcs;
    std::tie(redetectedUserTcs, notRedetectedUserTcs)
            = Utils::partition(autodetectedUserTcs,
                               [&autodetectedTcs](ToolChain *tc) { return containsByEqual(autodetectedTcs, tc); });

    // Remove redetected tcs from autodetectedTcs:
    const Toolchains newlyAutodetectedTcs
            = subtractByEqual(autodetectedTcs, redetectedUserTcs);

    const Toolchains notRedetectedButValidUserTcs
            = Utils::filtered(notRedetectedUserTcs, &ToolChain::isValid);

    ToolChainOperations result;
    result.toDemote = notRedetectedButValidUserTcs;
    result.toRegister = stabilizeOrder(systemFileTcs + manualUserFileTcs + result.toDemote // manual TCs
                                       + redetectedUserTcs + newlyAutodetectedTcs, // auto TCs
                                       userFileTcs);

    result.toDelete = makeUniqueByPointerEqual(subtractByPointerEqual(systemFileTcs + userFileTcs + autodetectedTcs,
                                                                      result.toRegister));
    return result;
}

// --------------------------------------------------------------------
// ToolChainSettingsAccessor:
// --------------------------------------------------------------------

ToolChainSettingsAccessor::ToolChainSettingsAccessor()
{
    setDocType("QtCreatorToolChains");
    setApplicationDisplayName(QGuiApplication::applicationDisplayName());
    setBaseFilePath(Core::ICore::userResourcePath(TOOLCHAIN_FILENAME));

    addVersionUpgrader(std::make_unique<ToolChainSettingsUpgraderV0>());
}

Toolchains ToolChainSettingsAccessor::restoreToolChains(QWidget *parent) const
{
    NANOTRACE_SCOPE("ProjectExplorer", "ToolChainSettingsAccessor::restoreToolChains");
    // read all tool chains from SDK
    const Toolchains systemFileTcs = toolChains(
        restoreSettings(Core::ICore::installerResourcePath(TOOLCHAIN_FILENAME), parent));
    for (ToolChain * const systemTc : systemFileTcs)
        systemTc->setDetection(ToolChain::AutoDetectionFromSdk);

    // read all tool chains from user file.
    const Toolchains userFileTcs = toolChains(restoreSettings(parent));

    // Autodetect: Pass autodetected toolchains from user file so the information can be reused:
    const Toolchains autodetectedUserFileTcs
            = Utils::filtered(userFileTcs, &ToolChain::isAutoDetected);

    // Autodect from system paths on the desktop device.
    // The restriction is intentional to keep startup and automatic validation a limited effort
    ToolchainDetector detector(autodetectedUserFileTcs, DeviceManager::defaultDesktopDevice(), {});
    const Toolchains autodetectedTcs = autoDetectToolChains(detector);

    // merge tool chains and register those that we need to keep:
    const ToolChainOperations ops = mergeToolChainLists(systemFileTcs, userFileTcs, autodetectedTcs);

    // Process ops:
    for (ToolChain *tc : ops.toDemote) {
        // FIXME: We currently only demote local toolchains, as they are not redetected.
        if (tc->detectionSource().isEmpty())
            tc->setDetection(ToolChain::ManualDetection);
    }

    qDeleteAll(ops.toDelete);

    return ops.toRegister;
}

void ToolChainSettingsAccessor::saveToolChains(const Toolchains &toolchains, QWidget *parent)
{
    Store data;

    int count = 0;
    for (const ToolChain *tc : toolchains) {
        if (!tc || (!tc->isValid() && tc->isAutoDetected()))
            continue;
        Store tmp;
        tc->toMap(tmp);
        if (tmp.isEmpty())
            continue;
        data.insert(numberedKey(TOOLCHAIN_DATA_KEY, count), variantFromStore(tmp));
        ++count;
    }
    data.insert(TOOLCHAIN_COUNT_KEY, count);

    // Do not save default debuggers! Those are set by the SDK!

    saveSettings(data, parent);
}

Toolchains ToolChainSettingsAccessor::toolChains(const Store &data) const
{
    Toolchains result;
    const QList<ToolChainFactory *> factories = ToolChainFactory::allToolChainFactories();

    const int count = data.value(TOOLCHAIN_COUNT_KEY, 0).toInt();
    for (int i = 0; i < count; ++i) {
        const Key key = numberedKey(TOOLCHAIN_DATA_KEY, i);
        if (!data.contains(key))
            break;

        const Store tcMap = storeFromVariant(data.value(key));

        bool restored = false;
        const Utils::Id tcType = ToolChainFactory::typeIdFromMap(tcMap);
        if (tcType.isValid()) {
            for (ToolChainFactory *f : factories) {
                if (f->supportedToolChainType() == tcType) {
                    if (ToolChain *tc = f->restore(tcMap)) {
                        result.append(tc);
                        restored = true;
                        break;
                    }
                }
            }
        }
        if (!restored)
            qWarning("Warning: Unable to restore compiler type '%s' for tool chain %s.",
                     qPrintable(tcType.toString()),
                     qPrintable(QString::fromUtf8(ToolChainFactory::idFromMap(tcMap))));
    }

    return result;
}

} // namespace Internal
} // namespace ProjectExplorer

#ifdef WITH_TESTS
#include "projectexplorer.h"

#include "abi.h"
#include "toolchainconfigwidget.h"

#include <QSet>
#include <QTest>

namespace ProjectExplorer {

const char TestTokenKey[] = "TestTokenKey";
const char TestToolChainType[] = "TestToolChainType";


class TTC : public ToolChain
{
public:
    TTC(const QByteArray &t = {}, bool v = true) :
        ToolChain(TestToolChainType),
        token(t),
        m_valid(v)
    {
        m_toolChains.append(this);
        setLanguage(Constants::CXX_LANGUAGE_ID);
        setTypeDisplayName("Test Tool Chain");
        setTargetAbiNoSignal(Abi::hostAbi());
        setCompilerCommand("/tmp/test/gcc");
    }

    static QList<TTC *> toolChains() { return m_toolChains; }
    static bool hasToolChains() { return !m_toolChains.isEmpty(); }

    bool isValid() const override { return m_valid; }
    MacroInspectionRunner createMacroInspectionRunner() const override { return {}; }
    LanguageExtensions languageExtensions(const QStringList &cxxflags) const override { Q_UNUSED(cxxflags) return LanguageExtension::None; }
    WarningFlags warningFlags(const QStringList &cflags) const override { Q_UNUSED(cflags) return WarningFlags::NoWarnings; }
    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner(const Utils::Environment &) const override { return {}; }
    void addToEnvironment(Environment &env) const override { Q_UNUSED(env) }
    FilePath makeCommand(const Environment &) const override { return "make"; }
    QList<OutputLineParser *> createOutputParsers() const override { return {}; }
    std::unique_ptr<ToolChainConfigWidget> createConfigurationWidget() override { return nullptr; }
    bool operator ==(const ToolChain &other) const override {
        if (!ToolChain::operator==(other))
            return false;
        return static_cast<const TTC *>(&other)->token == token;
    }

    void fromMap(const Store &data) final
    {
        ToolChain::fromMap(data);
        token = data.value(TestTokenKey).toByteArray();
    }

    void toMap(Store &data) const final
    {
        ToolChain::toMap(data);
        data[TestTokenKey] = token;
    }

    QByteArray token;

private:
    bool m_valid = false;

    static QList<TTC *> m_toolChains;
};

QList<TTC *> TTC::m_toolChains;

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::ToolChain *)

namespace ProjectExplorer {

void ProjectExplorerPlugin::testToolChainMerging_data()
{
    class TestToolChainFactory : ToolChainFactory
    {
    public:
        TestToolChainFactory() {
            setSupportedToolChainType(TestToolChainType);
            setToolchainConstructor([] { return new TTC; });
        }
    };

    TestToolChainFactory factory;

    QTest::addColumn<Toolchains>("system");
    QTest::addColumn<Toolchains>("user");
    QTest::addColumn<Toolchains>("autodetect");
    QTest::addColumn<Toolchains>("toDemote");
    QTest::addColumn<Toolchains>("toRegister");

    TTC *system1 = nullptr;
    ToolChain *system1c = nullptr;
    TTC *system2 = nullptr;
    TTC *system3i = nullptr;
    TTC *user1 = nullptr;
    ToolChain *user1c = nullptr;
    TTC *user3i = nullptr;
    TTC *user2 = nullptr;
    TTC *auto1 = nullptr;
    ToolChain *auto1c = nullptr;
    TTC *auto1_2 = nullptr;
    TTC *auto2 = nullptr;
    TTC *auto3i = nullptr;

    if (!TTC::hasToolChains()) {
        system1 = new TTC("system1");
        system1->setDetection(ToolChain::AutoDetection);
        system1c = system1->clone(); Q_UNUSED(system1c)
        system2 = new TTC("system2");
        system2->setDetection(ToolChain::AutoDetection);
        system3i = new TTC("system3", false);
        system3i->setDetection(ToolChain::AutoDetection);
        user1 = new TTC("user1");
        user1->setDetection(ToolChain::ManualDetection);
        user1c = user1->clone(); Q_UNUSED(user1c)
        user2 = new TTC("user2");
        user2->setDetection(ToolChain::ManualDetection);
        user3i = new TTC("user3", false);
        user3i->setDetection(ToolChain::ManualDetection);
        auto1 = new TTC("auto1");
        auto1->setDetection(ToolChain::AutoDetection);
        auto1c = auto1->clone();
        auto1_2 = new TTC("auto1");
        auto1_2->setDetection(ToolChain::AutoDetection);
        auto2 = new TTC("auto2");
        auto2->setDetection(ToolChain::AutoDetection);
        auto3i = new TTC("auto3", false);
        auto3i->setDetection(ToolChain::AutoDetection);
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

void ProjectExplorerPlugin::testToolChainMerging()
{
    QFETCH(Toolchains, system);
    QFETCH(Toolchains, user);
    QFETCH(Toolchains, autodetect);
    QFETCH(Toolchains, toRegister);
    QFETCH(Toolchains, toDemote);

    Internal::ToolChainOperations ops = Internal::mergeToolChainLists(system, user, autodetect);

    QSet<ToolChain *> expToRegister = Utils::toSet(toRegister);
    QSet<ToolChain *> expToDemote = Utils::toSet(toDemote);

    QSet<ToolChain *> actToRegister = Utils::toSet(ops.toRegister);
    QSet<ToolChain *> actToDemote = Utils::toSet(ops.toDemote);
    QSet<ToolChain *> actToDelete = Utils::toSet(ops.toDelete);

    QCOMPARE(actToRegister.count(), ops.toRegister.count()); // no dups!
    QCOMPARE(actToDemote.count(), ops.toDemote.count()); // no dups!
    QCOMPARE(actToDelete.count(), ops.toDelete.count()); // no dups!

    QSet<ToolChain *> tmp = actToRegister;
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

void ProjectExplorerPlugin::deleteTestToolchains()
{
    qDeleteAll(TTC::toolChains());
}

} // namespace ProjectExplorer

#endif // WITH_TESTS

