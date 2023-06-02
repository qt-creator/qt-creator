// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/filepath.h>
#include <utils/fsengine/fsengine.h>
#include <utils/environment.h>
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
    void testRead();
    void testWrite();
    void testRootFromDotDot();
    void testDirtyPaths();

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

    if (HostOsInfo::isWindowsHost())
        QSKIP("The fsengine tests are not supported on Windows.");

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

    QString asString = p.toFSPathString();
    QCOMPARE(asString, FilePath::specialDeviceRootPath() + "/test/test.txt");

    FilePath p2 = FilePath::fromString(asString);
    QCOMPARE(p2.scheme(), u"device");
    QCOMPARE(p2.host(), u"test");
    QCOMPARE(p2.path(), u"/test.txt");
}

void tst_fsengine::testRootPathContainsFakeDir()
{
    const QStringList rootList = QDir::root().entryList();
    QVERIFY(rootList.contains(FilePath::specialRootName()));

    QDir schemes(FilePath::specialRootPath());
    const QStringList schemeList = schemes.entryList();
    QVERIFY(schemeList.contains("device"));

    QDir devices(FilePath::specialDeviceRootPath());
    const QStringList deviceList = devices.entryList();
    QVERIFY(deviceList.contains("test"));

    QDir deviceRoot(FilePath::specialDeviceRootPath() + "/test" + startWithSlash(QDir::rootPath()));
    const QStringList deviceRootList = deviceRoot.entryList();
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
    if (asUrl)
        return QString("device://test%1/tst_fsengine/%2").arg(tempFolder, path);

    return QString(FilePath::specialDeviceRootPath() + "/test%1/tst_fsengine/%2")
        .arg(startWithSlash(tempFolder))
        .arg(path);
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

    const QStringList list = d.entryList();
    QVERIFY(list.contains("f1.txt"));
}

void tst_fsengine::testWindowsPaths()
{
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

void tst_fsengine::testRead()
{
    QTemporaryFile tmp;
    QVERIFY(tmp.open());

    const QByteArray data = "Hello World!";

    tmp.write(data);
    tmp.flush();

    QFile file(FilePath::specialDeviceRootPath() + "/test" + tmp.fileName());
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), data);
}

void tst_fsengine::testWrite()
{
    QTemporaryDir dir;
    const QString path = dir.path() + "/testWrite.txt";
    const QByteArray data = "Hello World!";
    {
        QFile file(FilePath::specialDeviceRootPath() + "/test" + path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(data), qint64(data.size()));
    }
    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), data);
}

void tst_fsengine::testRootFromDotDot()
{
    const QString path = QDir::rootPath() + "some-folder/..";
    QFileInfo fInfo(path);

    QCOMPARE(fInfo.fileName(), QString(".."));

    QDir dRoot(path);
    const auto dRootEntryList = dRoot.entryList();
    QVERIFY(dRootEntryList.contains(FilePath::specialRootName()));

    QFileInfo fInfo2(FilePath::specialRootPath() + "/xyz/..");
    QCOMPARE(fInfo2.fileName(), "..");

    QDir schemesWithDotDot(FilePath::specialRootPath() + "/xyz/..");
    const QStringList schemeWithDotDotList = schemesWithDotDot.entryList();
    QVERIFY(schemeWithDotDotList.contains("device"));

    QFileInfo fInfo3(FilePath::specialDeviceRootPath() + "/xyz/..");
    QCOMPARE(fInfo3.fileName(), "..");

    QDir devicesWithDotDot(FilePath::specialDeviceRootPath() + "/test/..");
    const QStringList deviceListWithDotDot = devicesWithDotDot.entryList();
    QVERIFY(deviceListWithDotDot.contains("test"));

    QFileInfo fInfo4(FilePath::specialDeviceRootPath() + "/test/tmp/..");
    QCOMPARE(fInfo4.fileName(), "..");
}

void tst_fsengine::testDirtyPaths()
{
    // "//__qtc_devices"
    QVERIFY(QFileInfo("/" + FilePath::specialRootPath()).exists());

    // "///__qtc_devices/device"
    QVERIFY(QFileInfo("//" + FilePath::specialDeviceRootPath()).exists());

    // "////__qtc_devices/device////test"
    QVERIFY(QFileInfo("///" + FilePath::specialDeviceRootPath() + "////test").exists());

    // "/////__qtc_devices/device/test/..."
    QVERIFY(QFileInfo("////" + makeTestPath("")).exists());
}

QTEST_GUILESS_MAIN(tst_fsengine)
#include "tst_fsengine.moc"
