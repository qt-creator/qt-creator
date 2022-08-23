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

#include "publickeydeploymentdialog.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/sshparameters.h>
#include <projectexplorer/devicesupport/sshsettings.h>
#include <utils/fileutils.h>
#include <utils/qtcprocess.h>
#include <utils/theme/theme.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class PublicKeyDeploymentDialogPrivate
{
public:
    QtcProcess m_process;
    bool m_done;
};
} // namespace Internal;

using namespace Internal;

PublicKeyDeploymentDialog *PublicKeyDeploymentDialog::createDialog(
        const IDevice::ConstPtr &deviceConfig, QWidget *parent)
{
    const FilePath dir = deviceConfig->sshParameters().privateKeyFile.parentDir();
    const FilePath publicKeyFileName = FileUtils::getOpenFilePath(nullptr,
        tr("Choose Public Key File"), dir,
        tr("Public Key Files (*.pub);;All Files (*)"));
    if (publicKeyFileName.isEmpty())
        return nullptr;
    return new PublicKeyDeploymentDialog(deviceConfig, publicKeyFileName, parent);
}

PublicKeyDeploymentDialog::PublicKeyDeploymentDialog(const IDevice::ConstPtr &deviceConfig,
        const FilePath &publicKeyFileName, QWidget *parent)
    : QProgressDialog(parent), d(new PublicKeyDeploymentDialogPrivate)
{
    setAutoReset(false);
    setAutoClose(false);
    setMinimumDuration(0);
    setMaximum(1);

    d->m_done = false;
    setLabelText(tr("Deploying..."));
    setValue(0);
    connect(this, &PublicKeyDeploymentDialog::canceled, this,
            [this] { d->m_done ? accept() : reject(); });
    connect(&d->m_process, &QtcProcess::done, this, [this] {
        const bool succeeded = d->m_process.result() == ProcessResult::FinishedWithSuccess;
        QString finalMessage;
        if (!succeeded) {
            QString errorMessage = d->m_process.errorString();
            if (errorMessage.isEmpty())
                errorMessage = d->m_process.cleanedStdErr();
            if (errorMessage.endsWith('\n'))
                errorMessage.chop(1);
            finalMessage = tr("Key deployment failed.");
            if (!errorMessage.isEmpty())
                finalMessage += '\n' + errorMessage;
        }
        handleDeploymentDone(succeeded, finalMessage);
    });

    FileReader reader;
    if (!reader.fetch(publicKeyFileName)) {
        handleDeploymentDone(false, tr("Public key error: %1").arg(reader.errorString()));
        return;
    }

    const QString command = "test -d .ssh || mkdir -p ~/.ssh && chmod 0700 .ssh && echo '"
            + QString::fromLocal8Bit(reader.data())
            + "' >> .ssh/authorized_keys && chmod 0600 .ssh/authorized_keys";

    const SshParameters params = deviceConfig->sshParameters();
    const QString hostKeyCheckingString = params.hostKeyCheckingMode == SshHostKeyCheckingStrict
            ? QLatin1String("yes") : QLatin1String("no");
    const bool isWindows = HostOsInfo::isWindowsHost()
            && SshSettings::sshFilePath().toString().toLower().contains("/system32/");
    const bool useTimeout = (params.timeout != 0) && !isWindows;

    Utils::CommandLine cmd{SshSettings::sshFilePath()};
    QStringList args{"-q",
                     "-o", "StrictHostKeyChecking=" + hostKeyCheckingString,
                     "-o", "Port=" + QString::number(params.port())};
    if (!params.userName().isEmpty())
        args << "-o" << "User=" + params.userName();
    args << "-o" << "BatchMode=no";
    if (useTimeout)
        args << "-o" << "ConnectTimeout=" + QString::number(params.timeout);
    args << params.host();
    cmd.addArgs(args);

    QString execCommandString("exec /bin/sh -c");
    ProcessArgs::addArg(&execCommandString, command, OsType::OsTypeLinux);
    cmd.addArg(execCommandString);

    d->m_process.setCommand(cmd);
    SshParameters::setupSshEnvironment(&d->m_process);
    d->m_process.start();
}

PublicKeyDeploymentDialog::~PublicKeyDeploymentDialog()
{
    delete d;
}

void PublicKeyDeploymentDialog::handleDeploymentDone(bool succeeded, const QString &errorMessage)
{
    QString buttonText = succeeded ? tr("Deployment finished successfully.") : errorMessage;
    const QString textColor = creatorTheme()->color(
                succeeded ? Theme::TextColorNormal : Theme::TextColorError).name();
    setLabelText(QString::fromLatin1("<font color=\"%1\">%2</font>")
            .arg(textColor, buttonText.replace("\n", "<br/>")));
    setCancelButtonText(tr("Close"));

    if (!succeeded)
        return;

    setValue(1);
    d->m_done = true;
}

} // namespace RemoteLinux
