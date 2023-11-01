// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevicefwd.h>

#include <QProgressDialog>

namespace Utils { class FilePath; }

namespace RemoteLinux::Internal {

class PublicKeyDeploymentDialogPrivate;

class PublicKeyDeploymentDialog : public QProgressDialog
{
    Q_OBJECT
public:
    // Asks for public key and returns null if the file dialog is canceled.
    static PublicKeyDeploymentDialog *createDialog(
        const ProjectExplorer::IDeviceConstPtr &deviceConfig, QWidget *parent = nullptr);

    PublicKeyDeploymentDialog(const ProjectExplorer::IDeviceConstPtr &deviceConfig,
                              const Utils::FilePath &publicKeyFileName, QWidget *parent = nullptr);

    ~PublicKeyDeploymentDialog() override;

private:
    void handleDeploymentDone(bool succeeded, const QString &errorMessage);

    Internal::PublicKeyDeploymentDialogPrivate * const d;
};

} // namespace RemoteLinux::Internal
