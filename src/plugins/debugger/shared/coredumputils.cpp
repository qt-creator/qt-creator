// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "coredumputils.h"
#include <utils/qtcprocess.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>

namespace Debugger::Internal {

const char COREDUMPCTL[] = "coredumpctl";
const char UNZSTD[] = "unzstd";

static LastCore findLastCore()
{
    static const auto coredumpctlBinary = QStandardPaths::findExecutable(COREDUMPCTL);
    if (coredumpctlBinary.isEmpty()) {
        qWarning("%s was not found", COREDUMPCTL);
        return {};
    }
    Utils::Process coredumpctl;
    coredumpctl.setCommand({Utils::FilePath::fromString(coredumpctlBinary), {"info"}});
    coredumpctl.runBlocking();
    if (coredumpctl.exitStatus() != QProcess::NormalExit
        || coredumpctl.exitCode() != 0) {
        qWarning("%s failed: %s", COREDUMPCTL, qPrintable(coredumpctl.errorString()));
        return {};
    }

    LastCore result;
    const QString output = coredumpctl.readAllStandardOutput();
    const auto lines = QStringView{output}.split('\n');
    for (const QStringView &line : lines) {
        if (line.startsWith(u"    Executable: ")) {
            const QStringView binary = line.sliced(line.indexOf(':') + 1).trimmed();
            result.binary = Utils::FilePath::fromString(binary.toString());
        } else if (line.startsWith(u"       Storage: ") && line.endsWith(u" (present)")) {
            auto pos = line.indexOf(':') + 1;
            const auto len = line.size() - 10 - pos;
            const QStringView coreFile = line.sliced(pos, len).trimmed();
            result.coreFile = Utils::FilePath::fromString(coreFile.toString());
        }
        if (result)
            break;
    }
    return result;
}

LastCore getLastCore()
{
    auto lastCore = findLastCore();
    if (!lastCore || !lastCore.coreFile.endsWith(".zst")) {
        qWarning("No core was found");
        return {};
    }

    // Copy core to /tmp and uncompress (unless it already exists
    // on 2nd invocation).
    QString tmpCorePath = QDir::tempPath() + '/' + lastCore.coreFile.fileName();
    const auto tmpCore = Utils::FilePath::fromString(tmpCorePath);
    if (!tmpCore.exists() && !QFile::copy(lastCore.coreFile.toString(), tmpCorePath))
        return {};

    const QString uncompressedCorePath = tmpCorePath.sliced(0, tmpCorePath.size() - 4);
    const auto uncompressedCore = Utils::FilePath::fromString(uncompressedCorePath);

    if (!uncompressedCore.exists()) {
        Utils::Process  uncompress;
        uncompress.setCommand({Utils::FilePath::fromString(UNZSTD),
                              {"-f", tmpCorePath}});
        uncompress.runBlocking(std::chrono::seconds(20));
        if (uncompress.exitStatus() != QProcess::NormalExit
            || uncompress.exitCode() != 0) {
            qWarning("%s failed: %s", UNZSTD, qPrintable(uncompress.errorString()));
            return {};
        }
    }

    lastCore.coreFile = uncompressedCore;
    return lastCore;
}

} // namespace Debugger::Internal {
