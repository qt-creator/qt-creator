// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "buildablehelperlibrary.h"

#include "algorithm.h"
#include "environment.h"
#include "hostosinfo.h"
#include "qtcprocess.h"

#include <QDebug>
#include <QRegularExpression>

#include <set>

using namespace std::chrono_literals;

namespace Utils {

bool BuildableHelperLibrary::isQtChooser(const FilePath &filePath)
{
    return filePath.symLinkTarget().endsWith("/qtchooser");
}

FilePath BuildableHelperLibrary::qtChooserToQmakePath(const FilePath &qtChooser)
{
    const QString toolDir = QLatin1String("QTTOOLDIR=\"");
    Process proc;
    proc.setEnvironment(qtChooser.deviceEnvironment());
    proc.setCommand({qtChooser, {"-print-env"}});
    proc.runBlocking(1s);
    if (proc.result() != ProcessResult::FinishedWithSuccess)
        return {};
    const QString output = proc.cleanedStdOut();
    int pos = output.indexOf(toolDir);
    if (pos == -1)
        return {};
    pos += toolDir.size();
    int end = output.indexOf('\"', pos);
    if (end == -1)
        return {};

    return qtChooser.withNewPath(output.mid(pos, end - pos) + "/qmake");
}

static bool isQmake(FilePath path)
{
    if (!path.isExecutableFile())
        return false;
    if (BuildableHelperLibrary::isQtChooser(path))
        path = BuildableHelperLibrary::qtChooserToQmakePath(path.symLinkTarget());
    if (!path.exists())
        return false;
    return !BuildableHelperLibrary::qtVersionForQMake(path).isEmpty();
}

static FilePaths findQmakesInDir(const FilePath &dir)
{
    if (dir.isEmpty())
        return {};

    // Prefer qmake-qt5 to qmake-qt4 by sorting the filenames in reverse order.
    FilePaths qmakes;
    std::set<FilePath> canonicalQmakes;
    const FilePaths candidates = dir.dirEntries(
                {BuildableHelperLibrary::possibleQMakeCommands(), QDir::Files},
                QDir::Name | QDir::Reversed);

    const auto probablyMatchesExistingQmake = [&](const FilePath &qmake) {
        // This deals with symlinks.
        if (!canonicalQmakes.insert(qmake).second)
            return true;

        // Symlinks have high potential for false positives in file size check, so skip that one.
        if (qmake.isSymLink())
            return false;

        // This deals with the case where copies are shipped instead of symlinks
        // (Windows, Qt installer, ...).
        return contains(qmakes, [size = qmake.fileSize()](const FilePath &existing) {
            return existing.fileSize() == size;
        });
    };

    for (const FilePath &candidate : candidates) {
        if (isQmake(candidate) && !probablyMatchesExistingQmake(candidate))
            qmakes << candidate;
    }
    return qmakes;
}

FilePaths BuildableHelperLibrary::findQtsInEnvironment(const Environment &env)
{
    FilePaths qmakeList;
    std::set<FilePath> canonicalEnvPaths;
    const FilePaths paths = env.path();
    for (const FilePath &path : paths) {
        if (!canonicalEnvPaths.insert(path.canonicalPath()).second)
            continue;
        qmakeList << findQmakesInDir(path);
    }
    return qmakeList;
}

QString BuildableHelperLibrary::qtVersionForQMake(const FilePath &qmakePath)
{
    if (qmakePath.isEmpty())
        return QString();

    Process qmake;
    qmake.setEnvironment(qmakePath.deviceEnvironment());
    qmake.setCommand({qmakePath, {"--version"}});
    qmake.runBlocking(5s);
    if (qmake.result() != ProcessResult::FinishedWithSuccess) {
        qWarning() << qmake.exitMessage();
        return QString();
    }

    const QString output = qmake.allOutput();
    static const QRegularExpression regexp("(QMake version:?)[\\s]*([\\d.]*)",
                                           QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = regexp.match(output);
    const QString qmakeVersion = match.captured(2);
    if (qmakeVersion.startsWith(QLatin1String("2."))
            || qmakeVersion.startsWith(QLatin1String("3."))) {
        static const QRegularExpression regexp2("Using Qt version[\\s]*([\\d\\.]*)",
                                                QRegularExpression::CaseInsensitiveOption);
        const QRegularExpressionMatch match2 = regexp2.match(output);
        const QString version = match2.captured(1);
        return version;
    }
    return QString();
}

QString BuildableHelperLibrary::filterForQmakeFileDialog()
{
    QString filter = QLatin1String("qmake (");
    const QStringList commands = possibleQMakeCommands();
    for (int i = 0; i < commands.size(); ++i) {
        if (i)
            filter += QLatin1Char(' ');
        if (HostOsInfo::isMacHost())
            // work around QTBUG-7739 that prohibits filters that don't start with *
            filter += QLatin1Char('*');
        filter += commands.at(i);
        if (HostOsInfo::isAnyUnixHost() && !HostOsInfo::isMacHost())
            // kde bug, we need at least one wildcard character
            // see QTCREATORBUG-7771
            filter += QLatin1Char('*');
    }
    filter += QLatin1Char(')');
    return filter;
}


QStringList BuildableHelperLibrary::possibleQMakeCommands()
{
    // On Windows it is always "qmake.exe"
    // On Unix some distributions renamed qmake with a postfix to avoid clashes
    // On OS X, Qt 4 binary packages also has renamed qmake. There are also symbolic links that are
    // named "qmake", but the file dialog always checks against resolved links (native Cocoa issue)
    QStringList commands(HostOsInfo::withExecutableSuffix("qmake*"));

    // Qt 6 CMake built targets, such as Android, are dependent on the host installation
    // and use a script wrapper around the host qmake executable
    if (HostOsInfo::isWindowsHost())
        commands.append("qmake*.bat");
    return commands;
}

} // namespace Utils
