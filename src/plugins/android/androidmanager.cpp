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
#include "androidconstants.h"
#include "androiddeployconfiguration.h"
#include "androidconfigurations.h"
#include "androidrunconfiguration.h"
#include "androidglobal.h"
#include "androidtoolchain.h"
#include "androiddeployqtstep.h"
#include "androidqtsupport.h"
#include "androidqtversion.h"
#include "androidbuildapkstep.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <utils/algorithm.h>
#include <utils/synchronousprocess.h>

#include <QDir>
#include <QFileSystemWatcher>
#include <QList>
#include <QProcess>
#include <QRegExp>
#include <QMessageBox>
#include <QApplication>
#include <QDomDocument>

namespace {
    const QLatin1String AndroidManifestName("AndroidManifest.xml");
    const QLatin1String AndroidDefaultPropertiesName("project.properties");
    const QLatin1String AndroidDeviceSn("AndroidDeviceSerialNumber");

} // anonymous namespace

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

typedef QMap<QString, Library> LibrariesMap;

static bool openXmlFile(QDomDocument &doc, const Utils::FileName &fileName);
static bool openManifest(ProjectExplorer::Target *target, QDomDocument &doc);
static int parseMinSdk(const QDomElement &manifestElem);

bool AndroidManager::supportsAndroid(const ProjectExplorer::Kit *kit)
{
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(kit);
    return version && version->targetDeviceTypes().contains(Constants::ANDROID_DEVICE_TYPE);
}

bool AndroidManager::supportsAndroid(const ProjectExplorer::Target *target)
{
    return supportsAndroid(target->kit());
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
    of the kit is returned if the manifest file of the APK can not be found
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
    if (supportsAndroid(kit)) {
        QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(kit);
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

QString AndroidManager::buildTargetSDK(ProjectExplorer::Target *target)
{
    AndroidBuildApkStep *androidBuildApkStep
            = AndroidGlobal::buildStep<AndroidBuildApkStep>(target->activeBuildConfiguration());
    if (androidBuildApkStep)
        return androidBuildApkStep->buildTargetSdk();

    QString fallback = AndroidConfig::apiLevelNameFor(AndroidConfigurations::currentConfig().highestAndroidSdk());
    return fallback;
}

bool AndroidManager::signPackage(ProjectExplorer::Target *target)
{
    AndroidBuildApkStep *androidBuildApkStep
            = AndroidGlobal::buildStep<AndroidBuildApkStep>(target->activeBuildConfiguration());
    if (androidBuildApkStep)
        return androidBuildApkStep->signPackage();
    return false;
}

QString AndroidManager::targetArch(ProjectExplorer::Target *target)
{
    AndroidQtVersion *qt = static_cast<AndroidQtVersion *>(QtSupport::QtKitInformation::qtVersion(target->kit()));
    return qt->targetArch();
}

Utils::FileName AndroidManager::dirPath(ProjectExplorer::Target *target)
{
    if (target->activeBuildConfiguration())
        return target->activeBuildConfiguration()->buildDirectory().appendPath(QLatin1String(Constants::ANDROID_BUILDDIRECTORY));
    return Utils::FileName();
}

Utils::FileName AndroidManager::manifestSourcePath(ProjectExplorer::Target *target)
{
    AndroidQtSupport *androidQtSupport = AndroidManager::androidQtSupport(target);
    Utils::FileName source = androidQtSupport->manifestSourcePath(target);
    if (!source.isEmpty())
        return source;
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

bool AndroidManager::bundleQt(ProjectExplorer::Target *target)
{
    AndroidBuildApkStep *androidBuildApkStep
            = AndroidGlobal::buildStep<AndroidBuildApkStep>(target->activeBuildConfiguration());
    if (androidBuildApkStep)
        return androidBuildApkStep->deployAction() == AndroidBuildApkStep::BundleLibrariesDeployment;

    return false;
}

QString AndroidManager::deviceSerialNumber(ProjectExplorer::Target *target)
{
    return target->namedSettings(AndroidDeviceSn).toString();
}

void AndroidManager::setDeviceSerialNumber(ProjectExplorer::Target *target, const QString &deviceSerialNumber)
{
    target->setNamedSettings(AndroidDeviceSn, deviceSerialNumber);
}

QPair<int, int> AndroidManager::apiLevelRange()
{
    return qMakePair(9, 23);
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
    default:
        return tr("Unknown Android version. API Level: %1").arg(QString::number(x));
    }
}

static void raiseError(const QString &reason)
{
    QMessageBox::critical(0, AndroidManager::tr("Error creating Android templates."), reason);
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
    AndroidDeviceInfo info = AndroidConfigurations::showDeviceDialog(target->project(), deviceAPILevel, targetArch, AndroidConfigurations::None);
    if (!info.isValid()) // aborted
        return;

    QString deviceSerialNumber = info.serialNumber;

    if (info.type == AndroidDeviceInfo::Emulator) {
        deviceSerialNumber = AndroidConfigurations::currentConfig().startAVD(info.avdname);
        if (deviceSerialNumber.isEmpty())
            Core::MessageManager::write(tr("Starting Android virtual device failed."));
    }

    QProcess *process = new QProcess();
    QStringList arguments = AndroidDeviceInfo::adbSelector(deviceSerialNumber);
    arguments << QLatin1String("shell") << QLatin1String("rm") << QLatin1String("-r") << QLatin1String("/data/local/tmp/qt");
    QObject::connect(process, static_cast<void (QProcess::*)(int)>(&QProcess::finished),
                     process, &QObject::deleteLater);
    const QString adb = AndroidConfigurations::currentConfig().adbToolPath().toString();
    Core::MessageManager::write(adb + QLatin1Char(' ') + arguments.join(QLatin1Char(' ')));
    process->start(adb, arguments);
    if (!process->waitForStarted(500))
        delete process;
}

void AndroidManager::installQASIPackage(ProjectExplorer::Target *target, const QString &packagePath)
{
    const QString targetArch = AndroidManager::targetArch(target);
    if (targetArch.isEmpty())
        return;
    const int deviceAPILevel = AndroidManager::minimumSDK(target);
    AndroidDeviceInfo info = AndroidConfigurations::showDeviceDialog(target->project(), deviceAPILevel, targetArch, AndroidConfigurations::None);
    if (!info.isValid()) // aborted
        return;

    QString deviceSerialNumber = info.serialNumber;
    if (info.type == AndroidDeviceInfo::Emulator) {
        deviceSerialNumber = AndroidConfigurations::currentConfig().startAVD(info.avdname);
        if (deviceSerialNumber.isEmpty())
            Core::MessageManager::write(tr("Starting Android virtual device failed."));
    }

    QProcess *process = new QProcess();
    QStringList arguments = AndroidDeviceInfo::adbSelector(deviceSerialNumber);
    arguments << QLatin1String("install") << QLatin1String("-r ") << packagePath;

    connect(process, static_cast<void (QProcess::*)(int)>(&QProcess::finished),
            process, &QObject::deleteLater);
    const QString adb = AndroidConfigurations::currentConfig().adbToolPath().toString();
    Core::MessageManager::write(adb + QLatin1Char(' ') + arguments.join(QLatin1Char(' ')));
    process->start(adb, arguments);
    if (!process->waitForStarted(500) && process->state() != QProcess::Running)
        delete process;
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

bool AndroidManager::checkForQt51Files(Utils::FileName fileName)
{
    fileName.appendPath(QLatin1String("android")).appendPath(QLatin1String("version.xml"));
    if (!fileName.exists())
        return false;
    QDomDocument dstVersionDoc;
    if (!openXmlFile(dstVersionDoc, fileName))
        return false;
    return dstVersionDoc.documentElement().attribute(QLatin1String("value")).toDouble() < 5.2;
}

AndroidQtSupport *AndroidManager::androidQtSupport(ProjectExplorer::Target *target)
{
    QList<AndroidQtSupport *> providerList = ExtensionSystem::PluginManager::getObjects<AndroidQtSupport>();
    foreach (AndroidQtSupport *provider, providerList) {
        if (provider->canHandle(target))
            return provider;
    }
    return 0;
}

bool AndroidManager::useGradle(ProjectExplorer::Target *target)
{
    if (!target)
        return false;
    AndroidBuildApkStep *buildApkStep
        = AndroidGlobal::buildStep<AndroidBuildApkStep>(target->activeBuildConfiguration());
    return buildApkStep && buildApkStep->useGradle();
}

typedef QMap<QByteArray, QByteArray> GradleProperties;

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

    AndroidBuildApkStep *buildApkStep
        = AndroidGlobal::buildStep<AndroidBuildApkStep>(target->activeBuildConfiguration());

    if (!buildApkStep || !buildApkStep->useGradle() || !buildApkStep->androidPackageSourceDir().appendPath(QLatin1String("gradlew")).exists())
        return false;

    Utils::FileName wrapperProps(buildApkStep->androidPackageSourceDir());
    wrapperProps.appendPath(QLatin1String("gradle/wrapper/gradle-wrapper.properties"));
    if (wrapperProps.exists()) {
        GradleProperties wrapperProperties = readGradleProperties(wrapperProps.toString());
        QString distributionUrl = QString::fromLocal8Bit(wrapperProperties["distributionUrl"]);
        QRegExp re(QLatin1String(".*services.gradle.org/distributions/gradle-2..*.zip"));
        if (!re.exactMatch(distributionUrl)) {
            wrapperProperties["distributionUrl"] = "https\\://services.gradle.org/distributions/gradle-2.2.1-all.zip";
            mergeGradleProperties(wrapperProps.toString(), wrapperProperties);
        }
    }

    GradleProperties localProperties;
    localProperties["sdk.dir"] = AndroidConfigurations::currentConfig().sdkLocation().toString().toLocal8Bit();
    if (!mergeGradleProperties(buildApkStep->androidPackageSourceDir().appendPath(QLatin1String("local.properties")).toString(), localProperties))
        return false;

    QString gradlePropertiesPath = buildApkStep->androidPackageSourceDir().appendPath(QLatin1String("gradle.properties")).toString();
    GradleProperties gradleProperties = readGradleProperties(gradlePropertiesPath);
    gradleProperties["qt5AndroidDir"] = version->qmakeProperty("QT_INSTALL_PREFIX")
            .append(QLatin1String("/src/android/java")).toLocal8Bit();
    gradleProperties["buildDir"] = ".build";
    gradleProperties["androidCompileSdkVersion"] = buildTargetSDK(target).split(QLatin1Char('-')).last().toLocal8Bit();
    if (gradleProperties["androidBuildToolsVersion"].isEmpty()) {
        QString maxVersion;
        QDir buildToolsDir(AndroidConfigurations::currentConfig().sdkLocation().appendPath(QLatin1String("build-tools")).toString());
        foreach (const QFileInfo &file, buildToolsDir.entryList(QDir::Dirs|QDir::NoDotAndDotDot)) {
            QString ver(file.fileName());
            if (maxVersion < ver)
                maxVersion = ver;
        }
        if (maxVersion.isEmpty())
            return false;
        gradleProperties["androidBuildToolsVersion"] = maxVersion.toLocal8Bit();
    }
    return mergeGradleProperties(gradlePropertiesPath, gradleProperties);
}

} // namespace Android
