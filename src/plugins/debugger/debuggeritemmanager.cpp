/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "debuggeritemmanager.h"
#include "debuggeritem.h"
#include "debuggerkitinformation.h"

#include <coreplugin/icore.h>

#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>
#include <utils/hostosinfo.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QProcess>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Debugger {

const char DEBUGGER_COUNT_KEY[] = "DebuggerItem.Count";
const char DEBUGGER_DATA_KEY[] = "DebuggerItem.";
const char DEBUGGER_LEGACY_FILENAME[] = "/qtcreator/profiles.xml";
const char DEBUGGER_FILE_VERSION_KEY[] = "Version";
const char DEBUGGER_FILENAME[] = "/qtcreator/debuggers.xml";

namespace {
QList<DebuggerItem> m_debuggers;
PersistentSettingsWriter *m_writer = 0;
}

// --------------------------------------------------------------------------
// DebuggerItemManager
// --------------------------------------------------------------------------

static void addDebugger(const DebuggerItem &item)
{
    QTC_ASSERT(item.id().isValid(), return);
    m_debuggers.append(item);
}

static FileName userSettingsFileName()
{
    QFileInfo settingsLocation(ICore::settings()->fileName());
    return FileName::fromString(settingsLocation.absolutePath() + QLatin1String(DEBUGGER_FILENAME));
}

static void readDebuggers(const FileName &fileName, bool isSystem)
{
    PersistentSettingsReader reader;
    if (!reader.load(fileName))
        return;
    QVariantMap data = reader.restoreValues();

    // Check version
    int version = data.value(QLatin1String(DEBUGGER_FILE_VERSION_KEY), 0).toInt();
    if (version < 1)
        return;

    int count = data.value(QLatin1String(DEBUGGER_COUNT_KEY), 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(DEBUGGER_DATA_KEY) + QString::number(i);
        if (!data.contains(key))
            continue;
        const QVariantMap dbMap = data.value(key).toMap();
        DebuggerItem item(dbMap);
        if (isSystem) {
            item.setAutoDetected(true);
            // SDK debuggers are always considered to be up-to-date, so no need to recheck them.
        } else {
            // User settings.
            if (item.isAutoDetected()) {
                if (!item.isValid() || item.engineType() == NoEngineType) {
                    qWarning() << QString::fromLatin1("DebuggerItem \"%1\" (%2) read from \"%3\" dropped since it is not valid.")
                                  .arg(item.command().toUserOutput(), item.id().toString(), fileName.toUserOutput());
                    continue;
                }
                if (!item.command().toFileInfo().isExecutable()) {
                    qWarning() << QString::fromLatin1("DebuggerItem \"%1\" (%2) read from \"%3\" dropped since the command is not executable.")
                                  .arg(item.command().toUserOutput(), item.id().toString(), fileName.toUserOutput());
                    continue;
                }
            }

        }
        DebuggerItemManager::registerDebugger(item);
    }
}

DebuggerItemManager::DebuggerItemManager()
{
    m_writer = new PersistentSettingsWriter(userSettingsFileName(), QLatin1String("QtCreatorDebuggers"));
    connect(ICore::instance(), &ICore::saveSettingsRequested,
            this, &DebuggerItemManager::saveDebuggers);
}

DebuggerItemManager::~DebuggerItemManager()
{
    delete m_writer;
}

QList<DebuggerItem> DebuggerItemManager::debuggers()
{
    return m_debuggers;
}

void DebuggerItemManager::autoDetectCdbDebuggers()
{
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
                        cdbs.append(FileName::fromString(cdb32.absoluteFilePath()));
                    const QFileInfo cdb64(path + QLatin1String("/Debuggers/x64/cdb.exe"));
                    if (cdb64.isExecutable())
                        cdbs.append(FileName::fromString(cdb64.absoluteFilePath()));
                }
            }
        }

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
        if (findByCommand(cdb))
            continue;
        DebuggerItem item;
        item.createId();
        item.setAutoDetected(true);
        item.setAbis(Abi::abisOfBinary(cdb));
        item.setCommand(cdb);
        item.setEngineType(CdbEngineType);
        item.setUnexpandedDisplayName(uniqueDisplayName(tr("Auto-detected CDB at %1").arg(cdb.toUserOutput())));
        addDebugger(item);
    }
}

void DebuggerItemManager::autoDetectGdbOrLldbDebuggers()
{
    QStringList filters;
    filters.append(QLatin1String("gdb-i686-pc-mingw32"));
    filters.append(QLatin1String("gdb-i686-pc-mingw32.exe"));
    filters.append(QLatin1String("gdb"));
    filters.append(QLatin1String("gdb.exe"));
    filters.append(QLatin1String("lldb"));
    filters.append(QLatin1String("lldb.exe"));
    filters.append(QLatin1String("lldb-*"));

//    DebuggerItem result;
//    result.setAutoDetected(true);
//    result.setDisplayName(tr("Auto-detected for Tool Chain %1").arg(tc->displayName()));
    /*
    // Check suggestions from the SDK.
    Environment env = Environment::systemEnvironment();
    if (tc) {
        tc->addToEnvironment(env); // Find MinGW gdb in toolchain environment.
        QString path = tc->suggestedDebugger().toString();
        if (!path.isEmpty()) {
            const QFileInfo fi(path);
            if (!fi.isAbsolute())
                path = env.searchInPath(path);
            result.command = FileName::fromString(path);
            result.engineType = engineTypeFromBinary(path);
            return maybeAddDebugger(result, false);
        }
    }
    */

    QList<FileName> suspects;

    if (HostOsInfo::isMacHost()) {
        QProcess lldbInfo;
        lldbInfo.start(QLatin1String("xcrun"), QStringList() << QLatin1String("--find")
                       << QLatin1String("lldb"));
        if (!lldbInfo.waitForFinished(2000)) {
            lldbInfo.kill();
            lldbInfo.waitForFinished();
        } else {
            QByteArray lPath = lldbInfo.readAll();
            const QFileInfo fi(QString::fromLocal8Bit(lPath.data(), lPath.size() -1));
            if (fi.exists() && fi.isExecutable() && !fi.isDir())
                suspects.append(FileName::fromString(fi.absoluteFilePath()));
        }
    }

    QStringList path = Environment::systemEnvironment().path();
    path.removeDuplicates();
    QDir dir;
    dir.setNameFilters(filters);
    dir.setFilter(QDir::Files | QDir::Executable);
    foreach (const QString &base, path) {
        dir.setPath(base);
        foreach (const QString &entry, dir.entryList())
            suspects.append(FileName::fromString(dir.absoluteFilePath(entry)));
    }

    foreach (const FileName &command, suspects) {
        if (findByCommand(command))
            continue;
        DebuggerItem item;
        item.createId();
        item.setCommand(command);
        item.reinitializeFromFile();
        //: %1: Debugger engine type (GDB, LLDB, CDB...), %2: Path
        item.setUnexpandedDisplayName(tr("System %1 at %2")
            .arg(item.engineTypeName()).arg(command.toUserOutput()));
        item.setAutoDetected(true);
        addDebugger(item);
    }
}

void DebuggerItemManager::readLegacyDebuggers(const FileName &file)
{
    PersistentSettingsReader reader;
    if (!reader.load(file))
        return;

    foreach (const QVariant &v, reader.restoreValues()) {
        QVariantMap data1 = v.toMap();
        QString kitName = data1.value(QLatin1String("PE.Profile.Name")).toString();
        QVariantMap data2 = data1.value(QLatin1String("PE.Profile.Data")).toMap();
        QVariant v3 = data2.value(DebuggerKitInformation::id().toString());
        QString fn;
        if (v3.type() == QVariant::String)
            fn = v3.toString();
        else
            fn = v3.toMap().value(QLatin1String("Binary")).toString();
        if (fn.isEmpty())
            continue;
        if (fn.startsWith(QLatin1Char('{')))
            continue;
        if (fn == QLatin1String("auto"))
            continue;
        FileName command = FileName::fromUserInput(fn);
        if (!command.exists())
            continue;
        if (findByCommand(command))
            continue;
        DebuggerItem item;
        item.createId();
        item.setCommand(command);
        item.setAutoDetected(true);
        item.reinitializeFromFile();
        item.setUnexpandedDisplayName(tr("Extracted from Kit %1").arg(kitName));
        addDebugger(item);
    }
}

const DebuggerItem *DebuggerItemManager::findByCommand(const FileName &command)
{
    foreach (const DebuggerItem &item, m_debuggers)
        if (item.command() == command)
            return &item;

    return 0;
}

const DebuggerItem *DebuggerItemManager::findById(const QVariant &id)
{
    foreach (const DebuggerItem &item, m_debuggers)
        if (item.id() == id)
            return &item;

    return 0;
}

const DebuggerItem *DebuggerItemManager::findByEngineType(DebuggerEngineType engineType)
{
    foreach (const DebuggerItem &item, m_debuggers)
        if (item.engineType() == engineType && item.isValid())
            return &item;
    return 0;
}

void DebuggerItemManager::restoreDebuggers()
{
    // Read debuggers from SDK
    QFileInfo systemSettingsFile(ICore::settings(QSettings::SystemScope)->fileName());
    readDebuggers(FileName::fromString(systemSettingsFile.absolutePath() + QLatin1String(DEBUGGER_FILENAME)), true);

    // Read all debuggers from user file.
    readDebuggers(userSettingsFileName(), false);

    // Auto detect current.
    autoDetectCdbDebuggers();
    autoDetectGdbOrLldbDebuggers();

    // Add debuggers from pre-3.x profiles.xml
    QFileInfo systemLocation(ICore::settings(QSettings::SystemScope)->fileName());
    readLegacyDebuggers(FileName::fromString(systemLocation.absolutePath() + QLatin1String(DEBUGGER_LEGACY_FILENAME)));
    QFileInfo userLocation(ICore::settings()->fileName());
    readLegacyDebuggers(FileName::fromString(userLocation.absolutePath() + QLatin1String(DEBUGGER_LEGACY_FILENAME)));
}

void DebuggerItemManager::saveDebuggers()
{
    QTC_ASSERT(m_writer, return);
    QVariantMap data;
    data.insert(QLatin1String(DEBUGGER_FILE_VERSION_KEY), 1);

    int count = 0;
    foreach (const DebuggerItem &item, m_debuggers) {
        if (item.isValid() && item.engineType() != NoEngineType) {
            QVariantMap tmp = item.toMap();
            if (tmp.isEmpty())
                continue;
            data.insert(QString::fromLatin1(DEBUGGER_DATA_KEY) + QString::number(count), tmp);
            ++count;
        }
    }
    data.insert(QLatin1String(DEBUGGER_COUNT_KEY), count);
    m_writer->save(data, ICore::mainWindow());

    // Do not save default debuggers as they are set by the SDK.
}

QVariant DebuggerItemManager::registerDebugger(const DebuggerItem &item)
{
    // Try re-using existing item first.
    foreach (const DebuggerItem &d, m_debuggers) {
        if (d.command() == item.command()
                && d.isAutoDetected() == item.isAutoDetected()
                && d.engineType() == item.engineType()
                && d.unexpandedDisplayName() == item.unexpandedDisplayName()
                && d.abis() == item.abis()) {
            return d.id();
        }
    }

    // If item already has an id, use it. Otherwise, create a new id.
    DebuggerItem di = item;
    if (!di.id().isValid())
        di.createId();

    addDebugger(di);
    return di.id();
}

void DebuggerItemManager::deregisterDebugger(const QVariant &id)
{
    for (int i = 0, n = m_debuggers.size(); i != n; ++i) {
        if (m_debuggers.at(i).id() == id) {
            m_debuggers.removeAt(i);
            break;
        }
    }
}

QString DebuggerItemManager::uniqueDisplayName(const QString &base)
{
    foreach (const DebuggerItem &item, m_debuggers)
        if (item.unexpandedDisplayName() == base)
            return uniqueDisplayName(base + QLatin1String(" (1)"));

    return base;
}

void DebuggerItemManager::updateOrAddDebugger(const DebuggerItem &treeItem)
{
    for (int i = 0, n = m_debuggers.size(); i != n; ++i) {
        DebuggerItem &item = m_debuggers[i];
        if (item.id() == treeItem.id()) {
            item = treeItem;
            return;
        }
    }

    // This is a new item.
    addDebugger(treeItem);
}

} // namespace Debugger;
