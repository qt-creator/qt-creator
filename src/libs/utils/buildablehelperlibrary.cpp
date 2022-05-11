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
#include "environment.h"
#include "hostosinfo.h"
#include "qtcprocess.h"

#include <QDebug>
#include <QDir>
#include <QRegularExpression>

#include <set>

namespace Utils {

bool BuildableHelperLibrary::isQtChooser(const FilePath &filePath)
{
    return filePath.symLinkTarget().endsWith("/qtchooser");
}

static const QStringList &queryToolNames()
{
    static const QStringList names = {"qmake", "qtpaths"};
    return names;
}

static bool isQueryTool(FilePath path)
{
    if (path.isEmpty())
        return false;
    if (BuildableHelperLibrary::isQtChooser(path))
        path = BuildableHelperLibrary::qtChooserToQueryToolPath(path.symLinkTarget());
    if (!path.exists())
        return false;
    QtcProcess proc;
    proc.setCommand({path, {"-query"}});
    proc.runBlocking();
    if (proc.result() != ProcessResult::FinishedWithSuccess)
        return false;
    const QString output = proc.stdOut();
    // Exemplary output of "[qmake|qtpaths] -query":
    // QT_SYSROOT:...
    // QT_INSTALL_PREFIX:...
    // [...]
    // QT_VERSION:6.4.0
    return output.contains("QT_VERSION:");
}

static FilePath findQueryToolInDir(const FilePath &dir)
{
    if (dir.isEmpty())
        return {};

    for (const QString &queryTool : queryToolNames()) {
        FilePath queryToolPath = dir.pathAppended(queryTool).withExecutableSuffix();
        if (queryToolPath.exists()) {
            if (isQueryTool(queryToolPath))
                return queryToolPath;
        }

        // Prefer qmake-qt5 to qmake-qt4 by sorting the filenames in reverse order.
        const FilePaths candidates = dir.dirEntries(
                    {BuildableHelperLibrary::possibleQtQueryTools(queryTool), QDir::Files},
                    QDir::Name | QDir::Reversed);
        for (const FilePath &candidate : candidates) {
            if (candidate == queryToolPath)
                continue;
            if (isQueryTool(candidate))
                return candidate;
        }
    }
    return {};
}

FilePath BuildableHelperLibrary::qtChooserToQueryToolPath(const FilePath &qtChooser)
{
    const QString toolDir = QLatin1String("QTTOOLDIR=\"");
    QtcProcess proc;
    proc.setTimeoutS(1);
    proc.setCommand({qtChooser, {"-print-env"}});
    proc.runBlocking();
    if (proc.result() != ProcessResult::FinishedWithSuccess)
        return {};
    const QString output = proc.stdOut();
    // Exemplary output of "qtchooser -print-env":
    // QT_SELECT="default"
    // QTTOOLDIR="/usr/lib/qt5/bin"
    // QTLIBDIR="/usr/lib/x86_64-linux-gnu"
    int pos = output.indexOf(toolDir);
    if (pos == -1)
        return {};
    pos += toolDir.count();
    int end = output.indexOf('\"', pos);
    if (end == -1)
        return {};

    FilePath queryToolPath = qtChooser;
    for (const QString &queryTool : queryToolNames()) {
        queryToolPath.setPath(output.mid(pos, end - pos) + "/" + queryTool);
        if (queryToolPath.exists())
            return queryToolPath;
    }
    return queryToolPath;
}

FilePath BuildableHelperLibrary::findSystemQt(const Environment &env)
{
    const FilePaths list = findQtsInEnvironment(env, 1);
    return list.size() == 1 ? list.first() : FilePath();
}

FilePaths BuildableHelperLibrary::findQtsInEnvironment(const Environment &env, int maxCount)
{
    FilePaths queryToolList;
    std::set<QString> canonicalEnvPaths;
    const FilePaths paths = env.path();
    for (const FilePath &path : paths) {
        if (!canonicalEnvPaths.insert(path.toFileInfo().canonicalFilePath()).second)
            continue;
        const FilePath queryTool = findQueryToolInDir(path);
        if (queryTool.isEmpty())
            continue;
        queryToolList << queryTool;
        if (maxCount != -1 && queryToolList.size() == maxCount)
            break;
    }
    return queryToolList;
}

QString BuildableHelperLibrary::filterForQtQueryToolsFileDialog()
{
    QStringList toolFilters;
    for (const QString &queryTool : queryToolNames()) {
        for (const QString &tool: BuildableHelperLibrary::possibleQtQueryTools(queryTool)) {
            QString toolFilter;
            if (HostOsInfo::isMacHost())
                // work around QTBUG-7739 that prohibits filters that don't start with *
                toolFilter += QLatin1Char('*');
            toolFilter += tool;
            if (HostOsInfo::isAnyUnixHost() && !HostOsInfo::isMacHost())
                // kde bug, we need at least one wildcard character
                // see QTCREATORBUG-7771
                toolFilter += QLatin1Char('*');
            toolFilters.append(toolFilter);
        }
    }
    return queryToolNames().join(", ") + " (" + toolFilters.join(" ") + ")";
}

QStringList BuildableHelperLibrary::possibleQtQueryTools(const QString &tool)
{
    // On Windows it is "<queryTool>.exe" or "<queryTool>.bat"
    // On Unix some distributions renamed qmake with a postfix to avoid clashes
    // On OS X, Qt 4 binary packages also has renamed qmake. There are also symbolic links that are
    // named <tool>, but the file dialog always checks against resolved links (native Cocoa issue)
    QStringList tools(HostOsInfo::withExecutableSuffix(tool + "*"));

    // Qt 6 CMake built targets, such as Android, are dependent on the host installation
    // and use a script wrapper around the host queryTool executable
    if (HostOsInfo::isWindowsHost())
        tools.append(tool + "*.bat");
    return tools;
}

} // namespace Utils
