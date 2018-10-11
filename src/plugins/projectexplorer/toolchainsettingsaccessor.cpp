/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "toolchainsettingsaccessor.h"

#include "toolchain.h"

#include <coreplugin/icore.h>

#include <app/app_version.h>

#include <utils/algorithm.h>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------
// ToolChainSettingsUpgraders:
// --------------------------------------------------------------------

class ToolChainSettingsUpgraderV0 : public Utils::VersionUpgrader
{
    // Necessary to make Version 1 supported.
public:
    ToolChainSettingsUpgraderV0() : Utils::VersionUpgrader(0, "4.6") { }

    // NOOP
    QVariantMap upgrade(const QVariantMap &data) final { return data; }
};

// --------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------

static const char TOOLCHAIN_DATA_KEY[] = "ToolChain.";
static const char TOOLCHAIN_COUNT_KEY[] = "ToolChain.Count";
static const char TOOLCHAIN_FILENAME[] = "/toolchains.xml";

struct ToolChainOperations
{
    QList<ToolChain *> toDemote;
    QList<ToolChain *> toRegister;
    QList<ToolChain *> toDelete;
};

static QList<ToolChain *> autoDetectToolChains(const QList<ToolChain *> alreadyKnownTcs)
{
    QList<ToolChain *> result;
    for (ToolChainFactory *f : ToolChainFactory::allToolChainFactories())
        result.append(f->autoDetect(alreadyKnownTcs));

    // Remove invalid toolchains that might have sneaked in.
    return Utils::filtered(result, [](const ToolChain *tc) { return tc->isValid(); });
}

static QList<ToolChain *> makeUniqueByEqual(const QList<ToolChain *> &a)
{
    QList<ToolChain *> result;
    foreach (ToolChain *tc, a) {
        if (!Utils::contains(result, [tc](ToolChain *rtc) { return *tc == *rtc; }))
            result.append(tc);
    }
    return result;
}

static QList<ToolChain *> makeUniqueByPointerEqual(const QList<ToolChain *> &a)
{
    return QSet<ToolChain *>::fromList(a).toList();
}

static QList<ToolChain *> subtractById(const QList<ToolChain *> &a, const QList<ToolChain *> &b)
{
    return Utils::filtered(a, [&b](ToolChain *atc) {
                                  return !Utils::anyOf(b, Utils::equal(&ToolChain::id, atc->id()));
                              });
}

static bool containsByEqual(const QList<ToolChain *> &a, const ToolChain *atc)
{
    return Utils::anyOf(a, [atc](ToolChain *btc) { return *atc == *btc; });
}

static QList<ToolChain *> subtractByEqual(const QList<ToolChain *> &a, const QList<ToolChain *> &b)
{
    return Utils::filtered(a, [&b](ToolChain *atc) {
                                  return !Utils::anyOf(b, [atc](ToolChain *btc) { return *atc == *btc; });
                              });
}

static QList<ToolChain *> subtractByPointerEqual(const QList<ToolChain *> &a, const QList<ToolChain *> &b)
{
    return Utils::filtered(a, [&b](ToolChain *atc) { return !b.contains(atc); });
}

static QList<ToolChain *> stabilizeOrder(const QList<ToolChain *> &toRegister,
                                         const QList<ToolChain *> &userFileTcs)
{
    // Keep the toolchains in their position in the user toolchain file to minimize diff!
    QList<ToolChain *> result;
    result.reserve(toRegister.size());
    QList<ToolChain *> toHandle = toRegister;

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

static ToolChainOperations mergeToolChainLists(const QList<ToolChain *> &systemFileTcs,
                                               const QList<ToolChain *> &userFileTcs,
                                               const QList<ToolChain *> &autodetectedTcs)
{
    const QList<ToolChain *> uniqueUserFileTcs = makeUniqueByEqual(userFileTcs);
    QList<ToolChain *> manualUserFileTcs;
    QList<ToolChain *> autodetectedUserFileTcs;
    std::tie(autodetectedUserFileTcs, manualUserFileTcs)
            = Utils::partition(uniqueUserFileTcs, &ToolChain::isAutoDetected);
    const QList<ToolChain *> autodetectedUserTcs = subtractById(autodetectedUserFileTcs, systemFileTcs);

    // Calculate a set of Tcs that were detected before (and saved to userFile) and that
    // got re-detected again. Take the userTcs (to keep Ids) over the same in autodetectedTcs.
    QList<ToolChain *> redetectedUserTcs;
    QList<ToolChain *> notRedetectedUserTcs;
    std::tie(redetectedUserTcs, notRedetectedUserTcs)
            = Utils::partition(autodetectedUserTcs,
                               [&autodetectedTcs](ToolChain *tc) { return containsByEqual(autodetectedTcs, tc); });

    // Remove redetected tcs from autodetectedTcs:
    const QList<ToolChain *> newlyAutodetectedTcs
            = subtractByEqual(autodetectedTcs, redetectedUserTcs);

    const QList<ToolChain *> notRedetectedButValidUserTcs
            = Utils::filtered(notRedetectedUserTcs, &ToolChain::isValid);

    const QList<ToolChain *> validManualUserTcs
            = Utils::filtered(manualUserFileTcs, &ToolChain::isValid);

    ToolChainOperations result;
    result.toDemote = notRedetectedButValidUserTcs;
    result.toRegister = stabilizeOrder(systemFileTcs + validManualUserTcs + result.toDemote // manual TCs
                                       + redetectedUserTcs + newlyAutodetectedTcs, // auto TCs
                                       userFileTcs);

    result.toDelete = makeUniqueByPointerEqual(subtractByPointerEqual(systemFileTcs + userFileTcs + autodetectedTcs,
                                                                      result.toRegister));
    return result;
}

// --------------------------------------------------------------------
// ToolChainSettingsAccessor:
// --------------------------------------------------------------------

ToolChainSettingsAccessor::ToolChainSettingsAccessor() :
    UpgradingSettingsAccessor("QtCreatorToolChains",
                              QCoreApplication::translate("ProjectExplorer::ToolChainManager", "Tool Chains"),
                              Core::Constants::IDE_DISPLAY_NAME)
{
    setBaseFilePath(FileName::fromString(Core::ICore::userResourcePath() + TOOLCHAIN_FILENAME));

    addVersionUpgrader(std::make_unique<ToolChainSettingsUpgraderV0>());
}

QList<ToolChain *> ToolChainSettingsAccessor::restoreToolChains(QWidget *parent) const
{
    // read all tool chains from SDK
    const QList<ToolChain *> systemFileTcs
            = toolChains(restoreSettings(FileName::fromString(Core::ICore::installerResourcePath() + TOOLCHAIN_FILENAME),
                                         parent));

    // read all tool chains from user file.
    const QList<ToolChain *> userFileTcs = toolChains(restoreSettings(parent));

    // Autodetect: Pass autodetected toolchains from user file so the information can be reused:
    const QList<ToolChain *> autodetectedUserFileTcs
            = Utils::filtered(userFileTcs, &ToolChain::isAutoDetected);
    const QList<ToolChain *> autodetectedTcs = autoDetectToolChains(autodetectedUserFileTcs);

    // merge tool chains and register those that we need to keep:
    const ToolChainOperations ops = mergeToolChainLists(systemFileTcs, userFileTcs, autodetectedTcs);

    // Process ops:
    for (ToolChain *tc : ops.toDemote)
        tc->setDetection(ToolChain::ManualDetection);

    qDeleteAll(ops.toDelete);

    return ops.toRegister;
}

void ToolChainSettingsAccessor::saveToolChains(const QList<ToolChain *> &toolchains, QWidget *parent)
{
    QVariantMap data;

    int count = 0;
    for (const ToolChain *tc : toolchains) {
        if (!tc || !tc->isValid())
            continue;
        const QVariantMap tmp = tc->toMap();
        if (tmp.isEmpty())
            continue;
        data.insert(QString::fromLatin1(TOOLCHAIN_DATA_KEY) + QString::number(count), tmp);
        ++count;
    }
    data.insert(TOOLCHAIN_COUNT_KEY, count);

    // Do not save default debuggers! Those are set by the SDK!

    saveSettings(data, parent);
}

QList<ToolChain *> ToolChainSettingsAccessor::toolChains(const QVariantMap &data) const
{
    QList<ToolChain *> result;
    const QList<ToolChainFactory *> factories = ToolChainFactory::allToolChainFactories();

    const int count = data.value(TOOLCHAIN_COUNT_KEY, 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(TOOLCHAIN_DATA_KEY) + QString::number(i);
        if (!data.contains(key))
            break;

        const QVariantMap tcMap = data.value(key).toMap();

        bool restored = false;
        for (ToolChainFactory *f : factories) {
            if (f->canRestore(tcMap)) {
                if (ToolChain *tc = f->restore(tcMap)) {
                    result.append(tc);
                    restored = true;
                    break;
                }
            }
        }
        if (!restored)
            qWarning("Warning: Unable to restore compiler type '%s' for tool chain %s.",
                     qPrintable(ToolChainFactory::typeIdFromMap(tcMap).toString()),
                     qPrintable(QString::fromUtf8(ToolChainFactory::idFromMap(tcMap))));
    }

    return result;
}

} // namespace Internal
} // namespace ProjectExplorer

#ifdef WITH_TESTS
#include "projectexplorer.h"

#include "headerpath.h"

#include "abi.h"
#include "toolchainconfigwidget.h"

#include <QSet>
#include <QTest>

namespace ProjectExplorer {

using TCList = QList<ToolChain *>;

class TTC : public ToolChain
{
public:
    TTC(ToolChain::Detection d, const QByteArray &t, bool v = true) :
        ToolChain("TestToolChainType", d),
        token(t),
        m_valid(v)
    {
        m_toolChains.append(this);
        setLanguage(Constants::CXX_LANGUAGE_ID);
    }

    static QList<TTC *> toolChains();
    static bool hasToolChains() { return !m_toolChains.isEmpty(); }

    QString typeDisplayName() const override { return QString("Test Tool Chain"); }
    Abi targetAbi() const override { return Abi::hostAbi(); }
    bool isValid() const override { return m_valid; }
    MacroInspectionRunner createMacroInspectionRunner() const override { return MacroInspectionRunner(); }
    Macros predefinedMacros(const QStringList &cxxflags) const override { Q_UNUSED(cxxflags); return Macros(); }
    LanguageExtensions languageExtensions(const QStringList &cxxflags) const override { Q_UNUSED(cxxflags); return LanguageExtension::None; }
    WarningFlags warningFlags(const QStringList &cflags) const override { Q_UNUSED(cflags); return WarningFlags::NoWarnings; }
    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner() const override { return BuiltInHeaderPathsRunner(); }
    HeaderPaths builtInHeaderPaths(const QStringList &cxxflags, const FileName &sysRoot) const override
    { Q_UNUSED(cxxflags); Q_UNUSED(sysRoot); return {}; }
    void addToEnvironment(Environment &env) const override { Q_UNUSED(env); }
    QString makeCommand(const Environment &env) const override { Q_UNUSED(env); return QString("make"); }
    FileName compilerCommand() const override { return Utils::FileName::fromString("/tmp/test/gcc"); }
    IOutputParser *outputParser() const override { return nullptr; }
    std::unique_ptr<ToolChainConfigWidget> createConfigurationWidget() override { return nullptr; }
    TTC *clone() const override { return new TTC(*this); }
    bool operator ==(const ToolChain &other) const override {
        if (!ToolChain::operator==(other))
            return false;
        return static_cast<const TTC *>(&other)->token == token;
    }

    QByteArray token;

private:
    TTC(const TTC &other) :
        ToolChain(other.typeId(), other.detection()),
        token(other.token)
    {}

    bool m_valid = false;

    static QList<TTC *> m_toolChains;

public:
};

QList<TTC *> TTC::m_toolChains;

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::ToolChain *)

namespace ProjectExplorer {

void ProjectExplorerPlugin::testToolChainMerging_data()
{
    QTest::addColumn<TCList>("system");
    QTest::addColumn<TCList>("user");
    QTest::addColumn<TCList>("autodetect");
    QTest::addColumn<TCList>("toDemote");
    QTest::addColumn<TCList>("toRegister");

    TTC *system1 = nullptr;
    TTC *system1c = nullptr;
    TTC *system2 = nullptr;
    TTC *system3i = nullptr;
    TTC *user1 = nullptr;
    TTC *user1c = nullptr;
    TTC *user3i = nullptr;
    TTC *user2 = nullptr;
    TTC *auto1 = nullptr;
    TTC *auto1c = nullptr;
    TTC *auto1_2 = nullptr;
    TTC *auto2 = nullptr;
    TTC *auto3i = nullptr;

    if (!TTC::hasToolChains()) {
        system1 = new TTC(ToolChain::AutoDetection, "system1"); Q_UNUSED(system1);
        system1c = system1->clone(); Q_UNUSED(system1c);
        system2 = new TTC(ToolChain::AutoDetection, "system2"); Q_UNUSED(system2);
        system3i = new TTC(ToolChain::AutoDetection, "system3", false); Q_UNUSED(system3i);
        user1 = new TTC(ToolChain::ManualDetection, "user1"); Q_UNUSED(user1);
        user1c = user1->clone(); Q_UNUSED(user1c);
        user2 = new TTC(ToolChain::ManualDetection, "user2"); Q_UNUSED(user2);
        user3i = new TTC(ToolChain::ManualDetection, "user3", false); Q_UNUSED(user3i);
        auto1 = new TTC(ToolChain::AutoDetectionFromSettings, "auto1"); Q_UNUSED(auto1);
        auto1c = auto1->clone(); Q_UNUSED(auto1c);
        auto1_2 = new TTC(ToolChain::AutoDetectionFromSettings, "auto1"); Q_UNUSED(auto1_2);
        auto2 = new TTC(ToolChain::AutoDetectionFromSettings, "auto2"); Q_UNUSED(auto2);
        auto3i = new TTC(ToolChain::AutoDetectionFromSettings, "auto3", false); Q_UNUSED(auto3i);
    }

    QTest::newRow("no toolchains")
            << (TCList()) << (TCList()) << (TCList())
            << (TCList()) << (TCList());

    QTest::newRow("System: system, no user")
            << (TCList() << system1) << (TCList()) << (TCList())
            << (TCList()) << (TCList() << system1);
    QTest::newRow("System: system, user")
            << (TCList() << system1) << (TCList() << system1) << (TCList())
            << (TCList()) << (TCList() << system1);
    QTest::newRow("System: no system, user") // keep, the user tool chain as it is still found
            << (TCList()) << (TCList() << system1) << (TCList())
            << (TCList() << system1) << (TCList() << system1);
    QTest::newRow("System: no system, invalid user")
            << (TCList()) << (TCList() << system3i) << (TCList())
            << (TCList()) << (TCList());

    QTest::newRow("Auto: no auto, user")
            << (TCList()) << (TCList() << auto1) << (TCList())
            << (TCList() << auto1) << (TCList() << auto1);
    QTest::newRow("Auto: auto, no user")
            << (TCList()) << (TCList()) << (TCList() << auto1)
            << (TCList()) << (TCList() << auto1);
    QTest::newRow("Auto: auto, user")
            << (TCList()) << (TCList() << auto1) << (TCList() << auto1)
            << (TCList()) << (TCList() << auto1);
    QTest::newRow("Auto: auto-redetect, user")
            << (TCList()) << (TCList() << auto1) << (TCList() << auto1_2)
            << (TCList()) << (TCList() << auto1);
    QTest::newRow("Auto: auto-redetect, duplicate users")
            << (TCList()) << (TCList() << auto1 << auto1c) << (TCList() << auto1_2)
            << (TCList()) << (TCList() << auto1);
    QTest::newRow("Auto: (no) auto, invalid user")
            << (TCList()) << (TCList() << auto3i) << (TCList())
            << (TCList()) << (TCList());

    QTest::newRow("Delete invalid user")
            << (TCList()) << (TCList() << user3i) << (TCList())
            << (TCList()) << (TCList());

    QTest::newRow("one of everything")
            << (TCList() << system1) << (TCList() << user1) << (TCList() << auto1)
            << (TCList()) << (TCList() << system1 << user1 << auto1);
}

void ProjectExplorerPlugin::testToolChainMerging()
{
    QFETCH(TCList, system);
    QFETCH(TCList, user);
    QFETCH(TCList, autodetect);
    QFETCH(TCList, toRegister);
    QFETCH(TCList, toDemote);

    Internal::ToolChainOperations ops = Internal::mergeToolChainLists(system, user, autodetect);

    QSet<ToolChain *> expToRegister = QSet<ToolChain *>::fromList(toRegister);
    QSet<ToolChain *> expToDemote = QSet<ToolChain *>::fromList(toDemote);

    QSet<ToolChain *> actToRegister = QSet<ToolChain *>::fromList(ops.toRegister);
    QSet<ToolChain *> actToDemote = QSet<ToolChain *>::fromList(ops.toDemote);
    QSet<ToolChain *> actToDelete = QSet<ToolChain *>::fromList(ops.toDelete);

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
    QCOMPARE(tmp, QSet<ToolChain *>::fromList(system + user + autodetect)); // All input is accounted for

    QCOMPARE(expToRegister, actToRegister);
    QCOMPARE(expToDemote, actToDemote);
    QCOMPARE(QSet<ToolChain *>::fromList(system + user + autodetect),
             QSet<ToolChain *>::fromList(ops.toRegister + ops.toDemote + ops.toDelete));
}

} // namespace ProjectExplorer

#endif // WITH_TESTS

