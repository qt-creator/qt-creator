/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "maemopublisherfremantlefree.h"

#include "maemoglobal.h"
#include "maemoconstants.h"
#include "debianmanager.h"
#include "maemopackagecreationstep.h"
#include "maemopublishingfileselectiondialog.h"
#include "qt4maemodeployconfiguration.h"

#include <coreplugin/idocument.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qmakestep.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <ssh/sshremoteprocessrunner.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QIcon>

using namespace Core;
using namespace Qt4ProjectManager;
using namespace RemoteLinux;
using namespace QSsh;
using namespace Utils;

namespace Madde {
namespace Internal {

MaemoPublisherFremantleFree::MaemoPublisherFremantleFree(const ProjectExplorer::Project *project,
    QObject *parent) :
    QObject(parent),
    m_project(project),
    m_state(Inactive),
    m_uploader(0)
{
    m_sshParams.authenticationType = SshConnectionParameters::AuthenticationByKey;
    m_sshParams.timeout = 30;
    m_sshParams.port = 22;
    m_process = new QProcess(this);
    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)),
        SLOT(handleProcessFinished()));
    connect(m_process, SIGNAL(error(QProcess::ProcessError)),
        SLOT(handleProcessError(QProcess::ProcessError)));
    connect(m_process, SIGNAL(readyReadStandardOutput()),
        SLOT(handleProcessStdOut()));
    connect(m_process, SIGNAL(readyReadStandardError()),
        SLOT(handleProcessStdErr()));
}

MaemoPublisherFremantleFree::~MaemoPublisherFremantleFree()
{
    QTC_ASSERT(m_state == Inactive, return);
    m_process->kill();
}

void MaemoPublisherFremantleFree::publish()
{
    createPackage();
}

void MaemoPublisherFremantleFree::setSshParams(const QString &hostName,
    const QString &userName, const QString &keyFile, const QString &remoteDir)
{
    Q_ASSERT(m_doUpload);
    m_sshParams.host = hostName;
    m_sshParams.userName = userName;
    m_sshParams.privateKeyFile = keyFile;
    m_remoteDir = remoteDir;
}

void MaemoPublisherFremantleFree::cancel()
{
    finishWithFailure(tr("Canceled."), tr("Publishing canceled by user."));
}

void MaemoPublisherFremantleFree::createPackage()
{
    setState(CopyingProjectDir);

    const QStringList &problems = findProblems();
    if (!problems.isEmpty()) {
        const QLatin1String separator("\n- ");
        finishWithFailure(tr("The project is missing some information "
            "important to publishing:") + separator + problems.join(separator),
            tr("Publishing failed: Missing project information."));
        return;
    }

    m_tmpProjectDir = tmpDirContainer() + QLatin1Char('/')
        + m_project->displayName();
    if (QFileInfo(tmpDirContainer()).exists()) {
        emit progressReport(tr("Removing left-over temporary directory..."));
        QString error;
        if (!FileUtils::removeRecursively(FileName::fromString(tmpDirContainer()), &error)) {
            finishWithFailure(tr("Error removing temporary directory: %1").arg(error),
                tr("Publishing failed: Could not create source package."));
            return;
        }
    }

    emit progressReport(tr("Setting up temporary directory..."));
    if (!QDir::temp().mkdir(QFileInfo(tmpDirContainer()).fileName())) {
        finishWithFailure(tr("Error: Could not create temporary directory."),
            tr("Publishing failed: Could not create source package."));
        return;
    }
    if (!copyRecursively(m_project->projectDirectory(), m_tmpProjectDir)) {
        finishWithFailure(tr("Error: Could not copy project directory."),
            tr("Publishing failed: Could not create source package."));
        return;
    }
    if (!fixNewlines()) {
        finishWithFailure(tr("Error: Could not fix newlines."),
            tr("Publishing failed: Could not create source package."));
        return;
    }

    emit progressReport(tr("Cleaning up temporary directory..."));
    AbstractMaemoPackageCreationStep::preparePackagingProcess(m_process,
            m_buildConfig, m_tmpProjectDir);
    setState(RunningQmake);
    ProjectExplorer::AbstractProcessStep * const qmakeStep
        = m_buildConfig->qmakeStep();
    qmakeStep->init();
    const ProjectExplorer::ProcessParameters * const pp
        = qmakeStep->processParameters();
    m_process->start(pp->effectiveCommand() + QLatin1String(" ")
        + pp->effectiveArguments());
}

bool MaemoPublisherFremantleFree::copyRecursively(const QString &srcFilePath,
    const QString &tgtFilePath)
{
    if (m_state == Inactive)
        return true;

    QFileInfo srcFileInfo(srcFilePath);
    if (srcFileInfo.isDir()) {
        if (srcFileInfo == QFileInfo(m_project->projectDirectory()
               + QLatin1String("/debian")))
            return true;
        QString actualSourcePath = srcFilePath;
        QString actualTargetPath = tgtFilePath;

        if (srcFileInfo.fileName() == QLatin1String("qtc_packaging")) {
            actualSourcePath += QLatin1String("/debian_fremantle");
            actualTargetPath.replace(QRegExp(QLatin1String("qtc_packaging$")),
                QLatin1String("debian"));
        }

        QDir targetDir(actualTargetPath);
        targetDir.cdUp();
        if (!targetDir.mkdir(QFileInfo(actualTargetPath).fileName())) {
            emit progressReport(tr("Failed to create directory '%1'.")
                .arg(QDir::toNativeSeparators(actualTargetPath)), ErrorOutput);
            return false;
        }
        QDir sourceDir(actualSourcePath);
        QStringList fileNames = sourceDir.entryList(QDir::Files | QDir::Hidden
            | QDir::System | QDir::Dirs | QDir::NoDotAndDotDot);
        foreach (const QString &fileName, fileNames) {
            if (!copyRecursively(actualSourcePath + QLatin1Char('/') + fileName,
                    actualTargetPath + QLatin1Char('/') + fileName))
                return false;
        }
    } else {
        if (tgtFilePath == m_tmpProjectDir + QLatin1String("/debian/rules")) {
            FileReader reader;
            if (!reader.fetch(srcFilePath)) {
                emit progressReport(reader.errorString(), ErrorOutput);
                return false;
            }
            QByteArray rulesContents = reader.data();
            rulesContents.replace("$(MAKE) clean", "# $(MAKE) clean");
            rulesContents.replace("# Add here commands to configure the package.",
                "qmake " + QFileInfo(m_project->document()->fileName()).fileName().toLocal8Bit());
            MaemoDebianPackageCreationStep::ensureShlibdeps(rulesContents);
            FileSaver saver(tgtFilePath);
            saver.write(rulesContents);
            if (!saver.finalize()) {
                emit progressReport(saver.errorString(), ErrorOutput);
                return false;
            }
            QFile rulesFile(tgtFilePath);
            if (!rulesFile.setPermissions(rulesFile.permissions() | QFile::ExeUser)) {
                emit progressReport(tr("Could not set execute permissions for rules file: %1")
                    .arg(rulesFile.errorString()));
                return false;
            }
        } else {
            QFile srcFile(srcFilePath);
            if (!srcFile.copy(tgtFilePath)) {
                emit progressReport(tr("Could not copy file '%1' to '%2': %3.")
                    .arg(QDir::toNativeSeparators(srcFilePath),
                         QDir::toNativeSeparators(tgtFilePath),
                         srcFile.errorString()));
                return false;
            }
        }
    }
    return true;
}

bool MaemoPublisherFremantleFree::fixNewlines()
{
    QDir debianDir(m_tmpProjectDir + QLatin1String("/debian"));
    const QStringList &fileNames = debianDir.entryList(QDir::Files);
    foreach (const QString &fileName, fileNames) {
        QString filePath = debianDir.filePath(fileName);
        FileReader reader;
        if (!reader.fetch(filePath))
            return false;
        QByteArray contents = reader.data();
        const QByteArray crlf("\r\n");
        if (!contents.contains(crlf))
            continue;
        contents.replace(crlf, "\n");
        FileSaver saver(filePath);
        saver.write(contents);
        if (!saver.finalize())
            return false;
    }
    return true;
}

void MaemoPublisherFremantleFree::handleProcessError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart)
        handleProcessFinished(true);
}

void MaemoPublisherFremantleFree::handleProcessFinished()
{
    handleProcessFinished(false);
}

void MaemoPublisherFremantleFree::handleProcessStdOut()
{
    if (m_state == RunningQmake || m_state == RunningMakeDistclean
            || m_state == BuildingPackage) {
        emit progressReport(QString::fromLocal8Bit(m_process->readAllStandardOutput()),
            ToolStatusOutput);
    }
}

void MaemoPublisherFremantleFree::handleProcessStdErr()
{
    if (m_state == RunningQmake || m_state == RunningMakeDistclean
            || m_state == BuildingPackage) {
        emit progressReport(QString::fromLocal8Bit(m_process->readAllStandardError()),
            ToolErrorOutput);
    }
}

void MaemoPublisherFremantleFree::handleProcessFinished(bool failedToStart)
{
    QTC_ASSERT(m_state == RunningQmake || m_state == RunningMakeDistclean
        || m_state == BuildingPackage || m_state == Inactive, return);

    switch (m_state) {
    case RunningQmake:
        if (failedToStart || m_process->exitStatus() != QProcess::NormalExit
                ||m_process->exitCode() != 0) {
            runDpkgBuildPackage();
        } else {
            setState(RunningMakeDistclean);
            // Toolchain might be null! (yes because this sucks)
            ProjectExplorer::ToolChain *tc
                    = ProjectExplorer::ToolChainKitInformation::toolChain(m_buildConfig->target()->kit());
            if (!tc) {
                finishWithFailure(QString(), tr("Make distclean failed: %1")
                                  .arg(ProjectExplorer::ToolChainKitInformation::msgNoToolChainInTarget()));
            }
            m_process->start(tc->makeCommand(m_buildConfig->environment()), QStringList() << QLatin1String("distclean"));
        }
        break;
    case RunningMakeDistclean:
        runDpkgBuildPackage();
        break;
    case BuildingPackage: {
        QString error;
        if (failedToStart) {
            error = tr("Error: Failed to start dpkg-buildpackage.");
        } else if (m_process->exitStatus() != QProcess::NormalExit
                   || m_process->exitCode() != 0) {
            error = tr("Error: dpkg-buildpackage did not succeed.");
        }

        if (!error.isEmpty()) {
            finishWithFailure(error, tr("Package creation failed."));
            return;
        }

        QDir dir(tmpDirContainer());
        const QStringList &fileNames = dir.entryList(QDir::Files);
        foreach (const QString &fileName, fileNames) {
            const QString filePath
                = tmpDirContainer() + QLatin1Char('/') + fileName;
            if (fileName.endsWith(QLatin1String(".dsc")))
                m_filesToUpload.append(filePath);
            else
                m_filesToUpload.prepend(filePath);
        }
        if (!m_doUpload) {
            emit progressReport(tr("Done."));
            QStringList nativeFilePaths;
            foreach (const QString &filePath, m_filesToUpload)
                nativeFilePaths << QDir::toNativeSeparators(filePath);
            m_resultString = tr("Packaging finished successfully. "
                "The following files were created:\n")
                + nativeFilePaths.join(QLatin1String("\n"));
            setState(Inactive);
        } else {
            uploadPackage();
        }
        break;
    }
    default:
        break;
    }
}

void MaemoPublisherFremantleFree::runDpkgBuildPackage()
{
    MaemoPublishingFileSelectionDialog d(m_tmpProjectDir);
    if (d.exec() == QDialog::Rejected) {
        cancel();
        return;
    }
    foreach (const QString &filePath, d.filesToExclude()) {
        QString error;
        if (!FileUtils::removeRecursively(FileName::fromString(filePath), &error)) {
            finishWithFailure(error,
                tr("Publishing failed: Could not create package."));
        }
    }

    QtSupport::BaseQtVersion *lqt = QtSupport::QtKitInformation::qtVersion(m_buildConfig->target()->kit());
    if (!lqt)
        finishWithFailure(QString(), tr("No Qt version set."));

    if (m_state == Inactive)
        return;
    setState(BuildingPackage);
    emit progressReport(tr("Building source package..."));
    const QStringList args = QStringList() << QLatin1String("dpkg-buildpackage")
        << QLatin1String("-S") << QLatin1String("-us") << QLatin1String("-uc");
    MaemoGlobal::callMad(*m_process, args, lqt->qmakeCommand().toString(), true);
}

// We have to implement the SCP protocol, because the maemo.org
// webmaster refuses to enable SFTP "for security reasons" ...
void MaemoPublisherFremantleFree::uploadPackage()
{
    if (!m_uploader)
        m_uploader = new SshRemoteProcessRunner(this);
    connect(m_uploader, SIGNAL(processStarted()), SLOT(handleScpStarted()));
    connect(m_uploader, SIGNAL(connectionError()), SLOT(handleConnectionError()));
    connect(m_uploader, SIGNAL(processClosed(int)), SLOT(handleUploadJobFinished(int)));
    connect(m_uploader, SIGNAL(readyReadStandardOutput()), SLOT(handleScpStdOut()));
    emit progressReport(tr("Starting scp..."));
    setState(StartingScp);
    m_uploader->run("scp -td " + m_remoteDir.toUtf8(), m_sshParams);
}

void MaemoPublisherFremantleFree::handleScpStarted()
{
    QTC_ASSERT(m_state == StartingScp || m_state == Inactive, return);

    if (m_state == StartingScp)
        prepareToSendFile();
}

void MaemoPublisherFremantleFree::handleConnectionError()
{
    if (m_state != Inactive) {
        finishWithFailure(tr("SSH error: %1").arg(m_uploader->lastConnectionErrorString()),
            tr("Upload failed."));
    }
}

void MaemoPublisherFremantleFree::handleUploadJobFinished(int exitStatus)
{
    QTC_ASSERT(m_state == PreparingToUploadFile || m_state == UploadingFile || m_state ==Inactive,
        return);

    if (m_state != Inactive && (exitStatus != SshRemoteProcess::NormalExit
            || m_uploader->processExitCode() != 0)) {
        QString error;
        if (exitStatus != SshRemoteProcess::NormalExit) {
            error = tr("Error uploading file: %1.")
                .arg(m_uploader->processErrorString());
        } else {
            error = tr("Error uploading file.");
        }
        finishWithFailure(error, tr("Upload failed."));
    }
}

void MaemoPublisherFremantleFree::prepareToSendFile()
{
    if (m_filesToUpload.isEmpty()) {
        emit progressReport(tr("All files uploaded."));
        m_resultString = tr("Upload succeeded. You should shortly "
            "receive an email informing you about the outcome "
            "of the build process.");
        setState(Inactive);
        return;
    }

    setState(PreparingToUploadFile);
    const QString &nextFilePath = m_filesToUpload.first();
    emit progressReport(tr("Uploading file %1...")
        .arg(QDir::toNativeSeparators(nextFilePath)));
    QFileInfo info(nextFilePath);
    m_uploader->writeDataToProcess("C0644 " + QByteArray::number(info.size())
        + ' ' + info.fileName().toUtf8() + '\n');
}

void MaemoPublisherFremantleFree::sendFile()
{
    Q_ASSERT(!m_filesToUpload.isEmpty());
    Q_ASSERT(m_state == PreparingToUploadFile);

    setState(UploadingFile);
    const QString filePath = m_filesToUpload.takeFirst();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        finishWithFailure(tr("Cannot open file for reading: %1.")
            .arg(file.errorString()), tr("Upload failed."));
        return;
    }
    qint64 bytesToSend = file.size();
    while (bytesToSend > 0) {
        const QByteArray &data
            = file.read(qMin(bytesToSend, Q_INT64_C(1024*1024)));
        if (data.count() == 0) {
            finishWithFailure(tr("Cannot read file: %1").arg(file.errorString()),
                tr("Upload failed."));
            return;
        }
        m_uploader->writeDataToProcess(data);
        bytesToSend -= data.size();
        QCoreApplication::processEvents();
        if (m_state == Inactive)
            return;
    }
    m_uploader->writeDataToProcess(QByteArray(1, '\0'));
}

void MaemoPublisherFremantleFree::handleScpStdOut()
{
    QTC_ASSERT(m_state == PreparingToUploadFile || m_state == UploadingFile || m_state == Inactive,
        return);

    if (m_state == Inactive)
        return;

    m_scpOutput += m_uploader->readAllStandardOutput();
    if (m_scpOutput == QByteArray(1, '\0')) {
        m_scpOutput.clear();
        switch (m_state) {
        case PreparingToUploadFile:
            sendFile();
            break;
        case UploadingFile:
            prepareToSendFile();
            break;
        default:
            break;
        }
    } else if (m_scpOutput.endsWith('\n')) {
        const QByteArray error = m_scpOutput.mid(1, m_scpOutput.count() - 2);
        QString progressError;
        if (!error.isEmpty()) {
            progressError = tr("Error uploading file: %1.")
                .arg(QString::fromUtf8(error));
        } else {
            progressError = tr("Error uploading file.");
        }
        finishWithFailure(progressError, tr("Upload failed."));
    }
}

QString MaemoPublisherFremantleFree::tmpDirContainer() const
{
    return QDir::tempPath() + QLatin1String("/qtc_packaging_")
        + m_project->displayName();
}

void MaemoPublisherFremantleFree::finishWithFailure(const QString &progressMsg,
    const QString &resultMsg)
{
    if (!progressMsg.isEmpty())
        emit progressReport(progressMsg, ErrorOutput);
    m_resultString = resultMsg;
    setState(Inactive);
}

QStringList MaemoPublisherFremantleFree::findProblems() const
{
    QStringList problems;
    ProjectExplorer::Target *target = m_buildConfig->target();
    Core::Id deviceType
            = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(target->kit());
    if (deviceType != Maemo5OsType)
        return QStringList();

    const QString &description = DebianManager::shortDescription(DebianManager::debianDirectory(target));
    if (description.trimmed().isEmpty()) {
        problems << tr("The package description is empty. You must set one "
            "in Projects -> Run -> Create Package -> Details.");
    } else if (description.contains(QLatin1String("insert up to"))) {
        problems << tr("The package description is '%1', which is probably "
            "not what you want. Please change it in "
            "Projects -> Run -> Create Package -> Details.").arg(description);
    }

    QString dummy;
    if (DebianManager::packageManagerIcon(DebianManager::debianDirectory(target), &dummy).isNull())
        problems << tr("You have not set an icon for the package manager. "
            "The icon must be set in Projects -> Run -> Create Package -> Details.");
    return problems;
}

void MaemoPublisherFremantleFree::setState(State newState)
{
    if (m_state == newState)
        return;
    const State oldState = m_state;
    m_state = newState;
    if (m_state == Inactive) {
        switch (oldState) {
        case RunningQmake:
        case RunningMakeDistclean:
        case BuildingPackage:
            disconnect(m_process, 0, this, 0);
            m_process->terminate();
            break;
        case StartingScp:
        case PreparingToUploadFile:
        case UploadingFile:
            // TODO: Can we ensure the remote scp exits, e.g. by sending
            //       an illegal sequence of bytes? (Probably not, if
            //       we are currently uploading a file.)
            disconnect(m_uploader, 0, this, 0);
            break;
        default:
            break;
        }
        emit finished();
    }
}

} // namespace Internal
} // namespace Madde
