/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "maemopackagecreationstep.h"

#include "deploymentinfo.h"
#include "maemoconstants.h"
#include "maemoglobal.h"
#include "maemopackagecreationwidget.h"
#include "qt4maemodeployconfiguration.h"
#include "qt4maemotarget.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4target.h>
#include <utils/environment.h>
#include <utils/fileutils.h>

#include <QtCore/QDateTime>
#include <QtCore/QProcess>
#include <QtCore/QRegExp>
#include <QtCore/QStringBuilder>
#include <QtGui/QWidget>

namespace {
    const QLatin1String MagicFileName(".qtcreator");
}

using namespace ProjectExplorer::Constants;
using ProjectExplorer::BuildStepList;
using ProjectExplorer::BuildStepConfigWidget;
using ProjectExplorer::Task;
using namespace Qt4ProjectManager;

namespace RemoteLinux {
namespace Internal {

const QLatin1String AbstractMaemoPackageCreationStep::DefaultVersionNumber("0.0.1");

AbstractMaemoPackageCreationStep::AbstractMaemoPackageCreationStep(BuildStepList *bsl,
    const QString &id)
    : ProjectExplorer::BuildStep(bsl, id)
{
    ctor();
}

AbstractMaemoPackageCreationStep::AbstractMaemoPackageCreationStep(BuildStepList *bsl,
    AbstractMaemoPackageCreationStep *other) : BuildStep(bsl, other)
{
    ctor();
}

AbstractMaemoPackageCreationStep::~AbstractMaemoPackageCreationStep()
{
}

void AbstractMaemoPackageCreationStep::ctor()
{
    m_lastBuildConfig = qt4BuildConfiguration();
    connect(target(),
        SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
        this, SLOT(handleBuildConfigChanged()));
    handleBuildConfigChanged();
}

bool AbstractMaemoPackageCreationStep::init()
{
    return true;
}

void AbstractMaemoPackageCreationStep::run(QFutureInterface<bool> &fi)
{
    if (!packagingNeeded()) {
        emit addOutput(tr("Package up to date."), MessageOutput);
        fi.reportResult(true);
        return;
    }

    // TODO: Make the build process asynchronous; i.e. no waitFor()-functions etc.
    QProcess * const buildProc = new QProcess;
    connect(buildProc, SIGNAL(readyReadStandardOutput()), this,
        SLOT(handleBuildOutput()));
    connect(buildProc, SIGNAL(readyReadStandardError()), this,
        SLOT(handleBuildOutput()));
    emit addOutput(tr("Creating package file ..."), MessageOutput);
    const bool success = createPackage(buildProc, fi);
    disconnect(buildProc, 0, this, 0);
    buildProc->deleteLater();
    if (success) {
        emit addOutput(tr("Package created."), BuildStep::MessageOutput);
        deployConfig()->deploymentInfo()->setUnmodified();
    }
    fi.reportResult(success);
}

BuildStepConfigWidget *AbstractMaemoPackageCreationStep::createConfigWidget()
{
    return new MaemoPackageCreationWidget(this);
}

void AbstractMaemoPackageCreationStep::handleBuildOutput()
{
    QProcess * const buildProc = qobject_cast<QProcess *>(sender());
    if (!buildProc)
        return;
    QByteArray stdOut = buildProc->readAllStandardOutput();
    stdOut.replace('\0', QByteArray()); // Output contains NUL characters.
    if (!stdOut.isEmpty())
        emit addOutput(QString::fromLocal8Bit(stdOut), BuildStep::NormalOutput,
                BuildStep::DontAppendNewline);
    QByteArray errorOut = buildProc->readAllStandardError();
    errorOut.replace('\0', QByteArray());
    if (!errorOut.isEmpty()) {
        emit addOutput(QString::fromLocal8Bit(errorOut), BuildStep::ErrorOutput,
            BuildStep::DontAppendNewline);
    }
}

void AbstractMaemoPackageCreationStep::handleBuildConfigChanged()
{
    if (m_lastBuildConfig)
        disconnect(m_lastBuildConfig, 0, this, 0);
    m_lastBuildConfig = qt4BuildConfiguration();
    connect(m_lastBuildConfig, SIGNAL(qtVersionChanged()), this,
        SIGNAL(qtVersionChanged()));
    connect(m_lastBuildConfig, SIGNAL(buildDirectoryChanged()), this,
        SIGNAL(packageFilePathChanged()));
    emit qtVersionChanged();
    emit packageFilePathChanged();
}

const Qt4BuildConfiguration *AbstractMaemoPackageCreationStep::qt4BuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(buildConfiguration());
}

AbstractQt4MaemoTarget *AbstractMaemoPackageCreationStep::maemoTarget() const
{
    return qobject_cast<AbstractQt4MaemoTarget *>(buildConfiguration()->target());
}

AbstractDebBasedQt4MaemoTarget *AbstractMaemoPackageCreationStep::debBasedMaemoTarget() const
{
    return qobject_cast<AbstractDebBasedQt4MaemoTarget*>(buildConfiguration()->target());
}

AbstractRpmBasedQt4MaemoTarget *AbstractMaemoPackageCreationStep::rpmBasedMaemoTarget() const
{
    return qobject_cast<AbstractRpmBasedQt4MaemoTarget*>(buildConfiguration()->target());
}

Qt4MaemoDeployConfiguration *AbstractMaemoPackageCreationStep::deployConfig() const
{
    return qobject_cast<Qt4MaemoDeployConfiguration *>(parent()->parent());
}

QString AbstractMaemoPackageCreationStep::buildDirectory() const
{
    return qt4BuildConfiguration()->buildDirectory();
}

QString AbstractMaemoPackageCreationStep::projectName() const
{
    return qt4BuildConfiguration()->qt4Target()->qt4Project()
        ->rootProjectNode()->displayName().toLower();
}

bool AbstractMaemoPackageCreationStep::packagingNeeded() const
{
    const QSharedPointer<DeploymentInfo> &deploymentInfo
        = deployConfig()->deploymentInfo();
    QFileInfo packageInfo(packageFilePath());
    if (!packageInfo.exists() || deploymentInfo->isModified())
        return true;

    const int deployableCount = deploymentInfo->deployableCount();
    for (int i = 0; i < deployableCount; ++i) {
        if (Utils::FileUtils::isFileNewerThan(deploymentInfo->deployableAt(i).localFilePath,
                packageInfo.lastModified()))
            return true;
    }

    return isMetaDataNewerThan(packageInfo.lastModified());
}

QString AbstractMaemoPackageCreationStep::packageFilePath() const
{
    QString error;
    const QString &version = versionString(&error);
    if (version.isEmpty())
        return QString();
    QFileInfo fi(maemoTarget()->packageFileName());
    const QString baseName = replaceDots(fi.completeBaseName());
    return buildDirectory() + QLatin1Char('/') + baseName
        + QLatin1Char('.') + fi.suffix();
}

QString AbstractMaemoPackageCreationStep::versionString(QString *error) const
{
    return maemoTarget()->projectVersion(error);
}

bool AbstractMaemoPackageCreationStep::setVersionString(const QString &version,
    QString *error)
{
    const bool success = maemoTarget()->setProjectVersion(version, error);
    if (success)
        emit packageFilePathChanged();
    return success;
}

QString AbstractMaemoPackageCreationStep::nativePath(const QFile &file)
{
    return QDir::toNativeSeparators(QFileInfo(file).filePath());
}

void AbstractMaemoPackageCreationStep::raiseError(const QString &shortMsg,
                                          const QString &detailedMsg)
{
    emit addOutput(detailedMsg.isNull() ? shortMsg : detailedMsg, BuildStep::ErrorOutput);
    emit addTask(Task(Task::Error, shortMsg, QString(), -1,
                      TASK_CATEGORY_BUILDSYSTEM));
}

bool AbstractMaemoPackageCreationStep::callPackagingCommand(QProcess *proc,
    const QStringList &arguments)
{
    preparePackagingProcess(proc, qt4BuildConfiguration(), buildDirectory());
    const QtSupport::BaseQtVersion * const qtVersion = qt4BuildConfiguration()->qtVersion();
    if (!qtVersion) {
        raiseError(tr("Packaging failed."), tr("Packaging error: No Qt version."));
        return false;
    }
    const QString madCommand = MaemoGlobal::madCommand(qtVersion->qmakeCommand());
    const QString cmdLine = madCommand + QLatin1Char(' ')
        + arguments.join(QLatin1String(" "));
    emit addOutput(tr("Package Creation: Running command '%1'.").arg(cmdLine),
        BuildStep::MessageOutput);
    MaemoGlobal::callMad(*proc, arguments, qtVersion->qmakeCommand(), true);
    if (!proc->waitForStarted()) {
        raiseError(tr("Packaging failed."),
            tr("Packaging error: Could not start command '%1'. Reason: %2")
            .arg(cmdLine, proc->errorString()));
        return false;
    }
    proc->waitForFinished(-1);
    if (proc->error() != QProcess::UnknownError || proc->exitCode() != 0) {
        QString mainMessage = tr("Packaging Error: Command '%1' failed.")
            .arg(cmdLine);
        if (proc->error() != QProcess::UnknownError)
            mainMessage += tr(" Reason: %1").arg(proc->errorString());
        else
            mainMessage += tr("Exit code: %1").arg(proc->exitCode());
        raiseError(mainMessage);
        return false;
    }
    return true;
}


void AbstractMaemoPackageCreationStep::preparePackagingProcess(QProcess *proc,
    const Qt4BuildConfiguration *bc, const QString &workingDir)
{
    Utils::Environment env = bc->environment();
    if (bc->qmakeBuildConfiguration() & QtSupport::BaseQtVersion::DebugBuild) {
        env.appendOrSet(QLatin1String("DEB_BUILD_OPTIONS"),
            QLatin1String("nostrip"), QLatin1String(" "));
    }
    proc->setEnvironment(env.toStringList());
    proc->setWorkingDirectory(workingDir);
}

QString AbstractMaemoPackageCreationStep::replaceDots(const QString &name)
{
    QString adaptedName = name;
    return adaptedName.replace(QLatin1Char('.'), QLatin1Char('_'));
}


MaemoDebianPackageCreationStep::MaemoDebianPackageCreationStep(BuildStepList *bsl)
    : AbstractMaemoPackageCreationStep(bsl, CreatePackageId)
{
    ctor();
}

const QString MaemoDebianPackageCreationStep::CreatePackageId
    = QLatin1String("MaemoDebianPackageCreationStep");

MaemoDebianPackageCreationStep::MaemoDebianPackageCreationStep(BuildStepList *buildConfig,
    MaemoDebianPackageCreationStep *other)
        : AbstractMaemoPackageCreationStep(buildConfig, other)
{
    ctor();
}

void MaemoDebianPackageCreationStep::ctor()
{
    setDefaultDisplayName(tr("Create Debian Package"));
}

bool MaemoDebianPackageCreationStep::createPackage(QProcess *buildProc,
    const QFutureInterface<bool> &fi)
{
    Q_UNUSED(fi);
    checkProjectName();
    const QString projectDir
        = buildConfiguration()->target()->project()->projectDirectory();
    const bool inSourceBuild
        = QFileInfo(buildDirectory()) == QFileInfo(projectDir);
    if (!copyDebianFiles(inSourceBuild))
        return false;
    const QStringList args = QStringList() << QLatin1String("dpkg-buildpackage")
        << QLatin1String("-nc") << QLatin1String("-uc") << QLatin1String("-us");
    if (!callPackagingCommand(buildProc, args))
        return false;

    QFile::remove(packageFilePath());

    // Workaround for non-working dh_builddeb --destdir=.
    if (!QDir(buildDirectory()).isRoot()) {
        const AbstractQt4MaemoTarget * const target = maemoTarget();
        QString error;
        const QString pkgFileName = target->packageFileName();
        if (!error.isEmpty())
            raiseError(tr("Packaging failed."), "Failed to get package name.");
        const QString changesSourceFileName = QFileInfo(pkgFileName).completeBaseName()
                + QLatin1String(".changes");
        const QString changesTargetFileName = replaceDots(QFileInfo(pkgFileName).completeBaseName())
                + QLatin1String(".changes");
        const QString packageSourceDir = buildDirectory() + QLatin1String("/../");
        const QString packageSourceFilePath
                = packageSourceDir + pkgFileName;
        const QString changesSourceFilePath
                = packageSourceDir + changesSourceFileName;
        const QString changesTargetFilePath
                = buildDirectory() + QLatin1Char('/') + changesTargetFileName;
        QFile::remove(changesTargetFilePath);
        if (!QFile::rename(packageSourceFilePath, packageFilePath())
                || !QFile::rename(changesSourceFilePath, changesTargetFilePath)) {
            raiseError(tr("Packaging failed."),
                       tr("Could not move package files from %1 to %2.")
                       .arg(packageSourceDir, buildDirectory()));
            return false;
        }
    }

    if (inSourceBuild) {
        buildProc->start(packagingCommand(qt4BuildConfiguration(),
            QLatin1String("dh_clean")));
        buildProc->waitForFinished();
        buildProc->terminate();
    }
    return true;
}

bool MaemoDebianPackageCreationStep::isMetaDataNewerThan(const QDateTime &packageDate) const
{
    const QString debianPath = debBasedMaemoTarget()->debianDirPath();
    if (packageDate <= QFileInfo(debianPath).lastModified())
        return true;
    const QStringList debianFiles = debBasedMaemoTarget()->debianFiles();
    foreach (const QString &debianFile, debianFiles) {
        const QString absFilePath
            = debianPath + QLatin1Char('/') + debianFile;
        if (packageDate <= QFileInfo(absFilePath).lastModified())
            return true;
    }
    return false;
}

void MaemoDebianPackageCreationStep::checkProjectName()
{
    const QRegExp legalName(QLatin1String("[0-9-+a-z\\.]+"));
    if (!legalName.exactMatch(buildConfiguration()->target()->project()->displayName())) {
        emit addTask(Task(Task::Warning,
            tr("Your project name contains characters not allowed in "
               "Debian packages.\nThey must only use lower-case letters, "
               "numbers, '-', '+' and '.'.\n""We will try to work around that, "
               "but you may experience problems."),
            QString(), -1, TASK_CATEGORY_BUILDSYSTEM));
    }
}

bool MaemoDebianPackageCreationStep::copyDebianFiles(bool inSourceBuild)
{
    const QString debianDirPath = buildDirectory() + QLatin1String("/debian");
    const QString magicFilePath
        = debianDirPath + QLatin1Char('/') + MagicFileName;
    if (inSourceBuild && QFileInfo(debianDirPath).isDir()
        && !QFileInfo(magicFilePath).exists()) {
        raiseError(tr("Packaging failed: Foreign debian directory detected."),
             tr("You are not using a shadow build and there is a debian "
                "directory in your project root ('%1'). Qt Creator will not "
                "overwrite that directory. Please remove it or use the "
                "shadow build feature.")
                   .arg(QDir::toNativeSeparators(debianDirPath)));
        return false;
    }
    QString error;
    if (!Utils::FileUtils::removeRecursively(debianDirPath, &error)) {
        raiseError(tr("Packaging failed."),
            tr("Could not remove directory '%1': %2").arg(debianDirPath, error));
        return false;
    }
    QDir buildDir(buildDirectory());
    if (!buildDir.mkdir("debian")) {
        raiseError(tr("Could not create Debian directory '%1'.")
                   .arg(debianDirPath));
        return false;
    }
    const QString templatesDirPath = debBasedMaemoTarget()->debianDirPath();
    QDir templatesDir(templatesDirPath);
    const QStringList &files = templatesDir.entryList(QDir::Files);
    foreach (const QString &fileName, files) {
        const QString srcFile = templatesDirPath + QLatin1Char('/') + fileName;
        QString newFileName = fileName;
        if (newFileName == Qt4HarmattanTarget::aegisManifestFileName()) {
            // If the user has touched the Aegis manifest file, we copy it for use
            // by MADDE. Otherwise the required capabilities will be auto-detected.
            if (QFileInfo(srcFile).size() == 0)
                continue;
            newFileName = maemoTarget()->packageName() + QLatin1String(".aegis");
        }
        const QString destFile = debianDirPath + QLatin1Char('/') + newFileName;
        if (fileName == QLatin1String("rules")) {
            if (!adaptRulesFile(srcFile, destFile))
                return false;
        } else if (!QFile::copy(srcFile, destFile)) {
            raiseError(tr("Could not copy file '%1' to '%2'")
                       .arg(QDir::toNativeSeparators(srcFile),
                            QDir::toNativeSeparators(destFile)));
            return false;
        }
    }

    QFile magicFile(magicFilePath);
    if (!magicFile.open(QIODevice::WriteOnly)) {
        raiseError(tr("Error: Could not create file '%1'.")
            .arg(QDir::toNativeSeparators(magicFilePath)));
        return false;
    }

    return true;
}

QString MaemoDebianPackageCreationStep::packagingCommand(const Qt4BuildConfiguration *bc,
    const QString &commandName)
{
    QString perl;
    QtSupport::BaseQtVersion *v = bc->qtVersion();
    const QString maddeRoot = MaemoGlobal::maddeRoot(v->qmakeCommand());
#ifdef Q_OS_WIN
    perl = maddeRoot + QLatin1String("/bin/perl.exe ");
#endif
    return perl + maddeRoot % QLatin1String("/madbin/") % commandName;
}

void MaemoDebianPackageCreationStep::ensureShlibdeps(QByteArray &rulesContent)
{
    QString contentAsString = QString::fromLocal8Bit(rulesContent);
    const QString whiteSpace(QLatin1String("[ \\t]*"));
    const QString pattern = QLatin1String("\\n") + whiteSpace
        + QLatin1Char('#') + whiteSpace + QLatin1String("dh_shlibdeps")
        + QLatin1String("([^\\n]*)\\n");
    contentAsString.replace(QRegExp(pattern),
        QLatin1String("\n\tdh_shlibdeps\\1\n"));
    rulesContent = contentAsString.toLocal8Bit();
}

bool MaemoDebianPackageCreationStep::adaptRulesFile(
    const QString &templatePath, const QString &rulesFilePath)
{
    Utils::FileReader reader;
    if (!reader.fetch(templatePath)) {
        raiseError(reader.errorString());
        return false;
    }
    QByteArray content = reader.data();
    const Qt4BuildConfiguration * const bc = qt4BuildConfiguration();

    // Always check for dependencies in release builds.
    if (!(bc->qmakeBuildConfiguration() & QtSupport::BaseQtVersion::DebugBuild))
        ensureShlibdeps(content);

    Utils::FileSaver saver(rulesFilePath);
    saver.write(content);
    if (!saver.finalize()) {
        raiseError(saver.errorString());
        return false;
    }
    QFile rulesFile(rulesFilePath);
    rulesFile.setPermissions(rulesFile.permissions() | QFile::ExeUser);
    return true;
}


MaemoRpmPackageCreationStep::MaemoRpmPackageCreationStep(BuildStepList *bsl)
    : AbstractMaemoPackageCreationStep(bsl, CreatePackageId)
{
    ctor();
}

MaemoRpmPackageCreationStep::MaemoRpmPackageCreationStep(BuildStepList *buildConfig,
    MaemoRpmPackageCreationStep *other)
        : AbstractMaemoPackageCreationStep(buildConfig, other)
{
    ctor();
}

void MaemoRpmPackageCreationStep::ctor()
{
    setDefaultDisplayName(tr("Create RPM Package"));
}

bool MaemoRpmPackageCreationStep::createPackage(QProcess *buildProc,
    const QFutureInterface<bool> &fi)
{
    Q_UNUSED(fi);
    const QStringList args = QStringList() << QLatin1String("rrpmbuild")
        << QLatin1String("-bb") << rpmBasedMaemoTarget()->specFilePath();
    if (!callPackagingCommand(buildProc, args))
        return false;
    QFile::remove(packageFilePath());
    const QString packageSourceFilePath = rpmBuildDir(qt4BuildConfiguration())
        + QLatin1Char('/') + rpmBasedMaemoTarget()->packageFileName();
    if (!QFile::rename(packageSourceFilePath, packageFilePath())) {
        raiseError(tr("Packaging failed."),
            tr("Could not move package file from %1 to %2.")
                .arg(packageSourceFilePath, packageFilePath()));
        return false;
    }

    return true;
}

bool MaemoRpmPackageCreationStep::isMetaDataNewerThan(const QDateTime &packageDate) const
{
    const QDateTime specFileChangeDate
        = QFileInfo(rpmBasedMaemoTarget()->specFilePath()).lastModified();
    return packageDate <= specFileChangeDate;
}

QString MaemoRpmPackageCreationStep::rpmBuildDir(const Qt4BuildConfiguration *bc)
{
    return bc->buildDirectory() + QLatin1String("/rrpmbuild");
}

const QString MaemoRpmPackageCreationStep::CreatePackageId
    = QLatin1String("MaemoRpmPackageCreationStep");



class CreateTarStepWidget : public BuildStepConfigWidget
{
    Q_OBJECT
public:
    CreateTarStepWidget(MaemoTarPackageCreationStep *step) : m_step(step)
    {
        connect(m_step, SIGNAL(packageFilePathChanged()),
            SIGNAL(updateSummary()));
    }

    virtual QString summaryText() const
    {
        return QLatin1String("<b>") + tr("Create tarball:")
            + QLatin1String("</b> ") + m_step->packageFilePath();
    }

    virtual QString displayName() const { return QString(); }

private:
    const MaemoTarPackageCreationStep * const m_step;
};

namespace {
const int TarBlockSize = 512;
struct TarFileHeader {
    char fileName[100];
    char fileMode[8];
    char uid[8];
    char gid[8];
    char length[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char fileNamePrefix[155];
    char padding[12];
};
} // Anonymous namespace.

MaemoTarPackageCreationStep::MaemoTarPackageCreationStep(BuildStepList *bsl)
    : AbstractMaemoPackageCreationStep(bsl, CreatePackageId)
{
    ctor();
}

MaemoTarPackageCreationStep::MaemoTarPackageCreationStep(BuildStepList *buildConfig,
    MaemoTarPackageCreationStep *other)
        : AbstractMaemoPackageCreationStep(buildConfig, other)
{
    ctor();
}

void MaemoTarPackageCreationStep::ctor()
{
    setDefaultDisplayName(tr("Create tarball"));
}

bool MaemoTarPackageCreationStep::createPackage(QProcess *buildProc,
    const QFutureInterface<bool> &fi)
{
    Q_UNUSED(buildProc);

    // TODO: Optimization: Only package changed files (needs refactoring in upper level; worth the effort?)
    QFile tarFile(packageFilePath());
    if (!tarFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        raiseError(tr("Error: tar file %1 cannot be opened (%2).")
            .arg(QDir::toNativeSeparators(packageFilePath()),
                 tarFile.errorString()));
        return false;
    }
    const QSharedPointer<DeploymentInfo> deploymentInfo = deployConfig()->deploymentInfo();
    for (int i = 0; i < deploymentInfo->deployableCount(); ++i) {
        const DeployableFile &d = deploymentInfo->deployableAt(i);
        QFileInfo fileInfo(d.localFilePath);
        if (!appendFile(tarFile, fileInfo, d.remoteDir + QLatin1Char('/')
            + fileInfo.fileName(), fi)) {
            return false;
        }
    }

    const QByteArray eofIndicator(2*sizeof(TarFileHeader), 0);
    if (tarFile.write(eofIndicator) != eofIndicator.length()) {
        raiseError(tr("Error writing tar file '%1': %2.")
            .arg(QDir::toNativeSeparators(tarFile.fileName()),
                 tarFile.errorString()));
        return false;
    }

    return true;
}

bool MaemoTarPackageCreationStep::appendFile(QFile &tarFile,
    const QFileInfo &fileInfo, const QString &remoteFilePath,
    const QFutureInterface<bool> &fi)
{
    if (!writeHeader(tarFile, fileInfo, remoteFilePath))
        return false;
    if (fileInfo.isDir()) {
        QDir dir(fileInfo.absoluteFilePath());
        foreach (const QString &fileName,
                 dir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot)) {
            const QString thisLocalFilePath
                = dir.path() + QLatin1Char('/') + fileName;
            const QString thisRemoteFilePath  = remoteFilePath
                + QLatin1Char('/') + fileName;
            if (!appendFile(tarFile, QFileInfo(thisLocalFilePath),
                thisRemoteFilePath, fi)) {
                return false;
            }
        }
    } else {
        const QString nativePath
            = QDir::toNativeSeparators(fileInfo.filePath());
        QFile file(fileInfo.filePath());
        if (!file.open(QIODevice::ReadOnly)) {
            raiseError(tr("Error reading file '%1': %2.")
                .arg(nativePath, file.errorString()));
            return false;
        }

        const int chunkSize = 1024*1024;

        // TODO: Wasteful. Work with fixed-size buffer.
        while (!file.atEnd() && !file.error() != QFile::NoError
               && !tarFile.error() != QFile::NoError) {
            const QByteArray data = file.read(chunkSize);
            tarFile.write(data);
            if (fi.isCanceled())
                return false;
        }
        if (file.error() != QFile::NoError) {
            raiseError(tr("Error reading file '%1': %2.")
                .arg(nativePath, file.errorString()));
            return false;
        }

        const int blockModulo = file.size() % TarBlockSize;
        if (blockModulo != 0) {
            tarFile.write(QByteArray(TarBlockSize - blockModulo, 0));
        }

        if (tarFile.error() != QFile::NoError) {
            raiseError(tr("Error writing tar file '%1': %2.")
                .arg(QDir::toNativeSeparators(tarFile.fileName()),
                     tarFile.errorString()));
            return false;
        }
    }
    return true;
}

bool MaemoTarPackageCreationStep::writeHeader(QFile &tarFile,
    const QFileInfo &fileInfo, const QString &remoteFilePath)
{
    TarFileHeader header;
    qMemSet(&header, '\0', sizeof header);
    const QByteArray &filePath = remoteFilePath.toUtf8();
    const int maxFilePathLength = sizeof header.fileNamePrefix
        + sizeof header.fileName;
    if (filePath.count() > maxFilePathLength) {
        raiseError(tr("Cannot add file '%1' to tar-archive: path too long.")
            .arg(QDir::toNativeSeparators(filePath)));
        return false;
    }

    const int fileNameBytesToWrite
        = qMin<int>(filePath.length(), sizeof header.fileName);
    const int fileNameOffset = filePath.length() - fileNameBytesToWrite;
    qMemCopy(&header.fileName, filePath.data() + fileNameOffset,
        fileNameBytesToWrite);
    if (fileNameOffset > 0)
        qMemCopy(&header.fileNamePrefix, filePath.data(), fileNameOffset);
    int permissions = (0400 * fileInfo.permission(QFile::ReadOwner))
        | (0200 * fileInfo.permission(QFile::WriteOwner))
        | (0100 * fileInfo.permission(QFile::ExeOwner))
        | (040 * fileInfo.permission(QFile::ReadGroup))
        | (020 * fileInfo.permission(QFile::WriteGroup))
        | (010 * fileInfo.permission(QFile::ExeGroup))
        | (04 * fileInfo.permission(QFile::ReadOther))
        | (02 * fileInfo.permission(QFile::WriteOther))
        | (01 * fileInfo.permission(QFile::ExeOther));
    const QByteArray permissionString = QString("%1").arg(permissions,
        sizeof header.fileMode - 1, 8, QLatin1Char('0')).toAscii();
    qMemCopy(&header.fileMode, permissionString.data(),
        permissionString.length());
    const QByteArray uidString = QString("%1").arg(fileInfo.ownerId(),
        sizeof header.uid - 1, 8, QLatin1Char('0')).toAscii();
    qMemCopy(&header.uid, uidString.data(), uidString.length());
    const QByteArray gidString = QString("%1").arg(fileInfo.groupId(),
        sizeof header.gid - 1, 8, QLatin1Char('0')).toAscii();
    qMemCopy(&header.gid, gidString.data(), gidString.length());
    const QByteArray sizeString = QString("%1").arg(fileInfo.size(),
        sizeof header.length - 1, 8, QLatin1Char('0')).toAscii();
    qMemCopy(&header.length, sizeString.data(), sizeString.length());
    const QByteArray mtimeString = QString("%1").arg(fileInfo.lastModified().toTime_t(),
        sizeof header.mtime - 1, 8, QLatin1Char('0')).toAscii();
    qMemCopy(&header.mtime, mtimeString.data(), mtimeString.length());
    if (fileInfo.isDir())
        header.typeflag = '5';
    qMemCopy(&header.magic, "ustar", sizeof "ustar");
    qMemCopy(&header.version, "00", 2);
    const QByteArray &owner = fileInfo.owner().toUtf8();
    qMemCopy(&header.uname, owner.data(),
        qMin<int>(owner.length(), sizeof header.uname - 1));
    const QByteArray &group = fileInfo.group().toUtf8();
    qMemCopy(&header.gname, group.data(),
        qMin<int>(group.length(), sizeof header.gname - 1));
    qMemSet(&header.chksum, ' ', sizeof header.chksum);
    quint64 checksum = 0;
    for (size_t i = 0; i < sizeof header; ++i)
        checksum += reinterpret_cast<char *>(&header)[i];
    const QByteArray checksumString = QString("%1").arg(checksum,
        sizeof header.chksum - 1, 8, QLatin1Char('0')).toAscii();
    qMemCopy(&header.chksum, checksumString.data(), checksumString.length());
    header.chksum[sizeof header.chksum-1] = 0;
    if (!tarFile.write(reinterpret_cast<char *>(&header), sizeof header)) {
        raiseError(tr("Error writing tar file '%1': %2")
           .arg(QDir::toNativeSeparators(packageFilePath()),
                tarFile.errorString()));
        return false;
    }
    return true;
}

bool MaemoTarPackageCreationStep::isMetaDataNewerThan(const QDateTime &packageDate) const
{
    Q_UNUSED(packageDate);
    return false;
}

QString MaemoTarPackageCreationStep::packageFilePath() const
{
    return buildDirectory() + QLatin1Char('/') + projectName()
        + QLatin1String(".tar");
}

BuildStepConfigWidget *MaemoTarPackageCreationStep::createConfigWidget()
{
    return new CreateTarStepWidget(this);
}

const QString MaemoTarPackageCreationStep::CreatePackageId
    = QLatin1String("MaemoTarPackageCreationStep");

} // namespace Internal
} // namespace RemoteLinux

#include "maemopackagecreationstep.moc"
