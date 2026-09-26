// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "javakitaspect_test.h"

#ifdef WITH_TESTS

#include "javaconstants.h"
#include "javakitaspect.h"
#include "jdkmanager.h"

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/environmentkitaspect.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitaspect.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/task.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/id.h>

#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <memory>
#include <optional>

using namespace ProjectExplorer;
using namespace Utils;

namespace Java::Internal {

// Enough of one for isJavaHome(): what it looks for is bin/java, and that it
// can be run.
static FilePath makeFakeJdk(const QTemporaryDir &dir, const QString &name)
{
    const FilePath home = FilePath::fromString(dir.path()).pathAppended(name);
    const FilePath bin = home.pathAppended("bin");
    if (!bin.ensureWritableDir())
        return {};
    for (const QString &tool : {QString("java"), QString("javac")}) {
        const FilePath path = bin.pathAppended(tool).withExecutableSuffix();
        if (!path.writeFileContents("#!/bin/sh\n"))
            return {};
        path.setPermissions(path.permissions() | QFile::ExeOwner | QFile::ExeUser);
    }
    return home;
}

QHash<Kit *, JdkOfAKit> theJdksOfTheKits()
{
    QHash<Kit *, JdkOfAKit> jdks;
    const Id id = JavaKitAspect::id();
    const Id mark = Constants::JAVA_HOME_OF_THE_MACHINE;
    for (Kit *k : KitManager::kits()) {
        jdks.insert(k, {k->hasValue(id), k->value(id), k->hasValue(mark), k->value(mark),
                        k->isSticky(id), k->isMutable(id)});
    }
    return jdks;
}

void putTheJdksOfTheKitsBack(const QHash<Kit *, JdkOfAKit> &jdks)
{
    const Id id = JavaKitAspect::id();
    for (Kit *k : KitManager::kits()) {
        const auto jdk = jdks.constFind(k);
        if (jdk == jdks.constEnd())
            continue;
        if (!jdk->hasValue)
            k->removeKey(id);
        else
            k->setValue(id, jdk->value);
        // After the JDK, which is what removeJavaHome() takes the mark with.
        if (!jdk->hasMark)
            k->removeKeySilently(Constants::JAVA_HOME_OF_THE_MACHINE);
        else
            k->setValueSilently(Constants::JAVA_HOME_OF_THE_MACHINE, jdk->mark);
        k->setSticky(id, jdk->sticky);
        k->setMutable(id, jdk->isMutable);
    }
}

// A device of its own, so that a path of the host is not one of its own.
class ForeignDevice final : public IDevice
{
public:
    ForeignDevice()
    {
        setupId(AutoDetected, Id::generate());
        setType("JavaKitAspectTest.ForeignType");
        setMachineType(Hardware);
        setOsType(HostOsInfo::hostOs());
        setDisplayType("JavaKitAspectTest");
    }

private:
    IDeviceWidget *createWidget() override { return nullptr; }
};

class JavaKitAspectTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // The looking at the machine is done on another thread, and until it
        // has come back a kit is given no JDK at all - which would have the
        // tests below hold without having asked anything.
        QTRY_VERIFY_WITH_TIMEOUT(JdkManager::jdksWereDetected(), 60000);

        QVERIFY(m_dir.isValid());
        m_jdk = makeFakeJdk(m_dir, "jdk-stored");
        QVERIFY(!m_jdk.isEmpty());
        m_offeredJdk = makeFakeJdk(m_dir, "jdk-offered");
        QVERIFY(!m_offeredJdk.isEmpty());

        // Only a JDK the list holds is one a kit can name, and a plugin that
        // offers one enters it there the same way.
        m_savedJdks = JdkManager::jdks();
        m_savedDefaultJdkId = JdkManager::chosenDefaultJdkId();
        m_savedJdksOfTheKits = theJdksOfTheKits();
        for (const FilePath &javaHome : {m_jdk, m_offeredJdk}) {
            const Jdk jdk = JdkManager::registerJavaHome(javaHome, DetectionSource::Manual);
            QVERIFY2(jdk.id.isValid(), qPrintable(javaHome.toUserOutput()));
        }
    }

    // The list goes back first: every change to it has the factory write a
    // JDK into every kit of the session, so what the kits named is put back
    // once the list they are brought in line with is the one they came from.
    void cleanupTestCase()
    {
        JdkManager::setJdks(m_savedJdks, m_savedDefaultJdkId);
        putTheJdksOfTheKitsBack(m_savedJdksOfTheKits);
    }

    void init() { m_savedProviders = javaHomeCandidateProviders(); }

    void cleanup()
    {
        javaHomeCandidateProviders() = m_savedProviders;
        if (m_kit) {
            KitManager::deregisterKit(m_kit);
            m_kit = nullptr;
        }
        if (m_device) {
            DeviceManager::removeDevice(m_device->id());
            m_device.reset();
        }
    }

    void testExportsTheJdkOfTheKit()
    {
        Kit *k = addKit();
        JavaKitAspect::setJavaHome(k, m_jdk);

        const Environment env = k->buildEnvironment();
        QCOMPARE(FilePath::fromUserInput(env.value("JAVA_HOME")), m_jdk);
        QVERIFY2(env.path().contains(m_jdk.pathAppended("bin")),
                 "the JDK's bin directory was not put on the PATH");
    }

    void testKeepsAJavaHomeTheKitSpellsOut()
    {
        Kit *k = addKit();
        JavaKitAspect::setJavaHome(k, m_jdk);
        EnvironmentChanges changes;
        changes.setItemsFromUser({{"JAVA_HOME", "/somewhere/else"}});
        EnvironmentKitAspect::setBuildEnvChanges(k, changes);

        const Environment env = k->buildEnvironment();
        QCOMPARE(env.value("JAVA_HOME"), QString("/somewhere/else"));
    }

    void testLetsAJavaHomeTheKitSwitchesOffStayOff()
    {
        Kit *k = addKit();
        JavaKitAspect::setJavaHome(k, m_jdk);
        EnvironmentItem disabled("JAVA_HOME", "/somewhere/else");
        disabled.operation = EnvironmentItem::SetDisabled;
        EnvironmentChanges changes;
        changes.setItemsFromUser({disabled});
        EnvironmentKitAspect::setBuildEnvChanges(k, changes);

        // A JAVA_HOME switched off in the environment of the kit is switched
        // off, this one included: that environment is applied after this
        // aspect, as it is after the toolchain, and it has the last word.
        const Environment env = k->buildEnvironment();
        QVERIFY2(env.value("JAVA_HOME").isEmpty(),
                 "the environment of the kit was overruled rather than applied over this");
    }

    void testKeepsTheJdkTheKitNamesOverAnOfferedOne()
    {
        Kit *k = addKit();
        JavaKitAspect::setJavaHome(k, m_jdk);
        offerJdk(m_offeredJdk);

        // The kit page shows what the kit names and lets the user change it,
        // so that is what the build gets: a plugin is asked where the kit
        // names nothing, not over a choice that has been made.
        const Environment env = k->buildEnvironment();
        QVERIFY2(FilePath::fromUserInput(env.value("JAVA_HOME")) == m_jdk,
                 "the build was sent to the JDK a plugin offers rather than the one the kit "
                 "names, so the field of the kit page has no effect on it");
    }

    void testTakesAnOfferedJdkWhereTheKitNamesNone()
    {
        // setup() runs while the kit is registered, so the provider has to be
        // there before that.
        offerJdk(m_offeredJdk);
        Kit *k = addKit();

        QCOMPARE(JavaKitAspect::javaHome(k), m_offeredJdk);
        const Environment env = k->buildEnvironment();
        QCOMPARE(FilePath::fromUserInput(env.value("JAVA_HOME")), m_offeredJdk);
    }

    // What the "None" of the list comes down to, and the only way a kit that
    // has nothing to do with Java keeps JAVA_HOME and the PATH out of its
    // environment: the key is there with nothing in it, so setup() does not
    // answer the question again on the next start.
    void testKeepsJavaOutOfAKitThatNamesNoJdk()
    {
        offerJdk(m_offeredJdk);
        Kit *k = addKit();
        k->setValue(JavaKitAspect::id(), {});

        QVERIFY2(k->hasValue(JavaKitAspect::id()),
                 "picking \"None\" left the question open rather than answering it");
        QVERIFY(JavaKitAspect::javaHome(k).isEmpty());

        const Environment env = k->buildEnvironment();
        const Environment before = BuildDeviceKitAspect::device(k)->systemEnvironment();
        QCOMPARE(env.value("JAVA_HOME"), before.value("JAVA_HOME"));
        QCOMPARE(env.path(), before.path());
    }

    // A kit set to "None" holds the key with nothing in it, while a kit that
    // has no key is one the default is written into. Putting the kits back
    // around a test must not turn the one into the other.
    void testPutsAKitThatNamesNoJdkBackAsOne()
    {
        Kit *k = addKit();
        k->setValue(JavaKitAspect::id(), {});
        QVERIFY(k->hasValue(JavaKitAspect::id()));

        const QHash<Kit *, JdkOfAKit> saved = theJdksOfTheKits();
        k->removeKey(JavaKitAspect::id());
        putTheJdksOfTheKitsBack(saved);

        QVERIFY2(k->hasValue(JavaKitAspect::id()),
                 "the \"None\" of the user came back as a kit that has yet to be given a JDK, "
                 "which the next change to the list writes the default into");
    }

    // Whether the JDK a kit holds is one it was given or one the user picked
    // is what decides whether a plugin that comes to speak for the kit takes
    // it over, so putting the kits back puts that back too.
    void testPutsBackWhoTheJdkOfAKitBelongsTo()
    {
        Kit *k = addKit();
        JavaKitAspect::setJavaHome(k, m_jdk);
        QVERIFY(!k->hasValue(Constants::JAVA_HOME_OF_THE_MACHINE));

        const QHash<Kit *, JdkOfAKit> saved = theJdksOfTheKits();
        k->setValueSilently(Constants::JAVA_HOME_OF_THE_MACHINE, k->value(JavaKitAspect::id()));
        putTheJdksOfTheKitsBack(saved);

        QVERIFY2(!k->hasValue(Constants::JAVA_HOME_OF_THE_MACHINE),
                 "a JDK the user picked came back marked as one the kit was given, which a "
                 "plugin speaking for that kit then takes away from them");
    }

    // A kit no plugin speaks for is given the default of the machine, so that
    // a Java project builds in a kit that was never set up for Java.
    void testGivesTheDefaultToAKitNoPluginSpeaksFor()
    {
        javaHomeCandidateProviders().append(
            [](const Kit *) -> std::optional<FilePaths> { return std::nullopt; });
        const Id savedDefault = makeTheDefault(m_jdk);
        const QScopeGuard restoreDefault(
            [savedDefault] { JdkManager::setJdks(JdkManager::jdks(), savedDefault); });

        Kit *k = addKit();

        QCOMPARE(JavaKitAspect::javaHome(k), m_jdk);
    }

    // A plugin that knows a kit answers for its JDK by naming none as much as
    // by naming one: the kits of Android take theirs from their toolchain,
    // which exports it as it is at the time, and the default of the machine
    // written into the kit would be exported over that.
    void testLeavesAKitThePluginThatKnowsItNamesNoJdkFor()
    {
        claimKitsWithoutNamingAJdk();
        const Id savedDefault = makeTheDefault(m_jdk);
        const QScopeGuard restoreDefault(
            [savedDefault] { JdkManager::setJdks(JdkManager::jdks(), savedDefault); });

        Kit *k = addKit();

        QVERIFY2(!k->hasValue(JavaKitAspect::id()),
                 "a kit whose JDK a plugin answers for was given the default of the machine, "
                 "which the build then uses over the one its toolchain exports");
        QVERIFY(JavaKitAspect::javaHome(k).isEmpty());
    }

    // The JDKs of the list are the ones of the host, and one of those names
    // nothing on the device a Docker or a remote kit builds on: the build
    // would go without it while the kit page showed it all the same.
    void testLeavesAKitThatBuildsOnAnotherDeviceWithoutAJdk()
    {
        m_device = IDevice::Ptr(new ForeignDevice);
        DeviceManager::addDevice(m_device);
        const Id savedDefault = makeTheDefault(m_jdk);
        const QScopeGuard restoreDefault(
            [savedDefault] { JdkManager::setJdks(JdkManager::jdks(), savedDefault); });

        const Id deviceId = m_device->id();
        m_kit = KitManager::registerKit([deviceId](Kit *k) {
            k->setUnexpandedDisplayName("JavaKitAspectForeignBuildTestKit");
            BuildDeviceKitAspect::setDeviceId(k, deviceId);
        });
        QVERIFY(m_kit);

        QVERIFY2(!m_kit->hasValue(JavaKitAspect::id()),
                 "a kit that builds on another device was given a JDK of the host, which its "
                 "build cannot reach and which the kit page shows all the same");
    }

    // A kit set up while the machine is still being looked at waits for the
    // answer: the list holds what the kits named until then, and the first of
    // those says nothing about which JDK the machine builds with.
    void testLeavesAKitWithoutAJdkUntilTheMachineHasBeenLookedAt()
    {
        const QList<Jdk> jdks = JdkManager::jdks();
        const Id savedDefault = JdkManager::chosenDefaultJdkId();
        const QScopeGuard restoreJdks(
            [jdks, savedDefault] { JdkManager::setJdks(jdks, savedDefault); });

        QSignalSpy lookedAt(JdkManager::instance(), &JdkManager::jdksChanged);
        JdkManager::restoreJdks();
        QVERIFY(!JdkManager::jdksWereDetected());

        Kit *k = addKit();
        QVERIFY2(!k->hasValue(JavaKitAspect::id()),
                 "a kit was set up with a JDK before the machine had been looked at, so what "
                 "it builds with is whichever JDK a kit happened to name first");

        QVERIFY(lookedAt.wait(60000));
        QVERIFY(JdkManager::jdksWereDetected());
        QVERIFY2(!JavaKitAspect::javaHome(k).isEmpty(),
                 "the kit was left with no JDK although the looking has come back with one");
    }

    // An entry outlives the directory it names, since one the user made is
    // theirs to remove. What a plugin offers has to be there all the same: a
    // kit given such an entry exports a JAVA_HOME that names nothing, and
    // nothing takes it off that kit again.
    void testPassesOverAnOfferedJdkThatIsNotThereAnyMore()
    {
        const FilePath gone = makeFakeJdk(m_dir, "jdk-that-goes");
        QVERIFY(!gone.isEmpty());
        QVERIFY(JdkManager::registerJavaHome(gone, DetectionSource::Manual).id.isValid());
        QVERIFY(gone.removeRecursively());

        offerJdk(gone);
        const Id savedDefault = makeTheDefault(m_jdk);
        const QScopeGuard restoreDefault(
            [savedDefault] { JdkManager::setJdks(JdkManager::jdks(), savedDefault); });

        Kit *k = addKit();

        QVERIFY2(JavaKitAspect::javaHome(k) != gone,
                 "the kit was sent to a JDK that is not on the machine, and exports a "
                 "JAVA_HOME that names nothing");
    }

    // What a kit is depends on its device type, which is picked after the
    // Kits page has made the kit and set it up. A plugin that comes to speak
    // for it answers for its JDK after all, the machine having answered only
    // for want of one that would.
    void testLetsGoOfTheDefaultOnceAPluginSpeaksForTheKit()
    {
        const Id savedDefault = makeTheDefault(m_jdk);
        const QScopeGuard restoreDefault(
            [savedDefault] { JdkManager::setJdks(JdkManager::jdks(), savedDefault); });

        Kit *k = addKit();
        QCOMPARE(JavaKitAspect::javaHome(k), m_jdk);

        claimKitsWithoutNamingAJdk();
        k->fix();

        QVERIFY2(!k->hasValue(JavaKitAspect::id()),
                 "a kit that was made before it was told what it is kept the JDK of the "
                 "machine, which its build then uses over the one its toolchain exports");
    }

    // An entry that stays the entry it was while coming to hold another
    // directory leaves the kits naming it where they are, so nothing of
    // theirs is written. What they build with has changed all the same, and
    // an environment built for them before is kept until they are told.
    void testTellsAKitWhenWhatItNamesComesToHoldAnotherJdk()
    {
        const Id savedDefault = JdkManager::chosenDefaultJdkId();
        const QList<Jdk> saved = JdkManager::jdks();
        const QScopeGuard restoreJdks(
            [saved, savedDefault] { JdkManager::setJdks(saved, savedDefault); });

        Kit *k = addKit();
        JavaKitAspect::setJavaHome(k, m_jdk);
        const Id named = Id::fromSetting(k->value(JavaKitAspect::id()));
        QVERIFY(named.isValid());

        const FilePath elsewhere = makeFakeJdk(m_dir, "jdk-elsewhere");
        QVERIFY(!elsewhere.isEmpty());
        QList<Jdk> jdks = JdkManager::jdks();
        for (Jdk &jdk : jdks) {
            if (jdk.id == named)
                jdk.javaHome = elsewhere;
        }

        QSignalSpy told(KitManager::instance(), &KitManager::kitUpdated);
        JdkManager::setJdks(jdks, savedDefault);

        QCOMPARE(JavaKitAspect::javaHome(k), elsewhere);
        const bool toldOfThisKit = Utils::anyOf(told, [k](const QList<QVariant> &call) {
            return call.first().value<Kit *>() == k;
        });
        QVERIFY2(toldOfThisKit,
                 "the kit was not told that what it names has come to hold another JDK, so a "
                 "build configuration of it goes on with the environment it built before");
    }

    // Two entries may well name one directory - cloning a row of the JDKs
    // page makes such a pair - and what the kit names is one of them.
    void testShowsTheJdkTheKitNamesRatherThanAnotherOfTheSameDirectory()
    {
        const Id savedDefault = JdkManager::chosenDefaultJdkId();
        const QList<Jdk> saved = JdkManager::jdks();
        const QScopeGuard restoreJdks(
            [saved, savedDefault] { JdkManager::setJdks(saved, savedDefault); });

        Jdk theirs;
        theirs.id = Id::generate();
        theirs.name = "Project JDK";
        theirs.javaHome = m_jdk;
        theirs.detectionSource = DetectionSource::Manual;
        JdkManager::setJdks(saved + QList<Jdk>{theirs}, savedDefault);

        Kit *k = addKit();
        k->setValue(JavaKitAspect::id(), theirs.id.toSetting());

        KitAspectFactory *factory = javaKitAspectFactory();
        QVERIFY(factory);
        const KitAspectFactory::ItemList shown = factory->toUserOutput(k);
        QCOMPARE(shown.size(), 1);
        QVERIFY2(shown.first().second.startsWith(theirs.name),
                 qPrintable("the kit is shown with another entry for the same directory: "
                            + shown.first().second));
    }

    // A kit is set up with the default of the machine where nothing else
    // answers for it, and keeps it: what a kit builds with is this field's to
    // say, not a later turn of the list. The default answers for the kits
    // that have yet to be given one.
    void testKeepsTheJdkAKitWasSetUpWithWhenTheDefaultMoves()
    {
        const Id savedDefault = makeTheDefault(m_jdk);
        const QScopeGuard restoreDefault(
            [savedDefault] { JdkManager::setJdks(JdkManager::jdks(), savedDefault); });

        Kit *k = addKit();
        QCOMPARE(JavaKitAspect::javaHome(k), m_jdk);

        JdkManager::setJdks(JdkManager::jdks(), JdkManager::jdkForJavaHome(m_offeredJdk).id);

        QCOMPARE(JavaKitAspect::javaHome(k), m_jdk);
    }

    // A repair takes the JDK out of the kit before putting another in, and
    // between the two the kit has none. A kit told of each step builds the
    // environment of every project on it without a JAVA_HOME and then builds
    // it again, so it is told once, of where it ends up.
    void testTellsAKitOfARepairOnce()
    {
        const Id savedDefault = JdkManager::chosenDefaultJdkId();
        const QList<Jdk> jdks = JdkManager::jdks();
        const QScopeGuard restoreJdks(
            [jdks, savedDefault] { JdkManager::setJdks(jdks, savedDefault); });

        Kit *k = addKit();
        JavaKitAspect::setJavaHome(k, m_jdk);
        k->setSticky(JavaKitAspect::id(), true);

        QSignalSpy told(KitManager::instance(), &KitManager::kitUpdated);
        takeOutOfTheList(m_jdk, m_offeredJdk);

        const int ofThisKit = Utils::count(told, [k](const QList<QVariant> &call) {
            return call.first().value<Kit *>() == k;
        });
        QCOMPARE(JavaKitAspect::javaHome(k), m_offeredJdk);
        QCOMPARE(ofThisKit, 1);
    }

    // Writing the JDK is what tells the kit, and the Kits page takes a copy
    // of it there and then. A copy that holds the JDK without the mark is a
    // kit that looks as though the user had picked it.
    void testMarksTheJdkOfTheMachineBeforeTellingTheKit()
    {
        const Id savedDefault = makeTheDefault(m_jdk);
        const QScopeGuard restoreDefault(
            [savedDefault] { JdkManager::setJdks(JdkManager::jdks(), savedDefault); });

        Kit *k = addKit();
        k->removeKey(JavaKitAspect::id());
        QVERIFY(!k->hasValue(JavaKitAspect::id()));

        QVariant markWhenTold;
        const QMetaObject::Connection told
            = connect(KitManager::instance(), &KitManager::kitUpdated, [&](Kit *updated) {
                  if (updated == k && markWhenTold.isNull())
                      markWhenTold = k->value("Java.JavaHomeOfTheMachine");
              });
        const QScopeGuard forget([told] { disconnect(told); });

        JdkManager::setJdks(JdkManager::jdks(),
                            JdkManager::jdkForJavaHome(m_offeredJdk).id);

        QCOMPARE(JavaKitAspect::javaHome(k), m_offeredJdk);
        QCOMPARE(markWhenTold, k->value(JavaKitAspect::id()));
    }

    // What the kit page shows is a copy of the kit, which the repair the
    // factory does on the kits of the manager does not reach: the field of the
    // page is what brings the copy in line, so that is what this goes through
    // rather than Kit::fix().
    void testGivesTheKitOfThePageAnotherJdkForOneThatIsGone()
    {
        const Id savedDefault = JdkManager::chosenDefaultJdkId();
        const QList<Jdk> jdks = JdkManager::jdks();
        const QScopeGuard restoreJdks(
            [jdks, savedDefault] { JdkManager::setJdks(jdks, savedDefault); });

        Kit copy;
        JavaKitAspect::setJavaHome(&copy, m_jdk);
        copy.setSticky(JavaKitAspect::id(), true);

        KitAspectFactory *factory = javaKitAspectFactory();
        QVERIFY(factory);
        const std::unique_ptr<KitAspect> field(factory->createKitAspect(&copy));
        QVERIFY(field);

        takeOutOfTheList(m_jdk, m_offeredJdk);

        QVERIFY2(copy.hasValue(JavaKitAspect::id()),
                 "the copy was left naming no JDK, which is a choice of the user's and lasts");
        QCOMPARE(JavaKitAspect::javaHome(&copy), m_offeredJdk);
        QVERIFY2(copy.isSticky(JavaKitAspect::id()),
                 "the field of a kit whose JDK a plugin writes was opened for editing");
    }

    // The kits of the manager are brought in line by the factory, with nothing
    // asking them to: a kit naming a JDK that is gone would export a JAVA_HOME
    // that is not there.
    void testGivesAKitOfTheManagerAnotherJdkForOneThatIsGone()
    {
        const Id savedDefault = JdkManager::chosenDefaultJdkId();
        const QList<Jdk> jdks = JdkManager::jdks();
        const QScopeGuard restoreJdks(
            [jdks, savedDefault] { JdkManager::setJdks(jdks, savedDefault); });

        Kit *k = addKit();
        JavaKitAspect::setJavaHome(k, m_jdk);

        takeOutOfTheList(m_jdk, m_offeredJdk);

        QCOMPARE(JavaKitAspect::javaHome(k), m_offeredJdk);
    }

    // A kit that names no JDK is given one as soon as the machine has one,
    // which is what a kit set up where there was none waits for.
    void testFillsAKitThatNamesNoJdkOnceThereIsOne()
    {
        const Id savedDefault = makeTheDefault(m_jdk);
        const QScopeGuard restoreDefault(
            [savedDefault] { JdkManager::setJdks(JdkManager::jdks(), savedDefault); });

        Kit *k = addKit();
        k->removeKey(JavaKitAspect::id());
        QVERIFY(!k->hasValue(JavaKitAspect::id()));

        JdkManager::setJdks(JdkManager::jdks(), JdkManager::jdkForJavaHome(m_offeredJdk).id);

        QCOMPARE(JavaKitAspect::javaHome(k), m_offeredJdk);
    }

    void testLeavesTheEnvironmentOfAnotherDeviceAlone()
    {
        m_device = IDevice::Ptr(new ForeignDevice);
        DeviceManager::addDevice(m_device);

        Kit *k = addKit();
        JavaKitAspect::setJavaHome(k, m_jdk);
        BuildDeviceKitAspect::setDeviceId(k, m_device->id());

        // The JDK is on this machine, and JAVA_HOME and PATH are read on the
        // one the environment belongs to, where it names nothing.
        const Environment env = k->buildEnvironment();
        QVERIFY2(FilePath::fromUserInput(env.value("JAVA_HOME")) != m_jdk,
                 "a JDK of the host was written into the environment of another device");
        QVERIFY2(!env.path().contains(m_jdk.pathAppended("bin")),
                 "a bin directory of the host was put on the PATH of another device");
    }

private:
    void offerJdk(const FilePath &javaHome)
    {
        javaHomeCandidateProviders().append([javaHome](const Kit *) -> std::optional<FilePaths> {
            return FilePaths{javaHome};
        });
    }

    // What a plugin answers for a kit whose JDK is not the list's to choose:
    // it speaks for the kit, and names none.
    void claimKitsWithoutNamingAJdk()
    {
        javaHomeCandidateProviders().append(
            [](const Kit *) -> std::optional<FilePaths> { return FilePaths(); });
    }

    // Sets the default JDK of the machine, so that a test can tell a kit that
    // was given it from one that was left alone. Returns what it was.
    Id makeTheDefault(const FilePath &javaHome)
    {
        const Id saved = JdkManager::chosenDefaultJdkId();
        JdkManager::setJdks(JdkManager::jdks(), JdkManager::jdkForJavaHome(javaHome).id);
        return saved;
    }

    // Takes \a javaHome out of the list of the JDKs of the machine, leaving
    // \a remaining as its default.
    void takeOutOfTheList(const FilePath &javaHome, const FilePath &remaining)
    {
        const QList<Jdk> jdks = Utils::filtered(JdkManager::jdks(), [&javaHome](const Jdk &jdk) {
            return jdk.javaHome != javaHome;
        });
        JdkManager::setJdks(jdks, JdkManager::jdkForJavaHome(remaining).id);
    }

    KitAspectFactory *javaKitAspectFactory() const
    {
        return Utils::findOrDefault(KitAspectFactory::kitAspectFactories(),
                                    [](const KitAspectFactory *factory) {
                                        return factory->id() == JavaKitAspect::id();
                                    });
    }

    Kit *addKit()
    {
        m_kit = KitManager::registerKit([](Kit *k) {
            k->setUnexpandedDisplayName("JavaKitAspectTestKit");
        });
        return m_kit;
    }

    QTemporaryDir m_dir;
    FilePath m_jdk;
    FilePath m_offeredJdk;
    Kit *m_kit = nullptr;
    IDevice::Ptr m_device;
    QList<JavaHomeCandidateProvider> m_savedProviders;
    QList<Jdk> m_savedJdks;
    Id m_savedDefaultJdkId;
    QHash<Kit *, JdkOfAKit> m_savedJdksOfTheKits;
};

QObject *createJavaKitAspectTest()
{
    return new JavaKitAspectTest;
}

} // namespace Java::Internal

#include "javakitaspect_test.moc"

#endif // WITH_TESTS
