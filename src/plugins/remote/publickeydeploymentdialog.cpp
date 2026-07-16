// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "publickeydeploymentdialog.h"

#include "powershellutils.h"
#include "remotelinuxtr.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/sshparameters.h>
#include <projectexplorer/devicesupport/sshsettings.h>

#include <utils/filepath.h>
#include <utils/fileutils.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>

#include <QProcess>
#include <QProgressDialog>

using namespace ProjectExplorer;
using namespace Utils;

namespace Remote::Internal {

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

    // The label text can contain a long error message
    auto label = new QLabel(this);
    label->setWordWrap(true);
    setLabel(label);

    setLabelText(Tr::tr("Deploying..."));
    setValue(0);
    connect(this, &PublicKeyDeploymentDialog::canceled, this,
            [this] { m_done ? accept() : reject(); });
    connect(&m_process, &Process::done, this, [this] {
        const bool succeeded = m_process.result() == ProcessResult::FinishedWithSuccess;
        Result<> result = ResultOk;
        if (!succeeded) {
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

    const IDevice::ConstPtr dev = device.lock();
    const bool isWindowsDevice = dev && dev->osType() == OsTypeWindows;

    const SshParameters params = device.sshParameters();

    CommandLine cmd{sshSettings().sshFilePath()};
    if (isWindowsDevice) {
        // Native Windows OpenSSH: no POSIX shell. Append the key to
        // %USERPROFILE%\.ssh\authorized_keys via "powershell -EncodedCommand", which works
        // whether the remote default shell is cmd.exe or PowerShell.
        cmd.addArgs(params.connectionOptions(sshSettings().sshFilePath()));
        cmd.addArg(params.host());

        const QString key = QString::fromLocal8Bit(publicKey.value()).trimmed();
        const QString script = QStringLiteral(
            "$ErrorActionPreference = 'Stop';"
            "$d = Join-Path $env:USERPROFILE '.ssh';"
            "if (!(Test-Path $d)) { New-Item -ItemType Directory -Path $d | Out-Null };"
            "Add-Content -Path (Join-Path $d 'authorized_keys') -Value %1 -Encoding ascii")
            .arg(psQuote(key));
        cmd.addArg("powershell -NoProfile -NonInteractive -EncodedCommand "
                   + encodePowerShellCommand(script));

        // Windows OpenSSH keeps the session open waiting for stdin EOF after the remote
        // command has finished, so point stdin at the null device to let it terminate.
        m_process.setStandardInputFile(QProcess::nullDevice());
    } else {
        const QString command = "test -d .ssh || mkdir -p ~/.ssh && chmod 0700 .ssh && echo '"
                + QString::fromLocal8Bit(publicKey.value())
                + "' >> .ssh/authorized_keys && chmod 0600 .ssh/authorized_keys";

        const QString hostKeyCheckingString
            = params.hostKeyCheckingMode() == SshHostKeyCheckingStrict ? QLatin1String("yes")
                                                                       : QLatin1String("no");
        const bool isWindowsHost = HostOsInfo::isWindowsHost()
                && sshSettings().sshFilePath().path().toLower().contains("/system32/");
        const bool useTimeout = (params.timeout() != 0) && !isWindowsHost;

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
    }

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
            Tr::tr("Choose Public Key File"),
            dir,
            Tr::tr("Public Key Files (*.pub)") + ";;"
                + Core::DocumentManager::allFilesFilterString());
    }
    if (keyPath.isEmpty())
        return false;

    PublicKeyDeploymentDialog dialog(device, keyPath);
    return dialog.exec() == QDialog::Accepted;
}

} // namespace Remote::Internal
