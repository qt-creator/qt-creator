// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debuggeritem.h"

#include "debuggerprotocol.h"
#include "debuggertr.h"

#include <coreplugin/icore.h>

#include <projectexplorer/abi.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/macroexpander.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>
#include <utils/winutils.h>

#include <QUuid>

using namespace Debugger;
using namespace Debugger::Internal;
using namespace ProjectExplorer;
using namespace Utils;

const DetectionSource DebuggerItem::genericDetectionSource
    = {DetectionSource::FromSystem, "Generic"};

static Result<QString> fetchVersionOutput(const FilePath &executable, Environment environment)
{
    // CDB only understands the single-dash -version, whereas GDB and LLDB are
    // happy with both -version and --version. So use the "working" -version
    // except for the experimental LLDB-MI which insists on --version.
    QString version = "-version";
    if (executable.baseName().toLower().contains("lldb-mi")
        || executable.baseName().startsWith("LLDBFrontend")) { // Comes with Android Studio
        version = "--version";
    }

    // QNX gdb unconditionally checks whether the QNX_TARGET env variable is
    // set and bails otherwise, even when it is not used by the specific
    // codepath triggered by the --version and --configuration arguments. The
    // hack below tricks it into giving us the information we want.
    environment.set("QNX_TARGET", QString());

    // On Windows, we need to prevent the Windows Error Reporting dialog from
    // popping up when a candidate is missing required DLLs.
    const WindowsCrashDialogBlocker blocker;

    Process proc;
    proc.setEnvironment(environment);
    proc.setCommand({executable, {version}});
    proc.runBlocking();
    QString output = proc.allOutput().trimmed();
    if (proc.result() != ProcessResult::FinishedWithSuccess)
        return ResultError(output);

    return output;
}

static std::optional<QString> extractLldbVersion(const QString &fromOutput)
{
    // Self-build binaries also emit clang and llvm revision.
    const QString line = fromOutput.split('\n')[0];

    // Linux typically, or some Windows builds
    const QString nonMacOSPrefix = "lldb version ";
    if (line.contains(nonMacOSPrefix)) {
        const qsizetype pos1 = line.indexOf(nonMacOSPrefix) + nonMacOSPrefix.length();
        const qsizetype pos2 = line.indexOf(' ', pos1);
        return line.mid(pos1, pos2 - pos1);
    }

    // Mac typically
    const QString macOSPrefix = "lldb-";
    if (line.startsWith(macOSPrefix, Qt::CaseInsensitive)) {
        return line.mid(macOSPrefix.length());
    }

    return {};
}

//! Return the configuration of gdb as a list of --key=value
//! \note That the list will also contain some output not in this format.
static QString getGdbConfiguration(const FilePath &command, const Environment &sysEnv)
{
    // run gdb with the --configuration opion
    Process proc;
    proc.setEnvironment(sysEnv);
    proc.setCommand({command, {"--configuration"}});
    proc.runBlocking();
    return proc.allOutput();
}

//! Extract the target ABI identifier from GDB output
//! \return QString() (aka Null) if unable to find something
static std::optional<QString> extractGdbTargetAbiStringFromGdbOutput(const QString &gdbOutput)
{
    const auto outputLines = gdbOutput.split('\n');
    const auto whitespaceSeparatedTokens = outputLines.join(' ').split(' ', Qt::SkipEmptyParts);

    const QString targetKey{"--target="};
    const QString targetValue
        = Utils::findOrDefault(whitespaceSeparatedTokens, [&targetKey](const QString &token) {
              return token.startsWith(targetKey);
          });
    if (!targetValue.isEmpty())
        return targetValue.mid(targetKey.size());

    return {};
}

static std::optional<Abi> extractGdbTargetAbi(
    const FilePath &fromExecutable, const int version, const Environment &env)
{
    // ABI
    const bool unableToFindAVersion = (0 == version);
    const bool gdbSupportsConfigurationFlag = (version >= 70700);
    if (!unableToFindAVersion && !gdbSupportsConfigurationFlag)
        return {};

    const std::optional<QString> gdbTargetAbiString = extractGdbTargetAbiStringFromGdbOutput(
        getGdbConfiguration(fromExecutable, env));
    if (!gdbTargetAbiString)
        return {};

    return Abi::abiFromTargetTriplet(*gdbTargetAbiString);
}

static std::optional<Abi> extractLegacyGdbTargetAbi(const QString &fromOutput)
{
    // ABI: legacy: the target was removed from the output of --version with
    // https://sourceware.org/git/gitweb.cgi?p=binutils-gdb.git;a=commit;h=c61b06a19a34baab66e3809c7b41b0c31009ed9f
    std::optional<QString> legacyGdbTargetAbiString = extractGdbTargetAbiStringFromGdbOutput(
        fromOutput);
    if (!legacyGdbTargetAbiString)
        return {};

    legacyGdbTargetAbiString->chop(1); // remove trailing "
    return Abi::abiFromTargetTriplet(*legacyGdbTargetAbiString);
}

static Utils::Result<DebuggerItem::TechnicalData> extractLldbTechnicalData(
    const FilePath &fromExecutable, const Environment &env, const QString &dapServerSuffix)
{
    // As of LLVM 19.1.4 `lldb-dap`/`lldb-vscode` has no `--version` switch
    // so we cannot use that binary directly.
    // However it should be pretty safe to assume that if there is an `lldb` binary
    // next to it both are of the same version.
    // It also makes sense that both follow the same naming scheme,
    // so replacing the name should work.
    QString binaryName = fromExecutable.fileName();
    binaryName.replace(dapServerSuffix, QString{});
    const FilePath lldb = fromExecutable.parentDir() / binaryName;
    if (!lldb.exists()) {
        return ResultError(QString{"%1 does not exist alongside %2"}
                                   .arg(lldb.fileNameView(), fromExecutable.toUserOutput()));
    }
    if (!lldb.isExecutableFile()) {
        return ResultError(QString{"%1 exists alongside %2 but is not executable"}
                                   .arg(lldb.fileNameView(), fromExecutable.toUserOutput()));
    }

    const Result<QString> output = fetchVersionOutput(lldb, env);
    if (!output)
        return make_unexpected(output.error());

    return DebuggerItem::TechnicalData{
        .engineType = LldbDapEngineType,
        .abis = Abi::abisOfBinary(fromExecutable),
        .version = extractLldbVersion(*output).value_or(""),
    };
}

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

// --------------------------------------------------------------------------
// DebuggerItem
// --------------------------------------------------------------------------

Result<DebuggerItem::TechnicalData> DebuggerItem::TechnicalData::extract(
    const FilePath &fromExecutable, const std::optional<Environment> &customEnvironment)
{
    Environment env = customEnvironment.value_or(fromExecutable.deviceEnvironment());
    DebuggerItem::addAndroidLldbPythonEnv(fromExecutable, env);
    DebuggerItem::fixupAndroidLlldbPythonDylib(fromExecutable);

    if (qgetenv("QTC_ENABLE_NATIVE_DAP_DEBUGGERS").toInt() != 0) {
        for (const auto &dapServerSuffix : {QString{"-dap"}, QString{"-vscode"}}) {
            const QString dapServerName = QString{"lldb%1"}.arg(dapServerSuffix);
            if (fromExecutable.fileName().startsWith(dapServerName, Qt::CaseInsensitive)) {
                return extractLldbTechnicalData(fromExecutable, env, dapServerSuffix);
            }
        }
    }

    // We don't need to start the uVision executable to determine its version.
    if (HostOsInfo::isWindowsHost() && fromExecutable.baseName() == "UV4") {
        QString errorMessage;
        QString version = winGetDLLVersion(
            WinDLLFileVersion, fromExecutable.absoluteFilePath().path(), &errorMessage);

        if (!errorMessage.isEmpty())
            return ResultError(std::move(errorMessage));

        return DebuggerItem::TechnicalData{
            .engineType = UvscEngineType,
            .abis = {},
            .version = std::move(version),
        };
    }

    const Result<QString> output = fetchVersionOutput(fromExecutable, env);
    if (!output) {
        return ResultError(output.error());
    }

    if (output->contains("gdb")) {
        // Version
        bool isMacGdb = false;
        bool isQnxGdb = false;
        int version = 0;
        int buildVersion = 0;
        Debugger::Internal::extractGdbVersion(*output, &version, &buildVersion, &isMacGdb, &isQnxGdb);
        QString versionStr = version != 0 ? QString::fromLatin1("%1.%2.%3")
                                                .arg(version / 10000)
                                                .arg((version / 100) % 100)
                                                .arg(version % 100)
                                          : "";
        Abis abis;
        if (std::optional<Abi> abi = extractGdbTargetAbi(fromExecutable, version, env)) {
            abis = {std::move(*abi)};
        } else if (std::optional<Abi> abi = extractLegacyGdbTargetAbi(*output)) {
            abis = {std::move(*abi)};
        } else {
            qWarning() << "Unable to determine gdb target ABI from" << *output;
        }

        return DebuggerItem::TechnicalData{
            .engineType = GdbEngineType, .abis = abis, .version = std::move(versionStr)};
    }

    if (output->contains("lldb") || output->startsWith("LLDB")) {
        return DebuggerItem::TechnicalData{
            .engineType = LldbEngineType,
            .abis = Abi::abisOfBinary(fromExecutable),
            .version = extractLldbVersion(*output).value_or(""),
        };
    }
    if (output->startsWith("cdb")) {
        // "cdb version 6.2.9200.16384"
        return DebuggerItem::TechnicalData{
            .engineType = CdbEngineType,
            .abis = Abi::abisOfBinary(fromExecutable),
            .version = output->section(' ', 2),
        };
    }

    return ResultError(
        QString{"Failed to determine debugger engine type from '%1'"}.arg(*output));
}

bool DebuggerItem::TechnicalData::isEmpty() const
{
    return version.isEmpty() && abis.isEmpty() && engineType == NoEngineType;
}

DebuggerItem::DebuggerItem(const QVariant &id)
{
    m_id = id;
}

DebuggerItem::DebuggerItem(const Store &data)
{
    m_id = data.value(DEBUGGER_INFORMATION_ID).toString();
    m_command = FilePath::fromSettings(data.value(DEBUGGER_INFORMATION_COMMAND));
    m_workingDirectory = FilePath::fromSettings(data.value(DEBUGGER_INFORMATION_WORKINGDIRECTORY));
    m_unexpandedDisplayName = data.value(DEBUGGER_INFORMATION_DISPLAYNAME).toString();
    const bool autoDetected = data.value(DEBUGGER_INFORMATION_AUTODETECTED, false).toBool();
    const QString detectionSourceId = data.value(DEBUGGER_INFORMATION_DETECTION_SOURCE).toString();
    m_detectionSource
        = {autoDetected ? DetectionSource::FromSystem : DetectionSource::Manual, detectionSourceId};

    m_technicalData.version = data.value(DEBUGGER_INFORMATION_VERSION).toString();
    m_technicalData.engineType = DebuggerEngineType(
        data.value(DEBUGGER_INFORMATION_ENGINETYPE, static_cast<int>(NoEngineType)).toInt());
    m_lastModified = data.value(DEBUGGER_INFORMATION_LASTMODIFIED).toDateTime();

    const QStringList abis = data.value(DEBUGGER_INFORMATION_ABIS).toStringList();
    for (const QString &a : abis) {
        Abi abi = Abi::fromString(a);
        if (!abi.isNull())
            m_technicalData.abis.append(abi);
    }

    const Abis &validAbis = m_technicalData.abis;
    const bool mightBeAPreQnxSeparateOSQnxDebugger = m_command.fileName().startsWith("nto")
                                                     && validAbis.count() == 1
                                                     && validAbis.front().os() == Abi::UnknownOS
                                                     && validAbis.front().osFlavor()
                                                            == Abi::UnknownFlavor
                                                     && validAbis.front().binaryFormat()
                                                            == Abi::UnknownFormat;

    if (m_technicalData.isEmpty() || mightBeAPreQnxSeparateOSQnxDebugger)
        reinitializeFromFile();
}

void DebuggerItem::createId()
{
    QTC_ASSERT(!m_id.isValid(), return);
    m_id = QUuid::createUuid().toString();
}

void DebuggerItem::reinitializeFromFile(QString *error, Utils::Environment *customEnv)
{
    if (isGeneric())
        return;

    auto env = customEnv ? std::optional<Environment>{*customEnv} : std::optional<Environment>{};
    Result<TechnicalData> technicalData = TechnicalData::extract(m_command, env);
    if (!technicalData) {
        if (error)
            *error = technicalData.error();
        m_technicalData.engineType = NoEngineType;
        return;
    }

    m_technicalData = std::move(*technicalData);
    m_lastModified = m_command.lastModified();
}

bool DebuggerItem::addAndroidLldbPythonEnv(const Utils::FilePath &lldbCmd, Utils::Environment &env)
{
    if (lldbCmd.baseName().contains("lldb")
            && lldbCmd.path().contains("/toolchains/llvm/prebuilt/")) {
        const FilePath pythonDir = lldbCmd.parentDir().parentDir().pathAppended("python3");
        const FilePath pythonBinDir =
                HostOsInfo::isAnyUnixHost() ? pythonDir.pathAppended("bin") : pythonDir;
        if (pythonBinDir.exists()) {
            env.set("PYTHONHOME", pythonDir.toUserOutput());
            env.prependOrSetPath(pythonBinDir);

            if (HostOsInfo::isAnyUnixHost()) {
                const FilePath pythonLibDir = pythonDir.pathAppended("lib");
                if (pythonLibDir.exists())
                    env.prependOrSetLibrarySearchPath(pythonLibDir);
            }

            return true;
        }
    }
    return false;
}

bool DebuggerItem::fixupAndroidLlldbPythonDylib(const FilePath &lldbCmd)
{
    if (!lldbCmd.baseName().contains("lldb")
        || !lldbCmd.path().contains("/toolchains/llvm/prebuilt/") || !HostOsInfo::isMacHost())
        return false;

    const FilePath lldbBaseDir = lldbCmd.parentDir().parentDir();
    const FilePath pythonLibDir = lldbBaseDir / "python3" / "lib";
    if (!pythonLibDir.exists())
        return false;

    pythonLibDir.iterateDirectory(
        [lldbBaseDir](const FilePath &file) {
            if (file.fileName().startsWith("libpython3")) {
                const FilePath lldbLibPython = lldbBaseDir / "lib" / file.fileName();
                if (!lldbLibPython.exists())
                    file.copyFile(lldbLibPython);
                return IterationPolicy::Stop;
            }
            return IterationPolicy::Continue;
        },
        {{"*.dylib"}});
    return true;
}

QString DebuggerItem::engineTypeName() const
{
    switch (m_technicalData.engineType) {
    case NoEngineType:
        return Tr::tr("Not recognized");
    case GdbEngineType:
        return QLatin1String("GDB");
    case CdbEngineType:
        return QLatin1String("CDB");
    case LldbEngineType:
        return QLatin1String("LLDB");
    case GdbDapEngineType:
        return QLatin1String("GDB DAP");
    case LldbDapEngineType:
        return QLatin1String("LLDB DAP");
    case UvscEngineType:
        return QLatin1String("UVSC");
    default:
        return QString();
    }
}

bool DebuggerItem::isGeneric() const
{
    return m_detectionSource.id == "Generic";
}

QStringList DebuggerItem::abiNames() const
{
    QStringList list;
    for (const Abi &abi : m_technicalData.abis)
        list.append(abi.toString());
    return list;
}

QDateTime DebuggerItem::lastModified() const
{
    return m_lastModified;
}

void DebuggerItem::setLastModified(const QDateTime &timestamp)
{
    m_lastModified = timestamp;
}

DebuggerItem::Problem DebuggerItem::problem() const
{
    if (isGeneric() || !m_id.isValid()) // Id can only be invalid for the "none" item.
        return Problem::None;
    if (m_technicalData.engineType == NoEngineType)
        return Problem::NoEngine;
    if (!m_command.isExecutableFile())
        return Problem::InvalidCommand;
    if (!m_workingDirectory.isEmpty() && !m_workingDirectory.isDir())
        return Problem::InvalidWorkingDir;
    return Problem::None;
}

QIcon DebuggerItem::decoration() const
{
    switch (problem()) {
    case Problem::NoEngine:
        return Icons::CRITICAL.icon();
    case Problem::InvalidCommand:
    case Problem::InvalidWorkingDir:
        return Icons::WARNING.icon();
    case Problem::None:
        break;
    }
    return {};
}

QString DebuggerItem::validityMessage() const
{
    switch (problem()) {
    case Problem::NoEngine:
        return Tr::tr("Could not determine debugger type");
    case Problem::InvalidCommand:
        return Tr::tr("Invalid debugger command");
    case Problem::InvalidWorkingDir:
        return Tr::tr("Invalid working directory");
    case Problem::None:
        break;
    }
    return {};
}

bool DebuggerItem::operator==(const DebuggerItem &other) const
{
    return m_id == other.m_id
            && m_unexpandedDisplayName == other.m_unexpandedDisplayName
            && m_detectionSource == other.m_detectionSource
            && m_command == other.m_command
            && m_workingDirectory == other.m_workingDirectory;
}

Store DebuggerItem::toMap() const
{
    Store data;
    data.insert(DEBUGGER_INFORMATION_DISPLAYNAME, m_unexpandedDisplayName);
    data.insert(DEBUGGER_INFORMATION_ID, m_id);
    data.insert(DEBUGGER_INFORMATION_COMMAND, m_command.toSettings());
    data.insert(DEBUGGER_INFORMATION_WORKINGDIRECTORY, m_workingDirectory.toSettings());
    data.insert(DEBUGGER_INFORMATION_ENGINETYPE, int(m_technicalData.engineType));
    data.insert(DEBUGGER_INFORMATION_AUTODETECTED, m_detectionSource.isAutoDetected());
    data.insert(DEBUGGER_INFORMATION_DETECTION_SOURCE, m_detectionSource.id);
    data.insert(DEBUGGER_INFORMATION_VERSION, m_technicalData.version);
    data.insert(DEBUGGER_INFORMATION_ABIS, abiNames());
    data.insert(DEBUGGER_INFORMATION_LASTMODIFIED, m_lastModified);
    return data;
}

QString DebuggerItem::displayName() const
{
    if (!m_unexpandedDisplayName.contains('%'))
        return m_unexpandedDisplayName;

    MacroExpander expander;
    expander.registerVariable("Debugger:Type", Tr::tr("Type of Debugger Backend"), [this] {
        return engineTypeName();
    });
    expander.registerVariable(
        "Debugger:Version", Tr::tr("Debugger"), [&version = m_technicalData.version] {
            return !version.isEmpty() ? version : Tr::tr("Unknown debugger version");
        });
    expander.registerVariable("Debugger:Abi", Tr::tr("Debugger"), [this] {
        return !m_technicalData.abis.isEmpty() ? abiNames().join(' ')
                                               : Tr::tr("Unknown debugger ABI");
    });
    return expander.expand(m_unexpandedDisplayName);
}

void DebuggerItem::setUnexpandedDisplayName(const QString &displayName)
{
    m_unexpandedDisplayName = displayName;
}

void DebuggerItem::setEngineType(const DebuggerEngineType &engineType)
{
    m_technicalData.engineType = engineType;
}

void DebuggerItem::setCommand(const FilePath &command)
{
    m_command = command;
}

void DebuggerItem::setAutoDetected(bool isAutoDetected)
{
    m_detectionSource.type = isAutoDetected ? ProjectExplorer::DetectionSource::FromSystem
                                            : ProjectExplorer::DetectionSource::Manual;
}

QString DebuggerItem::version() const
{
    return m_technicalData.version;
}

void DebuggerItem::setVersion(const QString &version)
{
    m_technicalData.version = version;
}

void DebuggerItem::setAbis(const Abis &abis)
{
    m_technicalData.abis = abis;
}

void DebuggerItem::setAbi(const Abi &abi)
{
    m_technicalData.abis.clear();
    m_technicalData.abis.append(abi);
}

static DebuggerItem::MatchLevel matchSingle(const Abi &debuggerAbi, const Abi &targetAbi, DebuggerEngineType engineType)
{
    DebuggerItem::MatchLevel matchOnMultiarch = DebuggerItem::DoesNotMatch;
    const bool isMsvcTarget = targetAbi.osFlavor() >= Abi::WindowsMsvc2005Flavor &&
            targetAbi.osFlavor() <= Abi::WindowsLastMsvcFlavor;
    if (!isMsvcTarget && (engineType == GdbEngineType || engineType == LldbEngineType))
        matchOnMultiarch = DebuggerItem::MatchesSomewhat;
    // arm64 cdb can debug x64 targets
    if (isMsvcTarget && engineType == CdbEngineType
            && debuggerAbi.architecture() == Abi::ArmArchitecture
            && targetAbi.architecture() == Abi::X86Architecture
            && debuggerAbi.wordWidth() == 64 && targetAbi.wordWidth() == 64)
        return DebuggerItem::MatchesSomewhat;
    if (debuggerAbi.architecture() != Abi::UnknownArchitecture
            && debuggerAbi.architecture() != targetAbi.architecture())
        return matchOnMultiarch;

    if (debuggerAbi.os() != Abi::UnknownOS
            && debuggerAbi.os() != targetAbi.os())
        return matchOnMultiarch;

    if (debuggerAbi.binaryFormat() != Abi::UnknownFormat
            && debuggerAbi.binaryFormat() != targetAbi.binaryFormat())
        return matchOnMultiarch;

    if (debuggerAbi.os() == Abi::WindowsOS) {
        if (debuggerAbi.osFlavor() == Abi::WindowsMSysFlavor && targetAbi.osFlavor() != Abi::WindowsMSysFlavor)
            return matchOnMultiarch;
        if (debuggerAbi.osFlavor() != Abi::WindowsMSysFlavor && targetAbi.osFlavor() == Abi::WindowsMSysFlavor)
            return matchOnMultiarch;
    }

    if (debuggerAbi.wordWidth() == 64 && targetAbi.wordWidth() == 32)
        return DebuggerItem::MatchesSomewhat;
    if (debuggerAbi.wordWidth() != 0 && debuggerAbi.wordWidth() != targetAbi.wordWidth())
        return matchOnMultiarch;

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
    for (const Abi &debuggerAbi : m_technicalData.abis) {
        MatchLevel currentMatch = matchSingle(debuggerAbi, targetAbi, m_technicalData.engineType);
        if (currentMatch > bestMatch)
            bestMatch = currentMatch;
    }
    return bestMatch;
}

bool DebuggerItem::isValid() const
{
    return !m_id.isNull();
}

void DebuggerItem::setDetectionSource(const QString &source)
{
    m_detectionSource.id = source;
}

DetectionSource DebuggerItem::detectionSource() const
{
    return m_detectionSource;
}

void DebuggerItem::setDetectionSource(const ProjectExplorer::DetectionSource &source)
{
    m_detectionSource = source;
}

bool DebuggerItem::isAutoDetected() const
{
    return m_detectionSource.isAutoDetected();
}

} // namespace Debugger
