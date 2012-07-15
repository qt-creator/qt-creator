/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "debuggerprofileinformation.h"

#include "debuggerprofileconfigwidget.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>

#include <utils/environment.h>

#include <QDir>
#include <QPair>

// --------------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------------

static QPair<QString, QString> autoDetectCdbDebugger()
{
    QPair<QString, QString> result;
    QList<Utils::FileName> cdbs;

    QStringList programDirs;
    programDirs.append(QString::fromLocal8Bit(qgetenv("ProgramFiles")));
    programDirs.append(QString::fromLocal8Bit(qgetenv("ProgramFiles(x86)")));
    programDirs.append(QString::fromLocal8Bit(qgetenv("ProgramW6432")));

    foreach (const QString &dirName, programDirs) {
        if (dirName.isEmpty())
            continue;
        QDir dir(dirName);
        // Windows SDK's starting from version 8 live in
        // "ProgramDir\Windows Kits\<version>"
        const QString windowsKitsFolderName = QLatin1String("Windows Kits");
        if (dir.exists(windowsKitsFolderName)) {
            QDir windowKitsFolder = dir;
            if (windowKitsFolder.cd(windowsKitsFolderName)) {
                // Check in reverse order (latest first)
                const QFileInfoList kitFolders =
                    windowKitsFolder.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot,
                                                   QDir::Time | QDir::Reversed);
                foreach (const QFileInfo &kitFolderFi, kitFolders) {
                    const QString path = kitFolderFi.absoluteFilePath();
                    const QFileInfo cdb32(path + QLatin1String("/Debuggers/x86/cdb.exe"));
                    if (cdb32.isExecutable())
                        cdbs.push_back(Utils::FileName::fromString(cdb32.absoluteFilePath()));
                    const QFileInfo cdb64(path + QLatin1String("/Debuggers/x64/cdb.exe"));
                    if (cdb64.isExecutable())
                        cdbs.push_back(Utils::FileName::fromString(cdb64.absoluteFilePath()));
                } // for Kits
            } // can cd to "Windows Kits"
        } // "Windows Kits" exists

        // Pre Windows SDK 8: Check 'Debugging Tools for Windows'
        foreach (const QFileInfo &fi, dir.entryInfoList(QStringList(QLatin1String("Debugging Tools for Windows*")),
                                                        QDir::Dirs | QDir::NoDotAndDotDot)) {
            Utils::FileName filePath(fi);
            filePath.appendPath(QLatin1String("cdb.exe"));
            if (!cdbs.contains(filePath))
                cdbs.append(filePath);
        }
    }

    foreach (const Utils::FileName &cdb, cdbs) {
        QList<ProjectExplorer::Abi> abis = ProjectExplorer::Abi::abisOfBinary(cdb);
        if (abis.isEmpty())
            continue;
        if (abis.first().wordWidth() == 32)
            result.first = cdb.toString();
        else if (abis.first().wordWidth() == 64)
            result.second = cdb.toString();
    }

    // prefer 64bit debugger, even for 32bit binaries:
    if (!result.second.isEmpty())
        result.first = result.second;

    return result;
}

namespace Debugger {

// --------------------------------------------------------------------------
// DebuggerProfileInformation:
// --------------------------------------------------------------------------

static const char DEBUGGER_INFORMATION[] = "Debugger.Information";

DebuggerProfileInformation::DebuggerProfileInformation()
{
    setObjectName(QLatin1String("DebuggerProfileInformation"));
}

Core::Id DebuggerProfileInformation::dataId() const
{
    static Core::Id id = Core::Id(DEBUGGER_INFORMATION);
    return id;
}

unsigned int DebuggerProfileInformation::priority() const
{
    return 28000;
}

QVariant DebuggerProfileInformation::defaultValue(ProjectExplorer::Profile *p) const
{
    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainProfileInformation::toolChain(p);
    ProjectExplorer::Abi abi = ProjectExplorer::Abi::hostAbi();
    if (tc)
        abi = tc->targetAbi();

    // CDB for windows:
    if (abi.os() == ProjectExplorer::Abi::WindowsOS
            && abi.osFlavor() != ProjectExplorer::Abi::WindowsMSysFlavor) {
        QPair<QString, QString> cdbs = autoDetectCdbDebugger();
        return (abi.wordWidth() == 32) ? cdbs.first : cdbs.second;
    }

    // fall back to system GDB:
    QString debugger = QLatin1String("gdb");
    if (tc) {
        // Check suggestions from the SDK:
        const QString path = tc->suggestedDebugger().toString();
        if (!path.isEmpty()) {
            QFileInfo fi(path);
            if (fi.isAbsolute())
                return path;
            debugger = path;
        }
    }

    Utils::Environment env = Utils::Environment::systemEnvironment();
    return env.searchInPath(debugger);
}

QList<ProjectExplorer::Task> DebuggerProfileInformation::validate(ProjectExplorer::Profile *p) const
{
    QList<ProjectExplorer::Task> result;
    Utils::FileName dbg = debuggerCommand(p);
    if (dbg.isEmpty()) {
        result << ProjectExplorer::Task(ProjectExplorer::Task::Warning,
                                        tr("No debugger set up."), Utils::FileName(), -1,
                                        Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        return result;
    }

    QFileInfo fi = dbg.toFileInfo();
    if (!fi.exists() || fi.isDir())
        result << ProjectExplorer::Task(ProjectExplorer::Task::Error,
                                        tr("Debugger not found."), Utils::FileName(), -1,
                                        Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    else if (!fi.isExecutable())
        result << ProjectExplorer::Task(ProjectExplorer::Task::Error,
                                        tr("Debugger not exectutable."), Utils::FileName(), -1,
                                        Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));

    return result;
}

ProjectExplorer::ProfileConfigWidget *
DebuggerProfileInformation::createConfigWidget(ProjectExplorer::Profile *p) const
{
    return new Internal::DebuggerProfileConfigWidget(p, this);
}

ProjectExplorer::ProfileInformation::ItemList DebuggerProfileInformation::toUserOutput(ProjectExplorer::Profile *p) const
{
    return ItemList() << qMakePair(tr("Debugger"), debuggerCommand(p).toUserOutput());
}

Utils::FileName DebuggerProfileInformation::debuggerCommand(const ProjectExplorer::Profile *p)
{
    return Utils::FileName::fromString(p->value(Core::Id(DEBUGGER_INFORMATION)).toString());
}

void DebuggerProfileInformation::setDebuggerCommand(ProjectExplorer::Profile *p, const Utils::FileName &command)
{
    p->setValue(Core::Id(DEBUGGER_INFORMATION), command.toString());
}

} // namespace Debugger
