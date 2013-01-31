/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
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
#include "androiddeploystep.h"
#include "androidglobal.h"
#include "androidpackagecreationstep.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4nodes.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qtsupport/customexecutablerunconfiguration.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

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

    QString cleanPackageName(QString packageName)
    {
        QRegExp legalChars(QLatin1String("[a-zA-Z0-9_\\.]"));

        for (int i = 0; i < packageName.length(); ++i)
            if (!legalChars.exactMatch(packageName.mid(i, 1)))
                packageName[i] = QLatin1Char('_');

        return packageName;
    }
} // anonymous namespace

namespace Android {
namespace Internal {

bool AndroidManager::supportsAndroid(ProjectExplorer::Target *target)
{
    if (!qobject_cast<Qt4ProjectManager::Qt4Project *>(target->project()))
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

bool AndroidManager::setPackageName(ProjectExplorer::Target *target, const QString &name)
{
    QDomDocument doc;
    if (!openManifest(target, doc))
        return false;
    QDomElement manifestElem = doc.documentElement();
    manifestElem.setAttribute(QLatin1String("package"), cleanPackageName(name));
    return saveManifest(target, doc);
}

QString AndroidManager::applicationName(ProjectExplorer::Target *target)
{
    QDomDocument doc;
    if (!openXmlFile(doc, stringsPath(target)))
        return QString();
    QDomElement metadataElem = doc.documentElement().firstChildElement(QLatin1String("string"));
    while (!metadataElem.isNull()) {
        if (metadataElem.attribute(QLatin1String("name")) == QLatin1String("app_name"))
            return metadataElem.text();
        metadataElem = metadataElem.nextSiblingElement(QLatin1String("string"));
    }
    return QString();
}

bool AndroidManager::setApplicationName(ProjectExplorer::Target *target, const QString &name)
{
    QDomDocument doc;
    Utils::FileName path = stringsPath(target);
    if (!openXmlFile(doc, path))
        return false;
    QDomElement metadataElem = doc.documentElement().firstChildElement(QLatin1String("string"));
    while (!metadataElem.isNull()) {
        if (metadataElem.attribute(QLatin1String("name")) == QLatin1String("app_name")) {
            metadataElem.removeChild(metadataElem.firstChild());
            metadataElem.appendChild(doc.createTextNode(name));
            break;
        }
        metadataElem = metadataElem.nextSiblingElement(QLatin1String("string"));
    }
    return saveXmlFile(target, doc, path);
}

QStringList AndroidManager::permissions(ProjectExplorer::Target *target)
{
    QStringList per;
    QDomDocument doc;
    if (!openManifest(target, doc))
        return per;
    QDomElement permissionElem = doc.documentElement().firstChildElement(QLatin1String("uses-permission"));
    while (!permissionElem.isNull()) {
        per << permissionElem.attribute(QLatin1String("android:name"));
        permissionElem = permissionElem.nextSiblingElement(QLatin1String("uses-permission"));
    }
    return per;
}

bool AndroidManager::setPermissions(ProjectExplorer::Target *target, const QStringList &permissions)
{
    QDomDocument doc;
    if (!openManifest(target, doc))
        return false;
    QDomElement docElement = doc.documentElement();
    QDomElement permissionElem = docElement.firstChildElement(QLatin1String("uses-permission"));
    while (!permissionElem.isNull()) {
        docElement.removeChild(permissionElem);
        permissionElem = docElement.firstChildElement(QLatin1String("uses-permission"));
    }

    foreach (const QString &permission, permissions ) {
        permissionElem = doc.createElement(QLatin1String("uses-permission"));
        permissionElem.setAttribute(QLatin1String("android:name"), permission);
        docElement.appendChild(permissionElem);
    }

    return saveManifest(target, doc);
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

int AndroidManager::versionCode(ProjectExplorer::Target *target)
{
    QDomDocument doc;
    if (!openManifest(target, doc))
        return 0;
    QDomElement manifestElem = doc.documentElement();
    return manifestElem.attribute(QLatin1String("android:versionCode")).toInt();
}

bool AndroidManager::setVersionCode(ProjectExplorer::Target *target, int version)
{
    QDomDocument doc;
    if (!openManifest(target, doc))
        return false;
    QDomElement manifestElem = doc.documentElement();
    manifestElem.setAttribute(QLatin1String("android:versionCode"), version);
    return saveManifest(target, doc);
}

QString AndroidManager::versionName(ProjectExplorer::Target *target)
{
    QDomDocument doc;
    if (!openManifest(target, doc))
        return QString();
    QDomElement manifestElem = doc.documentElement();
    return manifestElem.attribute(QLatin1String("android:versionName"));
}

bool AndroidManager::setVersionName(ProjectExplorer::Target *target, const QString &version)
{
    QDomDocument doc;
    if (!openManifest(target, doc))
        return false;
    QDomElement manifestElem = doc.documentElement();
    manifestElem.setAttribute(QLatin1String("android:versionName"), version);
    return saveManifest(target, doc);
}

QString AndroidManager::targetSDK(ProjectExplorer::Target *target)
{
    if (!createAndroidTemplatesIfNecessary(target))
        return AndroidConfigurations::instance().bestMatch(QLatin1String("android-8"));
    QFile file(defaultPropertiesPath(target).toString());
    if (!file.open(QIODevice::ReadOnly))
        return AndroidConfigurations::instance().bestMatch(QLatin1String("android-8"));
    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        if (line.startsWith("target="))
            return QString::fromLatin1(line.trimmed().mid(7));
    }
    return AndroidConfigurations::instance().bestMatch(QLatin1String("android-8"));
}

bool AndroidManager::setTargetSDK(ProjectExplorer::Target *target, const QString &sdk)
{
    updateTarget(target, sdk, applicationName(target));
    return true;
}

QIcon AndroidManager::highDpiIcon(ProjectExplorer::Target *target)
{
    return icon(target, HighDPI);
}

bool AndroidManager::setHighDpiIcon(ProjectExplorer::Target *target, const QString &iconFilePath)
{
    return setIcon(target, HighDPI, iconFilePath);
}

QIcon AndroidManager::mediumDpiIcon(ProjectExplorer::Target *target)
{
    return icon(target, MediumDPI);
}

bool AndroidManager::setMediumDpiIcon(ProjectExplorer::Target *target, const QString &iconFilePath)
{
    return setIcon(target, MediumDPI, iconFilePath);
}

QIcon AndroidManager::lowDpiIcon(ProjectExplorer::Target *target)
{
    return icon(target, LowDPI);
}

bool AndroidManager::setLowDpiIcon(ProjectExplorer::Target *target, const QString &iconFilePath)
{
    return setIcon(target, LowDPI, iconFilePath);
}

Utils::FileName AndroidManager::dirPath(ProjectExplorer::Target *target)
{
    return Utils::FileName::fromString(target->project()->projectDirectory()).appendPath(AndroidDirName);
}

Utils::FileName AndroidManager::manifestPath(ProjectExplorer::Target *target)
{
    return dirPath(target).appendPath(AndroidManifestName);
}

Utils::FileName AndroidManager::libsPath(ProjectExplorer::Target *target)
{
    return dirPath(target).appendPath(AndroidLibsFileName);
}

Utils::FileName AndroidManager::stringsPath(ProjectExplorer::Target *target)
{
    return dirPath(target).append(AndroidStringsFileName);
}

Utils::FileName AndroidManager::defaultPropertiesPath(ProjectExplorer::Target *target)
{
    return dirPath(target).appendPath(AndroidDefaultPropertiesName);
}

Utils::FileName AndroidManager::srcPath(ProjectExplorer::Target *target)
{
    return dirPath(target).appendPath(QLatin1String("/src"));
}

Utils::FileName AndroidManager::apkPath(ProjectExplorer::Target *target, BuildType buildType)
{
    return dirPath(target)
            .appendPath(QLatin1String("bin"))
            .appendPath(QString::fromLatin1("%1-%2.apk")
                        .arg(applicationName(target))
                        .arg(buildType == DebugBuild
                             ? QLatin1String("debug")
                             : (buildType == ReleaseBuildUnsigned)
                               ? QLatin1String("release-unsigned")
                               : QLatin1String("signed")));
}

QStringList AndroidManager::availableTargetApplications(ProjectExplorer::Target *target)
{
    QStringList apps;
    Qt4ProjectManager::Qt4Project *qt4Project = qobject_cast<Qt4ProjectManager::Qt4Project *>(target->project());
    foreach (Qt4ProjectManager::Qt4ProFileNode *proFile, qt4Project->applicationProFiles()) {
        if (proFile->projectType() == Qt4ProjectManager::ApplicationTemplate) {
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

QString AndroidManager::targetApplication(ProjectExplorer::Target *target)
{
    QDomDocument doc;
    if (!openManifest(target, doc))
        return QString();
    QDomElement metadataElem = doc.documentElement().firstChildElement(QLatin1String("application")).firstChildElement(QLatin1String("activity")).firstChildElement(QLatin1String("meta-data"));
    while (!metadataElem.isNull()) {
        if (metadataElem.attribute(QLatin1String("android:name")) == QLatin1String("android.app.lib_name"))
            return metadataElem.attribute(QLatin1String("android:value"));
        metadataElem = metadataElem.nextSiblingElement(QLatin1String("meta-data"));
    }
    return QString();
}

bool AndroidManager::setTargetApplication(ProjectExplorer::Target *target, const QString &name)
{
    QDomDocument doc;
    if (!openManifest(target, doc))
        return false;
    QDomElement metadataElem = doc.documentElement().firstChildElement(QLatin1String("application")).firstChildElement(QLatin1String("activity")).firstChildElement(QLatin1String("meta-data"));
    while (!metadataElem.isNull()) {
        if (metadataElem.attribute(QLatin1String("android:name")) == QLatin1String("android.app.lib_name")) {
            metadataElem.setAttribute(QLatin1String("android:value"), name);
            return saveManifest(target, doc);
        }
        metadataElem = metadataElem.nextSiblingElement(QLatin1String("meta-data"));
    }
    return false;
}

QString AndroidManager::targetApplicationPath(ProjectExplorer::Target *target)
{
    QString selectedApp = targetApplication(target);
    if (selectedApp.isEmpty())
        return QString();
    Qt4ProjectManager::Qt4Project *qt4Project = qobject_cast<Qt4ProjectManager::Qt4Project *>(target->project());
    foreach (Qt4ProjectManager::Qt4ProFileNode *proFile, qt4Project->applicationProFiles()) {
        if (proFile->projectType() == Qt4ProjectManager::ApplicationTemplate) {
            if (proFile->targetInformation().target.startsWith(QLatin1String("lib"))
                    && proFile->targetInformation().target.endsWith(QLatin1String(".so"))) {
                if (proFile->targetInformation().target.mid(3, proFile->targetInformation().target.lastIndexOf(QLatin1Char('.')) - 3)
                        == selectedApp)
                    return proFile->targetInformation().buildDir + QLatin1Char('/') + proFile->targetInformation().target;
            } else {
                if (proFile->targetInformation().target == selectedApp)
                    return proFile->targetInformation().buildDir + QLatin1String("/lib") + proFile->targetInformation().target + QLatin1String(".so");
            }
        }
    }
    return QString();
}

bool AndroidManager::createAndroidTemplatesIfNecessary(ProjectExplorer::Target *target)
{
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target->kit());
    Qt4ProjectManager::Qt4Project *qt4Project = qobject_cast<Qt4ProjectManager::Qt4Project*>(target->project());
    if (!qt4Project || !qt4Project->rootProjectNode() || !version)
        return false;

    Utils::FileName javaSrcPath
            = Utils::FileName::fromString(version->qmakeProperty("QT_INSTALL_PREFIX"))
            .appendPath(QLatin1String("src/android/java"));
    QDir projectDir(qt4Project->projectDirectory());
    Utils::FileName androidPath = dirPath(target);

    QStringList m_ignoreFiles;
    bool forceUpdate = false;
    QDomDocument srcVersionDoc;
    Utils::FileName srcVersionPath = javaSrcPath;
    srcVersionPath.appendPath(QLatin1String("version.xml"));
    if (openXmlFile(srcVersionDoc, srcVersionPath)) {
        QDomDocument dstVersionDoc;
        Utils::FileName dstVersionPath=androidPath;
        dstVersionPath.appendPath(QLatin1String("version.xml"));
        if (openXmlFile(dstVersionDoc, dstVersionPath))
            forceUpdate = (srcVersionDoc.documentElement().attribute(QLatin1String("value")).toDouble()
                           > dstVersionDoc.documentElement().attribute(QLatin1String("value")).toDouble());
        else
            forceUpdate = true;

        if (forceUpdate && androidPath.toFileInfo().exists()) {
            QDomElement ignoreFile = srcVersionDoc.documentElement().firstChildElement(QLatin1String("ignore")).firstChildElement(QLatin1String("file"));
            while (!ignoreFile.isNull()) {
                m_ignoreFiles << ignoreFile.text();
                ignoreFile = ignoreFile.nextSiblingElement();
            }
        }
    }

    Utils::FileName src = androidPath;
    src.appendPath(QLatin1String("src"));
    Utils::FileName res = androidPath;
    res.appendPath(QLatin1String("res"));

    if (!forceUpdate && androidPath.toFileInfo().exists()
            && manifestPath(target).toFileInfo().exists()
            && src.toFileInfo().exists()
            && res.toFileInfo().exists())
        return true;

    forceUpdate &= androidPath.toFileInfo().exists();

    if (!dirPath(target).toFileInfo().exists() && !projectDir.mkdir(AndroidDirName)) {
        raiseError(tr("Error creating Android directory '%1'.").arg(AndroidDirName));
        return false;
    }

    QStringList androidFiles;
    QDirIterator it(javaSrcPath.toString(), QDirIterator::Subdirectories);
    int pos = it.path().size();
    while (it.hasNext()) {
        it.next();
        if (it.fileInfo().isDir()) {
            projectDir.mkpath(AndroidDirName + it.filePath().mid(pos));
        } else {
            Utils::FileName dstFile = androidPath;
            dstFile.appendPath(it.filePath().mid(pos));
            if (m_ignoreFiles.contains(it.fileName())) {
                continue;
            } else {
                if (dstFile.toFileInfo().exists())
                    QFile::remove(dstFile.toString());
                else
                    androidFiles << dstFile.toString();
            }
            QFile::copy(it.filePath(), dstFile.toString());
        }
    }
    if (!androidFiles.isEmpty())
        qt4Project->rootProjectNode()->addFiles(ProjectExplorer::UnknownFileType, androidFiles);

    QStringList sdks = AndroidConfigurations::instance().sdkTargets();
    if (sdks.isEmpty()) {
        raiseError(tr("No Qt for Android SDKs were found.\nPlease install at least one SDK."));
        return false;
    }
    updateTarget(target, AndroidConfigurations::instance().sdkTargets().at(0));
    QStringList apps = availableTargetApplications(target);
    if (!apps.isEmpty())
        setTargetApplication(target, apps.at(0));

    QString applicationName = target->project()->displayName();
    if (!applicationName.isEmpty()) {
        setPackageName(target, packageName(target) + QLatin1Char('.') + applicationName);
        applicationName[0] = applicationName[0].toUpper();
        setApplicationName(target, applicationName);
    }

    if (forceUpdate)
        QMessageBox::warning(0, tr("Warning"), tr("Android files have been updated automatically"));

    return true;
}

void AndroidManager::updateTarget(ProjectExplorer::Target *target, const QString &targetSDK, const QString &name)
{
    QString androidDir = dirPath(target).toString();

    // clean previous build
    QProcess androidProc;
    androidProc.setWorkingDirectory(androidDir);
    androidProc.start(AndroidConfigurations::instance().antToolPath().toString(),
                      QStringList() << QLatin1String("clean"));
    if (!androidProc.waitForFinished(-1))
        androidProc.terminate();
    // clean previous build

    int targetSDKNumber = targetSDK.mid(targetSDK.lastIndexOf(QLatin1Char('-')) + 1).toInt();
    bool commentLines = false;
    QDirIterator it(androidDir, QStringList() << QLatin1String("*.java"), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QFile file(it.filePath());
        if (!file.open(QIODevice::ReadWrite))
            continue;
        QList<QByteArray> lines = file.readAll().trimmed().split('\n');

        bool modified = false;
        bool comment = false;
        for (int i = 0; i < lines.size(); i++) {
            if (lines[i].contains("@ANDROID-")) {
                commentLines = targetSDKNumber < lines[i].mid(lines[i].lastIndexOf('-') + 1).toInt();
                comment = !comment;
                continue;
            }
            if (!comment)
                continue;
            if (commentLines) {
                if (!lines[i].trimmed().startsWith("//QtCreator")) {
                    lines[i] = "//QtCreator " + lines[i];
                    modified = true;
                }
            } else { if (lines[i].trimmed().startsWith("//QtCreator")) {
                    lines[i] = lines[i].mid(12);
                    modified = true;
                }
            }
        }
        if (modified) {
            file.resize(0);
            foreach (const QByteArray &line, lines) {
                file.write(line);
                file.write("\n");
            }
        }
        file.close();
    }

    QStringList params;
    params << QLatin1String("update") << QLatin1String("project") << QLatin1String("-p") << androidDir;
    if (!targetSDK.isEmpty())
        params << QLatin1String("-t") << targetSDK;
    if (!name.isEmpty())
        params << QLatin1String("-n") << name;
    androidProc.start(AndroidConfigurations::instance().androidToolPath().toString(), params);
    if (!androidProc.waitForFinished(-1))
        androidProc.terminate();
}

Utils::FileName AndroidManager::localLibsRulesFilePath(ProjectExplorer::Target *target)
{
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target->kit());
    if (!version)
        return Utils::FileName();
    return Utils::FileName::fromString(version->qmakeProperty("QT_INSTALL_LIBS") + QLatin1String("/rules.xml"));
}

QString AndroidManager::loadLocalLibs(ProjectExplorer::Target *target, int apiLevel)
{
    return loadLocal(target, apiLevel, Lib);
}

QString AndroidManager::loadLocalJars(ProjectExplorer::Target *target, int apiLevel)
{
    return loadLocal(target, apiLevel, Jar);
}

QString AndroidManager::loadLocalJarsInitClasses(ProjectExplorer::Target *target, int apiLevel)
{
    return loadLocal(target, apiLevel, Jar, QLatin1String("initClass"));
}

QStringList AndroidManager::availableQtLibs(ProjectExplorer::Target *target)
{
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target->kit());
    if (!target->activeRunConfiguration())
        return QStringList();

    Utils::FileName readelfPath = AndroidConfigurations::instance().readelfPath(target->activeRunConfiguration()->abi().architecture());
    QStringList libs;
    const Qt4ProjectManager::Qt4Project *const qt4Project
            = qobject_cast<const Qt4ProjectManager::Qt4Project *>(target->project());
    if (!qt4Project || !version)
        return libs;
    QString qtLibsPath = version->qmakeProperty("QT_INSTALL_LIBS");
    if (!readelfPath.toFileInfo().exists()) {
        QDirIterator libsIt(qtLibsPath, QStringList() << QLatin1String("libQt*.so"));
        while (libsIt.hasNext()) {
            libsIt.next();
            libs << libsIt.fileName().mid(3, libsIt.fileName().indexOf(QLatin1Char('.')) - 3);
        }
        libs.sort();
        return libs;
    }
    LibrariesMap mapLibs;
    QDir libPath;
    QDirIterator it(qtLibsPath, QStringList() << QLatin1String("*.so"), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        libPath = it.next();
        const QString library = libPath.absolutePath().mid(libPath.absolutePath().lastIndexOf(QLatin1Char('/')) + 1);
        mapLibs[library].dependencies = dependencies(readelfPath, libPath.absolutePath());
    }

    // clean dependencies
    foreach (const QString &key, mapLibs.keys()) {
        int it = 0;
        while (it < mapLibs[key].dependencies.size()) {
            const QString &dependName = mapLibs[key].dependencies[it];
            if (!mapLibs.keys().contains(dependName) && dependName.startsWith(QLatin1String("lib")) && dependName.endsWith(QLatin1String(".so")))
                mapLibs[key].dependencies.removeAt(it);
            else
                ++it;
        }
        if (!mapLibs[key].dependencies.size())
            mapLibs[key].level = 0;
    }

    QVector<Library> qtLibraries;
    // calculate the level for every library
    foreach (const QString &key, mapLibs.keys()) {
        if (mapLibs[key].level < 0)
           setLibraryLevel(key, mapLibs);

        if (!mapLibs[key].name.length() && key.startsWith(QLatin1String("lib")) && key.endsWith(QLatin1String(".so")))
            mapLibs[key].name = key.mid(3, key.length() - 6);

        for (int it = 0; it < mapLibs[key].dependencies.size(); it++) {
            const QString &libName = mapLibs[key].dependencies[it];
            if (libName.startsWith(QLatin1String("lib")) && libName.endsWith(QLatin1String(".so")))
                mapLibs[key].dependencies[it] = libName.mid(3, libName.length() - 6);
        }
        qtLibraries.push_back(mapLibs[key]);
    }
    qSort(qtLibraries.begin(), qtLibraries.end(), qtLibrariesLessThan);
    foreach (Library lib, qtLibraries) {
        libs.push_back(lib.name);
    }
    return libs;
}

QStringList AndroidManager::qtLibs(ProjectExplorer::Target *target)
{
    return libsXml(target, QLatin1String("qt_libs"));
}

bool AndroidManager::setQtLibs(ProjectExplorer::Target *target, const QStringList &libs)
{
    return setLibsXml(target, libs, QLatin1String("qt_libs"));
}

QStringList AndroidManager::availablePrebundledLibs(ProjectExplorer::Target *target)
{
    QStringList libs;
    Qt4ProjectManager::Qt4Project *qt4Project = qobject_cast<Qt4ProjectManager::Qt4Project *>(target->project());
    if (!qt4Project)
        return libs;

    foreach (Qt4ProjectManager::Qt4ProFileNode *node, qt4Project->allProFiles())
        if (node->projectType() == Qt4ProjectManager::LibraryTemplate)
            libs << QLatin1String("lib") + node->targetInformation().target + QLatin1String(".so");
    return libs;
}

QStringList AndroidManager::prebundledLibs(ProjectExplorer::Target *target)
{
    return libsXml(target, QLatin1String("bundled_libs"));
}

bool AndroidManager::setPrebundledLibs(ProjectExplorer::Target *target, const QStringList &libs)
{
    return setLibsXml(target, libs, QLatin1String("bundled_libs"));
}

bool AndroidManager::openLibsXml(ProjectExplorer::Target *target, QDomDocument &doc)
{
    return openXmlFile(doc, libsPath(target));
}

bool AndroidManager::saveLibsXml(ProjectExplorer::Target *target, QDomDocument &doc)
{
    return saveXmlFile(target, doc, libsPath(target));
}

void AndroidManager::raiseError(const QString &reason)
{
    QMessageBox::critical(0, tr("Error creating Android templates"), reason);
}

QString AndroidManager::loadLocal(ProjectExplorer::Target *target, int apiLevel, ItemType item, const QString &attribute)
{
    QString itemType;
    if (item == Lib)
        itemType = QLatin1String("lib");
    else
        itemType = QLatin1String("jar");

    QString localLibs;

    QDomDocument doc;
    if (!openXmlFile(doc, localLibsRulesFilePath(target)))
        return localLibs;

    QStringList libs;
    libs << qtLibs(target) << prebundledLibs(target);
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
                if (libElement.hasAttribute(attribute))
                    localLibs += libElement.attribute(attribute).arg(apiLevel) + QLatin1Char(':');
                libElement = libElement.nextSiblingElement(itemType);
            }

            libElement = element.firstChildElement(QLatin1String("replaces")).firstChildElement(itemType);
            while (!libElement.isNull()) {
                if (libElement.hasAttribute(attribute))
                    localLibs.replace(libElement.attribute(attribute).arg(apiLevel) + QLatin1Char(':'), QString());
                libElement = libElement.nextSiblingElement(itemType);
            }
        }
        element = element.nextSiblingElement(QLatin1String("lib"));
    }
    return localLibs;
}

bool AndroidManager::openXmlFile(QDomDocument &doc, const Utils::FileName &fileName)
{
    QFile f(fileName.toString());
    if (!f.open(QIODevice::ReadOnly))
        return false;

    if (!doc.setContent(f.readAll())) {
        raiseError(tr("Can't parse '%1'").arg(fileName.toUserOutput()));
        return false;
    }
    return true;
}

bool AndroidManager::saveXmlFile(ProjectExplorer::Target *target, QDomDocument &doc, const Utils::FileName &fileName)
{
    if (!createAndroidTemplatesIfNecessary(target))
        return false;

    QFile f(fileName.toString());
    if (!f.open(QIODevice::WriteOnly)) {
        raiseError(tr("Can't open '%1'").arg(fileName.toUserOutput()));
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
    return saveXmlFile(target, doc, manifestPath(target));
}

QString AndroidManager::iconPath(ProjectExplorer::Target *target, AndroidManager::IconType type)
{
    switch (type) {
    case HighDPI:
        return dirPath(target).appendPath(QLatin1String("res/drawable-hdpi/icon.png")).toString();
    case MediumDPI:
        return dirPath(target).appendPath(QLatin1String("res/drawable-mdpi/icon.png")).toString();
    case LowDPI:
        return dirPath(target).appendPath(QLatin1String("res/drawable-ldpi/icon.png")).toString();
    default:
        return QString();
    }
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


QIcon AndroidManager::icon(ProjectExplorer::Target *target, IconType type)
{
    return QIcon(iconPath(target, type));
}

bool AndroidManager::setIcon(ProjectExplorer::Target *target, IconType type, const QString &iconFileName)
{
    if (!QFileInfo(iconFileName).exists())
        return false;

    const QString path = iconPath(target, type);
    QFile::remove(path);
    return QFile::copy(iconFileName, path);
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

bool AndroidManager::qtLibrariesLessThan(const Library &a, const Library &b)
{
    if (a.level == b.level)
        return a.name < b.name;
    return a.level < b.level;
}

} // namespace Internal
} // namespace Qt4ProjectManager
