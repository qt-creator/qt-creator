/**************************************************************************
**
** Copyright (c) 2014 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidmanager.h"
#include "androiddeployconfiguration.h"
#include "androidconfigurations.h"
#include "androidrunconfiguration.h"
#include "androidglobal.h"
#include "androidtoolchain.h"
#include "androiddeployqtstep.h"
#include "androidqtsupport.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakenodes.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>
#include <qmakeprojectmanager/qmakebuildconfiguration.h>
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
    const QLatin1String AndroidStringsFileName("/res/values/strings.xml");
    const QLatin1String AndroidDefaultPropertiesName("project.properties");
} // anonymous namespace

namespace Android {
namespace Internal {

bool AndroidManager::supportsAndroid(ProjectExplorer::Target *target)
{
    if (!qobject_cast<QmakeProjectManager::QmakeProject *>(target->project()))
        return false;
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target->kit());
    return version && version->platformName() == QLatin1String(QtSupport::Constants::ANDROID_PLATFORM);
}

QString AndroidManager::packageName(ProjectExplorer::Target *target)
{
    QDomDocument doc;
    if (!openManifest(target, doc))
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
    if (!openManifest(target, doc))
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
    if (!target->activeDeployConfiguration())
        return QLatin1String("android-9");
    AndroidDeployQtStep *step = AndroidGlobal::buildStep<AndroidDeployQtStep>(target->activeDeployConfiguration());
    if (step)
        return step->buildTargetSdk();
    return QLatin1String("android-9");
}

QString AndroidManager::targetArch(ProjectExplorer::Target *target)
{
    QmakeProjectManager::QmakeProject *pro = qobject_cast<QmakeProjectManager::QmakeProject *>(target->project());
    if (!pro)
        return QString();
    QmakeProjectManager::QmakeProFileNode *node = pro->rootQmakeProjectNode();
    if (!node)
        return QString();
    return node->singleVariableValue(QmakeProjectManager::AndroidArchVar);
}

Utils::FileName AndroidManager::dirPath(ProjectExplorer::Target *target)
{
    return target->activeBuildConfiguration()->buildDirectory().appendPath(QLatin1String(Constants::ANDROID_BUILDDIRECTORY));
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

Utils::FileName AndroidManager::apkPath(ProjectExplorer::Target *target, BuildType buildType)
{
    QString packageName = QLatin1String("QtApp");
    QString buildTypeName;
    if (buildType == DebugBuild)
        buildTypeName = QLatin1String("debug");
    else if (buildType == ReleaseBuildUnsigned)
        buildTypeName =QLatin1String("release-unsigned");
    else
        buildTypeName = QLatin1String("release");

    return dirPath(target)
            .appendPath(QLatin1String("bin"))
            .appendPath(QString::fromLatin1("%1-%2.apk")
                        .arg(packageName)
                        .arg(buildTypeName));
}

QStringList AndroidManager::availableTargetApplications(ProjectExplorer::Target *target)
{
    QStringList apps;
    QmakeProjectManager::QmakeProject *qmakeProject = qobject_cast<QmakeProjectManager::QmakeProject *>(target->project());
    if (!qmakeProject)
        return apps;
    foreach (QmakeProjectManager::QmakeProFileNode *proFile, qmakeProject->applicationProFiles()) {
        if (proFile->projectType() == QmakeProjectManager::ApplicationTemplate) {
            if (proFile->targetInformation().target.startsWith(QLatin1String("lib"))
                    && proFile->targetInformation().target.endsWith(QLatin1String(".so")))
                apps << proFile->targetInformation().target.mid(3, proFile->targetInformation().target.lastIndexOf(QLatin1Char('.')) - 3);
            else
                apps << proFile->targetInformation().target;
        }
    }
    apps.sort();
    return apps;
}

bool AndroidManager::bundleQt(ProjectExplorer::Target *target)
{
    AndroidDeployQtStep *androidDeployQtStep
            = AndroidGlobal::buildStep<AndroidDeployQtStep>(target->activeDeployConfiguration());
    if (androidDeployQtStep)
        return androidDeployQtStep->deployAction() == AndroidDeployQtStep::BundleLibrariesDeployment;

    return false;
}

bool AndroidManager::useLocalLibs(ProjectExplorer::Target *target)
{
    AndroidDeployQtStep *androidDeployQtStep
            = AndroidGlobal::buildStep<AndroidDeployQtStep>(target->activeDeployConfiguration());
    if (androidDeployQtStep) {
        return androidDeployQtStep->deployAction() == AndroidDeployQtStep::DebugDeployment
                || androidDeployQtStep->deployAction() == AndroidDeployQtStep::BundleLibrariesDeployment;
    }

    return false;
}

QString AndroidManager::deviceSerialNumber(ProjectExplorer::Target *target)
{
    AndroidDeployQtStep *androidDeployQtStep
            = AndroidGlobal::buildStep<AndroidDeployQtStep>(target->activeDeployConfiguration());
    if (androidDeployQtStep)
        return androidDeployQtStep->deviceSerialNumber();
    return QString();
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
    return qMakePair(9, 20);
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
        return QLatin1String("Android L"); // prelimary name?
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

bool AndroidManager::openLibsXml(ProjectExplorer::Target *target, QDomDocument &doc)
{
    return openXmlFile(doc, libsPath(target));
}

bool AndroidManager::saveLibsXml(ProjectExplorer::Target *target, QDomDocument &doc)
{
    return saveXmlFile(doc, libsPath(target));
}

void AndroidManager::raiseError(const QString &reason)
{
    QMessageBox::critical(0, tr("Error creating Android templates."), reason);
}

QString AndroidManager::loadLocal(ProjectExplorer::Target *target, int apiLevel, ItemType item, const QString &attribute)
{
    QString itemType;
    if (item == Lib)
        itemType = QLatin1String("lib");
    else if (item == BundledFile)
        itemType = QLatin1String("bundled");
    else // Jar or BundledJar
        itemType = QLatin1String("jar");

    QString localLibs;

    QDir rulesFilesDir(localLibsRulesFilePath(target).toString());
    if (!rulesFilesDir.exists())
        return localLibs;

    QStringList libs;
    libs << qtLibs(target) << prebundledLibs(target);

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
    localLibs = dependencyLibs.join(QLatin1String(":")) + QLatin1Char(':');
    foreach (QString replacedLib, replacedLibs)
        localLibs.remove(replacedLib + QLatin1Char(':'));

    return localLibs;
}

bool AndroidManager::openXmlFile(QDomDocument &doc, const Utils::FileName &fileName)
{
    QFile f(fileName.toString());
    if (!f.open(QIODevice::ReadOnly))
        return false;

    if (!doc.setContent(f.readAll())) {
        raiseError(tr("Cannot parse \"%1\".").arg(fileName.toUserOutput()));
        return false;
    }
    return true;
}

bool AndroidManager::saveXmlFile(QDomDocument &doc, const Utils::FileName &fileName)
{
    QFile f(fileName.toString());
    if (!f.open(QIODevice::WriteOnly)) {
        raiseError(tr("Cannot open \"%1\".").arg(fileName.toUserOutput()));
        return false;
    }
    return f.write(doc.toByteArray(4)) >= 0;
}

bool AndroidManager::openManifest(ProjectExplorer::Target *target, QDomDocument &doc)
{
    return openXmlFile(doc, manifestPath(target));
}

bool AndroidManager::saveManifest(ProjectExplorer::Target *target, QDomDocument &doc)
{
    Core::FileChangeBlocker blocker(manifestPath(target).toString());
    return saveXmlFile(doc, manifestPath(target));
}

QStringList AndroidManager::libsXml(ProjectExplorer::Target *target, const QString &tag)
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

bool AndroidManager::setLibsXml(ProjectExplorer::Target *target, const QStringList &libs, const QString &tag)
{
    QDomDocument doc;
    if (!openLibsXml(target, doc))
        return false;
    QDomElement arrayElem = doc.documentElement().firstChildElement(QLatin1String("array"));
    while (!arrayElem.isNull()) {
        if (arrayElem.attribute(QLatin1String("name")) == tag) {
            doc.documentElement().removeChild(arrayElem);
            arrayElem = doc.createElement(QLatin1String("array"));
            arrayElem.setAttribute(QLatin1String("name"), tag);
            foreach (const QString &lib, libs) {
                QDomElement item = doc.createElement(QLatin1String("item"));
                item.appendChild(doc.createTextNode(lib));
                arrayElem.appendChild(item);
            }
            doc.documentElement().appendChild(arrayElem);
            return saveLibsXml(target, doc);
        }
        arrayElem = arrayElem.nextSiblingElement(QLatin1String("array"));
    }
    return false;
}


QStringList AndroidManager::dependencies(const Utils::FileName &readelfPath, const QString &lib)
{
    QStringList libs;

    QProcess readelfProc;
    readelfProc.start(readelfPath.toString(), QStringList() << QLatin1String("-d") << QLatin1String("-W") << lib);

    if (!readelfProc.waitForFinished(-1)) {
        readelfProc.kill();
        return libs;
    }

    QList<QByteArray> lines = readelfProc.readAll().trimmed().split('\n');
    foreach (const QByteArray &line, lines) {
        if (line.contains("(NEEDED)") && line.contains("Shared library:") ) {
            const int pos = line.lastIndexOf('[') + 1;
            libs << QString::fromLatin1(line.mid(pos, line.lastIndexOf(']') - pos));
        }
    }
    return libs;
}

int AndroidManager::setLibraryLevel(const QString &library, LibrariesMap &mapLibs)
{
    int maxlevel = mapLibs[library].level;
    if (maxlevel > 0)
        return maxlevel;
    foreach (QString lib, mapLibs[library].dependencies) {
        foreach (const QString &key, mapLibs.keys()) {
            if (library == key)
                continue;
            if (key == lib) {
                int libLevel = mapLibs[key].level;

                if (libLevel < 0)
                    libLevel = setLibraryLevel(key, mapLibs);

                if (libLevel > maxlevel)
                    maxlevel = libLevel;
                break;
            }
        }
    }
    if (mapLibs[library].level < 0)
        mapLibs[library].level = maxlevel + 1;
    return maxlevel + 1;
}

void AndroidManager::cleanLibsOnDevice(ProjectExplorer::Target *target)
{
    const QString targetArch = AndroidManager::targetArch(target);
    if (targetArch.isEmpty())
        return;
    int deviceAPILevel = AndroidManager::minimumSDK(target);
    AndroidDeviceInfo info = AndroidConfigurations::showDeviceDialog(target->project(), deviceAPILevel, targetArch);
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
    Core::MessageManager::write(adb + QLatin1Char(' ') + arguments.join(QLatin1String(" ")));
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
    AndroidDeviceInfo info = AndroidConfigurations::showDeviceDialog(target->project(), deviceAPILevel, targetArch);
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
    Core::MessageManager::write(adb + QLatin1Char(' ') + arguments.join(QLatin1String(" ")));
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
    if (!fileName.toFileInfo().exists())
        return false;
    QDomDocument dstVersionDoc;
    if (!AndroidManager::openXmlFile(dstVersionDoc, fileName))
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

} // namespace Internal
} // namespace Android
