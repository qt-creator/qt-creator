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

#include "androidavdmanager.h"
#include "androiddevice.h"
#include "androiddeviceinfo.h"
#include "androidglobal.h"
#include "androidmanager.h"
#include "androidqmlpreviewworker.h"

#include <coreplugin/icore.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qmlprojectmanager/qmlprojectconstants.h>
#include <qmlprojectmanager/qmlprojectmanagerconstants.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>

#include <QThread>
#include <QTemporaryDir>
#include <QImageReader>
#include <QtConcurrent>

namespace Android {
namespace Internal {

using namespace Utils;

#define APP_ID "io.qt.designviewer"

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
    activityId(APP_ID "/org.qtproject.qt5.android.bindings.QtActivity"),
    name("Qt Design Viewer")
{
}

Q_GLOBAL_STATIC(ApkInfo, apkInfo)

const char packageSuffix[] = ".qmlrc";

static inline bool isMainThread()
{
    return QCoreApplication::instance()->thread() == QThread::currentThread();
}

static FilePath viewerApkPath(const QString &avdAbi)
{
    if (avdAbi.isEmpty())
        return {};

    if (apkInfo()->abis.contains(avdAbi))
        return Core::ICore::resourcePath(QString("android/qtdesignviewer/designviewer_%1.apk").
                                         arg(avdAbi));
    return {};
}

static SdkToolResult runAdbCommandAsyncAndWait(const QString &dev, const QStringList &arguments)
{
    QStringList args;
    if (!dev.isEmpty())
        args << AndroidDeviceInfo::adbSelector(dev);
    args << arguments;
    QFuture<SdkToolResult> asyncResult = QtConcurrent::run([args] {
        return AndroidManager::runAdbCommand(args);});

    while (asyncResult.isRunning()) {
        QCoreApplication::instance()->processEvents(QEventLoop::AllEvents, 100);
    }
    return asyncResult.result();
}

static SdkToolResult runAdbCommand(const QString &dev, const QStringList &arguments)
{
    if (isMainThread())
        return runAdbCommandAsyncAndWait(dev, arguments);
    QStringList args;
    if (!dev.isEmpty())
        args << AndroidDeviceInfo::adbSelector(dev);
    args << arguments;
    return AndroidManager::runAdbCommand(args);
}

static SdkToolResult runAdbShellCommand(const QString &dev, const QStringList &arguments)
{
    const QStringList shellCmd{"shell"};
    return runAdbCommand(dev, shellCmd + arguments);
}

static QString startAvd(const AndroidAvdManager &avd, const QString &name)
{
    QFuture<QString> asyncRes = QtConcurrent::run([avd, name] {
        return avd.startAvd(name);
    });
    while (asyncRes.isRunning())
        QCoreApplication::instance()->processEvents(QEventLoop::AllEvents, 100);
    return asyncRes.result();
}

static int pidofPreview(const QString &dev)
{
    const QStringList command{"pidof", apkInfo()->appId};
    const SdkToolResult res = runAdbShellCommand(dev, command);
    return res.success() ? res.stdOut().toInt() : -1;
}

static bool isPreviewRunning(const QString &dev, int lastKnownPid = -1)
{
    const int pid = pidofPreview(dev);
    return (lastKnownPid > 1) ? lastKnownPid == pid : pid > 1;
}

AndroidQmlPreviewWorker::AndroidQmlPreviewWorker(ProjectExplorer::RunControl *runControl)
    : ProjectExplorer::RunWorker(runControl)
    , m_rc(runControl)
    , m_config(AndroidConfigurations::currentConfig())
{
}

QStringList filterAppLog(const QStringList& oldList, const QStringList& newList)
{
    QStringList list = Utils::filtered(newList,
                                       [](const auto & arg){return arg.contains(apkInfo()->name);});
    for (const auto &oldEntry : oldList) {
        list.removeAll(oldEntry);
    }
    return list;
}

void AndroidQmlPreviewWorker::start()
{
    UploadInfo transfer;
    const bool res = ensureAvdIsRunning()
                     && checkAndInstallPreviewApp()
                     && prepareUpload(transfer)
                     && uploadFiles(transfer)
                     && runPreviewApp(transfer);

    if (!res) {
        reportFailure();
        return;
    }
    reportStarted();
    //Thread to monitor preview life
    QtConcurrent::run([this]() {
        QElapsedTimer timer;
        timer.start();
        while (runControl() && runControl()->isRunning()) {
            if (m_viewerPid == -1) {
                m_viewerPid = pidofPreview(m_devInfo.serialNumber);
                if (m_viewerPid > 0)
                    QMetaObject::invokeMethod(this, &AndroidQmlPreviewWorker::startLogcat);
            } else if (timer.elapsed() > 2000) {
                //Get the application output
                if (!isPreviewRunning(m_devInfo.serialNumber, m_viewerPid))
                    QMetaObject::invokeMethod(this, &AndroidQmlPreviewWorker::stop);

                timer.restart();
            }
            QThread::msleep(100);
        }
    });
}

void AndroidQmlPreviewWorker::startLogcat()
{
    QtConcurrent::run([this]() {
        QElapsedTimer timer;
        timer.start();
        int initialPid = m_viewerPid; // to check if our initial process is still alive
        QStringList logLines;
        auto appendLogLinesCall = [&logLines, this](){ appendLogLines(logLines); };
        auto runCondition = [this, initialPid](){ return (runControl() && runControl()->isRunning())
                                                          && initialPid == m_viewerPid;};
        QString timeFilter;
        while (runCondition()) {
            if (timer.elapsed() > 2000) {
                //Get the application output
                QStringList logcatCmd = {"logcat", QString("--pid=%1").arg(initialPid), "-t"};
                if (!timeFilter.isEmpty())
                    logcatCmd.append(QString("%1").arg(timeFilter));
                else
                    logcatCmd.append(QString("1000")); //show last 1000 lines (but for the 1st time)

                const SdkToolResult logcatResult = runAdbCommand(m_devInfo.serialNumber, logcatCmd);
                if (runCondition()) {
                    const QStringList output = logcatResult.stdOut().split('\n');
                    const QStringList filtered = filterAppLog(logLines, output);

                    if (!filtered.isEmpty()){
                        const QString lastLine = filtered.last();
                        timeFilter = lastLine.left(lastLine.indexOf(" ", lastLine.indexOf(" ") + 1));
                        QMetaObject::invokeMethod(this, appendLogLinesCall);
                        logLines = filtered;
                    }
                }
                timer.restart();
            }
            QThread::msleep(100);
        }
    });
}

void AndroidQmlPreviewWorker::stop()
{
    if (!isPreviewRunning(m_devInfo.serialNumber, m_viewerPid) || stopPreviewApp())
        appendMessage(tr("%1 has been stopped.").arg(apkInfo()->name), NormalMessageFormat);
    m_viewerPid = -1;
    reportStopped();
}

bool AndroidQmlPreviewWorker::ensureAvdIsRunning()
{
    AndroidAvdManager avdMan(m_config);
    QString devSN = AndroidManager::deviceSerialNumber(m_rc->target());

    if (devSN.isEmpty())
        devSN = m_devInfo.serialNumber;

    if (!avdMan.isAvdBooted(devSN)) {
        m_devInfo = {};
        int minTargetApi = AndroidManager::minimumSDK(m_rc->target());
        AndroidDeviceInfo devInfoLocal = AndroidConfigurations::showDeviceDialog(m_rc->project(),
                                                                                 minTargetApi,
                                                                                 apkInfo()->abis);
        if (devInfoLocal.isValid()) {
            if (devInfoLocal.type == AndroidDeviceInfo::Emulator) {
                appendMessage(tr("Launching AVD."), NormalMessageFormat);
                devInfoLocal.serialNumber = startAvd(avdMan, devInfoLocal.avdname);
            }
            if (devInfoLocal.serialNumber.isEmpty()) {
                appendMessage(tr("Could not run AVD."), ErrorMessageFormat);
            } else {
                m_devInfo = devInfoLocal;
                m_avdAbis = m_config.getAbis(m_config.adbToolPath(), m_devInfo.serialNumber);
            }
            return !devInfoLocal.serialNumber.isEmpty();
        } else {
            appendMessage(tr("No valid AVD has been selected."), ErrorMessageFormat);
        }
        return false;
    }
    m_avdAbis = m_config.getAbis(m_config.adbToolPath(), m_devInfo.serialNumber);
    return true;
}

bool AndroidQmlPreviewWorker::checkAndInstallPreviewApp()
{
    const QStringList command {"pm", "list", "packages", apkInfo()->appId};
    appendMessage(tr("Checking if %1 app is installed.").arg(apkInfo()->name), NormalMessageFormat);
    const SdkToolResult res = runAdbShellCommand(m_devInfo.serialNumber, command);
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
        const FilePath apkPath = viewerApkPath(m_avdAbis.first());
        if (!apkPath.exists()) {
            appendMessage(tr("Cannot install %1 app for %2 architecture. "
                             "The appropriate APK was not found in resources folders.").
                          arg(apkInfo()->name, m_avdAbis.first()), ErrorMessageFormat);
            return false;
        }

        appendMessage(tr("Installing %1 APK.").arg(apkInfo()->name), NormalMessageFormat);


        const SdkToolResult res = runAdbCommand(m_devInfo.serialNumber, {"install",
                                                                         apkPath.toString()});
        if (!res.success()) {
            appendMessage(res.stdErr(), StdErrFormat);

            return false;
        }
    }
    return true;
}

bool AndroidQmlPreviewWorker::prepareUpload(UploadInfo &transfer)
{
    if (m_rc->project()->id() == QmlProjectManager::Constants::QML_PROJECT_ID) {
        const auto bs = m_rc->target()->buildSystem();
        if (bs) {
            transfer.uploadPackage = FilePath::fromString(
                bs->additionalData(QmlProjectManager::Constants::mainFilePath).toString());
            transfer.projectFolder = bs->projectDirectory();
            return true;
        }
    } else {
        const FilePaths allFiles = m_rc->project()->files(m_rc->project()->SourceFiles);
        const FilePaths filesToExport = Utils::filtered(allFiles,[](const FilePath &path) {
            return path.suffix() == "qmlproject";});

        if (filesToExport.size() > 1) {
            appendMessage(tr("Too many .qmlproject files in your project. Open directly the "
                             ".qmlproject file you want to work with and then run the preview."),
                          ErrorMessageFormat);
        } else if (filesToExport.size() < 1) {
            appendMessage(tr("No .qmlproject file found among project files."), ErrorMessageFormat);
        } else {
            const FilePath qmlprojectFile = filesToExport.first();
            transfer.uploadPackage = transfer.
                                   projectFolder.
                                   resolvePath(qmlprojectFile.fileName());
            transfer.projectFolder = qmlprojectFile.parentDir();
            return true;
        }
    }
    appendMessage(tr("Could not gather information on project files."), ErrorMessageFormat);
    return false;
}

FilePath AndroidQmlPreviewWorker::createQmlrcFile(const FilePath &workFolder,
                                                  const QString &basename)
{
    const QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(m_rc->kit());
    const FilePath rccBinary = qtVersion->rccFilePath();
    QtcProcess rccProcess;
    FilePath qrcPath = FilePath::fromString(basename) + ".qrc4viewer";
    const FilePath qmlrcPath = FilePath::fromString(QDir::tempPath() + "/" + basename +
                                                    packageSuffix);

    rccProcess.setWorkingDirectory(workFolder);

    const QStringList arguments[2] = {{"--project", "--output", qrcPath.fileName()},
                                      {"--binary", "--output", qmlrcPath.path(),
                                       qrcPath.fileName()}};
    for (const auto &arguments : arguments) {
        rccProcess.setCommand({rccBinary, arguments});
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

bool AndroidQmlPreviewWorker::uploadFiles(const UploadInfo &transfer)
{
    appendMessage(tr("Uploading files."), NormalMessageFormat);

    const FilePath qresPath = createQmlrcFile(FilePath::fromString(transfer.projectFolder.path()),
                                              transfer.uploadPackage.baseName());
    if (!qresPath.exists())
        return false;

    runAdbShellCommand(m_devInfo.serialNumber, {"mkdir", "-p", apkInfo()->uploadDir});

    const SdkToolResult res = runAdbCommand(m_devInfo.serialNumber,
                                      {"push", qresPath.resolvePath(QString()).toString(),
                                       apkInfo()->uploadDir});
    if (!res.success()) {
        appendMessage(res.stdOut(), ErrorMessageFormat);
        if (res.stdOut().contains("Permission denied"))
            appendMessage("'Permission denied' error detected. Try restarting your device "
                          "and then running the preview.", NormalMessageFormat);
    }
    qresPath.removeFile();
    return res.success();
}

bool AndroidQmlPreviewWorker::runPreviewApp(const UploadInfo &transfer)
{
    stopPreviewApp();
    appendMessage(tr("Starting %1.").arg(apkInfo()->name), NormalMessageFormat);
    const QDir destDir(apkInfo()->uploadDir);
    const QStringList command{"am", "start",
                              "-n", apkInfo()->activityId,
                              "-e", "extraappparams",
                              QString::fromLatin1(
                                  destDir.filePath(transfer.uploadPackage.baseName() + packageSuffix).
                                  toUtf8().
                                  toBase64())};
    const SdkToolResult res = runAdbShellCommand(m_devInfo.serialNumber, command);
    if (!res.success()) {
        appendMessage(res.stdErr(), ErrorMessageFormat);
        return res.success();
    }
    appendMessage(tr("%1 is running.").arg(apkInfo()->name), NormalMessageFormat);
    m_viewerPid = pidofPreview(m_devInfo.serialNumber);
    return true;
}

bool AndroidQmlPreviewWorker::stopPreviewApp()
{
    const QStringList command{"am", "force-stop", apkInfo()->appId};
    const SdkToolResult res = runAdbShellCommand(m_devInfo.serialNumber, command);
    if (!res.success()) {
        appendMessage(res.stdErr(), ErrorMessageFormat);
        return res.success();
    }
    return true;
}

void AndroidQmlPreviewWorker::appendLogLines(const QStringList & lines)
{
    for (const QString& line : lines) {
        const int charsToSkip = apkInfo()->name.length() + 2; // strlen(": ") == 2
        const QString formatted = line.mid(line.indexOf(apkInfo()->name) + charsToSkip);
        // TODO: See AndroidRunnerWorker::logcatProcess() - filtering for logs to decide format.
        appendMessage(formatted, StdOutFormat);
    }
}

} // namespace Internal
} // namespace Android
