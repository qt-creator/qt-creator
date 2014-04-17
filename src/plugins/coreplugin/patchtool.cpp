/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
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
    QSettings *s = Core::ICore::settings();

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
    QSettings *s = Core::ICore::settings();

    const QString defaultCommand = readLegacyCommand(); // replace it with QLatin1String(patchCommandDefaultC) when dropping legacy stuff

    s->beginGroup(QLatin1String(settingsGroupC));
    const QString command = s->value(QLatin1String(patchCommandKeyC), defaultCommand).toString();
    s->endGroup();

    return command;
}

void PatchTool::setPatchCommand(const QString &newCommand)
{
    QSettings *s = Core::ICore::settings();
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
    QStringList args(QLatin1String("-p") + QString::number(strip));
    if (reverse)
        args << QLatin1String("-R");
    MessageManager::write(QApplication::translate("Core::PatchTool", "Executing in %1: %2 %3").
                          arg(QDir::toNativeSeparators(workingDirectory),
                              QDir::toNativeSeparators(patch), args.join(QLatin1String(" "))));
    patchProcess.start(patch, args);
    if (!patchProcess.waitForStarted()) {
        MessageManager::write(QApplication::translate("Core::PatchTool", "Unable to launch \"%1\": %2").arg(patch, patchProcess.errorString()));
        return false;
    }
    patchProcess.write(input);
    patchProcess.closeWriteChannel();
    QByteArray stdOut;
    QByteArray stdErr;
    if (!Utils::SynchronousProcess::readDataFromProcess(patchProcess, 30000, &stdOut, &stdErr, true)) {
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
