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

#include "buildablehelperlibrary.h"
#include "hostosinfo.h"
#include "qtcprocess.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QRegularExpression>

#include <set>

namespace Utils {

bool BuildableHelperLibrary::isQtChooser(const FilePath &filePath)
{
    return filePath.symLinkTarget().endsWith("/qtchooser");
}

FilePath BuildableHelperLibrary::qtChooserToQmakePath(const FilePath &qtChooser)
{
    const QString toolDir = QLatin1String("QTTOOLDIR=\"");
    QtcProcess proc;
    proc.setTimeoutS(1);
    proc.setCommand({qtChooser, {"-print-env"}});
    proc.runBlocking();
    if (proc.result() != QtcProcess::FinishedWithSuccess)
        return {};
    const QString output = proc.stdOut();
    int pos = output.indexOf(toolDir);
    if (pos == -1)
        return {};
    pos += toolDir.count();
    int end = output.indexOf('\"', pos);
    if (end == -1)
        return {};

    FilePath qmake = qtChooser;
    qmake.setPath(output.mid(pos, end - pos) + "/qmake");
    return qmake;
}

static bool isQmake(FilePath path)
{
    if (path.isEmpty())
        return false;
    if (BuildableHelperLibrary::isQtChooser(path))
        path = BuildableHelperLibrary::qtChooserToQmakePath(path.symLinkTarget());
    if (!path.exists())
        return false;
    return !BuildableHelperLibrary::qtVersionForQMake(path).isEmpty();
}

static FilePath findQmakeInDir(const FilePath &dir)
{
    if (dir.isEmpty())
        return {};

    FilePath qmakePath = dir.pathAppended("qmake").withExecutableSuffix();
    if (qmakePath.exists()) {
        if (isQmake(qmakePath))
            return qmakePath;
    }

    // Prefer qmake-qt5 to qmake-qt4 by sorting the filenames in reverse order.
    const FilePaths candidates = dir.dirEntries(
                BuildableHelperLibrary::possibleQMakeCommands(),
                QDir::Files,
                QDir::Name | QDir::Reversed);
    for (const FilePath &candidate : candidates) {
        if (candidate == qmakePath)
            continue;
        if (isQmake(candidate))
            return candidate;
    }
    return {};
}

FilePath BuildableHelperLibrary::findSystemQt(const Environment &env)
{
    const FilePaths list = findQtsInEnvironment(env, 1);
    return list.size() == 1 ? list.first() : FilePath();
}

FilePaths BuildableHelperLibrary::findQtsInEnvironment(const Environment &env, int maxCount)
{
    FilePaths qmakeList;
    std::set<QString> canonicalEnvPaths;
    const FilePaths paths = env.path();
    for (const FilePath &path : paths) {
        if (!canonicalEnvPaths.insert(path.toFileInfo().canonicalFilePath()).second)
            continue;
        const FilePath qmake = findQmakeInDir(path);
        if (qmake.isEmpty())
            continue;
        qmakeList << qmake;
        if (maxCount != -1 && qmakeList.size() == maxCount)
            break;
    }
    return qmakeList;
}

QString BuildableHelperLibrary::qtVersionForQMake(const FilePath &qmakePath)
{
    if (qmakePath.isEmpty())
        return QString();

    QtcProcess qmake;
    qmake.setTimeoutS(5);
    qmake.setCommand({qmakePath, {"--version"}});
    qmake.runBlocking();
    if (qmake.result() != QtcProcess::FinishedWithSuccess) {
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
