// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dockermounttest.h"

#include "dockerdevice.h"
#include "dockersettings.h"

#include <projectexplorer/task.h>

#include <utils/filepath.h>
#include <utils/macroexpander.h>

#include <QTemporaryDir>
#include <QTest>

using namespace Utils;

namespace Docker::Internal {

// splitMountEntry() decides where a mount entry is cut, without interpreting
// either half as a path. That keeps these cases meaningful on any host: a
// Windows entry can be checked from Linux and the other way round.
class DockerMountTest : public QObject
{
    Q_OBJECT

private slots:
    void testSplitMountEntry_data()
    {
        QTest::addColumn<QString>("entry");
        QTest::addColumn<QString>("path");
        QTest::addColumn<QString>("containerPath");

        QTest::newRow("plain unix path")
            << "/home/user/src" << "/home/user/src" << "/home/user/src";
        QTest::newRow("mapped unix path") << "/home/user/src:/work" << "/home/user/src" << "/work";
        QTest::newRow("plain windows path") << "C:\\src" << "C:\\src" << "C:\\src";
        QTest::newRow("mapped windows path") << "C:\\src:/work" << "C:\\src" << "/work";
        QTest::newRow("windows forward slashes") << "C:/src:/work" << "C:/src" << "/work";
        // A drive letter's colon is not a separator, whichever slash follows it.
        QTest::newRow("windows forward slashes, unmapped") << "C:/src" << "C:/src" << "C:/src";
        QTest::newRow("drive root") << "D:/" << "D:/" << "D:/";
        // A relative container path is not one, so the whole entry is the host side.
        QTest::newRow("relative right side") << "/home/user:work" << "/home/user:work"
                                             << "/home/user:work";
        QTest::newRow("trailing colon") << "/home/user:" << "/home/user:" << "/home/user:";
        // Only the last colon that opens an absolute path splits.
        QTest::newRow("colon in host path") << "/odd:name:/work" << "/odd:name" << "/work";
        QTest::newRow("empty") << "" << "" << "";
    }

    void testSplitMountEntry()
    {
        QFETCH(QString, entry);
        QFETCH(QString, path);
        QFETCH(QString, containerPath);

        const auto [actualPath, actualContainerPath] = splitMountEntry(entry);
        QCOMPARE(actualPath, path);
        QCOMPARE(actualContainerPath, containerPath);
    }

    // Maps a path inside the container back to the host through a mount that
    // names its container path.
    void testLocalSourceOfMappedMount_data()
    {
        QTest::addColumn<QString>("mount");
        QTest::addColumn<QString>("containerPath");
        QTest::addColumn<QString>("hostPath");

        QTest::newRow("mount root") << "/home/user/src:/work" << "/work" << "/home/user/src";
        QTest::newRow("below mount")
            << "/home/user/src:/work" << "/work/a/b" << "/home/user/src/a/b";
    }

    void testLocalSourceOfMappedMount()
    {
        QFETCH(QString, mount);
        QFETCH(QString, containerPath);
        QFETCH(QString, hostPath);

        const DockerDevice::Ptr device = DockerDevice::create(&dockerSettings());
        device->mounts.setValue({mount});

        const Result<FilePath> source
            = device->localSource(device->rootPath().withNewPath(containerPath));

        if (!source)
            QFAIL(qPrintable(source.error()));
        QCOMPARE(source->path(), hostPath);
    }

    // The drive-letter inversion a Windows host applies on the way back. The host
    // OS is a parameter, so both the Windows and the non-Windows answer are
    // checked wherever the tests run.
    void testInvertedDriveLetterPath_data()
    {
        QTest::addColumn<int>("hostOs");
        QTest::addColumn<QString>("containerPath");
        QTest::addColumn<QString>("hostPath");

        QTest::newRow("windows host") << int(OsTypeWindows) << "/c/dev/src" << "C:/dev/src";
        QTest::newRow("windows host, drive root") << int(OsTypeWindows) << "/c/" << "C:/";
        // Only a single letter is a drive, so a mount point keeps its name.
        QTest::newRow("not a drive") << int(OsTypeWindows) << "/work/a" << "";
        // Off Windows nothing was ever rewritten, so there is nothing to undo.
        QTest::newRow("linux host") << int(OsTypeLinux) << "/c/dev/src" << "";
        QTest::newRow("mac host") << int(OsTypeMac) << "/c/dev/src" << "";
    }

    void testInvertedDriveLetterPath()
    {
        QFETCH(int, hostOs);
        QFETCH(QString, containerPath);
        QFETCH(QString, hostPath);

        const DockerDevice::Ptr device = DockerDevice::create(&dockerSettings());
        const FilePath devicePath = device->rootPath().withNewPath(containerPath);

        QCOMPARE(invertedDriveLetterPath(OsType(hostOs), devicePath), hostPath);
    }

    // A mount that names its container path beats reading the same path as a
    // drive letter: "/w" is a mount point, not drive W. Passing the host OS in
    // means this is checked on every platform, not just on Windows.
    void testNamedMountBeatsDriveLetter()
    {
        const DockerDevice::Ptr device = DockerDevice::create(&dockerSettings());
        const FilePath devicePath = device->rootPath().withNewPath("/w/a");
        const QList<MountPair> mounts = parseMounts({"/home/user/src:/w"});

        const Result<FilePath> onWindows = hostPathFor(mounts, OsTypeWindows, devicePath);
        if (!onWindows)
            QFAIL(qPrintable(onWindows.error()));
        QCOMPARE(onWindows->path(), QString("/home/user/src/a"));

        // Without such a mount the drive letter is all there is to go by, and
        // "W:/a" is not mounted either, so the lookup fails rather than
        // inventing a host path.
        const QList<MountPair> other = parseMounts({"/home/user/src:/work"});
        QVERIFY(!hostPathFor(other, OsTypeWindows, devicePath));
    }

    // The spelling a one-to-one Windows mount gets inside the container.
    void testDriveLetterContainerPath_data()
    {
        QTest::addColumn<QString>("hostPath");
        QTest::addColumn<QString>("containerPath");

        QTest::newRow("drive") << "C:/dev/src" << "/C/dev/src";
        QTest::newRow("drive root") << "C:/" << "/C/";
        QTest::newRow("unix path") << "/work" << "/work";
    }

    void testDriveLetterContainerPath()
    {
        QFETCH(QString, hostPath);
        QFETCH(QString, containerPath);

        QCOMPARE(driveLetterContainerPath(FilePath::fromString(hostPath)), containerPath);
    }

    // The destination a mount is bound at and the path a process is handed have
    // to be the same string, or a Linux container reads them as two directories.
    void testMountDestinationMatchesMappedPath_data()
    {
        QTest::addColumn<int>("hostOs");
        QTest::addColumn<QString>("hostPath");

        QTest::newRow("windows host") << int(OsTypeWindows) << "C:/Dev/Src";
        QTest::newRow("windows host, lower case") << int(OsTypeWindows) << "c:/dev/src";
        QTest::newRow("unix host") << int(OsTypeLinux) << "/home/user/src";
    }

    void testMountDestinationMatchesMappedPath()
    {
        QFETCH(int, hostOs);
        QFETCH(QString, hostPath);

        QCOMPARE(mountPathFor(OsType(hostOs), FilePath::fromUserInput(hostPath)),
                 containerPathFor(parseMounts({hostPath}), OsType(hostOs), hostPath));
    }

    // The entries are macro-expanded before they are split. The default entry is
    // "%{Config:DefaultProjectDirectory:NativeFilePath}", so reading the raw
    // value would mount that literal string instead of a directory.
    void testMountEntriesAreExpanded()
    {
        const QString dir = globalMacroExpander()->expand(
            QString("%{Config:DefaultProjectDirectory:NativeFilePath}"));
        QVERIFY(!dir.isEmpty());
        QVERIFY(!dir.contains('%'));

        const DockerDevice::Ptr device = DockerDevice::create(&dockerSettings());
        device->mounts.setValue({"%{Config:DefaultProjectDirectory:NativeFilePath}:/work"});

        const Result<FilePath> source
            = device->localSource(device->rootPath().withNewPath("/work/a.pro"));

        if (!source)
            QFAIL(qPrintable(source.error()));
        QCOMPARE(source->path(), (FilePath::fromUserInput(dir) / "a.pro").path());
    }

    // Which entries make it onto the docker command line, and which are dropped
    // for the "Paths to mount" label to warn about. The reasons are translated,
    // so only the verdict is compared.
    void testValidateMount_data()
    {
        QTest::addColumn<QString>("entry");
        QTest::addColumn<bool>("isValid");

        const QString dir = m_existingDir.path();

        QTest::newRow("existing directory") << dir << true;
        QTest::newRow("mapped to a container path") << dir + ":/work" << true;
        // An entry whose macro expanded to nothing, which is what the default
        // entry does when there is no default project directory.
        QTest::newRow("empty") << QString() << false;
        QTest::newRow("relative host path") << "src:/work" << false;
        QTest::newRow("container root") << dir + ":/" << false;
        QTest::newRow("does not exist") << dir + "/nope" << false;
    }

    void testValidateMount()
    {
        QFETCH(QString, entry);
        QFETCH(bool, isValid);

        const Result<> res = validateMount(parseMount(entry));
        QCOMPARE(bool(res), isValid);
        // The label shows the reason, so a silent rejection is of no use.
        if (!isValid)
            QVERIFY(!res.error().isEmpty());
    }

    // Device validation answers from the same rule, so an entry that will be
    // dropped is not reported as fine on the way into a build.
    void testValidateReportsDroppedMount()
    {
        const DockerDevice::Ptr device = DockerDevice::create(&dockerSettings());
        device->mounts.setValue({m_existingDir.path()});
        QVERIFY(device->validate().isEmpty());

        // The container path is the root, which docker refuses, so the entry
        // never reaches the command line.
        device->mounts.setValue({m_existingDir.path() + ":/"});
        QCOMPARE(device->validate().size(), 1);
    }

    // The way in has to agree with the way out, or a file handed to a process in
    // the container and a file read back from it end up at different places.
    void testMapToContainerPath_data()
    {
        QTest::addColumn<QString>("mount");
        QTest::addColumn<QString>("hostPath");
        QTest::addColumn<QString>("containerPath");

        QTest::newRow("mount root") << "/home/user/src:/work" << "/home/user/src" << "/work";
        QTest::newRow("below mount")
            << "/home/user/src:/work" << "/home/user/src/a/b" << "/work/a/b";
        // One-to-one mounts are left to the caller, which keeps the path as is.
        QTest::newRow("one to one") << "/home/user/src" << "/home/user/src/a" << "";
        QTest::newRow("outside any mount") << "/home/user/src:/work" << "/elsewhere/a" << "";
    }

    void testMapToContainerPath()
    {
        QFETCH(QString, mount);
        QFETCH(QString, hostPath);
        QFETCH(QString, containerPath);

        const FilePath mapped
            = mapToContainerPath(parseMounts({mount}), FilePath::fromString(hostPath));
        QCOMPARE(mapped.path(), containerPath);
    }

    // configuredDevicePath() has to answer from the mount settings alone. A
    // device that was never started has no container, so anything reaching for
    // file access here would either block or spin one up. It is the way into
    // the container, so it has to agree with localSource() on the way out.
    void testConfiguredDevicePath()
    {
        const DockerDevice::Ptr device = DockerDevice::create(&dockerSettings());
        device->mounts.setValue({"/home/user/src:/work"});

        const FilePath hostPath = FilePath::fromString("/home/user/src/a.pro");
        const FilePath onDevice = device->configuredDevicePath(hostPath);
        QCOMPARE(onDevice, device->rootPath().withNewPath("/work/a.pro"));

        const Result<FilePath> back = device->localSource(onDevice);
        if (!back)
            QFAIL(qPrintable(back.error()));
        QCOMPARE(*back, hostPath);

        // Not mounted anywhere: the plain path, for the caller to use as is.
        QCOMPARE(device->configuredDevicePath(FilePath::fromString("/elsewhere/a.pro")).path(),
                 QString("/elsewhere/a.pro"));
        // Already on a device, so there is nothing to translate.
        QVERIFY(device->configuredDevicePath(device->rootPath().withNewPath("/work/a.pro"))
                    .isEmpty());
    }

private:
    QTemporaryDir m_existingDir;
};

QObject *createDockerMountTest()
{
    return new DockerMountTest;
}

} // namespace Docker::Internal

#include "dockermounttest.moc"
