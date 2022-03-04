/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "filesystemaccess_test.h"

#include "linuxdevice.h"
#include "remotelinux_constants.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <ssh/sshconnection.h>
#include <utils/filepath.h>

#include <QDebug>
#include <QTest>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

static const char TEST_DIR[] = "/tmp/testdir";

static const FilePath baseFilePath()
{
    return FilePath::fromString("ssh://" + QSsh::SshTest::userAtHost() + QString(TEST_DIR));
}

TestLinuxDeviceFactory::TestLinuxDeviceFactory()
    : IDeviceFactory("test")
{
    setDisplayName("Generic Linux Device");
    setIcon(QIcon());
    setConstructionFunction(&LinuxDevice::create);
    setCreator([] {
        LinuxDevice::Ptr newDev = LinuxDevice::create();
        newDev->setType("test");
        qDebug() << "device : " << newDev->type();
        QSsh::SshConnectionParameters sshParams = QSsh::SshTest::getParameters();
        newDev->setSshParameters(sshParams);
        return newDev;
    });
}

FilePath createFile(const QString &name)
{
    FilePath testFilePath = baseFilePath() / name;
    FilePath dummyFilePath = FilePath::fromString("ssh://" + QSsh::SshTest::userAtHost() + "/dev/null");
    dummyFilePath.copyFile(testFilePath);
    return testFilePath;
}

void FileSystemAccessTest::initTestCase()
{
    const QSsh::SshConnectionParameters params = QSsh::SshTest::getParameters();
    qDebug() << "Using following SSH parameter:"
             << "\nHost:" << params.host()
             << "\nPort:" << params.port()
             << "\nUser:" << params.userName()
             << "\nSSHKey:" << params.privateKeyFile;
    if (!QSsh::SshTest::checkParameters(params)) {
        m_skippedAtWhole = true;
        QSsh::SshTest::printSetupHelp();
        QSKIP("Ensure you have added your default ssh public key to your own authorized keys and "
              "environment QTC_REMOTELINUX_SSH_DEFAULTS set or follow setup help above.");
        return;
    }
    FilePath filePath = baseFilePath();

    if (DeviceManager::deviceForPath(filePath) == nullptr) {
        DeviceManager *const devMgr = DeviceManager::instance();
        const IDevice::Ptr newDev = m_testLinuxDeviceFactory.create();
        QVERIFY(!newDev.isNull());
        devMgr->addDevice(newDev);
    }
    QVERIFY(filePath.createDir());
}

void FileSystemAccessTest::cleanupTestCase()
{
    if (m_skippedAtWhole) // no need to clean up either
        return;
    QVERIFY(baseFilePath().exists());
    QVERIFY(baseFilePath().removeRecursively());
}

void FileSystemAccessTest::testDirStatuses()
{
    FilePath filePath = baseFilePath();
    QVERIFY(filePath.exists());
    QVERIFY(filePath.isDir());
    QVERIFY(filePath.isWritableDir());

    FilePath testFilePath = createFile("test");
    QVERIFY(testFilePath.exists());
    QVERIFY(testFilePath.isFile());

    bool fileExists = false;
    filePath.iterateDirectory(
        [&fileExists](const FilePath &filePath) {
            if (filePath.baseName() == "test") {
                fileExists = true;
                return true;
            }
            return false;
        },
        {{"test"}, QDir::Files});

    QVERIFY(fileExists);
    QVERIFY(testFilePath.removeFile());
    QVERIFY(!testFilePath.exists());
}

void FileSystemAccessTest::testBytesAvailable()
{
    FilePath testFilePath = FilePath::fromString("ssh://" + QSsh::SshTest::userAtHost() + "/tmp");
    QVERIFY(testFilePath.exists());
    QVERIFY(testFilePath.bytesAvailable() > 0);
}

void FileSystemAccessTest::testFileActions()
{
    FilePath testFilePath = createFile("test");
    QVERIFY(testFilePath.exists());
    QVERIFY(testFilePath.isFile());

    testFilePath.setPermissions(QFile::ReadOther | QFile::WriteOther | QFile::ExeOther
                                | QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup
                                | QFile::ReadUser | QFile::WriteUser | QFile::ExeUser);
    QVERIFY(testFilePath.isWritableFile());
    QVERIFY(testFilePath.isReadableFile());
    QVERIFY(testFilePath.isExecutableFile());

    QByteArray content("Test");
    testFilePath.writeFileContents(content);
    // ToDo: remove ".contains", make fileContents exact equal content
    QVERIFY(testFilePath.fileContents().contains(content));

    QVERIFY(testFilePath.renameFile(baseFilePath() / "test1"));
    // It is Ok that FilePath doesn't change itself after rename.
    FilePath newTestFilePath = baseFilePath() / "test1";
    QVERIFY(newTestFilePath.exists());
    QVERIFY(!testFilePath.removeFile());
    QVERIFY(newTestFilePath.exists());
    QVERIFY(newTestFilePath.removeFile());
    QVERIFY(!newTestFilePath.exists());
}

} // Internal
} // RemoteLinux
