// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "webassemblyemsdk.h"

#include "webassemblyconstants.h"

#include <coreplugin/icore.h>
#include <utils/environment.h>
#include <utils/process.h>
#include <utils/hostosinfo.h>

#include <QCache>

using namespace Utils;

namespace WebAssembly::Internal::WebAssemblyEmSdk {

using EmSdkEnvCache = QCache<FilePath, QString>;
static EmSdkEnvCache *emSdkEnvCache()
{
    static EmSdkEnvCache cache(10);
    return &cache;
}

using EmSdkVersionCache = QCache<FilePath, QVersionNumber>;
static EmSdkVersionCache *emSdkVersionCache()
{
    static EmSdkVersionCache cache(10);
    return &cache;
}

static QString emSdkEnvOutput(const FilePath &sdkRoot)
{
    const bool isWindows = sdkRoot.osType() == OsTypeWindows;
    if (!emSdkEnvCache()->contains(sdkRoot)) {
        const FilePath scriptFile = sdkRoot.pathAppended(QLatin1String("emsdk_env") +
                                        (isWindows ? ".bat" : ".sh"));
        Process emSdkEnv;
        if (isWindows) {
            emSdkEnv.setCommand(CommandLine(scriptFile));
        } else {
            // File needs to be source'd, not executed.
            emSdkEnv.setCommand({sdkRoot.withNewPath("bash"), {"-c", ". " + scriptFile.path()}});
        }
        emSdkEnv.runBlocking();
        const QString output = emSdkEnv.allOutput();
        emSdkEnvCache()->insert(sdkRoot, new QString(output));
    }
    return *emSdkEnvCache()->object(sdkRoot);
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

bool isValid(const FilePath &sdkRoot)
{
    return !version(sdkRoot).isNull();
}

void addToEnvironment(const FilePath &sdkRoot, Environment &env)
{
    if (sdkRoot.exists())
        parseEmSdkEnvOutputAndAddToEnv(emSdkEnvOutput(sdkRoot), env);
}

QVersionNumber version(const FilePath &sdkRoot)
{
    if (!sdkRoot.exists())
        return {};
    if (!emSdkVersionCache()->contains(sdkRoot)) {
        Environment env = sdkRoot.deviceEnvironment();
        addToEnvironment(sdkRoot, env);
        QLatin1String scriptFile{sdkRoot.osType() == OsType::OsTypeWindows ? "emcc.bat" : "emcc"};
        FilePath script = sdkRoot.withNewPath(scriptFile).searchInDirectories(env.path());
        const CommandLine command(script, {"-dumpversion"});
        Process emcc;
        emcc.setCommand(command);
        emcc.setEnvironment(env);
        emcc.runBlocking();
        const QString version = emcc.cleanedStdOut();
        emSdkVersionCache()->insert(sdkRoot,
                                    new QVersionNumber(QVersionNumber::fromString(version)));
    }
    return *emSdkVersionCache()->object(sdkRoot);
}

void clearCaches()
{
    emSdkEnvCache()->clear();
    emSdkVersionCache()->clear();
}

} // namespace WebAssembly::Internal::WebAssemblyEmSdk
