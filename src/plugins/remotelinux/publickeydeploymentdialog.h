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

#pragma once

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevice.h>

#include <QProgressDialog>

namespace RemoteLinux {
namespace Internal { class PublicKeyDeploymentDialogPrivate; }

class REMOTELINUX_EXPORT PublicKeyDeploymentDialog : public QProgressDialog
{
    Q_OBJECT
public:
    // Asks for public key and returns null if the file dialog is canceled.
    static PublicKeyDeploymentDialog *createDialog(const ProjectExplorer::IDevice::ConstPtr &deviceConfig,
        QWidget *parent = 0);

    ~PublicKeyDeploymentDialog();

private:
    explicit PublicKeyDeploymentDialog(const ProjectExplorer::IDevice::ConstPtr &deviceConfig,
        const QString &publicKeyFileName, QWidget *parent = 0);
    void handleDeploymentFinished(const QString &errorMsg);
    void handleDeploymentError(const QString &errorMsg);
    void handleDeploymentSuccess();
    void handleCanceled();

    Internal::PublicKeyDeploymentDialogPrivate * const d;
};

} // namespace RemoteLinux
