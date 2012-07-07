/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "androiddeploystep.h"

#include "androidconstants.h"
#include "androiddeploystepwidget.h"
#include "androidglobal.h"
#include "androidpackagecreationstep.h"
#include "androidrunconfiguration.h"
#include "androidmanager.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4nodes.h>

#include <qtsupport/qtprofileinformation.h>

#include <QDir>

#define ASSERT_STATE(state) ASSERT_STATE_GENERIC(State, state, m_state)

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

namespace Android {
namespace Internal {

static const char USE_LOCAL_QT_KEY[] = "Qt4ProjectManager.AndroidDeployStep.UseLocalQtLibs";

const Core::Id AndroidDeployStep::Id("Qt4ProjectManager.AndroidDeployStep");

AndroidDeployStep::AndroidDeployStep(ProjectExplorer::BuildStepList *parent)
    : BuildStep(parent, Id)
{
    ctor();
}

AndroidDeployStep::AndroidDeployStep(ProjectExplorer::BuildStepList *parent,
    AndroidDeployStep *other)
    : BuildStep(parent, other)
{
    ctor();
}

AndroidDeployStep::~AndroidDeployStep() { }

void AndroidDeployStep::ctor()
{
    //: AndroidDeployStep default display name
    setDefaultDisplayName(tr("Deploy to Android device"));
    m_deployAction = NoDeploy;
    m_useLocalQtLibs = false;
}

bool AndroidDeployStep::init()
{
    m_packageName = AndroidManager::packageName(target());
    const QString targetSDK = AndroidManager::targetSDK(target());

    writeOutput(tr("Please wait, searching for a suitable device for target:%1.").arg(targetSDK));
    m_deviceAPILevel = targetSDK.mid(targetSDK.indexOf(QLatin1Char('-')) + 1).toInt();
    m_deviceSerialNumber = AndroidConfigurations::instance().getDeployDeviceSerialNumber(&m_deviceAPILevel);
    if (!m_deviceSerialNumber.length()) {
        m_deviceSerialNumber.clear();
        raiseError(tr("Cannot deploy: no devices or emulators found for your package."));
        return false;
    }

    QtSupport::BaseQtVersion *version = QtSupport::QtProfileInformation::qtVersion(target()->profile());
    if (!version)
        return false;

    const Qt4BuildConfiguration *bc = static_cast<Qt4BuildConfiguration *>(target()->activeBuildConfiguration());
    if (!bc)
        return false;

    m_qtVersionSourcePath = version->sourcePath().toString();
    m_qtVersionQMakeBuildConfig = bc->qmakeBuildConfiguration();
    m_androidDirPath = AndroidManager::dirPath(target());
    m_apkPathDebug = AndroidManager::apkPath(target(), AndroidManager::DebugBuild).toString();
    m_apkPathRelease = AndroidManager::apkPath(target(), AndroidManager::ReleaseBuildSigned).toString();
    m_buildDirectory = static_cast<Qt4Project *>(target()->project())->rootQt4ProjectNode()->buildDir();
    m_runQASIPackagePath = m_QASIPackagePath;
    m_runDeployAction = m_deployAction;
    return true;
}

void AndroidDeployStep::run(QFutureInterface<bool> &fi)
{
    fi.reportResult(deployPackage());
}

BuildStepConfigWidget *AndroidDeployStep::createConfigWidget()
{
    return new AndroidDeployStepWidget(this);
}

AndroidDeployStep::AndroidDeployAction AndroidDeployStep::deployAction()
{
    return m_deployAction;
}

bool AndroidDeployStep::useLocalQtLibs()
{
    return m_useLocalQtLibs;
}

bool AndroidDeployStep::fromMap(const QVariantMap &map)
{
    m_useLocalQtLibs = map.value(QLatin1String(USE_LOCAL_QT_KEY), false).toBool();
    return ProjectExplorer::BuildStep::fromMap(map);
}

QVariantMap AndroidDeployStep::toMap() const
{
    QVariantMap map = ProjectExplorer::BuildStep::toMap();
    map.insert(QLatin1String(USE_LOCAL_QT_KEY), m_useLocalQtLibs);
    return map;
}

void AndroidDeployStep::setDeployAction(AndroidDeployStep::AndroidDeployAction deploy)
{
    m_deployAction = deploy;
}

void AndroidDeployStep::setDeployQASIPackagePath(const QString &package)
{
    m_QASIPackagePath = package;
    m_deployAction = InstallQASI;
}

void AndroidDeployStep::setUseLocalQtLibs(bool useLocal)
{
    m_useLocalQtLibs = useLocal;
}

bool AndroidDeployStep::runCommand(QProcess *buildProc,
    const QString &program, const QStringList &arguments)
{
    writeOutput(tr("Package deploy: Running command '%1 %2'.").arg(program).arg(arguments.join(QLatin1String(" "))), BuildStep::MessageOutput);
    buildProc->start(program, arguments);
    if (!buildProc->waitForStarted()) {
        writeOutput(tr("Packaging error: Could not start command '%1 %2'. Reason: %3")
            .arg(program).arg(arguments.join(QLatin1String(" "))).arg(buildProc->errorString()), BuildStep::ErrorMessageOutput);
        return false;
    }
    buildProc->waitForFinished(-1);
    if (buildProc->error() != QProcess::UnknownError
            || buildProc->exitCode() != 0) {
        QString mainMessage = tr("Packaging Error: Command '%1 %2' failed.")
                .arg(program).arg(arguments.join(QLatin1String(" ")));
        if (buildProc->error() != QProcess::UnknownError)
            mainMessage += tr(" Reason: %1").arg(buildProc->errorString());
        else
            mainMessage += tr("Exit code: %1").arg(buildProc->exitCode());
        writeOutput(mainMessage, BuildStep::ErrorMessageOutput);
        return false;
    }
    return true;
}

void AndroidDeployStep::handleBuildOutput()
{
    QProcess *const buildProc = qobject_cast<QProcess *>(sender());
    if (!buildProc)
        return;
    emit addOutput(QString::fromLocal8Bit(buildProc->readAllStandardOutput())
                   , BuildStep::NormalOutput);
}

void AndroidDeployStep::handleBuildError()
{
    QProcess *const buildProc = qobject_cast<QProcess *>(sender());
    if (!buildProc)
        return;
    emit addOutput(QString::fromLocal8Bit(buildProc->readAllStandardError())
                   , BuildStep::ErrorOutput);
}

QString AndroidDeployStep::deviceSerialNumber()
{
    return m_deviceSerialNumber;
}

int AndroidDeployStep::deviceAPILevel()
{
    return m_deviceAPILevel;
}

Utils::FileName AndroidDeployStep::localLibsRulesFilePath()
{
    return AndroidManager::localLibsRulesFilePath(target());
}

void AndroidDeployStep::copyLibs(const QString &srcPath, const QString &destPath, QStringList &copiedLibs, const QStringList &filter)
{
    QDir dir;
    dir.mkpath(destPath);
    QDirIterator libsIt(srcPath, filter, QDir::NoFilter, QDirIterator::Subdirectories);
    int pos = srcPath.size();
    while (libsIt.hasNext()) {
        libsIt.next();
        const QString destFile(destPath + libsIt.filePath().mid(pos));
        if (libsIt.fileInfo().isDir()) {
            dir.mkpath(destFile);
        } else {
            QFile::copy(libsIt.filePath(), destFile);
            copiedLibs.append(destFile);
        }
    }
}

bool AndroidDeployStep::deployPackage()
{
    QProcess *const deployProc = new QProcess;
    connect(deployProc, SIGNAL(readyReadStandardOutput()), this,
        SLOT(handleBuildOutput()));
    connect(deployProc, SIGNAL(readyReadStandardError()), this,
        SLOT(handleBuildError()));

    if (m_runDeployAction == DeployLocal) {
        writeOutput(tr("Clean old qt libs"));
        runCommand(deployProc, AndroidConfigurations::instance().adbToolPath().toString(),
                   QStringList() << QLatin1String("-s") << m_deviceSerialNumber
                   << QLatin1String("shell") << QLatin1String("rm") << QLatin1String("-r") << QLatin1String("/data/local/qt"));

        writeOutput(tr("Deploy qt libs ... this may take some time, please wait"));
        const QString tempPath = QDir::tempPath() + QLatin1String("/android_qt_libs_") + m_packageName;
        AndroidPackageCreationStep::removeDirectory(tempPath);
        QStringList stripFiles;
        copyLibs(m_qtVersionSourcePath + QLatin1String("/lib"),
                 tempPath + QLatin1String("/lib"), stripFiles, QStringList() << QLatin1String("*.so"));
        copyLibs(m_qtVersionSourcePath + QLatin1String("/plugins"),
                 tempPath + QLatin1String("/plugins"), stripFiles);
        copyLibs(m_qtVersionSourcePath + QLatin1String("/imports"),
                 tempPath + QLatin1String("/imports"), stripFiles);
        copyLibs(m_qtVersionSourcePath + QLatin1String("/jar"),
                 tempPath + QLatin1String("/jar"), stripFiles);
        AndroidPackageCreationStep::stripAndroidLibs(stripFiles, target()->activeRunConfiguration()->abi().architecture());
        runCommand(deployProc, AndroidConfigurations::instance().adbToolPath().toString(),
                   QStringList() << QLatin1String("-s") << m_deviceSerialNumber
                   << QLatin1String("push") << tempPath << QLatin1String("/data/local/qt"));
        AndroidPackageCreationStep::removeDirectory(tempPath);
        emit (resetDelopyAction());
    }

    if (m_runDeployAction == InstallQASI) {
        if (!runCommand(deployProc, AndroidConfigurations::instance().adbToolPath().toString(),
                        QStringList() << QLatin1String("-s") << m_deviceSerialNumber
                        << QLatin1String("install") << QLatin1String("-r ") << m_runQASIPackagePath)) {
            raiseError(tr("Qt Android smart installer instalation failed"));
            disconnect(deployProc, 0, this, 0);
            deployProc->deleteLater();
            return false;
        }
        emit resetDelopyAction();
    }
    deployProc->setWorkingDirectory(m_androidDirPath.toString());

    writeOutput(tr("Installing package onto %1.").arg(m_deviceSerialNumber));
    runCommand(deployProc, AndroidConfigurations::instance().adbToolPath().toString(),
               QStringList() << QLatin1String("-s") << m_deviceSerialNumber << QLatin1String("uninstall") << m_packageName);
    QString package = m_apkPathDebug;

    if (!(m_qtVersionQMakeBuildConfig & QtSupport::BaseQtVersion::DebugBuild)
         && QFile::exists(m_apkPathRelease))
        package = m_apkPathRelease;

    if (!runCommand(deployProc, AndroidConfigurations::instance().adbToolPath().toString(),
                    QStringList() << QLatin1String("-s") << m_deviceSerialNumber << QLatin1String("install") << package)) {
        raiseError(tr("Package instalation failed"));
        disconnect(deployProc, 0, this, 0);
        deployProc->deleteLater();
        return false;
    }

    writeOutput(tr("Pulling files necessary for debugging"));
    runCommand(deployProc, AndroidConfigurations::instance().adbToolPath().toString(),
               QStringList() << QLatin1String("-s") << m_deviceSerialNumber
               << QLatin1String("pull") << QLatin1String("/system/bin/app_process")
               << QString::fromLatin1("%1/app_process").arg(m_buildDirectory));
    runCommand(deployProc, AndroidConfigurations::instance().adbToolPath().toString(),
               QStringList() << QLatin1String("-s") << m_deviceSerialNumber << QLatin1String("pull")
               << QLatin1String("/system/lib/libc.so")
               << QString::fromLatin1("%1/libc.so").arg(m_buildDirectory));
    disconnect(deployProc, 0, this, 0);
    deployProc->deleteLater();
    return true;
}

void AndroidDeployStep::raiseError(const QString &errorString)
{
    emit addTask(Task(Task::Error, errorString, Utils::FileName::fromString(QString()), -1,
        ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
}

void AndroidDeployStep::writeOutput(const QString &text, OutputFormat format)
{
    emit addOutput(text, format);
}

} // namespace Internal
} // namespace Qt4ProjectManager
