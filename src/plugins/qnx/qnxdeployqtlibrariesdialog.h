// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

#include <projectexplorer/devicesupport/idevicefwd.h>
#include <utils/qtcprocess.h>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
QT_END_NAMESPACE

namespace ProjectExplorer { class DeployableFile; }
namespace RemoteLinux { class GenericDirectUploadService; }

namespace Qnx::Internal {

class QnxDeployQtLibrariesDialog : public QDialog
{
public:
    enum State {
        Inactive,
        CheckingRemoteDirectory,
        RemovingRemoteDirectory,
        Uploading
    };

    explicit QnxDeployQtLibrariesDialog(const ProjectExplorer::IDeviceConstPtr &device,
                                        QWidget *parent = nullptr);
    ~QnxDeployQtLibrariesDialog() override;

    int execAndDeploy(int qtVersionId, const QString &remoteDirectory);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void deployLibraries();
    void updateProgress(const QString &progressMessage);
    void handleUploadFinished();

    void startCheckDirProcess();
    void startRemoveDirProcess();

    void handleCheckDirDone();
    void handleRemoveDirDone();
    bool handleError(const Utils::QtcProcess &process);

    QList<ProjectExplorer::DeployableFile> gatherFiles();
    QList<ProjectExplorer::DeployableFile> gatherFiles(const QString &dirPath,
            const QString &baseDir = QString(),
            const QStringList &nameFilters = QStringList());

    QString fullRemoteDirectory() const;
    void startUpload();

    QComboBox *m_qtLibraryCombo;
    QPushButton *m_deployButton;
    QLabel *m_basePathLabel;
    QLineEdit *m_remoteDirectory;
    QProgressBar *m_deployProgress;
    QPlainTextEdit *m_deployLogWindow;
    QPushButton *m_closeButton;

    Utils::QtcProcess m_checkDirProcess;
    Utils::QtcProcess m_removeDirProcess;
    RemoteLinux::GenericDirectUploadService *m_uploadService;

    ProjectExplorer::IDeviceConstPtr m_device;

    int m_progressCount = 0;
    State m_state = Inactive;
};

} // Qnx::Internal
