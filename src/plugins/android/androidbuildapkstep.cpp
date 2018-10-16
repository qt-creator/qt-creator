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
#include "androidsdkmanager.h"
#include "androidqtsupport.h"
#include "certificatesmodel.h"

#include "javaparser.h"

#include <coreplugin/fileutils.h>
#include <coreplugin/icore.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/algorithm.h>
#include <utils/synchronousprocess.h>
#include <utils/utilsicons.h>

#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QVBoxLayout>

#include <memory>

using namespace ProjectExplorer;
using namespace Android::Internal;

namespace {
Q_LOGGING_CATEGORY(buildapkstepLog, "qtc.android.build.androidbuildapkstep", QtWarningMsg)
}

namespace Android {

const Core::Id ANDROID_BUILD_APK_ID("QmakeProjectManager.AndroidBuildApkStep");

const QVersionNumber gradleScriptRevokedSdkVersion(25, 3, 0);
const char KeystoreLocationKey[] = "KeystoreLocation";
const char BuildTargetSdkKey[] = "BuildTargetSdk";
const char VerboseOutputKey[] = "VerboseOutput";
const char UseMinistroKey[] = "UseMinistro";

static void setupProcessParameters(ProcessParameters *pp,
                                   BuildConfiguration *bc,
                                   const QStringList &arguments,
                                   const QString &command)
{
    pp->setMacroExpander(bc->macroExpander());
    pp->setWorkingDirectory(bc->buildDirectory().toString());
    Utils::Environment env = bc->environment();
    pp->setEnvironment(env);
    pp->setCommand(command);
    pp->setArguments(Utils::QtcProcess::joinArgs(arguments));
    pp->resolveAll();
}

class PasswordInputDialog : public QDialog
{
    Q_OBJECT

public:
    enum Context{
      KeystorePassword = 1,
      CertificatePassword
    };

    PasswordInputDialog(Context context, std::function<bool (const QString &)> callback,
                        const QString &extraContextStr, QWidget *parent = nullptr);

    static QString getPassword(Context context, std::function<bool (const QString &)> callback,
                               const QString &extraContextStr, bool *ok = nullptr,
                               QWidget *parent = nullptr);

private:
    std::function<bool (const QString &)> verifyCallback = [](const QString &) { return true; };
    QLabel *inputContextlabel = new QLabel(this);
    QLineEdit *inputEdit = new QLineEdit(this);
    QLabel *warningIcon = new QLabel(this);
    QLabel *warningLabel = new QLabel(this);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                       this);
};

AndroidBuildApkStep::AndroidBuildApkStep(BuildStepList *parent)
    : AbstractProcessStep(parent, ANDROID_BUILD_APK_ID),
      m_buildTargetSdk(AndroidConfig::apiLevelNameFor(AndroidConfigurations::
                                         sdkManager()->latestAndroidSdkPlatform()))
{
    //: AndroidBuildApkStep default display name
    setDefaultDisplayName(tr("Build Android APK"));
}

bool AndroidBuildApkStep::init(QList<const BuildStep *> &earlierSteps)
{
    ProjectExplorer::BuildConfiguration *bc = buildConfiguration();

    if (m_signPackage) {
        qCDebug(buildapkstepLog) << "Signing enabled";
        // check keystore and certificate passwords
        if (!verifyKeystorePassword() || !verifyCertificatePassword()) {
            qCDebug(buildapkstepLog) << "Init failed. Keystore/Certificate password verification failed.";
            return false;
        }

        if (bc->buildType() != ProjectExplorer::BuildConfiguration::Release)
            emit addOutput(tr("Warning: Signing a debug or profile package."),
                           OutputFormat::ErrorMessage);
    }

    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target()->kit());
    if (!version)
        return false;

    const QVersionNumber sdkToolsVersion = AndroidConfigurations::currentConfig().sdkToolsVersion();
    if (sdkToolsVersion >= gradleScriptRevokedSdkVersion) {
        if (!version->sourcePath().appendPath("src/3rdparty/gradle").exists()) {
            emit addOutput(tr("The installed SDK tools version (%1) does not include Gradle "
                              "scripts. The minimum Qt version required for Gradle build to work "
                              "is %2").arg(sdkToolsVersion.toString()).arg("5.9.0/5.6.3"),
                           OutputFormat::Stderr);
            return false;
        }
    } else if (version->qtVersion() < QtSupport::QtVersionNumber(5, 4, 0)) {
        emit addOutput(tr("The minimum Qt version required for Gradle build to work is %1. "
                          "It is recommended to install the latest Qt version.")
                       .arg("5.4.0"), OutputFormat::Stderr);
        return false;
    }

    int minSDKForKit = AndroidManager::minimumSDK(target()->kit());
    if (AndroidManager::minimumSDK(target()) < minSDKForKit) {
        emit addOutput(tr("The API level set for the APK is less than the minimum required by the kit."
                          "\nThe minimum API level required by the kit is %1.").arg(minSDKForKit), OutputFormat::Stderr);
        return false;
    }

    auto parser = new JavaParser;
    parser->setProjectFileList(Utils::transform(target()->project()->files(ProjectExplorer::Project::AllFiles),
                                                &Utils::FileName::toString));

    AndroidQtSupport *qtSupport = AndroidManager::androidQtSupport(target());
    QFileInfo sourceDirInfo(qtSupport->targetData(Constants::AndroidPackageSourceDir, target()).toString());
    parser->setSourceDirectory(Utils::FileName::fromString(sourceDirInfo.canonicalFilePath()));
    parser->setBuildDirectory(Utils::FileName::fromString(bc->buildDirectory().appendPath(Constants::ANDROID_BUILDDIRECTORY).toString()));
    setOutputParser(parser);

    m_openPackageLocationForRun = m_openPackageLocation;
    m_apkPath = AndroidManager::apkPath(target()).toString();
    qCDebug(buildapkstepLog) << "APK path:" << m_apkPath;

    if (!AbstractProcessStep::init(earlierSteps))
        return false;

    QString command = version->qmakeProperty("QT_HOST_BINS");
    if (!command.endsWith('/'))
        command += '/';
    command += "androiddeployqt";
    if (Utils::HostOsInfo::isWindowsHost())
        command += ".exe";

    QString outputDir = bc->buildDirectory().appendPath(Constants::ANDROID_BUILDDIRECTORY).toString();

    QString inputFile = AndroidManager::androidQtSupport(target())
            ->targetData(Constants::AndroidDeploySettingsFile, target()).toString();
    if (inputFile.isEmpty()) {
        m_skipBuilding = true;
        return true;
    }

    QString buildTargetSdk = AndroidManager::buildTargetSDK(target());
    if (buildTargetSdk.isEmpty()) {
        emit addOutput(tr("Android build SDK not defined. Check Android settings."),
                       OutputFormat::Stderr);
        return false;
    }

    QStringList arguments = {"--input", inputFile,
                             "--output", outputDir,
                             "--android-platform", AndroidManager::buildTargetSDK(target()),
                             "--jdk", AndroidConfigurations::currentConfig().openJDKLocation().toString()};

    if (m_verbose)
        arguments << "--verbose";

    arguments << "--gradle";

    if (m_useMinistro)
        arguments << "--deployment" << "ministro";

    QStringList argumentsPasswordConcealed = arguments;

    if (m_signPackage) {
        arguments << "--sign" << m_keystorePath.toString() << m_certificateAlias
                  << "--storepass" << m_keystorePasswd;
        argumentsPasswordConcealed << "--sign" << "******"
                                   << "--storepass" << "******";
        if (!m_certificatePasswd.isEmpty()) {
            arguments << "--keypass" << m_certificatePasswd;
            argumentsPasswordConcealed << "--keypass" << "******";
        }

    }

    // Must be the last option, otherwise androiddeployqt might use the other
    // params (e.g. --sign) to choose not to add gdbserver
    if (version->qtVersion() >= QtSupport::QtVersionNumber(5, 6, 0)) {
        if (m_addDebugger || bc->buildType() == ProjectExplorer::BuildConfiguration::Debug)
            arguments << "--gdbserver";
        else
            arguments << "--no-gdbserver";
    }

    ProjectExplorer::ProcessParameters *pp = processParameters();
    setupProcessParameters(pp, bc, arguments, command);

    // Generate arguments with keystore password concealed
    ProjectExplorer::ProcessParameters pp2;
    setupProcessParameters(&pp2, bc, argumentsPasswordConcealed, command);
    m_command = pp2.effectiveCommand();
    m_argumentsPasswordConcealed = pp2.prettyArguments();

    return true;
}

void AndroidBuildApkStep::showInGraphicalShell()
{
    Core::FileUtils::showInGraphicalShell(Core::ICore::mainWindow(), m_apkPath);
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

bool AndroidBuildApkStep::verifyKeystorePassword()
{
    if (!m_keystorePath.exists()) {
        addOutput(tr("Cannot sign the package. Invalid keystore path (%1).")
                  .arg(m_keystorePath.toString()), OutputFormat::ErrorMessage);
        return false;
    }

    if (AndroidManager::checkKeystorePassword(m_keystorePath.toString(), m_keystorePasswd))
        return true;

    bool success = false;
    auto verifyCallback = std::bind(&AndroidManager::checkKeystorePassword,
                                    m_keystorePath.toString(), std::placeholders::_1);
    m_keystorePasswd = PasswordInputDialog::getPassword(PasswordInputDialog::KeystorePassword,
                                                        verifyCallback, "", &success);
    return success;
}

bool AndroidBuildApkStep::verifyCertificatePassword()
{
    if (!AndroidManager::checkCertificateExists(m_keystorePath.toString(), m_keystorePasswd,
                                                 m_certificateAlias)) {
        addOutput(tr("Cannot sign the package. Certificate alias %1 does not exist.")
                  .arg(m_certificateAlias), OutputFormat::ErrorMessage);
        return false;
    }

    if (AndroidManager::checkCertificatePassword(m_keystorePath.toString(), m_keystorePasswd,
                                                 m_certificateAlias, m_certificatePasswd)) {
        return true;
    }

    bool success = false;
    auto verifyCallback = std::bind(&AndroidManager::checkCertificatePassword,
                                    m_keystorePath.toString(), m_keystorePasswd,
                                    m_certificateAlias, std::placeholders::_1);

    m_certificatePasswd = PasswordInputDialog::getPassword(PasswordInputDialog::CertificatePassword,
                                                           verifyCallback, m_certificateAlias,
                                                           &success);
    return success;
}

void AndroidBuildApkStep::run(QFutureInterface<bool> &fi)
{
    if (m_skipBuilding) {
        emit addOutput(tr("No application .pro file found, not building an APK."), BuildStep::OutputFormat::ErrorMessage);
        reportRunResult(fi, true);
        return;
    }
    AbstractProcessStep::run(fi);
}

void AndroidBuildApkStep::processStarted()
{
    emit addOutput(tr("Starting: \"%1\" %2")
                   .arg(QDir::toNativeSeparators(m_command),
                        m_argumentsPasswordConcealed),
                   BuildStep::OutputFormat::NormalMessage);
}

bool AndroidBuildApkStep::fromMap(const QVariantMap &map)
{
    m_keystorePath = Utils::FileName::fromString(map.value(KeystoreLocationKey).toString());
    m_signPackage = false; // don't restore this
    m_buildTargetSdk = map.value(BuildTargetSdkKey).toString();
    if (m_buildTargetSdk.isEmpty()) {
        m_buildTargetSdk = AndroidConfig::apiLevelNameFor(AndroidConfigurations::
                                                          sdkManager()->latestAndroidSdkPlatform());
    }
    m_verbose = map.value(VerboseOutputKey).toBool();
    m_useMinistro = map.value(UseMinistroKey).toBool();
    return ProjectExplorer::BuildStep::fromMap(map);
}

QVariantMap AndroidBuildApkStep::toMap() const
{
    QVariantMap map = ProjectExplorer::AbstractProcessStep::toMap();
    map.insert(KeystoreLocationKey, m_keystorePath.toString());
    map.insert(BuildTargetSdkKey, m_buildTargetSdk);
    map.insert(VerboseOutputKey, m_verbose);
    map.insert(UseMinistroKey, m_useMinistro);
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
    AndroidManager::updateGradleProperties(target());
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

bool AndroidBuildApkStep::useMinistro() const
{
    return m_useMinistro;
}

void AndroidBuildApkStep::setUseMinistro(bool useMinistro)
{
    m_useMinistro = useMinistro;
}

bool AndroidBuildApkStep::addDebugger() const
{
    return m_addDebugger;
}

void AndroidBuildApkStep::setAddDebugger(bool debug)
{
    m_addDebugger = debug;
}

bool AndroidBuildApkStep::verboseOutput() const
{
    return m_verbose;
}

QAbstractItemModel *AndroidBuildApkStep::keystoreCertificates()
{
    // check keystore passwords
    if (!verifyKeystorePassword())
        return nullptr;

    CertificatesModel *model = nullptr;
    const QStringList params = {"-list", "-v", "-keystore", m_keystorePath.toUserOutput(),
        "-storepass", m_keystorePasswd, "-J-Duser.language=en"};

    Utils::SynchronousProcess keytoolProc;
    keytoolProc.setTimeoutS(30);
    const Utils::SynchronousProcessResponse response
            = keytoolProc.run(AndroidConfigurations::currentConfig().keytoolPath().toString(), params);
    if (response.result > Utils::SynchronousProcessResponse::FinishedError)
        QMessageBox::critical(nullptr, tr("Error"), tr("Failed to run keytool."));
    else
        model = new CertificatesModel(response.stdOut(), this);

    return model;
}

PasswordInputDialog::PasswordInputDialog(PasswordInputDialog::Context context,
                                         std::function<bool (const QString &)> callback,
                                         const QString &extraContextStr,
                                         QWidget *parent) :
    QDialog(parent, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint),
    verifyCallback(callback)

{
    inputEdit->setEchoMode(QLineEdit::Password);

    warningIcon->setPixmap(Utils::Icons::WARNING.pixmap());
    warningIcon->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
    warningIcon->hide();

    warningLabel->hide();

    auto warningLayout = new QHBoxLayout;
    warningLayout->addWidget(warningIcon);
    warningLayout->addWidget(warningLabel);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(inputContextlabel);
    mainLayout->addWidget(inputEdit);
    mainLayout->addLayout(warningLayout);
    mainLayout->addWidget(buttonBox);

    connect(inputEdit, &QLineEdit::textChanged,[this](const QString &text) {
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!text.isEmpty());
    });

    connect(buttonBox, &QDialogButtonBox::accepted, [this]() {
        if (verifyCallback(inputEdit->text())) {
            accept(); // Dialog accepted.
        } else {
            warningIcon->show();
            warningLabel->show();
            warningLabel->setText(tr("Incorrect password."));
            inputEdit->clear();
            adjustSize();
        }
    });

    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    setWindowTitle(context == KeystorePassword ? tr("Keystore") : tr("Certificate"));

    QString contextStr;
    if (context == KeystorePassword)
        contextStr = tr("Enter keystore password");
    else
        contextStr = tr("Enter certificate password");

    contextStr += extraContextStr.isEmpty() ? QStringLiteral(":") :
                                              QStringLiteral(" (%1):").arg(extraContextStr);
    inputContextlabel->setText(contextStr);
}

QString PasswordInputDialog::getPassword(Context context, std::function<bool (const QString &)> callback,
                                         const QString &extraContextStr, bool *ok, QWidget *parent)
{
    std::unique_ptr<PasswordInputDialog> dlg(new PasswordInputDialog(context, callback,
                                                                     extraContextStr, parent));
    bool isAccepted = dlg->exec() == QDialog::Accepted;
    if (ok)
        *ok = isAccepted;
    return isAccepted ? dlg->inputEdit->text() : "";
}


namespace Internal {

// AndroidBuildApkStepFactory

AndroidBuildApkStepFactory::AndroidBuildApkStepFactory()
{
    registerStep<AndroidBuildApkStep>(ANDROID_BUILD_APK_ID);
    setSupportedProjectType(QmakeProjectManager::Constants::QMAKEPROJECT_ID);
    setSupportedDeviceType(Constants::ANDROID_DEVICE_TYPE);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    setDisplayName(AndroidBuildApkStep::tr("Build Android APK"));
    setRepeatable(false);
}

} // namespace Internal
} // namespace Android

#include "androidbuildapkstep.moc"
