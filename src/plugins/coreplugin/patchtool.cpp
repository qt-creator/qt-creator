// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "patchtool.h"

#include "coreplugintr.h"
#include "icore.h"
#include "messagemanager.h"
#include "systemsettings.h"

#include <utils/environment.h>
#include <utils/qtcprocess.h>

#include <QMessageBox>

using namespace Utils;
using namespace Core::Internal;

namespace Core {

FilePath PatchTool::patchCommand()
{
    FilePath command = systemSettings().patchCommand();

    if (HostOsInfo::isWindowsHost() && systemSettings().patchCommand.isDefaultValue()) {
        const QSettings settings(R"(HKEY_LOCAL_MACHINE\SOFTWARE\GitForWindows)",
                                 QSettings::NativeFormat);
        const FilePath gitInstall = FilePath::fromUserInput(settings.value("InstallPath").toString());
        if (gitInstall.exists())
            command = command.searchInPath({gitInstall.pathAppended("usr/bin")},
                                           FilePath::PrependToPath);
    }

    return command;
}

bool PatchTool::confirmPatching(QWidget *parent, PatchAction patchAction, bool isModified)
{
    const QString title = patchAction == PatchAction::Apply ? Tr::tr("Apply Chunk")
                                                            : Tr::tr("Revert Chunk");
    QString question = patchAction == PatchAction::Apply
            ? Tr::tr("Would you like to apply the chunk?")
            : Tr::tr("Would you like to revert the chunk?");
    if (isModified)
        question += "\n" + Tr::tr("Note: The file will be saved before this operation.");
    return QMessageBox::question(parent, title, question, QMessageBox::Yes | QMessageBox::No)
            == QMessageBox::Yes;
}

static bool runPatchHelper(const QByteArray &input, const FilePath &workingDirectory,
                           int strip, PatchAction patchAction, bool withCrlf,
                           const std::function<void(const QString &)> &logger)
{
    // When a logger is given, capture the messages instead of disrupting the
    // message pane; otherwise keep the previous pane behavior exactly.
    const auto report = [&logger](const QString &message, bool disrupting) {
        if (logger)
            logger(message);
        else if (disrupting)
            MessageManager::writeDisrupting(message);
        else
            MessageManager::writeFlashing(message);
    };

    const FilePath patch = PatchTool::patchCommand();
    if (patch.isEmpty()) {
        report(Tr::tr("There is no patch-command configured in "
                      "the general \"Environment\" settings."), true);
        return false;
    }

    if (!patch.exists() && !patch.searchInPath().exists()) {
        report(Tr::tr("The patch-command configured in the general "
                      "\"Environment\" settings does not exist."), true);
        return false;
    }

    Process patchProcess;
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
    if (patchAction == PatchAction::Revert)
        args << "-R";
    args << "--binary";
    report(Tr::tr("Running in \"%1\": %2 %3.")
               .arg(workingDirectory.toUserOutput(), patch.toUserOutput(), args.join(' ')),
           true);
    patchProcess.setCommand({patch, args});
    patchProcess.setWriteData(input);
    patchProcess.start();
    if (!patchProcess.waitForStarted()) {
        report(Tr::tr("Unable to launch \"%1\": %2")
                   .arg(patch.toUserOutput(), patchProcess.errorString()), false);
        return false;
    }

    QByteArray stdOut;
    QByteArray stdErr;
    if (!patchProcess.readDataFromProcess(&stdOut, &stdErr)) {
        patchProcess.stop();
        patchProcess.waitForFinished();
        report(Tr::tr("A timeout occurred running \"%1\".").arg(patch.toUserOutput()), false);
        return false;

    }
    if (!stdOut.isEmpty()) {
        if (stdOut.contains("(different line endings)") && !withCrlf) {
            QByteArray crlfInput = input;
            crlfInput.replace('\n', "\r\n");
            return runPatchHelper(crlfInput, workingDirectory, strip, patchAction, true, logger);
        } else {
            report(QString::fromLocal8Bit(stdOut), false);
        }
    }
    if (!stdErr.isEmpty())
        report(QString::fromLocal8Bit(stdErr), false);

    if (patchProcess.exitStatus() != ProcessExitStatus::NormalExit) {
        report(Tr::tr("\"%1\" crashed.").arg(patch.toUserOutput()), false);
        return false;
    }
    if (patchProcess.exitCode() != 0) {
        report(Tr::tr("\"%1\" failed (exit code %2).")
                   .arg(patch.toUserOutput()).arg(patchProcess.exitCode()), false);
        return false;
    }
    return true;
}

bool PatchTool::runPatch(const QByteArray &input, const FilePath &workingDirectory,
                         int strip, PatchAction patchAction,
                         const std::function<void(const QString &)> &logger)
{
    return runPatchHelper(input, workingDirectory, strip, patchAction, false, logger);
}

} // namespace Core
