/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidmanager.h"

#include "androidbuildapkstep.h"
#include "androidconstants.h"
#include "androidconfigurations.h"
#include "androidrunconfiguration.h"
#include "androidglobal.h"
#include "androidtoolchain.h"
#include "androiddeployqtstep.h"
#include "androidqtversion.h"
#include "androidavdmanager.h"
#include "androidsdkmanager.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>

#include <QDir>
#include <QFileSystemWatcher>
#include <QList>
#include <QLoggingCategory>
#include <QProcess>
#include <QRegExp>
#include <QMessageBox>
#include <QApplication>
#include <QDomDocument>
#include <QVersionNumber>
#include <QRegularExpression>

namespace {
    const QLatin1String AndroidManifestName("AndroidManifest.xml");
    const QLatin1String AndroidDefaultPropertiesName("project.properties");
    const QLatin1String AndroidDeviceSn("AndroidDeviceSerialNumber");
    const QLatin1String ApiLevelKey("AndroidVersion.ApiLevel");
    const QString packageNameRegEx("(?<token>package: )(.*?)(name=)'(?<target>.*?)'");
    const QString activityRegEx("(?<token>launchable-activity: )(.*?)(name=)'(?<target>.*?)'");
    const QString apkVersionRegEx("(?<token>package: )(.*?)(versionCode=)'(?<target>.*?)'");
    const QString versionCodeRegEx("(?<token>versionCode=)(?<version>\\d*)");

    Q_LOGGING_CATEGORY(androidManagerLog, "qtc.android.androidManager", QtWarningMsg)

    QString parseAaptOutput(const QString &output, const QString &regEx) {
        const QRegularExpression regRx(regEx,
                                       QRegularExpression::CaseInsensitiveOption |
                                       QRegularExpression::MultilineOption);
        QRegularExpressionMatch match = regRx.match(output);
        if (match.hasMatch())
            return match.captured("target");
        return QString();
    };
} // anonymous namespace


using namespace ProjectExplorer;
using namespace Utils;

namespace Android {

using namespace Internal;

class Library
{
public:
    Library()
    { level = -1; }
    int level;
    QStringList dependencies;
    QString name;
};

using LibrariesMap = QMap<QString, Library>;

static bool openXmlFile(QDomDocument &doc, const Utils::FileName &fileName);
static bool openManifest(ProjectExplorer::Target *target, QDomDocument &doc);
static int parseMinSdk(const QDomElement &manifestElem);

static const ProjectNode *currentProjectNode(Target *target)
{
    if (RunConfiguration *rc = target->activeRunConfiguration())
        return target->project()->findNodeForBuildKey(rc->buildKey());
    return nullptr;
}

QString AndroidManager::packageName(ProjectExplorer::Target *target)
{
    QDomDocument doc;
    if (!openManifest(target, doc))
        return QString();
    QDomElement manifestElem = doc.documentElement();
    return manifestElem.attribute(QLatin1String("package"));
}

QString AndroidManager::packageName(const Utils::FileName &manifestFile)
{
    QDomDocument doc;
    if (!openXmlFile(doc, manifestFile))
        return QString();
    QDomElement manifestElem = doc.documentElement();
    return manifestElem.attribute(QLatin1String("package"));
}

bool AndroidManager::packageInstalled(const QString &deviceSerial,
                                      const QString &packageName)
{
    if (deviceSerial.isEmpty() || packageName.isEmpty())
        return false;
    QStringList args = AndroidDeviceInfo::adbSelector(deviceSerial);
    args << "shell" << "pm" << "list" << "packages";
    QStringList lines = runAdbCommand(args).stdOut().split(QRegularExpression("[\\n\\r]"),
                                                           QString::SkipEmptyParts);
    for (const QString &line : lines) {
        // Don't want to confuse com.abc.xyz with com.abc.xyz.def so check with
        // endsWith
        if (line.endsWith(packageName))
            return true;
    }
    return false;
}

int AndroidManager::packageVersionCode(const QString &deviceSerial,
                                              const QString &packageName)
{
    if (deviceSerial.isEmpty() || packageName.isEmpty())
        return -1;

    QStringList args = AndroidDeviceInfo::adbSelector(deviceSerial);
    args << "shell" << "dumpsys" << "package" << packageName;
    const QRegularExpression regRx(versionCodeRegEx,
                                   QRegularExpression::CaseInsensitiveOption |
                                   QRegularExpression::MultilineOption);
    QRegularExpressionMatch match = regRx.match(runAdbCommand(args).stdOut());
    if (match.hasMatch())
        return match.captured("version").toInt();

    return -1;
}

void AndroidManager::apkInfo(const Utils::FileName &apkPath,
                                    QString *packageName,
                                    int *version,
                                    QString *activityPath)
{
    SdkToolResult result;
    result = runAaptCommand({"dump", "badging", apkPath.toString()});

    QString packageStr;
    if (activityPath) {
        packageStr = parseAaptOutput(result.stdOut(), packageNameRegEx);
        QString path = parseAaptOutput(result.stdOut(), activityRegEx);
        if (!packageStr.isEmpty() && !path.isEmpty())
            *activityPath = packageStr + '/' + path;
    }

    if (packageName) {
        *packageName = activityPath ? packageStr :
                                      parseAaptOutput(result.stdOut(), packageNameRegEx);
    }

    if (version) {
        QString versionStr = parseAaptOutput(result.stdOut(), apkVersionRegEx);
        *version = versionStr.toInt();
    }
}

QString AndroidManager::intentName(ProjectExplorer::Target *target)
{
    return packageName(target) + QLatin1Char('/') + activityName(target);
}

QString AndroidManager::activityName(ProjectExplorer::Target *target)
{
    QDomDocument doc;
    if (!openManifest(target, doc))
        return QString();
    QDomElement activityElem = doc.documentElement().firstChildElement(QLatin1String("application")).firstChildElement(QLatin1String("activity"));
    return activityElem.attribute(QLatin1String("android:name"));
}

/*!
    Returns the minimum Android API level set for the APK. Minimum API level
    of the kit is returned if the manifest file of the APK cannot be found
    or parsed.
*/
int AndroidManager::minimumSDK(ProjectExplorer::Target *target)
{
    QDomDocument doc;
    if (!openXmlFile(doc, AndroidManager::manifestSourcePath(target)))
        return minimumSDK(target->kit());
    return parseMinSdk(doc.documentElement());
}

/*!
    Returns the minimum Android API level required by the kit to compile. -1 is
    returned if the kit does not support Android.
*/
int AndroidManager::minimumSDK(const ProjectExplorer::Kit *kit)
{
    int minSDKVersion = -1;
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(kit);
    if (version && version->targetDeviceTypes().contains(Constants::ANDROID_DEVICE_TYPE)) {
        Utils::FileName stockManifestFilePath =
                Utils::FileName::fromUserInput(version->qmakeProperty("QT_INSTALL_PREFIX") +
                                               QLatin1String("/src/android/templates/AndroidManifest.xml"));
        QDomDocument doc;
        if (openXmlFile(doc, stockManifestFilePath)) {
            minSDKVersion = parseMinSdk(doc.documentElement());
        }
    }
    return minSDKVersion;
}

int AndroidManager::minimumNDK(const Kit *kit)
{
    auto qt = static_cast<AndroidQtVersion *>(QtSupport::QtKitInformation::qtVersion(kit));
    return qt->mininmumNDK();
}

QString AndroidManager::buildTargetSDK(ProjectExplorer::Target *target)
{
    if (auto androidBuildApkStep = AndroidBuildApkStep::findInBuild(target->activeBuildConfiguration()))
        return androidBuildApkStep->buildTargetSdk();

    QString fallback = AndroidConfig::apiLevelNameFor(
                AndroidConfigurations::sdkManager()->latestAndroidSdkPlatform());
    return fallback;
}

QString AndroidManager::targetArch(ProjectExplorer::Target *target)
{
    auto qt = static_cast<AndroidQtVersion *>(QtSupport::QtKitInformation::qtVersion(target->kit()));
    return qt->targetArch();
}

Utils::FileName AndroidManager::dirPath(const ProjectExplorer::Target *target)
{
    if (target->activeBuildConfiguration())
        return target->activeBuildConfiguration()->buildDirectory().appendPath(QLatin1String(Constants::ANDROID_BUILDDIRECTORY));
    return Utils::FileName();
}

Utils::FileName AndroidManager::apkPath(const ProjectExplorer::Target *target)
{
    QTC_ASSERT(target, return Utils::FileName());

    auto buildApkStep = AndroidBuildApkStep::findInBuild(target->activeBuildConfiguration());
    if (!buildApkStep)
        return Utils::FileName();

    QString apkPath("build/outputs/apk/android-build-");
    if (buildApkStep->signPackage())
        apkPath += QLatin1String("release.apk");
    else
        apkPath += QLatin1String("debug.apk");

    return dirPath(target).appendPath(apkPath);
}

Utils::FileName AndroidManager::manifestSourcePath(ProjectExplorer::Target *target)
{
    if (const ProjectNode *node = currentProjectNode(target)) {
        const QString packageSource
                = node->data(Android::Constants::AndroidPackageSourceDir).toString();
        if (!packageSource.isEmpty()) {
            const FileName manifest = FileName::fromUserInput(packageSource + "/AndroidManifest.xml");
            if (manifest.exists())
                return manifest;
        }
    }
    return manifestPath(target);
}

Utils::FileName AndroidManager::manifestPath(ProjectExplorer::Target *target)
{
    return dirPath(target).appendPath(AndroidManifestName);
}

Utils::FileName AndroidManager::defaultPropertiesPath(ProjectExplorer::Target *target)
{
    return dirPath(target).appendPath(AndroidDefaultPropertiesName);
}

QString AndroidManager::deviceSerialNumber(ProjectExplorer::Target *target)
{
    return target->namedSettings(AndroidDeviceSn).toString();
}

void AndroidManager::setDeviceSerialNumber(ProjectExplorer::Target *target, const QString &deviceSerialNumber)
{
    qCDebug(androidManagerLog) << "Device serial for the target changed"
                               << target->displayName() << deviceSerialNumber;
    target->setNamedSettings(AndroidDeviceSn, deviceSerialNumber);
}

int AndroidManager::deviceApiLevel(ProjectExplorer::Target *target)
{
    return target->namedSettings(ApiLevelKey).toInt();
}

void AndroidManager::setDeviceApiLevel(ProjectExplorer::Target *target, int level)
{
    qCDebug(androidManagerLog) << "Device API level for the target changed"
                               << target->displayName() << level;
    target->setNamedSettings(ApiLevelKey, level);
}

QPair<int, int> AndroidManager::apiLevelRange()
{
    return qMakePair(16, 28);
}

QString AndroidManager::androidNameForApiLevel(int x)
{
    switch (x) {
    case 4:
        return QLatin1String("Android 1.6");
    case 5:
        return QLatin1String("Android 2.0");
    case 6:
        return QLatin1String("Android 2.0.1");
    case 7:
        return QLatin1String("Android 2.1.x");
    case 8:
        return QLatin1String("Android 2.2.x");
    case 9:
        return QLatin1String("Android 2.3, 2.3.1, 2.3.2");
    case 10:
        return QLatin1String("Android 2.3.3, 2.3.4");
    case 11:
        return QLatin1String("Android 3.0.x");
    case 12:
        return QLatin1String("Android 3.1.x");
    case 13:
        return QLatin1String("Android 3.2");
    case 14:
        return QLatin1String("Android 4.0, 4.0.1, 4.0.2");
    case 15:
        return QLatin1String("Android 4.0.3, 4.0.4");
    case 16:
        return QLatin1String("Android 4.1, 4.1.1");
    case 17:
        return QLatin1String("Android 4.2, 4.2.2");
    case 18:
        return QLatin1String("Android 4.3");
    case 19:
        return QLatin1String("Android 4.4");
    case 20:
        return QLatin1String("Android 4.4W");
    case 21:
        return QLatin1String("Android 5.0");
    case 22:
        return QLatin1String("Android 5.1");
    case 23:
        return QLatin1String("Android 6.0");
    case 24:
        return QLatin1String("Android 7.0");
    case 25:
        return QLatin1String("Android 7.1");
    case 26:
        return QLatin1String("Android 8.0");
    case 27:
        return QLatin1String("Android 8.1");
    case 28:
        return QLatin1String("Android 9");
    default:
        return tr("Unknown Android version. API Level: %1").arg(QString::number(x));
    }
}

static void raiseError(const QString &reason)
{
    QMessageBox::critical(nullptr, AndroidManager::tr("Error creating Android templates."), reason);
}

static bool openXmlFile(QDomDocument &doc, const Utils::FileName &fileName)
{
    QFile f(fileName.toString());
    if (!f.open(QIODevice::ReadOnly))
        return false;

    if (!doc.setContent(f.readAll())) {
        raiseError(AndroidManager::tr("Cannot parse \"%1\".").arg(fileName.toUserOutput()));
        return false;
    }
    return true;
}

static bool openManifest(ProjectExplorer::Target *target, QDomDocument &doc)
{
    return openXmlFile(doc, AndroidManager::manifestPath(target));
}

static int parseMinSdk(const QDomElement &manifestElem)
{
    QDomElement usesSdk = manifestElem.firstChildElement(QLatin1String("uses-sdk"));
    if (usesSdk.isNull())
        return 0;
    if (usesSdk.hasAttribute(QLatin1String("android:minSdkVersion"))) {
        bool ok;
        int tmp = usesSdk.attribute(QLatin1String("android:minSdkVersion")).toInt(&ok);
        if (ok)
            return tmp;
    }
    return 0;
}

void AndroidManager::cleanLibsOnDevice(ProjectExplorer::Target *target)
{
    const QString targetArch = AndroidManager::targetArch(target);
    if (targetArch.isEmpty())
        return;
    const int deviceAPILevel = AndroidManager::minimumSDK(target);
    AndroidDeviceInfo info = AndroidConfigurations::showDeviceDialog(target->project(), deviceAPILevel, targetArch);
    if (!info.isValid()) // aborted
        return;

    QString deviceSerialNumber = info.serialNumber;

    if (info.type == AndroidDeviceInfo::Emulator) {
        deviceSerialNumber = AndroidAvdManager().startAvd(info.avdname);
        if (deviceSerialNumber.isEmpty())
            Core::MessageManager::write(tr("Starting Android virtual device failed."));
    }

    QStringList arguments = AndroidDeviceInfo::adbSelector(deviceSerialNumber);
    arguments << "shell" << "rm" << "-r" << "/data/local/tmp/qt";

    QString error;
    if (!runAdbCommandDetached(arguments, &error))
        Core::MessageManager::write(tr("Cleaning Qt libraries on device failed.\n%1").arg(error));
}

void AndroidManager::installQASIPackage(ProjectExplorer::Target *target, const QString &packagePath)
{
    const QString targetArch = AndroidManager::targetArch(target);
    if (targetArch.isEmpty())
        return;
    const int deviceAPILevel = AndroidManager::minimumSDK(target);
    AndroidDeviceInfo info = AndroidConfigurations::showDeviceDialog(target->project(), deviceAPILevel, targetArch);
    if (!info.isValid()) // aborted
        return;

    QString deviceSerialNumber = info.serialNumber;
    if (info.type == AndroidDeviceInfo::Emulator) {
        deviceSerialNumber = AndroidAvdManager().startAvd(info.avdname);
        if (deviceSerialNumber.isEmpty())
            Core::MessageManager::write(tr("Starting Android virtual device failed."));
    }

    QStringList arguments = AndroidDeviceInfo::adbSelector(deviceSerialNumber);
    arguments << "install" << "-r " << packagePath;
    QString error;
    if (!runAdbCommandDetached(arguments, &error, true))
        Core::MessageManager::write(tr("Android package installation failed.\n%1").arg(error));
}

bool AndroidManager::checkKeystorePassword(const QString &keystorePath, const QString &keystorePasswd)
{
    if (keystorePasswd.isEmpty())
        return false;
    QStringList arguments;
    arguments << QLatin1String("-list")
              << QLatin1String("-keystore")
              << keystorePath
              << QLatin1String("--storepass")
              << keystorePasswd;
    Utils::SynchronousProcess proc;
    proc.setTimeoutS(10);
    Utils::SynchronousProcessResponse response = proc.run(AndroidConfigurations::currentConfig().keytoolPath().toString(), arguments);
    return (response.result == Utils::SynchronousProcessResponse::Finished && response.exitCode == 0);
}

bool AndroidManager::checkCertificatePassword(const QString &keystorePath, const QString &keystorePasswd, const QString &alias, const QString &certificatePasswd)
{
    // assumes that the keystore password is correct
    QStringList arguments = {"-certreq", "-keystore", keystorePath,
                             "--storepass", keystorePasswd, "-alias", alias, "-keypass"};
    if (certificatePasswd.isEmpty())
        arguments << keystorePasswd;
    else
        arguments << certificatePasswd;

    Utils::SynchronousProcess proc;
    proc.setTimeoutS(10);
    Utils::SynchronousProcessResponse response
            = proc.run(AndroidConfigurations::currentConfig().keytoolPath().toString(), arguments);
    return response.result == Utils::SynchronousProcessResponse::Finished && response.exitCode == 0;
}

bool AndroidManager::checkCertificateExists(const QString &keystorePath,
                                            const QString &keystorePasswd, const QString &alias)
{
    // assumes that the keystore password is correct
    QStringList arguments = { "-list", "-keystore", keystorePath,
                              "--storepass", keystorePasswd, "-alias", alias };

    Utils::SynchronousProcess proc;
    proc.setTimeoutS(10);
    Utils::SynchronousProcessResponse response
            = proc.run(AndroidConfigurations::currentConfig().keytoolPath().toString(), arguments);
    return response.result == Utils::SynchronousProcessResponse::Finished && response.exitCode == 0;
}

using GradleProperties = QMap<QByteArray, QByteArray>;

static GradleProperties readGradleProperties(const QString &path)
{
    GradleProperties properties;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return properties;

    foreach (const QByteArray &line, file.readAll().split('\n')) {
        if (line.trimmed().startsWith('#'))
            continue;

        QList<QByteArray> prop(line.split('='));
        if (prop.size() > 1)
            properties[prop.at(0).trimmed()] = prop.at(1).trimmed();
    }
    file.close();
    return properties;
}

static bool mergeGradleProperties(const QString &path, GradleProperties properties)
{
    QFile::remove(path + QLatin1Char('~'));
    QFile::rename(path, path + QLatin1Char('~'));
    QFile file(path);
    if (!file.open(QIODevice::Truncate | QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QFile oldFile(path + QLatin1Char('~'));
    if (oldFile.open(QIODevice::ReadOnly)) {
        while (!oldFile.atEnd()) {
            QByteArray line(oldFile.readLine());
            QList<QByteArray> prop(line.split('='));
            if (prop.size() > 1) {
                GradleProperties::iterator it = properties.find(prop.at(0).trimmed());
                if (it != properties.end()) {
                    file.write(it.key() + '=' + it.value() + '\n');
                    properties.erase(it);
                    continue;
                }
            }
            file.write(line);
        }
        oldFile.close();
    } else {
        file.write("## This file is automatically generated by QtCreator.\n"
                   "#\n"
                   "# This file must *NOT* be checked into Version Control Systems,\n"
                   "# as it contains information specific to your local configuration.\n\n");

    }

    for (GradleProperties::const_iterator it = properties.constBegin(); it != properties.constEnd();
         ++it)
        file.write(it.key() + '=' + it.value() + '\n');

    file.close();
    return true;
}


bool AndroidManager::updateGradleProperties(ProjectExplorer::Target *target)
{
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target->kit());
    if (!version)
        return false;

    const ProjectNode *node = currentProjectNode(target);
    if (!node)
        return false;

    QFileInfo sourceDirInfo(node->data(Constants::AndroidPackageSourceDir).toString());
    FileName packageSourceDir = FileName::fromString(sourceDirInfo.canonicalFilePath());
    if (!packageSourceDir.appendPath("gradlew").exists())
        return false;

    Utils::FileName wrapperProps = packageSourceDir;
    wrapperProps.appendPath(QLatin1String("gradle/wrapper/gradle-wrapper.properties"));
    if (wrapperProps.exists()) {
        GradleProperties wrapperProperties = readGradleProperties(wrapperProps.toString());
        QString distributionUrl = QString::fromLocal8Bit(wrapperProperties["distributionUrl"]);
        // Update only old gradle distributionUrl
        if (distributionUrl.endsWith(QLatin1String("distributions/gradle-1.12-all.zip"))) {
            wrapperProperties["distributionUrl"] = "https\\://services.gradle.org/distributions/gradle-2.2.1-all.zip";
            mergeGradleProperties(wrapperProps.toString(), wrapperProperties);
        }
    }

    GradleProperties localProperties;
    localProperties["sdk.dir"] = AndroidConfigurations::currentConfig().sdkLocation().toString().toLocal8Bit();
    if (!mergeGradleProperties(packageSourceDir.appendPath("local.properties").toString(), localProperties))
        return false;

    QString gradlePropertiesPath = packageSourceDir.appendPath("gradle.properties").toString();
    GradleProperties gradleProperties = readGradleProperties(gradlePropertiesPath);
    gradleProperties["qt5AndroidDir"] = version->qmakeProperty("QT_INSTALL_PREFIX")
            .append(QLatin1String("/src/android/java")).toLocal8Bit();
    gradleProperties["buildDir"] = ".build";
    gradleProperties["androidCompileSdkVersion"] = buildTargetSDK(target).split(QLatin1Char('-')).last().toLocal8Bit();
    if (gradleProperties["androidBuildToolsVersion"].isEmpty()) {
        QVersionNumber buildtoolVersion = AndroidConfigurations::currentConfig().buildToolsVersion();
        if (buildtoolVersion.isNull())
            return false;
        gradleProperties["androidBuildToolsVersion"] = buildtoolVersion.toString().toLocal8Bit();
    }
    return mergeGradleProperties(gradlePropertiesPath, gradleProperties);
}

int AndroidManager::findApiLevel(const Utils::FileName &platformPath)
{
    int apiLevel = -1;
    Utils::FileName propertiesPath = platformPath;
    propertiesPath.appendPath("/source.properties");
    if (propertiesPath.exists()) {
        QSettings sdkProperties(propertiesPath.toString(), QSettings::IniFormat);
        bool validInt = false;
        apiLevel = sdkProperties.value(ApiLevelKey).toInt(&validInt);
        if (!validInt)
            apiLevel = -1;
    }
    return apiLevel;
}

QProcess *AndroidManager::runAdbCommandDetached(const QStringList &args, QString *err,
                                                bool deleteOnFinish)
{
    std::unique_ptr<QProcess> p(new QProcess);
    const QString adb = AndroidConfigurations::currentConfig().adbToolPath().toString();
    qCDebug(androidManagerLog) << "Running command:" << adb << args.join(' ');
    p->start(adb, args);
    if (p->waitForStarted(500) && p->state() == QProcess::Running) {
        if (deleteOnFinish) {
            connect(p.get(), static_cast<void (QProcess::*)(int)>(&QProcess::finished),
                    p.get(), &QObject::deleteLater);
        }
        return p.release();
    }

    QString errorStr = QString::fromUtf8(p->readAllStandardError());
    qCDebug(androidManagerLog) << "Running command failed" << adb << args.join(' ') << errorStr;
    if (err)
        *err = errorStr;
    return nullptr;
}

SdkToolResult AndroidManager::runCommand(const QString &executable, const QStringList &args,
                                         const QByteArray &writeData, int timeoutS)
{
    Android::SdkToolResult cmdResult;
    Utils::SynchronousProcess cmdProc;
    cmdProc.setTimeoutS(timeoutS);
    qCDebug(androidManagerLog) << "Running command: " << executable << args.join(' ');
    Utils::SynchronousProcessResponse response = cmdProc.run(executable, args, writeData);
    cmdResult.m_stdOut = response.stdOut().trimmed();
    cmdResult.m_stdErr = response.stdErr().trimmed();
    cmdResult.m_success = response.result == Utils::SynchronousProcessResponse::Finished;
    qCDebug(androidManagerLog) << "Running command finshed:" << executable << args.join(' ')
                               << "Success:" << cmdResult.m_success
                               << "Output:" << response.allRawOutput();
    if (!cmdResult.success())
        cmdResult.m_exitMessage = response.exitMessage(executable, timeoutS);
    return cmdResult;
}

SdkToolResult AndroidManager::runAdbCommand(const QStringList &args,
                                            const QByteArray &writeData, int timeoutS)
{
    return runCommand(AndroidConfigurations::currentConfig().adbToolPath().toString(), args,
                      writeData, timeoutS);
}

SdkToolResult AndroidManager::runAaptCommand(const QStringList &args, int timeoutS)
{
    return runCommand(AndroidConfigurations::currentConfig().aaptToolPath().toString(), args, {},
                      timeoutS);
}
} // namespace Android
