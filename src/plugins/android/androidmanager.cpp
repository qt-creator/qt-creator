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
    const QLatin1String AndroidDeviceAbis("AndroidDeviceAbis");
    const QLatin1String ApiLevelKey("AndroidVersion.ApiLevel");
    const QString packageNameRegEx("(?<token>package: )(.*?)(name=)'(?<target>.*?)'");
    const QString activityRegEx("(?<token>launchable-activity: )(.*?)(name=)'(?<target>.*?)'");
    const QString apkVersionRegEx("(?<token>package: )(.*?)(versionCode=)'(?<target>.*?)'");
    const QString versionCodeRegEx("(?<token>versionCode=)(?<version>\\d*)");

    static Q_LOGGING_CATEGORY(androidManagerLog, "qtc.android.androidManager", QtWarningMsg)

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

static bool openXmlFile(QDomDocument &doc, const Utils::FilePath &fileName);
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

QString AndroidManager::packageName(const Utils::FilePath &manifestFile)
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

void AndroidManager::apkInfo(const Utils::FilePath &apkPath,
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
    QtSupport::BaseQtVersion *version = QtSupport::QtKitAspect::qtVersion(kit);
    if (version && version->targetDeviceTypes().contains(Constants::ANDROID_DEVICE_TYPE)) {
        Utils::FilePath stockManifestFilePath = Utils::FilePath::fromUserInput(
            version->prefix().toString() + "/src/android/templates/AndroidManifest.xml");
        QDomDocument doc;
        if (openXmlFile(doc, stockManifestFilePath)) {
            minSDKVersion = parseMinSdk(doc.documentElement());
        }
    }
    return minSDKVersion;
}

QString AndroidManager::buildTargetSDK(ProjectExplorer::Target *target)
{
    if (auto bc = target->activeBuildConfiguration()) {
        if (auto androidBuildApkStep = bc->buildSteps()->firstOfType<AndroidBuildApkStep>())
            return androidBuildApkStep->buildTargetSdk();
    }

    QString fallback = AndroidConfig::apiLevelNameFor(
                AndroidConfigurations::sdkManager()->latestAndroidSdkPlatform());
    return fallback;
}

QStringList AndroidManager::applicationAbis(const Target *target)
{
    auto qt = static_cast<AndroidQtVersion *>(QtSupport::QtKitAspect::qtVersion(target->kit()));
    return qt->androidAbis();
}

QJsonObject AndroidManager::deploymentSettings(const Target *target)
{
    QtSupport::BaseQtVersion *qt = QtSupport::QtKitAspect::qtVersion(target->kit());
    if (!qt)
        return {};

    auto tc = ProjectExplorer::ToolChainKitAspect::toolChain(target->kit(), ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    if (!tc || tc->typeId() != Constants::ANDROID_TOOLCHAIN_TYPEID)
        return {};
    QJsonObject settings;
    settings["_description"] = "This file is generated by QtCreator to be read by androiddeployqt and should not be modified by hand.";
    settings["qt"] = qt->prefix().toString();
    settings["ndk"] = AndroidConfigurations::currentConfig().ndkLocation().toString();
    settings["sdk"] = AndroidConfigurations::currentConfig().sdkLocation().toString();
    settings["stdcpp-path"] = AndroidConfigurations::currentConfig().toolchainPath().pathAppended("sysroot/usr/lib/").toString();
    settings["toolchain-prefix"] =  "llvm";
    settings["tool-prefix"] = "llvm";
    settings["useLLVM"] = true;
    settings["ndk-host"] = AndroidConfigurations::currentConfig().toolchainHost();
    return settings;
}

Utils::FilePath AndroidManager::dirPath(const ProjectExplorer::Target *target)
{
    if (auto *bc = target->activeBuildConfiguration())
        return bc->buildDirectory().pathAppended(Constants::ANDROID_BUILDDIRECTORY);
    return Utils::FilePath();
}

Utils::FilePath AndroidManager::apkPath(const ProjectExplorer::Target *target)
{
    QTC_ASSERT(target, return Utils::FilePath());

    auto bc = target->activeBuildConfiguration();
    if (!bc)
        return {};
    auto buildApkStep = bc->buildSteps()->firstOfType<AndroidBuildApkStep>();
    if (!buildApkStep)
        return Utils::FilePath();

    QString apkPath("build/outputs/apk/android-build-");
    if (buildApkStep->signPackage())
        apkPath += QLatin1String("release.apk");
    else
        apkPath += QLatin1String("debug.apk");

    return dirPath(target).pathAppended(apkPath);
}

bool AndroidManager::matchedAbis(const QStringList &deviceAbis, const QStringList &appAbis)
{
    for (const auto &abi : appAbis) {
        if (deviceAbis.contains(abi))
            return true;
    }
    return false;
}

QString AndroidManager::devicePreferredAbi(const QStringList &deviceAbis, const QStringList &appAbis)
{
    for (const auto &abi : appAbis) {
        if (deviceAbis.contains(abi))
            return abi;
    }
    return {};
}

Abi AndroidManager::androidAbi2Abi(const QString &androidAbi)
{
    if (androidAbi == "arm64-v8a") {
        return Abi{Abi::Architecture::ArmArchitecture,
                   Abi::OS::LinuxOS,
                   Abi::OSFlavor::AndroidLinuxFlavor,
                   Abi::BinaryFormat::ElfFormat,
                   64, androidAbi};
    } else if (androidAbi == "armeabi-v7a") {
        return Abi{Abi::Architecture::ArmArchitecture,
                   Abi::OS::LinuxOS,
                   Abi::OSFlavor::AndroidLinuxFlavor,
                   Abi::BinaryFormat::ElfFormat,
                   32, androidAbi};
    } else if (androidAbi == "x86_64") {
        return Abi{Abi::Architecture::X86Architecture,
                   Abi::OS::LinuxOS,
                   Abi::OSFlavor::AndroidLinuxFlavor,
                   Abi::BinaryFormat::ElfFormat,
                   64, androidAbi};
    } else if (androidAbi == "x86") {
        return Abi{Abi::Architecture::X86Architecture,
                   Abi::OS::LinuxOS,
                   Abi::OSFlavor::AndroidLinuxFlavor,
                   Abi::BinaryFormat::ElfFormat,
                   32, androidAbi};
    } else {
        return Abi{Abi::Architecture::UnknownArchitecture,
                   Abi::OS::LinuxOS,
                   Abi::OSFlavor::AndroidLinuxFlavor,
                   Abi::BinaryFormat::ElfFormat,
                   0, androidAbi};
    }
}

Utils::FilePath AndroidManager::manifestSourcePath(ProjectExplorer::Target *target)
{
    if (const ProjectNode *node = currentProjectNode(target)) {
        const QString packageSource
                = node->data(Android::Constants::AndroidPackageSourceDir).toString();
        if (!packageSource.isEmpty()) {
            const FilePath manifest = FilePath::fromUserInput(packageSource + "/AndroidManifest.xml");
            if (manifest.exists())
                return manifest;
        }
    }
    return manifestPath(target);
}

Utils::FilePath AndroidManager::manifestPath(ProjectExplorer::Target *target)
{
    QVariant manifest = target->namedSettings(AndroidManifestName);
    if (manifest.isValid())
        return manifest.value<FilePath>();
    return dirPath(target).pathAppended(AndroidManifestName);
}

void AndroidManager::setManifestPath(Target *target, const FilePath &path)
{
     target->setNamedSettings(AndroidManifestName, QVariant::fromValue(path));
}

Utils::FilePath AndroidManager::defaultPropertiesPath(ProjectExplorer::Target *target)
{
    return dirPath(target).pathAppended(AndroidDefaultPropertiesName);
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

static QString preferredAbi(const QStringList &appAbis, Target *target)
{
    const auto deviceAbis = target->namedSettings(AndroidDeviceAbis).toStringList();
    for (const auto &abi : deviceAbis) {
        if (appAbis.contains(abi))
            return abi;
    }
    return {};
}

QString AndroidManager::apkDevicePreferredAbi(Target *target)
{
    auto libsPath = dirPath(target).pathAppended("libs");
    QStringList apkAbis;
    for (const auto &abi : QDir{libsPath.toString()}.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        if (QDir{libsPath.pathAppended(abi).toString()}.entryList(QStringList("*.so"), QDir::Files | QDir::NoDotAndDotDot).length())
            apkAbis << abi;
    return preferredAbi(apkAbis, target);
}

void AndroidManager::setDeviceAbis(ProjectExplorer::Target *target, const QStringList &deviceAbis)
{
    target->setNamedSettings(AndroidDeviceAbis, deviceAbis);
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
    return qMakePair(16, 29);
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
    case 29:
        return QLatin1String("Android 10");
    default:
        return tr("Unknown Android version. API Level: %1").arg(QString::number(x));
    }
}

static void raiseError(const QString &reason)
{
    QMessageBox::critical(nullptr, AndroidManager::tr("Error creating Android templates."), reason);
}

static bool openXmlFile(QDomDocument &doc, const Utils::FilePath &fileName)
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

void AndroidManager::installQASIPackage(ProjectExplorer::Target *target, const QString &packagePath)
{
    const QStringList appAbis = AndroidManager::applicationAbis(target);
    if (appAbis.isEmpty())
        return;
    const int deviceAPILevel = AndroidManager::minimumSDK(target);
    AndroidDeviceInfo info = AndroidConfigurations::showDeviceDialog(target->project(), deviceAPILevel, appAbis);
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
    const CommandLine cmd(AndroidConfigurations::currentConfig().keytoolPath(),
                          {"-list", "-keystore", keystorePath, "--storepass", keystorePasswd});
    SynchronousProcess proc;
    proc.setTimeoutS(10);
    SynchronousProcessResponse response = proc.run(cmd);
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

    SynchronousProcess proc;
    proc.setTimeoutS(10);
    SynchronousProcessResponse response
            = proc.run({AndroidConfigurations::currentConfig().keytoolPath(), arguments});
    return response.result == SynchronousProcessResponse::Finished && response.exitCode == 0;
}

bool AndroidManager::checkCertificateExists(const QString &keystorePath,
                                            const QString &keystorePasswd, const QString &alias)
{
    // assumes that the keystore password is correct
    QStringList arguments = { "-list", "-keystore", keystorePath,
                              "--storepass", keystorePasswd, "-alias", alias };

    SynchronousProcess proc;
    proc.setTimeoutS(10);
    SynchronousProcessResponse response
            = proc.run({AndroidConfigurations::currentConfig().keytoolPath(), arguments});
    return response.result == SynchronousProcessResponse::Finished && response.exitCode == 0;
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


bool AndroidManager::updateGradleProperties(ProjectExplorer::Target *target, const QString &buildKey)
{
    QtSupport::BaseQtVersion *version = QtSupport::QtKitAspect::qtVersion(target->kit());
    if (!version)
        return false;

    QString key = buildKey;
    if (key.isEmpty()) {
        // FIXME: This case is triggered from AndroidBuildApkWidget::createApplicationGroup
        // and should be avoided.
        if (RunConfiguration *rc = target->activeRunConfiguration())
            key = rc->buildKey();
    }

    QTC_ASSERT(!key.isEmpty(), return false);
    const ProjectNode *node = target->project()->findNodeForBuildKey(key);
    if (!node)
        return false;

    const QString sourceDirName = node->data(Constants::AndroidPackageSourceDir).toString();
    QFileInfo sourceDirInfo(sourceDirName);
    const FilePath packageSourceDir = FilePath::fromString(sourceDirInfo.canonicalFilePath())
            .pathAppended("gradlew");
    if (!packageSourceDir.exists())
        return false;

    const FilePath wrapperProps = packageSourceDir.pathAppended("gradle/wrapper/gradle-wrapper.properties");
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
    const FilePath localPropertiesFile = packageSourceDir.pathAppended("local.properties");
    if (!mergeGradleProperties(localPropertiesFile.toString(), localProperties))
        return false;

    const QString gradlePropertiesPath = packageSourceDir.pathAppended("gradle.properties").toString();
    GradleProperties gradleProperties = readGradleProperties(gradlePropertiesPath);
    gradleProperties["qt5AndroidDir"] = (version->prefix().toString() + "/src/android/java")
                                            .toLocal8Bit();
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

int AndroidManager::findApiLevel(const Utils::FilePath &platformPath)
{
    int apiLevel = -1;
    const Utils::FilePath propertiesPath = platformPath.pathAppended("/source.properties");
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
    qCDebug(androidManagerLog) << "Running command (async):" << CommandLine(adb, args).toUserOutput();
    p->start(adb, args);
    if (p->waitForStarted(500) && p->state() == QProcess::Running) {
        if (deleteOnFinish) {
            connect(p.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                    p.get(), &QObject::deleteLater);
        }
        return p.release();
    }

    QString errorStr = QString::fromUtf8(p->readAllStandardError());
    qCDebug(androidManagerLog) << "Running command (async) failed:"
                               << CommandLine(adb, args).toUserOutput()
                               << "Output:" << errorStr;
    if (err)
        *err = errorStr;
    return nullptr;
}

SdkToolResult AndroidManager::runCommand(const CommandLine &command,
                                         const QByteArray &writeData, int timeoutS)
{
    Android::SdkToolResult cmdResult;
    Utils::SynchronousProcess cmdProc;
    cmdProc.setTimeoutS(timeoutS);
    qCDebug(androidManagerLog) << "Running command (sync):" << command.toUserOutput();
    SynchronousProcessResponse response = cmdProc.run(command, writeData);
    cmdResult.m_stdOut = response.stdOut().trimmed();
    cmdResult.m_stdErr = response.stdErr().trimmed();
    cmdResult.m_success = response.result == Utils::SynchronousProcessResponse::Finished;
    qCDebug(androidManagerLog) << "Running command (sync) finshed:" << command.toUserOutput()
                               << "Success:" << cmdResult.m_success
                               << "Output:" << response.allRawOutput();
    if (!cmdResult.success())
        cmdResult.m_exitMessage = response.exitMessage(command.executable().toString(), timeoutS);
    return cmdResult;
}

SdkToolResult AndroidManager::runAdbCommand(const QStringList &args,
                                            const QByteArray &writeData, int timeoutS)
{
    return runCommand({AndroidConfigurations::currentConfig().adbToolPath(), args},
                      writeData, timeoutS);
}

SdkToolResult AndroidManager::runAaptCommand(const QStringList &args, int timeoutS)
{
    return runCommand({AndroidConfigurations::currentConfig().aaptToolPath(), args}, {},
                      timeoutS);
}
} // namespace Android
