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
    if (filePath.exists()) // Do initial cleanup after possible leftovers from previously failed test
        QVERIFY(filePath.removeRecursively());
    QVERIFY(!filePath.exists());
    QVERIFY(filePath.createDir());
    QVERIFY(filePath.exists());
}

void FileSystemAccessTest::cleanupTestCase()
{
    if (m_skippedAtWhole) // no need to clean up either
        return;
    QVERIFY(baseFilePath().exists());
    QVERIFY(baseFilePath().removeRecursively());
}

void FileSystemAccessTest::testCreateRemoteFile_data()
{
    QTest::addColumn<QByteArray>("data");

    QTest::newRow("Spaces") << QByteArray("Line with spaces");
    QTest::newRow("Newlines") << QByteArray("Some \n\n newlines \n");
    QTest::newRow("Carriage return") << QByteArray("Line with carriage \r return");
    QTest::newRow("Tab") << QByteArray("Line with \t tab");
    QTest::newRow("Apostrophe") << QByteArray("Line with apostrophe's character");
    QTest::newRow("Quotation marks") << QByteArray("Line with \"quotation marks\"");
    QTest::newRow("Backslash 1") << QByteArray("Line with \\ backslash");
    QTest::newRow("Backslash 2") << QByteArray("Line with \\\" backslash");
    QTest::newRow("Command output") << QByteArray("The date is: $(date +%D)");

    const int charSize = sizeof(char) * 0x100;
    QByteArray charString(charSize, Qt::Uninitialized);
    char *data = charString.data();
    for (int c = 0; c < charSize; ++c)
        data[c] = c;
    QTest::newRow("All Characters") << charString;
}

void FileSystemAccessTest::testCreateRemoteFile()
{
    QFETCH(QByteArray, data);

    const FilePath testFilePath = baseFilePath() / "test_file";

    QVERIFY(!testFilePath.exists());
    QVERIFY(testFilePath.writeFileContents(data));
    QVERIFY(testFilePath.exists());
    QCOMPARE(testFilePath.fileContents(), data);
    QVERIFY(testFilePath.removeFile());
    QVERIFY(!testFilePath.exists());
}

void FileSystemAccessTest::testDirStatus()
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
    const FilePath testFilePath = createFile("test");
    QVERIFY(testFilePath.exists());
    QVERIFY(testFilePath.isFile());

    testFilePath.setPermissions(QFile::ReadOther | QFile::WriteOther | QFile::ExeOther
                                | QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup
                                | QFile::ReadUser | QFile::WriteUser | QFile::ExeUser);
    QVERIFY(testFilePath.isWritableFile());
    QVERIFY(testFilePath.isReadableFile());
    QVERIFY(testFilePath.isExecutableFile());

    const QByteArray content("Test");
    testFilePath.writeFileContents(content);
    QCOMPARE(testFilePath.fileContents(), content);

    const FilePath newTestFilePath = baseFilePath() / "test1";
    // It is Ok that FilePath doesn't change itself after rename.
    // FilePath::renameFile() is a const method!
    QVERIFY(testFilePath.renameFile(newTestFilePath));
    QVERIFY(!testFilePath.exists());
    QVERIFY(newTestFilePath.exists());
    QCOMPARE(newTestFilePath.fileContents(), content);
    QVERIFY(!testFilePath.removeFile());
    QVERIFY(newTestFilePath.exists());
    QVERIFY(newTestFilePath.removeFile());
    QVERIFY(!newTestFilePath.exists());
}

} // Internal
} // RemoteLinux
