/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "debuggerkitinformation.h"

#include "debuggerkitconfigwidget.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>

#include <utils/environment.h>
#include <utils/qtcassert.h>

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

static DebuggerEngineType engineTypeFromBinary(const QString &binary)
{
    if (binary.contains(QLatin1String("cdb"), Qt::CaseInsensitive))
        return CdbEngineType;
    if (binary.contains(QLatin1String("lldb"), Qt::CaseInsensitive))
        return LldbEngineType;
    return GdbEngineType;
}

// --------------------------------------------------------------------------
// DebuggerKitInformation:
// --------------------------------------------------------------------------

static const char DEBUGGER_INFORMATION[] = "Debugger.Information";

DebuggerKitInformation::DebuggerItem::DebuggerItem()
    : engineType(NoEngineType)
{
}

DebuggerKitInformation::DebuggerItem::DebuggerItem(DebuggerEngineType et, const Utils::FileName &fn)
    : engineType(et)
    , binary(fn)
{
}

DebuggerKitInformation::DebuggerKitInformation()
{
    setObjectName(QLatin1String("DebuggerKitInformation"));
}

Core::Id DebuggerKitInformation::dataId() const
{
    static Core::Id id = Core::Id(DEBUGGER_INFORMATION);
    return id;
}

unsigned int DebuggerKitInformation::priority() const
{
    return 28000;
}

DebuggerKitInformation::DebuggerItem DebuggerKitInformation::autoDetectItem(const Kit *k)
{
    DebuggerItem result;
    const ToolChain *tc = ToolChainKitInformation::toolChain(k);
    Abi abi = Abi::hostAbi();
    if (tc)
        abi = tc->targetAbi();

    // CDB for windows:
    if (abi.os() == Abi::WindowsOS && abi.osFlavor() != Abi::WindowsMSysFlavor) {
        QPair<QString, QString> cdbs = autoDetectCdbDebugger();
        result.binary = Utils::FileName::fromString(abi.wordWidth() == 32 ? cdbs.first : cdbs.second);
        result.engineType = CdbEngineType;
        return result;
    }

    // Check suggestions from the SDK.
    Environment env = Environment::systemEnvironment();
    if (tc) {
        tc->addToEnvironment(env); // Find MinGW gdb in toolchain environment.
        QString path = tc->suggestedDebugger().toString();
        if (!path.isEmpty()) {
            const QFileInfo fi(path);
            if (!fi.isAbsolute())
                path = env.searchInPath(path);
            result.binary = Utils::FileName::fromString(path);
            result.engineType = engineTypeFromBinary(path);
            return result;
        }
    }

    // Default to GDB, system GDB
    result.engineType = GdbEngineType;
    QString gdb;
    const QString systemGdb = QLatin1String("gdb");
    // MinGW: Search for the python-enabled gdb first.
    if (abi.os() == Abi::WindowsOS && abi.osFlavor() == Abi::WindowsMSysFlavor)
        gdb = env.searchInPath(QLatin1String("gdb-i686-pc-mingw32"));
    if (gdb.isEmpty())
        gdb = env.searchInPath(systemGdb);
    result.binary = Utils::FileName::fromString(env.searchInPath(gdb.isEmpty() ? systemGdb : gdb));
    return result;
}

void DebuggerKitInformation::setup(Kit *k)
{
    setDebuggerItem(k, autoDetectItem(k));
}

// Check the configuration errors and return a flag mask. Provide a quick check and
// a verbose one with a list of errors.

enum DebuggerConfigurationErrors {
    NoDebugger = 0x1,
    DebuggerNotFound = 0x2,
    DebuggerNotExecutable = 0x4,
    DebuggerNeedsAbsolutePath = 0x8
};

static unsigned debuggerConfigurationErrors(const ProjectExplorer::Kit *k)
{
    unsigned result = 0;
    const DebuggerKitInformation::DebuggerItem item = DebuggerKitInformation::debuggerItem(k);
    if (item.engineType == NoEngineType || item.binary.isEmpty())
        return NoDebugger;

    const QFileInfo fi = item.binary.toFileInfo();
    if (!fi.exists() || fi.isDir())
        result |= DebuggerNotFound;
    else if (!fi.isExecutable())
        result |= DebuggerNotExecutable;

    if (!fi.exists() || fi.isDir())
        // We need an absolute path to be able to locate Python on Windows.
        if (item.engineType == GdbEngineType)
            if (const ToolChain *tc = ToolChainKitInformation::toolChain(k))
                if (tc->targetAbi().os() == Abi::WindowsOS && !fi.isAbsolute())
                    result |= DebuggerNeedsAbsolutePath;
    return result;
}

bool DebuggerKitInformation::isValidDebugger(const ProjectExplorer::Kit *k)
{
    return debuggerConfigurationErrors(k) == 0;
}

QList<ProjectExplorer::Task> DebuggerKitInformation::validateDebugger(const ProjectExplorer::Kit *k)
{
    const unsigned errors = debuggerConfigurationErrors(k);
    const Core::Id id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);
    QList<Task> result;
    if (errors & NoDebugger)
        result << Task(Task::Warning, tr("No debugger set up."), FileName(), -1, id);

    if (errors & DebuggerNotFound) {
        const QString path = DebuggerKitInformation::debuggerCommand(k).toUserOutput();
        result << Task(Task::Error, tr("Debugger '%1' not found.").arg(path),
                       FileName(), -1, id);
    }
    if (errors & DebuggerNotExecutable) {
        const QString path = DebuggerKitInformation::debuggerCommand(k).toUserOutput();
        result << Task(Task::Error, tr("Debugger '%1' not executable.").arg(path), FileName(), -1, id);
    }
    if (errors & DebuggerNeedsAbsolutePath) {
        const QString path = DebuggerKitInformation::debuggerCommand(k).toUserOutput();
        const QString message =
                tr("The debugger location must be given as an "
                   "absolute path (%1).").arg(path);
        result << Task(Task::Error, message, FileName(), -1, id);
    }
    return result;
}

KitConfigWidget *DebuggerKitInformation::createConfigWidget(Kit *k) const
{
    return new Internal::DebuggerKitConfigWidget(k);
}

QString DebuggerKitInformation::userOutput(const DebuggerItem &item)
{
    const QString binary = item.binary.toUserOutput();
    const QString name = debuggerEngineName(item.engineType);
    return binary.isEmpty() ? tr("%1 <None>").arg(name) : tr("%1 using \"%2\"").arg(name, binary);
}

KitInformation::ItemList DebuggerKitInformation::toUserOutput(Kit *k) const
{
    return ItemList() << qMakePair(tr("Debugger"), DebuggerKitInformation::userOutput(DebuggerKitInformation::debuggerItem(k)));
}

static const char engineTypeKeyC[] = "EngineType";
static const char binaryKeyC[] = "Binary";

DebuggerKitInformation::DebuggerItem DebuggerKitInformation::variantToItem(const QVariant &v)
{
    DebuggerItem result;
    if (v.type() == QVariant::String) { // Convert legacy config items, remove later.
        const QString binary = v.toString();
        result.binary = Utils::FileName::fromString(binary);
        result.engineType = engineTypeFromBinary(binary);
        return result;
    }
    QTC_ASSERT(v.type() == QVariant::Map, return result);
    const QVariantMap vmap = v.toMap();
    result.engineType = static_cast<DebuggerEngineType>(vmap.value(QLatin1String(engineTypeKeyC)).toInt());
    QString binary = vmap.value(QLatin1String(binaryKeyC)).toString();
    // Check for special 'auto' entry for binary written by the sdktool during
    // installation. Try to autodetect.
    if (binary == QLatin1String("auto")) {
        binary.clear();
        switch (result.engineType) {
        case Debugger::GdbEngineType: // Auto-detect system gdb on Unix
            if (Abi::hostAbi().os() != Abi::WindowsOS)
                binary = Environment::systemEnvironment().searchInPath(QLatin1String("gdb"));
            break;
        case Debugger::CdbEngineType: { // Auto-detect system CDB on Windows.
             const QPair<QString, QString> cdbs = autoDetectCdbDebugger();
             binary = cdbs.second.isEmpty() ? cdbs.first : cdbs.second;
        }
            break;
        default:
            break;
        }
    }
    result.binary = Utils::FileName::fromString(binary);
    return result;
}

QVariant DebuggerKitInformation::itemToVariant(const DebuggerItem &i)
{
    QVariantMap vmap;
    vmap.insert(QLatin1String(binaryKeyC), QVariant(i.binary.toUserOutput()));
    vmap.insert(QLatin1String(engineTypeKeyC), QVariant(int(i.engineType)));
    return QVariant(vmap);
}

DebuggerKitInformation::DebuggerItem DebuggerKitInformation::debuggerItem(const ProjectExplorer::Kit *k)
{
    return k ?
           DebuggerKitInformation::variantToItem(k->value(Core::Id(DEBUGGER_INFORMATION))) :
           DebuggerItem();
}

void DebuggerKitInformation::setDebuggerItem(ProjectExplorer::Kit *k, const DebuggerItem &item)
{
    QTC_ASSERT(k, return);
    k->setValue(Core::Id(DEBUGGER_INFORMATION), itemToVariant(item));
}

void DebuggerKitInformation::setDebuggerCommand(ProjectExplorer::Kit *k, const FileName &command)
{
    setDebuggerItem(k, DebuggerItem(engineType(k), command));
}

void DebuggerKitInformation::setEngineType(ProjectExplorer::Kit *k, DebuggerEngineType type)
{
    setDebuggerItem(k, DebuggerItem(type, debuggerCommand(k)));
}

QString DebuggerKitInformation::debuggerEngineName(DebuggerEngineType t)
{
    switch (t) {
    case Debugger::GdbEngineType:
        return tr("GDB Engine");
    case Debugger::CdbEngineType:
        return tr("CDB Engine");
    case Debugger::LldbEngineType:
        return tr("LLDB Engine");
    default:
        break;
    }
    return QString();
}

} // namespace Debugger
