/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "s60devicerunconfiguration.h"
#include "s60devicerunconfigurationwidget.h"
#include "qt4project.h"
#include "qtversionmanager.h"
#include "profilereader.h"
#include "s60manager.h"
#include "s60devices.h"
#include "s60runconfigbluetoothstarter.h"
#include "bluetoothlistener_gui.h"
#include "serialdevicelister.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <utils/qtcassert.h>
#include <utils/pathchooser.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/persistentsettings.h>
#include <projectexplorer/project.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/persistentsettings.h>

#include <debugger/debuggermanager.h>

#include <QtGui/QMessageBox>
#include <QtGui/QMainWindow>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager::Internal;

// Format information about a file
static QString lsFile(const QString &f)
{
    QString rc;
    const QFileInfo fi(f);
    QTextStream str(&rc);
    str << fi.size() << ' ' << fi.lastModified().toString(Qt::ISODate) << ' ' << QDir::toNativeSeparators(fi.absoluteFilePath());
    return rc;
}

// ======== S60DeviceRunConfiguration
S60DeviceRunConfiguration::S60DeviceRunConfiguration(Project *project, const QString &proFilePath)
    : RunConfiguration(project),
    m_proFilePath(proFilePath),
    m_cachedTargetInformationValid(false),
#ifdef Q_OS_WIN
    m_serialPortName(QLatin1String("COM5")),
    m_communicationType(SerialPortCommunication),
#else
    m_serialPortName(QLatin1String(SerialDeviceLister::linuxBlueToothDeviceRootC) + QLatin1Char('0')),
    m_communicationType(BlueToothCommunication),
#endif
    m_signingMode(SignSelf)
{
    if (!m_proFilePath.isEmpty())
        setName(tr("%1 on Symbian Device").arg(QFileInfo(m_proFilePath).completeBaseName()));
    else
        setName(tr("QtS60DeviceRunConfiguration"));

    connect(project, SIGNAL(activeBuildConfigurationChanged()),
            this, SLOT(invalidateCachedTargetInformation()));

    connect(project, SIGNAL(targetInformationChanged()),
            this, SLOT(invalidateCachedTargetInformation()));
}

S60DeviceRunConfiguration::~S60DeviceRunConfiguration()
{
}

QString S60DeviceRunConfiguration::type() const
{
    return QLatin1String("Qt4ProjectManager.DeviceRunConfiguration");
}

ProjectExplorer::ToolChain::ToolChainType S60DeviceRunConfiguration::toolChainType() const
{
    if (const Qt4Project *pro = qobject_cast<const Qt4Project*>(project()))
        return pro->toolChainType(pro->activeBuildConfiguration());
    return ProjectExplorer::ToolChain::INVALID;
}

bool S60DeviceRunConfiguration::isEnabled() const
{
    const ToolChain::ToolChainType type = toolChainType();
    return type == ToolChain::GCCE || type == ToolChain::RVCT_ARMV5 || type == ToolChain::RVCT_ARMV6;
}

QWidget *S60DeviceRunConfiguration::configurationWidget()
{
    return new S60DeviceRunConfigurationWidget(this);
}

void S60DeviceRunConfiguration::save(PersistentSettingsWriter &writer) const
{
    const QDir projectDir = QFileInfo(project()->file()->fileName()).absoluteDir();
    writer.saveValue("ProFile", projectDir.relativeFilePath(m_proFilePath));
    writer.saveValue("SigningMode", (int)m_signingMode);
    writer.saveValue("CustomSignaturePath", m_customSignaturePath);
    writer.saveValue("CustomKeyPath", m_customKeyPath);
    writer.saveValue("SerialPortName", m_serialPortName);
    writer.saveValue("CommunicationType", m_communicationType);
    RunConfiguration::save(writer);
}

void S60DeviceRunConfiguration::restore(const PersistentSettingsReader &reader)
{
    RunConfiguration::restore(reader);
    const QDir projectDir = QFileInfo(project()->file()->fileName()).absoluteDir();
    m_proFilePath = projectDir.filePath(reader.restoreValue("ProFile").toString());
    m_signingMode = (SigningMode)reader.restoreValue("SigningMode").toInt();
    m_customSignaturePath = reader.restoreValue("CustomSignaturePath").toString();
    m_customKeyPath = reader.restoreValue("CustomKeyPath").toString();
    m_serialPortName = reader.restoreValue("SerialPortName").toString().trimmed();
    m_communicationType = reader.restoreValue("CommunicationType").toInt();
}

QString S60DeviceRunConfiguration::serialPortName() const
{
    return m_serialPortName;
}

void S60DeviceRunConfiguration::setSerialPortName(const QString &name)
{
    m_serialPortName = name.trimmed();
}

int S60DeviceRunConfiguration::communicationType() const
{
    return m_communicationType;
}

void S60DeviceRunConfiguration::setCommunicationType(int t)
{
    m_communicationType = t;
}

QString S60DeviceRunConfiguration::targetName() const
{
    const_cast<S60DeviceRunConfiguration *>(this)->updateTarget();
    return m_targetName;
}

QString S60DeviceRunConfiguration::basePackageFilePath() const
{
    const_cast<S60DeviceRunConfiguration *>(this)->updateTarget();
    return m_baseFileName;
}

QString S60DeviceRunConfiguration::symbianPlatform() const
{
    const_cast<S60DeviceRunConfiguration *>(this)->updateTarget();
    return m_platform;
}

QString S60DeviceRunConfiguration::symbianTarget() const
{
    const_cast<S60DeviceRunConfiguration *>(this)->updateTarget();
    return m_target;
}

QString S60DeviceRunConfiguration::packageTemplateFileName() const
{
    const_cast<S60DeviceRunConfiguration *>(this)->updateTarget();
    return m_packageTemplateFileName;
}

S60DeviceRunConfiguration::SigningMode S60DeviceRunConfiguration::signingMode() const
{
    return m_signingMode;
}

void S60DeviceRunConfiguration::setSigningMode(SigningMode mode)
{
    m_signingMode = mode;
}

QString S60DeviceRunConfiguration::customSignaturePath() const
{
    return m_customSignaturePath;
}

void S60DeviceRunConfiguration::setCustomSignaturePath(const QString &path)
{
    m_customSignaturePath = path;
}

QString S60DeviceRunConfiguration::customKeyPath() const
{
    return m_customKeyPath;
}

void S60DeviceRunConfiguration::setCustomKeyPath(const QString &path)
{
    m_customKeyPath = path;
}

QString S60DeviceRunConfiguration::packageFileName() const
{
    QString rc = basePackageFilePath();
    if (!rc.isEmpty())
        rc += QLatin1String(".pkg");
    return rc;
}

QString S60DeviceRunConfiguration::localExecutableFileName() const
{
    Qt4Project *qt4project = qobject_cast<Qt4Project *>(project());
    S60Devices::Device device = S60Manager::instance()->deviceForQtVersion(
            qt4project->qtVersion(qt4project->activeBuildConfiguration()));

    QString localExecutable = device.epocRoot;
    localExecutable += QString::fromLatin1("/epoc32/release/%1/%2/%3.exe")
                       .arg(symbianPlatform()).arg(symbianTarget()).arg(targetName());
    qDebug() << localExecutable;
    return QDir::toNativeSeparators(localExecutable);
}

void S60DeviceRunConfiguration::updateTarget()
{
    if (m_cachedTargetInformationValid)
        return;
    Qt4Project *pro = static_cast<Qt4Project *>(project());
    Qt4PriFileNode * priFileNode = static_cast<Qt4Project *>(project())->rootProjectNode()->findProFileFor(m_proFilePath);
    if (!priFileNode) {
        m_baseFileName = QString::null;
        m_cachedTargetInformationValid = true;
        emit targetInformationChanged();
        return;
    }
    QtVersion *qtVersion = pro->qtVersion(pro->activeBuildConfiguration());
    ProFileReader *reader = priFileNode->createProFileReader();
    reader->setCumulative(false);
    reader->setQtVersion(qtVersion);

    // Find out what flags we pass on to qmake, this code is duplicated in the qmake step
    QtVersion::QmakeBuildConfig defaultBuildConfiguration = qtVersion->defaultBuildConfig();
    QtVersion::QmakeBuildConfig projectBuildConfiguration = QtVersion::QmakeBuildConfig(pro->activeBuildConfiguration()
                                                                                        ->value("buildConfiguration").toInt());
    QStringList addedUserConfigArguments;
    QStringList removedUserConfigArguments;
    if ((defaultBuildConfiguration & QtVersion::BuildAll) && !(projectBuildConfiguration & QtVersion::BuildAll))
        removedUserConfigArguments << "debug_and_release";
    if (!(defaultBuildConfiguration & QtVersion::BuildAll) && (projectBuildConfiguration & QtVersion::BuildAll))
        addedUserConfigArguments << "debug_and_release";
    if ((defaultBuildConfiguration & QtVersion::DebugBuild) && !(projectBuildConfiguration & QtVersion::DebugBuild))
        addedUserConfigArguments << "release";
    if (!(defaultBuildConfiguration & QtVersion::DebugBuild) && (projectBuildConfiguration & QtVersion::DebugBuild))
        addedUserConfigArguments << "debug";

    reader->setUserConfigCmdArgs(addedUserConfigArguments, removedUserConfigArguments);

    if (!reader->readProFile(m_proFilePath)) {
        delete reader;
        Core::ICore::instance()->messageManager()->printToOutputPane(tr("Could not parse %1. The QtS60 Device run configuration %2 can not be started.").arg(m_proFilePath).arg(name()));
        return;
    }

    // Extract data
    const QDir baseProjectDirectory = QFileInfo(project()->file()->fileName()).absoluteDir();
    const QString relSubDir = baseProjectDirectory.relativeFilePath(QFileInfo(m_proFilePath).path());
    const QDir baseBuildDirectory = project()->buildDirectory(project()->activeBuildConfiguration());
    const QString baseDir = baseBuildDirectory.absoluteFilePath(relSubDir);

    // Directory
    QString m_workingDir;
    if (reader->contains("DESTDIR")) {
        m_workingDir = reader->value("DESTDIR");
        if (QDir::isRelativePath(m_workingDir)) {
            m_workingDir = baseDir + QLatin1Char('/') + m_workingDir;
        }
    } else {
        m_workingDir = baseDir;
    }

    m_targetName = reader->value("TARGET");
    if (m_targetName.isEmpty())
        m_targetName = QFileInfo(m_proFilePath).baseName();

    m_baseFileName = QDir::cleanPath(m_workingDir + QLatin1Char('/') + m_targetName);
    m_packageTemplateFileName = QDir::cleanPath(
            m_workingDir + QLatin1Char('/') + m_targetName + QLatin1String("_template.pkg"));

    ToolChain::ToolChainType toolchain = pro->toolChainType(pro->activeBuildConfiguration());
    if (toolchain == ToolChain::GCCE)
        m_platform = QLatin1String("gcce");
    else if (toolchain == ToolChain::RVCT_ARMV5)
        m_platform = QLatin1String("armv5");
    else
        m_platform = QLatin1String("armv6");
    if (projectBuildConfiguration & QtVersion::DebugBuild)
        m_target = QLatin1String("udeb");
    else
        m_target = QLatin1String("urel");
    m_baseFileName += QLatin1Char('_') + m_platform + QLatin1Char('_') + m_target;
    delete reader;
    m_cachedTargetInformationValid = true;
    emit targetInformationChanged();
}

void S60DeviceRunConfiguration::invalidateCachedTargetInformation()
{
    m_cachedTargetInformationValid = false;
    emit targetInformationChanged();
}


// ======== S60DeviceRunConfigurationFactory

S60DeviceRunConfigurationFactory::S60DeviceRunConfigurationFactory(QObject *parent)
    : IRunConfigurationFactory(parent)
{
}

S60DeviceRunConfigurationFactory::~S60DeviceRunConfigurationFactory()
{
}

bool S60DeviceRunConfigurationFactory::canRestore(const QString &type) const
{
    return type == "Qt4ProjectManager.DeviceRunConfiguration";
}

QStringList S60DeviceRunConfigurationFactory::availableCreationTypes(Project *pro) const
{
    Qt4Project *qt4project = qobject_cast<Qt4Project *>(pro);
    if (qt4project) {
        QStringList applicationProFiles;
        QList<Qt4ProFileNode *> list = qt4project->applicationProFiles();
        foreach (Qt4ProFileNode * node, list) {
            applicationProFiles.append("QtSymbianDeviceRunConfiguration." + node->path());
        }
        return applicationProFiles;
    } else {
        return QStringList();
    }
}

QString S60DeviceRunConfigurationFactory::displayNameForType(const QString &type) const
{
    QString fileName = type.mid(QString("QtSymbianDeviceRunConfiguration.").size());
    return tr("%1 on Symbian Device").arg(QFileInfo(fileName).completeBaseName());
}

QSharedPointer<RunConfiguration> S60DeviceRunConfigurationFactory::create(Project *project, const QString &type)
{
    Qt4Project *p = qobject_cast<Qt4Project *>(project);
    Q_ASSERT(p);
    if (type.startsWith("QtSymbianDeviceRunConfiguration.")) {
        QString fileName = type.mid(QString("QtSymbianDeviceRunConfiguration.").size());
        return QSharedPointer<RunConfiguration>(new S60DeviceRunConfiguration(p, fileName));
    }
    Q_ASSERT(type == "Qt4ProjectManager.DeviceRunConfiguration");
    // The right path is set in restoreSettings
    QSharedPointer<RunConfiguration> rc(new S60DeviceRunConfiguration(p, QString::null));
    return rc;
}

// ======== S60DeviceRunControlBase

S60DeviceRunControlBase::S60DeviceRunControlBase(const QSharedPointer<RunConfiguration> &runConfiguration) :
    RunControl(runConfiguration),    
    m_makesis(new QProcess(this)),
    m_signsis(new QProcess(this)),
    m_launcher(0)
{    
    connect(m_makesis, SIGNAL(readyReadStandardError()),
            this, SLOT(readStandardError()));
    connect(m_makesis, SIGNAL(readyReadStandardOutput()),
            this, SLOT(readStandardOutput()));
    connect(m_makesis, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(makesisProcessFailed()));
    connect(m_makesis, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(makesisProcessFinished()));

    connect(m_signsis, SIGNAL(readyReadStandardError()),
            this, SLOT(readStandardError()));
    connect(m_signsis, SIGNAL(readyReadStandardOutput()),
            this, SLOT(readStandardOutput()));
    connect(m_signsis, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(signsisProcessFailed()));
    connect(m_signsis, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(signsisProcessFinished()));

    Qt4Project *project = qobject_cast<Qt4Project *>(runConfiguration->project());
    QTC_ASSERT(project, return);

    QSharedPointer<S60DeviceRunConfiguration> s60runConfig = runConfiguration.objectCast<S60DeviceRunConfiguration>();
    QTC_ASSERT(s60runConfig, return);

    m_serialPortName = s60runConfig->serialPortName();
    m_serialPortFriendlyName = S60Manager::instance()->serialDeviceLister()->friendlyNameForPort(m_serialPortName);
    m_communicationType = s60runConfig->communicationType();
    m_targetName = s60runConfig->targetName();
    m_baseFileName = s60runConfig->basePackageFilePath();
    m_symbianPlatform = s60runConfig->symbianPlatform();
    m_symbianTarget = s60runConfig->symbianTarget();
    m_packageTemplateFile = s60runConfig->packageTemplateFileName();
    m_workingDirectory = QFileInfo(m_baseFileName).absolutePath();
    m_qtDir = project->qtVersion(project->activeBuildConfiguration())->versionInfo().value("QT_INSTALL_DATA");
    m_useCustomSignature = (s60runConfig->signingMode() == S60DeviceRunConfiguration::SignCustom);
    m_customSignaturePath = s60runConfig->customSignaturePath();
    m_customKeyPath = s60runConfig->customKeyPath();
    m_toolsDirectory = S60Manager::instance()->deviceForQtVersion(
            project->qtVersion(project->activeBuildConfiguration())).toolsRoot
            + "/epoc32/tools";
    m_executableFileName = lsFile(s60runConfig->localExecutableFileName());
    m_makesisTool = m_toolsDirectory + "/makesis.exe";
    m_packageFilePath = s60runConfig->packageFileName();
    m_packageFile = QFileInfo(m_packageFilePath).fileName();
}

S60DeviceRunControlBase::~S60DeviceRunControlBase()
{
    if (m_launcher) {
        m_launcher->deleteLater();
        m_launcher = 0;
    }
}

void S60DeviceRunControlBase::start()
{
    emit started();
    if (m_serialPortName.isEmpty()) {
        error(this, tr("There is no device plugged in."));
        emit finished();
        return;
    }

    emit addToOutputWindow(this, tr("Creating %1.sisx ...").arg(QDir::toNativeSeparators(m_baseFileName)));
    emit addToOutputWindow(this, tr("Executable file: %1").arg(m_executableFileName));

    QString errorMessage;
    QString settingsCategory;
    QString settingsPage;
    if (!checkConfiguration(&errorMessage, &settingsCategory, &settingsPage)) {
        error(this, errorMessage);
        emit finished();
        Core::ICore::instance()->showWarningWithOptions(tr("Debugger for Symbian Platform"),
                                                        errorMessage, QString(),
                                                        settingsCategory, settingsPage);
        return;
    }

    if (!createPackageFileFromTemplate(&errorMessage)) {
        error(this, errorMessage);
        emit finished();
        return;
    }
    m_makesis->setWorkingDirectory(m_workingDirectory);
    emit addToOutputWindow(this, tr("%1 %2").arg(QDir::toNativeSeparators(m_makesisTool), m_packageFile));
    m_makesis->start(m_makesisTool, QStringList(m_packageFile), QIODevice::ReadOnly);
}

static inline void stopProcess(QProcess *p)
{
    const int timeOutMS = 200;
    if (p->state() != QProcess::Running)
        return;
    p->terminate();
    if (p->waitForFinished(timeOutMS))
        return;
    p->kill();
}

void S60DeviceRunControlBase::stop()
{
    if (m_makesis)
        stopProcess(m_makesis);
    if (m_signsis)
        stopProcess(m_signsis);
    if (m_launcher)
        m_launcher->terminate();
}

bool S60DeviceRunControlBase::isRunning() const
{
    return m_makesis->state() != QProcess::NotRunning;
}

void S60DeviceRunControlBase::readStandardError()
{
    QProcess *process = static_cast<QProcess *>(sender());
    QByteArray data = process->readAllStandardError();
    emit addToOutputWindowInline(this, QString::fromLocal8Bit(data.constData(), data.length()));
}

void S60DeviceRunControlBase::readStandardOutput()
{
    QProcess *process = static_cast<QProcess *>(sender());
    QByteArray data = process->readAllStandardOutput();
    emit addToOutputWindowInline(this, QString::fromLocal8Bit(data.constData(), data.length()));
}

bool S60DeviceRunControlBase::createPackageFileFromTemplate(QString *errorMessage)
{
    QFile packageTemplate(m_packageTemplateFile);
    if (!packageTemplate.open(QIODevice::ReadOnly)) {
        *errorMessage = tr("Could not read template package file '%1'").arg(QDir::toNativeSeparators(m_packageTemplateFile));
        return false;
    }
    QString contents = packageTemplate.readAll();
    packageTemplate.close();
    contents.replace(QLatin1String("$(PLATFORM)"), m_symbianPlatform);
    contents.replace(QLatin1String("$(TARGET)"), m_symbianTarget);
    QFile packageFile(m_packageFilePath);
    if (!packageFile.open(QIODevice::WriteOnly)) {
        *errorMessage = tr("Could not write package file '%1'").arg(QDir::toNativeSeparators(m_packageFilePath));
        return false;
    }
    packageFile.write(contents.toLocal8Bit());
    packageFile.close();
    return true;
}

void S60DeviceRunControlBase::makesisProcessFailed()
{
    processFailed("makesis.exe", m_makesis->error());
}

void S60DeviceRunControlBase::makesisProcessFinished()
{
    if (m_makesis->exitCode() != 0) {
        error(this, tr("An error occurred while creating the package."));
        stop();
        emit finished();
        return;
    }
    QString signsisTool = m_toolsDirectory + QLatin1String("/signsis.exe");
    QString sisFile = QFileInfo(m_baseFileName + QLatin1String(".sis")).fileName();
    QString sisxFile = QFileInfo(m_baseFileName + QLatin1String(".sisx")).fileName();
    QString signature = (m_useCustomSignature ? m_customSignaturePath
                         : m_qtDir + QLatin1String("/src/s60installs/selfsigned.cer"));
    QString key = (m_useCustomSignature ? m_customKeyPath
                         : m_qtDir + QLatin1String("/src/s60installs/selfsigned.key"));
    QStringList arguments;
    arguments << sisFile
            << sisxFile << QDir::toNativeSeparators(signature)
            << QDir::toNativeSeparators(key);
    m_signsis->setWorkingDirectory(m_workingDirectory);
    emit addToOutputWindow(this, tr("%1 %2").arg(QDir::toNativeSeparators(signsisTool), arguments.join(QString(QLatin1Char(' ')))));
    m_signsis->start(signsisTool, arguments, QIODevice::ReadOnly);
}

void S60DeviceRunControlBase::signsisProcessFailed()
{
    processFailed("signsis.exe", m_signsis->error());
}

void S60DeviceRunControlBase::signsisProcessFinished()
{
    if (m_signsis->exitCode() != 0) {
        error(this, tr("An error occurred while creating the package."));
        stop();
        emit finished();
        return;
    }
    m_launcher = new trk::Launcher();
    connect(m_launcher, SIGNAL(finished()), this, SLOT(launcherFinished()));
    connect(m_launcher, SIGNAL(canNotConnect(QString)), this, SLOT(printConnectFailed(QString)));
    connect(m_launcher, SIGNAL(copyingStarted()), this, SLOT(printCopyingNotice()));
    connect(m_launcher, SIGNAL(canNotCreateFile(QString,QString)), this, SLOT(printCreateFileFailed(QString,QString)));
    connect(m_launcher, SIGNAL(canNotWriteFile(QString,QString)), this, SLOT(printWriteFileFailed(QString,QString)));
    connect(m_launcher, SIGNAL(canNotCloseFile(QString,QString)), this, SLOT(printCloseFileFailed(QString,QString)));
    connect(m_launcher, SIGNAL(installingStarted()), this, SLOT(printInstallingNotice()));
    connect(m_launcher, SIGNAL(canNotInstall(QString,QString)), this, SLOT(printInstallFailed(QString,QString)));
    connect(m_launcher, SIGNAL(copyProgress(int)), this, SLOT(printCopyProgress(int)));
    connect(m_launcher, SIGNAL(stateChanged(int)), this, SLOT(slotLauncherStateChanged(int)));

    //TODO sisx destination and file path user definable
    m_launcher->setTrkServerName(m_serialPortName);
    m_launcher->setSerialFrame(m_communicationType == SerialPortCommunication);
    const QString copySrc(m_baseFileName + ".sisx");
    const QString copyDst = QString("C:\\Data\\%1.sisx").arg(QFileInfo(m_baseFileName).fileName());
    const QString runFileName = QString("C:\\sys\\bin\\%1.exe").arg(m_targetName);
    m_launcher->setCopyFileName(copySrc, copyDst);
    m_launcher->setInstallFileName(copyDst);
    initLauncher(runFileName, m_launcher);
    emit addToOutputWindow(this, tr("Package: %1\nDeploying application to '%2'...").arg(lsFile(copySrc), m_serialPortFriendlyName));
    QString errorMessage;
    // Prompt the user to start up the Blue tooth connection    
    const trk::PromptStartCommunicationResult src =
            S60RunConfigBluetoothStarter::startCommunication(m_launcher->trkDevice(),
                                                             m_serialPortName,
                                                             m_communicationType, 0,
                                                             &errorMessage);
    switch (src) {
    case trk::PromptStartCommunicationConnected:
        break;
    case trk::PromptStartCommunicationCanceled:
    case trk::PromptStartCommunicationError:
        error(this, errorMessage);
        stop();
        emit finished();
        return;
    };

    if (!m_launcher->startServer(&errorMessage)) {

        error(this, tr("Could not connect to phone on port '%1': %2\n"
                       "Check if the phone is connected and the TRK application is running.").arg(m_serialPortName, errorMessage));
        stop();
        emit finished();
    }
}

void S60DeviceRunControlBase::printCreateFileFailed(const QString &filename, const QString &errorMessage)
{
    emit addToOutputWindow(this, tr("Could not create file %1 on device: %2").arg(filename, errorMessage));
}

void S60DeviceRunControlBase::printWriteFileFailed(const QString &filename, const QString &errorMessage)
{
    emit addToOutputWindow(this, tr("Could not write to file %1 on device: %2").arg(filename, errorMessage));
}

void S60DeviceRunControlBase::printCloseFileFailed(const QString &filename, const QString &errorMessage)
{
    const QString msg = tr("Could not close file %1 on device: %2. It will be closed when App TRK is closed.");
    emit addToOutputWindow(this, msg.arg(filename, errorMessage));
}

void S60DeviceRunControlBase::printConnectFailed(const QString &errorMessage)
{
    emit addToOutputWindow(this, tr("Could not connect to App TRK on device: %1. Restarting App TRK might help.").arg(errorMessage));
}

void S60DeviceRunControlBase::printCopyingNotice()
{
    emit addToOutputWindow(this, tr("Copying install file..."));
}

void S60DeviceRunControlBase::printCopyProgress(int progress)
{
    emit addToOutputWindow(this, tr("%1% copied.").arg(progress));
}

void S60DeviceRunControlBase::printInstallingNotice()
{
    emit addToOutputWindow(this, tr("Installing application..."));
}

void S60DeviceRunControlBase::printInstallFailed(const QString &filename, const QString &errorMessage)
{
    emit addToOutputWindow(this, tr("Could not install from package %1 on device: %2").arg(filename, errorMessage));
}

void S60DeviceRunControlBase::launcherFinished()
{
    m_launcher->deleteLater();
    m_launcher = 0;
    handleLauncherFinished();
}

QMessageBox *S60DeviceRunControlBase::createTrkWaitingMessageBox(const QString &port, QWidget *parent)
{
    const QString title  = QCoreApplication::translate("Qt4ProjectManager::Internal::S60DeviceRunControlBase",
                                                       "Waiting for TRK");
    const QString text = QCoreApplication::translate("Qt4ProjectManager::Internal::S60DeviceRunControlBase",
                                                     "Please start TRK on %1.").arg(port);
    QMessageBox *rc = new QMessageBox(QMessageBox::Information, title, text,
                                      QMessageBox::Cancel, parent);
    return rc;
}

void S60DeviceRunControlBase::slotLauncherStateChanged(int s)
{
    if (s == trk::Launcher::WaitingForTrk) {
        QMessageBox *mb = S60DeviceRunControlBase::createTrkWaitingMessageBox(m_launcher->trkServerName(),
                                                     Core::ICore::instance()->mainWindow());
        connect(m_launcher, SIGNAL(stateChanged(int)), mb, SLOT(close()));
        connect(mb, SIGNAL(finished(int)), this, SLOT(slotWaitingForTrkClosed()));
        mb->open();
    }
}

void S60DeviceRunControlBase::slotWaitingForTrkClosed()
{
    if (m_launcher && m_launcher->state() == trk::Launcher::WaitingForTrk) {
        stop();
        error(this, tr("Canceled."));        
        emit finished();
    }
}

void S60DeviceRunControlBase::processFailed(const QString &program, QProcess::ProcessError errorCode)
{
    QString errorString;
    switch (errorCode) {
    case QProcess::FailedToStart:
        errorString = tr("Failed to start %1.");
        break;
    case QProcess::Crashed:
        errorString = tr("%1 has unexpectedly finished.");
        break;
    default:
        errorString = tr("An error has occurred while running %1.");
    }
    error(this, errorString.arg(program));
    stop();
    emit finished();
}

void S60DeviceRunControlBase::printApplicationOutput(const QString &output)
{
    emit addToOutputWindowInline(this, output);
}

bool S60DeviceRunControlBase::checkConfiguration(QString * /* errorMessage */,
                                                 QString * /* settingsCategory */,
                                                 QString * /* settingsPage */) const
{
    return true;
}

// =============== S60DeviceRunControl
S60DeviceRunControl::S60DeviceRunControl(const QSharedPointer<ProjectExplorer::RunConfiguration> &runConfiguration) :
    S60DeviceRunControlBase(runConfiguration)
{
}

void S60DeviceRunControl::initLauncher(const QString &executable, trk::Launcher *launcher)
{
     connect(launcher, SIGNAL(startingApplication()), this, SLOT(printStartingNotice()));
     connect(launcher, SIGNAL(applicationRunning(uint)), this, SLOT(printRunNotice(uint)));
     connect(launcher, SIGNAL(canNotRun(QString)), this, SLOT(printRunFailNotice(QString)));
     connect(launcher, SIGNAL(applicationOutputReceived(QString)), this, SLOT(printApplicationOutput(QString)));
     launcher->addStartupActions(trk::Launcher::ActionCopyInstallRun);
     launcher->setFileName(executable);
}

void S60DeviceRunControl::handleLauncherFinished()
{
     emit finished();
     emit addToOutputWindow(this, tr("Finished."));
 }

void S60DeviceRunControl::printStartingNotice()
{
    emit addToOutputWindow(this, tr("Starting application..."));
}

void S60DeviceRunControl::printRunNotice(uint pid)
{
    emit addToOutputWindow(this, tr("Application running with pid %1.").arg(pid));
}

void S60DeviceRunControl::printRunFailNotice(const QString &errorMessage) {
    emit addToOutputWindow(this, tr("Could not start application: %1").arg(errorMessage));
}

// ======== S60DeviceDebugRunControl

S60DeviceDebugRunControl::S60DeviceDebugRunControl(const QSharedPointer<ProjectExplorer::RunConfiguration> &runConfiguration) :
    S60DeviceRunControlBase(runConfiguration),
    m_startParams(new Debugger::DebuggerStartParameters)
{
    Debugger::DebuggerManager *dm = Debugger::DebuggerManager::instance();
    const QSharedPointer<S60DeviceRunConfiguration> rc = runConfiguration.objectCast<S60DeviceRunConfiguration>();
    QTC_ASSERT(dm && !rc.isNull(), return);

    connect(dm, SIGNAL(debuggingFinished()),
            this, SLOT(debuggingFinished()), Qt::QueuedConnection);
    connect(dm, SIGNAL(applicationOutputAvailable(QString)),
            this, SLOT(printApplicationOutput(QString)),
            Qt::QueuedConnection);

    m_startParams->remoteChannel = rc->serialPortName();
    m_startParams->remoteChannelType = rc->communicationType();
    m_startParams->startMode = Debugger::StartInternal;
    m_startParams->toolChainType = rc->toolChainType();
}

void S60DeviceDebugRunControl::stop()
{
    S60DeviceRunControlBase::stop();
    Debugger::DebuggerManager *dm = Debugger::DebuggerManager::instance();
    QTC_ASSERT(dm, return)
    if (dm->state() == Debugger::DebuggerNotReady)
        dm->exitDebugger();
}

S60DeviceDebugRunControl::~S60DeviceDebugRunControl()
{
}

void S60DeviceDebugRunControl::initLauncher(const QString &executable, trk::Launcher *launcher)
{
    // No setting an executable on the launcher causes it to deploy only
    m_startParams->executable = executable;
    // Prefer the '*.sym' file over the '.exe', which should exist at the same
    // location in debug builds
    const QSharedPointer<S60DeviceRunConfiguration> rc = runConfiguration().objectCast<S60DeviceRunConfiguration>();
    const QString localExecutableFileName = rc->localExecutableFileName();
    const int lastDotPos = localExecutableFileName.lastIndexOf(QLatin1Char('.'));
    if (lastDotPos != -1) {
        m_startParams->symbolFileName = localExecutableFileName.mid(0, lastDotPos) + QLatin1String(".sym");
        if (!QFileInfo(m_startParams->symbolFileName).isFile()) {
            m_startParams->symbolFileName.clear();
            emit addToOutputWindow(this, tr("Warning: Cannot locate the symbol file belonging to %1.").arg(localExecutableFileName));
        }
    }
    launcher->addStartupActions(trk::Launcher::ActionCopyInstall);
}

void S60DeviceDebugRunControl::handleLauncherFinished()
{
    emit addToOutputWindow(this, tr("Launching debugger..."));
    Debugger::DebuggerManager::instance()->startNewDebugger(m_startParams);
}

void S60DeviceDebugRunControl::debuggingFinished()
{
    emit addToOutputWindow(this, tr("Debugging finished."));
    emit finished();
}

bool S60DeviceDebugRunControl::checkConfiguration(QString *errorMessage,
                                                  QString *settingsCategory,
                                                  QString *settingsPage) const
{
    return Debugger::DebuggerManager::instance()->checkDebugConfiguration(m_startParams->toolChainType,
                                                                          errorMessage,
                                                                          settingsCategory,
                                                                          settingsPage);
}
