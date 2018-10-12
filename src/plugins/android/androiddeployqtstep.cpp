/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androiddeployqtstep.h"
#include "androiddeployqtwidget.h"
#include "androidqtsupport.h"
#include "certificatesmodel.h"

#include "javaparser.h"
#include "androidmanager.h"
#include "androidconstants.h"
#include "androidglobal.h"
#include "androidavdmanager.h"

#include <coreplugin/fileutils.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>

#include <QInputDialog>
#include <QMessageBox>
#include <QLoggingCategory>


using namespace ProjectExplorer;
using namespace Android;
using namespace Android::Internal;

namespace {
Q_LOGGING_CATEGORY(deployStepLog, "qtc.android.build.androiddeployqtstep", QtWarningMsg)
}

const QLatin1String UninstallPreviousPackageKey("UninstallPreviousPackage");
const QLatin1String InstallFailedInconsistentCertificatesString("INSTALL_PARSE_FAILED_INCONSISTENT_CERTIFICATES");
const QLatin1String InstallFailedUpdateIncompatible("INSTALL_FAILED_UPDATE_INCOMPATIBLE");
const QLatin1String InstallFailedPermissionModelDowngrade("INSTALL_FAILED_PERMISSION_MODEL_DOWNGRADE");
const QLatin1String InstallFailedVersionDowngrade("INSTALL_FAILED_VERSION_DOWNGRADE");
const Core::Id AndroidDeployQtStep::Id("Qt4ProjectManager.AndroidDeployQtStep");
static const char *qmlProjectRunConfigIdName = "QmlProjectManager.QmlRunConfiguration";


// AndroidDeployQtStepFactory

AndroidDeployQtStepFactory::AndroidDeployQtStepFactory()
{
    registerStep<AndroidDeployQtStep>(AndroidDeployQtStep::Id);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
    setSupportedDeviceType(Constants::ANDROID_DEVICE_TYPE);
    setRepeatable(false);
    setDisplayName(AndroidDeployQtStep::tr("Deploy to Android device or emulator"));
}

// AndroidDeployQtStep

AndroidDeployQtStep::AndroidDeployQtStep(ProjectExplorer::BuildStepList *parent)
    : ProjectExplorer::BuildStep(parent, Id)
{
    m_uninstallPreviousPackage = QtSupport::QtKitInformation::qtVersion(target()->kit())->qtVersion() < QtSupport::QtVersionNumber(5, 4, 0);

    //: AndroidDeployQtStep default display name
    setDefaultDisplayName(tr("Deploy to Android device"));

    connect(this, &AndroidDeployQtStep::askForUninstall,
            this, &AndroidDeployQtStep::slotAskForUninstall,
            Qt::BlockingQueuedConnection);

    connect(this, &AndroidDeployQtStep::setSerialNumber,
            this, &AndroidDeployQtStep::slotSetSerialNumber);
}

static AndroidDeviceInfo earlierDeviceInfo(QList<const ProjectExplorer::BuildStep *> &earlierSteps, Core::Id id)
{
    const ProjectExplorer::BuildStep *bs
            = Utils::findOrDefault(earlierSteps, Utils::equal(&ProjectExplorer::BuildStep::id, id));
    return bs ? static_cast<const AndroidDeployQtStep *>(bs)->deviceInfo() : AndroidDeviceInfo();
}

bool AndroidDeployQtStep::init(QList<const BuildStep *> &earlierSteps)
{
    Q_UNUSED(earlierSteps);
    m_androiddeployqtArgs.clear();

    m_targetArch = AndroidManager::targetArch(target());
    if (m_targetArch.isEmpty()) {
        emit addOutput(tr("No Android arch set by the .pro file."), OutputFormat::Stderr);
        return false;
    }

    emit addOutput(tr("initializing deployment to Android device/simulator"), OutputFormat::Stdout);

    RunConfiguration *rc = target()->activeRunConfiguration();
    QTC_ASSERT(rc, return false);
    const bool deployQtLive = rc->id().name().startsWith(qmlProjectRunConfigIdName);
    ProjectExplorer::BuildConfiguration *bc = buildConfiguration();
    QTC_ASSERT(deployQtLive || bc, return false);

    auto androidBuildApkStep = AndroidGlobal::buildStep<AndroidBuildApkStep>(bc);
    if (!androidBuildApkStep && !deployQtLive) {
        emit addOutput(tr("Cannot find the android build step."), OutputFormat::Stderr);
        return false;
    }

    int minTargetApi = AndroidManager::minimumSDK(target());
    qCDebug(deployStepLog) << "Target architecture:" << m_targetArch
                           << "Min target API" << minTargetApi;
    AndroidDeviceInfo info = earlierDeviceInfo(earlierSteps, Id);
    if (!info.isValid()) {
        info = AndroidConfigurations::showDeviceDialog(project(), minTargetApi, m_targetArch);
        m_deviceInfo = info; // Keep around for later steps
    }

    if (!info.isValid()) // aborted
        return false;

    m_avdName = info.avdname;
    m_serialNumber = info.serialNumber;
    qCDebug(deployStepLog) << "Selected Device:" << info;

    if (!deployQtLive)
        gatherFilesToPull();

    AndroidManager::setDeviceSerialNumber(target(), m_serialNumber);
    AndroidManager::setDeviceApiLevel(target(), info.sdk);

    emit addOutput(tr("Deploying to %1").arg(m_serialNumber), OutputFormat::Stdout);

    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target()->kit());
    if (!version)
        return false;

    m_uninstallPreviousPackageRun = m_uninstallPreviousPackage;
    if (m_uninstallPreviousPackageRun)
        m_manifestName = AndroidManager::manifestPath(target());

    AndroidQtSupport *qtSupport = AndroidManager::androidQtSupport(target());
    m_useAndroiddeployqt = !deployQtLive && qtSupport &&
            version->qtVersion() >= QtSupport::QtVersionNumber(5, 4, 0);

    if (m_useAndroiddeployqt) {
        m_command = version->qmakeProperty("QT_HOST_BINS");
        if (m_command.isEmpty()) {
            emit addOutput(tr("Cannot find the androiddeployqt tool."), OutputFormat::Stderr);
            return false;
        }
        qCDebug(deployStepLog) << "Using androiddeployqt";
        if (!m_command.endsWith(QLatin1Char('/')))
            m_command += QLatin1Char('/');
        m_command += Utils::HostOsInfo::withExecutableSuffix(QLatin1String("androiddeployqt"));

        m_workingDirectory = bc->buildDirectory().appendPath(QLatin1String(Constants::ANDROID_BUILDDIRECTORY)).toString();

        Utils::QtcProcess::addArg(&m_androiddeployqtArgs, QLatin1String("--verbose"));
        Utils::QtcProcess::addArg(&m_androiddeployqtArgs, QLatin1String("--output"));
        Utils::QtcProcess::addArg(&m_androiddeployqtArgs, m_workingDirectory);
        Utils::QtcProcess::addArg(&m_androiddeployqtArgs, QLatin1String("--no-build"));
        Utils::QtcProcess::addArg(&m_androiddeployqtArgs, QLatin1String("--input"));
        const QString jsonFile = qtSupport->targetData(Constants::AndroidDeploySettingsFile, target()).toString();
        if (jsonFile.isEmpty()) {
            emit addOutput(tr("Cannot find the androiddeploy Json file."), OutputFormat::Stderr);
            return false;
        }
        Utils::QtcProcess::addArg(&m_androiddeployqtArgs, jsonFile);
        if (androidBuildApkStep && androidBuildApkStep->useMinistro()) {
            qCDebug(deployStepLog) << "Using ministro";
            Utils::QtcProcess::addArg(&m_androiddeployqtArgs, QLatin1String("--deployment"));
            Utils::QtcProcess::addArg(&m_androiddeployqtArgs, QLatin1String("ministro"));
        }

        Utils::QtcProcess::addArg(&m_androiddeployqtArgs, QLatin1String("--gradle"));

        if (androidBuildApkStep && androidBuildApkStep->signPackage()) {
            // The androiddeployqt tool is not really written to do stand-alone installations.
            // This hack forces it to use the correct filename for the apk file when installing
            // as a temporary fix until androiddeployqt gets the support. Since the --sign is
            // only used to get the correct file name of the apk, its parameters are ignored.
            Utils::QtcProcess::addArg(&m_androiddeployqtArgs, QLatin1String("--sign"));
            Utils::QtcProcess::addArg(&m_androiddeployqtArgs, QLatin1String("foo"));
            Utils::QtcProcess::addArg(&m_androiddeployqtArgs, QLatin1String("bar"));
        }
    } else {
        m_uninstallPreviousPackageRun = true;
        m_command = AndroidConfigurations::currentConfig().adbToolPath().toString();
        const AndroidConfig &config = AndroidConfigurations::currentConfig();
        m_apkPath = deployQtLive ? config.qtLiveApkPath() : AndroidManager::apkPath(target());
        m_workingDirectory = bc ? bc->buildDirectory().toString() : QString();
    }
    m_environment = bc ? bc->environment() : Utils::Environment();

    m_adbPath = AndroidConfigurations::currentConfig().adbToolPath().toString();

    AndroidAvdManager avdManager;
    // Start the AVD if not running.
    if (!m_avdName.isEmpty() && avdManager.findAvd(m_avdName).isEmpty())
        avdManager.startAvdAsync(m_avdName);
    return true;
}

AndroidDeployQtStep::DeployErrorCode AndroidDeployQtStep::runDeploy(QFutureInterface<bool> &fi)
{
    QString args;
    if (m_useAndroiddeployqt) {
        args = m_androiddeployqtArgs;
        if (m_uninstallPreviousPackageRun)
            Utils::QtcProcess::addArg(&args, QLatin1String("--install"));
        else
            Utils::QtcProcess::addArg(&args, QLatin1String("--reinstall"));

        if (!m_serialNumber.isEmpty() && !m_serialNumber.startsWith(QLatin1String("????"))) {
            Utils::QtcProcess::addArg(&args, QLatin1String("--device"));
            Utils::QtcProcess::addArg(&args, m_serialNumber);
        }
    } else {
        RunConfiguration *rc = target()->activeRunConfiguration();
        QTC_ASSERT(rc, return DeployErrorCode::Failure);
        const bool deployQtLive = rc->id().name().startsWith(qmlProjectRunConfigIdName);
        QString packageName;
        if (deployQtLive) {
            AndroidManager::apkInfo(m_apkPath, &packageName);
            if (AndroidManager::packageInstalled(m_serialNumber, packageName))
                return DeployErrorCode::NoError;
        }

        if (m_uninstallPreviousPackageRun) {
            if (!deployQtLive)
                packageName = AndroidManager::packageName(m_manifestName);
            if (packageName.isEmpty()) {
                emit addOutput(tr("Cannot find the package name."), OutputFormat::Stderr);
                return Failure;
            }
            qCDebug(deployStepLog) << "Uninstalling previous package";
            emit addOutput(tr("Uninstall previous package %1.").arg(packageName), OutputFormat::NormalMessage);
            runCommand(m_adbPath,
                       AndroidDeviceInfo::adbSelector(m_serialNumber)
                       << QLatin1String("uninstall") << packageName);
        }

        foreach (const QString &arg, AndroidDeviceInfo::adbSelector(m_serialNumber))
            Utils::QtcProcess::addArg(&args, arg);

        Utils::QtcProcess::addArg(&args, QLatin1String("install"));
        Utils::QtcProcess::addArg(&args, QLatin1String("-r"));
        Utils::QtcProcess::addArg(&args, m_apkPath.toString());
    }

    m_process = new Utils::QtcProcess;
    m_process->setCommand(m_command, args);
    m_process->setWorkingDirectory(m_workingDirectory);
    m_process->setEnvironment(m_environment);

    if (Utils::HostOsInfo::isWindowsHost())
        m_process->setUseCtrlCStub(true);

    DeployErrorCode deployError = NoError;
    connect(m_process, &Utils::QtcProcess::readyReadStandardOutput,
            std::bind(&AndroidDeployQtStep::processReadyReadStdOutput, this, std::ref(deployError)));
    connect(m_process, &Utils::QtcProcess::readyReadStandardError,
            std::bind(&AndroidDeployQtStep::processReadyReadStdError, this, std::ref(deployError)));

    m_process->start();

    emit addOutput(tr("Starting: \"%1\" %2")
                   .arg(QDir::toNativeSeparators(m_command), args),
                   BuildStep::OutputFormat::NormalMessage);

    while (!m_process->waitForFinished(200)) {
        if (m_process->state() == QProcess::NotRunning)
            break;

        if (fi.isCanceled()) {
            m_process->kill();
            m_process->waitForFinished();
        }
    }

    QString line = QString::fromLocal8Bit(m_process->readAllStandardError());
    if (!line.isEmpty()) {
        deployError |= parseDeployErrors(line);
        stdError(line);
    }

    line = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    if (!line.isEmpty()) {
        deployError |= parseDeployErrors(line);
        stdOutput(line);
    }

    QProcess::ExitStatus exitStatus = m_process->exitStatus();
    int exitCode = m_process->exitCode();
    delete m_process;
    m_process = nullptr;

    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        emit addOutput(tr("The process \"%1\" exited normally.").arg(m_command),
                       BuildStep::OutputFormat::NormalMessage);
    } else if (exitStatus == QProcess::NormalExit) {
        emit addOutput(tr("The process \"%1\" exited with code %2.")
                       .arg(m_command, QString::number(exitCode)),
                       BuildStep::OutputFormat::ErrorMessage);
    } else {
        emit addOutput(tr("The process \"%1\" crashed.").arg(m_command), BuildStep::OutputFormat::ErrorMessage);
    }

    if (deployError != NoError) {
        if (m_uninstallPreviousPackageRun)
            deployError = Failure; // Even re-install failed. Set to Failure.
    } else if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
        // Set the deployError to Failure when no deployError code was detected
        // but the adb tool failed otherwise relay the detected deployError.
        deployError = Failure;
    }

    return deployError;
}

void AndroidDeployQtStep::slotAskForUninstall(DeployErrorCode errorCode)
{
    Q_ASSERT(errorCode > 0);

    QString uninstallMsg = tr("Deployment failed with the following errors:\n\n");
    uint errorCodeFlags = errorCode;
    uint mask = 1;
    while (errorCodeFlags) {
      switch (errorCodeFlags & mask) {
      case DeployErrorCode::PermissionModelDowngrade:
          uninstallMsg += InstallFailedPermissionModelDowngrade+"\n";
          break;
      case InconsistentCertificates:
          uninstallMsg += InstallFailedInconsistentCertificatesString+"\n";
          break;
      case UpdateIncompatible:
          uninstallMsg += InstallFailedUpdateIncompatible+"\n";
          break;
      case VersionDowngrade:
          uninstallMsg += InstallFailedVersionDowngrade+"\n";
          break;
      default:
          break;
      }
      errorCodeFlags &= ~mask;
      mask <<= 1;
    }

    uninstallMsg.append(tr("\nUninstalling the installed package may solve the issue.\nDo you want to uninstall the existing package?"));
    int button = QMessageBox::critical(nullptr, tr("Install failed"), uninstallMsg,
                                       QMessageBox::Yes, QMessageBox::No);
    m_askForUninstall = button == QMessageBox::Yes;
}

void AndroidDeployQtStep::slotSetSerialNumber(const QString &serialNumber)
{
    qCDebug(deployStepLog) << "Target device serial number change:" << serialNumber;
    AndroidManager::setDeviceSerialNumber(target(), serialNumber);
}

void AndroidDeployQtStep::run(QFutureInterface<bool> &fi)
{
    if (!m_avdName.isEmpty()) {
        QString serialNumber = AndroidAvdManager().waitForAvd(m_avdName, fi);
        qCDebug(deployStepLog) << "Deploying to AVD:" << m_avdName << serialNumber;
        if (serialNumber.isEmpty()) {
            reportRunResult(fi, false);
            return;
        }
        m_serialNumber = serialNumber;
        emit setSerialNumber(serialNumber);
    }

    DeployErrorCode returnValue = runDeploy(fi);
    if (returnValue > DeployErrorCode::NoError && returnValue < DeployErrorCode::Failure) {
        emit askForUninstall(returnValue);
        if (m_askForUninstall) {
            m_uninstallPreviousPackageRun = true;
            returnValue = runDeploy(fi);
        }
    }

    if (!m_filesToPull.isEmpty())
        emit addOutput(tr("Pulling files necessary for debugging."), OutputFormat::NormalMessage);

    for (auto itr = m_filesToPull.constBegin(); itr != m_filesToPull.constEnd(); ++itr) {
        QFile::remove(itr.value());
        runCommand(m_adbPath,
                   AndroidDeviceInfo::adbSelector(m_serialNumber)
                   << "pull" << itr.key() << itr.value());
        if (!QFileInfo::exists(itr.value())) {
            emit addOutput(tr("Package deploy: Failed to pull \"%1\" to \"%2\".")
                           .arg(itr.key())
                           .arg(itr.value()), OutputFormat::ErrorMessage);
        }
    }

    reportRunResult(fi, returnValue == NoError);
}

void AndroidDeployQtStep::gatherFilesToPull()
{
    m_filesToPull.clear();
    ProjectExplorer::BuildConfiguration *bc = buildConfiguration();
    QString buildDir = bc ? bc->buildDirectory().toString() : QString();
    if (bc && !buildDir.endsWith("/")) {
        buildDir += "/";
    }

    if (!m_deviceInfo.isValid())
        return;

    QString linkerName("linker");
    QString libDirName("lib");
    if (m_deviceInfo.cpuAbi.contains(QLatin1String("arm64-v8a")) ||
            m_deviceInfo.cpuAbi.contains(QLatin1String("x86_64"))) {
        const Core::Id cxxLanguageId = ProjectExplorer::Constants::CXX_LANGUAGE_ID;
        ToolChain *tc = ToolChainKitInformation::toolChain(target()->kit(), cxxLanguageId);
        if (tc && tc->targetAbi().wordWidth() == 64) {
            m_filesToPull["/system/bin/app_process64"] = buildDir + "app_process";
            libDirName = "lib64";
            linkerName = "linker64";
        } else {
            m_filesToPull["/system/bin/app_process32"] = buildDir + "app_process";
        }
    } else {
        m_filesToPull["/system/bin/app_process32"] = buildDir + "app_process";
        m_filesToPull["/system/bin/app_process"] = buildDir + "app_process";
    }

    m_filesToPull["/system/bin/" + linkerName] = buildDir + linkerName;
    m_filesToPull["/system/" + libDirName + "/libc.so"] = buildDir + "libc.so";

    qCDebug(deployStepLog) << "Files to pull from device:";
    for (auto itr = m_filesToPull.constBegin(); itr != m_filesToPull.constEnd(); ++itr)
        qCDebug(deployStepLog) << itr.key() << "to" << itr.value();
}

void AndroidDeployQtStep::runCommand(const QString &program, const QStringList &arguments)
{
    Utils::SynchronousProcess buildProc;
    buildProc.setTimeoutS(2 * 60);
    emit addOutput(tr("Package deploy: Running command \"%1 %2\".").arg(program).arg(arguments.join(QLatin1Char(' '))), BuildStep::OutputFormat::NormalMessage);
    Utils::SynchronousProcessResponse response = buildProc.run(program, arguments);
    if (response.result != Utils::SynchronousProcessResponse::Finished || response.exitCode != 0)
        emit addOutput(response.exitMessage(program, 2 * 60), BuildStep::OutputFormat::ErrorMessage);
}

AndroidDeviceInfo AndroidDeployQtStep::deviceInfo() const
{
    return m_deviceInfo;
}

ProjectExplorer::BuildStepConfigWidget *AndroidDeployQtStep::createConfigWidget()
{
    return new AndroidDeployQtWidget(this);
}

void AndroidDeployQtStep::processReadyReadStdOutput(DeployErrorCode &errorCode)
{
    m_process->setReadChannel(QProcess::StandardOutput);
    while (m_process->canReadLine()) {
        QString line = QString::fromLocal8Bit(m_process->readLine());
        errorCode |= parseDeployErrors(line);
        stdOutput(line);
    }
}

void AndroidDeployQtStep::stdOutput(const QString &line)
{
    emit addOutput(line, BuildStep::OutputFormat::Stdout, BuildStep::DontAppendNewline);
}

void AndroidDeployQtStep::processReadyReadStdError(DeployErrorCode &errorCode)
{
    m_process->setReadChannel(QProcess::StandardError);
    while (m_process->canReadLine()) {
        QString line = QString::fromLocal8Bit(m_process->readLine());
        errorCode |= parseDeployErrors(line);
        stdError(line);
    }
}

void AndroidDeployQtStep::stdError(const QString &line)
{
    emit addOutput(line, BuildStep::OutputFormat::Stderr, BuildStep::DontAppendNewline);
}

AndroidDeployQtStep::DeployErrorCode AndroidDeployQtStep::parseDeployErrors(QString &deployOutputLine) const
{
    DeployErrorCode errorCode = NoError;

    if (deployOutputLine.contains(InstallFailedInconsistentCertificatesString))
        errorCode |= InconsistentCertificates;
    if (deployOutputLine.contains(InstallFailedUpdateIncompatible))
        errorCode |= UpdateIncompatible;
    if (deployOutputLine.contains(InstallFailedPermissionModelDowngrade))
        errorCode |= PermissionModelDowngrade;
    if (deployOutputLine.contains(InstallFailedVersionDowngrade))
        errorCode |= VersionDowngrade;

    return errorCode;
}

bool AndroidDeployQtStep::fromMap(const QVariantMap &map)
{
    m_uninstallPreviousPackage = map.value(UninstallPreviousPackageKey, m_uninstallPreviousPackage).toBool();
    return ProjectExplorer::BuildStep::fromMap(map);
}

QVariantMap AndroidDeployQtStep::toMap() const
{
    QVariantMap map = ProjectExplorer::BuildStep::toMap();
    map.insert(UninstallPreviousPackageKey, m_uninstallPreviousPackage);
    return map;
}

void AndroidDeployQtStep::setUninstallPreviousPackage(bool uninstall)
{
    m_uninstallPreviousPackage = uninstall;
}

bool AndroidDeployQtStep::runInGuiThread() const
{
    return false;
}

AndroidDeployQtStep::UninstallType AndroidDeployQtStep::uninstallPreviousPackage()
{
    if (QtSupport::QtKitInformation::qtVersion(target()->kit())->qtVersion() < QtSupport::QtVersionNumber(5, 4, 0))
        return ForceUnintall;
    return m_uninstallPreviousPackage ? Uninstall : Keep;
}
