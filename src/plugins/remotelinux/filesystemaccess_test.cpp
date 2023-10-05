// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filesystemaccess_test.h"

#include "linuxdevice.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/filetransfer.h>
#include <projectexplorer/devicesupport/sshparameters.h>
#include <utils/filepath.h>
#include <utils/filestreamer.h>
#include <utils/filestreamermanager.h>
#include <utils/process.h>
#include <utils/processinterface.h>
#include <utils/scopedtimer.h>

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
    return FilePath::fromString("ssh://" + SshTest::userAtHostAndPort() + QString(TEST_DIR));
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
    FilePath dummyFilePath = FilePath::fromString("ssh://" + SshTest::userAtHostAndPort() + "/dev/null");
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

    const QString streamerLocalDir("streamerLocalDir");
    const QString streamerRemoteDir("streamerRemoteDir");
    const QString sourceDir("source");
    const QString destDir("dest");
    const QString localDir("local");
    const QString remoteDir("remote");
    const FilePath localRoot;
    const FilePath remoteRoot = m_device->rootPath();
    const FilePath localTempDir = *localRoot.tmpDir();
    const FilePath remoteTempDir = *remoteRoot.tmpDir();
    m_localStreamerDir = localTempDir / streamerLocalDir;
    m_remoteStreamerDir = remoteTempDir / streamerRemoteDir;
    m_localSourceDir = m_localStreamerDir / sourceDir;
    m_remoteSourceDir = m_remoteStreamerDir / sourceDir;
    m_localDestDir = m_localStreamerDir / destDir;
    m_remoteDestDir = m_remoteStreamerDir / destDir;
    m_localLocalDestDir = m_localDestDir / localDir;
    m_localRemoteDestDir = m_localDestDir / remoteDir;
    m_remoteLocalDestDir = m_remoteDestDir / localDir;
    m_remoteRemoteDestDir = m_remoteDestDir / remoteDir;

    QVERIFY(m_localSourceDir.createDir());
    QVERIFY(m_remoteSourceDir.createDir());
    QVERIFY(m_localDestDir.createDir());
    QVERIFY(m_remoteDestDir.createDir());
    QVERIFY(m_localLocalDestDir.createDir());
    QVERIFY(m_localRemoteDestDir.createDir());
    QVERIFY(m_remoteLocalDestDir.createDir());
    QVERIFY(m_remoteRemoteDestDir.createDir());

    QVERIFY(m_localSourceDir.exists());
    QVERIFY(m_remoteSourceDir.exists());
    QVERIFY(m_localDestDir.exists());
    QVERIFY(m_remoteDestDir.exists());
    QVERIFY(m_localLocalDestDir.exists());
    QVERIFY(m_localRemoteDestDir.exists());
    QVERIFY(m_remoteLocalDestDir.exists());
    QVERIFY(m_remoteRemoteDestDir.exists());
}

void FileSystemAccessTest::cleanupTestCase()
{
    if (m_skippedAtWhole) // no need to clean up either
        return;
    QVERIFY(baseFilePath().exists());
    QVERIFY(baseFilePath().removeRecursively());

    QVERIFY(m_localStreamerDir.removeRecursively());
    QVERIFY(m_remoteStreamerDir.removeRecursively());

    QVERIFY(!m_localStreamerDir.exists());
    QVERIFY(!m_remoteStreamerDir.exists());

    FileStreamerManager::stopAll();
}

void FileSystemAccessTest::testCreateRemoteFile_data()
{
    QTest::addColumn<QByteArray>("data");

    QTest::newRow("Spaces") << QByteArray("Line with spaces");
    QTest::newRow("Newlines") << QByteArray("Some \n\n newlines \n");
    QTest::newRow("CarriageReturn") << QByteArray("Line with carriage \r return");
    QTest::newRow("Tab") << QByteArray("Line with \t tab");
    QTest::newRow("Apostrophe") << QByteArray("Line with apostrophe's character");
    QTest::newRow("QuotationMarks") << QByteArray("Line with \"quotation marks\"");
    QTest::newRow("Backslash1") << QByteArray("Line with \\ backslash");
    QTest::newRow("Backslash2") << QByteArray("Line with \\\" backslash");
    QTest::newRow("CommandOutput") << QByteArray("The date is: $(date +%D)");

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

void FileSystemAccessTest::testWorkingDirectory()
{
    const FilePath dir = baseFilePath() / "testdir with space and 'various' \"quotes\" here";
    QVERIFY(dir.ensureWritableDir());
    Process proc;
    proc.setCommand({"pwd", {}});
    proc.setWorkingDirectory(dir);
    proc.start();
    QVERIFY(proc.waitForFinished());
    const QString out = proc.readAllStandardOutput().trimmed();
    QCOMPARE(out, dir.path());
    const QString err = proc.readAllStandardOutput();
    QVERIFY(err.isEmpty());
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
                return IterationPolicy::Continue;
            }
            return IterationPolicy::Stop;
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
    // TODO: By default rsync doesn't support creating target directories,
    // needs to be done manually - see RsyncDeployService.
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

void FileSystemAccessTest::testFileStreamer_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QByteArray>("data");

    const QByteArray spaces("Line with spaces");
    const QByteArray newlines("Some \n\n newlines \n");
    const QByteArray carriageReturn("Line with carriage \r return");
    const QByteArray tab("Line with \t tab");
    const QByteArray apostrophe("Line with apostrophe's character");
    const QByteArray quotationMarks("Line with \"quotation marks\"");
    const QByteArray backslash1("Line with \\ backslash");
    const QByteArray backslash2("Line with \\\" backslash");
    const QByteArray commandOutput("The date is: $(date +%D)");

    const int charSize = sizeof(char) * 0x100;
    QByteArray charString(charSize, Qt::Uninitialized);
    char *data = charString.data();
    for (int c = 0; c < charSize; ++c)
        data[c] = c;

    const int bigSize = 1024 * 1024; // = 256 * 1024 * 1024 = 268.435.456 bytes
    QByteArray bigString;
    for (int i = 0; i < bigSize; ++i)
        bigString += charString;

    QTest::newRow("Spaces") << QString("spaces") << spaces;
    QTest::newRow("Newlines") << QString("newlines") << newlines;
    QTest::newRow("CarriageReturn") << QString("carriageReturn") << carriageReturn;
    QTest::newRow("Tab") << QString("tab") << tab;
    QTest::newRow("Apostrophe") << QString("apostrophe") << apostrophe;
    QTest::newRow("QuotationMarks") << QString("quotationMarks") << quotationMarks;
    QTest::newRow("Backslash1") << QString("backslash1") << backslash1;
    QTest::newRow("Backslash2") << QString("backslash2") << backslash2;
    QTest::newRow("CommandOutput") << QString("commandOutput") << commandOutput;
    QTest::newRow("AllCharacters") << QString("charString") << charString;
    QTest::newRow("BigString") << QString("bigString") << bigString;
}

void FileSystemAccessTest::testFileStreamer()
{
    QTC_SCOPED_TIMER("testFileStreamer");

    QFETCH(QString, fileName);
    QFETCH(QByteArray, data);

    const FilePath localSourcePath = m_localSourceDir / fileName;
    const FilePath remoteSourcePath = m_remoteSourceDir / fileName;
    const FilePath localLocalDestPath = m_localDestDir / "local" / fileName;
    const FilePath localRemoteDestPath = m_localDestDir / "remote" / fileName;
    const FilePath remoteLocalDestPath = m_remoteDestDir / "local" / fileName;
    const FilePath remoteRemoteDestPath = m_remoteDestDir / "remote" / fileName;

    localSourcePath.removeFile();
    remoteSourcePath.removeFile();
    localLocalDestPath.removeFile();
    localRemoteDestPath.removeFile();
    remoteLocalDestPath.removeFile();
    remoteRemoteDestPath.removeFile();

    QVERIFY(!localSourcePath.exists());
    QVERIFY(!remoteSourcePath.exists());
    QVERIFY(!localLocalDestPath.exists());
    QVERIFY(!localRemoteDestPath.exists());
    QVERIFY(!remoteLocalDestPath.exists());
    QVERIFY(!remoteRemoteDestPath.exists());

    std::optional<QByteArray> localData;
    std::optional<QByteArray> remoteData;
    std::optional<QByteArray> localLocalData;
    std::optional<QByteArray> localRemoteData;
    std::optional<QByteArray> remoteLocalData;
    std::optional<QByteArray> remoteRemoteData;

    using namespace Tasking;

    const auto localWriter = [&] {
        const auto setup = [&](FileStreamer &streamer) {
            streamer.setStreamMode(StreamMode::Writer);
            streamer.setDestination(localSourcePath);
            streamer.setWriteData(data);
        };
        return FileStreamerTask(setup);
    };
    const auto remoteWriter = [&] {
        const auto setup = [&](FileStreamer &streamer) {
            streamer.setStreamMode(StreamMode::Writer);
            streamer.setDestination(remoteSourcePath);
            streamer.setWriteData(data);
        };
        return FileStreamerTask(setup);
    };
    const auto localReader = [&] {
        const auto setup = [&](FileStreamer &streamer) {
            streamer.setStreamMode(StreamMode::Reader);
            streamer.setSource(localSourcePath);
        };
        const auto onDone = [&](const FileStreamer &streamer) {
            localData = streamer.readData();
        };
        return FileStreamerTask(setup, onDone);
    };
    const auto remoteReader = [&] {
        const auto setup = [&](FileStreamer &streamer) {
            streamer.setStreamMode(StreamMode::Reader);
            streamer.setSource(remoteSourcePath);
        };
        const auto onDone = [&](const FileStreamer &streamer) {
            remoteData = streamer.readData();
        };
        return FileStreamerTask(setup, onDone);
    };
    const auto transfer = [](const FilePath &source, const FilePath &dest,
                             std::optional<QByteArray> *result) {
        const auto setupTransfer = [=](FileStreamer &streamer) {
            streamer.setSource(source);
            streamer.setDestination(dest);
        };
        const auto setupReader = [=](FileStreamer &streamer) {
            streamer.setStreamMode(StreamMode::Reader);
            streamer.setSource(dest);
        };
        const auto onReaderDone = [result](const FileStreamer &streamer) {
            *result = streamer.readData();
        };
        const Group root {
            FileStreamerTask(setupTransfer),
            FileStreamerTask(setupReader, onReaderDone)
        };
        return root;
    };

    // In total: 5 local reads, 3 local writes, 5 remote reads, 3 remote writes
    const Group root {
        Group {
            parallel,
            localWriter(),
            remoteWriter()
        },
        Group {
            parallel,
            localReader(),
            remoteReader()
        },
        Group {
            parallel,
            transfer(localSourcePath, localLocalDestPath, &localLocalData),
            transfer(remoteSourcePath, localRemoteDestPath, &localRemoteData),
            transfer(localSourcePath, remoteLocalDestPath, &remoteLocalData),
            transfer(remoteSourcePath, remoteRemoteDestPath, &remoteRemoteData),
        }
    };

    using namespace std::chrono_literals;
    QVERIFY(TaskTree::runBlocking(root, 10000ms));

    QVERIFY(localData);
    QCOMPARE(*localData, data);
    QVERIFY(remoteData);
    QCOMPARE(*remoteData, data);

    QVERIFY(localLocalData);
    QCOMPARE(*localLocalData, data);
    QVERIFY(localRemoteData);
    QCOMPARE(*localRemoteData, data);
    QVERIFY(remoteLocalData);
    QCOMPARE(*remoteLocalData, data);
    QVERIFY(remoteRemoteData);
    QCOMPARE(*remoteRemoteData, data);
}

void FileSystemAccessTest::testFileStreamerManager_data()
{
    testFileStreamer_data();
}

void FileSystemAccessTest::testFileStreamerManager()
{
    QTC_SCOPED_TIMER("testFileStreamerManager");

    QFETCH(QString, fileName);
    QFETCH(QByteArray, data);

    const FilePath localSourcePath = m_localSourceDir / fileName;
    const FilePath remoteSourcePath = m_remoteSourceDir / fileName;
    const FilePath localLocalDestPath = m_localDestDir / "local" / fileName;
    const FilePath localRemoteDestPath = m_localDestDir / "remote" / fileName;
    const FilePath remoteLocalDestPath = m_remoteDestDir / "local" / fileName;
    const FilePath remoteRemoteDestPath = m_remoteDestDir / "remote" / fileName;

    localSourcePath.removeFile();
    remoteSourcePath.removeFile();
    localLocalDestPath.removeFile();
    localRemoteDestPath.removeFile();
    remoteLocalDestPath.removeFile();
    remoteRemoteDestPath.removeFile();

    QVERIFY(!localSourcePath.exists());
    QVERIFY(!remoteSourcePath.exists());
    QVERIFY(!localLocalDestPath.exists());
    QVERIFY(!localRemoteDestPath.exists());
    QVERIFY(!remoteLocalDestPath.exists());
    QVERIFY(!remoteRemoteDestPath.exists());

    std::optional<QByteArray> localData;
    std::optional<QByteArray> remoteData;
    std::optional<QByteArray> localLocalData;
    std::optional<QByteArray> localRemoteData;
    std::optional<QByteArray> remoteLocalData;
    std::optional<QByteArray> remoteRemoteData;

    QEventLoop eventLoop1;
    QEventLoop *loop = &eventLoop1;
    int counter = 0;
    int *hitCount = &counter;

    const auto writeAndRead = [hitCount, loop, data](const FilePath &destination,
                                                     std::optional<QByteArray> *result) {
        const auto onWrite = [hitCount, loop, destination, result]
            (const expected_str<qint64> &writeResult) {
            QVERIFY(writeResult);
            const auto onRead = [hitCount, loop, result]
                (const expected_str<QByteArray> &readResult) {
                QVERIFY(readResult);
                *result = *readResult;
                ++(*hitCount);
                if (*hitCount == 2)
                    loop->quit();
            };
            FileStreamerManager::read(destination, onRead);
        };
        FileStreamerManager::write(destination, data, onWrite);
    };

    writeAndRead(localSourcePath, &localData);
    writeAndRead(remoteSourcePath, &remoteData);
    loop->exec();

    QVERIFY(localData);
    QCOMPARE(*localData, data);
    QVERIFY(remoteData);
    QCOMPARE(*remoteData, data);

    QEventLoop eventLoop2;
    loop = &eventLoop2;
    counter = 0;

    const auto transferAndRead = [hitCount, loop, data](const FilePath &source,
                                                        const FilePath &destination,
                                                        std::optional<QByteArray> *result) {
        const auto onTransfer = [hitCount, loop, destination, result]
            (const expected_str<void> &transferResult) {
                QVERIFY(transferResult);
                const auto onRead = [hitCount, loop, result]
                    (const expected_str<QByteArray> &readResult) {
                    QVERIFY(readResult);
                    *result = *readResult;
                    ++(*hitCount);
                    if (*hitCount == 4)
                        loop->quit();
                };
                FileStreamerManager::read(destination, onRead);
            };
        FileStreamerManager::copy(source, destination, onTransfer);
    };

    transferAndRead(localSourcePath, localLocalDestPath, &localLocalData);
    transferAndRead(remoteSourcePath, localRemoteDestPath, &localRemoteData);
    transferAndRead(localSourcePath, remoteLocalDestPath, &remoteLocalData);
    transferAndRead(remoteSourcePath, remoteRemoteDestPath, &remoteRemoteData);
    loop->exec();

    QVERIFY(localLocalData);
    QCOMPARE(*localLocalData, data);
    QVERIFY(localRemoteData);
    QCOMPARE(*localRemoteData, data);
    QVERIFY(remoteLocalData);
    QCOMPARE(*remoteLocalData, data);
    QVERIFY(remoteRemoteData);
    QCOMPARE(*remoteRemoteData, data);
}

} // Internal
} // RemoteLinux
