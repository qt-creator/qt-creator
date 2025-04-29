// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "publickeydeploymentdialog.h"

#include "remotelinuxtr.h"

#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/sshparameters.h>
#include <projectexplorer/devicesupport/sshsettings.h>

#include <utils/filepath.h>
#include <utils/fileutils.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>

#include <QProgressDialog>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux::Internal {

class PublicKeyDeploymentDialog : public QProgressDialog
{
public:
    PublicKeyDeploymentDialog(const DeviceConstRef &device, const FilePath &publicKeyFileName);

private:
    void handleDeploymentDone(const Result<> &result);

    Process m_process;
    bool m_done = false;
};

PublicKeyDeploymentDialog::PublicKeyDeploymentDialog(const DeviceConstRef &device,
                                                     const FilePath &publicKeyFileName)
    : QProgressDialog(Core::ICore::dialogParent())
{
    setAutoReset(false);
    setAutoClose(false);
    setMinimumDuration(0);
    setMaximum(1);

    setLabelText(Tr::tr("Deploying..."));
    setValue(0);
    connect(this, &PublicKeyDeploymentDialog::canceled, this,
            [this] { m_done ? accept() : reject(); });
    connect(&m_process, &Process::done, this, [this] {
        const bool succeeded = m_process.result() == ProcessResult::FinishedWithSuccess;
        Result<> result = ResultOk;
        if (!succeeded) {
            const QString errorString = m_process.errorString();
            const QString errorMessage = m_process.exitMessage(
                Process::FailureMessageFormat::WithStdErr);
            result = ResultError(Utils::joinStrings({Tr::tr("Key deployment failed."),
                                               Utils::trimBack(errorMessage, '\n')}, '\n'));
        }
        handleDeploymentDone(result);
    });

    const Result<QByteArray> publicKey = publicKeyFileName.fileContents();
    if (!publicKey) {
        handleDeploymentDone(ResultError(Tr::tr("Public key error: %1").arg(publicKey.error())));
        return;
    }

    const QString command = "test -d .ssh || mkdir -p ~/.ssh && chmod 0700 .ssh && echo '"
            + QString::fromLocal8Bit(publicKey.value())
            + "' >> .ssh/authorized_keys && chmod 0600 .ssh/authorized_keys";

    const SshParameters params = device.sshParameters();
    const QString hostKeyCheckingString = params.hostKeyCheckingMode() == SshHostKeyCheckingStrict
            ? QLatin1String("yes") : QLatin1String("no");
    const bool isWindows = HostOsInfo::isWindowsHost()
            && SshSettings::sshFilePath().toUrlishString().toLower().contains("/system32/");
    const bool useTimeout = (params.timeout() != 0) && !isWindows;

    CommandLine cmd{SshSettings::sshFilePath()};
    QStringList args{"-q",
                     "-o", "StrictHostKeyChecking=" + hostKeyCheckingString,
                     "-o", "Port=" + QString::number(params.port())};
    if (!params.userName().isEmpty())
        args << "-o" << "User=" + params.userName();
    args << "-o" << "BatchMode=no";
    if (useTimeout)
        args << "-o" << "ConnectTimeout=" + QString::number(params.timeout());
    args << params.host();
    cmd.addArgs(args);

    QString execCommandString("exec /bin/sh -c");
    ProcessArgs::addArg(&execCommandString, command, OsType::OsTypeLinux);
    cmd.addArg(execCommandString);

    m_process.setCommand(cmd);
    SshParameters::setupSshEnvironment(&m_process);
    m_process.start();
}

void PublicKeyDeploymentDialog::handleDeploymentDone(const Result<> &result)
{
    QString buttonText = result ? Tr::tr("Deployment finished successfully.") : result.error();
    const QString textColor = creatorColor(
                                  result ? Theme::TextColorNormal : Theme::TextColorError).name();
    setLabelText(QString::fromLatin1("<font color=\"%1\">%2</font>")
            .arg(textColor, buttonText.replace("\n", "<br/>")));
    setCancelButtonText(Tr::tr("Close"));

    if (!result)
        return;

    setValue(1);
    m_done = true;
}

bool runPublicKeyDeploymentDialog(const DeviceConstRef &device, const FilePath &publicKeyFilePath)
{
    FilePath keyPath = publicKeyFilePath;
    if (keyPath.isEmpty()) {
        const FilePath dir = device.sshParameters().privateKeyFile().parentDir();
        keyPath = FileUtils::getOpenFilePath(
            Tr::tr("Choose Public Key File"), dir,
            Tr::tr("Public Key Files (*.pub);;All Files (*)"));
    }
    if (keyPath.isEmpty())
        return false;

    PublicKeyDeploymentDialog dialog(device, keyPath);
    return dialog.exec() == QDialog::Accepted;
}

} // namespace RemoteLinux::Internal
