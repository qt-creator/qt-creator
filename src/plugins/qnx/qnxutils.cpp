// Copyright (C) 2016 BlackBerry Limited. All rights reserved
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qnxutils.h"

#include <utils/algorithm.h>
#include <utils/commandline.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>
#include <utils/temporaryfile.h>

#include <QDebug>
#include <QApplication>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qnx::Internal {

QnxTarget::QnxTarget(const Utils::FilePath &path, const ProjectExplorer::Abi &abi) :
    m_path(path), m_abi(abi)
{}

QString QnxTarget::shortDescription() const
{
    return QnxUtils::cpuDirShortDescription(cpuDir());
}

QString QnxUtils::cpuDirFromAbi(const Abi &abi)
{
    if (abi.os() != Abi::OS::QnxOS)
        return QString();
    if (abi.architecture() == Abi::Architecture::ArmArchitecture)
        return QString::fromLatin1(abi.wordWidth() == 32 ? "armle-v7" : "aarch64le");
    if (abi.architecture() == Abi::Architecture::X86Architecture)
        return QString::fromLatin1(abi.wordWidth() == 32 ? "x86" : "x86_64");
    return QString();
}

QString QnxUtils::cpuDirShortDescription(const QString &cpuDir)
{
    if (cpuDir == "armle-v7")
        return QLatin1String("32-bit ARM");

    if (cpuDir == "aarch64le")
        return QLatin1String("64-bit ARM");

    if (cpuDir == "x86")
        return QLatin1String("32-bit x86");

    if (cpuDir == "x86_64")
        return QLatin1String("64-bit x86");

    return cpuDir;
}

EnvironmentItems QnxUtils::qnxEnvironmentFromEnvFile(const FilePath &filePath)
{
    EnvironmentItems items;

    if (!filePath.exists())
        return items;

    static const QByteArray envVars[] = {
        "QNX_TARGET", "QNX_HOST", "QNX_CONFIGURATION", "QNX_CONFIGURATION_EXCLUSIVE",
        "MAKEFLAGS", "LD_LIBRARY_PATH", "PATH", "QDE", "CPUVARDIR", "PYTHONPATH"
    };

    Process process;

    const bool isWindows = filePath.osType() == Utils::OsTypeWindows;
    if (isWindows) {
        // cmd.exe needs a wrapper file.
        const Result<FilePath> tmpPath = filePath.tmpDir();
        if (!tmpPath)
            return {};

        const FilePath pattern = *tmpPath / "sdp-env-eval-XXXXXX.bat";
        const Result<FilePath> tmpFile = pattern.createTempFile();
        if (!tmpFile)
            return {};

        const QString quotedPath = ProcessArgs::quoteArg(filePath.path(), OsTypeWindows);
        QByteArray content = "@echo off\r\ncall " + quotedPath.toUtf8() + "\r\n";
        for (const QByteArray &envVar : envVars)
            content += "echo " + envVar + "=%" + envVar + "%\r\n";

        tmpFile->writeFileContents(content);
        process.setCommand({filePath.findCmdExe(), {"/C", tmpFile->path()}});
    } else {
        // This has to use bash as qnxsdp-env.sh requires it. Pipe the script in
        // via stdin instead of using a temporary file.
        const QString quotedPath = ProcessArgs::quoteArg(filePath.path(), OsTypeLinux);
        QByteArray content = "source " + quotedPath.toUtf8() + "\n";
        for (const QByteArray &envVar : envVars)
            content += "echo " + envVar + "=$" + envVar + "\n";
        process.setCommand({filePath.withNewPath("/bin/bash"), {}});
        process.setWriteData(content);
    }

    process.start();

    // waiting for finish
    QApplication::setOverrideCursor(Qt::BusyCursor);
    using namespace std::chrono_literals;
    bool waitResult = process.waitForFinished(10s);
    QApplication::restoreOverrideCursor();
    if (!waitResult)
        return items;

    if (process.result() != ProcessResult::FinishedWithSuccess)
        return items;

    // parsing process output
    const QString output = process.cleanedStdOut();
    for (const QString &line : output.split('\n')) {
        int equalIndex = line.indexOf('=');
        if (equalIndex < 0)
            continue;
        QString var = line.left(equalIndex);
        QString value = line.mid(equalIndex + 1);
        items.append(EnvironmentItem(var, value));
    }

    return items;
}

EnvironmentItems QnxUtils::qnxEnvironment(const FilePath &sdpPath)
{
    FilePaths entries;
    if (sdpPath.osType() == OsTypeWindows)
        entries = sdpPath.dirEntries({{"*-env.bat"}});
    else
        entries = sdpPath.dirEntries({{"*-env.sh"}});

    if (entries.isEmpty())
        return {};

    return qnxEnvironmentFromEnvFile(entries.first());
}

QList<QnxTarget> QnxUtils::findTargets(const FilePath &basePath)
{
    QList<QnxTarget> result;

    basePath.iterateDirectory(
            [&result](const FilePath &filePath) {
                const FilePath libc = filePath / "lib/libc.so";
                if (libc.exists()) {
                    const Abis abis = Abi::abisOfBinary(libc);
                    if (abis.isEmpty()) {
                        qWarning() << libc << "has no ABIs ... discarded";
                        return IterationPolicy::Continue;
                    }

                    if (abis.count() > 1)
                        qWarning() << libc << "has more than one ABI ... processing all";

                    for (const Abi &abi : abis)
                        result.append(QnxTarget(filePath, QnxUtils::convertAbi(abi)));
                }
                return IterationPolicy::Continue;
            },
            {{}, DirFilterFlag::Dirs});

    return result;
}

Abi QnxUtils::convertAbi(const Abi &abi)
{
    if (abi.os() == Abi::LinuxOS && abi.osFlavor() == Abi::GenericFlavor) {
        return Abi(abi.architecture(),
                   Abi::QnxOS,
                   Abi::GenericFlavor,
                   abi.binaryFormat(),
                   abi.wordWidth());
    }
    return abi;
}

Abis QnxUtils::convertAbis(const Abis &abis)
{
    return Utils::transform(abis, &QnxUtils::convertAbi);
}

} // Qnx::Internal
