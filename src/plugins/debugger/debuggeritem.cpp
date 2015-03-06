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

#include "debuggeritem.h"
#include "debuggerkitinformation.h"
#include "debuggerkitconfigwidget.h"
#include "debuggeroptionspage.h"
#include "debuggerprotocol.h"

#include <projectexplorer/abi.h>

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

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
    m_command = FileName::fromUserInput(data.value(QLatin1String(DEBUGGER_INFORMATION_COMMAND)).toString());
    m_id = data.value(QLatin1String(DEBUGGER_INFORMATION_ID)).toString();
    m_unexpandedDisplayName = data.value(QLatin1String(DEBUGGER_INFORMATION_DISPLAYNAME)).toString();
    m_isAutoDetected = data.value(QLatin1String(DEBUGGER_INFORMATION_AUTODETECTED), false).toBool();
    m_autoDetectionSource = data.value(QLatin1String(DEBUGGER_INFORMATION_AUTODETECTION_SOURCE)).toString();
    m_version = data.value(QLatin1String(DEBUGGER_INFORMATION_VERSION)).toString();
    m_engineType = DebuggerEngineType(data.value(QLatin1String(DEBUGGER_INFORMATION_ENGINETYPE),
                                                 static_cast<int>(NoEngineType)).toInt());

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
    QProcess proc;
    // CDB only understands the single-dash -version, whereas GDB and LLDB are
    // happy with both -version and --version. So use the "working" -version.
    proc.start(m_command.toString(), QStringList() << QLatin1String("-version"));
    if (!proc.waitForStarted() || !proc.waitForFinished()) {
        m_engineType = NoEngineType;
        return;
    }
    m_abis.clear();
    QByteArray ba = proc.readAll();
    if (ba.contains("gdb")) {
        m_engineType = GdbEngineType;
        const char needle[] = "This GDB was configured as \"";
        // E.g.  "--host=i686-pc-linux-gnu --target=arm-unknown-nto-qnx6.5.0".
        // or "i686-linux-gnu"
        int pos1 = ba.indexOf(needle);
        if (pos1 != -1) {
            pos1 += int(strlen(needle));
            int pos2 = ba.indexOf('"', pos1 + 1);
            QByteArray target = ba.mid(pos1, pos2 - pos1);
            int pos3 = target.indexOf("--target=");
            if (pos3 >= 0)
                target = target.mid(pos3 + 9);
            m_abis.append(Abi::abiFromTargetTriplet(QString::fromLatin1(target)));
        } else {
            // Fallback.
            m_abis = Abi::abisOfBinary(m_command); // FIXME: Wrong.
        }

        // Version
        QString all = QString::fromUtf8(ba);
        bool isMacGdb, isQnxGdb;
        int version = 0, buildVersion = 0;
        Debugger::Internal::extractGdbVersion(all,
            &version, &buildVersion, &isMacGdb, &isQnxGdb);
        if (version)
            m_version = QString::fromLatin1("%1.%2.%3")
                .arg(version / 10000).arg((version / 100) % 100).arg(version % 100);
        return;
    }
    if (ba.startsWith("lldb") || ba.startsWith("LLDB")) {
        m_engineType = LldbEngineType;
        m_abis = Abi::abisOfBinary(m_command);

        // Version
        if (ba.startsWith(("lldb version "))) { // Linux typically.
            int pos1 = int(strlen("lldb version "));
            int pos2 = ba.indexOf(' ', pos1);
            m_version = QString::fromLatin1(ba.mid(pos1, pos2 - pos1));
        } else if (ba.startsWith("lldb-") || ba.startsWith("LLDB-")) { // Mac typically.
            m_version = QString::fromLatin1(ba.mid(5));
        }
        return;
    }
    if (ba.startsWith("cdb")) {
        // "cdb version 6.2.9200.16384"
        m_engineType = CdbEngineType;
        m_abis = Abi::abisOfBinary(m_command);
        m_version = QString::fromLatin1(ba).section(QLatin1Char(' '), 2);
        return;
    }
    if (ba.startsWith("Python")) {
        m_engineType = PdbEngineType;
        return;
    }
    m_engineType = NoEngineType;
}

QString DebuggerItem::engineTypeName() const
{
    switch (m_engineType) {
    case NoEngineType:
        return DebuggerOptionsPage::tr("Not recognized");
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

bool DebuggerItem::operator==(const DebuggerItem &other) const
{
    return m_id == other.m_id
            && m_unexpandedDisplayName == other.m_unexpandedDisplayName
            && m_isAutoDetected == other.m_isAutoDetected
            && m_command == other.m_command;
}

QVariantMap DebuggerItem::toMap() const
{
    QVariantMap data;
    data.insert(QLatin1String(DEBUGGER_INFORMATION_DISPLAYNAME), m_unexpandedDisplayName);
    data.insert(QLatin1String(DEBUGGER_INFORMATION_ID), m_id);
    data.insert(QLatin1String(DEBUGGER_INFORMATION_COMMAND), m_command.toString());
    data.insert(QLatin1String(DEBUGGER_INFORMATION_ENGINETYPE), int(m_engineType));
    data.insert(QLatin1String(DEBUGGER_INFORMATION_AUTODETECTED), m_isAutoDetected);
    data.insert(QLatin1String(DEBUGGER_INFORMATION_AUTODETECTION_SOURCE), m_autoDetectionSource);
    data.insert(QLatin1String(DEBUGGER_INFORMATION_VERSION), m_version);
    data.insert(QLatin1String(DEBUGGER_INFORMATION_ABIS), abiNames());
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
    if (HostOsInfo::isMacHost() && engineType == LldbEngineType && targetAbi.os() == Abi::MacOS)
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

#ifdef WITH_TESTS

namespace Internal {

void DebuggerPlugin::testDebuggerMatching_data()
{
    QTest::addColumn<QStringList>("debugger");
    QTest::addColumn<QString>("target");
    QTest::addColumn<int>("result");

    QTest::newRow("Invalid data")
            << QStringList()
            << QString()
            << int(DebuggerItem::DoesNotMatch);
    QTest::newRow("Invalid debugger")
            << QStringList()
            << QString::fromLatin1("x86-linux-generic-elf-32bit")
            << int(DebuggerItem::DoesNotMatch);
    QTest::newRow("Invalid target")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-32bit"))
            << QString()
            << int(DebuggerItem::DoesNotMatch);

    QTest::newRow("Fuzzy match 1")
            << (QStringList() << QLatin1String("unknown-unknown-unknown-unknown-0bit"))
            << QString::fromLatin1("x86-linux-generic-elf-32bit")
            << int(DebuggerItem::MatchesWell); // Is this the expected behavior?
    QTest::newRow("Fuzzy match 2")
            << (QStringList() << QLatin1String("unknown-unknown-unknown-unknown-0bit"))
            << QString::fromLatin1("arm-windows-msys-pe-64bit")
            << int(DebuggerItem::MatchesWell); // Is this the expected behavior?

    QTest::newRow("Architecture mismatch")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-32bit"))
            << QString::fromLatin1("arm-linux-generic-elf-32bit")
            << int(DebuggerItem::DoesNotMatch);
    QTest::newRow("OS mismatch")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-32bit"))
            << QString::fromLatin1("x86-macosx-generic-elf-32bit")
            << int(DebuggerItem::DoesNotMatch);
    QTest::newRow("Format mismatch")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-32bit"))
            << QString::fromLatin1("x86-linux-generic-pe-32bit")
            << int(DebuggerItem::DoesNotMatch);

    QTest::newRow("Linux perfect match")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-32bit"))
            << QString::fromLatin1("x86-linux-generic-elf-32bit")
            << int(DebuggerItem::MatchesWell);
    QTest::newRow("Linux match")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-64bit"))
            << QString::fromLatin1("x86-linux-generic-elf-32bit")
            << int(DebuggerItem::MatchesSomewhat);

    QTest::newRow("Windows perfect match 1")
            << (QStringList() << QLatin1String("x86-windows-msvc2013-pe-64bit"))
            << QString::fromLatin1("x86-windows-msvc2013-pe-64bit")
            << int(DebuggerItem::MatchesWell);
    QTest::newRow("Windows perfect match 2")
            << (QStringList() << QLatin1String("x86-windows-msvc2013-pe-64bit"))
            << QString::fromLatin1("x86-windows-msvc2012-pe-64bit")
            << int(DebuggerItem::MatchesWell);
    QTest::newRow("Windows match 1")
            << (QStringList() << QLatin1String("x86-windows-msvc2013-pe-64bit"))
            << QString::fromLatin1("x86-windows-msvc2013-pe-32bit")
            << int(DebuggerItem::MatchesSomewhat);
    QTest::newRow("Windows match 2")
            << (QStringList() << QLatin1String("x86-windows-msvc2013-pe-64bit"))
            << QString::fromLatin1("x86-windows-msvc2012-pe-32bit")
            << int(DebuggerItem::MatchesSomewhat);
    QTest::newRow("Windows mismatch on word size")
            << (QStringList() << QLatin1String("x86-windows-msvc2013-pe-32bit"))
            << QString::fromLatin1("x86-windows-msvc2013-pe-64bit")
            << int(DebuggerItem::DoesNotMatch);
    QTest::newRow("Windows mismatch on osflavor 1")
            << (QStringList() << QLatin1String("x86-windows-msvc2013-pe-32bit"))
            << QString::fromLatin1("x86-windows-msys-pe-64bit")
            << int(DebuggerItem::DoesNotMatch);
    QTest::newRow("Windows mismatch on osflavor 2")
            << (QStringList() << QLatin1String("x86-windows-msys-pe-32bit"))
            << QString::fromLatin1("x86-windows-msvc2010-pe-64bit")
            << int(DebuggerItem::DoesNotMatch);
}

void DebuggerPlugin::testDebuggerMatching()
{
    QFETCH(QStringList, debugger);
    QFETCH(QString, target);
    QFETCH(int, result);

    DebuggerItem::MatchLevel expectedLevel = static_cast<DebuggerItem::MatchLevel>(result);

    QList<Abi> debuggerAbis;
    foreach (const QString &abi, debugger)
        debuggerAbis << Abi(abi);

    DebuggerItem item;
    item.setAbis(debuggerAbis);

    DebuggerItem::MatchLevel level = item.matchTarget(Abi(target));
    if (level == DebuggerItem::MatchesPerfectly)
        level = DebuggerItem::MatchesWell;

    QCOMPARE(expectedLevel, level);
}

} // namespace Internal

#endif

} // namespace Debugger;
