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
#include "publickeydeploymentdialog.h"

#include "sshkeydeployer.h"

#include <coreplugin/icore.h>
#include <ssh/sshconnection.h>

#include <QFileDialog>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {
class PublicKeyDeploymentDialogPrivate
{
public:
    SshKeyDeployer keyDeployer;
    bool done;
};
} // namespace Internal;

using namespace Internal;

PublicKeyDeploymentDialog *PublicKeyDeploymentDialog::createDialog(const IDevice::ConstPtr &deviceConfig,
    QWidget *parent)
{
    const QString &dir = QFileInfo(deviceConfig->sshParameters().privateKeyFile).path();
    const QString publicKeyFileName = QFileDialog::getOpenFileName(parent
            ? parent : Core::ICore::mainWindow(),
        tr("Choose Public Key File"), dir,
        tr("Public Key Files (*.pub);;All Files (*)"));
    if (publicKeyFileName.isEmpty())
        return 0;
    return new PublicKeyDeploymentDialog(deviceConfig, publicKeyFileName, parent);
}

PublicKeyDeploymentDialog::PublicKeyDeploymentDialog(const IDevice::ConstPtr &deviceConfig,
        const QString &publicKeyFileName, QWidget *parent)
    : QProgressDialog(parent), d(new PublicKeyDeploymentDialogPrivate)
{
    setAutoReset(false);
    setAutoClose(false);
    setMinimumDuration(0);
    setMaximum(1);

    d->done = false;
    setLabelText(tr("Deploying..."));
    setValue(0);
    connect(this, &PublicKeyDeploymentDialog::canceled,
            this, &PublicKeyDeploymentDialog::handleCanceled);
    connect(&d->keyDeployer, &SshKeyDeployer::error,
            this, &PublicKeyDeploymentDialog::handleDeploymentError);
    connect(&d->keyDeployer, &SshKeyDeployer::finishedSuccessfully,
            this, &PublicKeyDeploymentDialog::handleDeploymentSuccess);
    d->keyDeployer.deployPublicKey(deviceConfig->sshParameters(), publicKeyFileName);
}

PublicKeyDeploymentDialog::~PublicKeyDeploymentDialog()
{
    delete d;
}

void PublicKeyDeploymentDialog::handleDeploymentSuccess()
{
    handleDeploymentFinished(QString());
    setValue(1);
    d->done = true;
}

void PublicKeyDeploymentDialog::handleDeploymentError(const QString &errorMsg)
{
    handleDeploymentFinished(errorMsg);
}

void PublicKeyDeploymentDialog::handleDeploymentFinished(const QString &errorMsg)
{
    QString buttonText;
    const char *textColor;
    if (errorMsg.isEmpty()) {
        buttonText = tr("Deployment finished successfully.");
        textColor = "blue";
    } else {
        buttonText = errorMsg;
        textColor = "red";
    }
    setLabelText(QString::fromLatin1("<font color=\"%1\">%2</font>").arg(QLatin1String(textColor), buttonText));
    setCancelButtonText(tr("Close"));
}

void PublicKeyDeploymentDialog::handleCanceled()
{
    disconnect(&d->keyDeployer, 0, this, 0);
    d->keyDeployer.stopDeployment();
    if (d->done)
        accept();
    else
        reject();
}

} // namespace RemoteLinux
