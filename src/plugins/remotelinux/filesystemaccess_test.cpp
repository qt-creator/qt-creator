// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filesystemaccess_test.h"

#include "linuxdevice.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/filetransfer.h>
#include <projectexplorer/devicesupport/sshparameters.h>
#include <utils/filepath.h>
#include <utils/processinterface.h>

#include <QDebug>
#include <QFile>
#include <QRandomGenerator>
#include <QTest>
#include <QTimer>

Q_DECLARE_METATYPE(ProjectExplorer::FileTransferMethod)

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

static const char TEST_DIR[] = "/tmp/testdir";

static const FilePath baseFilePath()
{
    return FilePath::fromString("ssh://" + SshTest::userAtHost() + QString(TEST_DIR));
}

TestLinuxDeviceFactory::TestLinuxDeviceFactory()
    : IDeviceFactory("test")
{
    setDisplayName("Remote Linux Device");
    setIcon(QIcon());
    setConstructionFunction(&LinuxDevice::create);
    setCreator([] {
        LinuxDevice::Ptr device = LinuxDevice::create();
        device->setupId(IDevice::ManuallyAdded);
        device->setType("test");
        qDebug() << "device : " << device->type();
        device->setSshParameters(SshTest::getParameters());
        return device;
    });
}

FilePath createFile(const QString &name)
{
    FilePath testFilePath = baseFilePath() / name;
    FilePath dummyFilePath = FilePath::fromString("ssh://" + SshTest::userAtHost() + "/dev/null");
    dummyFilePath.copyFile(testFilePath);
    return testFilePath;
}

void FileSystemAccessTest::initTestCase()
{
    const SshParameters params = SshTest::getParameters();
    qDebug() << "Using following SSH parameter:"
             << "\nHost:" << params.host()
             << "\nPort:" << params.port()
             << "\nUser:" << params.userName()
             << "\nSSHKey:" << params.privateKeyFile;
    if (!SshTest::checkParameters(params)) {
        m_skippedAtWhole = true;
        SshTest::printSetupHelp();
        QSKIP("Ensure you have added your default ssh public key to your own authorized keys and "
              "environment QTC_REMOTELINUX_SSH_DEFAULTS set or follow setup help above.");
        return;
    }
    FilePath filePath = baseFilePath();

    if (DeviceManager::deviceForPath(filePath) == nullptr) {
        const IDevice::Ptr device = m_testLinuxDeviceFactory.create();
        QVERIFY(!device.isNull());
        DeviceManager *deviceManager = DeviceManager::instance();
        deviceManager->addDevice(device);
        m_device = deviceManager->find(device->id());
        QVERIFY(m_device);
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
    QCOMPARE(testFilePath.fileContents().value_or(QByteArray()), data);
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
    FilePath testFilePath = FilePath::fromString("ssh://" + SshTest::userAtHost() + "/tmp");
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
    QCOMPARE(testFilePath.fileContents().value_or(QByteArray()), content);

    const FilePath newTestFilePath = baseFilePath() / "test1";
    // It is Ok that FilePath doesn't change itself after rename.
    // FilePath::renameFile() is a const method!
    QVERIFY(testFilePath.renameFile(newTestFilePath));
    QVERIFY(!testFilePath.exists());
    QVERIFY(newTestFilePath.exists());
    QCOMPARE(newTestFilePath.fileContents().value_or(QByteArray()), content);
    QVERIFY(!testFilePath.removeFile());
    QVERIFY(newTestFilePath.exists());
    QVERIFY(newTestFilePath.removeFile());
    QVERIFY(!newTestFilePath.exists());
}

void FileSystemAccessTest::testFileTransfer_data()
{
    QTest::addColumn<FileTransferMethod>("fileTransferMethod");

    QTest::addRow("Sftp") << FileTransferMethod::Sftp;
//    QTest::addRow("Rsync") << FileTransferMethod::Rsync;
}

void FileSystemAccessTest::testFileTransfer()
{
    QFETCH(FileTransferMethod, fileTransferMethod);

    FileTransfer fileTransfer;
    fileTransfer.setTransferMethod(fileTransferMethod);

    // Create and upload 1000 small files and one big file
    QTemporaryDir dirForFilesToUpload;
    QTemporaryDir dirForFilesToDownload;
    QTemporaryDir dir2ForFilesToDownload;
    QVERIFY2(dirForFilesToUpload.isValid(), qPrintable(dirForFilesToUpload.errorString()));
    QVERIFY2(dirForFilesToDownload.isValid(), qPrintable(dirForFilesToDownload.errorString()));
    QVERIFY2(dir2ForFilesToDownload.isValid(), qPrintable(dirForFilesToDownload.errorString()));
    static const auto getRemoteFilePath = [this](const QString &localFileName) {
        return m_device->filePath(QString("/tmp/foo/bar/").append(localFileName).append(".upload"));
    };
    FilesToTransfer filesToUpload;
    std::srand(QDateTime::currentDateTime().toSecsSinceEpoch());
    for (int i = 0; i < 100; ++i) {
        const QString fileName = "testFile" + QString::number(i + 1);
        QFile file(dirForFilesToUpload.path() + '/' + fileName);
        QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(file.errorString()));
        int content[1024 / sizeof(int)];
        for (size_t j = 0; j < sizeof content / sizeof content[0]; ++j)
            content[j] = QRandomGenerator::global()->generate();
        file.write(reinterpret_cast<char *>(content), sizeof content);
        file.close();
        QVERIFY2(file.error() == QFile::NoError, qPrintable(file.errorString()));
        filesToUpload << FileToTransfer{FilePath::fromString(file.fileName()),
                                        getRemoteFilePath(fileName)};
    }

    const QString bigFileName("bigFile");
    QFile bigFile(dirForFilesToUpload.path() + '/' + bigFileName);
    QVERIFY2(bigFile.open(QIODevice::WriteOnly), qPrintable(bigFile.errorString()));
    const int bigFileSize = 100 * 1024 * 1024;
    const int blockSize = 8192;
    const int blockCount = bigFileSize / blockSize;
    for (int block = 0; block < blockCount; ++block) {
        int content[blockSize / sizeof(int)];
        for (size_t j = 0; j < sizeof content / sizeof content[0]; ++j)
            content[j] = QRandomGenerator::global()->generate();
        bigFile.write(reinterpret_cast<char *>(content), sizeof content);
    }
    bigFile.close();
    QVERIFY2(bigFile.error() == QFile::NoError, qPrintable(bigFile.errorString()));
    filesToUpload << FileToTransfer{FilePath::fromString(bigFile.fileName()),
                                    getRemoteFilePath(bigFileName)};
    fileTransfer.setFilesToTransfer(filesToUpload);

    ProcessResultData result;
    QEventLoop loop;
    connect(&fileTransfer, &FileTransfer::done, this, [&result, &loop]
            (const ProcessResultData &resultData) {
        result = resultData;
        loop.quit();
    });
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.setSingleShot(true);
    timer.setInterval(30 * 1000);
    timer.start();
    fileTransfer.start();
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
    QCOMPARE(result.m_exitStatus, QProcess::NormalExit);
    QVERIFY2(result.m_errorString.isEmpty(), qPrintable(result.m_errorString));
    QCOMPARE(result.m_exitCode, 0);
    QCOMPARE(result.m_error, QProcess::UnknownError);

    // Cleanup remote
    const FilePath remoteDir = m_device->filePath(QString("/tmp/foo/"));
    QString errorString;
    QVERIFY2(remoteDir.removeRecursively(&errorString), qPrintable(errorString));
}

} // Internal
} // RemoteLinux
