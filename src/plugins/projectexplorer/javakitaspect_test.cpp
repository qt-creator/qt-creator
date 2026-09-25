// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "javakitaspect_test.h"

#ifdef WITH_TESTS

#include "devicesupport/devicekitaspects.h"
#include "devicesupport/devicemanager.h"
#include "devicesupport/idevice.h"
#include "environmentkitaspect.h"
#include "javakitaspect.h"
#include "kit.h"
#include "kitmanager.h"

#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/id.h>

#include <QTemporaryDir>
#include <QTest>

using namespace Utils;

namespace ProjectExplorer::Internal {

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
        QVERIFY(m_dir.isValid());
        m_jdk = makeFakeJdk(m_dir, "jdk-stored");
        QVERIFY(!m_jdk.isEmpty());
        m_offeredJdk = makeFakeJdk(m_dir, "jdk-offered");
        QVERIFY(!m_offeredJdk.isEmpty());
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
        javaHomeCandidateProviders().append([javaHome](const Kit *) -> FilePaths {
            return {javaHome};
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
};

QObject *createJavaKitAspectTest()
{
    return new JavaKitAspectTest;
}

} // namespace ProjectExplorer::Internal

#include "javakitaspect_test.moc"

#endif // WITH_TESTS
