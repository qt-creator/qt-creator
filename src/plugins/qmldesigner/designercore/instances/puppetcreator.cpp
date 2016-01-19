/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "puppetcreator.h"

#include <QProcess>
#include <QTemporaryDir>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QMessageBox>
#include <QThread>

#include <projectexplorer/kit.h>
#include <projectexplorer/toolchain.h>
#include <utils/environment.h>
#include <coreplugin/messagebox.h>
#include <coreplugin/icore.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <coreplugin/icore.h>


#include <qmldesignerplugin.h>
#include "puppetbuildprogressdialog.h"


namespace QmlDesigner {

QHash<Core::Id, PuppetCreator::PuppetType> PuppetCreator::m_qml2PuppetForKitPuppetHash;

QByteArray PuppetCreator::qtHash() const
{
    if (m_kit) {
        QtSupport::BaseQtVersion *currentQtVersion = QtSupport::QtKitInformation::qtVersion(m_kit);
        if (currentQtVersion)
            return QCryptographicHash::hash(currentQtVersion->qmakeProperty("QT_INSTALL_DATA").toUtf8(), QCryptographicHash::Sha1).toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    }

    return QByteArray();
}

QDateTime PuppetCreator::qtLastModified() const
{
    if (m_kit) {
        QtSupport::BaseQtVersion *currentQtVersion = QtSupport::QtKitInformation::qtVersion(m_kit);
        if (currentQtVersion)
            return QFileInfo(currentQtVersion->qmakeProperty("QT_INSTALL_LIBS")).lastModified();
    }

    return QDateTime();
}

QDateTime PuppetCreator::puppetSourceLastModified() const
{
    QString basePuppetSourcePath = puppetSourceDirectoryPath();
    QStringList sourceDirectoryPathes;
    QDateTime lastModified;

    sourceDirectoryPathes.append(basePuppetSourcePath + QStringLiteral("/commands"));
    sourceDirectoryPathes.append(basePuppetSourcePath + QStringLiteral("/container"));
    sourceDirectoryPathes.append(basePuppetSourcePath + QStringLiteral("/instances"));
    sourceDirectoryPathes.append(basePuppetSourcePath + QStringLiteral("/interfaces"));
    sourceDirectoryPathes.append(basePuppetSourcePath + QStringLiteral("/types"));
    sourceDirectoryPathes.append(basePuppetSourcePath + QStringLiteral("/qmlpuppet"));
    sourceDirectoryPathes.append(basePuppetSourcePath + QStringLiteral("/qmlpuppet/instances"));
    sourceDirectoryPathes.append(basePuppetSourcePath + QStringLiteral("/qml2puppet"));
    sourceDirectoryPathes.append(basePuppetSourcePath + QStringLiteral("/qml2puppet/instances"));

    foreach (const QString directoryPath, sourceDirectoryPathes) {
        foreach (const QFileInfo fileEntry, QDir(directoryPath).entryInfoList()) {
            QDateTime filePathLastModified = fileEntry.lastModified();
            if (lastModified < filePathLastModified)
                lastModified = filePathLastModified;
        }
    }

    return lastModified;
}

bool PuppetCreator::useOnlyFallbackPuppet() const
{
    return m_designerSettings.value(DesignerSettingsKey::USE_ONLY_FALLBACK_PUPPET
        ).toBool() || m_kit == 0 || !m_kit->isValid();
}

PuppetCreator::PuppetCreator(ProjectExplorer::Kit *kit, const QString &qtCreatorVersion, const Model *model)
    : m_qtCreatorVersion(qtCreatorVersion),
      m_kit(kit),
      m_availablePuppetType(FallbackPuppet),
      m_model(model),
      m_designerSettings(QmlDesignerPlugin::instance()->settings())
{
}

PuppetCreator::~PuppetCreator()
{
}

void PuppetCreator::createPuppetExecutableIfMissing()
{
    createQml2PuppetExecutableIfMissing();
}

QProcess *PuppetCreator::createPuppetProcess(const QString &puppetMode, const QString &socketToken, QObject *handlerObject, const char *outputSlot, const char *finishSlot) const
{
    return puppetProcess(qml2PuppetPath(m_availablePuppetType),
                         qmlPuppetDirectory(m_availablePuppetType),
                         puppetMode,
                         socketToken,
                         handlerObject,
                         outputSlot,
                         finishSlot);
}


QProcess *PuppetCreator::puppetProcess(const QString &puppetPath,
                                       const QString &workingDirectory,
                                       const QString &puppetMode,
                                       const QString &socketToken,
                                       QObject *handlerObject,
                                       const char *outputSlot,
                                       const char *finishSlot) const
{
    QProcess *puppetProcess = new QProcess;
    puppetProcess->setObjectName(puppetMode);
    puppetProcess->setProcessEnvironment(processEnvironment());

    QObject::connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), puppetProcess, SLOT(kill()));
    QObject::connect(puppetProcess, SIGNAL(finished(int,QProcess::ExitStatus)), handlerObject, finishSlot);

    QString forwardOutput = m_designerSettings.value(DesignerSettingsKey::
        FORWARD_PUPPET_OUTPUT).toString();
    if (forwardOutput == puppetMode || forwardOutput == QLatin1String("all")) {
        puppetProcess->setProcessChannelMode(QProcess::MergedChannels);
        QObject::connect(puppetProcess, SIGNAL(readyRead()), handlerObject, outputSlot);
    }
    puppetProcess->setWorkingDirectory(workingDirectory);
    puppetProcess->start(puppetPath, QStringList() << socketToken << puppetMode << QLatin1String("-graphicssystem raster"));

    QString debugPuppet = m_designerSettings.value(DesignerSettingsKey::
        DEBUG_PUPPET).toString();
    if (debugPuppet == puppetMode || debugPuppet == QLatin1String("all")) {
        QMessageBox::information(Core::ICore::dialogParent(),
            QStringLiteral("Puppet is starting ..."),
            QStringLiteral("You can now attach your debugger to the %1 puppet with process id: %2."
            ).arg(puppetMode, QString::number(puppetProcess->processId())));
    }

    return puppetProcess;
}

static QString idealProcessCount()
{
    int processCount = QThread::idealThreadCount() + 1;
    if (processCount < 1)
        processCount = 4;

    return QString::number(processCount);
}

bool PuppetCreator::build(const QString &qmlPuppetProjectFilePath) const
{
    PuppetBuildProgressDialog progressDialog;

    m_compileLog.clear();

    QTemporaryDir buildDirectory;

    bool buildSucceeded = false;

    if (qtIsSupported()) {
        if (buildDirectory.isValid()) {
            QStringList qmakeArguments;
            qmakeArguments.append(QStringLiteral("-r"));
            qmakeArguments.append(QStringLiteral("-after"));
            qmakeArguments.append(QStringLiteral("DESTDIR=") + qmlPuppetDirectory(UserSpacePuppet));
#ifdef QT_DEBUG
            qmakeArguments.append(QStringLiteral("CONFIG+=debug"));
#else
            qmakeArguments.append(QStringLiteral("CONFIG+=release"));
#endif
            qmakeArguments.append(qmlPuppetProjectFilePath);
            buildSucceeded = startBuildProcess(buildDirectory.path(), qmakeCommand(), qmakeArguments, &progressDialog);
            if (buildSucceeded) {
                progressDialog.show();
                QString buildingCommand = buildCommand();
                QStringList buildArguments;
                if (buildingCommand == QStringLiteral("make")) {
                    buildArguments.append(QStringLiteral("-j"));
                    buildArguments.append(idealProcessCount());
                }
                buildSucceeded = startBuildProcess(buildDirectory.path(), buildingCommand, buildArguments, &progressDialog);
            }

            if (!buildSucceeded) {
                progressDialog.setWindowTitle(QCoreApplication::translate("PuppetCreator", "QML Emulation Layer (QML Puppet) Building was Unsuccessful"));
                progressDialog.setErrorMessage(QCoreApplication::translate("PuppetCreator",
                                                                           "The QML emulation layer (QML Puppet) cannot be built. "
                                                                           "The fallback emulation layer, which does not support all features, will be used."
                                                                           ));
                // now we want to keep the dialog open
                progressDialog.exec();
            }
        }
    } else {
        Core::AsynchronousMessageBox::warning(QCoreApplication::translate("PuppetCreator", "Qt Version is not supported"),
                                               QCoreApplication::translate("PuppetCreator",
                                                                           "The QML emulation layer (QML Puppet) cannot be built because the Qt version is too old "
                                                                           "or it cannot run natively on your computer. "
                                                                           "The fallback emulation layer, which does not support all features, will be used."
                                                                           ));
    }

    return buildSucceeded;
}

static void warnAboutInvalidKit()
{
    Core::AsynchronousMessageBox::warning(QCoreApplication::translate("PuppetCreator", "Kit is invalid"),
                                           QCoreApplication::translate("PuppetCreator",
                                                                       "The QML emulation layer (QML Puppet) cannot be built because the kit is not configured correctly. "
                                                                       "For example the compiler can be misconfigured. "
                                                                       "Fix the kit configuration and restart Qt Creator. "
                                                                       "Otherwise, the fallback emulation layer, which does not support all features, will be used."
                                                                       ));
}

void PuppetCreator::createQml2PuppetExecutableIfMissing()
{
    m_availablePuppetType = FallbackPuppet;

    if (!useOnlyFallbackPuppet()) {
        // check if there was an already failing try to get the UserSpacePuppet
        // -> imagine as result a FallbackPuppet and nothing will happen again
        if (m_qml2PuppetForKitPuppetHash.value(m_kit->id(), UserSpacePuppet) == UserSpacePuppet ) {
            if (checkQml2PuppetIsReady()) {
                m_availablePuppetType = UserSpacePuppet;
            } else {
                if (m_kit->isValid()) {
                    bool buildSucceeded = build(qml2PuppetProjectFile());
                    if (buildSucceeded)
                        m_availablePuppetType = UserSpacePuppet;
                } else {
                    warnAboutInvalidKit();
                }
                m_qml2PuppetForKitPuppetHash.insert(m_kit->id(), m_availablePuppetType);
            }
        }
    }
}

QString PuppetCreator::defaultPuppetToplevelBuildDirectory()
{
    return Core::ICore::userResourcePath()
            + QStringLiteral("/qmlpuppet/");
}

QString PuppetCreator::qmlPuppetToplevelBuildDirectory() const
{
    QString puppetToplevelBuildDirectory = m_designerSettings.value(
        DesignerSettingsKey::PUPPET_TOPLEVEL_BUILD_DIRECTORY).toString();
    if (puppetToplevelBuildDirectory.isEmpty())
        return defaultPuppetToplevelBuildDirectory();
    return puppetToplevelBuildDirectory;
}

QString PuppetCreator::qmlPuppetDirectory(PuppetType puppetType) const
{
    if (puppetType == UserSpacePuppet)
        return qmlPuppetToplevelBuildDirectory() + QStringLiteral("/")
            + QCoreApplication::applicationVersion() + QStringLiteral("/") + QString::fromLatin1(qtHash());

    return qmlPuppetFallbackDirectory();
}

QString PuppetCreator::defaultPuppetFallbackDirectory()
{
    if (Utils::HostOsInfo::isMacHost())
        return Core::ICore::libexecPath() + QLatin1String("/qmldesigner");
    else
        return Core::ICore::libexecPath();
}

QString PuppetCreator::qmlPuppetFallbackDirectory() const
{
    QString puppetFallbackDirectory = m_designerSettings.value(
        DesignerSettingsKey::PUPPET_FALLBACK_DIRECTORY).toString();
    if (puppetFallbackDirectory.isEmpty())
        return defaultPuppetFallbackDirectory();
    return puppetFallbackDirectory;
}

QString PuppetCreator::qml2PuppetPath(PuppetType puppetType) const
{
    return qmlPuppetDirectory(puppetType) + QStringLiteral("/qml2puppet") + QStringLiteral(QTC_HOST_EXE_SUFFIX);
}

QProcessEnvironment PuppetCreator::processEnvironment() const
{
#if defined(Q_OS_WIN)
    static QLatin1String pathSep(";");
#else
    static QLatin1String pathSep(":");
#endif
    Utils::Environment environment = Utils::Environment::systemEnvironment();
    if (!useOnlyFallbackPuppet())
        m_kit->addToEnvironment(environment);
    environment.set(QLatin1String("QML_BAD_GUI_RENDER_LOOP"), QLatin1String("true"));
    environment.set(QLatin1String("QML_USE_MOCKUPS"), QLatin1String("true"));
    environment.set(QLatin1String("QML_PUPPET_MODE"), QLatin1String("true"));

    const QString controlsStyle = m_designerSettings.value(DesignerSettingsKey::
            CONTROLS_STYLE).toString();
    if (!controlsStyle.isEmpty())
        environment.set(QLatin1String("QT_QUICK_CONTROLS_STYLE"), controlsStyle);

    if (!m_qrcMapping.isEmpty()) {
        environment.set(QLatin1String("QMLDESIGNER_RC_PATHS"), m_qrcMapping);
    }

    if (m_availablePuppetType != FallbackPuppet) {
        environment.appendOrSet("QML2_IMPORT_PATH", m_model->importPaths().join(pathSep), pathSep);
    }
    return environment.toProcessEnvironment();
}

QString PuppetCreator::buildCommand() const
{
    Utils::Environment environment = Utils::Environment::systemEnvironment();
    m_kit->addToEnvironment(environment);

    ProjectExplorer::ToolChain *toolChain = ProjectExplorer::ToolChainKitInformation::toolChain(m_kit);

    if (toolChain)
        return toolChain->makeCommand(environment);

    return QString();
}

QString PuppetCreator::qmakeCommand() const
{
    QtSupport::BaseQtVersion *currentQtVersion = QtSupport::QtKitInformation::qtVersion(m_kit);
    if (currentQtVersion)
        return currentQtVersion->qmakeCommand().toString();

    return QString();
}

QString PuppetCreator::compileLog() const
{
    return m_compileLog;
}

void PuppetCreator::setQrcMappingString(const QString qrcMapping)
{
    m_qrcMapping = qrcMapping;
}

bool PuppetCreator::startBuildProcess(const QString &buildDirectoryPath,
                                      const QString &command,
                                      const QStringList &processArguments,
                                      PuppetBuildProgressDialog *progressDialog) const
{
    if (command.isEmpty())
        return false;

    const QString errorOutputFilePath(buildDirectoryPath + QLatin1String("/build_error_output.txt"));
    if (QFile::exists(errorOutputFilePath))
        QFile(errorOutputFilePath).remove();
    progressDialog->setErrorOutputFile(errorOutputFilePath);

    QProcess process;
    process.setStandardErrorFile(errorOutputFilePath);
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.setProcessEnvironment(processEnvironment());
    process.setWorkingDirectory(buildDirectoryPath);
    process.start(command, processArguments);
    process.waitForStarted();
    while (process.waitForReadyRead(100) || process.state() == QProcess::Running) {
        if (progressDialog->useFallbackPuppet())
            return false;

        QCoreApplication::processEvents(QEventLoop::ExcludeSocketNotifiers);

        QByteArray newOutput = process.readAllStandardOutput();
        if (!newOutput.isEmpty()) {
            if (progressDialog)
                progressDialog->newBuildOutput(newOutput);

            m_compileLog.append(QString::fromLatin1(newOutput));
        }
    }

    process.waitForFinished();

    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0)
        return true;

    return false;
}

QString PuppetCreator::puppetSourceDirectoryPath()
{
    return Core::ICore::resourcePath() + QStringLiteral("/qml/qmlpuppet");
}

QString PuppetCreator::qml2PuppetProjectFile()
{
    return puppetSourceDirectoryPath() + QStringLiteral("/qml2puppet/qml2puppet.pro");
}

QString PuppetCreator::qmlPuppetProjectFile()
{
    return puppetSourceDirectoryPath() + QStringLiteral("/qmlpuppet/qmlpuppet.pro");
}

bool PuppetCreator::checkPuppetIsReady(const QString &puppetPath) const
{
    QFileInfo puppetFileInfo(puppetPath);
    if (puppetFileInfo.exists()) {
        QDateTime puppetExecutableLastModified = puppetFileInfo.lastModified();

        return puppetExecutableLastModified > qtLastModified() && puppetExecutableLastModified > puppetSourceLastModified();
    }

    return false;
}

bool PuppetCreator::checkQml2PuppetIsReady() const
{
    return checkPuppetIsReady(qml2PuppetPath(UserSpacePuppet));
}

static bool nonEarlyQt5Version(const QtSupport::QtVersionNumber &currentQtVersionNumber)
{
    return currentQtVersionNumber >= QtSupport::QtVersionNumber(5, 2, 0) || currentQtVersionNumber < QtSupport::QtVersionNumber(5, 0, 0);
}

bool PuppetCreator::qtIsSupported() const
{
    QtSupport::BaseQtVersion *currentQtVersion = QtSupport::QtKitInformation::qtVersion(m_kit);

    return currentQtVersion
            && currentQtVersion->isValid()
            && nonEarlyQt5Version(currentQtVersion->qtVersion())
            && currentQtVersion->type() == QLatin1String(QtSupport::Constants::DESKTOPQT);
}

bool PuppetCreator::checkPuppetVersion(const QString &qmlPuppetPath)
{

    QProcess qmlPuppetVersionProcess;
    qmlPuppetVersionProcess.start(qmlPuppetPath, QStringList() << QLatin1String("--version"));
    qmlPuppetVersionProcess.waitForReadyRead(6000);

    QByteArray versionString = qmlPuppetVersionProcess.readAll();

    bool canConvert;
    unsigned int versionNumber = versionString.toUInt(&canConvert);

    return canConvert && versionNumber == 2;

}

} // namespace QmlDesigner
