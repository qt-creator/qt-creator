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
#include <utils/layoutbuilder.h>
#include <utils/overridecursor.h>
#include <utils/port.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/temporaryfile.h>
#include <utils/treemodel.h>

#include <QApplication>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileSystemWatcher>
#include <QHeaderView>
#include <QPushButton>
#include <QTextBrowser>
#include <QThread>

using namespace Core;
using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

#define LOG(x)
//#define LOG(x) qDebug() << x

namespace Docker {
namespace Internal {

const QByteArray pidMarker = "__qtc";

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

    int runSynchronously(const CommandLine &cmd) const;

    void tryCreateLocalFileAccess();

    void autoDetect(QTextBrowser *log);
    void undoAutoDetect(QTextBrowser *log) const;

    BaseQtVersion *autoDetectQtVersion(QTextBrowser *log) const;
    QList<ToolChain *> autoDetectToolChains(QTextBrowser *log);
    void autoDetectCMake(QTextBrowser *log);

    void fetchSystemEnviroment();

    DockerDevice *q;
    DockerDeviceData m_data;

    // For local file access
    QPointer<QtcProcess> m_shell;
    QString m_container;
    QString m_mergedDir;
    QFileSystemWatcher m_mergedDirWatcher;

    Environment m_cachedEnviroment;
};

class DockerDeviceWidget final : public IDeviceWidget
{
    Q_DECLARE_TR_FUNCTIONS(Docker::Internal::DockerDevice)

public:
    explicit DockerDeviceWidget(const IDevice::Ptr &device)
        : IDeviceWidget(device)
    {
        auto dockerDevice = device.dynamicCast<DockerDevice>();
        QTC_ASSERT(dockerDevice, return);

        m_idLabel = new QLabel(tr("Image Id:"));
        m_idLineEdit = new QLineEdit;
        m_idLineEdit->setText(dockerDevice->data().imageId);
        m_idLineEdit->setEnabled(false);

        m_repoLabel = new QLabel(tr("Repository:"));
        m_repoLineEdit = new QLineEdit;
        m_repoLineEdit->setText(dockerDevice->data().repo);
        m_repoLineEdit->setEnabled(false);

        auto logView = new QTextBrowser;

        auto autoDetectButton = new QPushButton(tr("Auto-detect Kit Items"));
        auto undoAutoDetectButton = new QPushButton(tr("Remove Auto-Detected Kit Items"));

        connect(autoDetectButton, &QPushButton::clicked, this, [logView, dockerDevice] {
            logView->clear();
            dockerDevice->d->autoDetect(logView);
        });

        connect(undoAutoDetectButton, &QPushButton::clicked, this, [logView, dockerDevice] {
            logView->clear();
            dockerDevice->d->undoAutoDetect(logView);
        });

        using namespace Layouting;

        Form {
            m_idLabel, m_idLineEdit, Break(),
            m_repoLabel, m_repoLineEdit, Break(),
            Column {
                Space(20),
                Row { autoDetectButton, undoAutoDetectButton, Stretch() },
                new QLabel(tr("Detection Log:")),
                logView
            }
        }.attachTo(this);
    }

    void updateDeviceFromUi() final {}

private:
    QLabel *m_idLabel;
    QLineEdit *m_idLineEdit;
    QLabel *m_repoLabel;
    QLineEdit *m_repoLineEdit;
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

void DockerDevicePrivate::undoAutoDetect(QTextBrowser *log) const
{
    const QString id = q->id().toString();

    for (Kit *kit : KitManager::kits()) {
        if (kit->autoDetectionSource() == id) {
            if (log)
                log->append(tr("Removing kit: %1").arg(kit->displayName()));
            KitManager::deregisterKit(kit);
        }
    };
    for (BaseQtVersion *qtVersion : QtVersionManager::versions()) {
        if (qtVersion->autodetectionSource() == id) {
            if (log)
                log->append(tr("Removing Qt version: %1").arg(qtVersion->displayName()));
            QtVersionManager::removeVersion(qtVersion);
        }
    };

    if (log)
        log->append(tr("Toolchains not removed."));
    //        for (ToolChain *toolChain : ToolChainManager::toolChains()) {
    //            if (toolChain->autoDetectionSource() == id.toString())
    //                // FIXME: Implement
    //        };

    if (log)
        log->append(tr("Removal of auto-detected kit items finished."));
}

BaseQtVersion *DockerDevicePrivate::autoDetectQtVersion(QTextBrowser *log) const
{
    QString error;
    const QStringList candidates = {"/usr/local/bin/qmake", "/usr/bin/qmake"};
    log->append('\n' + tr("Searching Qt installation..."));
    for (const QString &candidate : candidates) {
        const FilePath qmake = q->mapToGlobalPath(FilePath::fromString(candidate));
        if (auto qtVersion = QtVersionFactory::createQtVersionFromQMakePath(qmake, false, m_data.id(), &error)) {
            QtVersionManager::addVersion(qtVersion);
            log->append(tr("Found Qt: %1").arg(qtVersion->qmakeCommand().toUserOutput()));
            return qtVersion;
        }
    }
    log->append(tr("No Qt installation found."));
    return nullptr;
}

QList<ToolChain *> DockerDevicePrivate::autoDetectToolChains(QTextBrowser *log)
{
    const QList<ToolChainFactory *> factories = ToolChainFactory::allToolChainFactories();

    QList<ToolChain *> toolChains;
    QApplication::processEvents();
    log->append('\n' + tr("Searching toolchains..."));
    for (ToolChainFactory *factory : factories) {
        const QList<ToolChain *> newToolChains = factory->autoDetect(toolChains, q->sharedFromThis());
        log->append(tr("Searching toolchains of type %1").arg(factory->displayName()));
        for (ToolChain *toolChain : newToolChains) {
            log->append(tr("Found ToolChain: %1").arg(toolChain->compilerCommand().toUserOutput()));
            ToolChainManager::registerToolChain(toolChain);
            toolChains.append(toolChain);
        }
    }
    log->append(tr("%1 new toolchains found.").arg(toolChains.size()));

    return toolChains;
}

void DockerDevicePrivate::autoDetectCMake(QTextBrowser *log)
{
    QObject *cmakeManager = ExtensionSystem::PluginManager::getObjectByName("CMakeToolManager");
    if (!cmakeManager)
        return;

    log->append('\n' + tr("Searching CMake binary..."));
    QString error;
    const QStringList candidates = {"/usr/local/bin/cmake", "/usr/bin/cmake"};
    for (const QString &candidate : candidates) {
        const FilePath cmake = q->mapToGlobalPath(FilePath::fromString(candidate));
        QTC_CHECK(q->hasLocalFileAccess());
        if (cmake.isExecutableFile()) {
            log->append(tr("Found CMake binary: %1").arg(cmake.toUserOutput()));
            const bool res = QMetaObject::invokeMethod(cmakeManager,
                                                       "registerCMakeByPath",
                                                       Q_ARG(Utils::FilePath, cmake));
            QTC_CHECK(res);
        }
    }
}

void DockerDevicePrivate::autoDetect(QTextBrowser *log)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    undoAutoDetect(log);

    tryCreateLocalFileAccess();

    if (log)
        log->append(tr("Starting auto-detection. This will take a while..."));

    QList<ToolChain *> toolChains = autoDetectToolChains(log);
    BaseQtVersion *qt = autoDetectQtVersion(log);

    autoDetectCMake(log);

    const auto initializeKit = [this, toolChains, qt](Kit *k) {
        k->setAutoDetected(false);
        k->setAutoDetectionSource(m_data.id());
        k->setUnexpandedDisplayName("%{Device:Name}");

        DeviceTypeKitAspect::setDeviceTypeId(k, Constants::DOCKER_DEVICE_TYPE);
        DeviceKitAspect::setDevice(k, q->sharedFromThis());
        for (ToolChain *tc : toolChains)
            ToolChainKitAspect::setToolChain(k, tc);
        QtSupport::QtKitAspect::setQtVersion(k, qt);

        k->setSticky(ToolChainKitAspect::id(), true);
        k->setSticky(QtSupport::QtKitAspect::id(), true);
        k->setSticky(DeviceKitAspect::id(), true);
        k->setSticky(DeviceTypeKitAspect::id(), true);
    };

    Kit *kit = KitManager::registerKit(initializeKit);
    log->append('\n' + tr("Registered Kit %1").arg(kit->displayName()));

    QApplication::restoreOverrideCursor();
}

void DockerDevice::tryCreateLocalFileAccess() const
{
    d->tryCreateLocalFileAccess();
}

void DockerDevicePrivate::tryCreateLocalFileAccess()
{
    if (!m_container.isEmpty())
        return;

    QString tempFileName;

    {
        TemporaryFile temp("qtc-docker-XXXXXX");
        temp.open();
        tempFileName = temp.fileName();
    }

    m_shell = new QtcProcess;
    // FIXME: Make mounts flexible
    m_shell->setCommand({"docker", {"run", "-i", "--cidfile=" + tempFileName,
                                    "-v", "/opt:/opt",
                                    "-v", "/data:/data",
                                    "-e", "DISPLAY=:0",
                                    "-e", "XAUTHORITY=/.Xauthority",
                                    "--net", "host",
                                    m_data.imageId, "/bin/sh"}});
    LOG("RUNNING: " << m_shell->commandLine().toUserOutput());
    m_shell->start();
    m_shell->waitForStarted();

    LOG("CHECKING: " << tempFileName);
    for (int i = 0; i <= 10; ++i) {
        QFile file(tempFileName);
        file.open(QIODevice::ReadOnly);
        m_container = QString::fromUtf8(file.readAll()).trimmed();
        if (!m_container.isEmpty()) {
            LOG("Container: " << d->m_container);
            break;
        }
        if (i == 10) {
            qWarning("Docker cid file empty.");
            return; // No
        }
        QThread::msleep(100);
    }

    QtcProcess proc;
    proc.setCommand({"docker", {"inspect", "--format={{.GraphDriver.Data.MergedDir}}", m_container}});
    //LOG(proc2.commandLine().toUserOutput());
    proc.start();
    proc.waitForFinished();
    const QString out = proc.stdOut();
    m_mergedDir = out.trimmed();
    if (m_mergedDir.endsWith('/'))
        m_mergedDir.chop(1);

    if (!QFileInfo(m_mergedDir).isWritable()) {
        MessageManager::writeFlashing(
            tr("Local write access to Docker container %1 unavailable through directory \"%2\".")
                .arg(m_container, m_mergedDir)
                    + '\n' + tr("Output: %1").arg(out)
                    + '\n' + tr("Error: %1").arg(proc.stdErr()));
    }

    m_mergedDirWatcher.addPath(m_mergedDir);
}

bool DockerDevice::hasLocalFileAccess() const
{
    return !d->m_mergedDir.isEmpty();
}

FilePath DockerDevice::mapToLocalAccess(const FilePath &filePath) const
{
    QTC_ASSERT(!d->m_mergedDir.isEmpty(), return {});
    QString path = filePath.path();
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

void DockerDevice::fromMap(const QVariantMap &map)
{
    ProjectExplorer::IDevice::fromMap(map);
    d->m_data.imageId = map.value(DockerDeviceDataImageIdKey).toString();
    d->m_data.repo = map.value(DockerDeviceDataRepoKey).toString();
    d->m_data.tag = map.value(DockerDeviceDataTagKey).toString();
    d->m_data.size = map.value(DockerDeviceDataSizeKey).toString();
}

QVariantMap DockerDevice::toMap() const
{
    QVariantMap map = ProjectExplorer::IDevice::toMap();
    map.insert(DockerDeviceDataImageIdKey, d->m_data.imageId);
    map.insert(DockerDeviceDataRepoKey, d->m_data.repo);
    map.insert(DockerDeviceDataTagKey, d->m_data.tag);
    map.insert(DockerDeviceDataSizeKey, d->m_data.size);
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
    const CommandLine cmd("test", {"-x", path});
    const int exitCode = d->runSynchronously(cmd);
    return exitCode == 0;
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
    const CommandLine cmd("test", {"-r", path, "-a", "-f", path});
    const int exitCode = d->runSynchronously(cmd);
    return exitCode == 0;
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
    const CommandLine cmd("test", {"-x", path, "-a", "-d", path});
    const int exitCode = d->runSynchronously(cmd);
    return exitCode == 0;
}

bool DockerDevice::isWritableDirectory(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    tryCreateLocalFileAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const bool res = localAccess.isReadableDir();
        LOG("WritableDirectory? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }
    const QString path = filePath.path();
    const CommandLine cmd("test", {"-x", path, "-a", "-d", path});
    const int exitCode = d->runSynchronously(cmd);
    return exitCode == 0;
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
    const CommandLine cmd("mkdir", {"-p", path});
    const int exitCode = d->runSynchronously(cmd);
    return exitCode == 0;
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
    const CommandLine cmd("test", {"-e", path});
    const int exitCode = d->runSynchronously(cmd);
    return exitCode == 0;
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
    const CommandLine cmd("rm", {filePath.path()});
    const int exitCode = d->runSynchronously(cmd);
    return exitCode == 0;
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
    const CommandLine cmd("cp", {filePath.path(), target.path()});
    const int exitCode = d->runSynchronously(cmd);
    return exitCode == 0;
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

FilePath DockerDevice::searchInPath(const FilePath &filePath) const
{
    const QString path = filePath.path();

    CommandLine dcmd{"docker", {"exec", d->m_container, "which", path}};
    QtcProcess proc;
    proc.setCommand(dcmd);
    proc.setWorkingDirectory("/tmp");
    proc.start();
    proc.waitForFinished();

    LOG("Run sync:" << dcmd.toUserOutput() << " result: " << proc.exitCode());
    QTC_ASSERT(proc.exitCode() == 0, return filePath);

    const QString output = proc.stdOut();
    return mapToGlobalPath(FilePath::fromString(output));
}

QList<FilePath> DockerDevice::directoryEntries(const FilePath &filePath,
                                               const QStringList &nameFilters,
                                               QDir::Filters filters) const
{
    QTC_ASSERT(handlesFile(filePath), return {});
    tryCreateLocalFileAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const QFileInfoList entryInfoList = QDir(localAccess.toString()).entryInfoList(nameFilters, filters);
        return Utils::transform(entryInfoList, [this](const QFileInfo &fi) {
            return mapFromLocalAccess(fi.absoluteFilePath());
        });
    }
    QTC_CHECK(false); // FIXME: Implement
    return {};
}

QByteArray DockerDevice::fileContents(const FilePath &filePath, int limit) const
{
    QTC_ASSERT(handlesFile(filePath), return {});
    tryCreateLocalFileAccess();
    if (hasLocalFileAccess())
        return mapToLocalAccess(filePath).fileContents(limit);

    QTC_CHECK(false); // FIXME: Implement
    return {};
}

void DockerDevice::runProcess(QtcProcess &process) const
{
    const FilePath workingDir = process.workingDirectory();
    const CommandLine origCmd = process.commandLine();

    CommandLine cmd{"docker", {"exec"}};
    cmd.addArgs({"-w", workingDir.path()});
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
    d->undoAutoDetect(nullptr);
}

void DockerDevicePrivate::fetchSystemEnviroment()
{
    SynchronousProcess proc;
    proc.setCommand({"env", {}});

    proc.runBlocking();

    const QString remoteOutput = proc.stdOut();
    m_cachedEnviroment = Environment(remoteOutput.split('\n', Qt::SkipEmptyParts), q->osType());
}

int DockerDevicePrivate::runSynchronously(const CommandLine &cmd) const
{
    CommandLine dcmd{"docker", {"exec", m_container}};
    dcmd.addArgs(cmd);

    QtcProcess proc;
    proc.setCommand(dcmd);
    proc.setWorkingDirectory("/tmp");
    proc.start();
    proc.waitForFinished();

    LOG("Run sync:" << dcmd.toUserOutput() << " result: " << proc.exitCode());
    return proc.exitCode();
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
        setWindowTitle(tr("Docker Image Selection"));
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
        m_log->append(tr("Running \"%1\"\n").arg(cmd.toUserOutput()));

        m_process = new QtcProcess(this);
        m_process->setCommand(cmd);

        connect(m_process, &QtcProcess::readyReadStandardOutput, [this] {
            const QString out = QString::fromUtf8(m_process->readAllStandardOutput().trimmed());
            m_log->append(out);
            for (const QString &line : out.split('\n')) {
                const QStringList parts = line.trimmed().split('\t');
                if (parts.size() != 4) {
                    m_log->append(tr("Unexpected result: %1").arg(line) + '\n');
                    continue;
                }
                auto item = new DockerImageItem;
                item->imageId = parts.at(0);
                item->repo = parts.at(1);
                item->tag = parts.at(2);
                item->size = parts.at(3);
                m_model.rootItem()->appendChild(item);
            }
            m_log->append(tr("Done."));
        });

        connect(m_process, &Utils::QtcProcess::readyReadStandardError, this, [this] {
            const QString out = tr("Error: %1").arg(m_process->stdErr());
            m_log->append(tr("Error: %1").arg(out));
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
        device->setupId(IDevice::ManuallyAdded, Utils::Id());
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
