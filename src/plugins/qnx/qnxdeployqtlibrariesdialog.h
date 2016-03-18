/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry (qt@blackberry.com), KDAB (info@kdab.com)
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

#include <QDialog>

#include <projectexplorer/deployablefile.h>
#include <projectexplorer/devicesupport/idevice.h>

namespace QSsh {
class SshRemoteProcessRunner;
}

namespace RemoteLinux {
class GenericDirectUploadService;
}

namespace Qnx {
namespace Internal {

namespace Ui {
class QnxDeployQtLibrariesDialog;
}

class QnxDeployQtLibrariesDialog : public QDialog
{
    Q_OBJECT

public:
    enum State {
        Inactive,
        CheckingRemoteDirectory,
        RemovingRemoteDirectory,
        Uploading
    };

    explicit QnxDeployQtLibrariesDialog(const ProjectExplorer::IDevice::ConstPtr &device,
                                        QWidget *parent = 0);
    ~QnxDeployQtLibrariesDialog();

    int execAndDeploy(int qtVersionId, const QString &remoteDirectory);

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void deployLibraries();
    void updateProgress(const QString &progressMessage);
    void handleUploadFinished();

    void handleRemoteProcessError();
    void handleRemoteProcessCompleted();

private:
    QList<ProjectExplorer::DeployableFile> gatherFiles();
    QList<ProjectExplorer::DeployableFile> gatherFiles(const QString &dirPath,
            const QString &baseDir = QString(),
            const QStringList &nameFilters = QStringList());

    QString fullRemoteDirectory() const;
    void checkRemoteDirectoryExistance();
    void removeRemoteDirectory();
    void startUpload();

    Ui::QnxDeployQtLibrariesDialog *m_ui;

    QSsh::SshRemoteProcessRunner *m_processRunner;
    RemoteLinux::GenericDirectUploadService *m_uploadService;

    ProjectExplorer::IDevice::ConstPtr m_device;

    int m_progressCount;

    State m_state;
};

} // namespace Internal
} // namespace Qnx
