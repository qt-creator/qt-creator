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
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>

#include <QCheckBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QInputDialog>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

using namespace ProjectExplorer;
using namespace Utils;

namespace Android {
namespace Internal {

namespace {
static Q_LOGGING_CATEGORY(deployStepLog, "qtc.android.build.androiddeployqtstep", QtWarningMsg)
}

const QLatin1String UninstallPreviousPackageKey("UninstallPreviousPackage");
const QLatin1String InstallFailedInconsistentCertificatesString("INSTALL_PARSE_FAILED_INCONSISTENT_CERTIFICATES");
const QLatin1String InstallFailedUpdateIncompatible("INSTALL_FAILED_UPDATE_INCOMPATIBLE");
const QLatin1String InstallFailedPermissionModelDowngrade("INSTALL_FAILED_PERMISSION_MODEL_DOWNGRADE");
const QLatin1String InstallFailedVersionDowngrade("INSTALL_FAILED_VERSION_DOWNGRADE");

// AndroidDeployQtStepFactory

AndroidDeployQtStepFactory::AndroidDeployQtStepFactory()
{
    registerStep<AndroidDeployQtStep>(AndroidDeployQtStep::stepId());
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
    setSupportedDeviceType(Constants::ANDROID_DEVICE_TYPE);
    setRepeatable(false);
    setDisplayName(AndroidDeployQtStep::tr("Deploy to Android device or emulator"));
}

// AndroidDeployQtWidget

class AndroidDeployQtWidget : public BuildStepConfigWidget
{
public:
    AndroidDeployQtWidget(AndroidDeployQtStep *step)
        : ProjectExplorer::BuildStepConfigWidget(step)
    {
        setDisplayName(QString("<b>%1</b>").arg(step->displayName()));
        setSummaryText(displayName());

        auto uninstallPreviousPackage = new QCheckBox(this);
        uninstallPreviousPackage->setText(AndroidDeployQtStep::tr("Uninstall the existing app first"));
        uninstallPreviousPackage->setChecked(step->uninstallPreviousPackage() > AndroidDeployQtStep::Keep);
        uninstallPreviousPackage->setEnabled(step->uninstallPreviousPackage() != AndroidDeployQtStep::ForceUnintall);

        connect(uninstallPreviousPackage, &QAbstractButton::toggled,
                step, &AndroidDeployQtStep::setUninstallPreviousPackage);

        auto resetDefaultDevices = new QPushButton(this);
        resetDefaultDevices->setText(AndroidDeployQtStep::tr("Reset Default Deployment Devices"));

        connect(resetDefaultDevices, &QAbstractButton::clicked, this, [step] {
            AndroidConfigurations::clearDefaultDevices(step->project());
        });

        auto installCustomApkButton = new QPushButton(this);
        installCustomApkButton->setText(AndroidDeployQtStep::tr("Install an APK File"));

        connect(installCustomApkButton, &QAbstractButton::clicked, this, [this, step] {
            const QString packagePath
                = QFileDialog::getOpenFileName(this,
                                               AndroidDeployQtStep::tr("Qt Android Installer"),
                                               QDir::homePath(),
                                               AndroidDeployQtStep::tr("Android package (*.apk)"));
            if (!packagePath.isEmpty())
                AndroidManager::installQASIPackage(step->target(), packagePath);
        });

        auto layout = new QVBoxLayout(this);
        layout->addWidget(uninstallPreviousPackage);
        layout->addWidget(resetDefaultDevices);
        layout->addWidget(installCustomApkButton);
    }
};

// AndroidDeployQtStep

AndroidDeployQtStep::AndroidDeployQtStep(BuildStepList *parent, Utils::Id id)
    : BuildStep(parent, id)
{
    setImmutable(true);
    const QtSupport::BaseQtVersion * const qt = QtSupport::QtKitAspect::qtVersion(target()->kit());
    m_uninstallPreviousPackage = qt && qt->qtVersion() < QtSupport::QtVersionNumber(5, 4, 0);

    //: AndroidDeployQtStep default display name
    setDefaultDisplayName(tr("Deploy to Android device"));

    connect(this, &AndroidDeployQtStep::askForUninstall,
            this, &AndroidDeployQtStep::slotAskForUninstall,
            Qt::BlockingQueuedConnection);

    connect(this, &AndroidDeployQtStep::setSerialNumber,
            this, &AndroidDeployQtStep::slotSetSerialNumber);
}

Utils::Id AndroidDeployQtStep::stepId()
{
    return "Qt4ProjectManager.AndroidDeployQtStep";
}

bool AndroidDeployQtStep::init()
{
    QtSupport::BaseQtVersion *version = QtSupport::QtKitAspect::qtVersion(target()->kit());
    if (!version) {
        qCDebug(deployStepLog,
                "The Qt version for kit %s is not valid.",
                qPrintable(target()->kit()->displayName()));
        return false;
    }

    m_androiddeployqtArgs = CommandLine();

    m_androidABIs = AndroidManager::applicationAbis(target());
    if (m_androidABIs.isEmpty()) {
        emit addOutput(tr("No Android arch set by the .pro file."), OutputFormat::Stderr);
        return false;
    }

    emit addOutput(tr("Initializing deployment to Android device/simulator"), OutputFormat::Stdout);

    RunConfiguration *rc = target()->activeRunConfiguration();
    QTC_ASSERT(rc, return false);
    BuildConfiguration *bc = target()->activeBuildConfiguration();
    QTC_ASSERT(bc, return false);

    auto androidBuildApkStep = bc->buildSteps()->firstOfType<AndroidBuildApkStep>();
    int minTargetApi = AndroidManager::minimumSDK(target());
    qCDebug(deployStepLog) << "Target architecture:" << m_androidABIs
                           << "Min target API" << minTargetApi;

    // Try to re-use user-provided information from an earlier step of the same type.
    BuildStepList *bsl = stepList();
    QTC_ASSERT(bsl, return false);
    auto androidDeployQtStep = bsl->firstOfType<AndroidDeployQtStep>();
    QTC_ASSERT(androidDeployQtStep, return false);
    AndroidDeviceInfo info;
    if (androidDeployQtStep != this)
        info = androidDeployQtStep->m_deviceInfo;

    if (!info.isValid()) {
        info = AndroidConfigurations::showDeviceDialog(project(), minTargetApi, m_androidABIs);
        m_deviceInfo = info; // Keep around for later steps
    }

    if (!info.isValid()) // aborted
        return false;

    m_avdName = info.avdname;
    m_serialNumber = info.serialNumber;
    qCDebug(deployStepLog) << "Selected device info:" << info;

    gatherFilesToPull();

    AndroidManager::setDeviceSerialNumber(target(), m_serialNumber);
    AndroidManager::setDeviceApiLevel(target(), info.sdk);
    AndroidManager::setDeviceAbis(target(), info.cpuAbi);

    emit addOutput(tr("Deploying to %1").arg(m_serialNumber), OutputFormat::Stdout);

    m_uninstallPreviousPackageRun = m_uninstallPreviousPackage;
    if (m_uninstallPreviousPackageRun)
        m_manifestName = AndroidManager::manifestPath(target());

    m_useAndroiddeployqt = version->qtVersion() >= QtSupport::QtVersionNumber(5, 4, 0);

    if (m_useAndroiddeployqt) {
        const QString buildKey = target()->activeBuildKey();
        const ProjectNode *node = target()->project()->findNodeForBuildKey(buildKey);
        if (!node)
            return false;
        m_apkPath = Utils::FilePath::fromString(node->data(Constants::AndroidApk).toString());
        if (!m_apkPath.isEmpty()) {
            m_manifestName = Utils::FilePath::fromString(node->data(Constants::AndroidManifest).toString());
            m_command = AndroidConfigurations::currentConfig().adbToolPath();
            AndroidManager::setManifestPath(target(), m_manifestName);
        } else {
            QString jsonFile = node->data(Constants::AndroidDeploySettingsFile).toString();
            if (jsonFile.isEmpty()) {
                emit addOutput(tr("Cannot find the androiddeploy Json file."), OutputFormat::Stderr);
                return false;
            }
            m_command = version->hostBinPath();
            if (m_command.isEmpty()) {
                emit addOutput(tr("Cannot find the androiddeployqt tool."), OutputFormat::Stderr);
                return false;
            }
            m_command = m_command.pathAppended(HostOsInfo::withExecutableSuffix("androiddeployqt"));

            m_workingDirectory = bc->buildDirectory().pathAppended(Constants::ANDROID_BUILDDIRECTORY);

            m_androiddeployqtArgs.addArgs({"--verbose",
                                           "--output", m_workingDirectory.toString(),
                                           "--no-build",
                                           "--input", jsonFile});

            m_androiddeployqtArgs.addArg("--gradle");

            if (androidBuildApkStep && androidBuildApkStep->signPackage()) {
                // The androiddeployqt tool is not really written to do stand-alone installations.
                // This hack forces it to use the correct filename for the apk file when installing
                // as a temporary fix until androiddeployqt gets the support. Since the --sign is
                // only used to get the correct file name of the apk, its parameters are ignored.
                m_androiddeployqtArgs.addArgs({"--sign", "foo", "bar"});
            }
        }
    } else {
        m_uninstallPreviousPackageRun = true;
        m_command = AndroidConfigurations::currentConfig().adbToolPath();
        m_apkPath = AndroidManager::apkPath(target());
        m_workingDirectory = bc ? bc->buildDirectory() : FilePath();
    }
    m_environment = bc ? bc->environment() : Utils::Environment();

    m_adbPath = AndroidConfigurations::currentConfig().adbToolPath();

    AndroidAvdManager avdManager;
    // Start the AVD if not running.
    if (!m_avdName.isEmpty() && avdManager.findAvd(m_avdName).isEmpty())
        avdManager.startAvdAsync(m_avdName);
    return true;
}

AndroidDeployQtStep::DeployErrorCode AndroidDeployQtStep::runDeploy()
{
    CommandLine cmd(m_command);
    if (m_useAndroiddeployqt && m_apkPath.isEmpty()) {
        cmd.addArgs(m_androiddeployqtArgs.arguments(), CommandLine::Raw);
        if (m_uninstallPreviousPackageRun)
            cmd.addArg("--install");
        else
            cmd.addArg("--reinstall");

        if (!m_serialNumber.isEmpty() && !m_serialNumber.startsWith("????"))
            cmd.addArgs({"--device", m_serialNumber});

    } else {
        RunConfiguration *rc = target()->activeRunConfiguration();
        QTC_ASSERT(rc, return DeployErrorCode::Failure);
        QString packageName;

        if (m_uninstallPreviousPackageRun) {
            packageName = AndroidManager::packageName(m_manifestName);
            if (packageName.isEmpty()) {
                emit addOutput(tr("Cannot find the package name."), OutputFormat::Stderr);
                return Failure;
            }
            qCDebug(deployStepLog) << "Uninstalling previous package";
            emit addOutput(tr("Uninstall previous package %1.").arg(packageName), OutputFormat::NormalMessage);
            runCommand({m_adbPath,
                       AndroidDeviceInfo::adbSelector(m_serialNumber)
                       << "uninstall" << packageName});
        }

        cmd.addArgs(AndroidDeviceInfo::adbSelector(m_serialNumber));
        cmd.addArgs({"install", "-r", m_apkPath.toString()});
    }

    m_process = new Utils::QtcProcess;
    m_process->setCommand(cmd);
    m_process->setWorkingDirectory(m_workingDirectory.toString());
    m_process->setEnvironment(m_environment);

    if (Utils::HostOsInfo::isWindowsHost())
        m_process->setUseCtrlCStub(true);

    DeployErrorCode deployError = NoError;
    connect(m_process, &Utils::QtcProcess::readyReadStandardOutput,
            std::bind(&AndroidDeployQtStep::processReadyReadStdOutput, this, std::ref(deployError)));
    connect(m_process, &Utils::QtcProcess::readyReadStandardError,
            std::bind(&AndroidDeployQtStep::processReadyReadStdError, this, std::ref(deployError)));

    m_process->start();

    emit addOutput(tr("Starting: \"%1\"").arg(cmd.toUserOutput()),
                   BuildStep::OutputFormat::NormalMessage);

    while (!m_process->waitForFinished(200)) {
        if (m_process->state() == QProcess::NotRunning)
            break;

        if (isCanceled()) {
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
        emit addOutput(tr("The process \"%1\" exited normally.").arg(m_command.toUserOutput()),
                       BuildStep::OutputFormat::NormalMessage);
    } else if (exitStatus == QProcess::NormalExit) {
        emit addOutput(tr("The process \"%1\" exited with code %2.")
                       .arg(m_command.toUserOutput(), QString::number(exitCode)),
                       BuildStep::OutputFormat::ErrorMessage);
    } else {
        emit addOutput(tr("The process \"%1\" crashed.").arg(m_command.toUserOutput()),
                       BuildStep::OutputFormat::ErrorMessage);
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

bool AndroidDeployQtStep::runImpl()
{
    if (!m_avdName.isEmpty()) {
        QString serialNumber = AndroidAvdManager().waitForAvd(m_avdName, cancelChecker());
        qCDebug(deployStepLog) << "Deploying to AVD:" << m_avdName << serialNumber;
        if (serialNumber.isEmpty())
            return false;
        m_serialNumber = serialNumber;
        emit setSerialNumber(serialNumber);
    }

    DeployErrorCode returnValue = runDeploy();
    if (returnValue > DeployErrorCode::NoError && returnValue < DeployErrorCode::Failure) {
        emit askForUninstall(returnValue);
        if (m_askForUninstall) {
            m_uninstallPreviousPackageRun = true;
            returnValue = runDeploy();
        }
    }

    if (!m_filesToPull.isEmpty())
        emit addOutput(tr("Pulling files necessary for debugging."), OutputFormat::NormalMessage);

    // Note that values are not necessarily unique, e.g. app_process is looked up in several directories
    for (auto itr = m_filesToPull.constBegin(); itr != m_filesToPull.constEnd(); ++itr) {
        QFile::remove(itr.value());
    }

    for (auto itr = m_filesToPull.constBegin(); itr != m_filesToPull.constEnd(); ++itr) {
        runCommand({m_adbPath,
                   AndroidDeviceInfo::adbSelector(m_serialNumber)
                   << "pull" << itr.key() << itr.value()});
        if (!QFileInfo::exists(itr.value())) {
            emit addOutput(tr("Package deploy: Failed to pull \"%1\" to \"%2\".")
                           .arg(itr.key())
                           .arg(itr.value()), OutputFormat::ErrorMessage);
        }
    }

    return returnValue == NoError;
}

void AndroidDeployQtStep::gatherFilesToPull()
{
    m_filesToPull.clear();
    BuildConfiguration *bc = target()->activeBuildConfiguration();
    QString buildDir = bc ? bc->buildDirectory().toString() : QString();
    if (bc && !buildDir.endsWith("/")) {
        buildDir += "/";
    }

    if (!m_deviceInfo.isValid())
        return;

    QString linkerName("linker");
    QString libDirName("lib");
    auto preferreABI = AndroidManager::apkDevicePreferredAbi(target());
    if (preferreABI == "arm64-v8a" || preferreABI == "x86_64") {
        m_filesToPull["/system/bin/app_process64"] = buildDir + "app_process";
        libDirName = "lib64";
        linkerName = "linker64";
    } else {
        m_filesToPull["/system/bin/app_process32"] = buildDir + "app_process";
        m_filesToPull["/system/bin/app_process"] = buildDir + "app_process";
    }

    m_filesToPull["/system/bin/" + linkerName] = buildDir + linkerName;
    m_filesToPull["/system/" + libDirName + "/libc.so"] = buildDir + "libc.so";

    for (auto itr = m_filesToPull.constBegin(); itr != m_filesToPull.constEnd(); ++itr)
        qCDebug(deployStepLog) << "Pulling file from device:" << itr.key() << "to:" << itr.value();
}

void AndroidDeployQtStep::doRun()
{
    runInThread([this] { return runImpl(); });
}

void AndroidDeployQtStep::runCommand(const CommandLine &command)
{
    SynchronousProcess buildProc;
    buildProc.setTimeoutS(2 * 60);
    emit addOutput(tr("Package deploy: Running command \"%1\".").arg(command.toUserOutput()),
                   OutputFormat::NormalMessage);
    SynchronousProcessResponse response = buildProc.run(command);
    if (response.result != SynchronousProcessResponse::Finished || response.exitCode != 0)
        emit addOutput(response.exitMessage(command.executable().toString(), 2 * 60),
                       OutputFormat::ErrorMessage);
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

AndroidDeployQtStep::UninstallType AndroidDeployQtStep::uninstallPreviousPackage()
{
    const QtSupport::BaseQtVersion * const qt = QtSupport::QtKitAspect::qtVersion(target()->kit());
    if (qt && qt->qtVersion() < QtSupport::QtVersionNumber(5, 4, 0))
        return ForceUnintall;
    return m_uninstallPreviousPackage ? Uninstall : Keep;
}

} // Internal
} // Android
