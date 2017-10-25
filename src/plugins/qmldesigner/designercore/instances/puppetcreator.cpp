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

#include "puppetbuildprogressdialog.h"

#include <model.h>
#ifndef QMLDESIGNER_TEST
#include <qmldesignerplugin.h>
#endif

#include <nodeinstanceview.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakenodes.h>
#include <coreplugin/messagebox.h>
#include <coreplugin/icore.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/temporarydirectory.h>

#include <QProcess>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QLoggingCategory>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QThread>
#include <QSettings>

static Q_LOGGING_CATEGORY(puppetStart, "qtc.puppet.start")
static Q_LOGGING_CATEGORY(puppetBuild, "qtc.puppet.build")

namespace QmlDesigner {

class EventFilter : public QObject {

public:
    bool eventFilter(QObject *o, QEvent *event) final
    {
        if (event->type() == QEvent::MouseButtonPress
                || event->type() == QEvent::MouseButtonRelease
                || event->type() == QEvent::KeyPress
                || event->type() == QEvent::KeyRelease) {
            return true;
        }

        return QObject::eventFilter(o, event);
    }
};

QHash<Core::Id, PuppetCreator::PuppetType> PuppetCreator::m_qml2PuppetForKitPuppetHash;

QByteArray PuppetCreator::qtHash() const
{
    QtSupport::BaseQtVersion *currentQtVersion = QtSupport::QtKitInformation::qtVersion(m_kit);
    if (currentQtVersion) {
        return QCryptographicHash::hash(currentQtVersion->qmakeProperty("QT_INSTALL_DATA").toUtf8(),
                                        QCryptographicHash::Sha1)
                .toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    }

    return QByteArray();
}

QDateTime PuppetCreator::qtLastModified() const
{
    QtSupport::BaseQtVersion *currentQtVersion = QtSupport::QtKitInformation::qtVersion(m_kit);
    if (currentQtVersion)
        return QFileInfo(currentQtVersion->qmakeProperty("QT_INSTALL_LIBS")).lastModified();

    return QDateTime();
}

QDateTime PuppetCreator::puppetSourceLastModified() const
{
    const QString basePuppetSourcePath = puppetSourceDirectoryPath();

    const QStringList sourceDirectoryPaths = {
        basePuppetSourcePath + "/commands",
        basePuppetSourcePath + "/container",
        basePuppetSourcePath + "/instances",
        basePuppetSourcePath + "/interfaces",
        basePuppetSourcePath + "/types",
        basePuppetSourcePath + "/qmlpuppet",
        basePuppetSourcePath + "/qmlpuppet/instances",
        basePuppetSourcePath + "/qml2puppet",
        basePuppetSourcePath + "/qml2puppet/instances"
    };

    QDateTime lastModified;
    foreach (const QString directoryPath, sourceDirectoryPaths) {
        foreach (const QFileInfo fileEntry, QDir(directoryPath).entryInfoList()) {
            const QDateTime filePathLastModified = fileEntry.lastModified();
            if (lastModified < filePathLastModified)
                lastModified = filePathLastModified;
        }
    }

    return lastModified;
}

bool PuppetCreator::useOnlyFallbackPuppet() const
{
#ifndef QMLDESIGNER_TEST
    if (!m_kit || !m_kit->isValid())
        qWarning() << "Invalid kit for QML puppet";
    return m_designerSettings.value(DesignerSettingsKey::USE_ONLY_FALLBACK_PUPPET
                                    ).toBool() || m_kit == 0 || !m_kit->isValid();
#else
    return true;
#endif
}

QString PuppetCreator::getStyleConfigFileName() const
{
#ifndef QMLDESIGNER_TEST
    if (m_currentProject) {
        for (const QString &fileName : m_currentProject->files(ProjectExplorer::Project::SourceFiles)) {
            QFileInfo fileInfo(fileName);
            if (fileInfo.fileName() == "qtquickcontrols2.conf")
                return  fileName;
        }
    }
#endif
    return QString();
}

PuppetCreator::PuppetCreator(ProjectExplorer::Kit *kit,
                             ProjectExplorer::Project *project,
                             const Model *model)

    : m_kit(kit)
    , m_availablePuppetType(FallbackPuppet)
    , m_model(model)
#ifndef QMLDESIGNER_TEST
    , m_designerSettings(QmlDesignerPlugin::instance()->settings())
#endif
    , m_currentProject(project)
{
}

QProcess *PuppetCreator::createPuppetProcess(const QString &puppetMode,
                                             const QString &socketToken,
                                             QObject *handlerObject,
                                             const char *outputSlot,
                                             const char *finishSlot) const
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
    auto puppetProcess = new QProcess;
    puppetProcess->setObjectName(puppetMode);
    puppetProcess->setProcessEnvironment(processEnvironment());

    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, puppetProcess, &QProcess::kill);
    QObject::connect(puppetProcess, SIGNAL(finished(int,QProcess::ExitStatus)), handlerObject, finishSlot);

#ifndef QMLDESIGNER_TEST
    QString forwardOutput = m_designerSettings.value(DesignerSettingsKey::
        FORWARD_PUPPET_OUTPUT).toString();
#else
    QString forwardOutput("all");
#endif
    if (forwardOutput == puppetMode || forwardOutput == "all") {
        puppetProcess->setProcessChannelMode(QProcess::MergedChannels);
        QObject::connect(puppetProcess, SIGNAL(readyRead()), handlerObject, outputSlot);
    }
    puppetProcess->setWorkingDirectory(workingDirectory);
    puppetProcess->start(puppetPath, {socketToken, puppetMode, "-graphicssystem raster"});

#ifndef QMLDESIGNER_TEST
    QString debugPuppet = m_designerSettings.value(DesignerSettingsKey::
        DEBUG_PUPPET).toString();
#else
    QString debugPuppet("all");
#endif
    if (debugPuppet == puppetMode || debugPuppet == "all") {
        QMessageBox::information(Core::ICore::dialogParent(),
            QCoreApplication::translate("PuppetCreator", "Puppet is starting ..."),
            QCoreApplication::translate("PuppetCreator", "You can now attach your debugger to the %1 puppet with process id: %2.")
                                 .arg(puppetMode, QString::number(puppetProcess->processId())));
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
    progressDialog.setParent(Core::ICore::mainWindow());

    m_compileLog.clear();

    Utils::TemporaryDirectory buildDirectory("qml-puppet-build");

    bool buildSucceeded = false;


    /* Ensure the model dialog is shown and no events are delivered to the rest of Qt Creator. */
    EventFilter eventFilter;
    QCoreApplication::instance()->installEventFilter(&eventFilter);
    progressDialog.show();
    QCoreApplication::processEvents();
    QCoreApplication::instance()->removeEventFilter(&eventFilter);
    /* Now the modal dialog will block input to the rest of Qt Creator.
       We can call process events without risking a mode change. */

    if (qtIsSupported()) {
        if (buildDirectory.isValid()) {
            QStringList qmakeArguments;
            qmakeArguments.append("-r");
            qmakeArguments.append("-after");
            qmakeArguments.append("DESTDIR=" + qmlPuppetDirectory(UserSpacePuppet));
#ifdef QT_DEBUG
            qmakeArguments.append("CONFIG+=debug");
#else
            qmakeArguments.append("CONFIG+=release");
#endif
            qmakeArguments.append(qmlPuppetProjectFilePath);
            buildSucceeded = startBuildProcess(buildDirectory.path(), qmakeCommand(), qmakeArguments, &progressDialog);
            if (buildSucceeded) {
                progressDialog.show();
                QString buildingCommand = buildCommand();
                QStringList buildArguments;
                if (buildingCommand == "make") {
                    buildArguments.append("-j");
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
            if (checkPuppetIsReady(qml2PuppetPath(UserSpacePuppet))) {
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
    return Core::ICore::userResourcePath() + "/qmlpuppet/";
}

QString PuppetCreator::qmlPuppetToplevelBuildDirectory() const
{
#ifndef QMLDESIGNER_TEST
    QString puppetToplevelBuildDirectory = m_designerSettings.value(
                DesignerSettingsKey::PUPPET_TOPLEVEL_BUILD_DIRECTORY).toString();
    if (puppetToplevelBuildDirectory.isEmpty())
        return defaultPuppetToplevelBuildDirectory();
    return puppetToplevelBuildDirectory;
#else
    return QString();
#endif
}

QString PuppetCreator::qmlPuppetDirectory(PuppetType puppetType) const
{
    if (puppetType == UserSpacePuppet)
        return qmlPuppetToplevelBuildDirectory() + '/' + QCoreApplication::applicationVersion()
                + '/' + QString::fromLatin1(qtHash());

    return qmlPuppetFallbackDirectory();
}

QString PuppetCreator::defaultPuppetFallbackDirectory()
{
    if (Utils::HostOsInfo::isMacHost())
        return Core::ICore::libexecPath() + "/qmldesigner";
    else
        return Core::ICore::libexecPath();
}

QString PuppetCreator::qmlPuppetFallbackDirectory() const
{
#ifndef QMLDESIGNER_TEST
    QString puppetFallbackDirectory = m_designerSettings.value(
        DesignerSettingsKey::PUPPET_FALLBACK_DIRECTORY).toString();
    if (puppetFallbackDirectory.isEmpty())
        return defaultPuppetFallbackDirectory();
    return puppetFallbackDirectory;
#else
    return QString();
#endif
}

QString PuppetCreator::qml2PuppetPath(PuppetType puppetType) const
{
    return qmlPuppetDirectory(puppetType) + "/qml2puppet" + QTC_HOST_EXE_SUFFIX;
}

static void filterOutQtBaseImportPath(QStringList *stringList)
{
    Utils::erase(*stringList, [](const QString &string) {
        QDir dir(string);
        return dir.dirName() == "qml" && !dir.entryInfoList(QStringList("QtQuick.2"), QDir::Dirs).isEmpty();
    });
}

QProcessEnvironment PuppetCreator::processEnvironment() const
{
    static const QString pathSep = Utils::HostOsInfo::pathListSeparator();
    Utils::Environment environment = Utils::Environment::systemEnvironment();
    if (!useOnlyFallbackPuppet())
        m_kit->addToEnvironment(environment);
    const QtSupport::BaseQtVersion *qt = QtSupport::QtKitInformation::qtVersion(m_kit);
    if (QTC_GUARD(qt)) { // Kits without a Qt version should not have a puppet!
        // Update PATH to include QT_HOST_BINS
        const Utils::FileName qtBinPath = qt->binPath();
        environment.prependOrSetPath(qtBinPath.toString());
    }
    environment.set("QML_BAD_GUI_RENDER_LOOP", "true");
    environment.set("QML_USE_MOCKUPS", "true");
    environment.set("QML_PUPPET_MODE", "true");
    environment.set("QML_DISABLE_DISK_CACHE", "true");
    if (!environment.hasKey("QT_SCREEN_SCALE_FACTORS"))
        environment.set("QT_AUTO_SCREEN_SCALE_FACTOR", "1");

#ifndef QMLDESIGNER_TEST
    const QString controlsStyle = m_designerSettings.value(DesignerSettingsKey::
            CONTROLS_STYLE).toString();
#else
    const QString controlsStyle;
#endif
    if (!controlsStyle.isEmpty() && controlsStyle != "Default") {
        environment.set("QT_QUICK_CONTROLS_STYLE", controlsStyle);
        environment.set("QT_LABS_CONTROLS_STYLE", controlsStyle);
    }

#ifndef QMLDESIGNER_TEST
    environment.set("FORMEDITOR_DEVICE_PIXEL_RATIO", QString::number(QmlDesignerPlugin::formEditorDevicePixelRatio()));
#endif

    const QString styleConfigFileName = getStyleConfigFileName();

    /* QT_QUICK_CONTROLS_CONF is not supported for Qt Version < 5.8.1,
     * but we can manually at least set the correct style. */
    if (!styleConfigFileName.isEmpty()) {
        QSettings infiFile(styleConfigFileName, QSettings::IniFormat);
        environment.set("QT_QUICK_CONTROLS_STYLE", infiFile.value("Controls/Style", "Default").toString());
    }

    if (!m_qrcMapping.isEmpty()) {
        environment.set("QMLDESIGNER_RC_PATHS", m_qrcMapping);
    }
#ifndef QMLDESIGNER_TEST
    QmlDesignerPlugin::instance()->viewManager().nodeInstanceView()->emitCustomNotification("PuppetStatus", {}, {QVariant(m_qrcMapping)});
#endif

    QStringList importPaths = m_model->importPaths();

    /* For the fallback puppet we have to remove the path to the original qtbase plugins to avoid conflics */
    if (m_availablePuppetType == FallbackPuppet)
        filterOutQtBaseImportPath(&importPaths);

    if (!styleConfigFileName.isEmpty())
        environment.appendOrSet("QT_QUICK_CONTROLS_CONF", styleConfigFileName);

    if (m_currentProject) {
        auto qmakeProject = qobject_cast<QmakeProjectManager::QmakeProject *>(m_currentProject);
        if (qmakeProject) {
            QStringList designerImports = qmakeProject->rootProjectNode()->variableValue(QmakeProjectManager::Variable::QmlDesignerImportPath);
            importPaths.append(designerImports);
        }
    }

    if (m_availablePuppetType == FallbackPuppet)
        importPaths.prepend(QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath));

    environment.appendOrSet("QML2_IMPORT_PATH", importPaths.join(pathSep), pathSep);

    qCInfo(puppetStart) << Q_FUNC_INFO;
    qCInfo(puppetStart) << "Puppet qrc mapping" << m_qrcMapping;
    qCInfo(puppetStart) << "Puppet import paths:" << importPaths;
    qCInfo(puppetStart) << "Puppet environment:" << environment.toStringList();

    return environment.toProcessEnvironment();
}

QString PuppetCreator::buildCommand() const
{
    Utils::Environment environment = Utils::Environment::systemEnvironment();
    m_kit->addToEnvironment(environment);

    ProjectExplorer::ToolChain *toolChain
            = ProjectExplorer::ToolChainKitInformation::toolChain(m_kit,
                                                                  ProjectExplorer::Constants::CXX_LANGUAGE_ID);

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

    const QString errorOutputFilePath(buildDirectoryPath + "/build_error_output.txt");
    if (QFile::exists(errorOutputFilePath))
        QFile(errorOutputFilePath).remove();
    progressDialog->setErrorOutputFile(errorOutputFilePath);

    QProcess process;
    process.setStandardErrorFile(errorOutputFilePath);
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.setProcessEnvironment(processEnvironment());
    process.setWorkingDirectory(buildDirectoryPath);
    process.start(command, processArguments);
    if (!process.waitForStarted())
        return false;
    while (process.waitForReadyRead(100) || process.state() == QProcess::Running) {
        if (progressDialog->useFallbackPuppet())
            return false;

        QCoreApplication::processEvents(QEventLoop::ExcludeSocketNotifiers);

        QByteArray newOutput = process.readAllStandardOutput();
        if (!newOutput.isEmpty()) {
            progressDialog->newBuildOutput(newOutput);
            m_compileLog.append(QString::fromLatin1(newOutput));
        }
    }

    process.waitForFinished();

    qCInfo(puppetBuild) << Q_FUNC_INFO;
    qCInfo(puppetBuild) << m_compileLog;
    m_compileLog.clear();

    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0)
        return true;

    return false;
}

QString PuppetCreator::puppetSourceDirectoryPath()
{
    return Core::ICore::resourcePath() + "/qml/qmlpuppet";
}

QString PuppetCreator::qml2PuppetProjectFile()
{
    return puppetSourceDirectoryPath() + "/qml2puppet/qml2puppet.pro";
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
            && currentQtVersion->type() == QtSupport::Constants::DESKTOPQT;
}

} // namespace QmlDesigner
