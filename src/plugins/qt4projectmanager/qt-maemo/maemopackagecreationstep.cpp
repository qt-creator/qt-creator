/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "maemopackagecreationstep.h"

#include "maemoconstants.h"
#include "maemodeployables.h"
#include "maemodeploystep.h"
#include "maemoglobal.h"
#include "maemopackagecreationwidget.h"
#include "maemoprofilewrapper.h"
#include "maemotemplatesmanager.h"
#include "maemotoolchain.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qt4buildconfiguration.h>
#include <qt4project.h>
#include <qt4target.h>

#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QRegExp>
#include <QtCore/QStringBuilder>
#include <QtGui/QWidget>

namespace {
    const QLatin1String PackagingEnabledKey("Packaging Enabled");
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
    connect(buildConfiguration(), SIGNAL(buildDirectoryChanged()), this,
        SIGNAL(packageFilePathChanged()));
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
    fi.reportResult(m_packagingEnabled ? createPackage() : true);
}

BuildStepConfigWidget *MaemoPackageCreationStep::createConfigWidget()
{
    return new MaemoPackageCreationWidget(this);
}

bool MaemoPackageCreationStep::createPackage()
{
    if (!packagingNeeded())
        return true;

    emit addOutput(tr("Creating package file ..."), BuildStep::MessageOutput);
    checkProjectName();
    m_buildProc.reset(new QProcess);
    QString error;
    if (!preparePackagingProcess(m_buildProc.data(), maemoToolChain(),
        buildDirectory(), &error)) {
        raiseError(error);
        return false;
    }

    connect(m_buildProc.data(), SIGNAL(readyReadStandardOutput()), this,
        SLOT(handleBuildOutput()));
    connect(m_buildProc.data(), SIGNAL(readyReadStandardError()), this,
        SLOT(handleBuildOutput()));

    const QString debianDirPath = buildDirectory() + QLatin1String("/debian");
    if (!removeDirectory(debianDirPath)) {
        raiseError(tr("Packaging failed."),
            tr("Could not remove directory '%1'.").arg(debianDirPath));
        return false;
    }
    QDir buildDir(buildDirectory());
    if (!buildDir.mkdir("debian")) {
        raiseError(tr("Could not create debian directory '%1'.")
                   .arg(debianDirPath));
        return false;
    }
    const QString templatesDirPath = MaemoTemplatesManager::instance()
        ->debianDirPath(buildConfiguration()->target()->project());
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
    }

    if (!runCommand(QLatin1String("dpkg-buildpackage -nc -uc -us")))
        return false;

    // Workaround for non-working dh_builddeb --destdir=.
    if (!QDir(buildDirectory()).isRoot()) {
        const ProjectExplorer::Project * const project
            = buildConfiguration()->target()->project();
        QString error;
        const QString pkgFileName = packageFileName(project,
            MaemoTemplatesManager::instance()->version(project, &error));
        if (!error.isEmpty())
            raiseError(tr("Packaging failed."), error);
        const QString changesFileName = QFileInfo(pkgFileName)
            .completeBaseName() + QLatin1String(".changes");
        const QString packageSourceDir = buildDirectory() + QLatin1String("/../");
        const QString packageSourceFilePath
            = packageSourceDir + pkgFileName;
        const QString changesSourceFilePath
            = packageSourceDir + changesFileName;
        const QString changesTargetFilePath
            = buildDirectory() + QLatin1Char('/') + changesFileName;
        QFile::remove(packageFilePath());
        QFile::remove(changesTargetFilePath);
        if (!QFile::rename(packageSourceFilePath, packageFilePath())
            || !QFile::rename(changesSourceFilePath, changesTargetFilePath)) {
            raiseError(tr("Packaging failed."),
                tr("Could not move package files from %1 to %2.")
                .arg(packageSourceDir, buildDirectory()));
            return false;
        }
    }

    emit addOutput(tr("Package created."), BuildStep::MessageOutput);
    deployStep()->deployables()->setUnmodified();
    return true;
}

bool MaemoPackageCreationStep::removeDirectory(const QString &dirPath)
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

bool MaemoPackageCreationStep::runCommand(const QString &command)
{
    emit addOutput(tr("Package Creation: Running command '%1'.").arg(command), BuildStep::MessageOutput);
    m_buildProc->start(packagingCommand(maemoToolChain(), command));
    if (!m_buildProc->waitForStarted()) {
        raiseError(tr("Packaging failed."),
            tr("Packaging error: Could not start command '%1'. Reason: %2")
            .arg(command).arg(m_buildProc->errorString()));
        return false;
    }
    m_buildProc->waitForFinished(-1);
    if (m_buildProc->error() != QProcess::UnknownError || m_buildProc->exitCode() != 0) {
        QString mainMessage = tr("Packaging Error: Command '%1' failed.")
            .arg(command);
        if (m_buildProc->error() != QProcess::UnknownError)
            mainMessage += tr(" Reason: %1").arg(m_buildProc->errorString());
        else
            mainMessage += tr("Exit code: %1").arg(m_buildProc->exitCode());
        raiseError(mainMessage);
        return false;
    }
    return true;
}

void MaemoPackageCreationStep::handleBuildOutput()
{
    const QByteArray &stdOut = m_buildProc->readAllStandardOutput();
    if (!stdOut.isEmpty())
        emit addOutput(QString::fromLocal8Bit(stdOut), BuildStep::NormalOutput);
    const QByteArray &errorOut = m_buildProc->readAllStandardError();
    if (!errorOut.isEmpty()) {
        emit addOutput(QString::fromLocal8Bit(errorOut), BuildStep::ErrorOutput);
    }
}

const Qt4BuildConfiguration *MaemoPackageCreationStep::qt4BuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(buildConfiguration());
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

const MaemoToolChain *MaemoPackageCreationStep::maemoToolChain() const
{
    return static_cast<MaemoToolChain *>(qt4BuildConfiguration()->toolChain());
}

MaemoDeployStep *MaemoPackageCreationStep::deployStep() const
{
    MaemoDeployStep * const deployStep
        = MaemoGlobal::buildStep<MaemoDeployStep>(target()->activeDeployConfiguration());
    Q_ASSERT(deployStep &&
        "Fatal error: Maemo build configuration without deploy step.");
    return deployStep;
}

QString MaemoPackageCreationStep::maddeRoot() const
{
    return maemoToolChain()->maddeRoot();
}

QString MaemoPackageCreationStep::targetRoot() const
{
    return maemoToolChain()->targetRoot();
}

bool MaemoPackageCreationStep::packagingNeeded() const
{
    const MaemoDeployables * const deployables = deployStep()->deployables();
    QFileInfo packageInfo(packageFilePath());
    if (!packageInfo.exists() || deployables->isModified())
        return true;

    const int deployableCount = deployables->deployableCount();
    for (int i = 0; i < deployableCount; ++i) {
        if (packageInfo.lastModified()
            <= QFileInfo(deployables->deployableAt(i).localFilePath)
               .lastModified())
            return true;
    }

    return false;
}

QString MaemoPackageCreationStep::packageFilePath() const
{
    QString error;
    const QString &version = versionString(&error);
    if (version.isEmpty())
        return QString();
    return buildDirectory() % '/'
        % packageFileName(buildConfiguration()->target()->project(), version);
}

QString MaemoPackageCreationStep::versionString(QString *error) const
{
    return MaemoTemplatesManager::instance()
        ->version(buildConfiguration()->target()->project(), error);

}

bool MaemoPackageCreationStep::setVersionString(const QString &version,
    QString *error)
{
    const bool success = MaemoTemplatesManager::instance()
        ->setVersion(buildConfiguration()->target()->project(), version, error);
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

bool MaemoPackageCreationStep::preparePackagingProcess(QProcess *proc,
    const MaemoToolChain *tc, const QString &workingDir, QString *error)
{
    QFile configFile(tc->targetRoot() % QLatin1String("/config.sh"));
    if (!configFile.open(QIODevice::ReadOnly)) {
        *error = tr("Cannot open MADDE config file '%1'.")
            .arg(nativePath(configFile));
        return false;
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString &path
        = QDir::toNativeSeparators(tc->maddeRoot() + QLatin1Char('/'));

    const QLatin1String key("PATH");
    QString colon = QLatin1String(":");
#ifdef Q_OS_WIN
    colon = QLatin1String(";");
    env.insert(key, path % QLatin1String("bin") % colon % env.value(key));
#endif

    env.insert(key, tc->targetRoot() % "/bin" % colon % env.value(key));
    env.insert(key, path % QLatin1String("madbin") % colon % env.value(key));

    QString perlLib
        = QDir::fromNativeSeparators(path % QLatin1String("madlib/perl5"));
#ifdef Q_OS_WIN
    perlLib = perlLib.remove(QLatin1Char(':'));
    perlLib = perlLib.prepend(QLatin1Char('/'));
#endif
    env.insert(QLatin1String("PERL5LIB"), perlLib);
    env.insert(QLatin1String("PWD"), workingDir);

    const QRegExp envPattern(QLatin1String("([^=]+)=[\"']?([^;\"']+)[\"']? ;.*"));
    QByteArray line;
    do {
        line = configFile.readLine(200);
        if (envPattern.exactMatch(line))
            env.insert(envPattern.cap(1), envPattern.cap(2));
    } while (!line.isEmpty());

    proc->setProcessEnvironment(env);
    proc->setWorkingDirectory(workingDir);
    proc->start("cd " + workingDir);
    proc->waitForFinished();
    return true;
}

QString MaemoPackageCreationStep::packagingCommand(const MaemoToolChain *tc,
    const QString &commandName)
{
    QString perl;
#ifdef Q_OS_WIN
    perl = tc->maddeRoot() + QLatin1String("/bin/perl.exe ");
#endif
    return perl + tc->maddeRoot() % QLatin1String("/madbin/") % commandName;
}

void MaemoPackageCreationStep::checkProjectName()
{
    const QRegExp legalName(QLatin1String("[0-9-+a-z\\.]+"));
    if (!legalName.exactMatch(buildConfiguration()->target()->project()->displayName())) {
        emit addTask(Task(Task::Warning,
            tr("Your project name contains characters not allowed in Debian packages.\n"
               "They must only use lower-case letters, numbers, '-', '+' and '.'.\n"
               "We will try to work around that, but you may experience problems."),
               QString(), -1, TASK_CATEGORY_BUILDSYSTEM));
    }
}

QString MaemoPackageCreationStep::packageName(const ProjectExplorer::Project *project)
{
    QString packageName = project->displayName().toLower();
    const QRegExp legalLetter(QLatin1String("[a-z0-9+-.]"), Qt::CaseSensitive,
        QRegExp::WildcardUnix);
    for (int i = 0; i < packageName.length(); ++i) {
        if (!legalLetter.exactMatch(packageName.mid(i, 1)))
            packageName[i] = QLatin1Char('-');
    }
    return packageName;
}

QString MaemoPackageCreationStep::packageFileName(const ProjectExplorer::Project *project,
    const QString &version)
{
    return packageName(project) % QLatin1Char('_') % version
        % QLatin1String("_armel.deb");
}

const QLatin1String MaemoPackageCreationStep::CreatePackageId("Qt4ProjectManager.MaemoPackageCreationStep");

} // namespace Internal
} // namespace Qt4ProjectManager
