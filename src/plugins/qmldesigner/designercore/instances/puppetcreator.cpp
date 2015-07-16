/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

QHash<Core::Id, PuppetCreator::PuppetType> PuppetCreator::m_qml1PuppetForKitPuppetHash;
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
    DesignerSettings settings = QmlDesignerPlugin::instance()->settings();
    return settings.useOnlyFallbackPuppet
            || !qgetenv("USE_ONLY_FALLBACK_PUPPET").isEmpty() || m_kit == 0 || !m_kit->isValid();
}

PuppetCreator::PuppetCreator(ProjectExplorer::Kit *kit, const QString &qtCreatorVersion, const Model *model, QmlPuppetVersion puppetVersion)
    : m_qtCreatorVersion(qtCreatorVersion),
      m_kit(kit),
      m_availablePuppetType(FallbackPuppet),
      m_model(model),
      m_designerSettings(QmlDesignerPlugin::instance()->settings()),
      m_puppetVersion(puppetVersion)
{
}

PuppetCreator::~PuppetCreator()
{
}

void PuppetCreator::createPuppetExecutableIfMissing()
{
    if (m_puppetVersion == Qml1Puppet)
        createQml1PuppetExecutableIfMissing();
    else
        createQml2PuppetExecutableIfMissing();
}

QProcess *PuppetCreator::createPuppetProcess(const QString &puppetMode, const QString &socketToken, QObject *handlerObject, const char *outputSlot, const char *finishSlot) const
{
    QString puppetPath;
    if (m_puppetVersion == Qml1Puppet)
        puppetPath = qmlPuppetPath(m_availablePuppetType);
     else
        puppetPath = qml2PuppetPath(m_availablePuppetType);

    const QString workingDirectory = qmlPuppetDirectory(m_availablePuppetType);

    return puppetProcess(puppetPath,
                         workingDirectory,
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
    QString forwardOutputMode = qgetenv("FORWARD_QML_PUPPET_OUTPUT").toLower();
    bool fowardQmlpuppetOutput = forwardOutputMode == puppetMode || forwardOutputMode == "true";
    if (fowardQmlpuppetOutput) {
        puppetProcess->setProcessChannelMode(QProcess::MergedChannels);
        QObject::connect(puppetProcess, SIGNAL(readyRead()), handlerObject, outputSlot);
    }
    puppetProcess->setWorkingDirectory(workingDirectory);
    puppetProcess->start(puppetPath, QStringList() << socketToken << puppetMode << "-graphicssystem raster");

    if (!qgetenv("DEBUG_QML_PUPPET").isEmpty())
        QMessageBox::information(Core::ICore::dialogParent(),
                                 QStringLiteral("Puppet is starting ..."),
                                 QStringLiteral("You can now attach your debugger to the %1 puppet with process id: %2.").arg(
                                     puppetMode, QString::number(puppetProcess->processId())));

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

void PuppetCreator::createQml1PuppetExecutableIfMissing()
{
    m_availablePuppetType = FallbackPuppet;

    if (!useOnlyFallbackPuppet()) {
        if (m_qml1PuppetForKitPuppetHash.contains(m_kit->id())) {
            m_availablePuppetType = m_qml1PuppetForKitPuppetHash.value(m_kit->id());
        } else if (checkQmlpuppetIsReady()) {
            m_availablePuppetType = UserSpacePuppet;
        } else {
            if (m_kit->isValid()) {
                bool buildSucceeded = build(qmlPuppetProjectFile());
                if (buildSucceeded)
                    m_availablePuppetType = UserSpacePuppet;
            } else {
                warnAboutInvalidKit();
            }
            m_qml1PuppetForKitPuppetHash.insert(m_kit->id(), m_availablePuppetType);
        }
    }
}

void PuppetCreator::createQml2PuppetExecutableIfMissing()
{
    m_availablePuppetType = FallbackPuppet;

    if (!useOnlyFallbackPuppet()) {
        if (m_qml2PuppetForKitPuppetHash.contains(m_kit->id())) {
            m_availablePuppetType = m_qml2PuppetForKitPuppetHash.value(m_kit->id());
        } else if (checkQml2PuppetIsReady()) {
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

QString PuppetCreator::defaultPuppetToplevelBuildDirectory()
{
    return Core::ICore::userResourcePath()
            + QStringLiteral("/qmlpuppet/");
}

QString PuppetCreator::qmlPuppetToplevelBuildDirectory() const
{
    if (m_designerSettings.puppetToplevelBuildDirectory.isEmpty())
        return defaultPuppetToplevelBuildDirectory();
    return QmlDesignerPlugin::instance()->settings().puppetToplevelBuildDirectory;
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
    return Core::ICore::libexecPath();
}

QString PuppetCreator::qmlPuppetFallbackDirectory() const
{
    if (m_designerSettings.puppetFallbackDirectory.isEmpty())
        return defaultPuppetFallbackDirectory();
    return QmlDesignerPlugin::instance()->settings().puppetFallbackDirectory;
}

QString PuppetCreator::qml2PuppetPath(PuppetType puppetType) const
{
    return qmlPuppetDirectory(puppetType) + QStringLiteral("/qml2puppet") + QStringLiteral(QTC_HOST_EXE_SUFFIX);
}

QString PuppetCreator::qmlPuppetPath(PuppetType puppetType) const
{
    return qmlPuppetDirectory(puppetType) + QStringLiteral("/qmlpuppet") + QStringLiteral(QTC_HOST_EXE_SUFFIX);
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
    environment.set("QML_BAD_GUI_RENDER_LOOP", "true");
    environment.set("QML_USE_MOCKUPS", "true");
    environment.set("QML_PUPPET_MODE", "true");

    const QString controlsStyle = QmlDesignerPlugin::instance()->settings().controlsStyle;
    if (!controlsStyle.isEmpty())
        environment.set(QLatin1String("QT_QUICK_CONTROLS_STYLE"), controlsStyle);

    if (!m_qrcMapping.isEmpty()) {
        environment.set(QLatin1String("QMLDESIGNER_RC_PATHS"), m_qrcMapping);
    }

    if (m_availablePuppetType != FallbackPuppet) {
        if (m_puppetVersion == Qml1Puppet)
            environment.appendOrSet("QML_IMPORT_PATH", m_model->importPaths().join(pathSep), pathSep);
        else
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

bool PuppetCreator::checkQmlpuppetIsReady() const
{
    return checkPuppetIsReady(qmlPuppetPath(UserSpacePuppet));
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
    qmlPuppetVersionProcess.start(qmlPuppetPath, QStringList() << "--version");
    qmlPuppetVersionProcess.waitForReadyRead(6000);

    QByteArray versionString = qmlPuppetVersionProcess.readAll();

    bool canConvert;
    unsigned int versionNumber = versionString.toUInt(&canConvert);

    return canConvert && versionNumber == 2;

}

} // namespace QmlDesigner
