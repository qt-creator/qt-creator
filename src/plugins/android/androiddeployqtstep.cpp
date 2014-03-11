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
#include "certificatesmodel.h"

#include "javaparser.h"
#include "androidmanager.h"
#include "androidconstants.h"

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <coreplugin/fileutils.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <qtsupport/qtkitinformation.h>
#include <qmakeprojectmanager/qmakebuildconfiguration.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakenodes.h>
#include <QInputDialog>
#include <QMessageBox>

using namespace Android;
using namespace Android::Internal;

const QLatin1String DeployActionKey("Qt4ProjectManager.AndroidDeployQtStep.DeployQtAction");
const QLatin1String KeystoreLocationKey("KeystoreLocation");
const QLatin1String SignPackageKey("SignPackage");
const QLatin1String BuildTargetSdkKey("BuildTargetSdk");
const QLatin1String VerboseOutputKey("VerboseOutput");
const QLatin1String InputFile("InputFile");
const QLatin1String ProFilePathForInputFile("ProFilePathForInputFile");
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
    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(parent->target()->kit());
    if (!qtVersion || qtVersion->qtVersion() < QtSupport::QtVersionNumber(5, 2, 0))
        return QList<Core::Id>();
    return QList<Core::Id>() << AndroidDeployQtStep::Id;
}

QString AndroidDeployQtStepFactory::displayNameForId(const Core::Id id) const
{
    if (id == AndroidDeployQtStep::Id)
        return tr("Deploy to Android device or emulator");
    return QString();
}

bool AndroidDeployQtStepFactory::canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

ProjectExplorer::BuildStep *AndroidDeployQtStepFactory::create(ProjectExplorer::BuildStepList *parent, const Core::Id id)
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
    //: AndroidDeployQtStep default display name
    setDefaultDisplayName(tr("Deploy to Android device"));
    m_deployAction = BundleLibrariesDeployment;
    m_signPackage = false;
    m_openPackageLocation = false;
    m_verbose = false;

    // will be overwriten by settings if the user choose something different
    m_buildTargetSdk = AndroidConfigurations::currentConfig().highestAndroidSdk();

    connect(project(), SIGNAL(proFilesEvaluated()),
           this, SLOT(updateInputFile()));
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

    QmakeProjectManager::QmakeBuildConfiguration *bc
            = static_cast<QmakeProjectManager::QmakeBuildConfiguration *>(target()->activeBuildConfiguration());

    if (m_signPackage) {
        // check keystore and certificate passwords
        while (!AndroidManager::checkKeystorePassword(m_keystorePath.toString(), m_keystorePasswd)) {
            if (!keystorePassword())
                return false; // user canceled
        }

        while (!AndroidManager::checkCertificatePassword(m_keystorePath.toString(), m_keystorePasswd, m_certificateAlias, m_certificatePasswd)) {
            if (!certificatePassword())
                return false; // user canceled
        }


        if ((bc->qmakeBuildConfiguration() & QtSupport::BaseQtVersion::DebugBuild))
            emit addOutput(tr("Warning: Signing a debug package."), BuildStep::ErrorMessageOutput);
    }

    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target()->kit());
    if (!version)
        return false;

    QmakeProjectManager::QmakeProject *pro = static_cast<QmakeProjectManager::QmakeProject *>(project());
    JavaParser *parser = new JavaParser;
    parser->setProjectFileList(pro->files(ProjectExplorer::Project::AllFiles));
    setOutputParser(parser);

    QString command = version->qmakeProperty("QT_HOST_BINS");
    if (!command.endsWith(QLatin1Char('/')))
        command += QLatin1Char('/');
    command += QLatin1String("androiddeployqt");
    if (Utils::HostOsInfo::isWindowsHost())
        command += QLatin1String(".exe");

    QString deploymentMethod;
    if (m_deployAction == MinistroDeployment)
        deploymentMethod = QLatin1String("ministro");
    else if (m_deployAction == DebugDeployment)
        deploymentMethod = QLatin1String("debug");
    else if (m_deployAction == BundleLibrariesDeployment)
        deploymentMethod = QLatin1String("bundled");

    QString outputDir = bc->buildDirectory().appendPath(QLatin1String(Constants::ANDROID_BUILDDIRECTORY)).toString();
    const QmakeProjectManager::QmakeProFileNode *node = pro->rootQmakeProjectNode()->findProFileFor(m_proFilePathForInputFile);
    if (!node) { // should never happen
        emit addOutput(tr("Internal Error: Could not find .pro file."), BuildStep::ErrorMessageOutput);
        return false;
    }

    QString inputFile = node->singleVariableValue(QmakeProjectManager::AndroidDeploySettingsFile);
    if (inputFile.isEmpty()) { // should never happen
        emit addOutput(tr("Internal Error: Unknown Android deployment JSON file location."), BuildStep::ErrorMessageOutput);
        return false;
    }

    QStringList arguments;
    arguments << QLatin1String("--input")
              << inputFile
              << QLatin1String("--output")
              << outputDir
              << QLatin1String("--deployment")
              << deploymentMethod
              << QLatin1String("--install")
              << QLatin1String("--ant")
              << AndroidConfigurations::currentConfig().antToolPath().toString()
              << QLatin1String("--android-platform")
              << m_buildTargetSdk
              << QLatin1String("--jdk")
              << AndroidConfigurations::currentConfig().openJDKLocation().toString();

    parser->setSourceDirectory(Utils::FileName::fromString(node->singleVariableValue(QmakeProjectManager::AndroidPackageSourceDir)));
    parser->setBuildDirectory(Utils::FileName::fromString(outputDir));

    if (m_verbose)
        arguments << QLatin1String("--verbose");
    if (m_avdName.isEmpty())
        arguments  << QLatin1String("--device")
                   << info.serialNumber;

    if (m_signPackage) {
        arguments << QLatin1String("--sign")
                  << m_keystorePath.toString()
                  << m_certificateAlias
                  << QLatin1String("--storepass")
                  << m_keystorePasswd;
        if (!m_certificatePasswd.isEmpty())
            arguments << QLatin1String("--keypass")
                      << m_certificatePasswd;
    }

    ProjectExplorer::ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setWorkingDirectory(bc->buildDirectory().toString());
    Utils::Environment env = bc->environment();
    pp->setEnvironment(env);
    pp->setCommand(command);
    pp->setArguments(Utils::QtcProcess::joinArgs(arguments));
    pp->resolveAll();

    m_openPackageLocationForRun = m_openPackageLocation;
    m_apkPath = AndroidManager::apkPath(target(), m_signPackage ? AndroidManager::ReleaseBuildSigned
                                                                : AndroidManager::DebugBuild).toString();
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
    if (!m_avdName.isEmpty()) {
        QString serialNumber = AndroidConfigurations::currentConfig().waitForAvd(m_deviceAPILevel, m_targetArch, fi);
        if (serialNumber.isEmpty()) {
            fi.reportResult(false);
            emit finished();
            return;
        }
        m_serialNumber = serialNumber;
        QString args = processParameters()->arguments();
        Utils::QtcProcess::addArg(&args, QLatin1String("--device"));
        Utils::QtcProcess::addArg(&args, serialNumber);
        processParameters()->setArguments(args);
    }

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
    emit addOutput(tr("Package deploy: Running command '%1 %2'.").arg(program).arg(arguments.join(QLatin1String(" "))), BuildStep::MessageOutput);
    buildProc.start(program, arguments);
    if (!buildProc.waitForStarted()) {
        emit addOutput(tr("Packaging error: Could not start command '%1 %2'. Reason: %3")
            .arg(program).arg(arguments.join(QLatin1String(" "))).arg(buildProc.errorString()), BuildStep::ErrorMessageOutput);
        return;
    }
    if (!buildProc.waitForFinished(2 * 60 * 1000)
            || buildProc.error() != QProcess::UnknownError
            || buildProc.exitCode() != 0) {
        QString mainMessage = tr("Packaging Error: Command '%1 %2' failed.")
                .arg(program).arg(arguments.join(QLatin1String(" ")));
        if (buildProc.error() != QProcess::UnknownError)
            mainMessage += QLatin1Char(' ') + tr("Reason: %1").arg(buildProc.errorString());
        else
            mainMessage += tr("Exit code: %1").arg(buildProc.exitCode());
        emit addOutput(mainMessage, BuildStep::ErrorMessageOutput);
    }
}

void AndroidDeployQtStep::updateInputFile()
{
    QmakeProjectManager::QmakeProject *pro = static_cast<QmakeProjectManager::QmakeProject *>(project());
    QList<QmakeProjectManager::QmakeProFileNode *> nodes = pro->applicationProFiles();

    const QmakeProjectManager::QmakeProFileNode *node = pro->rootQmakeProjectNode()->findProFileFor(m_proFilePathForInputFile);
    if (!nodes.contains(const_cast<QmakeProjectManager::QmakeProFileNode *>(node))) {
        if (!nodes.isEmpty())
            m_proFilePathForInputFile = nodes.first()->path();
        else
            m_proFilePathForInputFile.clear();
    }

    emit inputFileChanged();
}

void AndroidDeployQtStep::showInGraphicalShell()
{
    Core::FileUtils::showInGraphicalShell(Core::ICore::instance()->mainWindow(), m_apkPath);
}

ProjectExplorer::BuildStepConfigWidget *AndroidDeployQtStep::createConfigWidget()
{
    return new AndroidDeployQtWidget(this);
}

void AndroidDeployQtStep::processFinished(int exitCode, QProcess::ExitStatus status)
{
    AbstractProcessStep::processFinished(exitCode, status);
    if (m_openPackageLocationForRun)
        QMetaObject::invokeMethod(this, "showInGraphicalShell", Qt::QueuedConnection);
}

bool AndroidDeployQtStep::fromMap(const QVariantMap &map)
{
    m_deployAction = AndroidDeployQtAction(map.value(QLatin1String(DeployActionKey),
                                                     BundleLibrariesDeployment).toInt());
    m_keystorePath = Utils::FileName::fromString(map.value(KeystoreLocationKey).toString());
    m_signPackage = false; // don't restore this
    m_buildTargetSdk = map.value(BuildTargetSdkKey).toString();
    m_verbose = map.value(VerboseOutputKey).toBool();
    m_proFilePathForInputFile = map.value(ProFilePathForInputFile).toString();
    return ProjectExplorer::BuildStep::fromMap(map);
}

QVariantMap AndroidDeployQtStep::toMap() const
{
    QVariantMap map = ProjectExplorer::BuildStep::toMap();
    map.insert(QLatin1String(DeployActionKey), m_deployAction);
    map.insert(KeystoreLocationKey, m_keystorePath.toString());
    map.insert(SignPackageKey, m_signPackage);
    map.insert(BuildTargetSdkKey, m_buildTargetSdk);
    map.insert(VerboseOutputKey, m_verbose);
    map.insert(ProFilePathForInputFile, m_proFilePathForInputFile);
    return map;
}

void AndroidDeployQtStep::setBuildTargetSdk(const QString &sdk)
{
    m_buildTargetSdk = sdk;
}

QString AndroidDeployQtStep::buildTargetSdk() const
{
    return m_buildTargetSdk;
}

Utils::FileName AndroidDeployQtStep::keystorePath()
{
    return m_keystorePath;
}

AndroidDeployQtStep::AndroidDeployQtAction AndroidDeployQtStep::deployAction() const
{
    return m_deployAction;
}

void AndroidDeployQtStep::setDeployAction(AndroidDeployQtStep::AndroidDeployQtAction deploy)
{
    m_deployAction = deploy;
}

void AndroidDeployQtStep::setKeystorePath(const Utils::FileName &path)
{
    m_keystorePath = path;
    m_certificatePasswd.clear();
    m_keystorePasswd.clear();
}

void AndroidDeployQtStep::setKeystorePassword(const QString &pwd)
{
    m_keystorePasswd = pwd;
}

void AndroidDeployQtStep::setCertificateAlias(const QString &alias)
{
    m_certificateAlias = alias;
}

void AndroidDeployQtStep::setCertificatePassword(const QString &pwd)
{
    m_certificatePasswd = pwd;
}

bool AndroidDeployQtStep::signPackage() const
{
    return m_signPackage;
}

void AndroidDeployQtStep::setSignPackage(bool b)
{
    m_signPackage = b;
}

QString AndroidDeployQtStep::deviceSerialNumber()
{
    return m_serialNumber;
}

bool AndroidDeployQtStep::openPackageLocation() const
{
    return m_openPackageLocation;
}

void AndroidDeployQtStep::setOpenPackageLocation(bool open)
{
    m_openPackageLocation = open;
}

void AndroidDeployQtStep::setVerboseOutput(bool verbose)
{
    m_verbose = verbose;
}

QString AndroidDeployQtStep::proFilePathForInputFile() const
{
    return m_proFilePathForInputFile;
}

void AndroidDeployQtStep::setProFilePathForInputFile(const QString &path)
{
    m_proFilePathForInputFile = path;
}

bool AndroidDeployQtStep::runInGuiThread() const
{
    return true;
}

bool AndroidDeployQtStep::verboseOutput() const
{
    return m_verbose;
}

// Note this functions is duplicated between AndroidDeployStep and AndroidDeployQtStep
// since it does modify the stored password in AndroidDeployQtStep it's not easily
// extractable. The situation will clean itself up once AndroidDeployStep is no longer
// necessary
QAbstractItemModel *AndroidDeployQtStep::keystoreCertificates()
{
    QString rawCerts;
    QProcess keytoolProc;
    while (!rawCerts.length() || !m_keystorePasswd.length()) {
        QStringList params;
        params << QLatin1String("-list") << QLatin1String("-v") << QLatin1String("-keystore") << m_keystorePath.toUserOutput() << QLatin1String("-storepass");
        if (!m_keystorePasswd.length())
            keystorePassword();
        if (!m_keystorePasswd.length())
            return 0;
        params << m_keystorePasswd;
        Utils::Environment env = Utils::Environment::systemEnvironment();
        env.set(QLatin1String("LC_ALL"), QLatin1String("C"));
        keytoolProc.setProcessEnvironment(env.toProcessEnvironment());
        keytoolProc.start(AndroidConfigurations::currentConfig().keytoolPath().toString(), params);
        if (!keytoolProc.waitForStarted() || !keytoolProc.waitForFinished()) {
            QMessageBox::critical(0, tr("Error"),
                                  tr("Failed to run keytool."));
            return 0;
        }

        if (keytoolProc.exitCode()) {
            QMessageBox::critical(0, tr("Error"),
                                  tr("Invalid password."));
            m_keystorePasswd.clear();
        }
        rawCerts = QString::fromLatin1(keytoolProc.readAllStandardOutput());
    }
    return new CertificatesModel(rawCerts, this);
}

bool AndroidDeployQtStep::keystorePassword()
{
    m_keystorePasswd.clear();
    bool ok;
    QString text = QInputDialog::getText(0, tr("Keystore"),
                                         tr("Keystore password:"), QLineEdit::Password,
                                         QString(), &ok);
    if (ok && !text.isEmpty()) {
        m_keystorePasswd = text;
        return true;
    }
    return false;
}

bool AndroidDeployQtStep::certificatePassword()
{
    m_certificatePasswd.clear();
    bool ok;
    QString text = QInputDialog::getText(0, tr("Certificate"),
                                         tr("Certificate password (%1):").arg(m_certificateAlias), QLineEdit::Password,
                                         QString(), &ok);
    if (ok && !text.isEmpty()) {
        m_certificatePasswd = text;
        return true;
    }
    return false;
}
