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
#include "debuggerprotocol.h"

#include <projectexplorer/abi.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>
#include <utils/winutils.h>

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

namespace Debugger {

const char DEBUGGER_INFORMATION_COMMAND[] = "Binary";
const char DEBUGGER_INFORMATION_DISPLAYNAME[] = "DisplayName";
const char DEBUGGER_INFORMATION_ID[] = "Id";
const char DEBUGGER_INFORMATION_ENGINETYPE[] = "EngineType";
const char DEBUGGER_INFORMATION_AUTODETECTED[] = "AutoDetected"; // FIXME: Merge into DetectionSource
const char DEBUGGER_INFORMATION_DETECTION_SOURCE[] = "DetectionSource";
const char DEBUGGER_INFORMATION_VERSION[] = "Version";
const char DEBUGGER_INFORMATION_ABIS[] = "Abis";
const char DEBUGGER_INFORMATION_LASTMODIFIED[] = "LastModified";
const char DEBUGGER_INFORMATION_WORKINGDIRECTORY[] = "WorkingDirectory";


//! Return the configuration of gdb as a list of --key=value
//! \note That the list will also contain some output not in this format.
static QString getGdbConfiguration(const FilePath &command, const Environment &sysEnv)
{
    // run gdb with the --configuration opion
    QtcProcess proc;
    proc.setEnvironment(sysEnv);
    proc.setCommand({command, {"--configuration"}});
    proc.runBlocking();
    return proc.allOutput();
}

//! Extract the target ABI identifier from GDB output
//! \return QString() (aka Null) if unable to find something
static QString extractGdbTargetAbiStringFromGdbOutput(const QString &gdbOutput)
{
    const auto outputLines = gdbOutput.split('\n');
    const auto whitespaceSeparatedTokens = outputLines.join(' ').split(' ', Qt::SkipEmptyParts);

    const QString targetKey{"--target="};
    const QString targetValue = Utils::findOrDefault(whitespaceSeparatedTokens,
                                                [&targetKey](const QString &token) { return token.startsWith(targetKey); });
    if (!targetValue.isEmpty())
        return targetValue.mid(targetKey.size());

    return {};
}


// --------------------------------------------------------------------------
// DebuggerItem
// --------------------------------------------------------------------------

DebuggerItem::DebuggerItem() = default;

DebuggerItem::DebuggerItem(const QVariant &id)
{
    m_id = id;
}

DebuggerItem::DebuggerItem(const QVariantMap &data)
{
    m_id = data.value(DEBUGGER_INFORMATION_ID).toString();
    m_command = FilePath::fromVariant(data.value(DEBUGGER_INFORMATION_COMMAND));
    m_workingDirectory = FilePath::fromVariant(data.value(DEBUGGER_INFORMATION_WORKINGDIRECTORY));
    m_unexpandedDisplayName = data.value(DEBUGGER_INFORMATION_DISPLAYNAME).toString();
    m_isAutoDetected = data.value(DEBUGGER_INFORMATION_AUTODETECTED, false).toBool();
    m_detectionSource = data.value(DEBUGGER_INFORMATION_DETECTION_SOURCE).toString();
    m_version = data.value(DEBUGGER_INFORMATION_VERSION).toString();
    m_engineType = DebuggerEngineType(data.value(DEBUGGER_INFORMATION_ENGINETYPE,
                                                 static_cast<int>(NoEngineType)).toInt());
    m_lastModified = data.value(DEBUGGER_INFORMATION_LASTMODIFIED).toDateTime();

    const QStringList abis = data.value(DEBUGGER_INFORMATION_ABIS).toStringList();
    for (const QString &a : abis) {
        Abi abi = Abi::fromString(a);
        if (!abi.isNull())
            m_abis.append(abi);
    }

    bool mightBeAPreQnxSeparateOSQnxDebugger = m_command.fileName().startsWith("nto")
            && m_abis.count() == 1
            && m_abis[0].os() == Abi::UnknownOS
            && m_abis[0].osFlavor() == Abi::UnknownFlavor
            && m_abis[0].binaryFormat() == Abi::UnknownFormat;

    if (m_version.isEmpty() || mightBeAPreQnxSeparateOSQnxDebugger)
        reinitializeFromFile();
}

void DebuggerItem::createId()
{
    QTC_ASSERT(!m_id.isValid(), return);
    m_id = QUuid::createUuid().toString();
}

void DebuggerItem::reinitializeFromFile(const Environment &sysEnv, QString *error)
{
    // CDB only understands the single-dash -version, whereas GDB and LLDB are
    // happy with both -version and --version. So use the "working" -version
    // except for the experimental LLDB-MI which insists on --version.
    QString version = "-version";
    m_lastModified = m_command.lastModified();
    if (m_command.baseName().toLower().contains("lldb-mi"))
        version = "--version";

    // We don't need to start the uVision executable to
    // determine its version.
    if (HostOsInfo::isWindowsHost() && m_command.baseName() == "UV4") {
        QString errorMessage;
        m_version = winGetDLLVersion(WinDLLFileVersion,
                                     m_command.absoluteFilePath().path(),
                                     &errorMessage);
        m_engineType = UvscEngineType;
        m_abis.clear();
        return;
    }

    // Prevent calling lldb on Windows because the lldb from the llvm package is linked against
    // python but does not contain a python dll.
    if (HostOsInfo::isWindowsHost() && m_command.fileName().startsWith("lldb")) {
        QString errorMessage;
        m_version = winGetDLLVersion(WinDLLFileVersion,
                                     m_command.absoluteFilePath().path(),
                                     &errorMessage);
        m_engineType = LldbEngineType;
        m_abis = Abi::abisOfBinary(m_command);
        return;
    }

    QtcProcess proc;
    proc.setEnvironment(sysEnv);
    proc.setCommand({m_command, {version}});
    proc.runBlocking();
    const QString output = proc.allOutput().trimmed();
    if (proc.result() != QtcProcess::FinishedWithSuccess) {
        if (error)
            *error = output;
        m_engineType = NoEngineType;
        return;
    }
    m_abis.clear();
    if (output.contains("gdb")) {
        m_engineType = GdbEngineType;

        // Version
        bool isMacGdb, isQnxGdb;
        int version = 0, buildVersion = 0;
        Debugger::Internal::extractGdbVersion(output,
            &version, &buildVersion, &isMacGdb, &isQnxGdb);
        if (version)
            m_version = QString::fromLatin1("%1.%2.%3")
                .arg(version / 10000).arg((version / 100) % 100).arg(version % 100);

        // ABI
        const bool unableToFindAVersion = (0 == version);
        const bool gdbSupportsConfigurationFlag = (version >= 70700);
        if (gdbSupportsConfigurationFlag || unableToFindAVersion) {
            const QString gdbConfiguration = getGdbConfiguration(m_command, sysEnv);
            const QString gdbTargetAbiString =
                    extractGdbTargetAbiStringFromGdbOutput(gdbConfiguration);
            if (!gdbTargetAbiString.isEmpty()) {
                m_abis.append(Abi::abiFromTargetTriplet(gdbTargetAbiString));
                return;
            }
        }

        // ABI: legacy: the target was removed from the output of --version with
        // https://sourceware.org/git/gitweb.cgi?p=binutils-gdb.git;a=commit;h=c61b06a19a34baab66e3809c7b41b0c31009ed9f
        auto legacyGdbTargetAbiString = extractGdbTargetAbiStringFromGdbOutput(output);
        if (!legacyGdbTargetAbiString.isEmpty()) {
            // remove trailing "
            legacyGdbTargetAbiString =
                    legacyGdbTargetAbiString.left(legacyGdbTargetAbiString.length() - 1);
            m_abis.append(Abi::abiFromTargetTriplet(legacyGdbTargetAbiString));
            return;
        }

        qWarning() << "Unable to determine gdb target ABI";
        //! \note If unable to determine the GDB ABI, no ABI is appended to m_abis here.
        return;
    }
    if (output.startsWith("lldb") || output.startsWith("LLDB")) {
        m_engineType = LldbEngineType;
        m_abis = Abi::abisOfBinary(m_command);

        // Version
        // Self-build binaries also emit clang and llvm revision.
        const QString line = output.split('\n')[0];
        if (line.startsWith(("lldb version "))) { // Linux typically.
            int pos1 = int(strlen("lldb version "));
            int pos2 = line.indexOf(' ', pos1);
            m_version = line.mid(pos1, pos2 - pos1);
        } else if (line.startsWith("lldb-") || line.startsWith("LLDB-")) { // Mac typically.
            m_version = line.mid(5);
        }
        return;
    }
    if (output.startsWith("cdb")) {
        // "cdb version 6.2.9200.16384"
        m_engineType = CdbEngineType;
        m_abis = Abi::abisOfBinary(m_command);
        m_version = output.section(' ', 2);
        return;
    }
    if (output.startsWith("Python")) {
        m_engineType = PdbEngineType;
        return;
    }
    if (error)
        *error = output;
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
    case UvscEngineType:
        return QLatin1String("UVSC");
    default:
        return QString();
    }
}

QStringList DebuggerItem::abiNames() const
{
    QStringList list;
    for (const Abi &abi : m_abis)
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
        return Icons::CRITICAL.icon();
    if (!m_command.isExecutableFile())
        return Icons::WARNING.icon();
    if (!m_workingDirectory.isEmpty() && !m_workingDirectory.isDir())
        return Icons::WARNING.icon();
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
            && m_detectionSource == other.m_detectionSource
            && m_command == other.m_command
            && m_workingDirectory == other.m_workingDirectory;
}

QVariantMap DebuggerItem::toMap() const
{
    QVariantMap data;
    data.insert(DEBUGGER_INFORMATION_DISPLAYNAME, m_unexpandedDisplayName);
    data.insert(DEBUGGER_INFORMATION_ID, m_id);
    data.insert(DEBUGGER_INFORMATION_COMMAND, m_command.toVariant());
    data.insert(DEBUGGER_INFORMATION_WORKINGDIRECTORY, m_workingDirectory.toVariant());
    data.insert(DEBUGGER_INFORMATION_ENGINETYPE, int(m_engineType));
    data.insert(DEBUGGER_INFORMATION_AUTODETECTED, m_isAutoDetected);
    data.insert(DEBUGGER_INFORMATION_DETECTION_SOURCE, m_detectionSource);
    data.insert(DEBUGGER_INFORMATION_VERSION, m_version);
    data.insert(DEBUGGER_INFORMATION_ABIS, abiNames());
    data.insert(DEBUGGER_INFORMATION_LASTMODIFIED, m_lastModified);
    return data;
}

QString DebuggerItem::displayName() const
{
    if (!m_unexpandedDisplayName.contains('%'))
        return m_unexpandedDisplayName;

    MacroExpander expander;
    expander.registerVariable("Debugger:Type", DebuggerKitAspect::tr("Type of Debugger Backend"),
        [this] { return engineTypeName(); });
    expander.registerVariable("Debugger:Version", DebuggerKitAspect::tr("Debugger"),
        [this] { return !m_version.isEmpty() ? m_version :
                                               DebuggerKitAspect::tr("Unknown debugger version"); });
    expander.registerVariable("Debugger:Abi", DebuggerKitAspect::tr("Debugger"),
        [this] { return !m_abis.isEmpty() ? abiNames().join(' ') :
                                            DebuggerKitAspect::tr("Unknown debugger ABI"); });
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

void DebuggerItem::setCommand(const FilePath &command)
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

void DebuggerItem::setAbis(const Abis &abis)
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
    if (HostOsInfo::isWindowsHost() && engineType == CdbEngineType
        && targetAbi.osFlavor() >= Abi::WindowsMsvc2005Flavor
        && targetAbi.osFlavor() <= Abi::WindowsLastMsvcFlavor) {
        return DebuggerItem::MatchesPerfectly;
    }
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
    for (const Abi &debuggerAbi : m_abis) {
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
