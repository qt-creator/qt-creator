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

#include "debuggeritem.h"
#include "debuggeritemmanager.h"
#include "debuggerkitinformation.h"
#include "debuggerkitconfigwidget.h"
#include "debuggerprotocol.h"

#include <projectexplorer/abi.h>

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>
#include <utils/utilsicons.h>

#include <QFileInfo>
#include <QProcess>
#include <QUuid>

#ifdef WITH_TESTS
#    include <QTest>
#    include "debuggerplugin.h"
#endif

using namespace Debugger::Internal;
using namespace ProjectExplorer;
using namespace Utils;

const char DEBUGGER_INFORMATION_COMMAND[] = "Binary";
const char DEBUGGER_INFORMATION_DISPLAYNAME[] = "DisplayName";
const char DEBUGGER_INFORMATION_ID[] = "Id";
const char DEBUGGER_INFORMATION_ENGINETYPE[] = "EngineType";
const char DEBUGGER_INFORMATION_AUTODETECTED[] = "AutoDetected";
const char DEBUGGER_INFORMATION_AUTODETECTION_SOURCE[] = "AutoDetectionSource";
const char DEBUGGER_INFORMATION_VERSION[] = "Version";
const char DEBUGGER_INFORMATION_ABIS[] = "Abis";
const char DEBUGGER_INFORMATION_LASTMODIFIED[] = "LastModified";
const char DEBUGGER_INFORMATION_WORKINGDIRECTORY[] = "WorkingDirectory";

namespace Debugger {

// --------------------------------------------------------------------------
// DebuggerItem
// --------------------------------------------------------------------------

DebuggerItem::DebuggerItem()
{
    m_engineType = NoEngineType;
    m_isAutoDetected = false;
}

DebuggerItem::DebuggerItem(const QVariant &id)
{
    m_id = id;
    m_engineType = NoEngineType;
    m_isAutoDetected = false;
}

DebuggerItem::DebuggerItem(const QVariantMap &data)
{
    m_id = data.value(QLatin1String(DEBUGGER_INFORMATION_ID)).toString();
    m_command = FileName::fromUserInput(data.value(QLatin1String(DEBUGGER_INFORMATION_COMMAND)).toString());
    m_workingDirectory = FileName::fromUserInput(data.value(DEBUGGER_INFORMATION_WORKINGDIRECTORY).toString());
    m_unexpandedDisplayName = data.value(QLatin1String(DEBUGGER_INFORMATION_DISPLAYNAME)).toString();
    m_isAutoDetected = data.value(QLatin1String(DEBUGGER_INFORMATION_AUTODETECTED), false).toBool();
    m_autoDetectionSource = data.value(QLatin1String(DEBUGGER_INFORMATION_AUTODETECTION_SOURCE)).toString();
    m_version = data.value(QLatin1String(DEBUGGER_INFORMATION_VERSION)).toString();
    m_engineType = DebuggerEngineType(data.value(QLatin1String(DEBUGGER_INFORMATION_ENGINETYPE),
                                                 static_cast<int>(NoEngineType)).toInt());
    m_lastModified = data.value(QLatin1String(DEBUGGER_INFORMATION_LASTMODIFIED)).toDateTime();

    foreach (const QString &a, data.value(QLatin1String(DEBUGGER_INFORMATION_ABIS)).toStringList()) {
        Abi abi(a);
        if (!abi.isNull())
            m_abis.append(abi);
    }

    if (m_version.isEmpty())
        reinitializeFromFile();
}

void DebuggerItem::createId()
{
    QTC_ASSERT(!m_id.isValid(), return);
    m_id = QUuid::createUuid().toString();
}

void DebuggerItem::reinitializeFromFile()
{
    // CDB only understands the single-dash -version, whereas GDB and LLDB are
    // happy with both -version and --version. So use the "working" -version
    // except for the experimental LLDB-MI which insists on --version.
    const char *version = "-version";
    const QFileInfo fileInfo = m_command.toFileInfo();
    m_lastModified = fileInfo.lastModified();
    if (fileInfo.baseName().toLower().contains(QLatin1String("lldb-mi")))
        version = "--version";

    SynchronousProcess proc;
    SynchronousProcessResponse response
            = proc.runBlocking(m_command.toString(), QStringList({ QLatin1String(version) }));
    if (response.result != SynchronousProcessResponse::Finished) {
        m_engineType = NoEngineType;
        return;
    }
    m_abis.clear();
    const QString output = response.allOutput();
    if (output.contains("gdb")) {
        m_engineType = GdbEngineType;
        const char needle[] = "This GDB was configured as \"";
        // E.g.  "--host=i686-pc-linux-gnu --target=arm-unknown-nto-qnx6.5.0".
        // or "i686-linux-gnu"
        int pos1 = output.indexOf(needle);
        if (pos1 != -1) {
            pos1 += int(strlen(needle));
            int pos2 = output.indexOf('"', pos1 + 1);
            QString target = output.mid(pos1, pos2 - pos1);
            int pos3 = target.indexOf("--target=");
            if (pos3 >= 0)
                target = target.mid(pos3 + 9);
            m_abis.append(Abi::abiFromTargetTriplet(target));
        } else {
            // Fallback.
            m_abis = Abi::abisOfBinary(m_command); // FIXME: Wrong.
        }

        // Version
        bool isMacGdb, isQnxGdb;
        int version = 0, buildVersion = 0;
        Debugger::Internal::extractGdbVersion(output,
            &version, &buildVersion, &isMacGdb, &isQnxGdb);
        if (version)
            m_version = QString::fromLatin1("%1.%2.%3")
                .arg(version / 10000).arg((version / 100) % 100).arg(version % 100);
        return;
    }
    if (output.startsWith("lldb") || output.startsWith("LLDB")) {
        m_engineType = LldbEngineType;
        m_abis = Abi::abisOfBinary(m_command);

        // Version
        if (output.startsWith(("lldb version "))) { // Linux typically.
            int pos1 = int(strlen("lldb version "));
            int pos2 = output.indexOf(' ', pos1);
            m_version = output.mid(pos1, pos2 - pos1);
        } else if (output.startsWith("lldb-") || output.startsWith("LLDB-")) { // Mac typically.
            m_version = output.mid(5);
        }
        return;
    }
    if (output.startsWith("cdb")) {
        // "cdb version 6.2.9200.16384"
        m_engineType = CdbEngineType;
        m_abis = Abi::abisOfBinary(m_command);
        m_version = output.section(QLatin1Char(' '), 2);
        return;
    }
    if (output.startsWith("Python")) {
        m_engineType = PdbEngineType;
        return;
    }
    m_engineType = NoEngineType;
}

QString DebuggerItem::engineTypeName() const
{
    switch (m_engineType) {
    case NoEngineType:
        return DebuggerItemManager::tr("Not recognized");
    case GdbEngineType:
        return QLatin1String("GDB");
    case CdbEngineType:
        return QLatin1String("CDB");
    case LldbEngineType:
        return QLatin1String("LLDB");
    default:
        return QString();
    }
}

QStringList DebuggerItem::abiNames() const
{
    QStringList list;
    foreach (const Abi &abi, m_abis)
        list.append(abi.toString());
    return list;
}

QDateTime DebuggerItem::lastModified() const
{
    return m_lastModified;
}

QIcon DebuggerItem::decoration() const
{
    if (m_engineType == NoEngineType)
        return Utils::Icons::CRITICAL.icon();
    if (!m_command.toFileInfo().isExecutable())
        return Utils::Icons::WARNING.icon();
    if (!m_workingDirectory.isEmpty() && !m_workingDirectory.toFileInfo().isDir())
        return Utils::Icons::WARNING.icon();
    return QIcon();
}

QString DebuggerItem::validityMessage() const
{
    if (m_engineType == NoEngineType)
        return DebuggerItemManager::tr("Could not determine debugger type");
    return QString();
}

bool DebuggerItem::operator==(const DebuggerItem &other) const
{
    return m_id == other.m_id
            && m_unexpandedDisplayName == other.m_unexpandedDisplayName
            && m_isAutoDetected == other.m_isAutoDetected
            && m_command == other.m_command
            && m_workingDirectory == other.m_workingDirectory;
}

QVariantMap DebuggerItem::toMap() const
{
    QVariantMap data;
    data.insert(QLatin1String(DEBUGGER_INFORMATION_DISPLAYNAME), m_unexpandedDisplayName);
    data.insert(QLatin1String(DEBUGGER_INFORMATION_ID), m_id);
    data.insert(QLatin1String(DEBUGGER_INFORMATION_COMMAND), m_command.toString());
    data.insert(QLatin1String(DEBUGGER_INFORMATION_WORKINGDIRECTORY), m_workingDirectory.toString());
    data.insert(QLatin1String(DEBUGGER_INFORMATION_ENGINETYPE), int(m_engineType));
    data.insert(QLatin1String(DEBUGGER_INFORMATION_AUTODETECTED), m_isAutoDetected);
    data.insert(QLatin1String(DEBUGGER_INFORMATION_AUTODETECTION_SOURCE), m_autoDetectionSource);
    data.insert(QLatin1String(DEBUGGER_INFORMATION_VERSION), m_version);
    data.insert(QLatin1String(DEBUGGER_INFORMATION_ABIS), abiNames());
    data.insert(QLatin1String(DEBUGGER_INFORMATION_LASTMODIFIED), m_lastModified);
    return data;
}

QString DebuggerItem::displayName() const
{
    if (!m_unexpandedDisplayName.contains(QLatin1Char('%')))
        return m_unexpandedDisplayName;

    MacroExpander expander;
    expander.registerVariable("Debugger:Type", DebuggerKitInformation::tr("Type of Debugger Backend"),
        [this] { return engineTypeName(); });
    expander.registerVariable("Debugger:Version", DebuggerKitInformation::tr("Debugger"),
        [this] { return !m_version.isEmpty() ? m_version :
                                               DebuggerKitInformation::tr("Unknown debugger version"); });
    expander.registerVariable("Debugger:Abi", DebuggerKitInformation::tr("Debugger"),
        [this] { return !m_abis.isEmpty() ? abiNames().join(QLatin1Char(' ')) :
                                            DebuggerKitInformation::tr("Unknown debugger ABI"); });
    return expander.expand(m_unexpandedDisplayName);
}

void DebuggerItem::setUnexpandedDisplayName(const QString &displayName)
{
    m_unexpandedDisplayName = displayName;
}

void DebuggerItem::setEngineType(const DebuggerEngineType &engineType)
{
    m_engineType = engineType;
}

void DebuggerItem::setCommand(const FileName &command)
{
    m_command = command;
}

void DebuggerItem::setAutoDetected(bool isAutoDetected)
{
    m_isAutoDetected = isAutoDetected;
}

QString DebuggerItem::version() const
{
    return m_version;
}

void DebuggerItem::setVersion(const QString &version)
{
    m_version = version;
}

void DebuggerItem::setAutoDetectionSource(const QString &autoDetectionSource)
{
    m_autoDetectionSource = autoDetectionSource;
}

void DebuggerItem::setAbis(const QList<Abi> &abis)
{
    m_abis = abis;
}

void DebuggerItem::setAbi(const Abi &abi)
{
    m_abis.clear();
    m_abis.append(abi);
}

static DebuggerItem::MatchLevel matchSingle(const Abi &debuggerAbi, const Abi &targetAbi, DebuggerEngineType engineType)
{
    if (debuggerAbi.architecture() != Abi::UnknownArchitecture
            && debuggerAbi.architecture() != targetAbi.architecture())
        return DebuggerItem::DoesNotMatch;

    if (debuggerAbi.os() != Abi::UnknownOS
            && debuggerAbi.os() != targetAbi.os())
        return DebuggerItem::DoesNotMatch;

    if (debuggerAbi.binaryFormat() != Abi::UnknownFormat
            && debuggerAbi.binaryFormat() != targetAbi.binaryFormat())
        return DebuggerItem::DoesNotMatch;

    if (debuggerAbi.os() == Abi::WindowsOS) {
        if (debuggerAbi.osFlavor() == Abi::WindowsMSysFlavor && targetAbi.osFlavor() != Abi::WindowsMSysFlavor)
            return DebuggerItem::DoesNotMatch;
        if (debuggerAbi.osFlavor() != Abi::WindowsMSysFlavor && targetAbi.osFlavor() == Abi::WindowsMSysFlavor)
            return DebuggerItem::DoesNotMatch;
    }

    if (debuggerAbi.wordWidth() == 64 && targetAbi.wordWidth() == 32)
        return DebuggerItem::MatchesSomewhat;
    if (debuggerAbi.wordWidth() != 0 && debuggerAbi.wordWidth() != targetAbi.wordWidth())
        return DebuggerItem::DoesNotMatch;

    // We have at least 'Matches well' now. Mark the combinations we really like.
    if (HostOsInfo::isWindowsHost() && engineType == GdbEngineType && targetAbi.osFlavor() == Abi::WindowsMSysFlavor)
        return DebuggerItem::MatchesPerfectly;
    if (HostOsInfo::isLinuxHost() && engineType == GdbEngineType && targetAbi.os() == Abi::LinuxOS)
        return DebuggerItem::MatchesPerfectly;
    if (HostOsInfo::isMacHost() && engineType == LldbEngineType && targetAbi.os() == Abi::DarwinOS)
        return DebuggerItem::MatchesPerfectly;

    return DebuggerItem::MatchesWell;
}

DebuggerItem::MatchLevel DebuggerItem::matchTarget(const Abi &targetAbi) const
{
    MatchLevel bestMatch = DoesNotMatch;
    foreach (const Abi &debuggerAbi, m_abis) {
        MatchLevel currentMatch = matchSingle(debuggerAbi, targetAbi, m_engineType);
        if (currentMatch > bestMatch)
            bestMatch = currentMatch;
    }
    return bestMatch;
}

bool DebuggerItem::isValid() const
{
    return !m_id.isNull();
}

} // namespace Debugger;
