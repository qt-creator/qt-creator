/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "abstractmaemodeploystep.h"

#include "deployablefile.h"
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

#include <utils/ssh/sshconnectionmanager.h>

#include <QtCore/QDateTime>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>

#define ASSERT_STATE(state) ASSERT_STATE_GENERIC(BaseState, state, m_baseState)

using namespace Core;
using namespace Utils;
using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

namespace RemoteLinux {
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
        const QString &id)
    : BuildStep(parent, id),
      AbstractLinuxDeviceDeployStep(deployConfiguration())
{
    baseCtor();
}

AbstractMaemoDeployStep::AbstractMaemoDeployStep(BuildStepList *parent,
        AbstractMaemoDeployStep *other)
    : BuildStep(parent, other),
      AbstractLinuxDeviceDeployStep(deployConfiguration()),
      m_lastDeployed(other->m_lastDeployed)
{
    baseCtor();
}

AbstractMaemoDeployStep::~AbstractMaemoDeployStep() { }

void AbstractMaemoDeployStep::baseCtor()
{
    m_baseState = BaseInactive;
}

bool AbstractMaemoDeployStep::init()
{
    QString errorMsg;
    if (!initialize(errorMsg)) {
        writeOutput(errorMsg, ErrorMessageOutput);
        return false;
    }
    return true;
}

void AbstractMaemoDeployStep::run(QFutureInterface<bool> &fi)
{
    // Move to GUI thread.
    QTimer::singleShot(0, this, SLOT(start()));

    MaemoDeployEventHandler eventHandler(this, fi);
}

BuildStepConfigWidget *AbstractMaemoDeployStep::createConfigWidget()
{
    return new MaemoDeployStepBaseWidget(this);
}

QVariantMap AbstractMaemoDeployStep::toMap() const
{
    QVariantMap map(BuildStep::toMap());
    addDeployTimesToMap(map);
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
        const DeployableFile d(fileList.at(i).toString(),
            remotePathList.at(i).toString());
        m_lastDeployed.insert(DeployablePerHost(d, hostList.at(i).toString()),
            timeList.at(i).toDateTime());
    }
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
    const DeployableFile &deployable) const
{
    const QDateTime &lastDeployed
        = m_lastDeployed.value(DeployablePerHost(deployable, host));
    return !lastDeployed.isValid()
        || QFileInfo(deployable.localFilePath).lastModified() > lastDeployed;
}

void AbstractMaemoDeployStep::setDeployed(const QString &host,
    const DeployableFile &deployable)
{
    m_lastDeployed.insert(DeployablePerHost(deployable, host),
        QDateTime::currentDateTime());
}

void AbstractMaemoDeployStep::start()
{
    if (m_baseState != BaseInactive) {
        raiseError(tr("Cannot deploy: Still cleaning up from last time."));
        emit done();
        return;
    }

    m_hasError = false;
    if (isDeploymentNeeded(deviceConfiguration()->sshParameters().host)) {
        if (deviceConfiguration()->type() == LinuxDeviceConfiguration::Emulator
                && !MaemoQemuManager::instance().qemuIsRunning()) {
            MaemoQemuRuntime rt;
            const int qtId = qt4BuildConfiguration() && qt4BuildConfiguration()->qtVersion()
                ? qt4BuildConfiguration()->qtVersion()->uniqueId() : -1;
            if (MaemoQemuManager::instance().runtimeForQtVersion(qtId, &rt)) {
                MaemoQemuManager::instance().startRuntime();
                raiseError(tr("Cannot deploy: Qemu was not running. "
                    "It has now been started up for you, but it will take "
                    "a bit of time until it is ready. Please try again then."));
            } else {
                raiseError(tr("Cannot deploy: You want to deploy to Qemu, but it is not enabled "
                    "for this Qt version."));
            }
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
        ? MaemoGlobal::failedToConnectToServerMessage(m_connection, deviceConfiguration())
        : tr("Connection error: %1").arg(m_connection->errorString());
    raiseError(errorMsg);
    setDeploymentFinished();
}

void AbstractMaemoDeployStep::connectToDevice()
{
    ASSERT_STATE(QList<BaseState>() << BaseInactive);
    setBaseState(Connecting);

    m_connection = SshConnectionManager::instance().acquireConnection(deviceConfiguration()->sshParameters());
    connect(m_connection.data(), SIGNAL(error(Utils::SshError)), this,
        SLOT(handleConnectionFailure()));
    if (m_connection->state() == SshConnection::Connected) {
        handleConnected();
    } else {
        connect(m_connection.data(), SIGNAL(connected()), this,
            SLOT(handleConnected()));
        writeOutput(tr("Connecting to device..."));
        if (m_connection->state() == SshConnection::Unconnected)
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

const Qt4BuildConfiguration *AbstractMaemoDeployStep::qt4BuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(buildConfiguration());
}

SshConnection::Ptr AbstractMaemoDeployStep::connection() const
{
    return m_connection;
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
} // namespace RemoteLinux

#include "abstractmaemodeploystep.moc"
