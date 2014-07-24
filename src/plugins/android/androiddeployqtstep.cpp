/**************************************************************************
**
** Copyright (c) 2014 BogDan Vatra <bog_dan_ro@yahoo.com>
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "androiddeployqtstep.h"
#include "androiddeployqtwidget.h"
#include "androidqtsupport.h"
#include "certificatesmodel.h"

#include "javaparser.h"
#include "androidmanager.h"
#include "androidconstants.h"

#include <coreplugin/fileutils.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QInputDialog>
#include <QMessageBox>


using namespace Android;
using namespace Android::Internal;

const QLatin1String UninstallPreviousPackageKey("UninstallPreviousPackage");
const QLatin1String KeystoreLocationKey("KeystoreLocation");
const QLatin1String SignPackageKey("SignPackage");
const QLatin1String BuildTargetSdkKey("BuildTargetSdk");
const QLatin1String VerboseOutputKey("VerboseOutput");
const QLatin1String InputFile("InputFile");
const QLatin1String ProFilePathForInputFile("ProFilePathForInputFile");
const QLatin1String InstallFailedInconsistentCertificatesString("INSTALL_PARSE_FAILED_INCONSISTENT_CERTIFICATES");
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
    : ProjectExplorer::AbstractProcessStep(parent, Id)
{
    ctor();
}

AndroidDeployQtStep::AndroidDeployQtStep(ProjectExplorer::BuildStepList *parent,
    AndroidDeployQtStep *other)
    : ProjectExplorer::AbstractProcessStep(parent, other)
{
    ctor();
}

void AndroidDeployQtStep::ctor()
{
    m_uninstallPreviousPackage = false;
    m_uninstallPreviousPackageTemp = false;
    m_uninstallPreviousPackageRun = false;

    //: AndroidDeployQtStep default display name
    setDefaultDisplayName(tr("Deploy to Android device"));
}

bool AndroidDeployQtStep::init()
{
    if (AndroidManager::checkForQt51Files(project()->projectDirectory()))
        emit addOutput(tr("Found old folder \"android\" in source directory. Qt 5.2 does not use that folder by default."), ErrorOutput);

    m_targetArch = AndroidManager::targetArch(target());
    if (m_targetArch.isEmpty()) {
        emit addOutput(tr("No Android arch set by the .pro file."), ErrorOutput);
        return false;
    }
    m_deviceAPILevel = AndroidManager::minimumSDK(target());
    AndroidDeviceInfo info = AndroidConfigurations::showDeviceDialog(project(), m_deviceAPILevel, m_targetArch);
    if (info.serialNumber.isEmpty()) // aborted
        return false;

    if (info.type == AndroidDeviceInfo::Emulator) {
        m_avdName = info.serialNumber;
        m_serialNumber.clear();
        m_deviceAPILevel = info.sdk;
    } else {
        m_avdName.clear();
        m_serialNumber = info.serialNumber;
    }
    AndroidManager::setDeviceSerialNumber(target(), m_serialNumber);

    ProjectExplorer::BuildConfiguration *bc = target()->activeBuildConfiguration();

    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target()->kit());
    if (!version)
        return false;

    m_uninstallPreviousPackageRun = m_uninstallPreviousPackage || m_uninstallPreviousPackageTemp;
    m_uninstallPreviousPackageTemp = false;
    if (m_uninstallPreviousPackageRun) {
        m_packageName = AndroidManager::packageName(target());
        if (m_packageName.isEmpty()){
            emit addOutput(tr("Cannot find the package name."), ErrorOutput);
            return false;
        }
    }

    ProjectExplorer::ProcessParameters *pp = processParameters();
    pp->setCommand(AndroidConfigurations::currentConfig().adbToolPath().toString());
    pp->setMacroExpander(bc->macroExpander());
    pp->setWorkingDirectory(bc->buildDirectory().toString());
    Utils::Environment env = bc->environment();
    pp->setEnvironment(env);
    m_apkPath = AndroidManager::androidQtSupport(target())->apkPath(target(), AndroidManager::signPackage(target())
                                                                    ? AndroidQtSupport::ReleaseBuildSigned
                                                                    : AndroidQtSupport::DebugBuild).toString();

    m_buildDirectory = bc->buildDirectory().toString();

    bool result = AbstractProcessStep::init();
    if (!result)
        return false;

    if (AndroidConfigurations::currentConfig().findAvd(m_deviceAPILevel, m_targetArch).isEmpty())
        AndroidConfigurations::currentConfig().startAVDAsync(m_avdName);
    return true;
}

void AndroidDeployQtStep::run(QFutureInterface<bool> &fi)
{
    m_installOk = true;
    if (!m_avdName.isEmpty()) {
        QString serialNumber = AndroidConfigurations::currentConfig().waitForAvd(m_deviceAPILevel, m_targetArch, fi);
        if (serialNumber.isEmpty()) {
            fi.reportResult(false);
            emit finished();
            return;
        }
        m_serialNumber = serialNumber;
        AndroidManager::setDeviceSerialNumber(target(), serialNumber);
    }

    if (m_uninstallPreviousPackageRun) {
        emit addOutput(tr("Uninstall previous package %1.").arg(m_packageName), MessageOutput);
        runCommand(AndroidConfigurations::currentConfig().adbToolPath().toString(),
                   AndroidDeviceInfo::adbSelector(m_serialNumber)
                   << QLatin1String("uninstall") << m_packageName);
    }

    ProjectExplorer::ProcessParameters *pp = processParameters();
    QString args;
    foreach (const QString &arg, AndroidDeviceInfo::adbSelector(m_serialNumber))
        Utils::QtcProcess::addArg(&args, arg);

    Utils::QtcProcess::addArg(&args, QLatin1String("install"));
    Utils::QtcProcess::addArg(&args, QLatin1String("-r"));
    Utils::QtcProcess::addArg(&args, m_apkPath);
    pp->setArguments(args);
    pp->resolveAll();

    AbstractProcessStep::run(fi);

    emit addOutput(tr("Pulling files necessary for debugging."), MessageOutput);
    runCommand(AndroidConfigurations::currentConfig().adbToolPath().toString(),
               AndroidDeviceInfo::adbSelector(m_serialNumber)
               << QLatin1String("pull") << QLatin1String("/system/bin/app_process")
               << QString::fromLatin1("%1/app_process").arg(m_buildDirectory));
    runCommand(AndroidConfigurations::currentConfig().adbToolPath().toString(),
               AndroidDeviceInfo::adbSelector(m_serialNumber) << QLatin1String("pull")
               << QLatin1String("/system/lib/libc.so")
               << QString::fromLatin1("%1/libc.so").arg(m_buildDirectory));
}

void AndroidDeployQtStep::runCommand(const QString &program, const QStringList &arguments)
{
    QProcess buildProc;
    emit addOutput(tr("Package deploy: Running command \"%1 %2\".").arg(program).arg(arguments.join(QLatin1String(" "))), BuildStep::MessageOutput);
    buildProc.start(program, arguments);
    if (!buildProc.waitForStarted()) {
        emit addOutput(tr("Packaging error: Could not start command \"%1 %2\". Reason: %3")
            .arg(program).arg(arguments.join(QLatin1String(" "))).arg(buildProc.errorString()), BuildStep::ErrorMessageOutput);
        return;
    }
    if (!buildProc.waitForFinished(2 * 60 * 1000)
            || buildProc.error() != QProcess::UnknownError
            || buildProc.exitCode() != 0) {
        QString mainMessage = tr("Packaging Error: Command \"%1 %2\" failed.")
                .arg(program).arg(arguments.join(QLatin1String(" ")));
        if (buildProc.error() != QProcess::UnknownError)
            mainMessage += QLatin1Char(' ') + tr("Reason: %1").arg(buildProc.errorString());
        else
            mainMessage += tr("Exit code: %1").arg(buildProc.exitCode());
        emit addOutput(mainMessage, BuildStep::ErrorMessageOutput);
    }
}

ProjectExplorer::BuildStepConfigWidget *AndroidDeployQtStep::createConfigWidget()
{
    return new AndroidDeployQtWidget(this);
}

void AndroidDeployQtStep::stdOutput(const QString &line)
{
    if (line.contains(InstallFailedInconsistentCertificatesString))
        m_installOk = false;
    AbstractProcessStep::stdOutput(line);
}

void AndroidDeployQtStep::stdError(const QString &line)
{
    if (line.contains(InstallFailedInconsistentCertificatesString))
        m_installOk = false;
    AbstractProcessStep::stdError(line);
}

bool AndroidDeployQtStep::processSucceeded(int exitCode, QProcess::ExitStatus status)
{
    if (!m_installOk && !m_uninstallPreviousPackageRun &&
            QMessageBox::critical(0, tr("Install failed"),
                                  tr("Another application with the same package id but signed with "
                                     "different ceritificate already exists.\n"
                                     "Do you want to uninstall the existing package next time?"),
                                  QMessageBox::Yes, QMessageBox::No)
            == QMessageBox::Yes)  {
        m_uninstallPreviousPackageTemp = true;
    }
    return m_installOk && AbstractProcessStep::processSucceeded(exitCode, status);
}

bool AndroidDeployQtStep::fromMap(const QVariantMap &map)
{
    m_uninstallPreviousPackage = map.value(UninstallPreviousPackageKey, false).toBool();
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
    return true;
}

bool AndroidDeployQtStep::uninstallPreviousPackage()
{
    return m_uninstallPreviousPackage;
}
