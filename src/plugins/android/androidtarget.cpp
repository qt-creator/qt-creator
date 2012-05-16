/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "androidtarget.h"
#include "androiddeployconfiguration.h"
#include "androidconfigurations.h"
#include "androidrunconfiguration.h"
#include "androiddeploystep.h"
#include "androidglobal.h"
#include "androidpackagecreationstep.h"

#include <qt4projectmanager/qt4nodes.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <qt4projectmanager/qt4target.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qtsupport/customexecutablerunconfiguration.h>

#include <QDir>
#include <QFileSystemWatcher>
#include <QList>
#include <QProcess>
#include <QMessageBox>
#include <QApplication>
#include <QDomDocument>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

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

AndroidTarget::AndroidTarget(Qt4Project *parent, const Core::Id id) :
    Qt4BaseTarget(parent, id)
  , m_androidFilesWatcher(new QFileSystemWatcher(this))
  , m_buildConfigurationFactory(new Qt4BuildConfigurationFactory(this))
{
    setDisplayName(defaultDisplayName());
    setDefaultDisplayName(defaultDisplayName());
    setIcon(QIcon(QLatin1String(Constants::ANDROID_SETTINGS_CATEGORY_ICON)));
    connect(parent, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
        this, SLOT(handleTargetChanged(ProjectExplorer::Target*)));
}

AndroidTarget::~AndroidTarget()
{

}

Qt4BuildConfigurationFactory *AndroidTarget::buildConfigurationFactory() const
{
    return m_buildConfigurationFactory;
}

void AndroidTarget::createApplicationProFiles(bool reparse)
{
    if (!reparse)
        removeUnconfiguredCustomExectutableRunConfigurations();

    QList<Qt4ProFileNode *> profiles = qt4Project()->applicationProFiles();
    QSet<QString> paths;
    foreach (Qt4ProFileNode *pro, profiles)
        paths << pro->path();

    foreach (ProjectExplorer::RunConfiguration *rc, runConfigurations())
        if (AndroidRunConfiguration *qt4rc = qobject_cast<AndroidRunConfiguration *>(rc))
            paths.remove(qt4rc->proFilePath());

    // Only add new runconfigurations if there are none.
    foreach (const QString &path, paths)
        addRunConfiguration(new AndroidRunConfiguration(this, path));

    // Oh still none? Add a custom executable runconfiguration
    if (runConfigurations().isEmpty()) {
        addRunConfiguration(new QtSupport::CustomExecutableRunConfiguration(this));
    }
}

QList<ProjectExplorer::RunConfiguration *> AndroidTarget::runConfigurationsForNode(ProjectExplorer::Node *n)
{
    QList<ProjectExplorer::RunConfiguration *> result;
    foreach (ProjectExplorer::RunConfiguration *rc, runConfigurations())
        if (AndroidRunConfiguration *qt4c = qobject_cast<AndroidRunConfiguration *>(rc))
                if (qt4c->proFilePath() == n->path())
                    result << rc;
    return result;
}

QString AndroidTarget::defaultDisplayName()
{
    return QApplication::translate("Qt4ProjectManager::Qt4Target", "Android", "Qt4 Android target display name");
}

void AndroidTarget::handleTargetChanged(ProjectExplorer::Target *target)
{
    if (target != this)
        return;

    disconnect(project(), SIGNAL(addedTarget(ProjectExplorer::Target*)),
        this, SLOT(handleTargetChanged(ProjectExplorer::Target*)));
    connect(project(), SIGNAL(aboutToRemoveTarget(ProjectExplorer::Target*)),
        SLOT(handleTargetToBeRemoved(ProjectExplorer::Target*)));

    if (!createAndroidTemplatesIfNecessary())
        return;

    m_androidFilesWatcher->addPath(androidDirPath());
    m_androidFilesWatcher->addPath(androidManifestPath());
    m_androidFilesWatcher->addPath(androidSrcPath());
    connect(m_androidFilesWatcher, SIGNAL(directoryChanged(QString)), this,
        SIGNAL(androidDirContentsChanged()));
    connect(m_androidFilesWatcher, SIGNAL(fileChanged(QString)), this,
        SIGNAL(androidDirContentsChanged()));
}

void AndroidTarget::handleTargetToBeRemoved(ProjectExplorer::Target *target)
{
    if (target != this)
        return;

// I don't think is a good idea to remove android directory

//    const QString debianPath = debianDirPath();
//    if (!QFileInfo(debianPath).exists())
//        return;
//    const int answer = QMessageBox::warning(0, tr("Qt Creator"),
//        tr("Do you want to remove the packaging directory\n"
//           "associated with the target '%1'?").arg(displayName()),
//        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
//    if (answer == QMessageBox::No)
//        return;
//    QString error;
//    if (!MaemoGlobal::removeRecursively(debianPath, error))
//        qDebug("%s", qPrintable(error));
//    const QString packagingPath = project()->projectDirectory()
//        + QLatin1Char('/') + PackagingDirName;
//    const QStringList otherContents = QDir(packagingPath).entryList(QDir::Dirs
//        | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot);
//    if (otherContents.isEmpty()) {
//        if (!MaemoGlobal::removeRecursively(packagingPath, error))
//            qDebug("%s", qPrintable(error));
//    }
}

QString AndroidTarget::androidDirPath() const
{
    return project()->projectDirectory() + QLatin1Char('/') + AndroidDirName;
}

QString AndroidTarget::androidManifestPath() const
{
    return androidDirPath() + QLatin1Char('/') + AndroidManifestName;
}

QString AndroidTarget::androidLibsPath() const
{
    return androidDirPath() + AndroidLibsFileName;
}

QString AndroidTarget::androidStringsPath() const
{
    return androidDirPath() + AndroidStringsFileName;
}

QString AndroidTarget::androidDefaultPropertiesPath() const
{
    return androidDirPath() + QLatin1Char('/') + AndroidDefaultPropertiesName;
}

QString AndroidTarget::androidSrcPath() const
{
    return androidDirPath() + QLatin1String("/src");
}

QString AndroidTarget::apkPath(BuildType buildType) const
{
    return project()->projectDirectory() + QLatin1Char('/')
            + AndroidDirName
            + QString::fromLatin1("/bin/%1-%2.apk")
            .arg(applicationName())
            .arg(buildType == DebugBuild ? QLatin1String("debug")
                                         : (buildType == ReleaseBuildUnsigned) ? QLatin1String("release-unsigned")
                                                                               : QLatin1String("signed"));
}

QString AndroidTarget::localLibsRulesFilePath() const
{
    const Qt4Project *const qt4Project = qobject_cast<const Qt4Project *>(project());
    if (!qt4Project || !qt4Project->activeTarget()->activeQt4BuildConfiguration()->qtVersion())
        return QLatin1String("");

    return qt4Project->activeTarget()->activeQt4BuildConfiguration()
            ->qtVersion()->versionInfo()[QLatin1String("QT_INSTALL_LIBS")] + QLatin1String("/rules.xml");
}

QString AndroidTarget::loadLocal(int apiLevel, ItemType item) const
{
    QString itemType;
    if (item == Lib)
        itemType = QLatin1String("lib");
    else
        itemType = QLatin1String("jar");

    QString localLibs;

    QDomDocument doc;
    if (!openXmlFile(doc, localLibsRulesFilePath()))
        return localLibs;

    QStringList libs;
    libs << qtLibs() << prebundledLibs();
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
                localLibs += libElement.attribute(QLatin1String("file")).arg(apiLevel) + QLatin1Char(':');
                libElement = libElement.nextSiblingElement(itemType);
            }

            libElement = element.firstChildElement(QLatin1String("replaces")).firstChildElement(itemType);
            while (!libElement.isNull()) {
                localLibs.replace(libElement.attribute(QLatin1String("file")).arg(apiLevel) + QLatin1Char(':'), QString());
                libElement = libElement.nextSiblingElement(itemType);
            }
        }
        element = element.nextSiblingElement(QLatin1String("lib"));
    }
    return localLibs;
}

QString AndroidTarget::loadLocalLibs(int apiLevel) const
{
    return loadLocal(apiLevel, Lib);
}

QString AndroidTarget::loadLocalJars(int apiLevel) const
{
    return loadLocal(apiLevel, Jar);
}

void AndroidTarget::updateProject(const QString &targetSDK, const QString &name) const
{
    QString androidDir = androidDirPath();

    // clean previous build
    QProcess androidProc;
    androidProc.setWorkingDirectory(androidDir);
    androidProc.start(AndroidConfigurations::instance().antToolPath(), QStringList() << QLatin1String("clean"));
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
    androidProc.start(AndroidConfigurations::instance().androidToolPath(), params);
    if (!androidProc.waitForFinished(-1))
        androidProc.terminate();
}

bool AndroidTarget::createAndroidTemplatesIfNecessary() const
{
    const Qt4Project *qt4Project = qobject_cast<Qt4Project*>(project());
    if (!qt4Project || !qt4Project->rootProjectNode() || !qt4Project->activeTarget() || !qt4Project->activeTarget()->activeQt4BuildConfiguration()
            || !qt4Project->activeTarget()->activeQt4BuildConfiguration()->qtVersion())
        return false;
    QString javaSrcPath = qt4Project->activeTarget()->activeQt4BuildConfiguration()->qtVersion()->versionInfo()[QLatin1String("QT_INSTALL_PREFIX")] + QLatin1String("/src/android/java");
    QDir projectDir(project()->projectDirectory());
    QString androidPath = androidDirPath();

    QStringList m_ignoreFiles;
    bool forceUpdate = false;
    QDomDocument srcVersionDoc;
    if (openXmlFile(srcVersionDoc, javaSrcPath + QLatin1String("/version.xml"), false)) {
        QDomDocument dstVersionDoc;
        if (openXmlFile(dstVersionDoc, androidPath + QLatin1String("/version.xml"), false))
            forceUpdate = (srcVersionDoc.documentElement().attribute(QLatin1String("value")).toDouble()
                           > dstVersionDoc.documentElement().attribute(QLatin1String("value")).toDouble());

        else
            forceUpdate = true;

        if (forceUpdate && QFileInfo(androidPath).exists()) {
            QDomElement ignoreFile = srcVersionDoc.documentElement().firstChildElement(QLatin1String("ignore")).firstChildElement(QLatin1String("file"));
            while (!ignoreFile.isNull()) {
                m_ignoreFiles << ignoreFile.text();
                ignoreFile = ignoreFile.nextSiblingElement();
            }
        }
    }

    if (!forceUpdate && QFileInfo(androidPath).exists()
            && QFileInfo(androidManifestPath()).exists()
            && QFileInfo(androidPath + QLatin1String("/src")).exists()
            && QFileInfo(androidPath + QLatin1String("/res")).exists())
        return true;

    forceUpdate &= QFileInfo(androidPath).exists();

    if (!QFileInfo(androidDirPath()).exists() && !projectDir.mkdir(AndroidDirName)) {
            raiseError(tr("Error creating Android directory '%1'.")
                .arg(AndroidDirName));
            return false;
    }

    QStringList androidFiles;
    QDirIterator it(javaSrcPath, QDirIterator::Subdirectories);
    int pos = it.path().size();
    while (it.hasNext()) {
        it.next();
        if (it.fileInfo().isDir()) {
            projectDir.mkpath(AndroidDirName + it.filePath().mid(pos));
        } else {
            const QString dstFile(androidPath + it.filePath().mid(pos));
            if (m_ignoreFiles.contains(it.fileName()))
                continue;
            else
            {
                if (QFile::exists(dstFile))
                    QFile::remove(dstFile);
                else
                    androidFiles << dstFile;
            }
            QFile::copy(it.filePath(), dstFile);
        }
    }
    if (androidFiles.size())
        qt4Project->rootProjectNode()->addFiles(UnknownFileType, androidFiles);

    QStringList sdks = AndroidConfigurations::instance().sdkTargets();
    if (sdks.isEmpty()) {
        raiseError(tr("No Qt for Android SDKs were found.\nPlease install at least one SDK."));
        return false;
    }
    updateProject(AndroidConfigurations::instance().sdkTargets().at(0));
    if (availableTargetApplications().length())
        setTargetApplication(availableTargetApplications()[0]);

    QString applicationName = project()->displayName();
    if (applicationName.length()) {
        setPackageName(packageName() + QLatin1Char('.') + applicationName);
        applicationName[0] = applicationName[0].toUpper();
        setApplicationName(applicationName);
    }

    if (forceUpdate)
        QMessageBox::warning(0, tr("Warning"), tr("Android files have been updated automatically"));

    return true;
}

bool AndroidTarget::openXmlFile(QDomDocument &doc, const QString &fileName, bool createAndroidTemplates) const
{
    if (createAndroidTemplates && !createAndroidTemplatesIfNecessary())
        return false;

    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly))
        return false;

    if (!doc.setContent(f.readAll())) {
        raiseError(tr("Can't parse '%1'").arg(fileName));
        return false;
    }
    return true;
}

bool AndroidTarget::saveXmlFile(QDomDocument &doc, const QString &fileName) const
{
    if (!createAndroidTemplatesIfNecessary())
        return false;

    QFile f(fileName);
    if (!f.open(QIODevice::WriteOnly)) {
        raiseError(tr("Can't open '%1'").arg(fileName));
        return false;
    }
    return f.write(doc.toByteArray(4)) >= 0;
}

bool AndroidTarget::openAndroidManifest(QDomDocument &doc) const
{
    return openXmlFile(doc, androidManifestPath());
}

bool AndroidTarget::saveAndroidManifest(QDomDocument &doc) const
{
    return saveXmlFile(doc, androidManifestPath());
}

bool AndroidTarget::openLibsXml(QDomDocument &doc) const
{
    return openXmlFile(doc, androidLibsPath());
}

bool AndroidTarget::saveLibsXml(QDomDocument &doc) const
{
    return saveXmlFile(doc, androidLibsPath());
}

QString AndroidTarget::activityName() const
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return QString();
    QDomElement activityElem = doc.documentElement().firstChildElement(QLatin1String("application")).firstChildElement(QLatin1String("activity"));
    return activityElem.attribute(QLatin1String("android:name"));
}

QString AndroidTarget::intentName() const
{
    return packageName() + QLatin1Char('/') + activityName();
}

QString AndroidTarget::packageName() const
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return QString();
    QDomElement manifestElem = doc.documentElement();
    return manifestElem.attribute(QLatin1String("package"));
}

bool AndroidTarget::setPackageName(const QString &name) const
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return false;
    QDomElement manifestElem = doc.documentElement();
    manifestElem.setAttribute(QLatin1String("package"), cleanPackageName(name));
    return saveAndroidManifest(doc);
}

QString AndroidTarget::applicationName() const
{
    QDomDocument doc;
    if (!openXmlFile(doc, androidStringsPath()))
        return QString();
    QDomElement metadataElem = doc.documentElement().firstChildElement(QLatin1String("string"));
    while (!metadataElem.isNull()) {
        if (metadataElem.attribute(QLatin1String("name")) == QLatin1String("app_name"))
            return metadataElem.text();
        metadataElem = metadataElem.nextSiblingElement(QLatin1String("string"));
    }
    return QString();
}

bool AndroidTarget::setApplicationName(const QString &name) const
{
    QDomDocument doc;
    if (!openXmlFile(doc, androidStringsPath()))
        return false;
    QDomElement metadataElem =doc.documentElement().firstChildElement(QLatin1String("string"));
    while (!metadataElem.isNull()) {
        if (metadataElem.attribute(QLatin1String("name")) == QLatin1String("app_name")) {
            metadataElem.removeChild(metadataElem.firstChild());
            metadataElem.appendChild(doc.createTextNode(name));
            break;
        }
        metadataElem = metadataElem.nextSiblingElement(QLatin1String("string"));
    }
    return saveXmlFile(doc, androidStringsPath());
}

QStringList AndroidTarget::availableTargetApplications() const
{
    QStringList apps;
    Qt4Project *qt4Project = qobject_cast<Qt4Project *>(project());
    foreach (Qt4ProFileNode *proFile, qt4Project->applicationProFiles()) {
        if (proFile->projectType() == ApplicationTemplate) {
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

QString AndroidTarget::targetApplication() const
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return QString();
    QDomElement metadataElem = doc.documentElement().firstChildElement(QLatin1String("application")).firstChildElement(QLatin1String("activity")).firstChildElement(QLatin1String("meta-data"));
    while (!metadataElem.isNull()) {
        if (metadataElem.attribute(QLatin1String("android:name")) == QLatin1String("android.app.lib_name"))
            return metadataElem.attribute(QLatin1String("android:value"));
        metadataElem = metadataElem.nextSiblingElement(QLatin1String("meta-data"));
    }
    return QString();
}

bool AndroidTarget::setTargetApplication(const QString &name) const
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return false;
    QDomElement metadataElem = doc.documentElement().firstChildElement(QLatin1String("application")).firstChildElement(QLatin1String("activity")).firstChildElement(QLatin1String("meta-data"));
    while (!metadataElem.isNull()) {
        if (metadataElem.attribute(QLatin1String("android:name")) == QLatin1String("android.app.lib_name")) {
            metadataElem.setAttribute(QLatin1String("android:value"), name);
            return saveAndroidManifest(doc);
        }
        metadataElem = metadataElem.nextSiblingElement(QLatin1String("meta-data"));
    }
    return false;
}

QString AndroidTarget::targetApplicationPath() const
{
    QString selectedApp = targetApplication();
    if (!selectedApp.length())
        return QString();
    Qt4Project *qt4Project = qobject_cast<Qt4Project *>(project());
    foreach (Qt4ProFileNode *proFile, qt4Project->applicationProFiles()) {
        if (proFile->projectType() == ApplicationTemplate) {
            if (proFile->targetInformation().target.startsWith(QLatin1String("lib"))
                    && proFile->targetInformation().target.endsWith(QLatin1String(".so"))) {
                if (proFile->targetInformation().target.mid(3, proFile->targetInformation().target.lastIndexOf(QLatin1Char('.')) - 3)
                        == selectedApp)
                    return proFile->targetInformation().buildDir + QLatin1String("/") + proFile->targetInformation().target;
            } else {
                if (proFile->targetInformation().target == selectedApp)
                    return proFile->targetInformation().buildDir + QLatin1String("/lib") + proFile->targetInformation().target + QLatin1String(".so");
            }
        }
    }
    return QString();
}

int AndroidTarget::versionCode() const
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return 0;
    QDomElement manifestElem = doc.documentElement();
    return manifestElem.attribute(QLatin1String("android:versionCode")).toInt();
}

bool AndroidTarget::setVersionCode(int version) const
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return false;
    QDomElement manifestElem = doc.documentElement();
    manifestElem.setAttribute(QLatin1String("android:versionCode"), version);
    return saveAndroidManifest(doc);
}


QString AndroidTarget::versionName() const
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return QString();
    QDomElement manifestElem = doc.documentElement();
    return manifestElem.attribute(QLatin1String("android:versionName"));
}

bool AndroidTarget::setVersionName(const QString &version) const
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return false;
    QDomElement manifestElem = doc.documentElement();
    manifestElem.setAttribute(QLatin1String("android:versionName"), version);
    return saveAndroidManifest(doc);
}

QStringList AndroidTarget::permissions() const
{
    QStringList per;
    QDomDocument doc;
    if (!openAndroidManifest(doc))
        return per;
    QDomElement permissionElem = doc.documentElement().firstChildElement(QLatin1String("uses-permission"));
    while (!permissionElem.isNull()) {
        per << permissionElem.attribute(QLatin1String("android:name"));
        permissionElem = permissionElem.nextSiblingElement(QLatin1String("uses-permission"));
    }
    return per;
}

bool AndroidTarget::setPermissions(const QStringList &permissions) const
{
    QDomDocument doc;
    if (!openAndroidManifest(doc))
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

    return saveAndroidManifest(doc);
}


QStringList AndroidTarget::getDependencies(const QString &readelfPath, const QString &lib) const
{
    QStringList libs;

    QProcess readelfProc;
    readelfProc.start(readelfPath, QStringList() << QLatin1String("-d") << QLatin1String("-W") << lib);

    if (!readelfProc.waitForFinished(-1)) {
        readelfProc.terminate();
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

int AndroidTarget::setLibraryLevel(const QString &library, LibrariesMap &mapLibs) const
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

bool AndroidTarget::qtLibrariesLessThan(const Library &a, const Library &b)
{
    if (a.level == b.level)
        return a.name < b.name;
    return a.level < b.level;
}

QStringList AndroidTarget::availableQtLibs() const
{
    const QString readelfPath = AndroidConfigurations::instance().readelfPath(activeRunConfiguration()->abi().architecture());
    QStringList libs;
    const Qt4Project *const qt4Project = qobject_cast<const Qt4Project *>(project());
    if (!qt4Project || !qt4Project->activeTarget()->activeQt4BuildConfiguration()->qtVersion())
        return libs;
    QString qtLibsPath = qt4Project->activeTarget()->activeQt4BuildConfiguration()->qtVersion()->versionInfo()[QLatin1String("QT_INSTALL_LIBS")];
    if (!QFile::exists(readelfPath)) {
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
        mapLibs[library].dependencies = getDependencies(readelfPath, libPath.absolutePath());
    }

    // clean dependencies
    foreach (const QString &key, mapLibs.keys()) {
        int it = 0;
        while (it < mapLibs[key].dependencies.size()) {
            const QString &dependName = mapLibs[key].dependencies[it];
            if (!mapLibs.keys().contains(dependName) && dependName.startsWith(QLatin1String("lib")) && dependName.endsWith(QLatin1String(".so"))) {
                mapLibs[key].dependencies.removeAt(it);
            } else {
                ++it;
            }
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

QIcon AndroidTarget::androidIcon(AndroidIconType type) const
{
    switch (type) {
    case HighDPI:
        return QIcon(androidDirPath() + QLatin1String("/res/drawable-hdpi/icon.png"));
    case MediumDPI:
        return QIcon(androidDirPath() + QLatin1String("/res/drawable-mdpi/icon.png"));
    case LowDPI:
        return QIcon(androidDirPath() + QLatin1String("/res/drawable-ldpi/icon.png"));
    }
    return QIcon();
}

bool AndroidTarget::setAndroidIcon(AndroidIconType type, const QString &iconFileName) const
{
    switch (type) {
    case HighDPI:
        QFile::remove(androidDirPath() + QLatin1String("/res/drawable-hdpi/icon.png"));
        return QFile::copy(iconFileName, androidDirPath() + QLatin1String("/res/drawable-hdpi/icon.png"));
    case MediumDPI:
        QFile::remove(androidDirPath() + QLatin1String("/res/drawable-mdpi/icon.png"));
        return QFile::copy(iconFileName, androidDirPath() + QLatin1String("/res/drawable-mdpi/icon.png"));
    case LowDPI:
        QFile::remove(androidDirPath() + QLatin1String("/res/drawable-ldpi/icon.png"));
        return QFile::copy(iconFileName, androidDirPath() + QLatin1String("/res/drawable-ldpi/icon.png"));
    }
    return false;
}

QStringList AndroidTarget::libsXml(const QString &tag) const
{
    QStringList libs;
    QDomDocument doc;
    if (!openLibsXml(doc))
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

bool AndroidTarget::setLibsXml(const QStringList &libs, const QString &tag) const
{
    QDomDocument doc;
    if (!openLibsXml(doc))
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
            return saveLibsXml(doc);
        }
        arrayElem = arrayElem.nextSiblingElement(QLatin1String("array"));
    }
    return false;
}

QStringList AndroidTarget::qtLibs() const
{
    return libsXml(QLatin1String("qt_libs"));
}

bool AndroidTarget::setQtLibs(const QStringList &libs) const
{
    return setLibsXml(libs, QLatin1String("qt_libs"));
}

QStringList AndroidTarget::availablePrebundledLibs() const
{
    QStringList libs;
    Qt4Project *qt4Project = qobject_cast<Qt4Project *>(project());
    QList<Qt4Project *> qt4Projects;
    qt4Projects << qt4Project;

    foreach (Qt4Project *qt4Project, qt4Projects)
        foreach (Qt4ProFileNode *node, qt4Project->allProFiles())
            if (node->projectType() == LibraryTemplate)
                libs << QLatin1String("lib") + node->targetInformation().target + QLatin1String(".so");

    return libs;
}

QStringList AndroidTarget::prebundledLibs() const
{
    return libsXml(QLatin1String("bundled_libs"));
}

bool AndroidTarget::setPrebundledLibs(const QStringList &libs) const
{

    return setLibsXml(libs, QLatin1String("bundled_libs"));
}

QIcon AndroidTarget::highDpiIcon() const
{
    return androidIcon(HighDPI);
}

bool AndroidTarget::setHighDpiIcon(const QString &iconFilePath) const
{
    return setAndroidIcon(HighDPI, iconFilePath);
}

QIcon AndroidTarget::mediumDpiIcon() const
{
    return androidIcon(MediumDPI);
}

bool AndroidTarget::setMediumDpiIcon(const QString &iconFilePath) const
{
    return setAndroidIcon(MediumDPI, iconFilePath);
}

QIcon AndroidTarget::lowDpiIcon() const
{
    return androidIcon(LowDPI);
}

bool AndroidTarget::setLowDpiIcon(const QString &iconFilePath) const
{
    return setAndroidIcon(LowDPI, iconFilePath);
}

QString AndroidTarget::targetSDK() const
{
    if (!createAndroidTemplatesIfNecessary())
        return AndroidConfigurations::instance().bestMatch(QLatin1String("android-8"));
    QFile file(androidDefaultPropertiesPath());
    if (!file.open(QIODevice::ReadOnly))
        return AndroidConfigurations::instance().bestMatch(QLatin1String("android-8"));
    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        if (line.startsWith("target="))
            return QString::fromLatin1(line.trimmed().mid(7));
    }
    return AndroidConfigurations::instance().bestMatch(QLatin1String("android-8"));
}

bool AndroidTarget::setTargetSDK(const QString &target) const
{
    updateProject(target, applicationName());
    return true;
}

void AndroidTarget::raiseError(const QString &reason) const
{
    QMessageBox::critical(0, tr("Error creating Android templates"), reason);
}


} // namespace Internal
} // namespace Qt4ProjectManager
