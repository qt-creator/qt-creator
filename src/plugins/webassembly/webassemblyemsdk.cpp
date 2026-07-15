// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "webassemblyemsdk.h"
#include "webassemblytr.h"

#include "webassemblyconstants.h"

#include <coreplugin/icore.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>
#include <utils/settingsdatabase.h>

#include <QCache>

using namespace Utils;

namespace WebAssembly::Internal::WebAssemblyEmSdk {

const char emSdkEnvTSFileKey[] = "WebAssembly/emSdkEnvTimeStampFile";
const char emSdkEnvTSKey[] = "WebAssembly/emSdkEnvTimeStamp";
const char emSdkEnvOutputKey[] = "WebAssembly/emSdkEnvOutput1";

const char emSdkVersionTSFileKey[] = "WebAssembly/emSdkVersionTimeStampFile";
const char emSdkVersionTSKey[] = "WebAssembly/emSdkVersionTimeStamp";
const char emSdkVersionKey[] = "WebAssembly/emSdkVersion1";

static const FilePath timeStampFile(const FilePath &sdkRoot)
{
    return sdkRoot / Constants::WEBASSEMBLY_EMSDK_CONFIG_FILE;
}

static QString emSdkEnvOutput(const FilePath &sdkRoot)
{
    const FilePath tsFile = timeStampFile(sdkRoot); // ts == Timestamp
    if (!tsFile.exists())
        return {};
    const QDateTime ts = tsFile.lastModified();

    namespace DB = SettingsDatabase;
    if (DB::value(emSdkEnvTSKey).toDateTime() == ts
        && FilePath::fromVariant(DB::value(emSdkEnvTSFileKey)) == tsFile
        && DB::contains(emSdkEnvOutputKey)) {
        return DB::value(emSdkEnvOutputKey).toString();
    }

    const bool isWindows = sdkRoot.osType() == OsTypeWindows;
    const FilePath scriptFile = sdkRoot.pathAppended(QLatin1String("emsdk_env") +
                                                     (isWindows ? ".bat" : ".sh"));
    Process emSdkEnv;
    if (isWindows) {
        emSdkEnv.setCommand(CommandLine(scriptFile));
    } else {
        // File needs to be source'd, not executed.
        CommandLine cmd{sdkRoot.withNewPath("bash"), {"-c"}};
        cmd.addCommandLineAsSingleArg({".", {scriptFile.path()}});
        emSdkEnv.setCommand(cmd);
    }
    emSdkEnv.runBlocking();
    const QString result = emSdkEnv.allOutput();
    DB::setValue(emSdkEnvTSFileKey, tsFile.toVariant());
    DB::setValue(emSdkEnvTSKey, ts);
    DB::setValue(emSdkEnvOutputKey, result);

    return result;
}

void parseEmSdkEnvOutputAndAddToEnv(const QString &output, Environment &env)
{
    const QStringList lines = output.split('\n');

    for (const QString &line : lines) {
        const QStringList prependParts = line.trimmed().split(" += ");
        if (prependParts.count() == 2)
            env.prependOrSetPath(FilePath::fromUserInput(prependParts.last()));

        const QStringList setParts = line.trimmed().split(" = ");
        if (setParts.count() == 2) {
            if (setParts.first() != "PATH") // Path was already extended above
                env.set(setParts.first(), setParts.last());
            continue;
        }
    }

    // QTCREATORBUG-26199: Wrapper scripts (e.g. emcc.bat) of older emsdks might not find python
    const QString emsdkPython = env.value("EMSDK_PYTHON");
    if (!emsdkPython.isEmpty())
        env.appendOrSetPath(FilePath::fromUserInput(emsdkPython).parentDir());
}

void addToEnvironment(const FilePath &sdkRoot, Environment &env)
{
    if (sdkRoot.exists())
        parseEmSdkEnvOutputAndAddToEnv(emSdkEnvOutput(sdkRoot), env);
}

Result<QVersionNumber> version(const FilePath &sdkRoot)
{
    const FilePath tsFile = timeStampFile(sdkRoot); // ts == Timestamp
    if (!tsFile.exists()) {
        return ResultError(
            Tr::tr("\"%1\" config file does not exist in \"%2\".")
                .arg(Constants::WEBASSEMBLY_EMSDK_CONFIG_FILE)
                .arg(sdkRoot.toUserOutput()));
    }

    const QDateTime ts = tsFile.lastModified();

    namespace DB = SettingsDatabase;
    if (DB::value(emSdkVersionTSKey).toDateTime() == ts
        && FilePath::fromVariant(DB::value(emSdkVersionTSFileKey)) == tsFile
        && DB::contains(emSdkVersionKey)) {
        QVersionNumber version = QVersionNumber::fromString(DB::value(emSdkVersionKey).toString());
        if (version.isNull())
            return ResultError(Tr::tr("Cached emsdk version is invalid."));
        return version;
    }

    Environment env = sdkRoot.deviceEnvironment();
    addToEnvironment(sdkRoot, env);
    QLatin1String scriptFile{sdkRoot.osType() == OsType::OsTypeWindows ? "emcc.bat" : "emcc"};
    FilePath script = sdkRoot.withNewPath(scriptFile).searchInDirectories(env.path());
    if (!script.exists())
        return ResultError(Tr::tr("Failed to locate \"%1\".").arg(scriptFile));
    const CommandLine command(script, {"-dumpversion"});
    Process emcc;
    emcc.setCommand(command);
    emcc.setEnvironment(env);
    emcc.runBlocking();

    if (emcc.result() != ProcessResult::FinishedWithSuccess || emcc.exitCode() != 0) {
        return ResultError(
            Tr::tr("Failed to run %1:\n%2").arg(script.toUserOutput(), emcc.verboseExitMessage()));
    }

    const QString versionStr = emcc.cleanedStdOut();
    const QVersionNumber result = QVersionNumber::fromString(versionStr);

    if (result.isNull())
        return ResultError(
            Tr::tr("Failed to parse emsdk version from output: \"%1\".").arg(versionStr));

    DB::setValue(emSdkVersionTSFileKey, tsFile.toVariant());
    DB::setValue(emSdkVersionTSKey, ts);
    DB::setValue(emSdkVersionKey, result.toString());
    return result;
}

void clearCaches()
{
    namespace DB = SettingsDatabase;
    DB::remove(emSdkEnvTSFileKey);
    DB::remove(emSdkEnvTSKey);
    DB::remove(emSdkEnvOutputKey);
    DB::remove(emSdkVersionTSFileKey);
    DB::remove(emSdkVersionTSKey);
    DB::remove(emSdkVersionKey);
}

} // namespace WebAssembly::Internal::WebAssemblyEmSdk
