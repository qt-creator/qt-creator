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

#include "qt4project.h"
#include "qtversionmanager.h"
#include "profilereader.h"
#include "s60manager.h"
#include "s60devices.h"
#include "serialdevicelister.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <utils/qtcassert.h>
#include <utils/pathchooser.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>

#include <QtGui/QRadioButton>

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
    m_serialPortName("COM5"),
    m_signingMode(SignSelf)
{
    if (!m_proFilePath.isEmpty())
        setName(tr("%1 on Device").arg(QFileInfo(m_proFilePath).completeBaseName()));
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
    return "Qt4ProjectManager.DeviceRunConfiguration";
}

bool S60DeviceRunConfiguration::isEnabled() const
{
    Qt4Project *pro = qobject_cast<Qt4Project*>(project());
    QTC_ASSERT(pro, return false);
    ToolChain::ToolChainType type = pro->toolChainType(pro->activeBuildConfiguration());
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
}

QString S60DeviceRunConfiguration::serialPortName() const
{
    return m_serialPortName;
}

void S60DeviceRunConfiguration::setSerialPortName(const QString &name)
{
    m_serialPortName = name.trimmed();
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

/* Grep the pkg file for \code
; Executable and default resource files
"/S60/devices/S60_3rd_FP2_SDK_v1.1/epoc32/release/gcce/udeb/foo.exe"    - "!:\sys\bin\foo.exe"
\endcode */

static QString executableFromPkgFile(const QString &pkgFileName, QString *errorMessage)
{
    QFile pkgFile(pkgFileName);
    if (!pkgFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
        *errorMessage = S60DeviceRunConfiguration::tr("Cannot open %1: %2").arg(pkgFileName, pkgFile.errorString());
        return QString();
    }
    // "<SDK>/foo.exe"    - "!:\device_bin\foo.exe"
    const QRegExp exePattern = QRegExp(QLatin1String("^\"([^\"]+\\.exe)\" +-.*$"));
    Q_ASSERT(exePattern.isValid());
    foreach(const QString &line, QString::fromLocal8Bit(pkgFile.readAll()).split(QLatin1Char('\n')))
        if (exePattern.exactMatch(line))
            return exePattern.cap(1);
    *errorMessage = S60DeviceRunConfiguration::tr("Unable to find the executable in the package file %1.").arg(pkgFileName);
    return QString();
}

QString S60DeviceRunConfiguration::executableFileName() const
{
    const QString pkg = packageFileName();
    if (!pkg.isEmpty()) {
        QString errorMessage;
        const QString rc = executableFromPkgFile(pkg, &errorMessage);
        if (rc.isEmpty())
            qWarning("%s\n", qPrintable(errorMessage));
        return rc;
    }
    return QString();
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
    QtVersion::QmakeBuildConfig projectBuildConfiguration = QtVersion::QmakeBuildConfig(pro->value(pro->activeBuildConfiguration(), "buildConfiguration").toInt());
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

    if (pro->toolChainType(pro->activeBuildConfiguration()) == ToolChain::GCCE)
        m_baseFileName += "_gcce";
    else
        m_baseFileName += "_armv5";
    if (projectBuildConfiguration & QtVersion::DebugBuild)
        m_baseFileName += "_udeb";
    else
        m_baseFileName += "_urel";

    delete reader;
    m_cachedTargetInformationValid = true;
    emit targetInformationChanged();
}

void S60DeviceRunConfiguration::invalidateCachedTargetInformation()
{
    m_cachedTargetInformationValid = false;
    emit targetInformationChanged();
}

// ======== S60DeviceRunConfigurationWidget

S60DeviceRunConfigurationWidget::S60DeviceRunConfigurationWidget(S60DeviceRunConfiguration *runConfiguration,
                                                                     QWidget *parent)
    : QWidget(parent),
    m_runConfiguration(runConfiguration)
{
    QVBoxLayout *mainBoxLayout = new QVBoxLayout();
    mainBoxLayout->setMargin(0);
    setLayout(mainBoxLayout);
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setMargin(0);
    mainBoxLayout->addLayout(formLayout);

    QLabel *nameLabel = new QLabel(tr("Name:"));
    m_nameLineEdit = new QLineEdit(m_runConfiguration->name());
    nameLabel->setBuddy(m_nameLineEdit);
    formLayout->addRow(nameLabel, m_nameLineEdit);

    m_sisxFileLabel = new QLabel(m_runConfiguration->basePackageFilePath() + ".sisx");
    formLayout->addRow(tr("Install File:"), m_sisxFileLabel);

    m_serialPorts = new QComboBox;
    updateSerialDevices();
    connect(S60Manager::instance()->serialDeviceLister(), SIGNAL(updated()), this, SLOT(updateSerialDevices()));
    connect(m_serialPorts, SIGNAL(activated(int)), this, SLOT(setSerialPort(int)));
    formLayout->addRow(tr("Device on Serial Port:"), m_serialPorts);

    QWidget *signatureWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout();
    signatureWidget->setLayout(layout);
    mainBoxLayout->addWidget(signatureWidget);
    QRadioButton *selfSign = new QRadioButton(tr("Self-signed certificate"));
    QHBoxLayout *customHBox = new QHBoxLayout();
    customHBox->setMargin(0);
    QVBoxLayout *radioLayout = new QVBoxLayout();
    QRadioButton *customSignature = new QRadioButton();
    radioLayout->addWidget(customSignature);
    radioLayout->addStretch(10);
    customHBox->addLayout(radioLayout);
    QFormLayout *customLayout = new QFormLayout();
    customLayout->setMargin(0);
    customLayout->setLabelAlignment(Qt::AlignRight);
    Core::Utils::PathChooser *signaturePath = new Core::Utils::PathChooser();
    signaturePath->setExpectedKind(Core::Utils::PathChooser::File);
    signaturePath->setPromptDialogTitle(tr("Choose certificate file (.cer)"));
    customLayout->addRow(new QLabel(tr("Custom certificate:")), signaturePath);
    Core::Utils::PathChooser *keyPath = new Core::Utils::PathChooser();
    keyPath->setExpectedKind(Core::Utils::PathChooser::File);
    keyPath->setPromptDialogTitle(tr("Choose key file (.key / .pem)"));
    customLayout->addRow(new QLabel(tr("Key file:")), keyPath);
    customHBox->addLayout(customLayout);
    customHBox->addStretch(10);
    layout->addWidget(selfSign);
    layout->addLayout(customHBox);
    layout->addStretch(10);

    switch (m_runConfiguration->signingMode()) {
    case S60DeviceRunConfiguration::SignSelf:
        selfSign->setChecked(true);
        break;
    case S60DeviceRunConfiguration::SignCustom:
        customSignature->setChecked(true);
        break;
    }

    signaturePath->setPath(m_runConfiguration->customSignaturePath());
    keyPath->setPath(m_runConfiguration->customKeyPath());

    connect(m_nameLineEdit, SIGNAL(textEdited(QString)),
        this, SLOT(nameEdited(QString)));
    connect(m_runConfiguration, SIGNAL(targetInformationChanged()),
            this, SLOT(updateTargetInformation()));
    connect(selfSign, SIGNAL(toggled(bool)), this, SLOT(selfSignToggled(bool)));
    connect(customSignature, SIGNAL(toggled(bool)), this, SLOT(customSignatureToggled(bool)));
    connect(signaturePath, SIGNAL(changed(QString)), this, SLOT(signaturePathChanged(QString)));
    connect(keyPath, SIGNAL(changed(QString)), this, SLOT(keyPathChanged(QString)));
}

void S60DeviceRunConfigurationWidget::updateSerialDevices()
{
    m_serialPorts->clear();
    QString runConfigurationPortName = m_runConfiguration->serialPortName();
    QList<SerialDeviceLister::SerialDevice> serialDevices = S60Manager::instance()->serialDeviceLister()->serialDevices();
    for (int i = 0; i < serialDevices.size(); ++i) {
        const SerialDeviceLister::SerialDevice &device = serialDevices.at(i);
        m_serialPorts->addItem(device.friendlyName, device.portName);
        if (device.portName == runConfigurationPortName)
            m_serialPorts->setCurrentIndex(i);
    }
    QString selectedPortName = m_serialPorts->itemData(m_serialPorts->currentIndex()).toString();
    if (m_serialPorts->count() > 0 && runConfigurationPortName != selectedPortName)
        m_runConfiguration->setSerialPortName(selectedPortName);
}

void S60DeviceRunConfigurationWidget::nameEdited(const QString &text)
{
    m_runConfiguration->setName(text);
}

void S60DeviceRunConfigurationWidget::updateTargetInformation()
{
    m_sisxFileLabel->setText(m_runConfiguration->basePackageFilePath() + ".sisx");
}

void S60DeviceRunConfigurationWidget::setSerialPort(int index)
{
    m_runConfiguration->setSerialPortName(m_serialPorts->itemData(index).toString());
}

void S60DeviceRunConfigurationWidget::selfSignToggled(bool toggle)
{
    if (toggle)
        m_runConfiguration->setSigningMode(S60DeviceRunConfiguration::SignSelf);
}

void S60DeviceRunConfigurationWidget::customSignatureToggled(bool toggle)
{
    if (toggle)
        m_runConfiguration->setSigningMode(S60DeviceRunConfiguration::SignCustom);
}

void S60DeviceRunConfigurationWidget::signaturePathChanged(const QString &path)
{
    m_runConfiguration->setCustomSignaturePath(path);
}

void S60DeviceRunConfigurationWidget::keyPathChanged(const QString &path)
{
    m_runConfiguration->setCustomKeyPath(path);
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
            applicationProFiles.append("QtS60DeviceRunConfiguration." + node->path());
        }
        return applicationProFiles;
    } else {
        return QStringList();
    }
}

QString S60DeviceRunConfigurationFactory::displayNameForType(const QString &type) const
{
    QString fileName = type.mid(QString("QtS60DeviceRunConfiguration.").size());
    return tr("%1 on Device").arg(QFileInfo(fileName).completeBaseName());
}

QSharedPointer<RunConfiguration> S60DeviceRunConfigurationFactory::create(Project *project, const QString &type)
{
    Qt4Project *p = qobject_cast<Qt4Project *>(project);
    Q_ASSERT(p);
    if (type.startsWith("QtS60DeviceRunConfiguration.")) {
        QString fileName = type.mid(QString("QtS60DeviceRunConfiguration.").size());
        return QSharedPointer<RunConfiguration>(new S60DeviceRunConfiguration(p, fileName));
    }
    Q_ASSERT(type == "Qt4ProjectManager.DeviceRunConfiguration");
    // The right path is set in restoreSettings
    QSharedPointer<RunConfiguration> rc(new S60DeviceRunConfiguration(p, QString::null));
    return rc;
}

// ======== S60DeviceRunControlFactory

S60DeviceRunControlFactory::S60DeviceRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

bool S60DeviceRunControlFactory::canRun(const QSharedPointer<RunConfiguration> &runConfiguration, const QString &mode) const
{
    return (mode == ProjectExplorer::Constants::RUNMODE)
            && (!runConfiguration.objectCast<S60DeviceRunConfiguration>().isNull());
}

RunControl* S60DeviceRunControlFactory::create(const QSharedPointer<RunConfiguration> &runConfiguration, const QString &mode)
{
    QSharedPointer<S60DeviceRunConfiguration> rc = runConfiguration.objectCast<S60DeviceRunConfiguration>();
    QTC_ASSERT(!rc.isNull() && mode == ProjectExplorer::Constants::RUNMODE, return 0);
    return new S60DeviceRunControl(rc);
}

QString S60DeviceRunControlFactory::displayName() const
{
    return tr("Run on Device");
}

QWidget *S60DeviceRunControlFactory::configurationWidget(const QSharedPointer<ProjectExplorer::RunConfiguration>  & /* runConfiguration */)
{
    return 0;
}

// ======== S60DeviceRunControl

S60DeviceRunControl::S60DeviceRunControl(const QSharedPointer<RunConfiguration> &runConfiguration)
    : RunControl(runConfiguration), m_launcher(0)
{
    m_makesis = new QProcess(this);
    connect(m_makesis, SIGNAL(readyReadStandardError()),
            this, SLOT(readStandardError()));
    connect(m_makesis, SIGNAL(readyReadStandardOutput()),
            this, SLOT(readStandardOutput()));
    connect(m_makesis, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(makesisProcessFailed()));
    connect(m_makesis, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(makesisProcessFinished()));
    m_signsis = new QProcess(this);
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
    m_targetName = s60runConfig->targetName();
    m_baseFileName = s60runConfig->basePackageFilePath();
    m_workingDirectory = QFileInfo(m_baseFileName).absolutePath();
    m_qtDir = project->qtVersion(project->activeBuildConfiguration())->versionInfo().value("QT_INSTALL_DATA");
    m_useCustomSignature = (s60runConfig->signingMode() == S60DeviceRunConfiguration::SignCustom);
    m_customSignaturePath = s60runConfig->customSignaturePath();
    m_customKeyPath = s60runConfig->customKeyPath();
    m_toolsDirectory = S60Manager::instance()->deviceForQtVersion(
            project->qtVersion(project->activeBuildConfiguration())).toolsRoot
            + "/epoc32/tools";
    m_executableFileName = lsFile(s60runConfig->executableFileName());
    m_makesisTool = m_toolsDirectory + "/makesis.exe";
    m_packageFile = QFileInfo(s60runConfig->packageFileName()).fileName();
}

void S60DeviceRunControl::start()
{
    emit started();

    emit addToOutputWindow(this, tr("Creating %1.sisx ...").arg(QDir::toNativeSeparators(m_baseFileName)));
    emit addToOutputWindow(this, tr("Executable file: %1").arg(m_executableFileName));


    m_makesis->setWorkingDirectory(m_workingDirectory);
    emit addToOutputWindow(this, tr("%1 %2").arg(QDir::toNativeSeparators(m_makesisTool), m_packageFile));
    m_makesis->start(m_makesisTool, QStringList(m_packageFile), QIODevice::ReadOnly);
}

void S60DeviceRunControl::stop()
{
    m_makesis->kill();
    m_signsis->kill();
    if (m_launcher)
        m_launcher->terminate();
}

bool S60DeviceRunControl::isRunning() const
{
    return m_makesis->state() != QProcess::NotRunning;
}

void S60DeviceRunControl::readStandardError()
{
    QProcess *process = static_cast<QProcess *>(sender());
    QByteArray data = process->readAllStandardError();
    emit addToOutputWindowInline(this, QString::fromLocal8Bit(data.constData(), data.length()));
}

void S60DeviceRunControl::readStandardOutput()
{
    QProcess *process = static_cast<QProcess *>(sender());
    QByteArray data = process->readAllStandardOutput();
    emit addToOutputWindowInline(this, QString::fromLocal8Bit(data.constData(), data.length()));
}

void S60DeviceRunControl::makesisProcessFailed()
{
    processFailed("makesis.exe", m_makesis->error());
}

void S60DeviceRunControl::makesisProcessFinished()
{
    if (m_makesis->exitCode() != 0) {
        error(this, tr("An error occurred while creating the package."));
        emit finished();
        return;
    }
    QString signsisTool = m_toolsDirectory + "/signsis.exe";
    QString sisFile = QFileInfo(m_baseFileName + ".sis").fileName();
    QString sisxFile = QFileInfo(m_baseFileName + ".sisx").fileName();
    QString signature = (m_useCustomSignature ? m_customSignaturePath
                         : m_qtDir + "/selfsigned.cer");
    QString key = (m_useCustomSignature ? m_customKeyPath
                         : m_qtDir + "/selfsigned.key");
    QStringList arguments;
    arguments << sisFile
            << sisxFile << QDir::toNativeSeparators(signature)
            << QDir::toNativeSeparators(key);
    m_signsis->setWorkingDirectory(m_workingDirectory);
    emit addToOutputWindow(this, tr("%1 %2").arg(QDir::toNativeSeparators(signsisTool), arguments.join(QString(QLatin1Char(' ')))));
    m_signsis->start(signsisTool, arguments, QIODevice::ReadOnly);
}

void S60DeviceRunControl::signsisProcessFailed()
{
    processFailed("signsis.exe", m_signsis->error());
}

void S60DeviceRunControl::signsisProcessFinished()
{
    if (m_signsis->exitCode() != 0) {
        error(this, tr("An error occurred while creating the package."));
        emit finished();
        return;
    }
    m_launcher = new trk::Launcher;
    connect(m_launcher, SIGNAL(finished()), this, SLOT(runFinished()));
    connect(m_launcher, SIGNAL(copyingStarted()), this, SLOT(printCopyingNotice()));
    connect(m_launcher, SIGNAL(installingStarted()), this, SLOT(printInstallingNotice()));
    connect(m_launcher, SIGNAL(startingApplication()), this, SLOT(printStartingNotice()));
    connect(m_launcher, SIGNAL(applicationRunning(uint)), this, SLOT(printRunNotice(uint)));
    connect(m_launcher, SIGNAL(applicationOutputReceived(QString)), this, SLOT(printApplicationOutput(QString)));
    connect(m_launcher, SIGNAL(copyProgress(int)), this, SLOT(printCopyProgress(int)));

    //TODO sisx destination and file path user definable
    m_launcher->setTrkServerName(m_serialPortName);
    const QString copySrc(m_baseFileName + ".sisx");
    const QString copyDst = QString("C:\\Data\\%1.sisx").arg(QFileInfo(m_baseFileName).fileName());
    const QString runFileName = QString("C:\\sys\\bin\\%1.exe").arg(m_targetName);
    m_launcher->setCopyFileName(copySrc, copyDst);
    m_launcher->setInstallFileName(copyDst);
    m_launcher->setFileName(runFileName);
    emit addToOutputWindow(this, tr("Package: %1\nDeploying application to '%2'...").arg(lsFile(copySrc), m_serialPortFriendlyName));
    QString errorMessage;
    if (!m_launcher->startServer(&errorMessage)) {
        delete m_launcher;
        m_launcher = 0;
        error(this, tr("Could not connect to phone on port '%1': %2\n"
                       "Check if the phone is connected and the TRK application is running.").arg(m_serialPortName, errorMessage));
        emit finished();
    }
}

void S60DeviceRunControl::printCopyingNotice()
{
    emit addToOutputWindow(this, tr("Copying install file..."));
    emit addToOutputWindow(this, tr("0% copied."));
}

void S60DeviceRunControl::printCopyProgress(int progress)
{
    emit addToOutputWindow(this, tr("%1% copied.").arg(progress));
}

void S60DeviceRunControl::printInstallingNotice()
{
    emit addToOutputWindow(this, tr("Installing application..."));
}

void S60DeviceRunControl::printStartingNotice()
{
    emit addToOutputWindow(this, tr("Starting application..."));
}

void S60DeviceRunControl::printRunNotice(uint pid)
{
    emit addToOutputWindow(this, tr("Application running with pid %1.").arg(pid));
}

void S60DeviceRunControl::printApplicationOutput(const QString &output)
{
    emit addToOutputWindowInline(this, output);
}

void S60DeviceRunControl::runFinished()
{
    m_launcher->deleteLater();
    m_launcher = 0;
    emit addToOutputWindow(this, tr("Finished."));
    emit finished();
}

void S60DeviceRunControl::processFailed(const QString &program, QProcess::ProcessError errorCode)
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
    emit finished();
}
