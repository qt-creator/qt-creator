// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include <utils/filepath.h>
#include <utils/fileutils.h>
#include <utils/fsengine/fsengine.h>
#include <utils/hostosinfo.h>

#include <QDebug>
#include <QFileSystemModel>
#include <QtTest>

using namespace Utils;

class tst_fsengine : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testFilePathFromToString();

    void testRootPathContainsFakeDir();
    void testNotExistingFile();
    void testCreateFile();
    void testListDir();
    void testCreateDir();
    void testWindowsPaths();
    void testUrl();
    void testBrokenWindowsPath();

private:
    QString makeTestPath(QString path, bool asUrl = false);

private:
    FSEngine engine;
    QString tempFolder;
};

template<class... Args>
using Continuation = std::function<void(Args...)>;

QString startWithSlash(QString s)
{
    if (!s.startsWith('/'))
        s.prepend('/');
    return s;
}

void tst_fsengine::initTestCase()
{
    if (!FSEngine::isAvailable())
        QSKIP("Utils was built without Filesystem Engine");

    DeviceFileHooks &deviceHooks = DeviceFileHooks::instance();

    deviceHooks.fileContents =
        [](const FilePath &path, qint64 maxSize, qint64 offset) -> QByteArray {
        return FilePath::fromString(path.path()).fileContents(maxSize, offset).value_or(QByteArray());
    };

    deviceHooks.isExecutableFile = [](const FilePath &filePath) {
        return FilePath::fromString(filePath.path()).isExecutableFile();
    };
    deviceHooks.isReadableFile = [](const FilePath &filePath) {
        return FilePath::fromString(filePath.path()).isReadableFile();
    };
    deviceHooks.isReadableDir = [](const FilePath &filePath) {
        return FilePath::fromString(filePath.path()).isReadableDir();
    };
    deviceHooks.isWritableDir = [](const FilePath &filePath) {
        return FilePath::fromString(filePath.path()).isWritableDir();
    };
    deviceHooks.isWritableFile = [](const FilePath &filePath) {
        return FilePath::fromString(filePath.path()).isWritableFile();
    };
    deviceHooks.isFile = [](const FilePath &filePath) {
        return FilePath::fromString(filePath.path()).isFile();
    };
    deviceHooks.isDir = [](const FilePath &filePath) {
        return FilePath::fromString(filePath.path()).isDir();
    };
    deviceHooks.ensureWritableDir = [](const FilePath &filePath) {
        return FilePath::fromString(filePath.path()).ensureWritableDir();
    };
    deviceHooks.ensureExistingFile = [](const FilePath &filePath) {
        return FilePath::fromString(filePath.path()).ensureExistingFile();
    };
    deviceHooks.createDir = [](const FilePath &filePath) {
        return FilePath::fromString(filePath.path()).createDir();
    };
    deviceHooks.exists = [](const FilePath &filePath) {
        return FilePath::fromString(filePath.path()).exists();
    };
    deviceHooks.removeFile = [](const FilePath &filePath) {
        return FilePath::fromString(filePath.path()).removeFile();
    };
    deviceHooks.removeRecursively = [](const FilePath &filePath) {
        return FilePath::fromString(filePath.path()).removeRecursively();
    };
    deviceHooks.copyFile = [](const FilePath &filePath, const FilePath &target) {
        return FilePath::fromString(filePath.path()).copyFile(target);
    };
    deviceHooks.renameFile = [](const FilePath &filePath, const FilePath &target) {
        return FilePath::fromString(filePath.path()).renameFile(target);
    };
    deviceHooks.searchInPath = [](const FilePath &filePath, const FilePaths &dirs) {
        return FilePath::fromString(filePath.path()).searchInPath(dirs);
    };
    deviceHooks.symLinkTarget = [](const FilePath &filePath) {
        return FilePath::fromString(filePath.path()).symLinkTarget();
    };
    deviceHooks.iterateDirectory = [](const FilePath &filePath,
                                      const std::function<bool(const FilePath &)> &callBack,
                                      const FileFilter &filter) {
        return FilePath::fromString(filePath.path())
            .iterateDirectory(
                [&filePath, &callBack](const FilePath &path) -> bool {
                    const FilePath devicePath = path.onDevice(filePath);

                    return callBack(devicePath);
                },
                filter);
    };
    deviceHooks.asyncFileContents = [](const Continuation<const std::optional<QByteArray> &> &cont,
                                       const FilePath &filePath,
                                       qint64 maxSize,
                                       qint64 offset) {
        return FilePath::fromString(filePath.path()).asyncFileContents(cont, maxSize, offset);
    };
    deviceHooks.writeFileContents = [](const FilePath &filePath, const QByteArray &data) {
        return FilePath::fromString(filePath.path()).writeFileContents(data);
    };
    deviceHooks.lastModified = [](const FilePath &filePath) {
        return FilePath::fromString(filePath.path()).lastModified();
    };
    deviceHooks.permissions = [](const FilePath &filePath) {
        return FilePath::fromString(filePath.path()).permissions();
    };
    deviceHooks.setPermissions = [](const FilePath &filePath, QFile::Permissions permissions) {
        return FilePath::fromString(filePath.path()).setPermissions(permissions);
    };
    deviceHooks.osType = [](const FilePath &filePath) {
        return FilePath::fromString(filePath.path()).osType();
    };
    // deviceHooks.environment = [](const FilePath &filePath) -> Environment {return {};};
    deviceHooks.fileSize = [](const FilePath &filePath) {
        return FilePath::fromString(filePath.path()).fileSize();
    };
    deviceHooks.bytesAvailable = [](const FilePath &filePath) {
        return FilePath::fromString(filePath.path()).bytesAvailable();
    };

    deviceHooks.mapToDevicePath = [](const FilePath &filePath) { return filePath.path(); };

    FSEngine::addDevice(FilePath::fromString("device://test"));

    tempFolder = QDir::tempPath();
    QDir testFolder(QString("%1/tst_fsengine").arg(tempFolder));
    if (testFolder.exists())
        QVERIFY(testFolder.removeRecursively());

    QDir(tempFolder).mkdir("tst_fsengine");
}

void tst_fsengine::testFilePathFromToString()
{
    FilePath p = FilePath::fromString("device://test/test.txt");
    QCOMPARE(p.scheme(), u"device");
    QCOMPARE(p.host(), u"test");
    QCOMPARE(p.path(), u"/test.txt");

    QString asString = p.toString();
    QCOMPARE(asString,
             FilePath::specialPath(FilePath::SpecialPathComponent::DeviceRootPath)
                 + "/test/test.txt");

    FilePath p2 = FilePath::fromString(asString);
    QCOMPARE(p.scheme(), u"device");
    QCOMPARE(p.host(), u"test");
    QCOMPARE(p.path(), u"/test.txt");
}

void tst_fsengine::testRootPathContainsFakeDir()
{
    const auto rootList = QDir::root().entryList();
    QVERIFY(rootList.contains(FilePath::specialPath(FilePath::SpecialPathComponent::RootName)));

    QDir schemes(FilePath::specialPath(FilePath::SpecialPathComponent::RootPath));
    const auto schemeList = schemes.entryList();
    QVERIFY(schemeList.contains("device"));

    QDir deviceRoot(FilePath::specialPath(FilePath::SpecialPathComponent::DeviceRootPath) + "/test" + startWithSlash(QDir::rootPath()));
    const auto deviceRootList = deviceRoot.entryList();
    QVERIFY(!deviceRootList.isEmpty());
}

void tst_fsengine::testNotExistingFile()
{
    QFile f(makeTestPath("test-does-not-exist.txt"));

    QCOMPARE(f.open(QIODevice::ReadOnly), false);
}

void tst_fsengine::testCreateFile()
{
    {
        QFile f(makeTestPath("test-create-file.txt"));
        QCOMPARE(f.exists(), false);
        QVERIFY(f.open(QIODevice::WriteOnly));
    }

    QFile f(makeTestPath("test-create-file.txt"));
    QCOMPARE(f.exists(), true);
}

void tst_fsengine::testCreateDir()
{
    QDir d(makeTestPath({}));
    QCOMPARE(d.mkdir("test-create-dir"), true);
}

QString tst_fsengine::makeTestPath(QString path, bool asUrl)
{
    if (asUrl) {
        return QString("device://test%1/tst_fsengine/%2").arg(tempFolder, path);
    }

    return QString(FilePath::specialPath(FilePath::SpecialPathComponent::DeviceRootPath)
                   + "/test%1/tst_fsengine/%2")
        .arg(startWithSlash(tempFolder), path);
}

void tst_fsengine::testListDir()
{
    QDir dd(makeTestPath({}));
    QCOMPARE(dd.mkdir("test-list-dir"), true);

    QDir d(makeTestPath("test-list-dir"));

    {
        QFile f(makeTestPath("test-list-dir/f1.txt"));
        QVERIFY(f.open(QIODevice::WriteOnly));
    }

    const auto list = d.entryList();
    QVERIFY(list.contains("f1.txt"));
}

void tst_fsengine::testWindowsPaths()
{
    if (!HostOsInfo::isWindowsHost())
        QSKIP("This test is only valid on windows");

    // Test upper-case "C:"
    QVERIFY(FilePath::fromString("C:/__qtc_devices__/device/{cd6c7e4b-12fd-43ca-9bb2-053a38e6b7c5}")
                .needsDevice());

    // Test lower-case "C:"
    QVERIFY(FilePath::fromString("c:/__qtc_devices__/device/{cd6c7e4b-12fd-43ca-9bb2-053a38e6b7c5}")
                .needsDevice());
}

void tst_fsengine::testUrl()
{
    FilePath p = FilePath::fromString(makeTestPath("", true));

    QVERIFY(p.needsDevice());
}

void tst_fsengine::testBrokenWindowsPath()
{
    QTemporaryFile tmp;
    QVERIFY(tmp.open());

    QString localFileName = tmp.fileName();
    localFileName.insert(HostOsInfo::isWindowsHost() ? 2 : 0, '/');

    QFile file(localFileName);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(tmp.fileName(), QFileInfo(localFileName).canonicalFilePath());
}

QTEST_GUILESS_MAIN(tst_fsengine)
#include "tst_fsengine.moc"
