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
#include "androidtoolchain.h"

#include <coreplugin/documentmanager.h>
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
    const QLatin1String AndroidLibraryPrefix("--Managed_by_Qt_Creator--");

    QString cleanPackageName(QString packageName)
    {
        QRegExp legalChars(QLatin1String("[a-zA-Z0-9_\\.]"));

        for (int i = 0; i < packageName.length(); ++i)
            if (!legalChars.exactMatch(packageName.mid(i, 1)))
                packageName[i] = QLatin1Char('_');

        static QStringList keywords;
        if (keywords.isEmpty())
            keywords << QLatin1String("abstract") << QLatin1String("continue") << QLatin1String("for")
                     << QLatin1String("new") << QLatin1String("switch") << QLatin1String("assert")
                     << QLatin1String("default") << QLatin1String("if") << QLatin1String("package")
                     << QLatin1String("synchronized") << QLatin1String("boolean") << QLatin1String("do")
                     << QLatin1String("goto") << QLatin1String("private") << QLatin1String("this")
                     << QLatin1String("break") << QLatin1String("double") << QLatin1String("implements")
                     << QLatin1String("protected") << QLatin1String("throw") << QLatin1String("byte")
                     << QLatin1String("else") << QLatin1String("import") << QLatin1String("public")
                     << QLatin1String("throws") << QLatin1String("case") << QLatin1String("enum")
                     << QLatin1String("instanceof") << QLatin1String("return") << QLatin1String("transient")
                     << QLatin1String("catch") << QLatin1String("extends") << QLatin1String("int")
                     << QLatin1String("short") << QLatin1String("try") << QLatin1String("char")
                     << QLatin1String("final") << QLatin1String("interface") << QLatin1String("static")
                     << QLatin1String("void") << QLatin1String("class") << QLatin1String("finally")
                     << QLatin1String("long") << QLatin1String("strictfp") << QLatin1String("volatile")
                     << QLatin1String("const") << QLatin1String("float") << QLatin1String("native")
                     << QLatin1String("super") << QLatin1String("while");

        // No keywords
        int index = -1;
        while (index != packageName.length()) {
            int next = packageName.indexOf(QLatin1Char('.'), index + 1);
            if (next == -1)
                next = packageName.length();
            QString word = packageName.mid(index + 1, next - index - 1);
            if (!word.isEmpty()) {
                QChar c = word[0];
                if (c >= QChar(QLatin1Char('0')) && c<= QChar(QLatin1Char('9'))) {
                    packageName.insert(index + 1, QLatin1Char('_'));
                    index = next + 1;
                    continue;
                }
            }
            if (keywords.contains(word)) {
                packageName.insert(next, QLatin1String("_"));
                index = next + 1;
            } else {
                index = next;
            }
        }


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

QString AndroidManager::targetSDK(ProjectExplorer::Target *target)
{
    QVariant v = target->namedSettings(QLatin1String("AndroidManager.TargetSdk"));
    if (v.isValid())
        return v.toString();

    QString fallback = QLatin1String("android-8");
    if (QtSupport::BaseQtVersion *qt = QtSupport::QtKitInformation::qtVersion(target->kit()))
        if (qt->qtVersion() >= QtSupport::QtVersionNumber(5, 0, 0))
            fallback = QLatin1String("android-9");

    if (!createAndroidTemplatesIfNecessary(target))
        return AndroidConfigurations::instance().bestMatch(fallback);

    QFile file(defaultPropertiesPath(target).toString());
    if (!file.open(QIODevice::ReadOnly))
        return AndroidConfigurations::instance().bestMatch(fallback);
    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        if (line.startsWith("target="))
            return QString::fromLatin1(line.trimmed().mid(7));
    }
    return AndroidConfigurations::instance().bestMatch(fallback);
}

bool AndroidManager::setTargetSDK(ProjectExplorer::Target *target, const QString &sdk)
{
    updateTarget(target, sdk, applicationName(target));
    target->setNamedSettings(QLatin1String("AndroidManager.TargetSdk"), sdk);
    return true;
}

QString AndroidManager::targetArch(ProjectExplorer::Target *target)
{
    Qt4ProjectManager::Qt4Project *pro = qobject_cast<Qt4ProjectManager::Qt4Project *>(target->project());
    if (!pro)
        return QString();
    Qt4ProjectManager::Qt4ProFileNode *node = pro->rootQt4ProjectNode();
    if (!node)
        return QString();
    return node->singleVariableValue(Qt4ProjectManager::AndroidArchVar);
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

bool AndroidManager::bundleQt(ProjectExplorer::Target *target)
{
    ProjectExplorer::RunConfiguration *runConfiguration = target->activeRunConfiguration();
    AndroidRunConfiguration *androidRunConfiguration = qobject_cast<AndroidRunConfiguration *>(runConfiguration);
    if (androidRunConfiguration != 0) {
        AndroidDeployStep *deployStep = androidRunConfiguration->deployStep();
        return deployStep->deployAction() == AndroidDeployStep::BundleLibraries;
    }

    return false;
}

bool AndroidManager::updateDeploymentSettings(ProjectExplorer::Target *target)
{
    // For Qt 4, the "use local libs" options is handled by passing command line arguments to the
    // app, so no need to alter the AndroidManifest.xml
    QtSupport::BaseQtVersion *baseQtVersion = QtSupport::QtKitInformation::qtVersion(target->kit());
    if (baseQtVersion == 0 || baseQtVersion->qtVersion() < QtSupport::QtVersionNumber(5,0,0))
        return true;

    ProjectExplorer::RunConfiguration *runConfiguration = target->activeRunConfiguration();
    AndroidRunConfiguration *androidRunConfiguration = qobject_cast<AndroidRunConfiguration *>(runConfiguration);
    if (androidRunConfiguration == 0)
        return false;

    AndroidDeployStep *deployStep = androidRunConfiguration->deployStep();
    AndroidDeployStep::AndroidDeployAction deployAction = deployStep->deployAction();
    bool useLocalLibs = deployAction == AndroidDeployStep::DeployLocal
            || deployAction == AndroidDeployStep::BundleLibraries;
    bool bundleQtLibs = deployAction == AndroidDeployStep::BundleLibraries;

    QDomDocument doc;
    if (!openManifest(target, doc))
        return false;

    QDomElement metadataElem = doc.documentElement().firstChildElement(QLatin1String("application")).firstChildElement(QLatin1String("activity")).firstChildElement(QLatin1String("meta-data"));

    // ### Passes -1 for API level, which means it won't work with setups that require
    // library selection based on API level. Use the old approach (command line argument)
    // in these cases. Hence the Qt version > 4 condition at the beginning of this function.
    QString localLibs;
    QString localJars;
    QString staticInitClasses;
    if (useLocalLibs) {
        localLibs = loadLocalLibs(target, -1);
        localJars = loadLocalJars(target, -1);
        staticInitClasses = loadLocalJarsInitClasses(target, -1);
    }

    bool changedManifest = false;
    while (!metadataElem.isNull()) {
        if (metadataElem.attribute(QLatin1String("android:name")) == QLatin1String("android.app.use_local_qt_libs")) {
            if (metadataElem.attribute(QLatin1String("android:value")).toInt() != int(useLocalLibs)) {
                metadataElem.setAttribute(QLatin1String("android:value"), int(useLocalLibs));
                changedManifest = true;
            }
        } else if (metadataElem.attribute(QLatin1String("android:name")) == QLatin1String("android.app.load_local_libs")) {
            if (metadataElem.attribute(QLatin1String("android:value")) != localLibs) {
                metadataElem.setAttribute(QLatin1String("android:value"), localLibs);
                changedManifest = true;
            }
        } else if (metadataElem.attribute(QLatin1String("android:name")) == QLatin1String("android.app.load_local_jars")) {
            if (metadataElem.attribute(QLatin1String("android:value")) != localJars) {
                metadataElem.setAttribute(QLatin1String("android:value"), localJars);
                changedManifest = true;
            }
        } else if (metadataElem.attribute(QLatin1String("android:name")) == QLatin1String("android.app.static_init_classes")) {
            if (metadataElem.attribute(QLatin1String("android:value")) != staticInitClasses) {
                metadataElem.setAttribute(QLatin1String("android:value"), staticInitClasses);
                changedManifest = true;
            }
        } else if (metadataElem.attribute(QLatin1String("android:name")) == QLatin1String("android.app.bundle_local_qt_libs")) {
            if (metadataElem.attribute(QLatin1String("android:value")).toInt() != int(bundleQtLibs)) {
                metadataElem.setAttribute(QLatin1String("android:value"), int(bundleQtLibs));
                changedManifest = true;
            }
        }

        metadataElem = metadataElem.nextSiblingElement(QLatin1String("meta-data"));
    }

    if (changedManifest)
        return saveManifest(target, doc);
    else
        return true;
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

    int minApiLevel = 4;
    if (QtSupport::BaseQtVersion *qt = QtSupport::QtKitInformation::qtVersion(target->kit()))
        if (qt->qtVersion() >= QtSupport::QtVersionNumber(5, 0, 0))
            minApiLevel = 9;

    QStringList sdks = AndroidConfigurations::instance().sdkTargets(minApiLevel);
    if (sdks.isEmpty()) {
        raiseError(tr("No Qt for Android SDKs were found.\nPlease install at least one SDK."));
        return false;
    }

    updateTarget(target, AndroidConfigurations::instance().sdkTargets(minApiLevel).at(0));
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

    Utils::Environment env = Utils::Environment::systemEnvironment();
    QString javaHome = AndroidConfigurations::instance().config().openJDKLocation.toString();
    if (!javaHome.isEmpty())
        env.set(QLatin1String("JAVA_HOME"), javaHome);
    // clean previous build
    QProcess androidProc;
    androidProc.setWorkingDirectory(androidDir);
    androidProc.setProcessEnvironment(env.toProcessEnvironment());
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
            QByteArray trimmed = lines[i].trimmed();
            if (trimmed.contains("@ANDROID-")) {
                commentLines = targetSDKNumber < trimmed.mid(trimmed.lastIndexOf('-') + 1).toInt();
                comment = !comment;
                continue;
            }
            if (!comment)
                continue;
            if (commentLines) {
                if (!trimmed.startsWith("//QtCreator")) {
                    lines[i] = "//QtCreator " + lines[i];
                    modified = true;
                }
            } else { if (trimmed.startsWith("//QtCreator")) {
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
    return Utils::FileName::fromString(version->qmakeProperty("QT_INSTALL_LIBS"));
}

QString AndroidManager::loadLocalLibs(ProjectExplorer::Target *target, int apiLevel)
{
    return loadLocal(target, apiLevel, Lib);
}

QString AndroidManager::loadLocalBundledFiles(ProjectExplorer::Target *target, int apiLevel)
{
    return loadLocal(target, apiLevel, BundledFile);
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

QVector<AndroidManager::Library> AndroidManager::availableQtLibsWithDependencies(ProjectExplorer::Target *target)
{
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target->kit());
    if (!target->activeRunConfiguration())
        return QVector<AndroidManager::Library>();

    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(target->kit());
    if (tc->type() != QLatin1String(Constants::ANDROID_TOOLCHAIN_TYPE))
        return QVector<AndroidManager::Library>();

    Qt4ProjectManager::Qt4Project *project = static_cast<Qt4ProjectManager::Qt4Project *>(target->project());
    QString arch = project->rootQt4ProjectNode()->singleVariableValue(Qt4ProjectManager::AndroidArchVar);

    AndroidToolChain *atc = static_cast<AndroidToolChain *>(tc);
    QString libgnustl = libGnuStl(arch, atc->ndkToolChainVersion());

    Utils::FileName readelfPath = AndroidConfigurations::instance().readelfPath(target->activeRunConfiguration()->abi().architecture(),
                                                                                atc->ndkToolChainVersion());
    const Qt4ProjectManager::Qt4Project *const qt4Project
            = qobject_cast<const Qt4ProjectManager::Qt4Project *>(target->project());
    if (!qt4Project || !version)
        return QVector<AndroidManager::Library>();
    QString qtLibsPath = version->qmakeProperty("QT_INSTALL_LIBS");
    if (!readelfPath.toFileInfo().exists()) {
        return QVector<AndroidManager::Library>();
    }
    LibrariesMap mapLibs;
    QDir libPath;
    QDirIterator it(qtLibsPath, QStringList() << QLatin1String("*.so"), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        libPath = it.next();
        const QString library = libPath.absolutePath().mid(libPath.absolutePath().lastIndexOf(QLatin1Char('/')) + 1);
        mapLibs[library].dependencies = dependencies(readelfPath, libPath.absolutePath());
    }

    const QString library = libgnustl.mid(libgnustl.lastIndexOf(QLatin1Char('/')) + 1);
    mapLibs[library] = Library();;

    // clean dependencies
    const LibrariesMap::Iterator lend = mapLibs.end();
    for (LibrariesMap::Iterator lit = mapLibs.begin(); lit != lend; ++lit) {
        Library &library = lit.value();
        int it = 0;
        while (it < library.dependencies.size()) {
            const QString &dependName = library.dependencies[it];
            if (!mapLibs.contains(dependName) && dependName.startsWith(QLatin1String("lib")) && dependName.endsWith(QLatin1String(".so")))
                library.dependencies.removeAt(it);
            else
                ++it;
        }
        if (library.dependencies.isEmpty())
            library.level = 0;
    }

    QVector<Library> qtLibraries;
    // calculate the level for every library
    for (LibrariesMap::Iterator lit = mapLibs.begin(); lit != lend; ++lit) {
        Library &library = lit.value();
        const QString &key = lit.key();
        if (library.level < 0)
           setLibraryLevel(key, mapLibs);

        if (library.name.isEmpty() && key.startsWith(QLatin1String("lib")) && key.endsWith(QLatin1String(".so")))
            library.name = key.mid(3, key.length() - 6);

        for (int it = 0; it < library.dependencies.size(); it++) {
            const QString &libName = library.dependencies[it];
            if (libName.startsWith(QLatin1String("lib")) && libName.endsWith(QLatin1String(".so")))
                library.dependencies[it] = libName.mid(3, libName.length() - 6);
        }
        qtLibraries.push_back(library);
    }
    qSort(qtLibraries.begin(), qtLibraries.end(), qtLibrariesLessThan);

    return qtLibraries;

}

QStringList AndroidManager::availableQtLibs(ProjectExplorer::Target *target)
{
    QStringList libs;
    QVector<Library> qtLibraries = availableQtLibsWithDependencies(target);
    foreach (Library lib, qtLibraries)
        libs.push_back(lib.name);
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

bool AndroidManager::setBundledInAssets(ProjectExplorer::Target *target, const QStringList &fileList)
{
    return setLibsXml(target, fileList, QLatin1String("bundled_in_assets"));
}

bool AndroidManager::setBundledInLib(ProjectExplorer::Target *target, const QStringList &fileList)
{
    return setLibsXml(target, fileList, QLatin1String("bundled_in_lib"));
}

QStringList AndroidManager::availablePrebundledLibs(ProjectExplorer::Target *target)
{
    QStringList libs;
    Qt4ProjectManager::Qt4Project *qt4Project = qobject_cast<Qt4ProjectManager::Qt4Project *>(target->project());
    if (!qt4Project)
        return libs;

    foreach (Qt4ProjectManager::Qt4ProFileNode *node, qt4Project->allProFiles())
        if (node->projectType() == Qt4ProjectManager::LibraryTemplate)
            libs << node->targetInformation().target;
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
                            QString dependencyLib = libElement.attribute(attribute).arg(apiLevel);
                            if (libElement.hasAttribute(QLatin1String("extends"))) {
                                const QString extends = libElement.attribute(QLatin1String("extends"));
                                if (libs.contains(extends)) {
                                    dependencyLibs << dependencyLib;
                                }
                            } else if (!dependencyLibs.contains(dependencyLib)) {
                                dependencyLibs << dependencyLib;
                            }
                        }

                        if (libElement.hasAttribute(QLatin1String("replaces"))) {
                            QString replacedLib = libElement.attribute(QLatin1String("replaces")).arg(apiLevel);
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
    Core::FileChangeBlocker blocker(manifestPath(target).toString());
    return saveXmlFile(target, doc, manifestPath(target));
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

bool AndroidManager::qtLibrariesLessThan(const Library &a, const Library &b)
{
    if (a.level == b.level)
        return a.name < b.name;
    return a.level < b.level;
}

QString AndroidManager::libGnuStl(const QString &arch, const QString &ndkToolChainVersion)
{
    return AndroidConfigurations::instance().config().ndkLocation.toString()
            + QLatin1String("/sources/cxx-stl/gnu-libstdc++/")
            + ndkToolChainVersion + QLatin1String("/libs/")
            + arch
            + QLatin1String("/libgnustl_shared.so");
}

QString AndroidManager::libraryPrefix()
{
    return AndroidLibraryPrefix;
}

} // namespace Internal
} // namespace Qt4ProjectManager
