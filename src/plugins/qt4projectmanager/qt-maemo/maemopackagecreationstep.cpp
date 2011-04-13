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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "maemopackagecreationstep.h"

#include "maemoconstants.h"
#include "maemodeployables.h"
#include "maemodeploystep.h"
#include "maemoglobal.h"
#include "maemopackagecreationwidget.h"
#include "qt4maemotarget.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qt4buildconfiguration.h>
#include <qt4project.h>
#include <qt4target.h>
#include <utils/environment.h>

#include <QtCore/QDateTime>
#include <QtCore/QProcess>
#include <QtCore/QRegExp>
#include <QtCore/QStringBuilder>
#include <QtGui/QWidget>

namespace {
    const QLatin1String PackagingEnabledKey("Packaging Enabled");
    const QLatin1String MagicFileName(".qtcreator");
}

using namespace ProjectExplorer::Constants;
using ProjectExplorer::BuildStepList;
using ProjectExplorer::BuildStepConfigWidget;
using ProjectExplorer::Task;

namespace Qt4ProjectManager {
namespace Internal {

const QLatin1String MaemoPackageCreationStep::DefaultVersionNumber("0.0.1");

MaemoPackageCreationStep::MaemoPackageCreationStep(BuildStepList *bsl)
    : ProjectExplorer::BuildStep(bsl, CreatePackageId),
      m_packagingEnabled(true)
{
    ctor();
}

MaemoPackageCreationStep::MaemoPackageCreationStep(BuildStepList *bsl,
    MaemoPackageCreationStep *other)
    : BuildStep(bsl, other),
      m_packagingEnabled(other->m_packagingEnabled)
{
    ctor();
}

MaemoPackageCreationStep::~MaemoPackageCreationStep()
{
}

void MaemoPackageCreationStep::ctor()
{
    setDefaultDisplayName(tr("Packaging for Maemo"));

    m_lastBuildConfig = qt4BuildConfiguration();
    connect(target(),
        SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
        this, SLOT(handleBuildConfigChanged()));
    handleBuildConfigChanged();
}

bool MaemoPackageCreationStep::init()
{
    return true;
}

QVariantMap MaemoPackageCreationStep::toMap() const
{
    QVariantMap map(ProjectExplorer::BuildStep::toMap());
    map.insert(PackagingEnabledKey, m_packagingEnabled);
    return map;
}

bool MaemoPackageCreationStep::fromMap(const QVariantMap &map)
{
    m_packagingEnabled = map.value(PackagingEnabledKey, true).toBool();
    return ProjectExplorer::BuildStep::fromMap(map);
}

void MaemoPackageCreationStep::run(QFutureInterface<bool> &fi)
{
    bool success;
    if (m_packagingEnabled) {
        // TODO: Make the build process asynchronous; i.e. no waitFor()-functions etc.
        QProcess * const buildProc = new QProcess;
        connect(buildProc, SIGNAL(readyReadStandardOutput()), this,
            SLOT(handleBuildOutput()));
        connect(buildProc, SIGNAL(readyReadStandardError()), this,
            SLOT(handleBuildOutput()));
        success = createPackage(buildProc);
        disconnect(buildProc, 0, this, 0);
        buildProc->deleteLater();
    } else  {
        success = true;
    }
    fi.reportResult(success);
}

BuildStepConfigWidget *MaemoPackageCreationStep::createConfigWidget()
{
    return new MaemoPackageCreationWidget(this);
}

bool MaemoPackageCreationStep::createPackage(QProcess *buildProc)
{
    if (!packagingNeeded()) {
        emit addOutput(tr("Package up to date."), MessageOutput);
        return true;
    }

    emit addOutput(tr("Creating package file ..."), MessageOutput);
    checkProjectName();
    preparePackagingProcess(buildProc, qt4BuildConfiguration(),
        buildDirectory());

    const QString projectDir
        = buildConfiguration()->target()->project()->projectDirectory();
    const bool inSourceBuild
        = QFileInfo(buildDirectory()) == QFileInfo(projectDir);
    if (debBasedMaemoTarget() && !copyDebianFiles(inSourceBuild))
        return false;

    const QtVersion * const qtVersion = qt4BuildConfiguration()->qtVersion();
    const QString madCommand = MaemoGlobal::madCommand(qtVersion);
    QStringList args;
    if (debBasedMaemoTarget()) {
        args << QLatin1String("dpkg-buildpackage") << QLatin1String("-nc")
            << QLatin1String("-uc") << QLatin1String("-us");
    } else {
        args << QLatin1String("rrpmbuild") << QLatin1String("-bb")
            << rpmBasedMaemoTarget()->specFilePath();
    }
    const QString cmdLine = madCommand + QLatin1Char(' ')
        + args.join(QLatin1String(" "));
    emit addOutput(tr("Package Creation: Running command '%1'.").arg(cmdLine),
        BuildStep::MessageOutput);
    MaemoGlobal::callMad(*buildProc, args, qtVersion, true);
    if (!buildProc->waitForStarted()) {
        raiseError(tr("Packaging failed."),
            tr("Packaging error: Could not start command '%1'. Reason: %2")
            .arg(cmdLine, buildProc->errorString()));
        return false;
    }
    buildProc->waitForFinished(-1);
    if (buildProc->error() != QProcess::UnknownError
            || buildProc->exitCode() != 0) {
        QString mainMessage = tr("Packaging Error: Command '%1' failed.")
            .arg(cmdLine);
        if (buildProc->error() != QProcess::UnknownError)
            mainMessage += tr(" Reason: %1").arg(buildProc->errorString());
        else
            mainMessage += tr("Exit code: %1").arg(buildProc->exitCode());
        raiseError(mainMessage);
        return false;
    }

    QFile::remove(packageFilePath());
    if (debBasedMaemoTarget()) {
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
    } else {
        const QString packageSourceFilePath = rpmBuildDir(qt4BuildConfiguration())
            + QLatin1Char('/') + rpmBasedMaemoTarget()->packageFileName();
        if (!QFile::rename(packageSourceFilePath, packageFilePath())) {
            raiseError(tr("Packaging failed."),
                tr("Could not move package file from %1 to %2.")
                    .arg(packageSourceFilePath, packageFilePath()));
            return false;
        }
    }

    emit addOutput(tr("Package created."), BuildStep::MessageOutput);
    deployStep()->deployables()->setUnmodified();
    if (inSourceBuild && debBasedMaemoTarget()) {
        buildProc->start(packagingCommand(qt4BuildConfiguration(),
            QLatin1String("dh_clean")));
        buildProc->waitForFinished();
        buildProc->terminate();
    }
    return true;
}

bool MaemoPackageCreationStep::copyDebianFiles(bool inSourceBuild)
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
    if (!MaemoGlobal::removeRecursively(debianDirPath, error)) {
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
        const QString srcFile
                = templatesDirPath + QLatin1Char('/') + fileName;
        const QString destFile
                = debianDirPath + QLatin1Char('/') + fileName;
        if (!QFile::copy(srcFile, destFile)) {
            raiseError(tr("Could not copy file '%1' to '%2'")
                       .arg(QDir::toNativeSeparators(srcFile),
                            QDir::toNativeSeparators(destFile)));
            return false;
        }

        if (fileName == QLatin1String("rules"))
            adaptRulesFile(destFile);
    }

    QFile magicFile(magicFilePath);
    if (!magicFile.open(QIODevice::WriteOnly)) {
        raiseError(tr("Error: Could not create file '%1'.")
            .arg(QDir::toNativeSeparators(magicFilePath)));
        return false;
    }

    return true;
}

void MaemoPackageCreationStep::handleBuildOutput()
{
    QProcess * const buildProc = qobject_cast<QProcess *>(sender());
    if (!buildProc)
        return;
    QByteArray stdOut = buildProc->readAllStandardOutput();
    stdOut.replace('\0', QByteArray()); // Output contains NUL characters.
    if (!stdOut.isEmpty())
        emit addOutput(QString::fromLocal8Bit(stdOut), BuildStep::NormalOutput);
    QByteArray errorOut = buildProc->readAllStandardError();
    errorOut.replace('\0', QByteArray());
    if (!errorOut.isEmpty()) {
        emit addOutput(QString::fromLocal8Bit(errorOut), BuildStep::ErrorOutput);
    }
}

void MaemoPackageCreationStep::handleBuildConfigChanged()
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

const Qt4BuildConfiguration *MaemoPackageCreationStep::qt4BuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(buildConfiguration());
}

AbstractQt4MaemoTarget *MaemoPackageCreationStep::maemoTarget() const
{
    return qobject_cast<AbstractQt4MaemoTarget *>(buildConfiguration()->target());
}

AbstractDebBasedQt4MaemoTarget *MaemoPackageCreationStep::debBasedMaemoTarget() const
{
    return qobject_cast<AbstractDebBasedQt4MaemoTarget*>(buildConfiguration()->target());
}

AbstractRpmBasedQt4MaemoTarget *MaemoPackageCreationStep::rpmBasedMaemoTarget() const
{
    return qobject_cast<AbstractRpmBasedQt4MaemoTarget*>(buildConfiguration()->target());
}

QString MaemoPackageCreationStep::buildDirectory() const
{
    return qt4BuildConfiguration()->buildDirectory();
}

QString MaemoPackageCreationStep::projectName() const
{
    return qt4BuildConfiguration()->qt4Target()->qt4Project()
        ->rootProjectNode()->displayName().toLower();
}

MaemoDeployStep *MaemoPackageCreationStep::deployStep() const
{
    MaemoDeployStep * const deployStep
        = MaemoGlobal::buildStep<MaemoDeployStep>(target()->activeDeployConfiguration());
    Q_ASSERT(deployStep &&
        "Fatal error: Maemo build configuration without deploy step.");
    return deployStep;
}

bool MaemoPackageCreationStep::packagingNeeded() const
{
    const QSharedPointer<MaemoDeployables> &deployables
        = deployStep()->deployables();
    QFileInfo packageInfo(packageFilePath());
    if (!packageInfo.exists() || deployables->isModified())
        return true;

    const int deployableCount = deployables->deployableCount();
    for (int i = 0; i < deployableCount; ++i) {
        if (isFileNewerThan(deployables->deployableAt(i).localFilePath,
                packageInfo.lastModified()))
            return true;
    }

    if (debBasedMaemoTarget()) {
        const QString debianPath = debBasedMaemoTarget()->debianDirPath();
        if (packageInfo.lastModified() <= QFileInfo(debianPath).lastModified())
            return true;
        const QStringList debianFiles = debBasedMaemoTarget()->debianFiles();
        foreach (const QString &debianFile, debianFiles) {
            const QString absFilePath
                = debianPath + QLatin1Char('/') + debianFile;
            if (packageInfo.lastModified() <= QFileInfo(absFilePath).lastModified())
                return true;
        }
    } else {
        const QDateTime specFileChangeDate
            = QFileInfo(rpmBasedMaemoTarget()->specFilePath()).lastModified();
        if (packageInfo.lastModified() <= specFileChangeDate)
            return true;
    }

    return false;
}

bool MaemoPackageCreationStep::isFileNewerThan(const QString &filePath,
    const QDateTime &timeStamp) const
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || fileInfo.lastModified() >= timeStamp)
        return true;
    if (fileInfo.isDir()) {
        const QStringList dirContents = QDir(filePath)
            .entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        foreach (const QString &curFileName, dirContents) {
            const QString curFilePath
                = filePath + QLatin1Char('/') + curFileName;
            if (isFileNewerThan(curFilePath, timeStamp))
                return true;
        }
    }
    return false;
}

QString MaemoPackageCreationStep::packageFilePath() const
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

bool MaemoPackageCreationStep::isPackagingEnabled() const
{
    return m_packagingEnabled || !maemoTarget()->allowsPackagingDisabling();
}

QString MaemoPackageCreationStep::versionString(QString *error) const
{
    return maemoTarget()->projectVersion(error);
}

bool MaemoPackageCreationStep::setVersionString(const QString &version,
    QString *error)
{
    const bool success = maemoTarget()->setProjectVersion(version, error);
    if (success)
        emit packageFilePathChanged();
    return success;
}

QString MaemoPackageCreationStep::nativePath(const QFile &file)
{
    return QDir::toNativeSeparators(QFileInfo(file).filePath());
}

void MaemoPackageCreationStep::raiseError(const QString &shortMsg,
                                          const QString &detailedMsg)
{
    emit addOutput(detailedMsg.isNull() ? shortMsg : detailedMsg, BuildStep::ErrorOutput);
    emit addTask(Task(Task::Error, shortMsg, QString(), -1,
                      TASK_CATEGORY_BUILDSYSTEM));
}

void MaemoPackageCreationStep::preparePackagingProcess(QProcess *proc,
    const Qt4BuildConfiguration *bc, const QString &workingDir)
{
    Utils::Environment env = bc->environment();
    if (bc->qmakeBuildConfiguration() & QtVersion::DebugBuild) {
        env.appendOrSet(QLatin1String("DEB_BUILD_OPTIONS"),
            QLatin1String("nostrip"), QLatin1String(" "));
    }
    proc->setEnvironment(env.toStringList());
    proc->setWorkingDirectory(workingDir);
}

QString MaemoPackageCreationStep::packagingCommand(const Qt4BuildConfiguration *bc,
    const QString &commandName)
{
    QString perl;
    const QString maddeRoot = MaemoGlobal::maddeRoot(bc->qtVersion());
#ifdef Q_OS_WIN
    perl = maddeRoot + QLatin1String("/bin/perl.exe ");
#endif
    return perl + maddeRoot % QLatin1String("/madbin/") % commandName;
}

void MaemoPackageCreationStep::checkProjectName()
{
    if (debBasedMaemoTarget()) {
        const QRegExp legalName(QLatin1String("[0-9-+a-z\\.]+"));
        if (!legalName.exactMatch(buildConfiguration()->target()->project()->displayName())) {
            emit addTask(Task(Task::Warning,
                tr("Your project name contains characters not allowed in Debian packages.\n"
                   "They must only use lower-case letters, numbers, '-', '+' and '.'.\n"
                   "We will try to work around that, but you may experience problems."),
                QString(), -1, TASK_CATEGORY_BUILDSYSTEM));
        }
    }
}

void MaemoPackageCreationStep::ensureShlibdeps(QByteArray &rulesContent)
{
    QString contentAsString = QString::fromLocal8Bit(rulesContent);
    const QString whiteSpace(QLatin1String("[ \\t]*"));
    const QString pattern = QLatin1String("\\n") + whiteSpace
        + QLatin1Char('#') + whiteSpace + QLatin1String("dh_shlibdeps")
        + QLatin1String("[^\\n]*\\n");
    contentAsString.replace(QRegExp(pattern),
        QLatin1String("\n\tdh_shlibdeps\n"));
    rulesContent = contentAsString.toLocal8Bit();
}

void MaemoPackageCreationStep::adaptRulesFile(const QString &rulesFilePath)
{
    QFile rulesFile(rulesFilePath);
    rulesFile.setPermissions(rulesFile.permissions() | QFile::ExeUser);
    if (!rulesFile.open(QIODevice::ReadWrite)) {
        qWarning("Cannot open rules file for Maemo6 icon path adaptation.");
        return;
    }
    QByteArray content = rulesFile.readAll();
    const int makeInstallLine = content.indexOf("\t$(MAKE) INSTALL_ROOT");
    if (makeInstallLine == -1)
        return;
    const int makeInstallEol = content.indexOf('\n', makeInstallLine);
    if (makeInstallEol == -1)
        return;
    QString desktopFileDir = QFileInfo(rulesFile).dir().path()
        + QLatin1Char('/') + maemoTarget()->packageName()
        + QLatin1String("/usr/share/applications/");
    const Qt4BuildConfiguration * const bc = qt4BuildConfiguration();
    const MaemoGlobal::MaemoVersion version
        = MaemoGlobal::version(bc->qtVersion());
    if (version == MaemoGlobal::Maemo5)
        desktopFileDir += QLatin1String("hildon/");
#ifdef Q_OS_WIN
    desktopFileDir.remove(QLatin1Char(':'));
    desktopFileDir.prepend(QLatin1Char('/'));
#endif
    int insertPos = makeInstallEol + 1;
    for (int i = 0; i < deployStep()->deployables()->modelCount(); ++i) {
        const MaemoDeployableListModel * const model
            = deployStep()->deployables()->modelAt(i);
        if (!model->hasDesktopFile())
            continue;
        if (version == MaemoGlobal::Maemo6) {
            addWorkaroundForHarmattanBug(content, insertPos,
                model, desktopFileDir);
        }
        const QString executableFilePath = model->remoteExecutableFilePath();
        if (executableFilePath.isEmpty()) {
            qDebug("%s: Skipping subproject %s with missing deployment information.",
                Q_FUNC_INFO, qPrintable(model->proFilePath()));
            continue;
        }
        const QByteArray lineBefore("Exec=.*");
        const QByteArray lineAfter("Exec=" + executableFilePath.toUtf8());
        const QString desktopFilePath = desktopFileDir
            + model->applicationName() + QLatin1String(".desktop");
        addSedCmdToRulesFile(content, insertPos, desktopFilePath, lineBefore,
            lineAfter);
    }

    // Always check for dependencies in release builds.
    if (!(bc->qmakeBuildConfiguration() & QtVersion::DebugBuild))
        ensureShlibdeps(content);

    rulesFile.resize(0);
    rulesFile.write(content);
}

void MaemoPackageCreationStep::addWorkaroundForHarmattanBug(QByteArray &rulesFileContent,
    int &insertPos, const MaemoDeployableListModel *model,
    const QString &desktopFileDir)
{
    const QString iconFilePath = model->remoteIconFilePath();
    if (iconFilePath.isEmpty())
        return;
    const QByteArray lineBefore("^Icon=.*");
    const QByteArray lineAfter("Icon=" + iconFilePath.toUtf8());
    const QString desktopFilePath
        = desktopFileDir + model->applicationName() + QLatin1String(".desktop");
    addSedCmdToRulesFile(rulesFileContent, insertPos, desktopFilePath,
        lineBefore, lineAfter);
}

void MaemoPackageCreationStep::addSedCmdToRulesFile(QByteArray &rulesFileContent,
    int &insertPos, const QString &desktopFilePath, const QByteArray &oldString,
    const QByteArray &newString)
{
    const QString tmpFilePath = desktopFilePath + QLatin1String(".sed");
    const QByteArray sedCmd = "\tsed 's:" + oldString + ':' + newString
        + ":' " + desktopFilePath.toLocal8Bit() + " > "
        + tmpFilePath.toLocal8Bit() + " || echo -n\n";
    const QByteArray mvCmd = "\tmv " + tmpFilePath.toLocal8Bit() + ' '
        + desktopFilePath.toLocal8Bit() + " || echo -n\n";
    rulesFileContent.insert(insertPos, sedCmd);
    insertPos += sedCmd.length();
    rulesFileContent.insert(insertPos, mvCmd);
    insertPos += mvCmd.length();
}

QString MaemoPackageCreationStep::replaceDots(const QString &name)
{
    QString adaptedName = name;
    return adaptedName.replace(QLatin1Char('.'), QLatin1Char('_'));
}

QString MaemoPackageCreationStep::rpmBuildDir(const Qt4BuildConfiguration *bc)
{
    return bc->buildDirectory() + QLatin1String("/rrpmbuild");
}

const QLatin1String MaemoPackageCreationStep::CreatePackageId("Qt4ProjectManager.MaemoPackageCreationStep");

} // namespace Internal
} // namespace Qt4ProjectManager
