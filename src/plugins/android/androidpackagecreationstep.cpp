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

#include "androidpackagecreationstep.h"

#include "androidconstants.h"
#include "androiddeploystep.h"
#include "androidglobal.h"
#include "androidpackagecreationwidget.h"
#include "androidmanager.h"
#include "androidgdbserverkitinformation.h"
#include "androidtoolchain.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4nodes.h>
#include <qtsupport/qtkitinformation.h>

#include <coreplugin/icore.h>
#include <coreplugin/fileutils.h>

#include <QAbstractListModel>
#include <QProcess>
#include <QVector>
#include <QPair>
#include <QWidget>
#include <QMessageBox>
#include <QInputDialog>
#include <QMainWindow>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Constants;

namespace Android {
namespace Internal {

namespace {
    const QLatin1String KeystoreLocationKey("KeystoreLocation");
    const QLatin1String AliasString("Alias name:");
    const QLatin1String CertificateSeparator("*******************************************");
}

using namespace Qt4ProjectManager;

class CertificatesModel: public QAbstractListModel
{
public:
    CertificatesModel(const QString &rowCertificates, QObject *parent)
        : QAbstractListModel(parent)
    {
        int from = rowCertificates.indexOf(AliasString);
        QPair<QString, QString> item;
        while (from > -1) {
            from += 11;// strlen(AliasString);
            const int eol = rowCertificates.indexOf(QLatin1Char('\n'), from);
            item.first = rowCertificates.mid(from, eol - from).trimmed();
            const int eoc = rowCertificates.indexOf(CertificateSeparator, eol);
            item.second = rowCertificates.mid(eol + 1, eoc - eol - 2).trimmed();
            from = rowCertificates.indexOf(AliasString, eoc);
            m_certs.push_back(item);
        }
    }

protected:
    int rowCount(const QModelIndex &parent = QModelIndex()) const
    {
        if (parent.isValid())
            return 0;
        return m_certs.size();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
    {
        if (!index.isValid() || (role != Qt::DisplayRole && role != Qt::ToolTipRole))
            return QVariant();
        if (role == Qt::DisplayRole)
            return m_certs[index.row()].first;
        return m_certs[index.row()].second;
    }

private:
    QVector<QPair<QString, QString> > m_certs;
};


AndroidPackageCreationStep::AndroidPackageCreationStep(BuildStepList *bsl)
    : BuildStep(bsl, CreatePackageId)
{
    ctor();
}

AndroidPackageCreationStep::AndroidPackageCreationStep(BuildStepList *bsl,
    AndroidPackageCreationStep *other)
    : BuildStep(bsl, other)
{
    ctor();
}

void AndroidPackageCreationStep::ctor()
{
    setDefaultDisplayName(tr("Packaging for Android"));
    m_openPackageLocation = true;
    m_bundleQt = false;
    connect(&m_outputParser, SIGNAL(addTask(ProjectExplorer::Task)), this, SIGNAL(addTask(ProjectExplorer::Task)));
}

bool AndroidPackageCreationStep::init()
{
    const Qt4BuildConfiguration *bc = qobject_cast<Qt4BuildConfiguration *>(target()->activeBuildConfiguration());
    if (!bc) {
        raiseError(tr("Cannot create Android package: current build configuration is not Qt 4."));
        return false;
    }
    Qt4Project *project = static_cast<Qt4Project *>(target()->project());
    m_outputParser.setProjectFileList(project->files(Project::AllFiles));

    // Copying
    m_androidDir = AndroidManager::dirPath(target());
    Utils::FileName path = m_androidDir;
    QString androidTargetArch = project->rootQt4ProjectNode()->singleVariableValue(Qt4ProjectManager::AndroidArchVar);
    if (androidTargetArch.isEmpty()) {
        raiseError(tr("Cannot create Android package: No ANDROID_TARGET_ARCH set in make spec."));
        return false;
    }

    Utils::FileName androidLibPath = path.appendPath(QLatin1String("libs/") + androidTargetArch);
    m_gdbServerDestination = androidLibPath.appendPath(QLatin1String("gdbserver"));
    m_gdbServerSource = AndroidGdbServerKitInformation::gdbServer(target()->kit());
    m_debugBuild = bc->qmakeBuildConfiguration() & QtSupport::BaseQtVersion::DebugBuild;

    if (!AndroidManager::createAndroidTemplatesIfNecessary(target()))
        return false;

    AndroidManager::updateTarget(target(), AndroidManager::targetSDK(target()), AndroidManager::applicationName(target()));
    m_antToolPath = AndroidConfigurations::instance().antToolPath();
    m_apkPathUnsigned = AndroidManager::apkPath(target(), AndroidManager::ReleaseBuildUnsigned);
    m_apkPathSigned = AndroidManager::apkPath(target(), AndroidManager::ReleaseBuildSigned);
    m_keystorePathForRun = m_keystorePath;
    m_certificatePasswdForRun = m_certificatePasswd;
    m_jarSigner = AndroidConfigurations::instance().jarsignerPath();
    m_zipAligner = AndroidConfigurations::instance().zipalignPath();
    m_environment = bc->environment();

    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(target()->kit());
    if (tc->type() != QLatin1String(Constants::ANDROID_TOOLCHAIN_TYPE))
        return false;

    initCheckRequiredLibrariesForRun();
    return true;
}

void AndroidPackageCreationStep::run(QFutureInterface<bool> &fi)
{
    fi.reportResult(createPackage());
}

BuildStepConfigWidget *AndroidPackageCreationStep::createConfigWidget()
{
    return new AndroidPackageCreationWidget(this);
}

static inline QString msgCannotFindElfInformation()
{
    return AndroidPackageCreationStep::tr("Cannot find ELF information");
}

static inline QString msgCannotFindExecutable(const QString &appPath)
{
    return AndroidPackageCreationStep::tr("Cannot find '%1'.\n"
        "Please make sure your application is "
        "built successfully and is selected in Application tab ('Run option').").arg(appPath);
}

static void parseSharedLibs(const QByteArray &buffer, QStringList *libs)
{
#if defined(_WIN32)
    QList<QByteArray> lines = buffer.trimmed().split('\r');
#else
    QList<QByteArray> lines = buffer.trimmed().split('\n');
#endif
    foreach (const QByteArray &line, lines) {
        if (line.contains("(NEEDED)") && line.contains("Shared library:") ) {
            const int pos = line.lastIndexOf('[') + 1;
            (*libs) << QString::fromLatin1(line.mid(pos, line.length() - pos - 1));
        }
    }
}

void markNeeded(const QString &library,
                const QVector<AndroidManager::Library> &dependencies,
                QMap<QString, bool> *neededMap)
{
    if (!neededMap->contains(library))
        return;
    if (neededMap->value(library))
        return;
    neededMap->insert(library, true);
    for (int i = 0; i < dependencies.size(); ++i) {
        if (dependencies.at(i).name == library) {
            foreach (const QString &dependency, dependencies.at(i).dependencies)
                markNeeded(dependency, dependencies, neededMap);
            break;
        }
    }
}

QStringList requiredLibraries(QVector<AndroidManager::Library> availableLibraries,
                      const QStringList &checkedLibs, const QStringList &dependencies)
{
    QMap<QString, bool> neededLibraries;
    QVector<AndroidManager::Library>::const_iterator it, end;
    it = availableLibraries.constBegin();
    end = availableLibraries.constEnd();

    for (; it != end; ++it)
        neededLibraries[(*it).name] = false;

    // Checked items are always needed
    foreach (const QString &lib, checkedLibs)
        markNeeded(lib, availableLibraries, &neededLibraries);

    foreach (const QString &lib, dependencies) {
        if (lib.startsWith(QLatin1String("lib"))
                && lib.endsWith(QLatin1String(".so")))
            markNeeded(lib.mid(3, lib.size() - 6), availableLibraries, &neededLibraries);
    }

    for (int i = availableLibraries.size() - 1; i>= 0; --i)
        if (!neededLibraries.value(availableLibraries.at(i).name))
            availableLibraries.remove(i);

    QStringList requiredLibraries;
    foreach (const AndroidManager::Library &lib, availableLibraries) {
        if (neededLibraries.value(lib.name))
            requiredLibraries << lib.name;
    }
    return requiredLibraries;
}

void AndroidPackageCreationStep::checkRequiredLibraries()
{
    QProcess readelfProc;
    QString appPath = AndroidManager::targetApplicationPath(target());
    if (!QFile::exists(appPath)) {
        raiseError(msgCannotFindElfInformation(), msgCannotFindExecutable(appPath));
        return;
    }

    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(target()->kit());
    if (tc->type() != QLatin1String(Constants::ANDROID_TOOLCHAIN_TYPE))
        return;
    AndroidToolChain *atc = static_cast<AndroidToolChain *>(tc);

    readelfProc.start(AndroidConfigurations::instance().readelfPath(target()->activeRunConfiguration()->abi().architecture(), atc->ndkToolChainVersion()).toString(),
                      QStringList() << QLatin1String("-d") << QLatin1String("-W") << appPath);
    if (!readelfProc.waitForFinished(-1)) {
        readelfProc.kill();
        return;
    }
    QStringList libs;
    parseSharedLibs(readelfProc.readAll(), &libs);
    AndroidManager::setQtLibs(target(), requiredLibraries(AndroidManager::availableQtLibsWithDependencies(target()),
                                                          AndroidManager::qtLibs(target()), libs));
    emit updateRequiredLibrariesModels();
}

void AndroidPackageCreationStep::initCheckRequiredLibrariesForRun()
{
    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(target()->kit());
    if (tc->type() != QLatin1String(Constants::ANDROID_TOOLCHAIN_TYPE))
        return;
    AndroidToolChain *atc = static_cast<AndroidToolChain *>(tc);

    m_appPath = Utils::FileName::fromString(AndroidManager::targetApplicationPath(target()));
    m_readElf = AndroidConfigurations::instance().readelfPath(target()->activeRunConfiguration()->abi().architecture(),
                                                              atc->ndkToolChainVersion());
    m_qtLibs = AndroidManager::qtLibs(target());
    m_availableQtLibs = AndroidManager::availableQtLibsWithDependencies(target());
    m_prebundledLibs = AndroidManager::prebundledLibs(target());
}

void AndroidPackageCreationStep::getBundleInformation()
{
    m_bundleQt = AndroidManager::bundleQt(target());
    if (m_bundleQt) {
        m_bundledJars = AndroidManager::loadLocalJars(target()).split(QLatin1Char(':'),
                                                                      QString::SkipEmptyParts);
        m_otherBundledFiles = AndroidManager::loadLocalBundledFiles(target()).split(QLatin1Char(':'),
                                                                                    QString::SkipEmptyParts);
    }
}

void AndroidPackageCreationStep::checkRequiredLibrariesForRun()
{
    QProcess readelfProc;
    if (!m_appPath.toFileInfo().exists()) {
        raiseError(msgCannotFindElfInformation(), msgCannotFindExecutable(m_appPath.toUserOutput()));
        return;
    }
    readelfProc.start(m_readElf.toString(), QStringList() << QLatin1String("-d") << QLatin1String("-W") << m_appPath.toUserOutput());
    if (!readelfProc.waitForFinished(-1)) {
        readelfProc.kill();
        return;
    }
    QStringList libs;
    parseSharedLibs(readelfProc.readAll(), &libs);

    m_qtLibsWithDependencies = requiredLibraries(m_availableQtLibs, m_qtLibs, libs);
    QMetaObject::invokeMethod(this, "setQtLibs",Qt::BlockingQueuedConnection,
                              Q_ARG(QStringList, m_qtLibsWithDependencies));

    QMetaObject::invokeMethod(this, "getBundleInformation", Qt::BlockingQueuedConnection);

    emit updateRequiredLibrariesModels();
}

void AndroidPackageCreationStep::setQtLibs(const QStringList &qtLibs)
{
    AndroidManager::setQtLibs(target(), qtLibs);
}

void AndroidPackageCreationStep::setPrebundledLibs(const QStringList &prebundledLibs)
{
    AndroidManager::setPrebundledLibs(target(), prebundledLibs);
}

Utils::FileName AndroidPackageCreationStep::keystorePath()
{
    return m_keystorePath;
}

void AndroidPackageCreationStep::setKeystorePath(const Utils::FileName &path)
{
    m_keystorePath = path;
    m_certificatePasswd.clear();
    m_keystorePasswd.clear();
}

void AndroidPackageCreationStep::setKeystorePassword(const QString &pwd)
{
    m_keystorePasswd = pwd;
}

void AndroidPackageCreationStep::setCertificateAlias(const QString &alias)
{
    m_certificateAlias = alias;
}

void AndroidPackageCreationStep::setCertificatePassword(const QString &pwd)
{
    m_certificatePasswd = pwd;
}

void AndroidPackageCreationStep::setOpenPackageLocation(bool open)
{
    m_openPackageLocation = open;
}

QAbstractItemModel *AndroidPackageCreationStep::keystoreCertificates()
{
    QString rawCerts;
    QProcess keytoolProc;
    while (!rawCerts.length() || !m_keystorePasswd.length()) {
        QStringList params;
        params << QLatin1String("-list") << QLatin1String("-v") << QLatin1String("-keystore") << m_keystorePath.toUserOutput() << QLatin1String("-storepass");
        if (!m_keystorePasswd.length())
            keystorePassword();
        if (!m_keystorePasswd.length())
            return 0;
        params << m_keystorePasswd;
        Utils::Environment env = Utils::Environment::systemEnvironment();
        env.set(QLatin1String("LANG"), QLatin1String("C"));
        keytoolProc.setProcessEnvironment(env.toProcessEnvironment());
        keytoolProc.start(AndroidConfigurations::instance().keytoolPath().toString(), params);
        if (!keytoolProc.waitForStarted() || !keytoolProc.waitForFinished()) {
            QMessageBox::critical(0, tr("Error"),
                                  tr("Failed to run keytool"));
            return 0;
        }

        if (keytoolProc.exitCode()) {
            QMessageBox::critical(0, tr("Error"),
                                  tr("Invalid password"));
            m_keystorePasswd.clear();
        }
        rawCerts = QString::fromLatin1(keytoolProc.readAllStandardOutput());
    }
    return new CertificatesModel(rawCerts, this);
}

bool AndroidPackageCreationStep::fromMap(const QVariantMap &map)
{
    if (!BuildStep::fromMap(map))
        return false;
    m_keystorePath = Utils::FileName::fromString(map.value(KeystoreLocationKey).toString());
    return true;
}

QVariantMap AndroidPackageCreationStep::toMap() const
{
    QVariantMap map(BuildStep::toMap());
    map.insert(KeystoreLocationKey, m_keystorePath.toString());
    return map;
}

QStringList AndroidPackageCreationStep::collectRelativeFilePaths(const QString &parentPath)
{
    QStringList relativeFilePaths;

    QDirIterator libsIt(parentPath, QDir::NoFilter, QDirIterator::Subdirectories);
    int pos = parentPath.size();
    while (libsIt.hasNext()) {
        libsIt.next();
        if (!libsIt.fileInfo().isDir())
            relativeFilePaths.append(libsIt.filePath().mid(pos));
    }

    return relativeFilePaths;
}

void AndroidPackageCreationStep::collectFiles(QList<DeployItem> *deployList,
                                              QList<DeployItem> *pluginsAndImportsList)
{
    Q_ASSERT(deployList != 0);
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target()->kit());
    if (!version)
        return;

    Qt4Project *project = static_cast<Qt4Project *>(target()->project());
    QString androidTargetArch = project->rootQt4ProjectNode()->singleVariableValue(Qt4ProjectManager::AndroidArchVar);

    QString androidAssetsPath = m_androidDir.toString() + QLatin1String("/assets/");
    QString androidJarPath = m_androidDir.toString() + QLatin1String("/libs/");
    QString androidLibPath = m_androidDir.toString() + QLatin1String("/libs/") + androidTargetArch;

    QString qtVersionSourcePath = version->sourcePath().toString();

    foreach (QString qtLib, m_qtLibsWithDependencies) {
        QString fullPath = qtVersionSourcePath
            + QLatin1String("/lib/lib")
            + qtLib
            + QLatin1String(".so");
        QString destinationPath = androidLibPath
                + QLatin1String("/lib")
                + qtLib
                + QLatin1String(".so");

        // If the Qt lib/ folder contains libgnustl_shared.so, don't deploy it from there, since
        // it will be deployed directly from the NDK instead.
        if (qtLib != QLatin1String("gnustl_shared")) {
            DeployItem deployItem(fullPath, 0, destinationPath, true);
            deployList->append(deployItem);
        }
    }

    if (!androidTargetArch.isEmpty()) {
        ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(target()->kit());
        if (tc->type() != QLatin1String(Constants::ANDROID_TOOLCHAIN_TYPE))
            return;

        AndroidToolChain *atc = static_cast<AndroidToolChain *>(tc);

        QString libgnustl = AndroidManager::libGnuStl(androidTargetArch, atc->ndkToolChainVersion());
        DeployItem deployItem(libgnustl, 0, androidLibPath + QLatin1String("/libgnustl_shared.so"), false);
        deployList->append(deployItem);
    }

    foreach (QString jar, m_bundledJars) {
        QString fullPath = qtVersionSourcePath + QLatin1Char('/') + jar;
        QFileInfo fileInfo(fullPath);
        if (fileInfo.exists()) {
            QString destinationPath = androidJarPath
                    + AndroidManager::libraryPrefix()
                    + fileInfo.fileName();
            deployList->append(DeployItem(fullPath, 0, destinationPath, true));
        }
    }

    QSet<QString> alreadyListed;
    foreach (QString bundledFile, m_otherBundledFiles) {
        QStringList allFiles;
        if (QFileInfo(qtVersionSourcePath + QLatin1Char('/') + bundledFile).isDir()) {
            if (!bundledFile.endsWith(QLatin1Char('/')))
                bundledFile.append(QLatin1Char('/'));

            allFiles = collectRelativeFilePaths(qtVersionSourcePath + QLatin1Char('/') + bundledFile);
        } else {
            // If we need to bundle a specific file, we just add an empty string and the file
            // names and data will be prepared correctly in the loop below.
            allFiles = QStringList(QString());
        }

        foreach (QString file, allFiles) {
            QString fullPath = qtVersionSourcePath + QLatin1Char('/') + bundledFile + file;
            if (alreadyListed.contains(fullPath))
                continue;

            alreadyListed.insert(fullPath);

            QString garbledFileName;
            QString destinationPath;
            bool shouldStrip = false;

            QString fullFileName = bundledFile + file;
            if (fullFileName.endsWith(QLatin1String(".so"))) {
                if (fullFileName.startsWith(QLatin1String("lib/"))) {
                    // Special case when the destination folder is lib/
                    // Since this is also the source folder, there is no need to garble the file
                    // name and copy it. We also won't have write access to this folder, so we
                    // couldn't if we wanted to.
                    garbledFileName = fullFileName.mid(sizeof("lib/") - 1);
                } else {
                    garbledFileName = QLatin1String("lib")
                        + AndroidManager::libraryPrefix()
                        + QString(fullFileName).replace(QLatin1Char('/'), QLatin1Char('_'));
                }
                destinationPath = androidLibPath + QLatin1Char('/') + garbledFileName;
                shouldStrip = true;
            } else {
                garbledFileName = AndroidManager::libraryPrefix() + QLatin1Char('/') + fullFileName;
                destinationPath = androidAssetsPath + garbledFileName;
            }

            deployList->append(DeployItem(fullPath, 0, destinationPath, shouldStrip));
            pluginsAndImportsList->append(DeployItem(garbledFileName,
                                                     0,
                                                     fullFileName,
                                                     shouldStrip));
        }
    }
}

void AndroidPackageCreationStep::removeManagedFilesFromPackage()
{
    // Clean up all files managed by Qt Creator
    {
        QString androidLibPath = m_androidDir.toString() + QLatin1String("/libs/");
        QDirIterator dirIt(m_androidDir.toString(), QDirIterator::Subdirectories);
        while (dirIt.hasNext()) {
            dirIt.next();

            if (!dirIt.fileInfo().isDir()) {
                bool isQtLibrary = dirIt.fileInfo().path().startsWith(androidLibPath)
                        && dirIt.fileName().startsWith(QLatin1String("libQt5"))
                        && dirIt.fileName().endsWith(QLatin1String(".so"));

                if (dirIt.filePath().contains(AndroidManager::libraryPrefix()) || isQtLibrary)
                    QFile::remove(dirIt.filePath());
            }
        }
    }

    removeDirectory(m_androidDir.toString() + QLatin1String("/assets/") + AndroidManager::libraryPrefix());
}

void AndroidPackageCreationStep::copyFilesIntoPackage(const QList<DeployItem> &deployList)
{
    foreach (DeployItem item, deployList) {
        QFileInfo info(item.remoteFileName);
        if (info.exists())
            QFile::remove(item.remoteFileName);
        else
            QDir().mkpath(info.absolutePath());

        QFile::copy(item.localFileName, item.remoteFileName);
    }
}

void AndroidPackageCreationStep::stripFiles(const QList<DeployItem> &deployList)
{

    QStringList fileList;
    foreach (DeployItem item, deployList)
        if (item.needsStrip)
            fileList.append(item.remoteFileName);

    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(target()->kit());
    if (tc->type() != QLatin1String(Constants::ANDROID_TOOLCHAIN_TYPE))
        return;

    AndroidToolChain *atc = static_cast<AndroidToolChain *>(tc);
    stripAndroidLibs(fileList,
                     target()->activeRunConfiguration()->abi().architecture(),
                     atc->ndkToolChainVersion());
}

void AndroidPackageCreationStep::updateXmlForFiles(const QStringList &inLibList,
                                                   const QStringList &inAssetsList)
{
    AndroidManager::setBundledInLib(target(), inLibList);
    AndroidManager::setBundledInAssets(target(), inAssetsList);
}


bool AndroidPackageCreationStep::createPackage()
{
    checkRequiredLibrariesForRun();

    emit addOutput(tr("Copy Qt app & libs to Android package ..."), MessageOutput);

    QStringList build;
    build << QLatin1String("clean");
    QFile::remove(m_gdbServerDestination.toString());
    if (m_debugBuild || !m_certificateAlias.length()) {
        build << QLatin1String("debug");
        QDir dir;
        dir.mkpath(m_gdbServerDestination.toFileInfo().absolutePath());
        if (!QFile::copy(m_gdbServerSource.toString(), m_gdbServerDestination.toString())) {
            raiseError(tr("Can't copy gdbserver from '%1' to '%2'").arg(m_gdbServerSource.toUserOutput())
                       .arg(m_gdbServerDestination.toUserOutput()));
            return false;
        }
    } else {
        build << QLatin1String("release");
    }

    QList<DeployItem> deployFiles;
    QList<DeployItem> importsAndPlugins;

    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target()->kit());

    // Qt 5 supports bundling libraries inside the apk. We guard the code for Qt 5 to be sure we
    // do not disrupt existing projects.
    if (version && version->qtVersion() >= QtSupport::QtVersionNumber(5, 0, 0)) {
        bool bundleQt = AndroidManager::bundleQt(target());

        // Collect the files to bundle in the package
        if (bundleQt)
            collectFiles(&deployFiles, &importsAndPlugins);

        // Remove files from package if they are not needed
        removeManagedFilesFromPackage();

        // Deploy files to package
        if (bundleQt) {
            copyFilesIntoPackage(deployFiles);
            stripFiles(deployFiles);

            QStringList inLibList;
            QStringList inAssetsList;
            foreach (DeployItem deployItem, importsAndPlugins) {
                QString conversionInfo = deployItem.localFileName
                        + QLatin1Char(':')
                        + deployItem.remoteFileName;

                if (deployItem.localFileName.endsWith(QLatin1String(".so")))
                    inLibList.append(conversionInfo);
                else
                    inAssetsList.append(conversionInfo);
            }

            QMetaObject::invokeMethod(this,
                                      "updateXmlForFiles",
                                      Qt::BlockingQueuedConnection,
                                      Q_ARG(QStringList, inLibList),
                                      Q_ARG(QStringList, inAssetsList));
        }
    }

    emit addOutput(tr("Creating package file ..."), MessageOutput);

    QProcess *const buildProc = new QProcess;
    buildProc->setProcessEnvironment(m_environment.toProcessEnvironment());

    connect(buildProc, SIGNAL(readyReadStandardOutput()), this,
        SLOT(handleBuildStdOutOutput()), Qt::DirectConnection);
    connect(buildProc, SIGNAL(readyReadStandardError()), this,
        SLOT(handleBuildStdErrOutput()), Qt::DirectConnection);

    buildProc->setWorkingDirectory(m_androidDir.toString());

    if (!runCommand(buildProc, m_antToolPath.toString(), build)) {
        disconnect(buildProc, 0, this, 0);
        buildProc->deleteLater();
        return false;
    }

    if (!(m_debugBuild) && m_certificateAlias.length()) {
        emit addOutput(tr("Signing package ..."), MessageOutput);
        while (true) {
            if (m_certificatePasswdForRun.isEmpty())
                QMetaObject::invokeMethod(this, "certificatePassword", Qt::BlockingQueuedConnection);

            if (m_certificatePasswdForRun.isEmpty()) {
                disconnect(buildProc, 0, this, 0);
                buildProc->deleteLater();
                return false;
            }

            QByteArray keyPass = m_certificatePasswdForRun.toUtf8();
            build.clear();
            build << QLatin1String("-verbose") << QLatin1String("-keystore") << m_keystorePathForRun.toUserOutput()
                  << QLatin1String("-storepass") << m_keystorePasswd
                  << m_apkPathUnsigned.toUserOutput()
                  << m_certificateAlias;
            buildProc->start(m_jarSigner.toString(), build);
            if (!buildProc->waitForStarted()) {
                disconnect(buildProc, 0, this, 0);
                buildProc->deleteLater();
                return false;
            }

            keyPass += '\n';
            buildProc->write(keyPass);
            buildProc->waitForBytesWritten();
            buildProc->waitForFinished();

            if (!buildProc->exitCode())
                break;
            emit addOutput(tr("Failed, try again"), ErrorMessageOutput);
            m_certificatePasswdForRun.clear();
        }
        build.clear();
        build << QLatin1String("-f") << QLatin1String("-v") << QLatin1String("4") << m_apkPathUnsigned.toString() << m_apkPathSigned.toString();
        buildProc->start(m_zipAligner.toString(), build);
        buildProc->waitForFinished();
        if (!buildProc->exitCode()) {
            QFile::remove(m_apkPathUnsigned.toString());
            emit addOutput(tr("Release signed package created to %1")
                           .arg(m_apkPathSigned.toUserOutput())
                           , MessageOutput);

            if (m_openPackageLocation)
                QMetaObject::invokeMethod(this, "showInGraphicalShell", Qt::QueuedConnection);
        }
    }
    emit addOutput(tr("Package created."), BuildStep::MessageOutput);
    disconnect(buildProc, 0, this, 0);
    buildProc->deleteLater();
    return true;
}

void AndroidPackageCreationStep::stripAndroidLibs(const QStringList & files, Abi::Architecture architecture, const QString &ndkToolchainVersion)
{
    QProcess stripProcess;
    foreach (const QString &file, files) {
        stripProcess.start(AndroidConfigurations::instance().stripPath(architecture, ndkToolchainVersion).toString(),
                           QStringList()<<QLatin1String("--strip-unneeded") << file);
        stripProcess.waitForStarted();
        if (!stripProcess.waitForFinished())
            stripProcess.kill();
    }
}

bool AndroidPackageCreationStep::removeDirectory(const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists())
        return true;

    const QStringList &files
        = dir.entryList(QDir::Files | QDir::Hidden | QDir::System);
    foreach (const QString &fileName, files) {
        if (!dir.remove(fileName))
            return false;
    }

    const QStringList &subDirs
        = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (const QString &subDirName, subDirs) {
        if (!removeDirectory(dirPath + QLatin1Char('/') + subDirName))
            return false;
    }

    return dir.rmdir(dirPath);
}

bool AndroidPackageCreationStep::runCommand(QProcess *buildProc
    , const QString &program, const QStringList &arguments)
{
    emit addOutput(tr("Package deploy: Running command '%1 %2'.").arg(program).arg(arguments.join(QLatin1String(" "))), BuildStep::MessageOutput);
    buildProc->start(program, arguments);
    if (!buildProc->waitForStarted()) {
        raiseError(tr("Packaging failed."),
                   tr("Packaging error: Could not start command '%1 %2'. Reason: %3")
                               .arg(program).arg(arguments.join(QLatin1String(" "))).arg(buildProc->errorString()));
        return false;
    }
    buildProc->waitForFinished(-1);

    handleProcessOutput(buildProc, false);
    handleProcessOutput(buildProc, true);

    if (buildProc->error() != QProcess::UnknownError
        || buildProc->exitCode() != 0) {
        QString mainMessage = tr("Packaging Error: Command '%1 %2' failed.")
                .arg(program).arg(arguments.join(QLatin1String(" ")));
        if (buildProc->error() != QProcess::UnknownError)
            mainMessage += tr(" Reason: %1").arg(buildProc->errorString());
        else
            mainMessage += tr("Exit code: %1").arg(buildProc->exitCode());
        raiseError(mainMessage);
        return false;
    }
    return true;
}

void AndroidPackageCreationStep::handleBuildStdOutOutput()
{
    QProcess *const process = qobject_cast<QProcess *>(sender());
    if (!process)
        return;
    handleProcessOutput(process, false);
}

void AndroidPackageCreationStep::handleBuildStdErrOutput()
{
    QProcess *const process = qobject_cast<QProcess *>(sender());
    if (!process)
        return;

    handleProcessOutput(process, true);
}

void AndroidPackageCreationStep::handleProcessOutput(QProcess *process, bool stdErr)
{
    process->setReadChannel(stdErr ? QProcess::StandardError : QProcess::StandardOutput);
    while (process->canReadLine()) {
        QString line = QString::fromLocal8Bit(process->readLine());
        if (stdErr)
            m_outputParser.stdError(line);
        else
            m_outputParser.stdOutput(line);
        emit addOutput(line, stdErr ? BuildStep::ErrorOutput
                                    : BuildStep::NormalOutput,
                       BuildStep::DontAppendNewline);
    }
}

void AndroidPackageCreationStep::keystorePassword()
{
    m_keystorePasswd.clear();
    bool ok;
    QString text = QInputDialog::getText(0, tr("Keystore"),
                                         tr("Keystore password:"), QLineEdit::Password,
                                         QString(), &ok);
    if (ok && !text.isEmpty())
        m_keystorePasswd = text;
}

void AndroidPackageCreationStep::certificatePassword()
{
    m_certificatePasswdForRun.clear();
    bool ok;
    QString text = QInputDialog::getText(0, tr("Certificate"),
                                         tr("Certificate password (%1):").arg(m_certificateAlias), QLineEdit::Password,
                                         QString(), &ok);
    if (ok && !text.isEmpty())
        m_certificatePasswdForRun = text;
}

void AndroidPackageCreationStep::showInGraphicalShell()
{
    Core::FileUtils::showInGraphicalShell(Core::ICore::instance()->mainWindow(), m_apkPathSigned.toString());
}

void AndroidPackageCreationStep::raiseError(const QString &shortMsg,
                                            const QString &detailedMsg)
{
    emit addOutput(detailedMsg.isNull() ? shortMsg : detailedMsg, BuildStep::ErrorOutput);
    emit addTask(Task(Task::Error, shortMsg, Utils::FileName::fromString(QString()), -1,
                      TASK_CATEGORY_BUILDSYSTEM));
}

const Core::Id AndroidPackageCreationStep::CreatePackageId("Qt4ProjectManager.AndroidPackageCreationStep");

} // namespace Internal
} // namespace Qt4ProjectManager
