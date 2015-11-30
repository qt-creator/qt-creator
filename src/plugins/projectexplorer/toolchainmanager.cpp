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

#include <tuple>

static const char TOOLCHAIN_DATA_KEY[] = "ToolChain.";
static const char TOOLCHAIN_COUNT_KEY[] = "ToolChain.Count";
static const char TOOLCHAIN_FILE_VERSION_KEY[] = "Version";
static const char TOOLCHAIN_FILENAME[] = "/qtcreator/toolchains.xml";

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

    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
            this, &ToolChainManager::saveToolChains);
    connect(this, &ToolChainManager::toolChainAdded, this, &ToolChainManager::toolChainsChanged);
    connect(this, &ToolChainManager::toolChainRemoved, this, &ToolChainManager::toolChainsChanged);
    connect(this, &ToolChainManager::toolChainUpdated, this, &ToolChainManager::toolChainsChanged);
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

static QList<ToolChain *> autoDetectToolChains(const QList<ToolChain *> alreadyKnownTcs)
{
    QList<ToolChain *> result;
    const QList<ToolChainFactory *> factories
            = ExtensionSystem::PluginManager::getObjects<ToolChainFactory>();
    foreach (ToolChainFactory *f, factories)
        result.append(f->autoDetect(alreadyKnownTcs));

    return result;
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

static QList<ToolChain *> makeUnique(const QList<ToolChain *> &a)
{
    return QSet<ToolChain *>::fromList(a).toList();
}

namespace {

struct ToolChainOperations
{
    QList<ToolChain *> toDemote;
    QList<ToolChain *> toRegister;
    QList<ToolChain *> toDelete;
};

} // namespace

static ToolChainOperations mergeToolChainLists(const QList<ToolChain *> &systemFileTcs,
                                               const QList<ToolChain *> &userFileTcs,
                                               const QList<ToolChain *> &autodetectedTcs)
{
    // Remove systemFileTcs from autodetectedUserFileTcs based on id-matches:
    QList<ToolChain *> manualUserFileTcs;
    QList<ToolChain *> autodetectedUserFileTcs;
    std::tie(autodetectedUserFileTcs, manualUserFileTcs)
            = Utils::partition(userFileTcs, &ToolChain::isAutoDetected);
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
    result.toRegister = systemFileTcs + validManualUserTcs + result.toDemote // manual TCs
            + redetectedUserTcs + newlyAutodetectedTcs; // auto TCs

    result.toDelete = makeUnique(subtractByPointerEqual(systemFileTcs + userFileTcs + autodetectedTcs,
                                                        result.toRegister));
    return result;
}

void ToolChainManager::restoreToolChains()
{
    QTC_ASSERT(!d->m_writer, return);
    d->m_writer =
            new PersistentSettingsWriter(settingsFileName(QLatin1String(TOOLCHAIN_FILENAME)),
                                         QLatin1String("QtCreatorToolChains"));

    // read all tool chains from SDK
    const QList<ToolChain *> systemFileTcs = readSystemFileToolChains();

    // read all tool chains from user file.
    const QList<ToolChain *> userFileTcs
            = restoreFromFile(settingsFileName(QLatin1String(TOOLCHAIN_FILENAME)));

    // Autodetect: Pass autodetected toolchains from user file so the information can be reused:
    const QList<ToolChain *> autodetectedUserFileTcs
            = Utils::filtered(userFileTcs, &ToolChain::isAutoDetected);
    const QList<ToolChain *> autodetectedTcs = autoDetectToolChains(autodetectedUserFileTcs);

    // merge tool chains and register those that we need to keep:
    ToolChainOperations ops = mergeToolChainLists(systemFileTcs, userFileTcs, autodetectedTcs);

    // Process ops:
    foreach (ToolChain *tc, ops.toDemote)
        tc->setDetection(ToolChain::ManualDetection);

    foreach (ToolChain *tc, ops.toRegister)
        registerToolChain(tc);

    qDeleteAll(ops.toDelete);

    emit m_instance->toolChainsLoaded();
}

QList<ToolChain *> ToolChainManager::readSystemFileToolChains()
{
    QFileInfo systemSettingsFile(Core::ICore::settings(QSettings::SystemScope)->fileName());
    QList<ToolChain *> systemTcs
            = restoreFromFile(FileName::fromString(systemSettingsFile.absolutePath() + QLatin1String(TOOLCHAIN_FILENAME)));

    foreach (ToolChain *tc, systemTcs)
        tc->setDetection(ToolChain::AutoDetection);

    return systemTcs;
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

#ifdef WITH_TESTS
#include "projectexplorer.h"

#include "headerpath.h"

#include <QSet>
#include <QTest>

namespace ProjectExplorer {

typedef QList<ToolChain *> TCList;

class TTC : public ToolChain
{
public:
    TTC(ToolChain::Detection d, const QByteArray &t, bool v = true) :
        ToolChain("TestToolChainType", d),
        token(t),
        m_valid(v)
    { m_toolChains.append(this); }

    static QList<TTC *> toolChains();
    static bool hasToolChains() { return !m_toolChains.isEmpty(); }

    QString typeDisplayName() const override { return QLatin1String("Test Tool Chain"); }
    Abi targetAbi() const override { return Abi::hostAbi(); }
    bool isValid() const override { return m_valid; }
    QByteArray predefinedMacros(const QStringList &cxxflags) const override { Q_UNUSED(cxxflags); return QByteArray(); }
    CompilerFlags compilerFlags(const QStringList &cxxflags) const override { Q_UNUSED(cxxflags); return NoFlags; }
    WarningFlags warningFlags(const QStringList &cflags) const override { Q_UNUSED(cflags); return 0; }
    QList<HeaderPath> systemHeaderPaths(const QStringList &cxxflags, const FileName &sysRoot) const override
    { Q_UNUSED(cxxflags); Q_UNUSED(sysRoot); return QList<HeaderPath>(); }
    void addToEnvironment(Environment &env) const override { Q_UNUSED(env); }
    QString makeCommand(const Environment &env) const override { Q_UNUSED(env); return QLatin1String("make"); }
    FileName compilerCommand() const override { return Utils::FileName::fromString(QLatin1String("/tmp/test/gcc")); }
    IOutputParser *outputParser() const override { return 0; }
    ToolChainConfigWidget *configurationWidget() override { return 0; }
    TTC *clone() const override { return new TTC(*this); }
    bool operator ==(const ToolChain &other) const override {
        if (!ToolChain::operator==(other))
            return false;
        return static_cast<const TTC *>(&other)->token == token;
    }

    QByteArray token;

private:
    TTC(const TTC &other) :
        ToolChain(other)
    { token = other.token; }

    bool m_valid;

    static QList<TTC *> m_toolChains;

    // ToolChain interface
public:
};

QList<TTC *> TTC::m_toolChains;

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::ToolChain *)

namespace ProjectExplorer {

void ProjectExplorerPlugin::testToolChainManager_data()
{
    QTest::addColumn<TCList>("system");
    QTest::addColumn<TCList>("user");
    QTest::addColumn<TCList>("autodetect");
    QTest::addColumn<TCList>("toDemote");
    QTest::addColumn<TCList>("toRegister");

    TTC *system1 = 0;
    TTC *system1c = 0;
    TTC *system2 = 0;
    TTC *system3i = 0;
    TTC *user1 = 0;
    TTC *user1c = 0;
    TTC *user3i = 0;
    TTC *user2 = 0;
    TTC *auto1 = 0;
    TTC *auto1c = 0;
    TTC *auto1_2 = 0;
    TTC *auto2 = 0;
    TTC *auto3i = 0;

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

void ProjectExplorerPlugin::testToolChainManager()
{
    QFETCH(TCList, system);
    QFETCH(TCList, user);
    QFETCH(TCList, autodetect);
    QFETCH(TCList, toRegister);
    QFETCH(TCList, toDemote);

    ToolChainOperations ops = mergeToolChainLists(system, user, autodetect);

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
