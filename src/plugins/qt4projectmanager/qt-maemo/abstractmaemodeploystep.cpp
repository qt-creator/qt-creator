/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "abstractmaemodeploystep.h"

#include "maemoconstants.h"
#include "maemodeploystepwidget.h"
#include "maemoglobal.h"
#include "maemopackagecreationstep.h"
#include "maemopertargetdeviceconfigurationlistmodel.h"
#include "maemoqemumanager.h"
#include "qt4maemodeployconfiguration.h"

#include <utils/ssh/sshconnection.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>

#include <utils/ssh/sshconnectionmanager.h>

#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>

#define ASSERT_STATE(state) ASSERT_STATE_GENERIC(BaseState, state, m_baseState)

using namespace Core;
using namespace Utils;
using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {
namespace {

class MaemoDeployEventHandler : public QObject
{
    Q_OBJECT
public:
    MaemoDeployEventHandler(AbstractMaemoDeployStep *deployStep,
        QFutureInterface<bool> &future);

private slots:
    void handleDeployingDone();
    void handleDeployingFailed();
    void checkForCanceled();

private:
    AbstractMaemoDeployStep * const m_deployStep;
    const QFutureInterface<bool> m_future;
    QEventLoop * const m_eventLoop;
    bool m_error;
};

} // anonymous namespace


AbstractMaemoDeployStep::AbstractMaemoDeployStep(BuildStepList *parent,
    const QString &id) : BuildStep(parent, id)
{
    baseCtor();
}

AbstractMaemoDeployStep::AbstractMaemoDeployStep(BuildStepList *parent,
    AbstractMaemoDeployStep *other)
    : BuildStep(parent, other), m_lastDeployed(other->m_lastDeployed)
{
    baseCtor();
}

AbstractMaemoDeployStep::~AbstractMaemoDeployStep() { }

void AbstractMaemoDeployStep::baseCtor()
{
    m_baseState = BaseInactive;
    m_deviceConfig = maemoDeployConfig()->deviceConfigModel()->defaultDeviceConfig();
    connect(maemoDeployConfig()->deviceConfigModel(), SIGNAL(updated()),
        SLOT(handleDeviceConfigurationsUpdated()));
}

void AbstractMaemoDeployStep::run(QFutureInterface<bool> &fi)
{
    // Move to GUI thread.
    QTimer::singleShot(0, this, SLOT(start()));

    MaemoDeployEventHandler eventHandler(this, fi);
}

BuildStepConfigWidget *AbstractMaemoDeployStep::createConfigWidget()
{
    return new MaemoDeployStepWidget(this);
}

QVariantMap AbstractMaemoDeployStep::toMap() const
{
    QVariantMap map(BuildStep::toMap());
    addDeployTimesToMap(map);
    map.insert(DeviceIdKey,
        MaemoDeviceConfigurations::instance()->internalId(m_deviceConfig));
    return map;
}

void AbstractMaemoDeployStep::addDeployTimesToMap(QVariantMap &map) const
{
    QVariantList hostList;
    QVariantList fileList;
    QVariantList remotePathList;
    QVariantList timeList;
    typedef QHash<DeployablePerHost, QDateTime>::ConstIterator DepIt;
    for (DepIt it = m_lastDeployed.begin(); it != m_lastDeployed.end(); ++it) {
        fileList << it.key().first.localFilePath;
        remotePathList << it.key().first.remoteDir;
        hostList << it.key().second;
        timeList << it.value();
    }
    map.insert(LastDeployedHostsKey, hostList);
    map.insert(LastDeployedFilesKey, fileList);
    map.insert(LastDeployedRemotePathsKey, remotePathList);
    map.insert(LastDeployedTimesKey, timeList);
}

bool AbstractMaemoDeployStep::fromMap(const QVariantMap &map)
{
    if (!BuildStep::fromMap(map))
        return false;
    getDeployTimesFromMap(map);
    setDeviceConfig(map.value(DeviceIdKey, MaemoDeviceConfig::InvalidId).toULongLong());
    return true;
}

void AbstractMaemoDeployStep::getDeployTimesFromMap(const QVariantMap &map)
{
    const QVariantList &hostList = map.value(LastDeployedHostsKey).toList();
    const QVariantList &fileList = map.value(LastDeployedFilesKey).toList();
    const QVariantList &remotePathList
        = map.value(LastDeployedRemotePathsKey).toList();
    const QVariantList &timeList = map.value(LastDeployedTimesKey).toList();
    const int elemCount
        = qMin(qMin(hostList.size(), fileList.size()),
            qMin(remotePathList.size(), timeList.size()));
    for (int i = 0; i < elemCount; ++i) {
        const MaemoDeployable d(fileList.at(i).toString(),
            remotePathList.at(i).toString());
        m_lastDeployed.insert(DeployablePerHost(d, hostList.at(i).toString()),
            timeList.at(i).toDateTime());
    }
}

const AbstractMaemoPackageCreationStep *AbstractMaemoDeployStep::packagingStep() const
{
    return MaemoGlobal::earlierBuildStep<AbstractMaemoPackageCreationStep>(maemoDeployConfig(), this);
}

void AbstractMaemoDeployStep::raiseError(const QString &errorString)
{
    emit addTask(Task(Task::Error, errorString, QString(), -1,
        ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    m_hasError = true;
    emit error();
}

void AbstractMaemoDeployStep::writeOutput(const QString &text, OutputFormat format,
    OutputNewlineSetting newlineSetting)
{
    emit addOutput(text, format, newlineSetting);
}

void AbstractMaemoDeployStep::stop()
{
    if (m_baseState == StopRequested || m_baseState == BaseInactive)
        return;

    writeOutput(tr("Operation canceled by user, cleaning up..."));
    const BaseState oldState = m_baseState;
    setBaseState(StopRequested);
    switch (oldState) {
    case Connecting:
        m_connection->disconnectFromHost();
        setDeploymentFinished();
        break;
    case Deploying:
        stopInternal();
        break;
    default:
        qFatal("Missing switch case in %s.", Q_FUNC_INFO);
    }
}

bool AbstractMaemoDeployStep::currentlyNeedsDeployment(const QString &host,
    const MaemoDeployable &deployable) const
{
    const QDateTime &lastDeployed
        = m_lastDeployed.value(DeployablePerHost(deployable, host));
    return !lastDeployed.isValid()
        || QFileInfo(deployable.localFilePath).lastModified() > lastDeployed;
}

void AbstractMaemoDeployStep::setDeployed(const QString &host,
    const MaemoDeployable &deployable)
{
    m_lastDeployed.insert(DeployablePerHost(deployable, host),
        QDateTime::currentDateTime());
}

void AbstractMaemoDeployStep::handleDeviceConfigurationsUpdated()
{
    setDeviceConfig(MaemoDeviceConfigurations::instance()->internalId(m_deviceConfig));
}

void AbstractMaemoDeployStep::setDeviceConfig(MaemoDeviceConfig::Id internalId)
{
    m_deviceConfig = maemoDeployConfig()->deviceConfigModel()->find(internalId);
    emit deviceConfigChanged();
}

void AbstractMaemoDeployStep::setDeviceConfig(int i)
{
    m_deviceConfig = maemoDeployConfig()->deviceConfigModel()->deviceAt(i);
    emit deviceConfigChanged();
}

bool AbstractMaemoDeployStep::isDeploymentPossible(QString &whyNot) const
{
    if (!m_deviceConfig) {
        whyNot = tr("No valid device set.");
        return false;
    }
    return isDeploymentPossibleInternal(whyNot);
}

void AbstractMaemoDeployStep::start()
{
    if (m_baseState != BaseInactive) {
        raiseError(tr("Cannot deploy: Still cleaning up from last time."));
        emit done();
        return;
    }

    m_cachedDeviceConfig = m_deviceConfig;

    QString message;
    if (!isDeploymentPossible(message)) {
        raiseError(tr("Cannot deploy: %1").arg(message));
        emit done();
        return;
    }

    m_hasError = false;
    if (isDeploymentNeeded(m_cachedDeviceConfig->sshParameters().host)) {
        if (m_cachedDeviceConfig->type() == MaemoDeviceConfig::Emulator
                && !MaemoQemuManager::instance().qemuIsRunning()) {
            MaemoQemuManager::instance().startRuntime();
            raiseError(tr("Cannot deploy: Qemu was not running. "
                "It has now been started up for you, but it will take "
                "a bit of time until it is ready."));
            emit done();
            return;
        }

        connectToDevice();
    } else {
        writeOutput(tr("All files up to date, no installation necessary."));
        emit done();
    }
}

void AbstractMaemoDeployStep::handleConnectionFailure()
{
    if (m_baseState == BaseInactive)
        return;

    const QString errorMsg = m_baseState == Connecting
        ? MaemoGlobal::failedToConnectToServerMessage(m_connection, m_cachedDeviceConfig)
        : tr("Connection error: %1").arg(m_connection->errorString());
    raiseError(errorMsg);
    setDeploymentFinished();
}

void AbstractMaemoDeployStep::connectToDevice()
{
    ASSERT_STATE(QList<BaseState>() << BaseInactive);
    setBaseState(Connecting);

    m_connection = SshConnectionManager::instance().acquireConnection(m_cachedDeviceConfig->sshParameters());
    connect(m_connection.data(), SIGNAL(error(Utils::SshError)), this,
        SLOT(handleConnectionFailure()));
    if (m_connection->state() == SshConnection::Connected) {
        handleConnected();
    } else {
        connect(m_connection.data(), SIGNAL(connected()), this,
            SLOT(handleConnected()));
        writeOutput(tr("Connecting to device..."));
        m_connection->connectToHost();
    }
}

void AbstractMaemoDeployStep::handleConnected()
{
    ASSERT_STATE(QList<BaseState>() << Connecting << StopRequested);

    if (m_baseState == Connecting) {
        setBaseState(Deploying);
        startInternal();
    }
}

void AbstractMaemoDeployStep::handleProgressReport(const QString &progressMsg)
{
    ASSERT_STATE(QList<BaseState>() << Deploying << StopRequested << BaseInactive);

    switch (m_baseState) {
    case Deploying:
    case StopRequested:
        writeOutput(progressMsg);
        break;
    case BaseInactive:
    default:
        break;
    }
}

void AbstractMaemoDeployStep::setDeploymentFinished()
{
    if (m_hasError)
        writeOutput(tr("Deployment failed."), ErrorMessageOutput);
    else
        writeOutput(tr("Deployment finished."));
    setBaseState(BaseInactive);
}

void AbstractMaemoDeployStep::setBaseState(BaseState newState)
{
    if (newState == m_baseState)
        return;
    m_baseState = newState;
    if (m_baseState == BaseInactive) {
        disconnect(m_connection.data(), 0, this, 0);
        SshConnectionManager::instance().releaseConnection(m_connection);
        emit done();
    }
}

void AbstractMaemoDeployStep::handleRemoteStdout(const QString &output)
{
    ASSERT_STATE(QList<BaseState>() << Deploying << StopRequested);

    switch (m_baseState) {
    case Deploying:
    case StopRequested:
        writeOutput(output, NormalOutput, DontAppendNewline);
        break;
    default:
        break;
    }
}

void AbstractMaemoDeployStep::handleRemoteStderr(const QString &output)
{
    ASSERT_STATE(QList<BaseState>() << Deploying << StopRequested);

    switch (m_baseState) {
    case Deploying:
    case StopRequested:
        writeOutput(output, ErrorOutput, DontAppendNewline);
        break;
    default:
        break;
    }
}

MaemoPortList AbstractMaemoDeployStep::freePorts() const
{
    const Qt4BuildConfiguration * const qt4bc = qt4BuildConfiguration();
    const MaemoDeviceConfig::ConstPtr &devConf
        = m_cachedDeviceConfig ? m_cachedDeviceConfig : m_deviceConfig;
    if (!devConf)
        return MaemoPortList();
    if (devConf->type() == MaemoDeviceConfig::Emulator && qt4bc) {
        MaemoQemuRuntime rt;
        const int id = qt4bc->qtVersion()->uniqueId();
        if (MaemoQemuManager::instance().runtimeForQtVersion(id, &rt))
            return rt.m_freePorts;
    }
    return devConf->freePorts();
}

const Qt4BuildConfiguration *AbstractMaemoDeployStep::qt4BuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(buildConfiguration());
}

Qt4MaemoDeployConfiguration *AbstractMaemoDeployStep::maemoDeployConfig() const
{
    return qobject_cast<Qt4MaemoDeployConfiguration *>(deployConfiguration());
}

MaemoDeployEventHandler::MaemoDeployEventHandler(AbstractMaemoDeployStep *deployStep,
    QFutureInterface<bool> &future)
    : m_deployStep(deployStep), m_future(future), m_eventLoop(new QEventLoop),
      m_error(false)
{
    connect(m_deployStep, SIGNAL(done()), this, SLOT(handleDeployingDone()));
    connect(m_deployStep, SIGNAL(error()), this, SLOT(handleDeployingFailed()));
    QTimer cancelChecker;
    connect(&cancelChecker, SIGNAL(timeout()), this, SLOT(checkForCanceled()));
    cancelChecker.start(500);
    future.reportResult(m_eventLoop->exec() == 0);
}

void MaemoDeployEventHandler::handleDeployingDone()
{
    m_eventLoop->exit(m_error ? 1 : 0);
}

void MaemoDeployEventHandler::handleDeployingFailed()
{
    m_error = true;
}

void MaemoDeployEventHandler::checkForCanceled()
{
    if (!m_error && m_future.isCanceled()) {
        QMetaObject::invokeMethod(m_deployStep, "stop");
        m_error = true;
        handleDeployingDone();
    }
}

} // namespace Internal
} // namespace Qt4ProjectManager

#include "abstractmaemodeploystep.moc"
