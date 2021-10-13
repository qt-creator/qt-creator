/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "androidqmlpreviewworker.h"

#include "androidavdmanager.h"
#include "androiddevice.h"
#include "androiddeviceinfo.h"
#include "androidglobal.h"
#include "androidmanager.h"

#include <coreplugin/icore.h>

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

#include <qmlprojectmanager/qmlprojectconstants.h>
#include <qmlprojectmanager/qmlprojectmanagerconstants.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/runextensions.h>

#include <QDateTime>
#include <QDeadlineTimer>
#include <QThread>

namespace Android {
namespace Internal {

using namespace Utils;

#define APP_ID "io.qt.qtdesignviewer"

class ApkInfo {
public:
    ApkInfo();
    const QStringList abis;
    const QString appId;
    const QString uploadDir;
    const QString activityId;
    const QString name;
};

ApkInfo::ApkInfo() :
    abis({ProjectExplorer::Constants::ANDROID_ABI_X86,
            ProjectExplorer::Constants::ANDROID_ABI_X86_64,
            ProjectExplorer::Constants::ANDROID_ABI_ARM64_V8A,
            ProjectExplorer::Constants::ANDROID_ABI_ARMEABI_V7A}),
    appId(APP_ID),
    uploadDir("/data/local/tmp/" APP_ID "/"),
    // TODO Add possibility to run Qt5 built version of Qt Design Viewer
    activityId(APP_ID "/org.qtproject.qt.android.bindings.QtActivity"),
    name("Qt Design Viewer")
{
}

Q_GLOBAL_STATIC(ApkInfo, apkInfo)

static const char packageSuffix[] = ".qmlrc";

FilePath AndroidQmlPreviewWorker::designViewerApkPath(const QString &abi) const
{
    if (abi.isEmpty())
        return {};

    if (apkInfo()->abis.contains(abi)) {
        return Core::ICore::resourcePath(QString("android/qtdesignviewer/qtdesignviewer_%1.apk")
                                         .arg(abi));
    }
    return {};
}

SdkToolResult AndroidQmlPreviewWorker::runAdbCommand(const QStringList &arguments) const
{
    QStringList args;
    if (!m_serialNumber.isEmpty())
        args << AndroidDeviceInfo::adbSelector(m_serialNumber);
    args << arguments;
    return AndroidManager::runAdbCommand(args);
}

SdkToolResult AndroidQmlPreviewWorker::runAdbShellCommand(const QStringList &arguments) const
{
    return runAdbCommand(QStringList() << "shell" << arguments);
}

int AndroidQmlPreviewWorker::pidofPreview() const
{
    const QStringList command{"pidof", apkInfo()->appId};
    const SdkToolResult res = runAdbShellCommand(command);
    return res.success() ? res.stdOut().toInt() : -1;
}

bool AndroidQmlPreviewWorker::isPreviewRunning(int lastKnownPid) const
{
    const int pid = pidofPreview();
    return (lastKnownPid > 1) ? lastKnownPid == pid : pid > 1;
}

void AndroidQmlPreviewWorker::startPidWatcher()
{
    m_pidFutureWatcher.setFuture(Utils::runAsync([this]() {
        // wait for started
        const int sleepTimeMs = 2000;
        QDeadlineTimer deadline(20000);
        while (!m_pidFutureWatcher.isCanceled() && !deadline.hasExpired()) {
            if (m_viewerPid == -1) {
                m_viewerPid = pidofPreview();
                if (m_viewerPid > 0) {
                    emit previewPidChanged();
                    break;
                }
            }
            QThread::msleep(sleepTimeMs);
        }

        while (!m_pidFutureWatcher.isCanceled()) {
            if (!isPreviewRunning(m_viewerPid)) {
                stop();
                break;
            }
            QThread::msleep(sleepTimeMs);
        }
    }));
}

void AndroidQmlPreviewWorker::startLogcat()
{
    QString args = QString("logcat --pid=%1").arg(m_viewerPid);
    if (!m_logcatStartTimeStamp.isEmpty())
        args += QString(" -T '%1'").arg(m_logcatStartTimeStamp);
    Utils::CommandLine cmd(AndroidConfigurations::currentConfig().adbToolPath());
    cmd.setArguments(args);
    m_logcatProcess.setCommand(cmd);
    m_logcatProcess.setUseCtrlCStub(true);
    m_logcatProcess.start();
}

void AndroidQmlPreviewWorker::filterLogcatAndAppendMessage(const QString &stdOut)
{
    for (const QString &line : stdOut.split('\n')) {
        QStringList splittedLine = line.split(QLatin1String("%1: ").arg(apkInfo()->name));
        if (splittedLine.count() == 1)
            continue;

        const QString outLine = splittedLine.last();
        const QString firstPart = splittedLine.first();
        if (firstPart.contains(" I ") || firstPart.contains(" D "))
            appendMessage(outLine, NormalMessageFormat);
        else
            appendMessage(outLine, ErrorMessageFormat);
    }
}

AndroidQmlPreviewWorker::AndroidQmlPreviewWorker(ProjectExplorer::RunControl *runControl)
    : ProjectExplorer::RunWorker(runControl),
      m_rc(runControl),
      m_androidConfig(AndroidConfigurations::currentConfig())
{
    connect(this, &RunWorker::started, this, &AndroidQmlPreviewWorker::startPidWatcher);
    connect(this, &RunWorker::stopped, &m_pidFutureWatcher, &QFutureWatcher<void>::cancel);
    connect(this, &AndroidQmlPreviewWorker::previewPidChanged,
            this, &AndroidQmlPreviewWorker::startLogcat);

    connect(this, &RunWorker::stopped, &m_logcatProcess, &Utils::QtcProcess::stopProcess);
    m_logcatProcess.setStdOutCallback([this](const QString &stdOut) {
        filterLogcatAndAppendMessage(stdOut);
    });
}

AndroidQmlPreviewWorker::~AndroidQmlPreviewWorker()
{
    m_pidFutureWatcher.cancel();
    m_pidFutureWatcher.waitForFinished();
}

void AndroidQmlPreviewWorker::start()
{
    const SdkToolResult dateResult = runAdbCommand({"shell", "date", "+%s"});
    if (dateResult.success()) {
        m_logcatStartTimeStamp = QDateTime::fromSecsSinceEpoch(dateResult.stdOut().toInt())
                                                        .toString("MM-dd hh:mm:ss.mmm");
    }
    const bool previewStarted = ensureAvdIsRunning()
                                && checkAndInstallPreviewApp()
                                && preparePreviewArtefacts()
                                && uploadPreviewArtefacts()
                                && startPreviewApp();

    previewStarted ? reportStarted() : reportStopped();
}

void AndroidQmlPreviewWorker::stop()
{
    if (!isPreviewRunning(m_viewerPid) || stopPreviewApp())
        appendMessage(tr("%1 has been stopped.").arg(apkInfo()->name), NormalMessageFormat);
    m_viewerPid = -1;
    reportStopped();
}

bool AndroidQmlPreviewWorker::ensureAvdIsRunning()
{
    AndroidAvdManager avdMananager(m_androidConfig);
    QString devSN = AndroidManager::deviceSerialNumber(m_rc->target());

    if (devSN.isEmpty())
        devSN = m_serialNumber;

    if (!avdMananager.isAvdBooted(devSN)) {
        using namespace ProjectExplorer;
        const IDevice *dev = DeviceKitAspect::device(m_rc->target()->kit()).data();
        if (!dev) {
            appendMessage(tr("Selected device is invalid."), ErrorMessageFormat);
            return false;
        }
        if (dev->deviceState() == IDevice::DeviceDisconnected) {
            appendMessage(tr("Selected device is disconnected."), ErrorMessageFormat);
            return false;
        }
        AndroidDeviceInfo devInfoLocal = AndroidDevice::androidDeviceInfoFromIDevice(dev);

        if (devInfoLocal.isValid()) {
            if (dev->machineType() == IDevice::Emulator) {
                appendMessage(tr("Launching AVD."), NormalMessageFormat);
                devInfoLocal.serialNumber = avdMananager.startAvd(devInfoLocal.avdName);
            }
            if (devInfoLocal.serialNumber.isEmpty()) {
                appendMessage(tr("Could not start AVD."), ErrorMessageFormat);
            } else {
                m_serialNumber = devInfoLocal.serialNumber;
                m_avdAbis = m_androidConfig.getAbis(m_serialNumber);
            }
            return !devInfoLocal.serialNumber.isEmpty();
        } else {
            appendMessage(tr("No valid AVD has been selected."), ErrorMessageFormat);
        }
        return false;
    }
    m_avdAbis = m_androidConfig.getAbis(m_serialNumber);
    return true;
}

bool AndroidQmlPreviewWorker::checkAndInstallPreviewApp()
{
    const QStringList command {"pm", "list", "packages", apkInfo()->appId};
    appendMessage(tr("Checking if %1 app is installed.").arg(apkInfo()->name), NormalMessageFormat);
    const SdkToolResult res = runAdbShellCommand(command);
    if (!res.success()) {
        appendMessage(res.stdErr(), ErrorMessageFormat);
        return false;
    }

    if (res.stdOut().isEmpty()) {
        if (m_avdAbis.isEmpty()) {
            appendMessage(tr("ABI of the selected device is unknown. Cannot install APK."),
                          ErrorMessageFormat);
            return false;
        }
        const FilePath apkPath = designViewerApkPath(m_avdAbis.first());
        if (!apkPath.exists()) {
            appendMessage(tr("Cannot install %1 app for %2 architecture. "
                             "The appropriate APK was not found in resources folders.").
                          arg(apkInfo()->name, m_avdAbis.first()), ErrorMessageFormat);
            return false;
        }

        appendMessage(tr("Installing %1 APK.").arg(apkInfo()->name), NormalMessageFormat);

        const SdkToolResult res = runAdbCommand({"install", apkPath.toString()});
        if (!res.success())
            appendMessage(res.stdErr(), StdErrFormat);
    }

    return res.success();
}

bool AndroidQmlPreviewWorker::preparePreviewArtefacts()
{
    if (m_rc->project()->id() == QmlProjectManager::Constants::QML_PROJECT_ID) {
        const auto bs = m_rc->target()->buildSystem();
        if (bs) {
            m_uploadInfo.uploadPackage = FilePath::fromString(
                bs->additionalData(QmlProjectManager::Constants::mainFilePath).toString());
            m_uploadInfo.projectFolder = bs->projectDirectory();
            return true;
        }
    } else {
        const FilePaths allFiles = m_rc->project()->files(m_rc->project()->SourceFiles);
        const FilePaths filesToExport = Utils::filtered(allFiles,[](const FilePath &path) {
            return path.suffix() == "qmlproject";
        });

        if (filesToExport.size() > 1) {
            appendMessage(tr("Too many .qmlproject files in your project. Open directly the "
                             ".qmlproject file you want to work with and then run the preview."),
                          ErrorMessageFormat);
        } else if (filesToExport.size() < 1) {
            appendMessage(tr("No .qmlproject file found among project files."), ErrorMessageFormat);
        } else {
            const FilePath qmlprojectFile = filesToExport.first();
            m_uploadInfo.uploadPackage = m_uploadInfo.projectFolder.resolvePath(
                                                                        qmlprojectFile.fileName());
            m_uploadInfo.projectFolder = qmlprojectFile.parentDir();
            return true;
        }
    }
    appendMessage(tr("Could not gather information on project files."), ErrorMessageFormat);
    return false;
}

FilePath AndroidQmlPreviewWorker::createQmlrcFile(const FilePath &workFolder,
                                                  const QString &basename)
{
    const QtSupport::QtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(m_rc->kit());
    const FilePath rccBinary = qtVersion->rccFilePath();
    QtcProcess rccProcess;
    FilePath qrcPath = FilePath::fromString(basename) + ".qrc4viewer";
    const FilePath qmlrcPath = FilePath::fromString(QDir::tempPath()) / basename + packageSuffix;

    rccProcess.setWorkingDirectory(workFolder);

    const QStringList arguments[2] = {{"--project", "--output", qrcPath.fileName()},
                                      {"--binary", "--output", qmlrcPath.path(),
                                       qrcPath.fileName()}};
    for (const QStringList &args : arguments) {
        rccProcess.setCommand({rccBinary, args});
        rccProcess.start();
        if (!rccProcess.waitForStarted()) {
            appendMessage(tr("Could not create file for %1 \"%2\"").
                          arg(apkInfo()->name, rccProcess.commandLine().toUserOutput()),
                          StdErrFormat);
            qrcPath.removeFile();
            return {};
        }
        QByteArray stdOut;
        QByteArray stdErr;
        if (!rccProcess.readDataFromProcess(30, &stdOut, &stdErr, true)) {
            rccProcess.stopProcess();
            appendMessage(tr("A timeout occurred running \"%1\"").
                          arg(rccProcess.commandLine().toUserOutput()), StdErrFormat);
            qrcPath.removeFile();
            return {};
        }
        if (!stdOut.trimmed().isEmpty())
            appendMessage(QString::fromLocal8Bit(stdOut), StdErrFormat);

        if (!stdErr.trimmed().isEmpty())
            appendMessage(QString::fromLocal8Bit(stdErr), StdErrFormat);

        if (rccProcess.exitStatus() != QProcess::NormalExit) {
            appendMessage(tr("Crash while creating file for %1 \"%2\"").
                          arg(apkInfo()->name, rccProcess.commandLine().toUserOutput()),
                          StdErrFormat);
            qrcPath.removeFile();
            return {};
        }
        if (rccProcess.exitCode() != 0) {
            appendMessage(tr("Creating file for %1 failed. \"%2\" (exit code %3).").
                          arg(apkInfo()->name).
                          arg(rccProcess.commandLine().toUserOutput()).
                          arg(rccProcess.exitCode()),

                          StdErrFormat);
            qrcPath.removeFile();
            return {};
        }
    }
    return qmlrcPath;
}

bool AndroidQmlPreviewWorker::uploadPreviewArtefacts()
{
    appendMessage(tr("Uploading files."), NormalMessageFormat);
    const FilePath qresPath = createQmlrcFile(m_uploadInfo.projectFolder,
                                              m_uploadInfo.uploadPackage.baseName());
    if (!qresPath.exists())
        return false;

    runAdbShellCommand({"mkdir", "-p", apkInfo()->uploadDir});
    const SdkToolResult res = runAdbCommand({"push", qresPath.resolvePath(QString()).toString(),
                                             apkInfo()->uploadDir});
    if (!res.success()) {
        appendMessage(res.stdOut(), ErrorMessageFormat);
        if (res.stdOut().contains("Permission denied")) {
            appendMessage("'Permission denied' error detected. Try restarting your device "
                          "and then running the preview.", NormalMessageFormat);
        }
    }
    qresPath.removeFile();
    return res.success();
}

bool AndroidQmlPreviewWorker::startPreviewApp()
{
    stopPreviewApp();
    appendMessage(tr("Starting %1.").arg(apkInfo()->name), NormalMessageFormat);
    const QDir destDir(apkInfo()->uploadDir);
    const QString qmlrcPath = destDir.filePath(m_uploadInfo.uploadPackage.baseName()
                                               + packageSuffix);
    const QStringList envVars = m_rc->aspect<EnvironmentAspect>()->environment().toStringList();

    const QStringList command {
        "am", "start",
        "-n", apkInfo()->activityId,
        "-e", "extraappparams", QLatin1String(qmlrcPath.toUtf8().toBase64()),
        "-e", "extraenvvars", QLatin1String(envVars.join('\t').toUtf8().toBase64())
    };
    const SdkToolResult result = runAdbShellCommand(command);
    if (result.success())
        appendMessage(tr("%1 is running.").arg(apkInfo()->name), NormalMessageFormat);
    else
        appendMessage(result.stdErr(), ErrorMessageFormat);

    return result.success();
}

bool AndroidQmlPreviewWorker::stopPreviewApp()
{
    const QStringList command{"am", "force-stop", apkInfo()->appId};
    const SdkToolResult res = runAdbShellCommand(command);
    if (!res.success())
        appendMessage(res.stdErr(), ErrorMessageFormat);
    return res.success();
}

} // namespace Internal
} // namespace Android
