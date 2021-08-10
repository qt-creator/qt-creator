/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "patchtool.h"
#include "messagemanager.h"
#include "icore.h"

#include <utils/environment.h>
#include <utils/qtcprocess.h>

#include <QDir>
#include <QApplication>

static const char settingsGroupC[] = "General";
static const char patchCommandKeyC[] = "PatchCommand";
static const char patchCommandDefaultC[] = "patch";

using namespace Utils;

namespace Core {

QString PatchTool::patchCommand()
{
    QSettings *s = ICore::settings();

    s->beginGroup(settingsGroupC);
    const QString command = s->value(patchCommandKeyC, patchCommandDefaultC).toString();
    s->endGroup();

    return command;
}

void PatchTool::setPatchCommand(const QString &newCommand)
{
    Utils::QtcSettings *s = ICore::settings();
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValueWithDefault(QLatin1String(patchCommandKeyC), newCommand, QString(patchCommandKeyC));
    s->endGroup();
}

static bool runPatchHelper(const QByteArray &input, const QString &workingDirectory,
                           int strip, bool reverse, bool withCrlf)
{
    const QString patch = PatchTool::patchCommand();
    if (patch.isEmpty()) {
        MessageManager::writeDisrupting(QApplication::translate(
            "Core::PatchTool",
            "There is no patch-command configured in the general \"Environment\" settings."));
        return false;
    }

    if (!Utils::FilePath::fromString(patch).exists()
            && !Utils::Environment::systemEnvironment().searchInPath(patch).exists()) {
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
    if (patch.endsWith(QLatin1String("git"), Qt::CaseInsensitive)
        || patch.endsWith(QLatin1String("git.exe"), Qt::CaseInsensitive)) {
        args << QLatin1String("apply");
    }
    if (strip >= 0)
        args << (QLatin1String("-p") + QString::number(strip));
    if (reverse)
        args << QLatin1String("-R");
    if (withCrlf)
        args << QLatin1String("--binary");
    MessageManager::writeDisrupting(
        QApplication::translate("Core::PatchTool", "Running in %1: %2 %3")
            .arg(QDir::toNativeSeparators(workingDirectory),
                 QDir::toNativeSeparators(patch),
                 args.join(QLatin1Char(' '))));
    patchProcess.setCommand({FilePath::fromString(patch), args});
    patchProcess.setWriteData(input);
    patchProcess.start();
    if (!patchProcess.waitForStarted()) {
        MessageManager::writeFlashing(
            QApplication::translate("Core::PatchTool", "Unable to launch \"%1\": %2")
                .arg(patch, patchProcess.errorString()));
        return false;
    }

    QByteArray stdOut;
    QByteArray stdErr;
    if (!patchProcess.readDataFromProcess(30, &stdOut, &stdErr, true)) {
        patchProcess.stopProcess();
        MessageManager::writeFlashing(
            QApplication::translate("Core::PatchTool", "A timeout occurred running \"%1\"")
                .arg(patch));
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
            QApplication::translate("Core::PatchTool", "\"%1\" crashed.").arg(patch));
        return false;
    }
    if (patchProcess.exitCode() != 0) {
        MessageManager::writeFlashing(
            QApplication::translate("Core::PatchTool", "\"%1\" failed (exit code %2).")
                .arg(patch)
                .arg(patchProcess.exitCode()));
        return false;
    }
    return true;
}

bool PatchTool::runPatch(const QByteArray &input, const QString &workingDirectory,
                         int strip, bool reverse)
{
    return runPatchHelper(input, workingDirectory, strip, reverse, false);
}

} // namespace Core
