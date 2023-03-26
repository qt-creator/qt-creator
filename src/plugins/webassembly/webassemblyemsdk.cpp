// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "webassemblyemsdk.h"

#include "webassemblyconstants.h"

#include <coreplugin/icore.h>
#include <utils/environment.h>
#include <utils/qtcprocess.h>
#include <utils/hostosinfo.h>

#include <QCache>
#include <QSettings>

using namespace Utils;

namespace WebAssembly::Internal::WebAssemblyEmSdk {

using EmSdkEnvCache = QCache<QString, QString>;
Q_GLOBAL_STATIC_WITH_ARGS(EmSdkEnvCache, emSdkEnvCache, (10))
using EmSdkVersionCache = QCache<QString, QVersionNumber>;
Q_GLOBAL_STATIC_WITH_ARGS(EmSdkVersionCache, emSdkVersionCache, (10))

static QString emSdkEnvOutput(const FilePath &sdkRoot)
{
    const QString cacheKey = sdkRoot.toString();
    const bool isWindows = sdkRoot.osType() == OsTypeWindows;
    if (!emSdkEnvCache()->contains(cacheKey)) {
        const FilePath scriptFile = sdkRoot.pathAppended(QLatin1String("emsdk_env") +
                                        (isWindows ? ".bat" : ".sh"));
        QtcProcess emSdkEnv;
        if (isWindows) {
            emSdkEnv.setCommand(CommandLine(scriptFile));
        } else {
            // File needs to be source'd, not executed.
            emSdkEnv.setCommand({sdkRoot.withNewPath("bash"), {"-c", ". " + scriptFile.path()}});
        }
        emSdkEnv.runBlocking();
        const QString output = emSdkEnv.allOutput();
        emSdkEnvCache()->insert(cacheKey, new QString(output));
    }
    return *emSdkEnvCache()->object(cacheKey);
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
    const QString cacheKey = sdkRoot.toString();
    if (!emSdkVersionCache()->contains(cacheKey)) {
        Environment env = sdkRoot.deviceEnvironment();
        addToEnvironment(sdkRoot, env);
        QLatin1String scriptFile{sdkRoot.osType() == OsType::OsTypeWindows ? "emcc.bat" : "emcc"};
        FilePath script = sdkRoot.withNewPath(scriptFile).searchInDirectories(env.path());
        const CommandLine command(script, {"-dumpversion"});
        QtcProcess emcc;
        emcc.setCommand(command);
        emcc.setEnvironment(env);
        emcc.runBlocking();
        const QString version = emcc.cleanedStdOut();
        emSdkVersionCache()->insert(cacheKey,
                                    new QVersionNumber(QVersionNumber::fromString(version)));
    }
    return *emSdkVersionCache()->object(cacheKey);
}

void registerEmSdk(const FilePath &sdkRoot)
{
    QSettings *s = Core::ICore::settings();
    s->setValue(QLatin1String(Constants::SETTINGS_GROUP) + '/'
                + QLatin1String(Constants::SETTINGS_KEY_EMSDK), sdkRoot.toString());
}

FilePath registeredEmSdk()
{
    QSettings *s = Core::ICore::settings();
    const QString path = s->value(QLatin1String(Constants::SETTINGS_GROUP) + '/'
                     + QLatin1String(Constants::SETTINGS_KEY_EMSDK)).toString();
    return FilePath::fromUserInput(path);
}

void clearCaches()
{
    emSdkEnvCache()->clear();
    emSdkVersionCache()->clear();
}

} // namespace WebAssembly::Internal::WebAssemblyEmSdk
