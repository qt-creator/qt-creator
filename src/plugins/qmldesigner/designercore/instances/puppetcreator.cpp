/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "puppetcreator.h"

#include <QProcess>
#include <QTemporaryDir>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QMessageBox>

#include <projectexplorer/kit.h>
#include <projectexplorer/toolchain.h>
#include <utils/environment.h>
#include <coreplugin/icore.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <coreplugin/icore.h>

#include "puppetbuildprogressdialog.h"

#include <QtDebug>

namespace QmlDesigner {

bool PuppetCreator::m_useOnlyFallbackPuppet = !qgetenv("USE_ONLY_FALLBACK_PUPPET").isEmpty();
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

PuppetCreator::PuppetCreator(ProjectExplorer::Kit *kit, const QString &qtCreatorVersion)
    : m_qtCreatorVersion(qtCreatorVersion),
      m_kit(kit),
      m_availablePuppetType(FallbackPuppet)
{
}

PuppetCreator::~PuppetCreator()
{
}

void PuppetCreator::createPuppetExecutableIfMissing(PuppetCreator::QmlPuppetVersion puppetVersion)
{
    if (puppetVersion == Qml1Puppet)
        createQml1PuppetExecutableIfMissing();
    else
        createQml2PuppetExecutableIfMissing();
}

QProcess *PuppetCreator::createPuppetProcess(PuppetCreator::QmlPuppetVersion puppetVersion, const QString &puppetMode, const QString &socketToken, QObject *handlerObject, const char *outputSlot, const char *finishSlot) const
{
    QString puppetPath;
    if (puppetVersion == Qml1Puppet)
        puppetPath = qmlpuppetPath(m_availablePuppetType);
     else
        puppetPath = qml2puppetPath(m_availablePuppetType);

    return puppetProcess(puppetPath,
                         puppetMode,
                         socketToken,
                         handlerObject,
                         outputSlot,
                         finishSlot);
}


QProcess *PuppetCreator::puppetProcess(const QString &puppetPath,
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
    bool fowardQmlpuppetOutput = !qgetenv("FORWARD_QMLPUPPET_OUTPUT").isEmpty();
    if (fowardQmlpuppetOutput) {
        puppetProcess->setProcessChannelMode(QProcess::MergedChannels);
        QObject::connect(puppetProcess, SIGNAL(readyRead()), handlerObject, outputSlot);
    }
    puppetProcess->start(puppetPath, QStringList() << socketToken << puppetMode << "-graphicssystem raster");

    if (!qgetenv("DEBUG_QML_PUPPET").isEmpty())
        QMessageBox::information(Core::ICore::dialogParent(),
                                 QStringLiteral("Puppet is starting ..."),
                                 QStringLiteral("You can now attach your debugger to the puppet."));

    return puppetProcess;
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
            qmakeArguments.append(QStringLiteral("DESTDIR=") + qmlpuppetDirectory(UserSpacePuppet));
#ifndef QT_DEBUG
            qmakeArguments.append(QStringLiteral("CONFIG+=release"));
#endif
            qmakeArguments.append(qmlPuppetProjectFilePath);
            buildSucceeded = startBuildProcess(buildDirectory.path(), qmakeCommand(), qmakeArguments);
            if (buildSucceeded) {
                progressDialog.show();
                buildSucceeded = startBuildProcess(buildDirectory.path(), buildCommand(), QStringList(), &progressDialog);
                progressDialog.hide();
            }
        } else {
            buildSucceeded = true;
        }
    }

    return buildSucceeded;
}

void PuppetCreator::createQml1PuppetExecutableIfMissing()
{
    if (!m_useOnlyFallbackPuppet && m_kit) {
        if (m_qml1PuppetForKitPuppetHash.contains(m_kit->id())) {
            m_availablePuppetType = m_qml1PuppetForKitPuppetHash.value(m_kit->id());
        } else if (checkQmlpuppetIsReady()) {
            m_availablePuppetType = UserSpacePuppet;
        } else {
            bool buildSucceeded = build(qmlpuppetProjectFile());
            if (buildSucceeded)
                m_availablePuppetType = UserSpacePuppet;
            else
                m_availablePuppetType = FallbackPuppet;
        }

        m_qml1PuppetForKitPuppetHash.insert(m_kit->id(), m_availablePuppetType);

    } else {
        m_availablePuppetType = FallbackPuppet;
    }
}

void PuppetCreator::createQml2PuppetExecutableIfMissing()
{
    if (!m_useOnlyFallbackPuppet && m_kit) {
        if (m_qml2PuppetForKitPuppetHash.contains(m_kit->id())) {
            m_availablePuppetType = m_qml2PuppetForKitPuppetHash.value(m_kit->id());
        } else if (checkQml2puppetIsReady()) {
            m_availablePuppetType = UserSpacePuppet;
        } else {
            bool buildSucceeded = build(qml2puppetProjectFile());
            if (buildSucceeded)
                m_availablePuppetType = UserSpacePuppet;
            else
                m_availablePuppetType = FallbackPuppet;
        }

        m_qml2PuppetForKitPuppetHash.insert(m_kit->id(), m_availablePuppetType);

    } else {
        m_availablePuppetType = FallbackPuppet;
    }
}

QString PuppetCreator::qmlpuppetDirectory(PuppetType puppetType) const
{

    if (puppetType == UserSpacePuppet)
        return Core::ICore::userResourcePath()
                + QStringLiteral("/qmlpuppet/")
                + QCoreApplication::applicationVersion() + QStringLiteral("/")
                + qtHash();


    return qmlpuppetFallbackDirectory();
}

QString PuppetCreator::qmlpuppetFallbackDirectory() const
{
    return QCoreApplication::applicationDirPath();
}

QString PuppetCreator::qml2puppetPath(PuppetType puppetType) const
{
    return qmlpuppetDirectory(puppetType) + QStringLiteral("/qml2puppet") + QStringLiteral(QTC_HOST_EXE_SUFFIX);
}

QString PuppetCreator::qmlpuppetPath(PuppetType puppetType) const
{
    return qmlpuppetDirectory(puppetType) + QStringLiteral("/qmlpuppet") + QStringLiteral(QTC_HOST_EXE_SUFFIX);
}

QProcessEnvironment PuppetCreator::processEnvironment() const
{
    Utils::Environment environment = Utils::Environment::systemEnvironment();
    m_kit->addToEnvironment(environment);
    environment.set("QML_BAD_GUI_RENDER_LOOP", "true");
    environment.set("QML_USE_MOCKUPS", "true");
    environment.set("QML_PUPPET_MODE", "true");

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

bool PuppetCreator::startBuildProcess(const QString &buildDirectoryPath,
                                      const QString &command,
                                      const QStringList &processArguments,
                                      PuppetBuildProgressDialog *progressDialog) const
{
    if (command.isEmpty())
        return false;

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.setProcessEnvironment(processEnvironment());
    process.setWorkingDirectory(buildDirectoryPath);
    process.start(command, processArguments);
    process.waitForStarted();
    while (process.waitForReadyRead(-1)) {
        QByteArray newOutput = process.readAllStandardOutput();
        if (progressDialog)
            progressDialog->newBuildOutput(newOutput);

        m_compileLog.append(newOutput);
    }

    process.waitForFinished();

    if (process.exitStatus() == QProcess::NormalExit || process.exitCode() == 0)
        return true;
    else
        return false;
}

QString PuppetCreator::puppetSourceDirectoryPath()
{
    return Core::ICore::resourcePath() + QStringLiteral("/qml/qmlpuppet");
}

QString PuppetCreator::qml2puppetProjectFile()
{
    return puppetSourceDirectoryPath() + QStringLiteral("/qml2puppet/qml2puppet.pro");
}

QString PuppetCreator::qmlpuppetProjectFile()
{
    return puppetSourceDirectoryPath() + QStringLiteral("/qmlpuppet/qmlpuppet.pro");
}

bool PuppetCreator::checkPuppetIsReady(const QString &puppetPath) const
{
    QFileInfo puppetFileInfo(puppetPath);

    return puppetFileInfo.exists() && puppetFileInfo.lastModified() > qtLastModified();
}

bool PuppetCreator::checkQml2puppetIsReady() const
{
    return checkPuppetIsReady(qml2puppetPath(UserSpacePuppet));
}

bool PuppetCreator::checkQmlpuppetIsReady() const
{
    return checkPuppetIsReady(qmlpuppetPath(UserSpacePuppet));
}

bool PuppetCreator::qtIsSupported() const
{
    QtSupport::BaseQtVersion *currentQtVersion = QtSupport::QtKitInformation::qtVersion(m_kit);

    if (currentQtVersion
            && currentQtVersion->isValid()
            && currentQtVersion->qtVersion() >= QtSupport::QtVersionNumber(5, 2, 0)
            && (currentQtVersion->type() == QLatin1String(QtSupport::Constants::DESKTOPQT)
                || currentQtVersion->type() == QLatin1String(QtSupport::Constants::SIMULATORQT)))
        return true;

    return false;
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
