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

#include <coreplugin/fileutils.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QInputDialog>
#include <QMessageBox>


using namespace Android;
using namespace Android::Internal;

const QLatin1String UninstallPreviousPackageKey("UninstallPreviousPackage");
const QLatin1String InstallFailedInconsistentCertificatesString("INSTALL_PARSE_FAILED_INCONSISTENT_CERTIFICATES");
const QLatin1String InstallFailedInconsistentCertificatesString2("INSTALL_FAILED_UPDATE_INCOMPATIBLE");
const Core::Id AndroidDeployQtStep::Id("Qt4ProjectManager.AndroidDeployQtStep");

//////////////////
// AndroidDeployQtStepFactory
/////////////////

AndroidDeployQtStepFactory::AndroidDeployQtStepFactory(QObject *parent)
    : IBuildStepFactory(parent)
{
}

QList<Core::Id> AndroidDeployQtStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
        return QList<Core::Id>();
    if (!AndroidManager::supportsAndroid(parent->target()))
        return QList<Core::Id>();
    if (parent->contains(AndroidDeployQtStep::Id))
        return QList<Core::Id>();
    return QList<Core::Id>() << AndroidDeployQtStep::Id;
}

QString AndroidDeployQtStepFactory::displayNameForId(Core::Id id) const
{
    if (id == AndroidDeployQtStep::Id)
        return tr("Deploy to Android device or emulator");
    return QString();
}

bool AndroidDeployQtStepFactory::canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

ProjectExplorer::BuildStep *AndroidDeployQtStepFactory::create(ProjectExplorer::BuildStepList *parent, Core::Id id)
{
    Q_ASSERT(canCreate(parent, id));
    Q_UNUSED(id);
    return new AndroidDeployQtStep(parent);
}

bool AndroidDeployQtStepFactory::canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::BuildStep *AndroidDeployQtStepFactory::restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map)
{
    Q_ASSERT(canRestore(parent, map));
    AndroidDeployQtStep * const step = new AndroidDeployQtStep(parent);
    if (!step->fromMap(map)) {
        delete step;
        return 0;
    }
    return step;
}

bool AndroidDeployQtStepFactory::canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const
{
    return canCreate(parent, product->id());
}

ProjectExplorer::BuildStep *AndroidDeployQtStepFactory::clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product)
{
    Q_ASSERT(canClone(parent, product));
    return new AndroidDeployQtStep(parent, static_cast<AndroidDeployQtStep *>(product));
}

//////////////////
// AndroidDeployQtStep
/////////////////

AndroidDeployQtStep::AndroidDeployQtStep(ProjectExplorer::BuildStepList *parent)
    : ProjectExplorer::BuildStep(parent, Id)
{
    ctor();
}

AndroidDeployQtStep::AndroidDeployQtStep(ProjectExplorer::BuildStepList *parent,
    AndroidDeployQtStep *other)
    : ProjectExplorer::BuildStep(parent, other)
{
    ctor();
}

void AndroidDeployQtStep::ctor()
{
    m_uninstallPreviousPackage = QtSupport::QtKitInformation::qtVersion(target()->kit())->qtVersion() < QtSupport::QtVersionNumber(5, 4, 0);
    m_uninstallPreviousPackageRun = false;

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

    if (AndroidManager::checkForQt51Files(project()->projectDirectory()))
        emit addOutput(tr("Found old folder \"android\" in source directory. Qt 5.2 does not use that folder by default."), ErrorOutput);

    m_targetArch = AndroidManager::targetArch(target());
    if (m_targetArch.isEmpty()) {
        emit addOutput(tr("No Android arch set by the .pro file."), ErrorOutput);
        return false;
    }

    AndroidBuildApkStep *androidBuildApkStep
        = AndroidGlobal::buildStep<AndroidBuildApkStep>(target()->activeBuildConfiguration());
    if (!androidBuildApkStep) {
        emit addOutput(tr("Cannot find the android build step."), ErrorOutput);
        return false;
    }

    int deviceAPILevel = AndroidManager::minimumSDK(target());
    AndroidConfigurations::Options options = AndroidConfigurations::None;
    if (androidBuildApkStep->deployAction() == AndroidBuildApkStep::DebugDeployment)
        options = AndroidConfigurations::FilterAndroid5;
    AndroidDeviceInfo info = earlierDeviceInfo(earlierSteps, Id);
    if (!info.isValid()) {
        info = AndroidConfigurations::showDeviceDialog(project(), deviceAPILevel, m_targetArch, options);
        m_deviceInfo = info; // Keep around for later steps
    }

    if (!info.isValid()) // aborted
        return false;

    m_avdName = info.avdname;
    m_serialNumber = info.serialNumber;

    m_appProcessBinaries.clear();
    m_libdir = QLatin1String("lib");
    if (info.cpuAbi.contains(QLatin1String("arm64-v8a")) ||
            info.cpuAbi.contains(QLatin1String("x86_64"))) {
        ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(target()->kit());
        if (tc && tc->targetAbi().wordWidth() == 64) {
            m_appProcessBinaries << QLatin1String("/system/bin/app_process64");
            m_libdir += QLatin1String("64");
        } else {
            m_appProcessBinaries << QLatin1String("/system/bin/app_process32");
        }
    } else {
        m_appProcessBinaries << QLatin1String("/system/bin/app_process32")
                             << QLatin1String("/system/bin/app_process");
    }

    AndroidManager::setDeviceSerialNumber(target(), m_serialNumber);

    ProjectExplorer::BuildConfiguration *bc = target()->activeBuildConfiguration();

    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target()->kit());
    if (!version)
        return false;

    m_uninstallPreviousPackageRun = m_uninstallPreviousPackage;
    if (m_uninstallPreviousPackageRun)
        m_manifestName = AndroidManager::manifestPath(target());

    m_useAndroiddeployqt = version->qtVersion() >= QtSupport::QtVersionNumber(5, 4, 0);
    if (m_useAndroiddeployqt) {
        Utils::FileName tmp = AndroidManager::androidQtSupport(target())->androiddeployqtPath(target());
        if (tmp.isEmpty()) {
            emit addOutput(tr("Cannot find the androiddeployqt tool."), ErrorOutput);
            return false;
        }

        m_command = tmp.toString();
        m_workingDirectory = bc->buildDirectory().appendPath(QLatin1String(Constants::ANDROID_BUILDDIRECTORY)).toString();

        Utils::QtcProcess::addArg(&m_androiddeployqtArgs, QLatin1String("--verbose"));
        Utils::QtcProcess::addArg(&m_androiddeployqtArgs, QLatin1String("--output"));
        Utils::QtcProcess::addArg(&m_androiddeployqtArgs, m_workingDirectory);
        Utils::QtcProcess::addArg(&m_androiddeployqtArgs, QLatin1String("--no-build"));
        Utils::QtcProcess::addArg(&m_androiddeployqtArgs, QLatin1String("--input"));
        tmp = AndroidManager::androidQtSupport(target())->androiddeployJsonPath(target());
        if (tmp.isEmpty()) {
            emit addOutput(tr("Cannot find the androiddeploy Json file."), ErrorOutput);
            return false;
        }
        Utils::QtcProcess::addArg(&m_androiddeployqtArgs, tmp.toString());

        Utils::QtcProcess::addArg(&m_androiddeployqtArgs, QLatin1String("--deployment"));
        switch (androidBuildApkStep->deployAction()) {
            case AndroidBuildApkStep::MinistroDeployment:
                Utils::QtcProcess::addArg(&m_androiddeployqtArgs, QLatin1String("ministro"));
                break;
            case AndroidBuildApkStep::DebugDeployment:
                Utils::QtcProcess::addArg(&m_androiddeployqtArgs, QLatin1String("debug"));
                break;
            case AndroidBuildApkStep::BundleLibrariesDeployment:
                Utils::QtcProcess::addArg(&m_androiddeployqtArgs, QLatin1String("bundled"));
                break;
        }
        if (androidBuildApkStep->useGradle())
            Utils::QtcProcess::addArg(&m_androiddeployqtArgs, QLatin1String("--gradle"));

        if (androidBuildApkStep->signPackage()) {
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
        m_apkPath = AndroidManager::androidQtSupport(target())->apkPath(target()).toString();
        m_workingDirectory = bc->buildDirectory().toString();
    }
    m_environment = bc->environment();

    m_buildDirectory = bc->buildDirectory().toString();

    m_adbPath = AndroidConfigurations::currentConfig().adbToolPath().toString();

    if (AndroidConfigurations::currentConfig().findAvd(m_avdName).isEmpty())
        AndroidConfigurations::currentConfig().startAVDAsync(m_avdName);
    return true;
}

AndroidDeployQtStep::DeployResult AndroidDeployQtStep::runDeploy(QFutureInterface<bool> &fi)
{
    m_installOk = true;
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
        if (m_uninstallPreviousPackageRun) {
            const QString packageName = AndroidManager::packageName(m_manifestName);
            if (packageName.isEmpty()) {
                emit addOutput(tr("Cannot find the package name."), ErrorOutput);
                return Failure;
            }

            emit addOutput(tr("Uninstall previous package %1.").arg(packageName), MessageOutput);
            runCommand(m_adbPath,
                       AndroidDeviceInfo::adbSelector(m_serialNumber)
                       << QLatin1String("uninstall") << packageName);
        }

        foreach (const QString &arg, AndroidDeviceInfo::adbSelector(m_serialNumber))
            Utils::QtcProcess::addArg(&args, arg);

        Utils::QtcProcess::addArg(&args, QLatin1String("install"));
        Utils::QtcProcess::addArg(&args, QLatin1String("-r"));
        Utils::QtcProcess::addArg(&args, m_apkPath);
    }

    m_process = new Utils::QtcProcess;
    m_process->setCommand(m_command, args);
    m_process->setWorkingDirectory(m_workingDirectory);
    m_process->setEnvironment(m_environment);

    if (Utils::HostOsInfo::isWindowsHost())
        m_process->setUseCtrlCStub(true);

    connect(m_process, &Utils::QtcProcess::readyReadStandardOutput,
            this, &AndroidDeployQtStep::processReadyReadStdOutput, Qt::DirectConnection);
    connect(m_process, &Utils::QtcProcess::readyReadStandardError,
            this, &AndroidDeployQtStep::processReadyReadStdError, Qt::DirectConnection);

    m_process->start();

    emit addOutput(tr("Starting: \"%1\" %2")
                   .arg(QDir::toNativeSeparators(m_command), args),
                   BuildStep::MessageOutput);

    while (m_process->state() != QProcess::NotRunning && !m_process->waitForFinished(200)) {
        if (fi.isCanceled()) {
            m_process->kill();
            m_process->waitForFinished();
        }
    }

    QString line = QString::fromLocal8Bit(m_process->readAllStandardError());
    if (!line.isEmpty())
        stdError(line);

    line = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    if (!line.isEmpty())
        stdOutput(line);

    QProcess::ExitStatus exitStatus = m_process->exitStatus();
    int exitCode = m_process->exitCode();
    delete m_process;
    m_process = 0;

    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        emit addOutput(tr("The process \"%1\" exited normally.").arg(m_command),
                       BuildStep::MessageOutput);
    } else if (exitStatus == QProcess::NormalExit) {
        emit addOutput(tr("The process \"%1\" exited with code %2.")
                       .arg(m_command, QString::number(exitCode)),
                       BuildStep::ErrorMessageOutput);
    } else {
        emit addOutput(tr("The process \"%1\" crashed.").arg(m_command), BuildStep::ErrorMessageOutput);
    }

    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
        if (!m_installOk) {
            if (!m_uninstallPreviousPackageRun)
                return AskUinstall;
            else
                return Failure;
        }
        return Success;
    }
    return Failure;
}

void AndroidDeployQtStep::slotAskForUninstall()
{
    int button = QMessageBox::critical(0, tr("Install failed"),
                                       tr("Another application with the same package id but signed with "
                                          "different certificate already exists.\n"
                                          "Do you want to uninstall the existing package?"),
                                       QMessageBox::Yes, QMessageBox::No);
    m_askForUinstall = button == QMessageBox::Yes;
}

void AndroidDeployQtStep::slotSetSerialNumber(const QString &serialNumber)
{
    AndroidManager::setDeviceSerialNumber(target(), serialNumber);
}

void AndroidDeployQtStep::run(QFutureInterface<bool> &fi)
{
    if (!m_avdName.isEmpty()) {
        QString serialNumber = AndroidConfigurations::currentConfig().waitForAvd(m_avdName, fi);
        if (serialNumber.isEmpty()) {
            fi.reportResult(false);
            emit finished();
            return;
        }
        m_serialNumber = serialNumber;
        emit setSerialNumber(serialNumber);
    }

    DeployResult returnValue = runDeploy(fi);
    if (returnValue == AskUinstall) {
        emit askForUninstall();
        if (m_askForUinstall) {
            m_uninstallPreviousPackageRun = true;
            returnValue = runDeploy(fi);
        }
    }

    emit addOutput(tr("Pulling files necessary for debugging."), MessageOutput);

    QString localAppProcessFile = QString::fromLatin1("%1/app_process").arg(m_buildDirectory);
    QFile::remove(localAppProcessFile);

    foreach (const QString &remoteAppProcessFile, m_appProcessBinaries) {
        runCommand(m_adbPath,
                   AndroidDeviceInfo::adbSelector(m_serialNumber)
                   << QLatin1String("pull") << remoteAppProcessFile
                   << localAppProcessFile);
        if (QFileInfo::exists(localAppProcessFile))
            break;
    }

    if (!QFileInfo::exists(localAppProcessFile)) {
        returnValue = Failure;
        emit addOutput(tr("Package deploy: Failed to pull \"%1\" to \"%2\".")
                       .arg(m_appProcessBinaries.join(QLatin1String("\", \"")))
                       .arg(localAppProcessFile), ErrorMessageOutput);
    }

    runCommand(m_adbPath,
               AndroidDeviceInfo::adbSelector(m_serialNumber) << QLatin1String("pull")
               << QLatin1String("/system/") + m_libdir + QLatin1String("/libc.so")
               << QString::fromLatin1("%1/libc.so").arg(m_buildDirectory));

    fi.reportResult(returnValue == Success ? true : false);
    fi.reportFinished();
}

void AndroidDeployQtStep::runCommand(const QString &program, const QStringList &arguments)
{
    QProcess buildProc;
    emit addOutput(tr("Package deploy: Running command \"%1 %2\".").arg(program).arg(arguments.join(QLatin1Char(' '))), BuildStep::MessageOutput);
    buildProc.start(program, arguments);
    if (!buildProc.waitForStarted()) {
        emit addOutput(tr("Packaging error: Could not start command \"%1 %2\". Reason: %3")
            .arg(program).arg(arguments.join(QLatin1Char(' '))).arg(buildProc.errorString()), BuildStep::ErrorMessageOutput);
        return;
    }
    if (!buildProc.waitForFinished(2 * 60 * 1000)
            || buildProc.error() != QProcess::UnknownError
            || buildProc.exitCode() != 0) {
        QString mainMessage = tr("Packaging error: Command \"%1 %2\" failed.")
                .arg(program).arg(arguments.join(QLatin1Char(' ')));
        if (buildProc.error() != QProcess::UnknownError)
            mainMessage += QLatin1Char(' ') + tr("Reason: %1").arg(buildProc.errorString());
        else
            mainMessage += tr("Exit code: %1").arg(buildProc.exitCode());
        emit addOutput(mainMessage, BuildStep::ErrorMessageOutput);
    }
}

AndroidDeviceInfo AndroidDeployQtStep::deviceInfo() const
{
    return m_deviceInfo;
}

ProjectExplorer::BuildStepConfigWidget *AndroidDeployQtStep::createConfigWidget()
{
    return new AndroidDeployQtWidget(this);
}

void AndroidDeployQtStep::processReadyReadStdOutput()
{
    m_process->setReadChannel(QProcess::StandardOutput);
    while (m_process->canReadLine()) {
        QString line = QString::fromLocal8Bit(m_process->readLine());
        stdOutput(line);
    }
}

void AndroidDeployQtStep::stdOutput(const QString &line)
{
    if (line.contains(InstallFailedInconsistentCertificatesString)
            || line.contains(InstallFailedInconsistentCertificatesString2))
        m_installOk = false;
    emit addOutput(line, BuildStep::NormalOutput, BuildStep::DontAppendNewline);
}

void AndroidDeployQtStep::processReadyReadStdError()
{
    m_process->setReadChannel(QProcess::StandardError);
    while (m_process->canReadLine()) {
        QString line = QString::fromLocal8Bit(m_process->readLine());
        stdError(line);
    }
}

void AndroidDeployQtStep::stdError(const QString &line)
{
    if (line.contains(InstallFailedInconsistentCertificatesString)
            || line.contains(InstallFailedInconsistentCertificatesString2))
        m_installOk = false;
    emit addOutput(line, BuildStep::ErrorOutput, BuildStep::DontAppendNewline);
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
