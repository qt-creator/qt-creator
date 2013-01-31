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

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4nodes.h>

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
    Utils::FileName androidLibPath;
    if (project->rootQt4ProjectNode()->variableValue(Qt4ProjectManager::ConfigVar).contains(QLatin1String("x86")))
        androidLibPath = path.appendPath(QLatin1String("libs/x86"));
    else if (project->rootQt4ProjectNode()
             ->variableValue(Qt4ProjectManager::ConfigVar).contains(QLatin1String("armeabi-v7a")))
        androidLibPath = path.appendPath(QLatin1String("libs/armeabi-v7a"));
    else
        androidLibPath = path.appendPath(QLatin1String("libs/armeabi"));
    m_gdbServerDestination = androidLibPath.appendPath(QLatin1String("gdbserver"));
    m_gdbServerSource = AndroidConfigurations::instance().gdbServerPath(target()->activeRunConfiguration()->abi().architecture());
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
    foreach (QByteArray line, lines) {
        if (line.contains("(NEEDED)") && line.contains("Shared library:") ) {
            const int pos = line.lastIndexOf('[') + 1;
            (*libs) << QString::fromLatin1(line.mid(pos, line.length() - pos - 1));
        }
    }
}

void AndroidPackageCreationStep::checkRequiredLibraries()
{
    QProcess readelfProc;
    QString appPath = AndroidManager::targetApplicationPath(target());
    if (!QFile::exists(appPath)) {
        raiseError(msgCannotFindElfInformation(), msgCannotFindExecutable(appPath));
        return;
    }
    readelfProc.start(AndroidConfigurations::instance().readelfPath(target()->activeRunConfiguration()->abi().architecture()).toString(),
                      QStringList() << QLatin1String("-d") << QLatin1String("-W") << appPath);
    if (!readelfProc.waitForFinished(-1)) {
        readelfProc.kill();
        return;
    }
    QStringList libs;
    parseSharedLibs(readelfProc.readAll(), &libs);
    QStringList checkedLibs = AndroidManager::qtLibs(target());
    QStringList requiredLibraries;
    foreach (const QString &qtLib, AndroidManager::availableQtLibs(target())) {
        if (libs.contains(QLatin1String("lib") + qtLib + QLatin1String(".so")) || checkedLibs.contains(qtLib))
            requiredLibraries << qtLib;
    }
    AndroidManager::setQtLibs(target(), requiredLibraries);

    checkedLibs = AndroidManager::prebundledLibs(target());
    requiredLibraries.clear();
    foreach (const QString &qtLib, AndroidManager::availableQtLibs(target())) {
        if (libs.contains(qtLib) || checkedLibs.contains(qtLib))
            requiredLibraries << qtLib;
    }
    AndroidManager::setPrebundledLibs(target(), requiredLibraries);
    emit updateRequiredLibrariesModels();
}

void AndroidPackageCreationStep::initCheckRequiredLibrariesForRun()
{
    m_appPath = Utils::FileName::fromString(AndroidManager::targetApplicationPath(target()));
    m_readElf = AndroidConfigurations::instance().readelfPath(target()->activeRunConfiguration()->abi().architecture());
    m_qtLibs = AndroidManager::qtLibs(target());
    m_availableQtLibs = AndroidManager::availableQtLibs(target());
    m_prebundledLibs = AndroidManager::prebundledLibs(target());
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
    QStringList requiredLibraries;
    foreach (const QString &qtLib, m_availableQtLibs) {
        if (libs.contains(QLatin1String("lib") + qtLib + QLatin1String(".so")) || m_qtLibs.contains(qtLib))
            requiredLibraries << qtLib;
    }
    QMetaObject::invokeMethod(this, "setQtLibs",Qt::BlockingQueuedConnection,
                              Q_ARG(QStringList, requiredLibraries));

    requiredLibraries.clear();
    foreach (const QString &qtLib, m_availableQtLibs) {
        if (libs.contains(qtLib) || m_prebundledLibs.contains(qtLib))
            requiredLibraries << qtLib;
    }

    QMetaObject::invokeMethod(this, "setPrebundledLibs", Qt::BlockingQueuedConnection,
                              Q_ARG(QStringList, requiredLibraries));
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

bool AndroidPackageCreationStep::createPackage()
{
    checkRequiredLibrariesForRun();

    emit addOutput(tr("Copy Qt app & libs to Android package ..."), MessageOutput);

    QStringList build;
    build << QLatin1String("clean");
    QFile::remove(m_gdbServerDestination.toString());
    if (m_debugBuild || !m_certificateAlias.length()) {
        build << QLatin1String("debug");
        if (!QFile::copy(m_gdbServerSource.toString(), m_gdbServerDestination.toString())) {
            raiseError(tr("Can't copy gdbserver from '%1' to '%2'").arg(m_gdbServerSource.toUserOutput())
                       .arg(m_gdbServerDestination.toUserOutput()));
            return false;
        }
    } else {
        build << QLatin1String("release");
    }

    emit addOutput(tr("Creating package file ..."), MessageOutput);

    QProcess *const buildProc = new QProcess;

    connect(buildProc, SIGNAL(readyReadStandardOutput()), this,
        SLOT(handleBuildStdOutOutput()));
    connect(buildProc, SIGNAL(readyReadStandardError()), this,
        SLOT(handleBuildStdErrOutput()));

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

void AndroidPackageCreationStep::stripAndroidLibs(const QStringList & files, Abi::Architecture architecture)
{
    QProcess stripProcess;
    foreach (const QString &file, files) {
        stripProcess.start(AndroidConfigurations::instance().stripPath(architecture).toString(),
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

    process->setReadChannel(QProcess::StandardOutput);
    while (process->canReadLine()) {
        QString line = QString::fromLocal8Bit(process->readLine());
        m_outputParser.stdOutput(line);
        emit addOutput(line, BuildStep::NormalOutput, BuildStep::DontAppendNewline);
    }
}

void AndroidPackageCreationStep::handleBuildStdErrOutput()
{
    QProcess *const process = qobject_cast<QProcess *>(sender());
    if (!process)
        return;

    process->setReadChannel(QProcess::StandardError);
    while (process->canReadLine()) {
        QString line = QString::fromLocal8Bit(process->readLine());
        m_outputParser.stdError(line);
        emit addOutput(line, BuildStep::ErrorOutput, BuildStep::DontAppendNewline);
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
