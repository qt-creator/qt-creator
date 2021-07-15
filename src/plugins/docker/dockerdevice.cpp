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

#include "dockerdevice.h"

#include "dockerconstants.h"

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevicewidget.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionfactory.h>
#include <qtsupport/qtversionmanager.h>

#include <utils/algorithm.h>
#include <utils/basetreeview.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/utilsicons.h>
#include <utils/layoutbuilder.h>
#include <utils/overridecursor.h>
#include <utils/port.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/temporaryfile.h>
#include <utils/treemodel.h>
#include <utils/fileutils.h>

#include <QApplication>
#include <QCheckBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileSystemWatcher>
#include <QHeaderView>
#include <QLoggingCategory>
#include <QPushButton>
#include <QTextBrowser>
#include <QToolButton>
#include <QThread>

#include <numeric>

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <sys/types.h>
#endif

using namespace Core;
using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

namespace Docker {
namespace Internal {

static Q_LOGGING_CATEGORY(dockerDeviceLog, "qtc.docker.device", QtWarningMsg);
#define LOG(x) qCDebug(dockerDeviceLog) << x << '\n'

class DockerDeviceProcess : public ProjectExplorer::DeviceProcess
{
public:
    DockerDeviceProcess(const QSharedPointer<const IDevice> &device, QObject *parent = nullptr);
    ~DockerDeviceProcess() {}

    void start(const Runnable &runnable) override;

    void interrupt() override;
    void terminate() override { m_process.terminate(); }
    void kill() override;

    QProcess::ProcessState state() const override;
    QProcess::ExitStatus exitStatus() const override;
    int exitCode() const override;
    QString errorString() const override;

    QByteArray readAllStandardOutput() override;
    QByteArray readAllStandardError() override;

    qint64 write(const QByteArray &data) override { return m_process.write(data); }

private:
    QtcProcess m_process;
};

DockerDeviceProcess::DockerDeviceProcess(const QSharedPointer<const IDevice> &device,
                                           QObject *parent)
    : DeviceProcess(device, parent)
{
}

void DockerDeviceProcess::start(const Runnable &runnable)
{
    QTC_ASSERT(m_process.state() == QProcess::NotRunning, return);
    DockerDevice::ConstPtr dockerDevice = qSharedPointerCast<const DockerDevice>(device());
    QTC_ASSERT(dockerDevice, return);

    const QStringList dockerRunFlags = runnable.extraData[Constants::DOCKER_RUN_FLAGS].toStringList();

    connect(this, &DeviceProcess::readyReadStandardOutput, this, [this] {
        MessageManager::writeSilently(QString::fromLocal8Bit(readAllStandardError()));
    });
    connect(this, &DeviceProcess::readyReadStandardError, this, [this] {
        MessageManager::writeDisrupting(QString::fromLocal8Bit(readAllStandardError()));
    });

    disconnect(&m_process);

    m_process.setCommand(runnable.commandLine());
    m_process.setEnvironment(runnable.environment);
    m_process.setWorkingDirectory(runnable.workingDirectory);
    connect(&m_process, &QtcProcess::errorOccurred, this, &DeviceProcess::error);
    connect(&m_process, &QtcProcess::finished, this, &DeviceProcess::finished);
    connect(&m_process, &QtcProcess::readyReadStandardOutput,
            this, &DeviceProcess::readyReadStandardOutput);
    connect(&m_process, &QtcProcess::readyReadStandardError,
            this, &DeviceProcess::readyReadStandardError);
    connect(&m_process, &QtcProcess::started, this, &DeviceProcess::started);
    dockerDevice->runProcess(m_process);
}

void DockerDeviceProcess::interrupt()
{
    device()->signalOperation()->interruptProcess(m_process.processId());
}

void DockerDeviceProcess::kill()
{
    m_process.kill();
}

QProcess::ProcessState DockerDeviceProcess::state() const
{
    return m_process.state();
}

QProcess::ExitStatus DockerDeviceProcess::exitStatus() const
{
    return m_process.exitStatus();
}

int DockerDeviceProcess::exitCode() const
{
    return m_process.exitCode();
}

QString DockerDeviceProcess::errorString() const
{
    return m_process.errorString();
}

QByteArray DockerDeviceProcess::readAllStandardOutput()
{
    return m_process.readAllStandardOutput();
}

QByteArray DockerDeviceProcess::readAllStandardError()
{
    return m_process.readAllStandardError();
}


class DockerPortsGatheringMethod : public PortsGatheringMethod
{
    Runnable runnable(QAbstractSocket::NetworkLayerProtocol protocol) const override
    {
        // We might encounter the situation that protocol is given IPv6
        // but the consumer of the free port information decides to open
        // an IPv4(only) port. As a result the next IPv6 scan will
        // report the port again as open (in IPv6 namespace), while the
        // same port in IPv4 namespace might still be blocked, and
        // re-use of this port fails.
        // GDBserver behaves exactly like this.

        Q_UNUSED(protocol)

        // /proc/net/tcp* covers /proc/net/tcp and /proc/net/tcp6
        Runnable runnable;
        runnable.executable = FilePath::fromString("sed");
        runnable.commandLineArguments
                = "-e 's/.*: [[:xdigit:]]*:\\([[:xdigit:]]\\{4\\}\\).*/\\1/g' /proc/net/tcp*";
        return runnable;
    }

    QList<Utils::Port> usedPorts(const QByteArray &output) const override
    {
        QList<Utils::Port> ports;
        QList<QByteArray> portStrings = output.split('\n');
        foreach (const QByteArray &portString, portStrings) {
            if (portString.size() != 4)
                continue;
            bool ok;
            const Utils::Port port(portString.toInt(&ok, 16));
            if (ok) {
                if (!ports.contains(port))
                    ports << port;
            } else {
                qWarning("%s: Unexpected string '%s' is not a port.",
                         Q_FUNC_INFO, portString.data());
            }
        }
        return ports;
    }
};

class KitDetectorPrivate
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::KitItemDetector)

public:
    KitDetectorPrivate(KitDetector *parent, const IDevice::ConstPtr &device)
        : q(parent), m_device(device)
    {}

    void autoDetect();
    void undoAutoDetect() const;

    QList<BaseQtVersion *> autoDetectQtVersions() const;
    QList<ToolChain *> autoDetectToolChains();
    void autoDetectCMake();
    void autoDetectDebugger();

    KitDetector *q;
    IDevice::ConstPtr m_device;
    QString m_sharedId;
};

KitDetector::KitDetector(const IDevice::ConstPtr &device)
    : d(new KitDetectorPrivate(this, device))
{}

KitDetector::~KitDetector()
{
    delete d;
}

void KitDetector::autoDetect(const QString &sharedId) const
{
    d->m_sharedId = sharedId;
    d->autoDetect();
}

void KitDetector::undoAutoDetect(const QString &sharedId) const
{
    d->m_sharedId = sharedId;
    d->undoAutoDetect();
}

class DockerDevicePrivate : public QObject
{
    Q_DECLARE_TR_FUNCTIONS(Docker::Internal::DockerDevice)

public:
    DockerDevicePrivate(DockerDevice *parent) : q(parent)
    {
        connect(&m_mergedDirWatcher, &QFileSystemWatcher::fileChanged, this, [](const QString &path) {
            Q_UNUSED(path)
            LOG("Container watcher change, file: " << path);
        });
        connect(&m_mergedDirWatcher, &QFileSystemWatcher::directoryChanged, this, [](const QString &path) {
            Q_UNUSED(path)
            LOG("Container watcher change, directory: " << path);
        });
    }

    ~DockerDevicePrivate() { delete m_shell; }

    bool runInContainer(const CommandLine &cmd) const;

    void tryCreateLocalFileAccess();

    void stopCurrentContainer();
    void fetchSystemEnviroment();

    DockerDevice *q;
    DockerDeviceData m_data;

    // For local file access
    QPointer<QtcProcess> m_shell;
    QString m_container;
    QString m_mergedDir;
    QFileSystemWatcher m_mergedDirWatcher;

    Environment m_cachedEnviroment;

    enum LocalAccessState
    {
        NotEvaluated,
        NoDaemon,
        Accessible,
        NotAccessible
    } m_accessible = NotEvaluated;
};

class DockerDeviceWidget final : public IDeviceWidget
{
    Q_DECLARE_TR_FUNCTIONS(Docker::Internal::DockerDevice)

public:
    explicit DockerDeviceWidget(const IDevice::Ptr &device)
        : IDeviceWidget(device), m_kitItemDetector(device)
    {
        auto dockerDevice = device.dynamicCast<DockerDevice>();
        QTC_ASSERT(dockerDevice, return);

        DockerDeviceData &data = dockerDevice->data();

        auto idLabel = new QLabel(tr("Image Id:"));
        m_idLineEdit = new QLineEdit;
        m_idLineEdit->setText(data.imageId);
        m_idLineEdit->setEnabled(false);

        auto repoLabel = new QLabel(tr("Repository:"));
        m_repoLineEdit = new QLineEdit;
        m_repoLineEdit->setText(data.repo);
        m_repoLineEdit->setEnabled(false);

        auto daemonStateLabel = new QLabel(tr("Daemon state:"));
        m_daemonReset = new QToolButton;
        m_daemonReset->setIcon(Icons::INFO.icon());
        m_daemonReset->setToolTip(tr("Daemon state not evaluated."));

        connect(m_daemonReset, &QToolButton::clicked, this, [this, dockerDevice] {
            dockerDevice->resetDaemonState();
            m_daemonReset->setIcon(Icons::INFO.icon());
            m_daemonReset->setToolTip(tr("Daemon state not evaluated."));
        });

        m_runAsOutsideUser = new QCheckBox(tr("Run as outside user"));
        m_runAsOutsideUser->setToolTip(tr("Use user ID and group ID of the user running Qt Creator "
                                          "in the Docker container."));
        m_runAsOutsideUser->setChecked(data.useLocalUidGid);
        m_runAsOutsideUser->setEnabled(HostOsInfo::isLinuxHost());

        connect(m_runAsOutsideUser, &QCheckBox::toggled, this, [&data](bool on) {
            data.useLocalUidGid = on;
        });

        m_pathsLineEdit = new QLineEdit;
        m_pathsLineEdit->setText(data.repo);
        m_pathsLineEdit->setToolTip(tr("Paths in this semi-colon separated list will be "
            "mapped one-to-one into the Docker container."));
        m_pathsLineEdit->setText(data.mounts.join(';'));
        m_pathsLineEdit->setPlaceholderText(tr("List project source directories here"));

        connect(m_pathsLineEdit, &QLineEdit::textChanged, this, [dockerDevice](const QString &text) {
            dockerDevice->setMounts(text.split(';', Qt::SkipEmptyParts));
        });

        auto logView = new QTextBrowser;
        connect(&m_kitItemDetector, &KitDetector::logOutput,
                logView, &QTextBrowser::append);

        auto autoDetectButton = new QPushButton(tr("Auto-detect Kit Items"));
        auto undoAutoDetectButton = new QPushButton(tr("Remove Auto-Detected Kit Items"));

        connect(autoDetectButton, &QPushButton::clicked, this, [this, logView, id = data.id(), dockerDevice] {
            logView->clear();
            dockerDevice->tryCreateLocalFileAccess();
            m_kitItemDetector.autoDetect(id);

            if (!dockerDevice->isDaemonRunning()) {
                logView->append(tr("Docker daemon appears to be not running."));
                m_daemonReset->setToolTip(tr("Daemon not running. Push to reset the state."));
                m_daemonReset->setIcon(Icons::CRITICAL.icon());
            } else {
                m_daemonReset->setToolTip(tr("Docker daemon running."));
                m_daemonReset->setIcon(Icons::OK.icon());

            }
        });

        connect(undoAutoDetectButton, &QPushButton::clicked, this, [this, logView, id = data.id()] {
            logView->clear();
            m_kitItemDetector.undoAutoDetect(id);
        });

        using namespace Layouting;

        Form {
            idLabel, m_idLineEdit, Break(),
            repoLabel, m_repoLineEdit, Break(),
            daemonStateLabel, m_daemonReset, Break(),
            m_runAsOutsideUser, Break(),
            tr("Paths to mount:"), m_pathsLineEdit, Break(),
            Column {
                Space(20),
                Row { autoDetectButton, undoAutoDetectButton, Stretch() },
                new QLabel(tr("Detection log:")),
                logView
            }
        }.attachTo(this);
    }

    void updateDeviceFromUi() final {}

private:
    QLineEdit *m_idLineEdit;
    QLineEdit *m_repoLineEdit;
    QToolButton *m_daemonReset;
    QCheckBox *m_runAsOutsideUser;
    QLineEdit *m_pathsLineEdit;

    KitDetector m_kitItemDetector;
};

IDeviceWidget *DockerDevice::createWidget()
{
    return new DockerDeviceWidget(sharedFromThis());
}


// DockerDevice

DockerDevice::DockerDevice(const DockerDeviceData &data)
    : d(new DockerDevicePrivate(this))
{
    d->m_data = data;

    setDisplayType(tr("Docker"));
    setOsType(OsTypeOtherUnix);
    setDefaultDisplayName(tr("Docker Image"));;
    setDisplayName(tr("Docker Image \"%1\" (%2)").arg(data.repo).arg(data.imageId));
    setAllowEmptyCommand(true);

    setOpenTerminal([this](const Environment &env, const QString &workingDir) {
        DeviceProcess * const proc = createProcess(nullptr);
        QObject::connect(proc, &DeviceProcess::finished, [proc] {
            if (!proc->errorString().isEmpty()) {
                MessageManager::writeDisrupting(
                    tr("Error running remote shell: %1").arg(proc->errorString()));
            }
            proc->deleteLater();
        });
        QObject::connect(proc, &DeviceProcess::error, [proc] {
            MessageManager::writeDisrupting(tr("Error starting remote shell."));
            proc->deleteLater();
        });

        Runnable runnable;
        runnable.executable = FilePath::fromString("/bin/sh");
        runnable.device = sharedFromThis();
        runnable.environment = env;
        runnable.workingDirectory = workingDir;
        runnable.extraData[Constants::DOCKER_RUN_FLAGS] = QStringList({"--interactive", "--tty"});

        proc->setRunInTerminal(true);
        proc->start(runnable);
    });

    if (HostOsInfo::isAnyUnixHost()) {
        addDeviceAction({tr("Open Shell in Container"), [](const IDevice::Ptr &device, QWidget *) {
            device->openTerminal(Environment(), QString());
        }});
    }
}

DockerDevice::~DockerDevice()
{
    delete d;
}

const DockerDeviceData &DockerDevice::data() const
{
    return d->m_data;
}

DockerDeviceData &DockerDevice::data()
{
    return d->m_data;
}

void KitDetectorPrivate::undoAutoDetect() const
{
    emit q->logOutput(tr("Start removing auto-detected items associated with this docker image."));

    emit q->logOutput('\n' + tr("Removing kits..."));
    for (Kit *kit : KitManager::kits()) {
        if (kit->autoDetectionSource() == m_sharedId) {
            emit q->logOutput(tr("Removed \"%1\"").arg(kit->displayName()));
            KitManager::deregisterKit(kit);
        }
    };

    emit q->logOutput('\n' + tr("Removing Qt version entries..."));
    for (BaseQtVersion *qtVersion : QtVersionManager::versions()) {
        if (qtVersion->detectionSource() == m_sharedId) {
            emit q->logOutput(tr("Removed \"%1\"").arg(qtVersion->displayName()));
            QtVersionManager::removeVersion(qtVersion);
        }
    };

    emit q->logOutput('\n' + tr("Removing toolchain entries..."));
    for (ToolChain *toolChain : ToolChainManager::toolChains()) {
        QString detectionSource = toolChain->detectionSource();
        if (toolChain->detectionSource() == m_sharedId) {
            emit q->logOutput(tr("Removed \"%1\"").arg(toolChain->displayName()));
            ToolChainManager::deregisterToolChain(toolChain);
        }
    };

    if (QObject *cmakeManager = ExtensionSystem::PluginManager::getObjectByName("CMakeToolManager")) {
        QString logMessage;
        const bool res = QMetaObject::invokeMethod(cmakeManager,
                                                   "removeDetectedCMake",
                                                   Q_ARG(QString, m_sharedId),
                                                   Q_ARG(QString *, &logMessage));
        QTC_CHECK(res);
        emit q->logOutput('\n' + logMessage);
    }

    if (QObject *debuggerPlugin = ExtensionSystem::PluginManager::getObjectByName("DebuggerPlugin")) {
        QString logMessage;
        const bool res = QMetaObject::invokeMethod(debuggerPlugin,
                                                   "removeDetectedDebuggers",
                                                   Q_ARG(QString, m_sharedId),
                                                   Q_ARG(QString *, &logMessage));
        QTC_CHECK(res);
        emit q->logOutput('\n' + logMessage);
    }

    emit q->logOutput('\n' + tr("Removal of previously auto-detected kit items finished.") + "\n\n");
}

QList<BaseQtVersion *> KitDetectorPrivate::autoDetectQtVersions() const
{
    QList<BaseQtVersion *> qtVersions;

    QString error;
    const QStringList candidates = {"qmake-qt6", "qmake-qt5", "qmake"};
    emit q->logOutput('\n' + tr("Searching Qt installations..."));
    for (const QString &candidate : candidates) {
        emit q->logOutput(tr("Searching for %1 executable...").arg(candidate));
        const FilePath qmake = m_device->searchExecutableInPath(candidate);
        if (qmake.isEmpty())
            continue;
        BaseQtVersion *qtVersion = QtVersionFactory::createQtVersionFromQMakePath(qmake, false, m_sharedId, &error);
        if (!qtVersion)
            continue;
        qtVersions.append(qtVersion);
        QtVersionManager::addVersion(qtVersion);
        emit q->logOutput(tr("Found \"%1\"").arg(qtVersion->qmakeFilePath().toUserOutput()));
    }
    if (qtVersions.isEmpty())
        emit q->logOutput(tr("No Qt installation found."));
    return qtVersions;
}

QList<ToolChain *> KitDetectorPrivate::autoDetectToolChains()
{
    const QList<ToolChainFactory *> factories = ToolChainFactory::allToolChainFactories();

    QList<ToolChain *> alreadyKnown = ToolChainManager::toolChains();
    QList<ToolChain *> allNewToolChains;
    QApplication::processEvents();
    emit q->logOutput('\n' + tr("Searching toolchains..."));
    for (ToolChainFactory *factory : factories) {
        emit q->logOutput(tr("Searching toolchains of type %1").arg(factory->displayName()));
        const QList<ToolChain *> newToolChains = factory->autoDetect(alreadyKnown, m_device.constCast<IDevice>());
        for (ToolChain *toolChain : newToolChains) {
            emit q->logOutput(tr("Found \"%1\"").arg(toolChain->compilerCommand().toUserOutput()));
            toolChain->setDetectionSource(m_sharedId);
            ToolChainManager::registerToolChain(toolChain);
            alreadyKnown.append(toolChain);
        }
        allNewToolChains.append(newToolChains);
    }
    emit q->logOutput(tr("%1 new toolchains found.").arg(allNewToolChains.size()));

    return allNewToolChains;
}

void KitDetectorPrivate::autoDetectCMake()
{
    QObject *cmakeManager = ExtensionSystem::PluginManager::getObjectByName("CMakeToolManager");
    if (!cmakeManager)
        return;

    const FilePath deviceRoot = m_device->mapToGlobalPath({});
    QString logMessage;
    const bool res = QMetaObject::invokeMethod(cmakeManager,
                                               "autoDetectCMakeForDevice",
                                               Q_ARG(Utils::FilePath, deviceRoot),
                                               Q_ARG(QString, m_sharedId),
                                               Q_ARG(QString *, &logMessage));
    QTC_CHECK(res);
    emit q->logOutput('\n' + logMessage);
}

void KitDetectorPrivate::autoDetectDebugger()
{
    QObject *debuggerPlugin = ExtensionSystem::PluginManager::getObjectByName("DebuggerPlugin");
    if (!debuggerPlugin)
        return;

    const FilePath deviceRoot = m_device->mapToGlobalPath({});
    QString logMessage;
    const bool res = QMetaObject::invokeMethod(debuggerPlugin,
                                               "autoDetectDebuggersForDevice",
                                               Q_ARG(Utils::FilePath, deviceRoot),
                                               Q_ARG(QString, m_sharedId),
                                               Q_ARG(QString *, &logMessage));
    QTC_CHECK(res);
    emit q->logOutput('\n' + logMessage);
}

void KitDetectorPrivate::autoDetect()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    undoAutoDetect();

    emit q->logOutput(tr("Starting auto-detection. This will take a while..."));

    QList<ToolChain *> toolChains = autoDetectToolChains();
    QList<BaseQtVersion *> qtVersions = autoDetectQtVersions();

    autoDetectCMake();
    autoDetectDebugger();

    const auto initializeKit = [this, toolChains, qtVersions](Kit *k) {
        k->setAutoDetected(false);
        k->setAutoDetectionSource(m_sharedId);
        k->setUnexpandedDisplayName("%{Device:Name}");

        DeviceTypeKitAspect::setDeviceTypeId(k, Constants::DOCKER_DEVICE_TYPE);
        DeviceKitAspect::setDevice(k, m_device);
        for (ToolChain *tc : toolChains)
            ToolChainKitAspect::setToolChain(k, tc);
        if (!qtVersions.isEmpty())
            QtSupport::QtKitAspect::setQtVersion(k, qtVersions.at(0));

        k->setSticky(ToolChainKitAspect::id(), true);
        k->setSticky(QtSupport::QtKitAspect::id(), true);
        k->setSticky(DeviceKitAspect::id(), true);
        k->setSticky(DeviceTypeKitAspect::id(), true);
    };

    Kit *kit = KitManager::registerKit(initializeKit);
    emit q->logOutput('\n' + tr("Registered kit %1").arg(kit->displayName()));

    QApplication::restoreOverrideCursor();
}

void DockerDevice::tryCreateLocalFileAccess() const
{
    d->tryCreateLocalFileAccess();
}

void DockerDevicePrivate::stopCurrentContainer()
{
    if (m_container.isEmpty() || m_accessible == NoDaemon)
        return;

    QtcProcess proc;
    proc.setCommand({"docker", {"container", "stop", m_container}});

    m_container.clear();
    m_mergedDir.clear();

    proc.runBlocking();
}

void DockerDevicePrivate::tryCreateLocalFileAccess()
{
    if (!m_container.isEmpty() || m_accessible == NoDaemon)
        return;

    QString tempFileName;

    {
        TemporaryFile temp("qtc-docker-XXXXXX");
        temp.open();
        tempFileName = temp.fileName();
    }

    CommandLine dockerRun{"docker", {"run", "-i", "--cidfile=" + tempFileName,
                                    "--rm",
                                    "-e", "DISPLAY=:0",
                                    "-e", "XAUTHORITY=/.Xauthority",
                                    "--net", "host"}};

#ifdef Q_OS_UNIX
    if (m_data.useLocalUidGid)
        dockerRun.addArgs({"-u", QString("%1:%2").arg(getuid()).arg(getgid())});
#endif

    for (const QString &mount : qAsConst(m_data.mounts)) {
        if (!mount.isEmpty())
            dockerRun.addArgs({"-v", mount + ':' + mount});
    }

    dockerRun.addArg(m_data.imageId);
    dockerRun.addArg("/bin/sh");

    LOG("RUNNING: " << dockerRun.toUserOutput());
    m_shell = new QtcProcess;
    m_shell->setCommand(dockerRun);
    connect(m_shell, &QtcProcess::finished, this, [this] {
        LOG("\nSHELL FINISHED\n");
        if (m_shell) {
            LOG("RES: " << m_shell->result()
                << " STDOUT: " << m_shell->readAllStandardOutput()
                << " STDERR: " << m_shell->readAllStandardError());
            // negative exit codes indicate problems like no docker daemon, missing permissions,
            // no shell and seem to result in exit codes 125+
            if (m_shell->exitCode() > 120) {
                m_accessible = NoDaemon;
                LOG("DOCKER DAEMON NOT RUNNING?");
                MessageManager::writeFlashing(tr("Docker Daemon appears to be not running. "
                                                 "Verify daemon is up and running and reset the "
                                                 "docker daemon on the docker device settings page "
                                                 "or restart Qt Creator."));
            }
        }
        m_container.clear();
    });

    m_shell->start();
    m_shell->waitForStarted();

    if (m_shell->state() != QProcess::Running) {
        LOG("DOCKER SHELL FAILED");
        return;
    }

    LOG("CHECKING: " << tempFileName);
    for (int i = 0; i <= 20; ++i) {
        QFile file(tempFileName);
        if (file.open(QIODevice::ReadOnly)) {
            m_container = QString::fromUtf8(file.readAll()).trimmed();
            if (!m_container.isEmpty()) {
                LOG("Container: " << m_container);
                break;
            }
        }
        if (i == 20 || m_accessible == NoDaemon) {
            qWarning("Docker cid file empty.");
            return; // No
        }
        qApp->processEvents(); // FIXME turn this for-loop into QEventLoop
        QThread::msleep(100);
    }

    QtcProcess proc;
    proc.setCommand({"docker", {"inspect", "--format={{.GraphDriver.Data.MergedDir}}", m_container}});
    LOG(proc.commandLine().toUserOutput());
    proc.start();
    proc.waitForFinished();
    const QString out = proc.stdOut();
    m_mergedDir = out.trimmed();
    LOG("Found merged dir: " << m_mergedDir);
    if (m_mergedDir.endsWith('/'))
        m_mergedDir.chop(1);

    if (!QFileInfo(m_mergedDir).isReadable()) {
        MessageManager::writeFlashing(
            tr("Local read access to Docker container %1 unavailable through directory \"%2\".")
                .arg(m_container, m_mergedDir)
                    + '\n' + tr("Output: '%1'").arg(out)
                    + '\n' + tr("Error: '%1'").arg(proc.stdErr()));
        if (HostOsInfo::isWindowsHost()) {
            // Disabling merged layer access. This is not supported and anything
            // related to accessing merged layers on Windows fails due to the need
            // of using wsl or a named pipe.
            // TODO investigate how to make it possible nevertheless.
            m_mergedDir.clear();
            m_accessible = NotAccessible;
            MessageManager::writeSilently(tr("This is expected on Windows."));
            return;
        }
    }

    m_accessible = Accessible;
    m_mergedDirWatcher.addPath(m_mergedDir);
}

bool DockerDevice::hasLocalFileAccess() const
{
    return !d->m_mergedDir.isEmpty();
}

bool DockerDevice::isDaemonRunning() const
{
    switch (d->m_accessible) {
    case DockerDevicePrivate::NoDaemon:
        return false;
    case DockerDevicePrivate::NotEvaluated: // FIXME?
    case DockerDevicePrivate::Accessible:
    case DockerDevicePrivate::NotAccessible:
        return true;
    }
    return false;
}

void DockerDevice::resetDaemonState()
{
    d->m_accessible = DockerDevicePrivate::NotEvaluated;
}

void DockerDevice::setMounts(const QStringList &mounts) const
{
    d->m_data.mounts = mounts;
    d->stopCurrentContainer(); // Force re-start with new mounts.
}

FilePath DockerDevice::mapToLocalAccess(const FilePath &filePath) const
{
    QTC_ASSERT(!d->m_mergedDir.isEmpty(), return {});
    QString path = filePath.path();
    for (const QString &mount : qAsConst(d->m_data.mounts)) {
        if (path.startsWith(mount + '/'))
            return FilePath::fromString(path);
    }
    if (path.startsWith('/'))
        return FilePath::fromString(d->m_mergedDir + path);
    return FilePath::fromString(d->m_mergedDir + '/' + path);
}

FilePath DockerDevice::mapFromLocalAccess(const FilePath &filePath) const
{
    QTC_ASSERT(!filePath.needsDevice(), return {});
    return mapFromLocalAccess(filePath.toString());
}

FilePath DockerDevice::mapFromLocalAccess(const QString &filePath) const
{
    QTC_ASSERT(!d->m_mergedDir.isEmpty(), return {});
    QTC_ASSERT(filePath.startsWith(d->m_mergedDir), return FilePath::fromString(filePath));
    return mapToGlobalPath(FilePath::fromString(filePath.mid(d->m_mergedDir.size())));
}

const char DockerDeviceDataImageIdKey[] = "DockerDeviceDataImageId";
const char DockerDeviceDataRepoKey[] = "DockerDeviceDataRepo";
const char DockerDeviceDataTagKey[] = "DockerDeviceDataTag";
const char DockerDeviceDataSizeKey[] = "DockerDeviceDataSize";
const char DockerDeviceUseOutsideUser[] = "DockerDeviceUseUidGid";
const char DockerDeviceMappedPaths[] = "DockerDeviceMappedPaths";

void DockerDevice::fromMap(const QVariantMap &map)
{
    ProjectExplorer::IDevice::fromMap(map);
    d->m_data.imageId = map.value(DockerDeviceDataImageIdKey).toString();
    d->m_data.repo = map.value(DockerDeviceDataRepoKey).toString();
    d->m_data.tag = map.value(DockerDeviceDataTagKey).toString();
    d->m_data.size = map.value(DockerDeviceDataSizeKey).toString();
    d->m_data.useLocalUidGid = map.value(DockerDeviceUseOutsideUser,
                                         HostOsInfo::isLinuxHost()).toBool();
    d->m_data.mounts = map.value(DockerDeviceMappedPaths).toStringList();
}

QVariantMap DockerDevice::toMap() const
{
    QVariantMap map = ProjectExplorer::IDevice::toMap();
    map.insert(DockerDeviceDataImageIdKey, d->m_data.imageId);
    map.insert(DockerDeviceDataRepoKey, d->m_data.repo);
    map.insert(DockerDeviceDataTagKey, d->m_data.tag);
    map.insert(DockerDeviceDataSizeKey, d->m_data.size);
    map.insert(DockerDeviceUseOutsideUser, d->m_data.useLocalUidGid);
    map.insert(DockerDeviceMappedPaths, d->m_data.mounts);
    return map;
}

DeviceProcess *DockerDevice::createProcess(QObject *parent) const
{
    return new DockerDeviceProcess(sharedFromThis(), parent);
}

bool DockerDevice::canAutoDetectPorts() const
{
    return true;
}

PortsGatheringMethod::Ptr DockerDevice::portsGatheringMethod() const
{
    return DockerPortsGatheringMethod::Ptr(new DockerPortsGatheringMethod);
}

DeviceProcessList *DockerDevice::createProcessListModel(QObject *) const
{
    return nullptr;
}

DeviceTester *DockerDevice::createDeviceTester() const
{
    return nullptr;
}

DeviceProcessSignalOperation::Ptr DockerDevice::signalOperation() const
{
    return DeviceProcessSignalOperation::Ptr();
}

DeviceEnvironmentFetcher::Ptr DockerDevice::environmentFetcher() const
{
    return DeviceEnvironmentFetcher::Ptr();
}

FilePath DockerDevice::mapToGlobalPath(const FilePath &pathOnDevice) const
{
    if (pathOnDevice.needsDevice()) {
        // Already correct form, only sanity check it's ours...
        QTC_CHECK(handlesFile(pathOnDevice));
        return pathOnDevice;
    }
    FilePath result;
    result.setScheme("docker");
    result.setHost(d->m_data.imageId);
    result.setPath(pathOnDevice.path());
    return result;
}

bool DockerDevice::handlesFile(const FilePath &filePath) const
{
    return filePath.scheme() == "docker" && filePath.host() == d->m_data.imageId;
}

bool DockerDevice::isExecutableFile(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    tryCreateLocalFileAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const bool res = localAccess.isExecutableFile();
        LOG("Executable? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }
    const QString path = filePath.path();
    return d->runInContainer({"test", {"-x", path}});
}

bool DockerDevice::isReadableFile(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    tryCreateLocalFileAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const bool res = localAccess.isReadableFile();
        LOG("ReadableFile? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }
    const QString path = filePath.path();
    return d->runInContainer({"test", {"-r", path, "-a", "-f", path}});
}

bool DockerDevice::isWritableFile(const Utils::FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    tryCreateLocalFileAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const bool res = localAccess.isWritableFile();
        LOG("WritableFile? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }
    const QString path = filePath.path();
    return d->runInContainer({"test", {"-w", path, "-a", "-f", path}});
}

bool DockerDevice::isReadableDirectory(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    tryCreateLocalFileAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const bool res = localAccess.isReadableDir();
        LOG("ReadableDirectory? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }
    const QString path = filePath.path();
    return d->runInContainer({"test", {"-r", path, "-a", "-d", path}});
}

bool DockerDevice::isWritableDirectory(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    tryCreateLocalFileAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const bool res = localAccess.isWritableDir();
        LOG("WritableDirectory? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }
    const QString path = filePath.path();
    return d->runInContainer({"test", {"-w", path, "-a", "-d", path}});
}

bool DockerDevice::isFile(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    tryCreateLocalFileAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const bool res = localAccess.isFile();
        LOG("IsFile? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }
    const QString path = filePath.path();
    return d->runInContainer({"test", {"-f", path}});
}

bool DockerDevice::isDirectory(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    tryCreateLocalFileAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const bool res = localAccess.isDir();
        LOG("IsDirectory? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }
    const QString path = filePath.path();
    return d->runInContainer({"test", {"-d", path}});
}

bool DockerDevice::createDirectory(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    tryCreateLocalFileAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const bool res = localAccess.createDir();
        LOG("CreateDirectory? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }
    const QString path = filePath.path();
    return d->runInContainer({"mkdir", {"-p", path}});
}

bool DockerDevice::exists(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    tryCreateLocalFileAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const bool res = localAccess.exists();
        LOG("Exists? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }
    const QString path = filePath.path();
    return d->runInContainer({"test", {"-e", path}});
}

bool DockerDevice::ensureExistingFile(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    tryCreateLocalFileAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const bool res = localAccess.ensureExistingFile();
        LOG("Ensure existing file? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }
    const QString path = filePath.path();
    return d->runInContainer({"touch", {path}});
}

bool DockerDevice::removeFile(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    tryCreateLocalFileAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const bool res = localAccess.removeFile();
        LOG("Remove? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }
    return d->runInContainer({"rm", {filePath.path()}});
}

bool DockerDevice::removeRecursively(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    QTC_ASSERT(filePath.path().startsWith('/'), return false);
    tryCreateLocalFileAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const bool res = localAccess.removeRecursively();
        LOG("Remove recursively? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }
// Open this up only when really needed.
//   return d->runInContainer({"rm", "-rf", {filePath.path()}});
    return false;
}

bool DockerDevice::copyFile(const FilePath &filePath, const FilePath &target) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    QTC_ASSERT(handlesFile(target), return false);
    tryCreateLocalFileAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const FilePath localTarget = mapToLocalAccess(target);
        const bool res = localAccess.copyFile(localTarget);
        LOG("Copy " << filePath.toUserOutput() << localAccess.toUserOutput() << localTarget << res);
        return res;
    }
    return d->runInContainer({"cp", {filePath.path(), target.path()}});
}

bool DockerDevice::renameFile(const FilePath &filePath, const FilePath &target) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    QTC_ASSERT(handlesFile(target), return false);
    tryCreateLocalFileAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const FilePath localTarget = mapToLocalAccess(target);
        const bool res = localAccess.renameFile(localTarget);
        LOG("Move " << filePath.toUserOutput() << localAccess.toUserOutput() << localTarget << res);
        return res;
    }
    return d->runInContainer({"mv", {filePath.path(), target.path()}});
}

QDateTime DockerDevice::lastModified(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return {});
    tryCreateLocalFileAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const QDateTime res = localAccess.lastModified();
        LOG("Last modified? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }
    QTC_CHECK(false);
    return {};
}

FilePath DockerDevice::symLinkTarget(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return {});
    tryCreateLocalFileAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const FilePath target = localAccess.symLinkTarget();
        LOG("SymLinkTarget? " << filePath.toUserOutput() << localAccess.toUserOutput() << target);
        if (target.isEmpty())
            return {};
        return mapToGlobalPath(target);
    }
    QTC_CHECK(false);
    return {};
}

FilePaths DockerDevice::directoryEntries(const FilePath &filePath,
                                         const QStringList &nameFilters,
                                         QDir::Filters filters,
                                         QDir::SortFlags sort) const
{
    QTC_ASSERT(handlesFile(filePath), return {});
    tryCreateLocalFileAccess();
    if (hasLocalFileAccess()) {
        const FilePaths entries = mapToLocalAccess(filePath).dirEntries(nameFilters, filters, sort);
        return Utils::transform(entries, [this](const FilePath &entry) {
            return mapFromLocalAccess(entry);
        });
    }

    QTC_CHECK(false); // FIXME: Implement
    return {};
}

QByteArray DockerDevice::fileContents(const FilePath &filePath, qint64 limit, qint64 offset) const
{
    QTC_ASSERT(handlesFile(filePath), return {});
    tryCreateLocalFileAccess();
    if (hasLocalFileAccess())
        return mapToLocalAccess(filePath).fileContents(limit, offset);

    QStringList args = {"if=" + filePath.path(), "status=none"};
    if (limit > 0 || offset > 0) {
        const qint64 gcd = std::gcd(limit, offset);
        args += {QString("bs=%1").arg(gcd),
                 QString("count=%1").arg(limit / gcd),
                 QString("seek=%1").arg(offset / gcd)};
    }

    QtcProcess proc;
    proc.setCommand({"dd", args});
    runProcess(proc);
    proc.waitForFinished();

    QByteArray output = proc.readAllStandardOutput();
    return output;
}

bool DockerDevice::writeFileContents(const Utils::FilePath &filePath, const QByteArray &data) const
{
    QTC_ASSERT(handlesFile(filePath), return {});
    tryCreateLocalFileAccess();
    if (hasLocalFileAccess())
        return mapToLocalAccess(filePath).writeFileContents(data);

    QTC_CHECK(false); // FIXME: Implement
    return {};
}

void DockerDevice::runProcess(QtcProcess &process) const
{
    tryCreateLocalFileAccess();
    if (d->m_container.isEmpty() || d->m_accessible == DockerDevicePrivate::NoDaemon) {
        LOG("No container set to run " << process.commandLine().toUserOutput());
        QTC_CHECK(false);
        process.setResult(QtcProcess::StartFailed);
        return;
    }

    const FilePath workingDir = process.workingDirectory();
    const CommandLine origCmd = process.commandLine();

    CommandLine cmd{"docker", {"exec"}};
    if (!workingDir.isEmpty())
        cmd.addArgs({"-w", workingDir.path()});
    if (process.keepsWriteChannelOpen())
        cmd.addArg("-i");
    cmd.addArg(d->m_container);
    cmd.addArg(origCmd.executable().path()); // Cut off the docker://.../ bits.
    cmd.addArgs(origCmd.splitArguments(osType()));

    LOG("Run" << cmd.toUserOutput() << " in " << workingDir.toUserOutput());

    process.setCommand(cmd);
    process.start();
}

Environment DockerDevice::systemEnvironment() const
{
    if (d->m_cachedEnviroment.size() == 0)
        d->fetchSystemEnviroment();

    QTC_CHECK(d->m_cachedEnviroment.size() != 0);
    return d->m_cachedEnviroment;
}

void DockerDevice::aboutToBeRemoved() const
{
    KitDetector detector(sharedFromThis());
    detector.undoAutoDetect(d->m_data.id());
}

void DockerDevicePrivate::fetchSystemEnviroment()
{
    QtcProcess proc;
    proc.setCommand({"env", {}});

    q->runProcess(proc); // FIXME: This only starts.
    proc.waitForFinished();

    const QString remoteOutput = proc.stdOut();
    m_cachedEnviroment = Environment(remoteOutput.split('\n', Qt::SkipEmptyParts), q->osType());
}

bool DockerDevicePrivate::runInContainer(const CommandLine &cmd) const
{
    if (m_accessible == NoDaemon)
        return false;
    CommandLine dcmd{"docker", {"exec", m_container}};
    dcmd.addArgs(cmd);

    QtcProcess proc;
    proc.setCommand(dcmd);
    proc.setWorkingDirectory(QDir::tempPath());
    proc.start();
    proc.waitForFinished();

    LOG("Run sync:" << dcmd.toUserOutput() << " result: " << proc.exitCode());
    const int exitCode = proc.exitCode();
    return exitCode == 0;
}

// Factory

DockerDeviceFactory::DockerDeviceFactory()
    : IDeviceFactory(Constants::DOCKER_DEVICE_TYPE)
{
    setDisplayName(DockerDevice::tr("Docker Device"));
    setIcon(QIcon());
    setCanCreate(true);
    setConstructionFunction([] { return DockerDevice::create({}); });
}

class DockerImageItem final : public TreeItem, public DockerDeviceData
{
public:
    DockerImageItem() {}

    QVariant data(int column, int role) const final
    {
        switch (column) {
        case 0:
            if (role == Qt::DisplayRole)
                return imageId;
            break;
        case 1:
            if (role == Qt::DisplayRole)
                return repo;
            break;
        case 2:
            if (role == Qt::DisplayRole)
                return tag;
            break;
        case 3:
            if (role == Qt::DisplayRole)
                return size;
            break;
        }

        return QVariant();
    }
};

class DockerDeviceSetupWizard final : public QDialog
{
public:
    DockerDeviceSetupWizard()
        : QDialog(ICore::dialogParent())
    {
        setWindowTitle(DockerDevice::tr("Docker Image Selection"));
        resize(800, 600);

        m_model.setHeader({"Image", "Repository", "Tag", "Size"});

        m_view = new TreeView;
        m_view->setModel(&m_model);
        m_view->header()->setStretchLastSection(true);
        m_view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
        m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_view->setSelectionMode(QAbstractItemView::SingleSelection);

        m_log = new QTextBrowser;
        m_log->setVisible(false);

        m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

        using namespace Layouting;
        Column {
            m_view,
            m_log,
            m_buttons,
        }.attachTo(this);

        connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        m_buttons->button(QDialogButtonBox::Ok)->setEnabled(false);

        CommandLine cmd{"docker", {"images", "--format", "{{.ID}}\\t{{.Repository}}\\t{{.Tag}}\\t{{.Size}}"}};
        m_log->append(DockerDevice::tr("Running \"%1\"\n").arg(cmd.toUserOutput()));

        m_process = new QtcProcess(this);
        m_process->setCommand(cmd);

        connect(m_process, &QtcProcess::readyReadStandardOutput, [this] {
            const QString out = QString::fromUtf8(m_process->readAllStandardOutput().trimmed());
            m_log->append(out);
            for (const QString &line : out.split('\n')) {
                const QStringList parts = line.trimmed().split('\t');
                if (parts.size() != 4) {
                    m_log->append(DockerDevice::tr("Unexpected result: %1").arg(line) + '\n');
                    continue;
                }
                auto item = new DockerImageItem;
                item->imageId = parts.at(0);
                item->repo = parts.at(1);
                item->tag = parts.at(2);
                item->size = parts.at(3);
                m_model.rootItem()->appendChild(item);
            }
            m_log->append(DockerDevice::tr("Done."));
        });

        connect(m_process, &Utils::QtcProcess::readyReadStandardError, this, [this] {
            const QString out = DockerDevice::tr("Error: %1").arg(m_process->stdErr());
            m_log->append(DockerDevice::tr("Error: %1").arg(out));
        });

        connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, [this] {
            const QModelIndexList selectedRows = m_view->selectionModel()->selectedRows();
            QTC_ASSERT(selectedRows.size() == 1, return);
            m_buttons->button(QDialogButtonBox::Ok)->setEnabled(selectedRows.size() == 1);
        });

        m_process->start();
    }

    DockerDevice::Ptr device() const
    {
        const QModelIndexList selectedRows = m_view->selectionModel()->selectedRows();
        QTC_ASSERT(selectedRows.size() == 1, return {});
        DockerImageItem *item = m_model.itemForIndex(selectedRows.front());
        QTC_ASSERT(item, return {});

        auto device = DockerDevice::create(*item);
        device->setupId(IDevice::ManuallyAdded, Id::fromString(item->id()));
        device->setType(Constants::DOCKER_DEVICE_TYPE);
        device->setMachineType(IDevice::Hardware);

        return device;
    }

public:
    TreeModel<DockerImageItem> m_model;
    TreeView *m_view = nullptr;
    QTextBrowser *m_log = nullptr;
    QDialogButtonBox *m_buttons;

    QtcProcess *m_process = nullptr;
    QString m_selectedId;
};

IDevice::Ptr DockerDeviceFactory::create() const
{
    DockerDeviceSetupWizard wizard;
    if (wizard.exec() != QDialog::Accepted)
        return IDevice::Ptr();
    return wizard.device();
}

} // Internal
} // Docker
