// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <ioutils.h>
#include <qmakeglobals.h>

#include <QDir>
#include <QTest>

using namespace QMakeInternal;

// Distinct per OS, as the OS of a device root is remembered process-wide.
const QString windowsRoot = "/__qtc_devices__/ssh/user@windows/";
const QString unixRoot = "/__qtc_devices__/ssh/user@unix/";

class tst_ProParser : public QObject
{
    Q_OBJECT

private slots:
    void walkUpOnWindowsDevice();
    void walkUpOnUnixDevice();
    void walkUpLocally();
    void separatorsFollowTheDevice();
    void drivePathIsAbsoluteOnWindowsDevice();
    void pathListSplitsWithDeviceSemantics();
};

// Collects the directories a walk-up visits, bounded so a walk that does not terminate fails
// instead of hanging the test.
static QStringList walkUp(const QString &device, const QString &start)
{
    QStringList visited;
    for (QString dir = start; !dir.isEmpty(); dir = IoUtils::parentPath(device, dir)) {
        visited << dir;
        if (visited.size() > 16)
            return {"walk-up does not terminate"};
    }
    return visited;
}

void tst_ProParser::walkUpOnWindowsDevice()
{
    IoUtils::setDeviceOsIsWindows(windowsRoot, true);

    QCOMPARE(walkUp(windowsRoot, "C:/a/b/c"), QStringList({"C:/a/b/c", "C:/a/b", "C:/a", "C:/"}));

    // A path without a drive, and one relative to a drive's current directory, are both what a
    // Windows device calls relative, and must still end.
    QCOMPARE(walkUp(windowsRoot, "/a"), QStringList({"/a", "/"}));
    QCOMPARE(walkUp(windowsRoot, "a"), QStringList({"a"}));
    QCOMPARE(walkUp(windowsRoot, "C:a/b"), QStringList({"C:a/b", "C:a"}));
}

void tst_ProParser::walkUpOnUnixDevice()
{
    IoUtils::setDeviceOsIsWindows(unixRoot, false);

    QCOMPARE(walkUp(unixRoot, "/a/b"), QStringList({"/a/b", "/a", "/"}));
}

void tst_ProParser::walkUpLocally()
{
    const QString root = QDir::rootPath();

    QCOMPARE(IoUtils::parentPath({}, root), QString());
    QCOMPARE(IoUtils::parentPath({}, root + "a/b"), root + "a");
}

void tst_ProParser::separatorsFollowTheDevice()
{
    QMakeGlobals windows;
    windows.setDevice(windowsRoot, true);
    QCOMPARE(windows.dirlist_sep, QString(";"));
    QCOMPARE(windows.dir_sep, QString("\\"));

    QMakeGlobals posix;
    posix.setDevice(unixRoot, false);
    QCOMPARE(posix.dirlist_sep, QString(":"));
    QCOMPARE(posix.dir_sep, QString("/"));

    QMakeGlobals local;
    const QString hostListSeparator = local.dirlist_sep;
    local.setDevice({}, true);
    QCOMPARE(local.dirlist_sep, hostListSeparator);
}

void tst_ProParser::drivePathIsAbsoluteOnWindowsDevice()
{
    IoUtils::setDeviceOsIsWindows(windowsRoot, true);

    QVERIFY(IoUtils::isAbsolutePath(windowsRoot, "C:/Qt/6.12.0"));
    QVERIFY(IoUtils::isRelativePath(windowsRoot, "/Qt/6.12.0"));
    QVERIFY(IoUtils::isRelativePath(windowsRoot, "C:Qt/6.12.0"));
}

void tst_ProParser::pathListSplitsWithDeviceSemantics()
{
    QMakeGlobals windows;
    windows.setDevice(windowsRoot, true);
    QCOMPARE(windows.splitPathList("C:/Qt/6.12.0/mingw_64;C:/Qt/6.12.0/msvc2022_64"),
             QStringList({"C:/Qt/6.12.0/mingw_64", "C:/Qt/6.12.0/msvc2022_64"}));

    QMakeGlobals posix;
    posix.setDevice(unixRoot, false);
    QCOMPARE(posix.splitPathList("/opt/qt/6.12.0/gcc_64:/opt/qt/6.12.0/clang_64"),
             QStringList({"/opt/qt/6.12.0/gcc_64", "/opt/qt/6.12.0/clang_64"}));
}

QTEST_GUILESS_MAIN(tst_ProParser)

#include "tst_proparser.moc"
