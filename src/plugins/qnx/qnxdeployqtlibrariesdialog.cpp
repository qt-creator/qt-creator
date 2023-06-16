// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qnxdeployqtlibrariesdialog.h"

#include "qnxconstants.h"
#include "qnxqtversion.h"
#include "qnxtr.h"

#include <projectexplorer/deployablefile.h>
#include <projectexplorer/devicesupport/filetransfer.h>
#include <projectexplorer/devicesupport/idevice.h>

#include <qtsupport/qtversionmanager.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/process.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>

#include <QComboBox>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Tasking;
using namespace Utils;

namespace Qnx::Internal {

const int MaxConcurrentStatCalls = 10;

class QnxDeployQtLibrariesDialogPrivate : public QObject
{
public:
    QnxDeployQtLibrariesDialogPrivate(QnxDeployQtLibrariesDialog *parent,
                                      const IDevice::ConstPtr &device);

    void start();
    void stop();

    void updateProgress(const QString &progressMessage);
    void handleUploadFinished();

    QList<ProjectExplorer::DeployableFile> gatherFiles();
    QList<ProjectExplorer::DeployableFile> gatherFiles(const QString &dirPath,
                                                       const QString &baseDir = {},
                                                       const QStringList &nameFilters = {});

    QString fullRemoteDirectory() const { return m_remoteDirectory->text(); }

    QnxDeployQtLibrariesDialog *q;

    QComboBox *m_qtLibraryCombo;
    QPushButton *m_deployButton;
    QLabel *m_basePathLabel;
    QLineEdit *m_remoteDirectory;
    QProgressBar *m_deployProgress;
    QPlainTextEdit *m_deployLogWindow;
    QPushButton *m_closeButton;

    ProjectExplorer::IDeviceConstPtr m_device;

    int m_progressCount = 0;

    void emitProgressMessage(const QString &msg)
    {
        updateProgress(msg);
        m_deployLogWindow->appendPlainText(msg);
    }

    void emitErrorMessage(const QString &msg)
    {
        m_deployLogWindow->appendPlainText(msg);
    }

    void emitWarningMessage(const QString &message)
    {
        if (!message.contains("stat:"))
            m_deployLogWindow->appendPlainText(message);
    }

private:
    Group deployRecipe();
    GroupItem checkDirTask();
    GroupItem removeDirTask();
    GroupItem uploadTask();
    GroupItem chmodTask(const DeployableFile &file);
    GroupItem chmodTree();

    enum class CheckResult { RemoveDir, SkipRemoveDir, Abort };
    CheckResult m_checkResult = CheckResult::Abort;
    mutable QList<DeployableFile> m_deployableFiles;
    std::unique_ptr<TaskTree> m_taskTree;
};

QList<DeployableFile> collectFilesToUpload(const DeployableFile &deployable)
{
    QList<DeployableFile> collected;
    FilePath localFile = deployable.localFilePath();
    if (localFile.isDir()) {
        const FilePaths files = localFile.dirEntries(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        const QString remoteDir = deployable.remoteDirectory() + '/' + localFile.fileName();
        for (const FilePath &localFilePath : files)
            collected.append(collectFilesToUpload(DeployableFile(localFilePath, remoteDir)));
    } else {
        collected << deployable;
    }
    return collected;
}

GroupItem QnxDeployQtLibrariesDialogPrivate::checkDirTask()
{
    const auto setupHandler = [this](Process &process) {
        m_deployLogWindow->appendPlainText(Tr::tr("Checking existence of \"%1\"")
                                           .arg(fullRemoteDirectory()));
        process.setCommand({m_device->filePath("test"), {"-d", fullRemoteDirectory()}});
    };
    const auto doneHandler = [this](const Process &process) {
        Q_UNUSED(process)
        const int answer = QMessageBox::question(q, q->windowTitle(),
                Tr::tr("The remote directory \"%1\" already exists.\n"
                       "Deploying to that directory will remove any files already present.\n\n"
                       "Are you sure you want to continue?").arg(fullRemoteDirectory()),
                       QMessageBox::Yes | QMessageBox::No);
        m_checkResult = answer == QMessageBox::Yes ? CheckResult::RemoveDir : CheckResult::Abort;
    };
    const auto errorHandler = [this](const Process &process) {
        if (process.result() != ProcessResult::FinishedWithError) {
            m_deployLogWindow->appendPlainText(Tr::tr("Connection failed: %1")
                                               .arg(process.errorString()));
            m_checkResult = CheckResult::Abort;
            return;
        }
        m_checkResult = CheckResult::SkipRemoveDir;
    };
    return ProcessTask(setupHandler, doneHandler, errorHandler);
}

GroupItem QnxDeployQtLibrariesDialogPrivate::removeDirTask()
{
    const auto setupHandler = [this](Process &process) {
        if (m_checkResult != CheckResult::RemoveDir)
            return SetupResult::StopWithDone;
        m_deployLogWindow->appendPlainText(Tr::tr("Removing \"%1\"").arg(fullRemoteDirectory()));
        process.setCommand({m_device->filePath("rm"), {"-rf", fullRemoteDirectory()}});
        return SetupResult::Continue;
    };
    const auto errorHandler = [this](const Process &process) {
        QTC_ASSERT(process.exitCode() == 0, return);
        m_deployLogWindow->appendPlainText(Tr::tr("Connection failed: %1")
                                           .arg(process.errorString()));
    };
    return ProcessTask(setupHandler, {}, errorHandler);
}

GroupItem QnxDeployQtLibrariesDialogPrivate::uploadTask()
{
    const auto setupHandler = [this](FileTransfer &transfer) {
        if (m_deployableFiles.isEmpty()) {
            emitProgressMessage(Tr::tr("No files need to be uploaded."));
            return SetupResult::StopWithDone;
        }
        emitProgressMessage(Tr::tr("%n file(s) need to be uploaded.", "",
                                   m_deployableFiles.size()));
        FilesToTransfer files;
        for (const DeployableFile &file : std::as_const(m_deployableFiles)) {
            if (!file.localFilePath().exists()) {
                const QString message = Tr::tr("Local file \"%1\" does not exist.")
                                              .arg(file.localFilePath().toUserOutput());
                emitErrorMessage(message);
                return SetupResult::StopWithError;
            }
            files.append({file.localFilePath(), m_device->filePath(file.remoteFilePath())});
        }
        if (files.isEmpty()) {
            emitProgressMessage(Tr::tr("No files need to be uploaded."));
            return SetupResult::StopWithDone;
        }
        transfer.setFilesToTransfer(files);
        QObject::connect(&transfer, &FileTransfer::progress,
                         this, &QnxDeployQtLibrariesDialogPrivate::emitProgressMessage);
        return SetupResult::Continue;
    };
    const auto errorHandler = [this](const FileTransfer &transfer) {
        emitErrorMessage(transfer.resultData().m_errorString);
    };
    return FileTransferTask(setupHandler, {}, errorHandler);
}

GroupItem QnxDeployQtLibrariesDialogPrivate::chmodTask(const DeployableFile &file)
{
    const auto setupHandler = [=](Process &process) {
        process.setCommand({m_device->filePath("chmod"),
                {"a+x", Utils::ProcessArgs::quoteArgUnix(file.remoteFilePath())}});
    };
    const auto errorHandler = [=](const Process &process) {
        const QString error = process.errorString();
        if (!error.isEmpty()) {
            emitWarningMessage(Tr::tr("Remote chmod failed for file \"%1\": %2")
                                   .arg(file.remoteFilePath(), error));
        } else if (process.exitCode() != 0) {
            emitWarningMessage(Tr::tr("Remote chmod failed for file \"%1\": %2")
                                   .arg(file.remoteFilePath(), process.cleanedStdErr()));
        }
    };
    return ProcessTask(setupHandler, {}, errorHandler);
}

GroupItem QnxDeployQtLibrariesDialogPrivate::chmodTree()
{
    const auto setupChmodHandler = [=](TaskTree &tree) {
        QList<DeployableFile> filesToChmod;
        for (const DeployableFile &file : std::as_const(m_deployableFiles)) {
            if (file.isExecutable())
                filesToChmod << file;
        }
        QList<GroupItem> chmodList{finishAllAndDone, parallelLimit(MaxConcurrentStatCalls)};
        for (const DeployableFile &file : std::as_const(filesToChmod)) {
            QTC_ASSERT(file.isValid(), continue);
            chmodList.append(chmodTask(file));
        }
        tree.setRecipe(chmodList);
    };
    return TaskTreeTask{setupChmodHandler};
}

Group QnxDeployQtLibrariesDialogPrivate::deployRecipe()
{
    const auto setupHandler = [this] {
        if (!m_device) {
            emitErrorMessage(Tr::tr("No device configuration set."));
            return SetupResult::StopWithError;
        }
        QList<DeployableFile> collected;
        for (int i = 0; i < m_deployableFiles.count(); ++i)
            collected.append(collectFilesToUpload(m_deployableFiles.at(i)));

        QTC_CHECK(collected.size() >= m_deployableFiles.size());
        m_deployableFiles = collected;
        if (!m_deployableFiles.isEmpty())
            return SetupResult::Continue;

        emitProgressMessage(Tr::tr("No deployment action necessary. Skipping."));
        return SetupResult::StopWithDone;
    };
    const auto doneHandler = [this] {
        emitProgressMessage(Tr::tr("All files successfully deployed."));
    };
    const auto subGroupSetupHandler = [this] {
        if (m_checkResult == CheckResult::Abort)
            return SetupResult::StopWithError;
        return SetupResult::Continue;
    };
    const Group root {
        onGroupSetup(setupHandler),
        Group {
            finishAllAndDone,
            checkDirTask()
        },
        Group {
            onGroupSetup(subGroupSetupHandler),
            removeDirTask(),
            uploadTask(),
            chmodTree()
        },
        onGroupDone(doneHandler)
    };
    return root;
}

void QnxDeployQtLibrariesDialogPrivate::start()
{
    QTC_ASSERT(m_device, return);
    QTC_ASSERT(!m_taskTree, return);
    if (m_remoteDirectory->text().isEmpty()) {
        QMessageBox::warning(q, q->windowTitle(),
                             Tr::tr("Please input a remote directory to deploy to."));
        return;
    }

    m_progressCount = 0;
    m_deployProgress->setValue(0);
    m_remoteDirectory->setEnabled(false);
    m_deployButton->setEnabled(false);
    m_qtLibraryCombo->setEnabled(false);
    m_deployLogWindow->clear();

    m_checkResult = CheckResult::Abort;

    m_deployableFiles = gatherFiles();
    m_deployProgress->setRange(0, m_deployableFiles.count());

    m_taskTree.reset(new TaskTree(deployRecipe()));
    const auto endHandler = [this] {
        m_taskTree.release()->deleteLater();
        handleUploadFinished();
    };
    connect(m_taskTree.get(), &TaskTree::done, this, endHandler);
    connect(m_taskTree.get(), &TaskTree::errorOccurred, this, endHandler);
    m_taskTree->start();
}

void QnxDeployQtLibrariesDialogPrivate::stop()
{
    if (!m_taskTree)
        return;
    m_taskTree.reset();
    handleUploadFinished();
}

QnxDeployQtLibrariesDialog::QnxDeployQtLibrariesDialog(const IDevice::ConstPtr &device,
                                                       QWidget *parent)
    : QDialog(parent), d(new QnxDeployQtLibrariesDialogPrivate(this, device))
{
    setWindowTitle(Tr::tr("Deploy Qt to QNX Device"));
}

QnxDeployQtLibrariesDialogPrivate::QnxDeployQtLibrariesDialogPrivate(
    QnxDeployQtLibrariesDialog *parent, const IDevice::ConstPtr &device)
    : q(parent), m_device(device)
{
    m_qtLibraryCombo = new QComboBox(q);
    const QList<QtVersion*> qtVersions = QtVersionManager::sortVersions(
                QtVersionManager::versions(QtVersion::isValidPredicate(
                equal(&QtVersion::type, QString::fromLatin1(Constants::QNX_QNX_QT)))));
    for (QtVersion *v : qtVersions)
        m_qtLibraryCombo->addItem(v->displayName(), v->uniqueId());

    m_deployButton = new QPushButton(Tr::tr("Deploy"), q);
    m_deployButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_basePathLabel = new QLabel(q);

    m_remoteDirectory = new QLineEdit(q);
    m_remoteDirectory->setText(QLatin1String("/qt"));

    m_deployProgress = new QProgressBar(q);
    m_deployProgress->setValue(0);
    m_deployProgress->setTextVisible(true);

    m_deployLogWindow = new QPlainTextEdit(q);

    m_closeButton = new QPushButton(Tr::tr("Close"), q);

    using namespace Layouting;

    Column {
        Form {
            Tr::tr("Qt library to deploy:"), m_qtLibraryCombo, m_deployButton, br,
            Tr::tr("Remote directory:"), m_basePathLabel, m_remoteDirectory, br
        },
        m_deployProgress,
        m_deployLogWindow,
        Row { st, m_closeButton }
    }.attachTo(q);

    connect(m_deployButton, &QAbstractButton::clicked,
            this, &QnxDeployQtLibrariesDialogPrivate::start);
    connect(m_closeButton, &QAbstractButton::clicked,
            q, &QWidget::close);
}

QnxDeployQtLibrariesDialog::~QnxDeployQtLibrariesDialog()
{
    delete d;
}

int QnxDeployQtLibrariesDialog::execAndDeploy(int qtVersionId, const QString &remoteDirectory)
{
    d->m_remoteDirectory->setText(remoteDirectory);
    d->m_qtLibraryCombo->setCurrentIndex(d->m_qtLibraryCombo->findData(qtVersionId));

    d->start();
    return exec();
}

void QnxDeployQtLibrariesDialog::closeEvent(QCloseEvent *event)
{
    // A disabled Deploy button indicates the upload is still running
    if (!d->m_deployButton->isEnabled()) {
        const int answer = QMessageBox::question(this, windowTitle(),
            Tr::tr("Closing the dialog will stop the deployment. Are you sure you want to do this?"),
            QMessageBox::Yes | QMessageBox::No);
        if (answer == QMessageBox::No)
            event->ignore();
        else if (answer == QMessageBox::Yes)
            d->stop();
    }
}

void QnxDeployQtLibrariesDialogPrivate::updateProgress(const QString &progressMessage)
{
    const int progress = progressMessage.count("sftp> put") + progressMessage.count("sftp> ln -s");
    if (progress != 0) {
        m_progressCount += progress;
        m_deployProgress->setValue(m_progressCount);
    }
}

void QnxDeployQtLibrariesDialogPrivate::handleUploadFinished()
{
    m_remoteDirectory->setEnabled(true);
    m_deployButton->setEnabled(true);
    m_qtLibraryCombo->setEnabled(true);
}

QList<DeployableFile> QnxDeployQtLibrariesDialogPrivate::gatherFiles()
{
    QList<DeployableFile> result;

    const int qtVersionId = m_qtLibraryCombo->itemData(m_qtLibraryCombo->currentIndex()).toInt();

    auto qtVersion = dynamic_cast<const QnxQtVersion *>(QtVersionManager::version(qtVersionId));

    QTC_ASSERT(qtVersion, return result);

    if (HostOsInfo::isWindowsHost()) {
        result.append(gatherFiles(qtVersion->libraryPath().toString(), {}, {{"*.so.?"}}));
        result.append(gatherFiles(qtVersion->libraryPath().toString() + QLatin1String("/fonts")));
    } else {
        result.append(gatherFiles(qtVersion->libraryPath().toString()));
    }

    result.append(gatherFiles(qtVersion->pluginPath().toString()));
    result.append(gatherFiles(qtVersion->importsPath().toString()));
    result.append(gatherFiles(qtVersion->qmlPath().toString()));
    return result;
}

QList<DeployableFile> QnxDeployQtLibrariesDialogPrivate::gatherFiles(
        const QString &dirPath, const QString &baseDirPath, const QStringList &nameFilters)
{
    QList<DeployableFile> result;
    if (dirPath.isEmpty())
        return result;

    static const QStringList unusedDirs = {"include", "mkspecs", "cmake", "pkgconfig"};
    const QString dp = dirPath.endsWith('/') ? dirPath.left(dirPath.size() - 1) : dirPath;
    if (unusedDirs.contains(dp))
        return result;

    const QDir dir(dirPath);
    const QFileInfoList list = dir.entryInfoList(nameFilters,
                                                 QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo &fileInfo : list) {
        if (fileInfo.isDir()) {
            result.append(gatherFiles(fileInfo.absoluteFilePath(), baseDirPath.isEmpty() ?
                                          dirPath : baseDirPath));
        } else {
            static const QStringList unusedSuffixes = {"cmake", "la", "prl", "a", "pc"};
            if (unusedSuffixes.contains(fileInfo.suffix()))
                continue;

            QString remoteDir;
            if (baseDirPath.isEmpty()) {
                remoteDir = fullRemoteDirectory() + '/' + QFileInfo(dirPath).baseName();
            } else {
                QDir baseDir(baseDirPath);
                baseDir.cdUp();
                remoteDir = fullRemoteDirectory() + '/' + baseDir.relativeFilePath(dirPath);
            }
            result.append(DeployableFile(FilePath::fromString(fileInfo.absoluteFilePath()),
                                         remoteDir));
        }
    }
    return result;
}

} // Qnx::Internal
