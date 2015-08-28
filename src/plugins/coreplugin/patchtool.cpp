/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "patchtool.h"
#include "messagemanager.h"
#include "icore.h"
#include <utils/synchronousprocess.h>

#include <QProcess>
#include <QDir>
#include <QApplication>

static const char settingsGroupC[] = "General";
static const char legacySettingsGroupC[] = "VCS";
static const char patchCommandKeyC[] = "PatchCommand";
static const char patchCommandDefaultC[] = "patch";

namespace Core {

static QString readLegacyCommand()
{
    QSettings *s = ICore::settings();

    s->beginGroup(QLatin1String(legacySettingsGroupC));
    const bool legacyExists = s->contains(QLatin1String(patchCommandKeyC));
    const QString legacyCommand = s->value(QLatin1String(patchCommandKeyC), QLatin1String(patchCommandDefaultC)).toString();
    if (legacyExists)
        s->remove(QLatin1String(patchCommandKeyC));
    s->endGroup();

    if (legacyExists && legacyCommand != QLatin1String(patchCommandDefaultC))
        PatchTool::setPatchCommand(legacyCommand);

    return legacyCommand;
}

QString PatchTool::patchCommand()
{
    QSettings *s = ICore::settings();

    const QString defaultCommand = readLegacyCommand(); // replace it with QLatin1String(patchCommandDefaultC) when dropping legacy stuff

    s->beginGroup(QLatin1String(settingsGroupC));
    const QString command = s->value(QLatin1String(patchCommandKeyC), defaultCommand).toString();
    s->endGroup();

    return command;
}

void PatchTool::setPatchCommand(const QString &newCommand)
{
    QSettings *s = ICore::settings();
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(patchCommandKeyC), newCommand);
    s->endGroup();
}

bool PatchTool::runPatch(const QByteArray &input, const QString &workingDirectory,
                         int strip, bool reverse)
{
    const QString patch = patchCommand();
    if (patch.isEmpty()) {
        MessageManager::write(QApplication::translate("Core::PatchTool", "There is no patch-command configured in the general \"Environment\" settings."));
        return false;
    }

    QProcess patchProcess;
    if (!workingDirectory.isEmpty())
        patchProcess.setWorkingDirectory(workingDirectory);
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
    MessageManager::write(QApplication::translate("Core::PatchTool", "Executing in %1: %2 %3").
                          arg(QDir::toNativeSeparators(workingDirectory),
                              QDir::toNativeSeparators(patch), args.join(QLatin1Char(' '))));
    patchProcess.start(patch, args);
    if (!patchProcess.waitForStarted()) {
        MessageManager::write(QApplication::translate("Core::PatchTool", "Unable to launch \"%1\": %2").arg(patch, patchProcess.errorString()));
        return false;
    }
    patchProcess.write(input);
    patchProcess.closeWriteChannel();
    QByteArray stdOut;
    QByteArray stdErr;
    if (!Utils::SynchronousProcess::readDataFromProcess(patchProcess, 30, &stdOut, &stdErr, true)) {
        Utils::SynchronousProcess::stopProcess(patchProcess);
        MessageManager::write(QApplication::translate("Core::PatchTool", "A timeout occurred running \"%1\"").arg(patch));
        return false;

    }
    if (!stdOut.isEmpty())
        MessageManager::write(QString::fromLocal8Bit(stdOut));
    if (!stdErr.isEmpty())
        MessageManager::write(QString::fromLocal8Bit(stdErr));

    if (patchProcess.exitStatus() != QProcess::NormalExit) {
        MessageManager::write(QApplication::translate("Core::PatchTool", "\"%1\" crashed.").arg(patch));
        return false;
    }
    if (patchProcess.exitCode() != 0) {
        MessageManager::write(QApplication::translate("Core::PatchTool", "\"%1\" failed (exit code %2).").arg(patch).arg(patchProcess.exitCode()));
        return false;
    }
    return true;
}


} // namespace Core
