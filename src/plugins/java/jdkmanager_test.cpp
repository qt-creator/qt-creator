// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jdkmanager_test.h"

#ifdef WITH_TESTS

#include "javakitaspect_test.h"
#include "jdkmanager.h"

#include <projectexplorer/kit.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/id.h>

#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>

using namespace ProjectExplorer;
using namespace Utils;

namespace Java::Internal {

static bool makeRunnable(const FilePath &path)
{
    if (!path.writeFileContents("#!/bin/sh\n"))
        return false;
    return bool(path.setPermissions(path.permissions() | QFile::ExeOwner | QFile::ExeUser));
}

// A directory that holds what the detection reads a JDK off: bin/java, the
// bin/javac that tells a development kit from a runtime environment, and the
// "release" file a JDK describes itself in. One made without a version has no
// such file, which is what a directory a package manager links into the real
// JDK looks like.
static FilePath makeJdk(const FilePath &parent, const QString &name, const QString &version = {})
{
    const FilePath home = parent / name;
    const FilePath bin = home / "bin";
    if (!bin.ensureWritableDir())
        return {};
    for (const QString &tool : {QString("java"), QString("javac")}) {
        if (!makeRunnable((bin / tool).withExecutableSuffix()))
            return {};
    }
    if (!version.isEmpty()) {
        const QString release = QString("JAVA_VERSION=\"%1\"\nIMPLEMENTOR=\"Test\"\n").arg(version);
        if (!(home / "release").writeFileContents(release.toUtf8()))
            return {};
    }
    return home;
}

class JdkManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        m_savedJdks = JdkManager::jdks();
        m_savedDefaultId = JdkManager::chosenDefaultJdkId();
        // Every change to the list has the JavaKitAspect factory bring every
        // kit of the session in line with it, so what the kits of whoever
        // started this named is put back along with the list.
        m_savedJdksOfTheKits = theJdksOfTheKits();
    }

    void cleanupTestCase() { putTheListBack(); }

    void init() { m_savedProviders = jdkProviders(); }

    void cleanup()
    {
        jdkProviders() = m_savedProviders;
        putTheListBack();
    }

    // What a JDK says about itself is in a file, and a directory that holds a
    // java while saying nothing is not yet a JDK: macOS puts a stub in
    // /usr/bin, and a package manager puts one into a directory of its own
    // that links into the JDK behind it.
    void testTakesADirectoryThatDescribesItselfForAJdk()
    {
        const FilePath root = directoryFor("describing");
        const FilePath jdk = makeJdk(root, "jdk-21", "21.0.1");
        const FilePath mute = makeJdk(root, "jdk-mute");
        QVERIFY(!jdk.isEmpty());
        QVERIFY(!mute.isEmpty());

        offer({jdk, mute});
        const QList<Jdk> detected = JdkManager::detectJdks();

        QVERIFY2(hasJavaHome(detected, jdk), "a directory holding a release file was passed over");
        QVERIFY2(!hasJavaHome(detected, mute),
                 "a directory that holds a java but describes no JDK was taken for one");
    }

    // The java of such a directory points into the JDK it belongs to, and
    // that one is what JAVA_HOME is to name.
    void testFollowsAJavaThatPointsIntoTheJdkBehindIt()
    {
        if (HostOsInfo::isWindowsHost())
            QSKIP("a link to the java of another directory is not how Windows installs a JDK");

        const FilePath root = directoryFor("following");
        const FilePath jdk = makeJdk(root, "jdk-21", "21.0.1");
        QVERIFY(!jdk.isEmpty());
        const FilePath wrapper = root / "wrapper";
        QVERIFY((wrapper / "bin").ensureWritableDir());
        QVERIFY(javaExecutableIn(jdk).createSymLink(javaExecutableIn(wrapper)));

        offer({wrapper});
        const QList<Jdk> detected = JdkManager::detectJdks();

        QVERIFY2(hasJavaHome(detected, jdk),
                 "the JDK the java of the directory points into was not the one entered");
        QVERIFY2(!hasJavaHome(detected, wrapper),
                 "a directory that only holds a java was entered as the JDK itself");
    }

    // One JDK that two directories name is one JDK: a distribution and a
    // version manager each name it from one of their own.
    void testEntersAJdkFoundUnderTwoNamesOnce()
    {
        if (HostOsInfo::isWindowsHost())
            QSKIP("a second name for a directory is not how Windows installs a JDK");

        const FilePath root = directoryFor("two-names");
        const FilePath jdk = makeJdk(root, "jdk-21", "21.0.1");
        QVERIFY(!jdk.isEmpty());
        const FilePath alias = root / "current";
        QVERIFY(jdk.createSymLink(alias));

        offer({jdk, alias});
        const QList<Jdk> detected = JdkManager::detectJdks();

        QCOMPARE(countOf(detected, jdk), 1);
    }

    // The machine builds with the newest JDK it has, so that is the one a kit
    // that has yet to be given one is set up with.
    void testPutsTheNewestJdkFirst()
    {
        const FilePath root = directoryFor("newest");
        const FilePath older = makeJdk(root, "jdk-17", "17.0.20");
        const FilePath newer = makeJdk(root, "jdk-21", "21");
        const FilePath newest = makeJdk(root, "jdk-21-later", "21.0.1");
        QVERIFY(!older.isEmpty());
        QVERIFY(!newer.isEmpty());
        QVERIFY(!newest.isEmpty());

        offer({older, newest, newer});
        const QList<Jdk> detected = JdkManager::detectJdks();

        QVERIFY(indexOfJavaHome(detected, newest) < indexOfJavaHome(detected, newer));
        QVERIFY(indexOfJavaHome(detected, newer) < indexOfJavaHome(detected, older));
    }

    // A JDK that cannot build is a poor one to set a kit up with, so the
    // default passes over a runtime environment where there is a kit to be
    // had.
    void testTakesAJdkThatCanBuildAsTheDefault()
    {
        const FilePath root = directoryFor("default");
        const FilePath runtime = makeJdk(root, "jre-21", "21.0.1");
        const FilePath kit = makeJdk(root, "jdk-21", "21.0.1");
        QVERIFY(!runtime.isEmpty());
        QVERIFY(!kit.isEmpty());
        QVERIFY(javaCompilerIn(runtime).removeFile());

        JdkManager::setJdks({jdkFor(runtime, DetectionSource::FromSystem),
                             jdkFor(kit, DetectionSource::FromSystem)},
                            Id());

        QCOMPARE(JdkManager::defaultJdk().javaHome, kit);
    }

    // A package manager installs its JDKs under a link of its own and points
    // that link at the newest of them. The entry is the same one - the kits
    // name it, and what it leads to is still where they build - but what it
    // is called and what it holds are the JDK the link points at now.
    void testFollowsALinkThatIsPointedAtAnotherJdk()
    {
        if (HostOsInfo::isWindowsHost())
            QSKIP("a link that a package manager re-points is not how Windows installs a JDK");

        const FilePath root = directoryFor("re-pointed");
        const FilePath older = makeJdk(root, "jdk-17", "17.0.20");
        const FilePath newer = makeJdk(root, "jdk-21", "21.0.1");
        QVERIFY(!older.isEmpty());
        QVERIFY(!newer.isEmpty());
        const FilePath link = root / "current";
        QVERIFY(older.createSymLink(link));

        offer({link});
        JdkManager::setJdks({}, Id());
        JdkManager::enterDetectedJdks(JdkManager::detectJdks());
        const Jdk asItWas = JdkManager::jdkForJavaHome(link);
        QVERIFY(asItWas.id.isValid());
        QVERIFY2(asItWas.name.contains("17.0.20"), qPrintable(asItWas.name));

        QVERIFY(link.removeFile());
        QVERIFY(newer.createSymLink(link));
        JdkManager::enterDetectedJdks(JdkManager::detectJdks());

        const Jdk asItIs = JdkManager::jdk(asItWas.id);
        QVERIFY2(asItIs.id.isValid(),
                 "the entry was replaced rather than followed, so every kit naming it was sent "
                 "to another JDK");
        QVERIFY2(asItIs.name.contains("21.0.1"),
                 qPrintable("the entry still goes by the name of the JDK the link pointed at "
                            "before: " + asItIs.name));
        QCOMPARE(countOf(JdkManager::jdks(), link), 1);
    }

    // A plugin that speaks for a kit hands the JDK of that kit over while the
    // kits are restored, which is before the looking at the machine comes
    // back. One the machine turns out to have is one of the machine's from
    // then on, and goes the way of the rest once it is uninstalled.
    void testTakesOverAJdkAKitNamedBeforeTheLookingCameBack()
    {
        const FilePath root = directoryFor("handed-over");
        const FilePath jdk = makeJdk(root, "jdk-21", "21.0.1");
        QVERIFY(!jdk.isEmpty());

        JdkManager::setJdks({}, Id());
        const Jdk ofAKit = JdkManager::registerJavaHome(jdk,
                                                        {DetectionSource::Manual,
                                                         JDK_NAMED_BY_A_KIT});
        QVERIFY(ofAKit.id.isValid());
        QVERIFY(!ofAKit.detectionSource.isAutoDetected());

        JdkManager::enterDetectedJdks({jdkFor(jdk, DetectionSource::FromSystem)});

        const Jdk asItIs = JdkManager::jdk(ofAKit.id);
        QVERIFY2(asItIs.detectionSource.isAutoDetected(),
                 "a JDK of the machine that a kit happened to name first is kept among the "
                 "ones the user entered, which a re-detection never takes away again");
        QCOMPARE(asItIs.javaHome, jdk);

        // And is gone with the machine, as one that was found there would be,
        // rather than kept the way an entry the user made is.
        QVERIFY(jdk.removeRecursively());
        JdkManager::enterDetectedJdks({});

        QVERIFY2(!JdkManager::jdk(ofAKit.id).id.isValid(),
                 "the entry outlived the JDK, so the kits naming it export a JAVA_HOME that "
                 "is not there");
    }

    // A JDK that is in the list under one of its names is in it: a plugin
    // that names it by the directory it was configured with must not put a
    // second entry for it there.
    void testFindsAJdkOfTheListUnderAnotherOfItsNames()
    {
        if (HostOsInfo::isWindowsHost())
            QSKIP("a second name for a directory is not how Windows installs a JDK");

        const FilePath root = directoryFor("naming");
        const FilePath jdk = makeJdk(root, "jdk-21", "21.0.1");
        QVERIFY(!jdk.isEmpty());
        const FilePath alias = root / "current";
        QVERIFY(jdk.createSymLink(alias));

        const Jdk stored = jdkFor(alias, DetectionSource::FromSystem);
        JdkManager::setJdks({stored}, Id());

        const Jdk registered = JdkManager::registerJavaHome(jdk, DetectionSource::Manual);

        QCOMPARE(registered.id, stored.id);
        QCOMPARE(JdkManager::jdks().size(), 1);
    }

    // The order the machine has its JDKs in is what decides the default, and
    // a JDK a kit named while the looking was still going on is no answer to
    // which one the machine builds with.
    void testLetsTheMachineDecideTheDefaultOverAJdkAKitNamed()
    {
        const FilePath root = directoryFor("deciding");
        const FilePath older = makeJdk(root, "jdk-17", "17.0.20");
        const FilePath newer = makeJdk(root, "jdk-21", "21.0.1");
        QVERIFY(!older.isEmpty());
        QVERIFY(!newer.isEmpty());

        JdkManager::setJdks({}, Id());
        QVERIFY(JdkManager::registerJavaHome(older, DetectionSource::Manual).id.isValid());
        QCOMPARE(JdkManager::defaultJdk().javaHome, older);

        JdkManager::enterDetectedJdks({jdkFor(newer, DetectionSource::FromSystem),
                                       jdkFor(older, DetectionSource::FromSystem)});

        QCOMPARE(JdkManager::defaultJdk().javaHome, newer);
    }

    // What the user picked is theirs, and a detection that comes in after
    // does not pick another one over it.
    void testKeepsTheDefaultTheUserPicked()
    {
        const FilePath root = directoryFor("picked");
        const FilePath older = makeJdk(root, "jdk-17", "17.0.20");
        const FilePath newer = makeJdk(root, "jdk-21", "21.0.1");
        QVERIFY(!older.isEmpty());
        QVERIFY(!newer.isEmpty());

        const Jdk theirs = jdkFor(older, DetectionSource::FromSystem);
        JdkManager::setJdks({theirs}, theirs.id);

        JdkManager::enterDetectedJdks({jdkFor(newer, DetectionSource::FromSystem),
                                       jdkFor(older, DetectionSource::FromSystem)});

        QCOMPARE(JdkManager::defaultJdk().javaHome, older);
    }

    // An entry the user made for a directory the machine has as well - what
    // cloning a detected row on the page comes to - is theirs, and a kit may
    // well name it. The detection finding that directory is no reason to take
    // it from them.
    void testKeepsAnEntryOfTheUserForADirectoryTheMachineHasToo()
    {
        const FilePath root = directoryFor("entered-too");
        const FilePath jdk = makeJdk(root, "jdk-21", "21.0.1");
        QVERIFY(!jdk.isEmpty());

        const Jdk found = jdkFor(jdk, DetectionSource::FromSystem);
        const Jdk theirs = jdkFor(jdk, DetectionSource::Manual);
        JdkManager::setJdks({found, theirs}, theirs.id);

        JdkManager::enterDetectedJdks({jdkFor(jdk, DetectionSource::FromSystem)});

        const QList<Id> ids = Utils::transform(JdkManager::jdks(), &Jdk::id);
        QVERIFY2(ids.contains(theirs.id),
                 "the entry the user made was dropped over the machine having that directory, "
                 "and a kit naming it is sent to another JDK");
        QVERIFY(ids.contains(found.id));
        QCOMPARE(JdkManager::defaultJdk().id, theirs.id);
    }

    // A kit that builds on another device puts the JDK of that device into
    // the list. It is no answer to which JDK this machine builds with, and
    // asking after it is a round trip to a device that may not be reachable.
    void testPassesOverAJdkOfAnotherDeviceWhenAnsweringForTheMachine()
    {
        const FilePath root = directoryFor("of-a-device");
        // A runtime environment, so that the answer falls back on the first
        // of the list rather than on the first that can build.
        const FilePath ofTheMachine = makeJdk(root, "jre-21", "21.0.1");
        QVERIFY(!ofTheMachine.isEmpty());
        QVERIFY(javaCompilerIn(ofTheMachine).removeFile());
        const FilePath ofADevice = FilePath::fromParts(u"docker", u"container", u"/opt/jdk-21");

        JdkManager::setJdks({jdkFor(ofADevice, DetectionSource::Manual),
                             jdkFor(ofTheMachine, DetectionSource::FromSystem)},
                            Id());

        QVERIFY2(JdkManager::defaultJdk().javaHome == ofTheMachine,
                 "the JDK of another device was taken for the one this machine builds with, "
                 "which no kit that builds here is then given");
    }

    // Which holds for a pick of the user as well: the list shows them the
    // JDKs of their kits, whatever device those build on, and picking one of
    // another device is no answer for a kit that builds here either.
    void testPassesOverAPickedJdkOfAnotherDevice()
    {
        const FilePath root = directoryFor("picked-of-a-device");
        const FilePath ofTheMachine = makeJdk(root, "jdk-21", "21.0.1");
        QVERIFY(!ofTheMachine.isEmpty());
        const FilePath ofADevice = FilePath::fromParts(u"docker", u"container", u"/opt/jdk-21");

        const Jdk theirs = jdkFor(ofADevice, DetectionSource::Manual);
        JdkManager::setJdks({theirs, jdkFor(ofTheMachine, DetectionSource::FromSystem)},
                            theirs.id);

        QVERIFY2(JdkManager::defaultJdk().javaHome == ofTheMachine,
                 "a JDK of another device was handed out as the default, which leaves every "
                 "kit of this machine without one for as long as the pick stands");
    }

    void testKeepsAJdkOfTheUserThatIsGone()
    {
        const FilePath root = directoryFor("of-the-user");
        const FilePath gone = root / "jdk-uninstalled";
        JdkManager::setJdks({jdkFor(gone, DetectionSource::Manual)}, Id());

        JdkManager::enterDetectedJdks(JdkManager::detectJdks());

        QVERIFY2(hasJavaHome(JdkManager::jdks(), gone),
                 "a JDK the user entered was taken out of the list over a directory that is "
                 "not there, which only they can put back");
    }

    void testDropsADetectedJdkThatIsGone()
    {
        const FilePath root = directoryFor("detected");
        const FilePath gone = root / "jdk-uninstalled";
        JdkManager::setJdks({jdkFor(gone, DetectionSource::FromSystem)}, Id());

        JdkManager::enterDetectedJdks(JdkManager::detectJdks());

        QVERIFY2(!hasJavaHome(JdkManager::jdks(), gone),
                 "a JDK that was uninstalled is still offered to every kit");
    }

    // Which of the directories naming one JDK the detection sees first is not
    // the same from one start to the next: JAVA_HOME names one of them, and
    // what the directory of a distribution holds is listed in no fixed order.
    // Either way it is the one entry, and the one the kits already name.
    void testKeepsOneEntryForAJdkDetectedUnderAnotherName()
    {
        if (HostOsInfo::isWindowsHost())
            QSKIP("a second name for a directory is not how Windows installs a JDK");

        const FilePath root = directoryFor("another-name");
        const FilePath jdk = makeJdk(root, "jdk-21", "21.0.1");
        QVERIFY(!jdk.isEmpty());
        const FilePath alias = root / "current";
        QVERIFY(jdk.createSymLink(alias));

        const Jdk stored = jdkFor(alias, DetectionSource::FromSystem);
        JdkManager::setJdks({stored}, stored.id);
        offer({jdk});

        JdkManager::enterDetectedJdks(JdkManager::detectJdks());

        const QList<Jdk> jdks = JdkManager::jdks();
        QCOMPARE(countOf(jdks, jdk), 1);
        QVERIFY2(Utils::anyOf(jdks, [&stored](const Jdk &e) { return e.id == stored.id; }),
                 "the entry the kits name was replaced by another entry for the same JDK");
        QVERIFY2(namesJavaHome(jdks, jdk),
                 "the entry goes on naming a directory the machine did not name it by");
    }

private:
    void putTheListBack()
    {
        JdkManager::setJdks(m_savedJdks, m_savedDefaultId);
        putTheJdksOfTheKitsBack(m_savedJdksOfTheKits);
    }

    // One per test, so that what a test leaves behind is not what the next
    // one finds.
    FilePath directoryFor(const QString &name)
    {
        const FilePath directory = FilePath::fromString(m_dir.path()) / name;
        return directory.ensureWritableDir() ? directory : FilePath();
    }

    void offer(const FilePaths &javaHomes)
    {
        jdkProviders().append([javaHomes] { return javaHomes; });
    }

    static Jdk jdkFor(const FilePath &javaHome, DetectionSource::DetectionType type)
    {
        Jdk jdk;
        jdk.id = Id::generate();
        jdk.name = javaHome.fileName();
        jdk.javaHome = javaHome;
        jdk.detectionSource = {type, "JDK"};
        return jdk;
    }

    // By what the directories come down to, not by their names: the detection
    // follows a java into the JDK behind it, and a temporary directory of
    // macOS lies under /var, which is a link to /private/var.
    static int countOf(const QList<Jdk> &jdks, const FilePath &javaHome)
    {
        const FilePath identity = javaHomeIdentity(javaHome);
        return Utils::count(jdks, [&identity](const Jdk &jdk) {
            return javaHomeIdentity(jdk.javaHome) == identity;
        });
    }

    static bool hasJavaHome(const QList<Jdk> &jdks, const FilePath &javaHome)
    {
        return countOf(jdks, javaHome) > 0;
    }

    static bool namesJavaHome(const QList<Jdk> &jdks, const FilePath &javaHome)
    {
        return Utils::anyOf(jdks, [&javaHome](const Jdk &jdk) { return jdk.javaHome == javaHome; });
    }

    static int indexOfJavaHome(const QList<Jdk> &jdks, const FilePath &javaHome)
    {
        const FilePath identity = javaHomeIdentity(javaHome);
        return Utils::indexOf(jdks, [&identity](const Jdk &jdk) {
            return javaHomeIdentity(jdk.javaHome) == identity;
        });
    }

    QTemporaryDir m_dir;
    QList<Jdk> m_savedJdks;
    Id m_savedDefaultId;
    QHash<Kit *, JdkOfAKit> m_savedJdksOfTheKits;
    QList<JdkProvider> m_savedProviders;
};

QObject *createJdkManagerTest()
{
    return new JdkManagerTest;
}

} // namespace Java::Internal

#include "jdkmanager_test.moc"

#endif // WITH_TESTS
