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
#include "certificatesmodel.h"

#include "javaparser.h"

#include <coreplugin/fileutils.h>
#include <coreplugin/icore.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/algorithm.h>
#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>
#include <utils/utilsicons.h>

#include <QDateTime>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QVBoxLayout>

#include <memory>

using namespace ProjectExplorer;
using namespace Utils;
using namespace Android::Internal;

namespace {
Q_LOGGING_CATEGORY(buildapkstepLog, "qtc.android.build.androidbuildapkstep", QtWarningMsg)
}

namespace Android {

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
    pp->setWorkingDirectory(bc->buildDirectory());
    Utils::Environment env = bc->environment();
    pp->setEnvironment(env);
    pp->setCommandLine({command, arguments});
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
    : AbstractProcessStep(parent, Constants::ANDROID_BUILD_APK_ID),
      m_buildTargetSdk(AndroidConfig::apiLevelNameFor(AndroidConfigurations::
                                         sdkManager()->latestAndroidSdkPlatform()))
{
    //: AndroidBuildApkStep default display name
    setDefaultDisplayName(tr("Build Android APK"));
    setImmutable(true);
}

AndroidBuildApkStep *AndroidBuildApkStep::findInBuild(const BuildConfiguration *bc)
{
    if (!bc)
        return nullptr;
    BuildStepList *bsl = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    QTC_ASSERT(bsl, return nullptr);
    return bsl->firstOfType<AndroidBuildApkStep>();
}

bool AndroidBuildApkStep::init()
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

    QtSupport::BaseQtVersion *version = QtSupport::QtKitAspect::qtVersion(target()->kit());
    if (!version)
        return false;

    const QVersionNumber sdkToolsVersion = AndroidConfigurations::currentConfig().sdkToolsVersion();
    if (sdkToolsVersion >= gradleScriptRevokedSdkVersion) {
        if (!version->sourcePath().pathAppended("src/3rdparty/gradle").exists()) {
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
                                                &Utils::FilePath::toString));

    RunConfiguration *rc = target()->activeRunConfiguration();
    const QString buildKey = rc ? rc->buildKey() : QString();
    const ProjectNode *node = rc ? target()->project()->findNodeForBuildKey(buildKey) : nullptr;

    QString sourceDirName;
    if (node)
        sourceDirName = node->data(Constants::AndroidPackageSourceDir).toString();

    QFileInfo sourceDirInfo(sourceDirName);
    parser->setSourceDirectory(Utils::FilePath::fromString(sourceDirInfo.canonicalFilePath()));
    parser->setBuildDirectory(bc->buildDirectory().pathAppended(Constants::ANDROID_BUILDDIRECTORY));
    setOutputParser(parser);

    m_openPackageLocationForRun = m_openPackageLocation;
    m_packagePath = m_buildAAB ? AndroidManager::aabPath(target()).toString()
                           : AndroidManager::apkPath(target()).toString();
    qCDebug(buildapkstepLog) << "Package path:" << m_packagePath;

    if (!AbstractProcessStep::init())
        return false;

    QString command = version->hostBinPath().toString();
    if (!command.endsWith('/'))
        command += '/';
    command += "androiddeployqt";
    if (Utils::HostOsInfo::isWindowsHost())
        command += ".exe";

    QString outputDir = bc->buildDirectory().pathAppended(Constants::ANDROID_BUILDDIRECTORY).toString();

    if (node)
        m_inputFile = node->data(Constants::AndroidDeploySettingsFile).toString();

    if (m_inputFile.isEmpty()) {
        m_skipBuilding = true;
        return true;
    }

    QString buildTargetSdk = AndroidManager::buildTargetSDK(target());
    if (buildTargetSdk.isEmpty()) {
        emit addOutput(tr("Android build SDK not defined. Check Android settings."),
                       OutputFormat::Stderr);
        return false;
    }

    QStringList arguments = {"--input", m_inputFile,
                             "--output", outputDir,
                             "--android-platform", AndroidManager::buildTargetSDK(target()),
                             "--jdk", AndroidConfigurations::currentConfig().openJDKLocation().toString()};

    if (m_verbose)
        arguments << "--verbose";

    arguments << "--gradle";

    if (m_buildAAB)
        arguments << "--aab" <<  "--jarsigner";

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
    m_command = pp2.effectiveCommand().toString();
    m_argumentsPasswordConcealed = pp2.prettyArguments();

    return true;
}

void AndroidBuildApkStep::showInGraphicalShell()
{
    Core::FileUtils::showInGraphicalShell(Core::ICore::mainWindow(), m_packagePath);
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
        emit addOutput(tr("Cannot sign the package. Invalid keystore path (%1).")
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
        emit addOutput(tr("Cannot sign the package. Certificate alias %1 does not exist.")
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


static bool copyFileIfNewer(const QString &sourceFileName,
                            const QString &destinationFileName)
{
    if (sourceFileName == destinationFileName)
        return true;
    if (QFile::exists(destinationFileName)) {
        QFileInfo destinationFileInfo(destinationFileName);
        QFileInfo sourceFileInfo(sourceFileName);
        if (sourceFileInfo.lastModified() <= destinationFileInfo.lastModified())
            return true;
        if (!QFile(destinationFileName).remove())
            return false;
    }

    if (!QDir().mkpath(QFileInfo(destinationFileName).path()))
        return false;
    return QFile::copy(sourceFileName, destinationFileName);
}

void AndroidBuildApkStep::doRun()
{
    if (m_skipBuilding) {
        emit addOutput(tr("Android deploy settings file not found, not building an APK."), BuildStep::OutputFormat::ErrorMessage);
        emit finished(true);
        return;
    }

    auto setup = [this] {
        auto bc = target()->activeBuildConfiguration();
        const auto androidAbis = AndroidManager::applicationAbis(target());
        for (const auto &abi : androidAbis) {
            Utils::FilePath androidLibsDir = bc->buildDirectory()
                    .pathAppended("android-build/libs")
                    .pathAppended(abi);
            if (!androidLibsDir.exists() && !QDir{bc->buildDirectory().toString()}.mkpath(androidLibsDir.toString()))
                return false;
        }

        RunConfiguration *rc = target()->activeRunConfiguration();
        const QString buildKey = rc ? rc->buildKey() : QString();
        const ProjectNode *node = rc ? target()->project()->findNodeForBuildKey(buildKey) : nullptr;

        if (!node)
            return false;

        bool inputExists = QFile::exists(m_inputFile);
        if (inputExists && !AndroidManager::isQtCreatorGenerated(FilePath::fromString(m_inputFile)))
            return true; // use the generated file if it was not generated by qtcreator

        auto targets = node->data(Android::Constants::AndroidTargets).toStringList();
        if (targets.isEmpty())
            return inputExists; // qmake does this job for us


        QtSupport::BaseQtVersion *version = QtSupport::QtKitAspect::qtVersion(target()->kit());
        if (!version)
            return false;

        QJsonObject deploySettings = Android::AndroidManager::deploymentSettings(target());
        QString applicationBinary;
        if (version->qtVersion() < QtSupport::QtVersionNumber(5, 14, 0)) {
            QTC_ASSERT(androidAbis.size() == 1, return false);
            applicationBinary = target()->activeRunConfiguration()->buildTargetInfo().targetFilePath.toString();
            Utils::FilePath androidLibsDir = bc->buildDirectory().pathAppended("android-build/libs").pathAppended(androidAbis.first());
            for (const auto &target : targets) {
                if (!copyFileIfNewer(target, androidLibsDir.pathAppended(QFileInfo{target}.fileName()).toString()))
                    return false;
            }
            deploySettings["target-architecture"] = androidAbis.first();
        } else {
            applicationBinary = target()->activeRunConfiguration()->buildTargetInfo().targetFilePath.toFileInfo().fileName();
            QJsonObject architectures;

            // Copy targets to android build folder
            for (const auto &abi : androidAbis) {
                QString targetSuffix = QString{"_%1.so"}.arg(abi);
                if (applicationBinary.endsWith(targetSuffix)) {
                    // Keep only TargetName from "lib[TargetName]_abi.so"
                    applicationBinary.remove(0, 3).chop(targetSuffix.size());
                }

                Utils::FilePath androidLibsDir = bc->buildDirectory()
                                                     .pathAppended("android-build/libs")
                                                     .pathAppended(abi);
                for (const auto &target : targets) {
                    if (target.endsWith(targetSuffix)) {
                        if (!copyFileIfNewer(target, androidLibsDir.pathAppended(QFileInfo{target}.fileName()).toString()))
                            return false;
                        architectures[abi] = AndroidManager::archTriplet(abi);
                    }
                }
            }
            deploySettings["architectures"] = architectures;
        }
        deploySettings["application-binary"] = applicationBinary;

        QString extraLibs = node->data(Android::Constants::AndroidExtraLibs).toString();
        if (!extraLibs.isEmpty())
            deploySettings["android-extra-libs"] = extraLibs;

        QString androidSrcs = node->data(Android::Constants::AndroidPackageSourceDir).toString();
        if (!androidSrcs.isEmpty())
            deploySettings["android-package-source-directory"] = androidSrcs;

        QString qmlImportPath = node->data("QML_IMPORT_PATH").toString();
        if (!qmlImportPath.isEmpty())
            deploySettings["qml-import-paths"] = qmlImportPath;

        QString qmlRootPath = node->data("QML_ROOT_PATH").toString();
        if (qmlRootPath.isEmpty())
            qmlRootPath = target()->project()->rootProjectDirectory().toString();
         deploySettings["qml-root-path"] = qmlRootPath;

        QFile f{m_inputFile};
        if (!f.open(QIODevice::WriteOnly))
            return false;
        f.write(QJsonDocument{deploySettings}.toJson());
        return true;
    };

    if (!setup()) {
        emit addOutput(tr("Cannot set up Android, not building an APK."), BuildStep::OutputFormat::ErrorMessage);
        emit finished(false);
        return;
    }

    AbstractProcessStep::doRun();
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
    m_keystorePath = Utils::FilePath::fromString(map.value(KeystoreLocationKey).toString());
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

Utils::FilePath AndroidBuildApkStep::keystorePath()
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
}

QVariant AndroidBuildApkStep::data(Core::Id id) const
{
    if (id == Constants::AndroidNdkPlatform)
        return AndroidConfigurations::currentConfig().bestNdkPlatformMatch(AndroidManager::minimumSDK(target())).mid(8);
    if (id == Constants::NdkLocation)
        return QVariant::fromValue(AndroidConfigurations::currentConfig().ndkLocation());
    if (id == Constants::SdkLocation)
        return QVariant::fromValue(AndroidConfigurations::currentConfig().sdkLocation());
    if (id == Constants::AndroidABIs)
        return AndroidManager::applicationAbis(target());

    return AbstractProcessStep::data(id);
}

void AndroidBuildApkStep::setKeystorePath(const Utils::FilePath &path)
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

bool AndroidBuildApkStep::buildAAB() const
{
    return m_buildAAB;
}

void AndroidBuildApkStep::setBuildAAB(bool aab)
{
    m_buildAAB = aab;
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
    const SynchronousProcessResponse response
            = keytoolProc.run({AndroidConfigurations::currentConfig().keytoolPath(), params});
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
    registerStep<AndroidBuildApkStep>(Constants::ANDROID_BUILD_APK_ID);
    setSupportedDeviceType(Constants::ANDROID_DEVICE_TYPE);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    setDisplayName(AndroidBuildApkStep::tr("Build Android APK"));
    setRepeatable(false);
}

} // namespace Internal
} // namespace Android

#include "androidbuildapkstep.moc"
