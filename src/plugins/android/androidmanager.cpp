/**************************************************************************
**
** Copyright (C) 2015 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

#include <qtsupport/customexecutablerunconfiguration.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <utils/algorithm.h>

#include <QDir>
#include <QFileSystemWatcher>
#include <QList>
#include <QProcess>
#include <QMessageBox>
#include <QApplication>
#include <QDomDocument>

namespace {
    const QLatin1String AndroidDirName("android");
    const QLatin1String AndroidManifestName("AndroidManifest.xml");
    const QLatin1String AndroidLibsFileName("/res/values/libs.xml");
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
static QStringList libsXml(ProjectExplorer::Target *target, const QString &tag);

enum ItemType
{
    Lib,
    Jar,
    BundledFile,
    BundledJar
};
static QString loadLocal(ProjectExplorer::Target *target, int apiLevel, ItemType item, const QString &attribute=QLatin1String("file"));


bool AndroidManager::supportsAndroid(const ProjectExplorer::Kit *kit)
{
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(kit);
    return version && version->platformName() == QLatin1String(QtSupport::Constants::ANDROID_PLATFORM);
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

int AndroidManager::minimumSDK(ProjectExplorer::Target *target)
{
    QDomDocument doc;
    if (!openXmlFile(doc, AndroidManager::manifestSourcePath(target)))
        return 0;
    QDomElement manifestElem = doc.documentElement();
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

Utils::FileName AndroidManager::libsPath(ProjectExplorer::Target *target)
{
    return dirPath(target).appendPath(AndroidLibsFileName);
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

bool AndroidManager::useLocalLibs(ProjectExplorer::Target *target)
{
    AndroidBuildApkStep *androidBuildApkStep
            = AndroidGlobal::buildStep<AndroidBuildApkStep>(target->activeBuildConfiguration());
    if (androidBuildApkStep) {
        return androidBuildApkStep->deployAction() == AndroidBuildApkStep::DebugDeployment
                || androidBuildApkStep->deployAction() == AndroidBuildApkStep::BundleLibrariesDeployment;
    }

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

Utils::FileName AndroidManager::localLibsRulesFilePath(ProjectExplorer::Target *target)
{
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target->kit());
    if (!version)
        return Utils::FileName();
    return Utils::FileName::fromString(version->qmakeProperty("QT_INSTALL_LIBS"));
}

QString AndroidManager::loadLocalLibs(ProjectExplorer::Target *target, int apiLevel)
{
    return loadLocal(target, apiLevel, Lib);
}

QString AndroidManager::loadLocalJars(ProjectExplorer::Target *target, int apiLevel)
{
    ItemType type = bundleQt(target) ? BundledJar : Jar;
    return loadLocal(target, apiLevel, type);
}

QString AndroidManager::loadLocalJarsInitClasses(ProjectExplorer::Target *target, int apiLevel)
{
    ItemType type = bundleQt(target) ? BundledJar : Jar;
    return loadLocal(target, apiLevel, type, QLatin1String("initClass"));
}

QPair<int, int> AndroidManager::apiLevelRange()
{
    return qMakePair(9, 21);
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
    default:
        return tr("Unknown Android version. API Level: %1").arg(QString::number(x));
    }
}

QStringList AndroidManager::qtLibs(ProjectExplorer::Target *target)
{
    return libsXml(target, QLatin1String("qt_libs"));
}

QStringList AndroidManager::prebundledLibs(ProjectExplorer::Target *target)
{
    return libsXml(target, QLatin1String("bundled_libs"));
}

static bool openLibsXml(ProjectExplorer::Target *target, QDomDocument &doc)
{
    return openXmlFile(doc, AndroidManager::libsPath(target));
}

static void raiseError(const QString &reason)
{
    QMessageBox::critical(0, AndroidManager::tr("Error creating Android templates."), reason);
}

static QString loadLocal(ProjectExplorer::Target *target, int apiLevel, ItemType item, const QString &attribute)
{
    QString itemType;
    if (item == Lib)
        itemType = QLatin1String("lib");
    else if (item == BundledFile)
        itemType = QLatin1String("bundled");
    else // Jar or BundledJar
        itemType = QLatin1String("jar");

    QString localLibs;

    QDir rulesFilesDir(AndroidManager::localLibsRulesFilePath(target).toString());
    if (!rulesFilesDir.exists())
        return localLibs;

    QStringList libs;
    libs << AndroidManager::qtLibs(target) << AndroidManager::prebundledLibs(target);

    QFileInfoList rulesFiles = rulesFilesDir.entryInfoList(QStringList() << QLatin1String("*.xml"),
                                                           QDir::Files | QDir::Readable);

    QStringList dependencyLibs;
    QStringList replacedLibs;
    foreach (QFileInfo rulesFile, rulesFiles) {
        if (rulesFile.baseName() != QLatin1String("rules")
                && !rulesFile.baseName().endsWith(QLatin1String("-android-dependencies"))) {
            continue;
        }

        QDomDocument doc;
        if (!openXmlFile(doc, Utils::FileName::fromString(rulesFile.absoluteFilePath())))
            return localLibs;

        QDomElement element = doc.documentElement().firstChildElement(QLatin1String("platforms")).firstChildElement(itemType + QLatin1Char('s')).firstChildElement(QLatin1String("version"));
        while (!element.isNull()) {
            if (element.attribute(QLatin1String("value")).toInt() == apiLevel) {
                if (element.hasAttribute(QLatin1String("symlink")))
                    apiLevel = element.attribute(QLatin1String("symlink")).toInt();
                break;
            }
            element = element.nextSiblingElement(QLatin1String("version"));
        }

        element = doc.documentElement().firstChildElement(QLatin1String("dependencies")).firstChildElement(QLatin1String("lib"));
        while (!element.isNull()) {
            if (libs.contains(element.attribute(QLatin1String("name")))) {
                QDomElement libElement = element.firstChildElement(QLatin1String("depends")).firstChildElement(itemType);
                while (!libElement.isNull()) {
                    if (libElement.attribute(QLatin1String("bundling")).toInt() == (item == BundledJar ? 1 : 0)) {
                        if (libElement.hasAttribute(attribute)) {
                            QString dependencyLib = libElement.attribute(attribute);
                            if (dependencyLib.contains(QLatin1String("%1")))
                                dependencyLib = dependencyLib.arg(apiLevel);
                            if (libElement.hasAttribute(QLatin1String("extends"))) {
                                const QString extends = libElement.attribute(QLatin1String("extends"));
                                if (libs.contains(extends))
                                    dependencyLibs << dependencyLib;
                            } else if (!dependencyLibs.contains(dependencyLib)) {
                                dependencyLibs << dependencyLib;
                            }
                        }

                        if (libElement.hasAttribute(QLatin1String("replaces"))) {
                            QString replacedLib = libElement.attribute(QLatin1String("replaces"));
                            if (replacedLib.contains(QLatin1String("%1")))
                                replacedLib = replacedLib.arg(apiLevel);
                            if (!replacedLibs.contains(replacedLib))
                                replacedLibs << replacedLib;
                        }
                    }

                    libElement = libElement.nextSiblingElement(itemType);
                }

                libElement = element.firstChildElement(QLatin1String("replaces")).firstChildElement(itemType);
                while (!libElement.isNull()) {
                    if (libElement.hasAttribute(attribute)) {
                        QString replacedLib = libElement.attribute(attribute).arg(apiLevel);
                        if (!replacedLibs.contains(replacedLib))
                            replacedLibs << replacedLib;
                    }

                    libElement = libElement.nextSiblingElement(itemType);
                }
            }
            element = element.nextSiblingElement(QLatin1String("lib"));
        }
    }

    // The next loop requires all library names to end with a ":" so we append one
    // to the end after joining.
    localLibs = dependencyLibs.join(QLatin1Char(':')) + QLatin1Char(':');
    foreach (QString replacedLib, replacedLibs)
        localLibs.remove(replacedLib + QLatin1Char(':'));

    return localLibs;
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

static QStringList libsXml(ProjectExplorer::Target *target, const QString &tag)
{
    QStringList libs;
    QDomDocument doc;
    if (!openLibsXml(target, doc))
        return libs;
    QDomElement arrayElem = doc.documentElement().firstChildElement(QLatin1String("array"));
    while (!arrayElem.isNull()) {
        if (arrayElem.attribute(QLatin1String("name")) == tag) {
            arrayElem = arrayElem.firstChildElement(QLatin1String("item"));
            while (!arrayElem.isNull()) {
                libs << arrayElem.text();
                arrayElem = arrayElem.nextSiblingElement(QLatin1String("item"));
            }
            return libs;
        }
        arrayElem = arrayElem.nextSiblingElement(QLatin1String("array"));
    }
    return libs;
}

void AndroidManager::cleanLibsOnDevice(ProjectExplorer::Target *target)
{
    const QString targetArch = AndroidManager::targetArch(target);
    if (targetArch.isEmpty())
        return;
    int deviceAPILevel = AndroidManager::minimumSDK(target);
    AndroidDeviceInfo info = AndroidConfigurations::showDeviceDialog(target->project(), deviceAPILevel, targetArch, AndroidConfigurations::None);
    if (info.serialNumber.isEmpty()) // aborted
        return;

    deviceAPILevel = info.sdk;
    QString deviceSerialNumber = info.serialNumber;

    if (info.type == AndroidDeviceInfo::Emulator) {
        deviceSerialNumber = AndroidConfigurations::currentConfig().startAVD(deviceSerialNumber, deviceAPILevel, targetArch);
        if (deviceSerialNumber.isEmpty())
            Core::MessageManager::write(tr("Starting Android virtual device failed."));
    }

    QProcess *process = new QProcess();
    QStringList arguments = AndroidDeviceInfo::adbSelector(deviceSerialNumber);
    arguments << QLatin1String("shell") << QLatin1String("rm") << QLatin1String("-r") << QLatin1String("/data/local/tmp/qt");
    process->connect(process, SIGNAL(finished(int)), process, SLOT(deleteLater()));
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
    int deviceAPILevel = AndroidManager::minimumSDK(target);
    AndroidDeviceInfo info = AndroidConfigurations::showDeviceDialog(target->project(), deviceAPILevel, targetArch, AndroidConfigurations::None);
    if (info.serialNumber.isEmpty()) // aborted
        return;

    deviceAPILevel = info.sdk;
    QString deviceSerialNumber = info.serialNumber;
    if (info.type == AndroidDeviceInfo::Emulator) {
        deviceSerialNumber = AndroidConfigurations::currentConfig().startAVD(deviceSerialNumber, deviceAPILevel, targetArch);
        if (deviceSerialNumber.isEmpty())
            Core::MessageManager::write(tr("Starting Android virtual device failed."));
    }

    QProcess *process = new QProcess();
    QStringList arguments = AndroidDeviceInfo::adbSelector(deviceSerialNumber);
    arguments << QLatin1String("install") << QLatin1String("-r ") << packagePath;

    process->connect(process, SIGNAL(finished(int)), process, SLOT(deleteLater()));
    const QString adb = AndroidConfigurations::currentConfig().adbToolPath().toString();
    Core::MessageManager::write(adb + QLatin1Char(' ') + arguments.join(QLatin1Char(' ')));
    process->start(adb, arguments);
    if (!process->waitForFinished(500))
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
    QProcess proc;
    proc.start(AndroidConfigurations::currentConfig().keytoolPath().toString(), arguments);
    if (!proc.waitForStarted(4000))
        return false;
    if (!proc.waitForFinished(4000)) {
        proc.kill();
        proc.waitForFinished();
        return false;
    }
    return proc.exitCode() == 0;
}

bool AndroidManager::checkCertificatePassword(const QString &keystorePath, const QString &keystorePasswd, const QString &alias, const QString &certificatePasswd)
{
    // assumes that the keystore password is correct
    QStringList arguments;
    arguments << QLatin1String("-certreq")
              << QLatin1String("-keystore")
              << keystorePath
              << QLatin1String("--storepass")
              << keystorePasswd
              << QLatin1String("-alias")
              << alias
              << QLatin1String("-keypass");
    if (certificatePasswd.isEmpty())
        arguments << keystorePasswd;
    else
        arguments << certificatePasswd;

    QProcess proc;
    proc.start(AndroidConfigurations::currentConfig().keytoolPath().toString(), arguments);
    if (!proc.waitForStarted(4000))
        return false;
    if (!proc.waitForFinished(4000)) {
        proc.kill();
        proc.waitForFinished();
        return false;
    }
    return proc.exitCode() == 0;
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

    if (!buildApkStep || !buildApkStep->androidPackageSourceDir().appendPath(QLatin1String("gradlew")).exists())
        return false;

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
