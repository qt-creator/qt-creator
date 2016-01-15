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

#include "androidbuildapkstep.h"
#include "androidbuildapkwidget.h"
#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androidmanager.h"
#include "androidqtsupport.h"
#include "certificatesmodel.h"

#include "javaparser.h"

#include <coreplugin/fileutils.h>
#include <coreplugin/icore.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/qtcprocess.h>

#include <QInputDialog>
#include <QMessageBox>

namespace Android {
using namespace Internal;

const QLatin1String DeployActionKey("Qt4ProjectManager.AndroidDeployQtStep.DeployQtAction");
const QLatin1String KeystoreLocationKey("KeystoreLocation");
const QLatin1String BuildTargetSdkKey("BuildTargetSdk");
const QLatin1String VerboseOutputKey("VerboseOutput");
const QLatin1String UseGradleKey("UseGradle");

AndroidBuildApkStep::AndroidBuildApkStep(ProjectExplorer::BuildStepList *parent, const Core::Id id)
    : ProjectExplorer::AbstractProcessStep(parent, id),
      m_deployAction(BundleLibrariesDeployment),
      m_signPackage(false),
      m_verbose(false),
      m_useGradle(false),
      m_openPackageLocation(false),
      m_buildTargetSdk(AndroidConfig::apiLevelNameFor(AndroidConfigurations::currentConfig().highestAndroidSdk()))
{
    const QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target()->kit());
    if (version && version->qtVersion() >=  QtSupport::QtVersionNumber(5, 4, 0))
        m_useGradle = AndroidConfigurations::currentConfig().useGrandle();
    //: AndroidBuildApkStep default display name
    setDefaultDisplayName(tr("Build Android APK"));
}

AndroidBuildApkStep::AndroidBuildApkStep(ProjectExplorer::BuildStepList *parent,
    AndroidBuildApkStep *other)
    : ProjectExplorer::AbstractProcessStep(parent, other),
      m_deployAction(other->deployAction()),
      m_signPackage(other->signPackage()),
      m_verbose(other->m_verbose),
      m_useGradle(other->m_useGradle),
      m_openPackageLocation(other->m_openPackageLocation),
      m_buildTargetSdk(other->m_buildTargetSdk)
{
    const QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target()->kit());
    if (version->qtVersion() <  QtSupport::QtVersionNumber(5, 4, 0)) {
        if (m_deployAction == DebugDeployment)
            m_deployAction = BundleLibrariesDeployment;
        if (m_useGradle)
            m_useGradle = false;
    }
}

bool AndroidBuildApkStep::init(QList<const BuildStep *> &earlierSteps)
{
    ProjectExplorer::BuildConfiguration *bc = buildConfiguration();

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


        if (bc->buildType() != ProjectExplorer::BuildConfiguration::Release)
            emit addOutput(tr("Warning: Signing a debug or profile package."),
                           BuildStep::ErrorMessageOutput);
    }

    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target()->kit());
    if (!version)
        return false;

    JavaParser *parser = new JavaParser;
    parser->setProjectFileList(target()->project()->files(ProjectExplorer::Project::AllFiles));
    parser->setSourceDirectory(androidPackageSourceDir());
    parser->setBuildDirectory(Utils::FileName::fromString(bc->buildDirectory().appendPath(QLatin1String(Constants::ANDROID_BUILDDIRECTORY)).toString()));
    setOutputParser(parser);

    m_openPackageLocationForRun = m_openPackageLocation;
    m_apkPath = AndroidManager::androidQtSupport(target())->apkPath(target()).toString();

    bool result = AbstractProcessStep::init(earlierSteps);
    if (!result)
        return false;

    return true;
}

void AndroidBuildApkStep::showInGraphicalShell()
{
    Core::FileUtils::showInGraphicalShell(Core::ICore::instance()->mainWindow(), m_apkPath);
}

ProjectExplorer::BuildStepConfigWidget *AndroidBuildApkStep::createConfigWidget()
{
    return new AndroidBuildApkWidget(this);
}

void AndroidBuildApkStep::processFinished(int exitCode, QProcess::ExitStatus status)
{
    AbstractProcessStep::processFinished(exitCode, status);
    if (m_openPackageLocationForRun && status == QProcess::NormalExit && exitCode == 0)
        QMetaObject::invokeMethod(this, "showInGraphicalShell", Qt::QueuedConnection);
}

bool AndroidBuildApkStep::fromMap(const QVariantMap &map)
{
    m_deployAction = AndroidDeployAction(map.value(DeployActionKey, BundleLibrariesDeployment).toInt());
    if ( m_deployAction == DebugDeployment
         && QtSupport::QtKitInformation::qtVersion(target()->kit())->qtVersion() < QtSupport::QtVersionNumber(5, 4, 0)) {
        m_deployAction = BundleLibrariesDeployment;
    }

    m_keystorePath = Utils::FileName::fromString(map.value(KeystoreLocationKey).toString());
    m_signPackage = false; // don't restore this
    m_buildTargetSdk = map.value(BuildTargetSdkKey).toString();
    if (m_buildTargetSdk.isEmpty())
        m_buildTargetSdk = AndroidConfig::apiLevelNameFor(AndroidConfigurations::currentConfig().highestAndroidSdk());
    m_verbose = map.value(VerboseOutputKey).toBool();
    m_useGradle = map.value(UseGradleKey).toBool();
    return ProjectExplorer::BuildStep::fromMap(map);
}

QVariantMap AndroidBuildApkStep::toMap() const
{
    QVariantMap map = ProjectExplorer::AbstractProcessStep::toMap();
    map.insert(DeployActionKey, m_deployAction);
    map.insert(KeystoreLocationKey, m_keystorePath.toString());
    map.insert(BuildTargetSdkKey, m_buildTargetSdk);
    map.insert(VerboseOutputKey, m_verbose);
    map.insert(UseGradleKey, m_useGradle);
    return map;
}

Utils::FileName AndroidBuildApkStep::keystorePath()
{
    return m_keystorePath;
}

QString AndroidBuildApkStep::buildTargetSdk() const
{
    return m_buildTargetSdk;
}

void AndroidBuildApkStep::setBuildTargetSdk(const QString &sdk)
{
    m_buildTargetSdk = sdk;
    if (m_useGradle)
        AndroidManager::updateGradleProperties(target());
}

AndroidBuildApkStep::AndroidDeployAction AndroidBuildApkStep::deployAction() const
{
    return m_deployAction;
}

void AndroidBuildApkStep::setDeployAction(AndroidDeployAction deploy)
{
    m_deployAction = deploy;
}

void AndroidBuildApkStep::setKeystorePath(const Utils::FileName &path)
{
    m_keystorePath = path;
    m_certificatePasswd.clear();
    m_keystorePasswd.clear();
}

void AndroidBuildApkStep::setKeystorePassword(const QString &pwd)
{
    m_keystorePasswd = pwd;
}

void AndroidBuildApkStep::setCertificateAlias(const QString &alias)
{
    m_certificateAlias = alias;
}

void AndroidBuildApkStep::setCertificatePassword(const QString &pwd)
{
    m_certificatePasswd = pwd;
}

bool AndroidBuildApkStep::signPackage() const
{
    return m_signPackage;
}

void AndroidBuildApkStep::setSignPackage(bool b)
{
    m_signPackage = b;
}

bool AndroidBuildApkStep::openPackageLocation() const
{
    return m_openPackageLocation;
}

void AndroidBuildApkStep::setOpenPackageLocation(bool open)
{
    m_openPackageLocation = open;
}

void AndroidBuildApkStep::setVerboseOutput(bool verbose)
{
    m_verbose = verbose;
}

bool AndroidBuildApkStep::useGradle() const
{
    return m_useGradle;
}

void AndroidBuildApkStep::setUseGradle(bool b)
{
    m_useGradle = b;
    if (m_useGradle)
        AndroidManager::updateGradleProperties(target());
}

bool AndroidBuildApkStep::runInGuiThread() const
{
    return true;
}

bool AndroidBuildApkStep::verboseOutput() const
{
    return m_verbose;
}

QAbstractItemModel *AndroidBuildApkStep::keystoreCertificates()
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
        params << QLatin1String("-J-Duser.language=en");
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

bool AndroidBuildApkStep::keystorePassword()
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

bool AndroidBuildApkStep::certificatePassword()
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

} // namespace Android
