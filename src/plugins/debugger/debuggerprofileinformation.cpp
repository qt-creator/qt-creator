/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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

using namespace ProjectExplorer;
using namespace Utils;

// --------------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------------

static QPair<QString, QString> autoDetectCdbDebugger()
{
    QPair<QString, QString> result;
    QList<FileName> cdbs;

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
                        cdbs.push_back(FileName::fromString(cdb32.absoluteFilePath()));
                    const QFileInfo cdb64(path + QLatin1String("/Debuggers/x64/cdb.exe"));
                    if (cdb64.isExecutable())
                        cdbs.push_back(FileName::fromString(cdb64.absoluteFilePath()));
                } // for Kits
            } // can cd to "Windows Kits"
        } // "Windows Kits" exists

        // Pre Windows SDK 8: Check 'Debugging Tools for Windows'
        foreach (const QFileInfo &fi, dir.entryInfoList(QStringList(QLatin1String("Debugging Tools for Windows*")),
                                                        QDir::Dirs | QDir::NoDotAndDotDot)) {
            FileName filePath(fi);
            filePath.appendPath(QLatin1String("cdb.exe"));
            if (!cdbs.contains(filePath))
                cdbs.append(filePath);
        }
    }

    foreach (const FileName &cdb, cdbs) {
        QList<Abi> abis = Abi::abisOfBinary(cdb);
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

QVariant DebuggerProfileInformation::defaultValue(Profile *p) const
{
    ToolChain *tc = ToolChainProfileInformation::toolChain(p);
    Abi abi = Abi::hostAbi();
    if (tc)
        abi = tc->targetAbi();

    // CDB for windows:
    if (abi.os() == Abi::WindowsOS && abi.osFlavor() != Abi::WindowsMSysFlavor) {
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

    Environment env = Environment::systemEnvironment();
    return env.searchInPath(debugger);
}

QList<Task> DebuggerProfileInformation::validate(Profile *p) const
{
    const Core::Id id(Constants::TASK_CATEGORY_BUILDSYSTEM);
    QList<Task> result;
    FileName dbg = debuggerCommand(p);
    if (dbg.isEmpty()) {
        result << Task(Task::Warning, tr("No debugger set up."), FileName(), -1, id);
        return result;
    }

    QFileInfo fi = dbg.toFileInfo();
    if (!fi.exists() || fi.isDir())
        result << Task(Task::Error, tr("Debugger not found."), FileName(), -1, id);
    else if (!fi.isExecutable())
        result << Task(Task::Error, tr("Debugger not exectutable."), FileName(), -1, id);

    if (ToolChain *tc = ToolChainProfileInformation::toolChain(p)) {
        // We need an absolute path to be able to locate Python on Windows.
        const Abi abi = tc->targetAbi();
        if (abi.os() == Abi::WindowsOS && !fi.isAbsolute()) {
            result << Task(Task::Error, tr("The debugger location must be given as an "
                   "absolute path (%1).").arg(dbg.toString()), FileName(), -1, id);
        }
        // FIXME: Make sure debugger matches toolchain.
        // if (isCdb()) {
        //    if (abi.binaryFormat() != Abi::PEFormat || abi.os() != Abi::WindowsOS) {
        //        result << Task(Tas->errorDetails.push_back(CdbEngine::tr("The CDB debug engine does not support the %1 ABI.").
        //                                      arg(abi.toString()));
        //        return false;
        //    }
        // }
    }

    return result;
}

ProfileConfigWidget *DebuggerProfileInformation::createConfigWidget(Profile *p) const
{
    return new Internal::DebuggerProfileConfigWidget(p, this);
}

ProfileInformation::ItemList DebuggerProfileInformation::toUserOutput(Profile *p) const
{
    return ItemList() << qMakePair(tr("Debugger"), debuggerCommand(p).toUserOutput());
}

FileName DebuggerProfileInformation::debuggerCommand(const Profile *p)
{
    return FileName::fromString(p->value(Core::Id(DEBUGGER_INFORMATION)).toString());
}

void DebuggerProfileInformation::setDebuggerCommand(Profile *p, const FileName &command)
{
    p->setValue(Core::Id(DEBUGGER_INFORMATION), command.toString());
}

} // namespace Debugger
