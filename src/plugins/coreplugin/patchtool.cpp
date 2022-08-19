// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "patchtool.h"
#include "messagemanager.h"
#include "icore.h"

#include <utils/environment.h>
#include <utils/qtcprocess.h>

#include <QApplication>

using namespace Utils;

namespace Core {

const char settingsGroupC[] = "General";
const char patchCommandKeyC[] = "PatchCommand";
const char patchCommandDefaultC[] = "patch";

FilePath PatchTool::patchCommand()
{
    QSettings *s = ICore::settings();

    s->beginGroup(settingsGroupC);
    const FilePath command = FilePath::fromVariant(s->value(patchCommandKeyC, patchCommandDefaultC));
    s->endGroup();

    return command;
}

void PatchTool::setPatchCommand(const FilePath &newCommand)
{
    Utils::QtcSettings *s = ICore::settings();
    s->beginGroup(settingsGroupC);
    s->setValueWithDefault(patchCommandKeyC, newCommand.toVariant(), QVariant(QString(patchCommandDefaultC)));
    s->endGroup();
}

static bool runPatchHelper(const QByteArray &input, const FilePath &workingDirectory,
                           int strip, bool reverse, bool withCrlf)
{
    const FilePath patch = PatchTool::patchCommand();
    if (patch.isEmpty()) {
        MessageManager::writeDisrupting(QApplication::translate(
            "Core::PatchTool",
            "There is no patch-command configured in the general \"Environment\" settings."));
        return false;
    }

    if (!patch.exists() && !patch.searchInPath().exists()) {
        MessageManager::writeDisrupting(
            QApplication::translate("Core::PatchTool",
                                    "The patch-command configured in the general \"Environment\" "
                                    "settings does not exist."));
        return false;
    }

    QtcProcess patchProcess;
    if (!workingDirectory.isEmpty())
        patchProcess.setWorkingDirectory(workingDirectory);
    Environment env = Environment::systemEnvironment();
    env.setupEnglishOutput();
    patchProcess.setEnvironment(env);
    QStringList args;
    // Add argument 'apply' when git is used as patch command since git 2.5/Windows
    // no longer ships patch.exe.
    if (patch.endsWith("git") || patch.endsWith("git.exe"))
        args << "apply";

    if (strip >= 0)
        args << ("-p" + QString::number(strip));
    if (reverse)
        args << "-R";
    if (withCrlf)
        args << "--binary";
    MessageManager::writeDisrupting(
        QApplication::translate("Core::PatchTool", "Running in %1: %2 %3")
            .arg(workingDirectory.toUserOutput(), patch.toUserOutput(), args.join(' ')));
    patchProcess.setCommand({patch, args});
    patchProcess.setWriteData(input);
    patchProcess.start();
    if (!patchProcess.waitForStarted()) {
        MessageManager::writeFlashing(
            QApplication::translate("Core::PatchTool", "Unable to launch \"%1\": %2")
                .arg(patch.toUserOutput(), patchProcess.errorString()));
        return false;
    }

    QByteArray stdOut;
    QByteArray stdErr;
    if (!patchProcess.readDataFromProcess(&stdOut, &stdErr)) {
        patchProcess.stop();
        patchProcess.waitForFinished();
        MessageManager::writeFlashing(
            QApplication::translate("Core::PatchTool", "A timeout occurred running \"%1\"")
                .arg(patch.toUserOutput()));
        return false;

    }
    if (!stdOut.isEmpty()) {
        if (stdOut.contains("(different line endings)") && !withCrlf) {
            QByteArray crlfInput = input;
            crlfInput.replace('\n', "\r\n");
            return runPatchHelper(crlfInput, workingDirectory, strip, reverse, true);
        } else {
            MessageManager::writeFlashing(QString::fromLocal8Bit(stdOut));
        }
    }
    if (!stdErr.isEmpty())
        MessageManager::writeFlashing(QString::fromLocal8Bit(stdErr));

    if (patchProcess.exitStatus() != QProcess::NormalExit) {
        MessageManager::writeFlashing(
            QApplication::translate("Core::PatchTool", "\"%1\" crashed.").arg(patch.toUserOutput()));
        return false;
    }
    if (patchProcess.exitCode() != 0) {
        MessageManager::writeFlashing(
            QApplication::translate("Core::PatchTool", "\"%1\" failed (exit code %2).")
                .arg(patch.toUserOutput())
                .arg(patchProcess.exitCode()));
        return false;
    }
    return true;
}

bool PatchTool::runPatch(const QByteArray &input, const FilePath &workingDirectory,
                         int strip, bool reverse)
{
    return runPatchHelper(input, workingDirectory, strip, reverse, false);
}

} // namespace Core
