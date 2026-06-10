// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "coredumputils.h"

#include "../debuggertr.h"

#include <utils/messagebox.h>
#include <utils/qtcprocess.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>

using namespace QtTaskTree;
using namespace Utils;

namespace Debugger::Internal {

const char COREDUMPCTL[] = "coredumpctl";
const char UNZSTD[] = "unzstd";

Group lastCoreRecipe(const Storage<LastCore> &storage)
{
    const auto onFindCoreSetup = [](Process &process) {
        static const auto coredumpctlBinary = QStandardPaths::findExecutable(COREDUMPCTL);
        if (coredumpctlBinary.isEmpty()) {
            qWarning("%s was not found", COREDUMPCTL);
            return SetupResult::StopWithError;
        }
        process.setCommand({Utils::FilePath::fromString(coredumpctlBinary), {"info"}});
        return SetupResult::Continue;
    };
    const auto onFindCoreDone = [storage](const Process &process, DoneWith result) {
        if (result != DoneWith::Success) {
            qWarning("%s failed: %s", COREDUMPCTL, qPrintable(process.errorString()));
            return DoneResult::Error;
        }

        const QString output = process.stdOut();
        const auto lines = QStringView{output}.split('\n');
        for (const QStringView &line : lines) {
            if (line.startsWith(u"    Executable: ")) {
                const QStringView binary = line.sliced(line.indexOf(':') + 1).trimmed();
                storage->binary = Utils::FilePath::fromString(binary.toString());
            } else if (line.startsWith(u"       Storage: ") && line.endsWith(u" (present)")) {
                auto pos = line.indexOf(':') + 1;
                const auto len = line.size() - 10 - pos;
                const QStringView coreFile = line.sliced(pos, len).trimmed();
                storage->coreFile = Utils::FilePath::fromString(coreFile.toString());
            }
            if (bool(*storage) && storage->coreFile.endsWith(".zst"))
                return DoneResult::Success;
        }
        qWarning("No core was found");
        return DoneResult::Error;
    };

    const auto onGetCoreSetup = [storage](Process &process) {
        // Copy core to /tmp and uncompress (unless it already exists on 2nd invocation).
        const QString tmpCorePath = QDir::tempPath() + '/' + storage->coreFile.fileName();
        const auto tmpCore = Utils::FilePath::fromString(tmpCorePath);
        if (!tmpCore.exists() && !QFile::copy(storage->coreFile.toUrlishString(), tmpCorePath))
            return SetupResult::StopWithError;

        const QString uncompressedCorePath = tmpCorePath.sliced(0, tmpCorePath.size() - 4);
        const auto uncompressedCore = FilePath::fromString(uncompressedCorePath);
        storage->coreFile = uncompressedCore;
        if (uncompressedCore.exists())
            return SetupResult::StopWithSuccess;

        process.setCommand({FilePath::fromString(UNZSTD), {"-f", tmpCorePath}});
        return SetupResult::Continue;
    };
    const auto onGetCoreDone = [](const Process &process) {
        qWarning("%s failed: %s", UNZSTD, qPrintable(process.errorString()));
    };

    const auto onError = [] {
        Utils::AsynchronousMessageBox::warning(Tr::tr("Warning"),
            Tr::tr("coredumpctl did not find any cores created by systemd-coredump."));
    };

    return {
        ProcessTask(onFindCoreSetup, onFindCoreDone),
        ProcessTask(onGetCoreSetup, onGetCoreDone, CallDoneFlag::OnError),
        onGroupDone(onError, CallDoneFlag::OnError)
    };
}

} // namespace Debugger::Internal {
