// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

#include <projectexplorer/devicesupport/idevicefwd.h>

namespace Qnx::Internal {

class QnxDeployQtLibrariesDialog : public QDialog
{
public:
    explicit QnxDeployQtLibrariesDialog(const ProjectExplorer::IDeviceConstPtr &device,
                                        QWidget *parent = nullptr);
    ~QnxDeployQtLibrariesDialog() override;

    int execAndDeploy(int qtVersionId, const QString &remoteDirectory);

private:
    void closeEvent(QCloseEvent *event) override;

    class QnxDeployQtLibrariesDialogPrivate *d = nullptr;
};

} // Qnx::Internal
