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
#include "dockerplugin.h"

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
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/overridecursor.h>
#include <utils/pathlisteditor.h>
#include <utils/port.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/temporaryfile.h>
#include <utils/treemodel.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileSystemWatcher>
#include <QHeaderView>
#include <QHostAddress>
#include <QLoggingCategory>
#include <QNetworkInterface>
#include <QPushButton>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QTextBrowser>
#include <QThread>
#include <QToolButton>

#include <numeric>

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <sys/types.h>
#endif

//#define ALLOW_LOCAL_ACCESS 1

using namespace Core;
using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

namespace Docker {
namespace Internal {

static Q_LOGGING_CATEGORY(dockerDeviceLog, "qtc.docker.device", QtWarningMsg);
#define LOG(x) qCDebug(dockerDeviceLog) << this << x << '\n'

class DockerDeviceProcess : public ProjectExplorer::DeviceProcess
{
public:
    DockerDeviceProcess(const QSharedPointer<const IDevice> &device, QObject *parent = nullptr);
    ~DockerDeviceProcess() {}

    void start() override;
    void interrupt() override;
};

DockerDeviceProcess::DockerDeviceProcess(const QSharedPointer<const IDevice> &device,
                                          QObject *parent)
    : DeviceProcess(device, parent)
{
    setProcessMode(ProcessMode::Writer);
}

void DockerDeviceProcess::start()
{
    QTC_ASSERT(state() == QProcess::NotRunning, return);
    DockerDevice::ConstPtr dockerDevice = qSharedPointerCast<const DockerDevice>(device());
    QTC_ASSERT(dockerDevice, return);

    connect(this, &DeviceProcess::readyReadStandardOutput, this, [this] {
        MessageManager::writeSilently(QString::fromLocal8Bit(readAllStandardError()));
    });
    connect(this, &DeviceProcess::readyReadStandardError, this, [this] {
        MessageManager::writeDisrupting(QString::fromLocal8Bit(readAllStandardError()));
    });

    CommandLine command = commandLine();
    command.setExecutable(
        command.executable().withNewPath(dockerDevice->mapToDevicePath(command.executable())));
    setCommand(command);

    LOG("Running process:" << command.toUserOutput() << "in" << workingDirectory().toUserOutput());
    QtcProcess::start();
}

void DockerDeviceProcess::interrupt()
{
    device()->signalOperation()->interruptProcess(processId());
}

class DockerPortsGatheringMethod : public PortsGatheringMethod
{
    CommandLine commandLine(QAbstractSocket::NetworkLayerProtocol protocol) const override
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
        return {"sed", "-e 's/.*: [[:xdigit:]]*:\\([[:xdigit:]]\\{4\\}\\).*/\\1/g' /proc/net/tcp*",
                CommandLine::Raw};
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
    void listAutoDetected() const;

    void setSharedId(const QString &sharedId) { m_sharedId = sharedId; }
    void setSearchPaths(const FilePaths &searchPaths) { m_searchPaths = searchPaths; }

private:
    QtVersions autoDetectQtVersions() const;
    QList<ToolChain *> autoDetectToolChains();
    void autoDetectCMake();
    void autoDetectDebugger();

    KitDetector *q;
    IDevice::ConstPtr m_device;
    QString m_sharedId;
    FilePaths m_searchPaths;
};

KitDetector::KitDetector(const IDevice::ConstPtr &device)
    : d(new KitDetectorPrivate(this, device))
{}

KitDetector::~KitDetector()
{
    delete d;
}

void KitDetector::autoDetect(const QString &sharedId, const FilePaths &searchPaths) const
{
    d->setSharedId(sharedId);
    d->setSearchPaths(searchPaths);
    d->autoDetect();
}

void KitDetector::undoAutoDetect(const QString &sharedId) const
{
    d->setSharedId(sharedId);
    d->undoAutoDetect();
}

void KitDetector::listAutoDetected(const QString &sharedId) const
{
    d->setSharedId(sharedId);
    d->listAutoDetected();
}

class DockerDevicePrivate : public QObject
{
    Q_DECLARE_TR_FUNCTIONS(Docker::Internal::DockerDevice)

public:
    DockerDevicePrivate(DockerDevice *parent) : q(parent)
    {
#ifdef ALLOW_LOCAL_ACCESS
        connect(&m_mergedDirWatcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &path) {
            Q_UNUSED(path)
            LOG("Container watcher change, file: " << path);
        });
        connect(&m_mergedDirWatcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString &path) {
            Q_UNUSED(path)
            LOG("Container watcher change, directory: " << path);
        });
#endif
    }

    ~DockerDevicePrivate() { stopCurrentContainer(); }

    bool runInContainer(const CommandLine &cmd) const;
    bool runInShell(const CommandLine &cmd) const;
    QByteArray outputForRunInShell(const CommandLine &cmd) const;

    void updateContainerAccess();
    void updateFileSystemAccess();

    void startContainer();
    void stopCurrentContainer();
    void fetchSystemEnviroment();

    DockerDevice *q;
    DockerDeviceData m_data;

    // For local file access
    QPointer<QtcProcess> m_shell;
    mutable QMutex m_shellMutex;
    QString m_container;

#ifdef ALLOW_LOCAL_ACCESS
    QString m_mergedDir;
    QFileSystemWatcher m_mergedDirWatcher;
#endif

    Environment m_cachedEnviroment;

    bool m_useFind = true;  // prefer find over ls and hacks, but be able to use ls as fallback
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

        auto repoLabel = new QLabel(tr("Repository:"));
        m_repoLineEdit = new QLineEdit;
        m_repoLineEdit->setText(data.repo);
        m_repoLineEdit->setEnabled(false);

        auto tagLabel = new QLabel(tr("Tag:"));
        m_tagLineEdit = new QLineEdit;
        m_tagLineEdit->setText(data.tag);
        m_tagLineEdit->setEnabled(false);

        auto idLabel = new QLabel(tr("Image ID:"));
        m_idLineEdit = new QLineEdit;
        m_idLineEdit->setText(data.imageId);
        m_idLineEdit->setEnabled(false);

        auto daemonStateLabel = new QLabel(tr("Daemon state:"));
        m_daemonReset = new QToolButton;
        m_daemonReset->setToolTip(tr("Clears detected daemon state. "
            "It will be automatically re-evaluated next time access is needed."));

        m_daemonState = new QLabel;
        updateDaemonStateTexts();

        connect(m_daemonReset, &QToolButton::clicked, this, [this, dockerDevice] {
            DockerPlugin::setGlobalDaemonState(Utils::nullopt);
            updateDaemonStateTexts();
        });

        m_runAsOutsideUser = new QCheckBox(tr("Run as outside user"));
        m_runAsOutsideUser->setToolTip(tr("Uses user ID and group ID of the user running Qt Creator "
                                          "in the Docker container."));
        m_runAsOutsideUser->setChecked(data.useLocalUidGid);
        m_runAsOutsideUser->setEnabled(HostOsInfo::isLinuxHost());

        connect(m_runAsOutsideUser, &QCheckBox::toggled, this, [&data](bool on) {
            data.useLocalUidGid = on;
        });

#ifdef ALLOW_LOCAL_ACCESS
        // This tries to find the directory in the host file system that corresponds to the
        // docker container root file system, which is a merge of the layers from the
        // container image and the volumes mapped using -v on container startup.
        //
        // Accessing files there is much faster than using 'docker exec', but conceptually
        // only works on Linux, and is restricted there by proper matching of user
        // permissions between host and container.
        m_usePathMapping = new QCheckBox(tr("Use local file path mapping"));
        m_usePathMapping->setToolTip(tr("Maps docker filesystem to a local directory."));
        m_usePathMapping->setChecked(data.useFilePathMapping);
        m_usePathMapping->setEnabled(HostOsInfo::isLinuxHost());
        connect(m_usePathMapping, &QCheckBox::toggled, this, [&, dockerDevice](bool on) {
            data.useFilePathMapping = on;
            dockerDevice->updateContainerAccess();
        });
#endif

        m_pathsListEdit = new PathListEditor;
        m_pathsListEdit->setToolTip(tr("Maps paths in this list one-to-one to the "
                                       "Docker container."));
        m_pathsListEdit->setPathList(data.mounts);

        connect(m_pathsListEdit, &PathListEditor::changed, this, [dockerDevice, this]() {
            dockerDevice->setMounts(m_pathsListEdit->pathList());
        });

        auto logView = new QTextBrowser;
        connect(&m_kitItemDetector, &KitDetector::logOutput,
                logView, &QTextBrowser::append);

        auto autoDetectButton = new QPushButton(tr("Auto-detect Kit Items"));
        auto undoAutoDetectButton = new QPushButton(tr("Remove Auto-Detected Kit Items"));
        auto listAutoDetectedButton = new QPushButton(tr("List Auto-Detected Kit Items"));

        auto searchDirsComboBox = new QComboBox;
        searchDirsComboBox->addItem(tr("Search in PATH"));
        searchDirsComboBox->addItem(tr("Search in selected directories"));

        auto searchDirsLineEdit = new QLineEdit;
        searchDirsLineEdit->setText("/usr/bin;/opt");
        searchDirsLineEdit->setToolTip(
            tr("Select the paths in the Docker image that should be scanned for Kit entries"));

        auto searchPaths = [this, searchDirsComboBox, searchDirsLineEdit, dockerDevice] {
            FilePaths paths;
            if (searchDirsComboBox->currentIndex() == 0) {
                paths = dockerDevice->systemEnvironment().path();
            } else {
                for (const QString &path : searchDirsLineEdit->text().split(';'))
                    paths.append(FilePath::fromString(path.trimmed()));
            }
            paths = Utils::transform(paths, [dockerDevice](const FilePath &path) {
                return dockerDevice->mapToGlobalPath(path);
            });
            return paths;
        };

        connect(autoDetectButton, &QPushButton::clicked, this,
                [this, logView, data, dockerDevice, searchPaths] {
            logView->clear();
            dockerDevice->updateContainerAccess();

            m_kitItemDetector.autoDetect(data.autodetectId(), searchPaths());

            if (DockerPlugin::isDaemonRunning().value_or(false) == false)
                logView->append(tr("Docker daemon appears to be not running."));
            else
                logView->append(tr("Docker daemon appears to be running."));
            updateDaemonStateTexts();
        });

        connect(undoAutoDetectButton, &QPushButton::clicked, this, [this, logView, data] {
            logView->clear();
            m_kitItemDetector.undoAutoDetect(data.autodetectId());
        });

        connect(listAutoDetectedButton, &QPushButton::clicked, this, [this, logView, data] {
            logView->clear();
            m_kitItemDetector.listAutoDetected(data.autodetectId());
        });

        using namespace Layouting;

        Form {
            repoLabel, m_repoLineEdit, Break(),
            tagLabel, m_tagLineEdit, Break(),
            idLabel, m_idLineEdit, Break(),
            daemonStateLabel, m_daemonReset, m_daemonState, Break(),
            m_runAsOutsideUser, Break(),
#ifdef ALLOW_LOCAL_ACCESS
            m_usePathMapping, Break(),
#endif
            Column {
                new QLabel(tr("Paths to mount:")),
                m_pathsListEdit,
            }, Break(),
            Column {
                Space(20),
                Row {
                    searchDirsComboBox,
                    searchDirsLineEdit
                },
                Row {
                    autoDetectButton,
                    undoAutoDetectButton,
                    listAutoDetectedButton,
                    Stretch(),
                },
                new QLabel(tr("Detection log:")),
                logView
            }
        }.attachTo(this);

        searchDirsLineEdit->setVisible(false);
        auto updateDirectoriesLineEdit = [this, searchDirsLineEdit](int index) {
            searchDirsLineEdit->setVisible(index == 1);
        };
        QObject::connect(searchDirsComboBox, qOverload<int>(&QComboBox::activated),
                         this, updateDirectoriesLineEdit);
    }

    void updateDeviceFromUi() final {}
    void updateDaemonStateTexts();

private:
    QLineEdit *m_repoLineEdit;
    QLineEdit *m_tagLineEdit;
    QLineEdit *m_idLineEdit;
    QToolButton *m_daemonReset;
    QLabel *m_daemonState;
    QCheckBox *m_runAsOutsideUser;
#ifdef ALLOW_LOCAL_ACCESS
    QCheckBox *m_usePathMapping;
#endif
    Utils::PathListEditor *m_pathsListEdit;

    KitDetector m_kitItemDetector;
};

IDeviceWidget *DockerDevice::createWidget()
{
    return new DockerDeviceWidget(sharedFromThis());
}

Tasks DockerDevice::validate() const
{
    Tasks result;
    if (d->m_data.mounts.isEmpty()) {
        result << Task(Task::Error,
                       tr("The Docker device has not set up shared directories."
                          "This will not work for building."),
                       {}, -1, {});
    }
    return result;
}


// DockerDeviceData

QString DockerDeviceData::dockerId() const
{
    if (repo == "<none>")
        return imageId;

    if (tag == "<none>")
        return repo;

    return repo + ':' + tag;
}

// DockerDevice

DockerDevice::DockerDevice(const DockerDeviceData &data)
    : d(new DockerDevicePrivate(this))
{
    d->m_data = data;

    setDisplayType(tr("Docker"));
    setOsType(OsTypeOtherUnix);
    setDefaultDisplayName(tr("Docker Image"));;
    setDisplayName(tr("Docker Image \"%1\" (%2)").arg(data.dockerId()).arg(data.imageId));
    setAllowEmptyCommand(true);

    setOpenTerminal([this](const Environment &env, const FilePath &workingDir) {
        Q_UNUSED(env); // TODO: That's the runnable's environment in general. Use it via -e below.
        updateContainerAccess();
        if (d->m_container.isEmpty()) {
            MessageManager::writeDisrupting(tr("Error starting remote shell. No container"));
            return;
        }

        QtcProcess *proc = new QtcProcess;
        proc->setTerminalMode(TerminalMode::On);

        QObject::connect(proc, &QtcProcess::finished, proc, &QObject::deleteLater);

        QObject::connect(proc, &DeviceProcess::errorOccurred, [proc] {
            MessageManager::writeDisrupting(tr("Error starting remote shell."));
            proc->deleteLater();
        });

        const QString wd = workingDir.isEmpty() ? "/" : workingDir.path();
        proc->setCommand({"docker", {"exec", "-it", "-w", wd, d->m_container, "/bin/sh"}});
        proc->setEnvironment(Environment::systemEnvironment()); // The host system env. Intentional.
        proc->start();
    });

    addDeviceAction({tr("Open Shell in Container"), [](const IDevice::Ptr &device, QWidget *) {
                         device->openTerminal(device->systemEnvironment(), FilePath());
    }});
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
    for (QtVersion *qtVersion : QtVersionManager::versions()) {
        if (qtVersion->detectionSource() == m_sharedId) {
            emit q->logOutput(tr("Removed \"%1\"").arg(qtVersion->displayName()));
            QtVersionManager::removeVersion(qtVersion);
        }
    };

    emit q->logOutput('\n' + tr("Removing toolchain entries..."));
    const Toolchains toolchains = ToolChainManager::toolchains();
    for (ToolChain *toolChain : toolchains) {
        if (toolChain && toolChain->detectionSource() == m_sharedId) {
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

void KitDetectorPrivate::listAutoDetected() const
{
    emit q->logOutput(tr("Start listing auto-detected items associated with this docker image."));

    emit q->logOutput('\n' + tr("Kits:"));
    for (Kit *kit : KitManager::kits()) {
        if (kit->autoDetectionSource() == m_sharedId)
            emit q->logOutput(kit->displayName());
    };

    emit q->logOutput('\n' + tr("Qt versions:"));
    for (QtVersion *qtVersion : QtVersionManager::versions()) {
        if (qtVersion->detectionSource() == m_sharedId)
            emit q->logOutput(qtVersion->displayName());
    };

    emit q->logOutput('\n' + tr("Toolchains:"));
    for (ToolChain *toolChain : ToolChainManager::toolchains()) {
        if (toolChain->detectionSource() == m_sharedId)
            emit q->logOutput(toolChain->displayName());
    };

    if (QObject *cmakeManager = ExtensionSystem::PluginManager::getObjectByName("CMakeToolManager")) {
        QString logMessage;
        const bool res = QMetaObject::invokeMethod(cmakeManager,
                                                   "listDetectedCMake",
                                                   Q_ARG(QString, m_sharedId),
                                                   Q_ARG(QString *, &logMessage));
        QTC_CHECK(res);
        emit q->logOutput('\n' + logMessage);
    }

    if (QObject *debuggerPlugin = ExtensionSystem::PluginManager::getObjectByName("DebuggerPlugin")) {
        QString logMessage;
        const bool res = QMetaObject::invokeMethod(debuggerPlugin,
                                                   "listDetectedDebuggers",
                                                   Q_ARG(QString, m_sharedId),
                                                   Q_ARG(QString *, &logMessage));
        QTC_CHECK(res);
        emit q->logOutput('\n' + logMessage);
    }

    emit q->logOutput('\n' + tr("Listing of previously auto-detected kit items finished.") + "\n\n");
}

QtVersions KitDetectorPrivate::autoDetectQtVersions() const
{
    QtVersions qtVersions;

    QString error;

    const auto handleQmake = [this, &qtVersions, &error](const FilePath &qmake) {
        if (QtVersion *qtVersion = QtVersionFactory::createQtVersionFromQMakePath(qmake, false, m_sharedId, &error)) {
            qtVersions.append(qtVersion);
            QtVersionManager::addVersion(qtVersion);
            emit q->logOutput(tr("Found \"%1\"").arg(qtVersion->qmakeFilePath().toUserOutput()));
        }
        return true;
    };

    emit q->logOutput(tr("Searching for qmake executables..."));

    const QStringList candidates = {"qmake-qt6", "qmake-qt5", "qmake"};
    for (const FilePath &searchPath : m_searchPaths) {
        searchPath.iterateDirectory(handleQmake, {candidates, QDir::Files | QDir::Executable,
                                                  QDirIterator::Subdirectories});
    }

    if (!error.isEmpty())
        emit q->logOutput(tr("Error: %1.").arg(error));
    if (qtVersions.isEmpty())
        emit q->logOutput(tr("No Qt installation found."));
    return qtVersions;
}

Toolchains KitDetectorPrivate::autoDetectToolChains()
{
    const QList<ToolChainFactory *> factories = ToolChainFactory::allToolChainFactories();

    Toolchains alreadyKnown = ToolChainManager::toolchains();
    Toolchains allNewToolChains;
    QApplication::processEvents();
    emit q->logOutput('\n' + tr("Searching toolchains..."));
    for (ToolChainFactory *factory : factories) {
        emit q->logOutput(tr("Searching toolchains of type %1").arg(factory->displayName()));
        const ToolchainDetector detector(alreadyKnown, m_device);
        const Toolchains newToolChains = factory->autoDetect(detector);
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

    QString logMessage;
    const bool res = QMetaObject::invokeMethod(cmakeManager,
                                               "autoDetectCMakeForDevice",
                                               Q_ARG(Utils::FilePaths, m_searchPaths),
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

    QString logMessage;
    const bool res = QMetaObject::invokeMethod(debuggerPlugin,
                                               "autoDetectDebuggersForDevice",
                                               Q_ARG(Utils::FilePaths, m_searchPaths),
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

    const Toolchains toolchains = autoDetectToolChains();
    const QtVersions qtVersions = autoDetectQtVersions();

    autoDetectCMake();
    autoDetectDebugger();

    const auto initializeKit = [this, toolchains, qtVersions](Kit *k) {
        k->setAutoDetected(false);
        k->setAutoDetectionSource(m_sharedId);
        k->setUnexpandedDisplayName("%{Device:Name}");

        DeviceTypeKitAspect::setDeviceTypeId(k, Constants::DOCKER_DEVICE_TYPE);
        DeviceKitAspect::setDevice(k, m_device);

        QtVersion *qt = nullptr;
        if (!qtVersions.isEmpty()) {
            qt = qtVersions.at(0);
            QtSupport::QtKitAspect::setQtVersion(k, qt);
        }
        Toolchains toolchainsToSet;
        toolchainsToSet = ToolChainManager::toolchains([qt, this](const ToolChain *tc){
             return tc->detectionSource() == m_sharedId
                    && (!qt || qt->qtAbis().contains(tc->targetAbi()));
        });
        for (ToolChain *toolChain : toolchainsToSet)
            ToolChainKitAspect::setToolChain(k, toolChain);

        k->setSticky(ToolChainKitAspect::id(), true);
        k->setSticky(QtSupport::QtKitAspect::id(), true);
        k->setSticky(DeviceKitAspect::id(), true);
        k->setSticky(DeviceTypeKitAspect::id(), true);
    };

    Kit *kit = KitManager::registerKit(initializeKit);
    emit q->logOutput('\n' + tr("Registered kit %1").arg(kit->displayName()));

    QApplication::restoreOverrideCursor();
}

void DockerDevice::updateContainerAccess() const
{
    d->updateContainerAccess();
}

void DockerDevicePrivate::stopCurrentContainer()
{
    if (m_container.isEmpty() || !DockerPlugin::isDaemonRunning().value_or(false))
        return;

    if (m_shell) {
        QMutexLocker l(&m_shellMutex);
        m_shell->write("exit\n");
        m_shell->waitForFinished(2000);
        if (m_shell->state() == QProcess::NotRunning) {
            LOG("Clean exit via shell");
            m_container.clear();
#ifdef ALLOW_LOCAL_ACCESS
            m_mergedDir.clear();
#endif
            delete m_shell;
            m_shell = nullptr;
            return;
        }
    }

    QtcProcess proc;
    proc.setCommand({"docker", {"container", "stop", m_container}});

    m_container.clear();
#ifdef ALLOW_LOCAL_ACCESS
    m_mergedDir.clear();
#endif

    proc.runBlocking();
}

static QString getLocalIPv4Address()
{
    const QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (auto &a : addresses) {
        if (a.isInSubnet(QHostAddress("192.168.0.0"), 16))
            return a.toString();
        if (a.isInSubnet(QHostAddress("10.0.0.0"), 8))
            return a.toString();
        if (a.isInSubnet(QHostAddress("172.16.0.0"), 12))
            return a.toString();
    }
    return QString();
}

void DockerDevicePrivate::startContainer()
{
    const QString display = HostOsInfo::isLinuxHost() ? QString(":0")
                                                      : QString(getLocalIPv4Address() + ":0.0");
    CommandLine dockerCreate{"docker", {"create",
                                        "-i",
                                        "--rm",
                                        "-e", QString("DISPLAY=%1").arg(display),
                                        "-e", "XAUTHORITY=/.Xauthority",
                                        "--net", "host"}};

#ifdef Q_OS_UNIX
    // no getuid() and getgid() on Windows.
    if (m_data.useLocalUidGid)
        dockerCreate.addArgs({"-u", QString("%1:%2").arg(getuid()).arg(getgid())});
#endif

    for (QString mount : qAsConst(m_data.mounts)) {
        if (mount.isEmpty())
            continue;
        mount = q->mapToDevicePath(FilePath::fromUserInput(mount));
        dockerCreate.addArgs({"-v", mount + ':' + mount});
    }
    FilePath dumperPath = FilePath::fromString("/tmp/qtcreator/debugger");
    dockerCreate.addArgs({"-v", q->debugDumperPath().toUserOutput() + ':' + dumperPath.path()});
    q->setDebugDumperPath(dumperPath);

    dockerCreate.addArgs({"--entrypoint", "/bin/sh", m_data.dockerId()});

    LOG("RUNNING: " << dockerCreate.toUserOutput());
    QtcProcess createProcess;
    createProcess.setCommand(dockerCreate);
    createProcess.runBlocking();

    if (createProcess.result() != QtcProcess::FinishedWithSuccess)
        return;

    m_container = createProcess.stdOut().trimmed();
    if (m_container.isEmpty())
        return;
    LOG("Container via process: " << m_container);

    CommandLine dockerRun{"docker", {"container" , "start", "-i", "-a", m_container}};
    LOG("RUNNING: " << dockerRun.toUserOutput());
    QPointer<QtcProcess> shell = new QtcProcess;
    shell->setProcessMode(ProcessMode::Writer);
    connect(shell, &QtcProcess::finished, this, [this, shell] {
        LOG("\nSHELL FINISHED\n");
        QTC_ASSERT(shell, return);
        const int exitCode = shell->exitCode();
        LOG("RES: " << shell->result()
            << " EXIT CODE: " << exitCode
            << " STDOUT: " << shell->readAllStandardOutput()
            << " STDERR: " << shell->readAllStandardError());
        // negative exit codes indicate problems like no docker daemon, missing permissions,
        // no shell and seem to result in exit codes 125+
        if (exitCode > 120) {
            DockerPlugin::setGlobalDaemonState(false);
            LOG("DOCKER DAEMON NOT RUNNING?");
            MessageManager::writeFlashing(tr("Docker Daemon appears to be not running. "
                                             "Verify daemon is up and running and reset the "
                                             "docker daemon on the docker device settings page "
                                             "or restart Qt Creator."));
        }
    });

    QTC_ASSERT(!m_shell, delete m_shell);
    m_shell = shell;
    m_shell->setCommand(dockerRun);
    m_shell->start();
    m_shell->waitForStarted();

    if (!m_shell->isRunning()) {
        DockerPlugin::setGlobalDaemonState(false);
        LOG("DOCKER SHELL FAILED");
        return;
    }

    DockerPlugin::setGlobalDaemonState(true);
}

void DockerDevicePrivate::updateContainerAccess()
{
    if (!m_container.isEmpty())
        return;

    if (DockerPlugin::isDaemonRunning().value_or(true) == false)
        return;

    if (!m_shell)
        startContainer();

    updateFileSystemAccess();
}

void DockerDevicePrivate::updateFileSystemAccess()
{
#ifdef ALLOW_LOCAL_ACCESS
    if (!m_data.useFilePathMapping) {
        // Direct access was used previously, but is not wanted anymore.
        if (!m_mergedDir.isEmpty()) {
            m_mergedDirWatcher.removePath(m_mergedDir);
            m_mergedDir.clear();
        }
        return;
    }

    if (!DockerPlugin::isDaemonRunning().value_or(false))
        return;

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
                    + '\n' + tr("Output: \"%1\"").arg(out)
                    + '\n' + tr("Error: \"%1\"").arg(proc.stdErr()));
        if (!HostOsInfo::isLinuxHost()) {
            // Disabling merged layer access. This is not supported and anything
            // related to accessing merged layers on Windows or macOS fails due
            // to the need of using wsl or a named pipe.
            // TODO investigate how to make it possible nevertheless.
            m_mergedDir.clear();
            MessageManager::writeSilently(tr("This is expected on Windows and macOS."));
            return;
        }
    }

    m_mergedDirWatcher.addPath(m_mergedDir);
#endif
}

bool DockerDevice::hasLocalFileAccess() const
{
#ifdef ALLOW_LOCAL_ACCESS
    static const bool denyLocalAccess = qEnvironmentVariableIsSet("QTC_DOCKER_DENY_LOCAL_ACCESS");
    if (denyLocalAccess)
        return false;
    return !d->m_mergedDir.isEmpty();
#else
    return false;
#endif
}

void DockerDevice::setMounts(const QStringList &mounts) const
{
    d->m_data.mounts = mounts;
    d->stopCurrentContainer(); // Force re-start with new mounts.
}

FilePath DockerDevice::mapToLocalAccess(const FilePath &filePath) const
{
#ifdef ALLOW_LOCAL_ACCESS
    QTC_ASSERT(!d->m_mergedDir.isEmpty(), return {});
    QString path = filePath.path();
    for (const QString &mount : qAsConst(d->m_data.mounts)) {
        if (path.startsWith(mount + '/'))
            return FilePath::fromString(path);
    }
    if (path.startsWith('/'))
        return FilePath::fromString(d->m_mergedDir + path);
    return FilePath::fromString(d->m_mergedDir + '/' + path);
#else
    QTC_CHECK(false);
    Q_UNUSED(filePath)
    return {};
#endif
}

FilePath DockerDevice::mapFromLocalAccess(const FilePath &filePath) const
{
    QTC_ASSERT(!filePath.needsDevice(), return {});
    return mapFromLocalAccess(filePath.toString());
}

FilePath DockerDevice::mapFromLocalAccess(const QString &filePath) const
{
#ifdef ALLOW_LOCAL_FILE_ACCESS
    QTC_ASSERT(!d->m_mergedDir.isEmpty(), return {});
    QTC_ASSERT(filePath.startsWith(d->m_mergedDir), return FilePath::fromString(filePath));
    return mapToGlobalPath(FilePath::fromString(filePath.mid(d->m_mergedDir.size())));
#else
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return {};
#endif
}

const char DockerDeviceDataImageIdKey[] = "DockerDeviceDataImageId";
const char DockerDeviceDataRepoKey[] = "DockerDeviceDataRepo";
const char DockerDeviceDataTagKey[] = "DockerDeviceDataTag";
const char DockerDeviceDataSizeKey[] = "DockerDeviceDataSize";
const char DockerDeviceUseOutsideUser[] = "DockerDeviceUseUidGid";
const char DockerDeviceUseFilePathMapping[] = "DockerDeviceFilePathMapping";
const char DockerDeviceMappedPaths[] = "DockerDeviceMappedPaths";

void DockerDevice::fromMap(const QVariantMap &map)
{
    ProjectExplorer::IDevice::fromMap(map);
    d->m_data.repo = map.value(DockerDeviceDataRepoKey).toString();
    d->m_data.tag = map.value(DockerDeviceDataTagKey).toString();
    d->m_data.imageId = map.value(DockerDeviceDataImageIdKey).toString();
    d->m_data.size = map.value(DockerDeviceDataSizeKey).toString();
    d->m_data.useLocalUidGid = map.value(DockerDeviceUseOutsideUser,
                                         HostOsInfo::isLinuxHost()).toBool();
    d->m_data.useFilePathMapping = map.value(DockerDeviceUseFilePathMapping,
                                             HostOsInfo::isLinuxHost()).toBool();
    d->m_data.mounts = map.value(DockerDeviceMappedPaths).toStringList();
}

QVariantMap DockerDevice::toMap() const
{
    QVariantMap map = ProjectExplorer::IDevice::toMap();
    map.insert(DockerDeviceDataRepoKey, d->m_data.repo);
    map.insert(DockerDeviceDataTagKey, d->m_data.tag);
    map.insert(DockerDeviceDataImageIdKey, d->m_data.imageId);
    map.insert(DockerDeviceDataSizeKey, d->m_data.size);
    map.insert(DockerDeviceUseOutsideUser, d->m_data.useLocalUidGid);
    map.insert(DockerDeviceUseFilePathMapping, d->m_data.useFilePathMapping);
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
    result.setHost(d->m_data.dockerId());
    result.setPath(pathOnDevice.path());
    return result;
}

QString DockerDevice::mapToDevicePath(const Utils::FilePath &globalPath) const
{
    // make sure to convert windows style paths to unix style paths with the file system case:
    // C:/dev/src -> /c/dev/src
    const FilePath normalized = FilePath::fromString(globalPath.path()).normalizedPathName();
    QString path = normalized.path();
    if (normalized.startsWithDriveLetter()) {
        const QChar lowerDriveLetter = path.at(0).toLower();
        path = '/' + lowerDriveLetter + path.mid(2); // strip C:
    }
    return path;
}

bool DockerDevice::handlesFile(const FilePath &filePath) const
{
    return filePath.scheme() == "docker" && filePath.host() == d->m_data.dockerId();
}

bool DockerDevice::isExecutableFile(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    updateContainerAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const bool res = localAccess.isExecutableFile();
        LOG("Executable? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }
    const QString path = filePath.path();
    return d->runInShell({"test", {"-x", path}});
}

bool DockerDevice::isReadableFile(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    updateContainerAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const bool res = localAccess.isReadableFile();
        LOG("ReadableFile? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }
    const QString path = filePath.path();
    return d->runInShell({"test", {"-r", path, "-a", "-f", path}});
}

bool DockerDevice::isWritableFile(const Utils::FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    updateContainerAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const bool res = localAccess.isWritableFile();
        LOG("WritableFile? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }
    const QString path = filePath.path();
    return d->runInShell({"test", {"-w", path, "-a", "-f", path}});
}

bool DockerDevice::isReadableDirectory(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    updateContainerAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const bool res = localAccess.isReadableDir();
        LOG("ReadableDirectory? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }
    const QString path = filePath.path();
    return d->runInShell({"test", {"-r", path, "-a", "-d", path}});
}

bool DockerDevice::isWritableDirectory(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    updateContainerAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const bool res = localAccess.isWritableDir();
        LOG("WritableDirectory? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }
    const QString path = filePath.path();
    return d->runInShell({"test", {"-w", path, "-a", "-d", path}});
}

bool DockerDevice::isFile(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    updateContainerAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const bool res = localAccess.isFile();
        LOG("IsFile? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }
    const QString path = filePath.path();
    return d->runInShell({"test", {"-f", path}});
}

bool DockerDevice::isDirectory(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    updateContainerAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const bool res = localAccess.isDir();
        LOG("IsDirectory? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }
    const QString path = filePath.path();
    return d->runInShell({"test", {"-d", path}});
}

bool DockerDevice::createDirectory(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    updateContainerAccess();
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
    updateContainerAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const bool res = localAccess.exists();
        LOG("Exists? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }
    const QString path = filePath.path();
    return d->runInShell({"test", {"-e", path}});
}

bool DockerDevice::ensureExistingFile(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    updateContainerAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const bool res = localAccess.ensureExistingFile();
        LOG("Ensure existing file? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }
    const QString path = filePath.path();
    return d->runInShell({"touch", {path}});
}

bool DockerDevice::removeFile(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    updateContainerAccess();
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
    updateContainerAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const bool res = localAccess.removeRecursively();
        LOG("Remove recursively? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }

    const QString path = filePath.cleanPath().path();
    // We are expecting this only to be called in a context of build directories or similar.
    // Chicken out in some cases that _might_ be user code errors.
    QTC_ASSERT(path.startsWith('/'), return false);
    const int levelsNeeded = path.startsWith("/home/") ? 4 : 3;
    QTC_ASSERT(path.count('/') >= levelsNeeded, return false);

    return d->runInContainer({"rm", {"-rf", "--", path}});
}

bool DockerDevice::copyFile(const FilePath &filePath, const FilePath &target) const
{
    QTC_ASSERT(handlesFile(filePath), return false);
    QTC_ASSERT(handlesFile(target), return false);
    updateContainerAccess();
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
    updateContainerAccess();
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
    updateContainerAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const QDateTime res = localAccess.lastModified();
        LOG("Last modified? " << filePath.toUserOutput() << localAccess.toUserOutput() << res);
        return res;
    }

    const QByteArray output = d->outputForRunInShell({"stat", {"-c", "%Y", filePath.path()}});
    qint64 secs = output.toLongLong();
    const QDateTime dt = QDateTime::fromSecsSinceEpoch(secs, Qt::UTC);
    return dt;
}

FilePath DockerDevice::symLinkTarget(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return {});
    updateContainerAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        const FilePath target = localAccess.symLinkTarget();
        LOG("SymLinkTarget? " << filePath.toUserOutput() << localAccess.toUserOutput() << target);
        if (target.isEmpty())
            return {};
        return mapToGlobalPath(target);
    }

    const QByteArray output = d->outputForRunInShell({"readlink", {"-n", "-e", filePath.path()}});
    const QString out = QString::fromUtf8(output.data(), output.size());
    return out.isEmpty() ? FilePath() : filePath.withNewPath(out);
}

qint64 DockerDevice::fileSize(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return -1);
    updateContainerAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        LOG("File size? " << filePath.toUserOutput() << localAccess.toUserOutput() << localAccess.fileSize());
        return localAccess.fileSize();
    }

    const QByteArray output = d->outputForRunInShell({"stat", {"-c", "%s", filePath.path()}});
    return output.toLongLong();
}

QFileDevice::Permissions DockerDevice::permissions(const FilePath &filePath) const
{
    QTC_ASSERT(handlesFile(filePath), return {});
    updateContainerAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        LOG("Permissions? " << filePath.toUserOutput() << localAccess.toUserOutput() << localAccess.permissions());
        return localAccess.permissions();
    }

    const QByteArray output = d->outputForRunInShell({"stat", {"-c", "%a", filePath.path()}});
    const uint bits = output.toUInt(nullptr, 8);
    QFileDevice::Permissions perm = {};
#define BIT(n, p) if (bits & (1<<n)) perm |= QFileDevice::p
    BIT(0, ExeOther);
    BIT(1, WriteOther);
    BIT(2, ReadOther);
    BIT(3, ExeGroup);
    BIT(4, WriteGroup);
    BIT(5, ReadGroup);
    BIT(6, ExeUser);
    BIT(7, WriteUser);
    BIT(8, ReadUser);
#undef BIT
    return perm;
}

bool DockerDevice::setPermissions(const FilePath &filePath, QFileDevice::Permissions permissions) const
{
    QTC_ASSERT(handlesFile(filePath), return {});
    updateContainerAccess();
    if (hasLocalFileAccess()) {
        const FilePath localAccess = mapToLocalAccess(filePath);
        LOG("Set permissions? " << filePath.toUserOutput() << localAccess.toUserOutput() << localAccess.permissions());
        return localAccess.setPermissions(permissions);
    }

    QTC_CHECK(false); // FIXME: Implement.
    return false;
}

void DockerDevice::iterateWithFind(const FilePath &filePath,
                                   const std::function<bool(const Utils::FilePath &)> &callBack,
                                   const FileFilter &filter) const
{
    QTC_ASSERT(callBack, return);
    QTC_CHECK(filePath.isAbsolutePath());
    QStringList arguments{filePath.path()};

    const QDir::Filters filters = filter.fileFilters;
    if (filters & QDir::NoSymLinks)
        arguments.prepend("-H");
    else
        arguments.prepend("-L");

    if (!filter.iteratorFlags.testFlag(QDirIterator::Subdirectories))
        arguments.append({"-maxdepth", "1"});

    QStringList filterOptions;
    if (filters & QDir::Dirs)
        filterOptions << "-type" << "d";
    if (filters & QDir::Files) {
        if (!filterOptions.isEmpty())
            filterOptions << "-o";
        filterOptions << "-type" << "f";
    }

    if (filters & QDir::Readable)
        filterOptions << "-readable";
    if (filters & QDir::Writable)
        filterOptions << "-writable";
    if (filters & QDir::Executable)
        filterOptions << "-executable";

    QTC_CHECK(filters ^ QDir::AllDirs);
    QTC_CHECK(filters ^ QDir::Drives);
    QTC_CHECK(filters ^ QDir::NoDot);
    QTC_CHECK(filters ^ QDir::NoDotDot);
    QTC_CHECK(filters ^ QDir::Hidden);
    QTC_CHECK(filters ^ QDir::System);

    const QString nameOption = (filters & QDir::CaseSensitive) ? QString{"-name"}
                                                               : QString{"-iname"};
    if (!filter.nameFilters.isEmpty()) {
        const QRegularExpression oneChar("\\[.*?\\]");
        bool addedFirst = false;
        for (const QString &current : filter.nameFilters) {
            if (current.indexOf(oneChar) != -1) {
                LOG("Skipped" << current << "due to presence of [] wildcard");
                continue;
            }

            if (addedFirst)
                filterOptions << "-o";
            filterOptions << nameOption << current;
            addedFirst = true;
        }
    }
    arguments << filterOptions;
    const QByteArray output = d->outputForRunInShell({"find", arguments});
    const QString out = QString::fromUtf8(output.data(), output.size());
    if (!output.isEmpty() && !out.startsWith(filePath.path())) { // missing find, unknown option
        LOG("Setting 'do not use find'" << out.left(out.indexOf('\n')));
        d->m_useFind = false;
        return;
    }

    const QStringList entries = out.split("\n", Qt::SkipEmptyParts);
    for (const QString &entry : entries) {
        if (entry.startsWith("find: "))
            continue;
        const FilePath fp = FilePath::fromString(entry);

        if (!callBack(fp.onDevice(filePath)))
            break;
    }
}

void DockerDevice::iterateDirectory(const FilePath &filePath,
                                    const std::function<bool(const FilePath &)> &callBack,
                                    const FileFilter &filter) const
{
    QTC_ASSERT(handlesFile(filePath), return);
    updateContainerAccess();
    if (hasLocalFileAccess()) {
        const FilePath local = mapToLocalAccess(filePath);
        local.iterateDirectory([&callBack, this](const FilePath &entry) {
                                    return callBack(mapFromLocalAccess(entry));
                               },
                               filter);
        return;
    }

    if (d->m_useFind) {
        iterateWithFind(filePath, callBack, filter);
        // d->m_useFind will be set to false if 'find' is not found. In this
        // case fall back to 'ls' below.
        if (d->m_useFind)
            return;
    }

    // if we do not have find - use ls as fallback
    const QByteArray output = d->outputForRunInShell({"ls", {"-1", "-b", "--", filePath.path()}});
    const QStringList entries = QString::fromUtf8(output).split('\n', Qt::SkipEmptyParts);
    FileUtils::iterateLsOutput(filePath, entries, filter, callBack);
}

QByteArray DockerDevice::fileContents(const FilePath &filePath, qint64 limit, qint64 offset) const
{
    QTC_ASSERT(handlesFile(filePath), return {});
    updateContainerAccess();
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

bool DockerDevice::writeFileContents(const FilePath &filePath, const QByteArray &data) const
{
    QTC_ASSERT(handlesFile(filePath), return {});
    updateContainerAccess();
    if (hasLocalFileAccess())
        return mapToLocalAccess(filePath).writeFileContents(data);

// This following would be the generic Unix solution.
// But it doesn't pass input. FIXME: Why?
//    QtcProcess proc;
//    proc.setCommand({"dd", {"of=" + filePath.path()}});
//    proc.setWriteData(data);
//    runProcess(proc);
//    proc.waitForFinished();

    TemporaryFile tempFile("dockertransport-XXXXXX");
    tempFile.open();
    tempFile.write(data);

    const QString tempName = tempFile.fileName();
    tempFile.close();

    CommandLine cmd{"docker", {"cp", tempName, d->m_container + ':' + filePath.path()}};

    QtcProcess proc;
    proc.setCommand(cmd);
    proc.runBlocking();

    return proc.exitCode() == 0;
}

void DockerDevice::runProcess(QtcProcess &process) const
{
    updateContainerAccess();
    if (!DockerPlugin::isDaemonRunning().value_or(false))
        return;
    if (d->m_container.isEmpty()) {
        LOG("No container set to run " << process.commandLine().toUserOutput());
        QTC_CHECK(false);
        process.setResult(QtcProcess::StartFailed);
        return;
    }

    const FilePath workingDir = process.workingDirectory();
    const Environment env = process.environment();

    CommandLine cmd{"docker", {"exec"}};
    if (!workingDir.isEmpty()) {
        cmd.addArgs({"-w", mapToDevicePath(workingDir)});
        if (QTC_GUARD(workingDir.needsDevice())) // warn on local working directory for docker cmd
            process.setWorkingDirectory(FileUtils::homePath()); // reset working dir for docker exec
    }
    if (process.processMode() == ProcessMode::Writer)
        cmd.addArg("-i");
    if (env.size() != 0 && !hasLocalFileAccess()) {
        process.unsetEnvironment();
        // FIXME the below would be probably correct if the respective tools would use correct
        //       environment already, but most are using the host environment which usually makes
        //       no sense on the device and may degrade performance
        // const QStringList envList = env.toStringList();
        // for (const QString &keyValue : envList) {
        //     cmd.addArg("-e");
        //     cmd.addArg(keyValue);
        // }
    }
    cmd.addArg(d->m_container);
    cmd.addCommandLineAsArgs(process.commandLine());

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
    detector.undoAutoDetect(d->m_data.autodetectId());
}

void DockerDevicePrivate::fetchSystemEnviroment()
{
    if (m_shell) {
        const QByteArray output = outputForRunInShell({"env", {}});
        const QString out = QString::fromUtf8(output.data(), output.size());
        m_cachedEnviroment = Environment(out.split('\n', Qt::SkipEmptyParts), q->osType());
        return;
    }

    QtcProcess proc;
    proc.setCommand({"env", {}});

    q->runProcess(proc); // FIXME: This only starts.
    proc.waitForFinished();

    const QString remoteOutput = proc.stdOut();
    m_cachedEnviroment = Environment(remoteOutput.split('\n', Qt::SkipEmptyParts), q->osType());

    const QString remoteError = proc.stdErr();
    if (!remoteError.isEmpty())
        qWarning("Cannot read container environment: %s\n", qPrintable(remoteError));
}

bool DockerDevicePrivate::runInContainer(const CommandLine &cmd) const
{
    if (!DockerPlugin::isDaemonRunning().value_or(false))
        return false;
    CommandLine dcmd{"docker", {"exec", m_container}};
    dcmd.addCommandLineAsArgs(cmd);

    QtcProcess proc;
    proc.setCommand(dcmd);
    proc.setWorkingDirectory(FilePath::fromString(QDir::tempPath()));
    proc.start();
    proc.waitForFinished();

    LOG("Run sync:" << dcmd.toUserOutput() << " result: " << proc.exitCode());
    const int exitCode = proc.exitCode();
    return exitCode == 0;
}

bool DockerDevicePrivate::runInShell(const CommandLine &cmd) const
{
    if (!QTC_GUARD(DockerPlugin::isDaemonRunning().value_or(false))) {
        LOG("No daemon. Could not run " << cmd.toUserOutput());
        return false;
    }
    QTC_ASSERT(m_shell, LOG("No shell. Could not run " << cmd.toUserOutput()); return false);
    QMutexLocker l(&m_shellMutex);
    m_shell->readAllStandardOutput(); // clean possible left-overs
    m_shell->write(cmd.toUserOutput().toUtf8() + "\necho $?\n");
    QTC_ASSERT(m_shell->waitForReadyRead(), return false);
    QByteArray output = m_shell->readAllStandardOutput();
    bool ok;
    int result = output.toInt(&ok);
    LOG("Run command in shell:" << cmd.toUserOutput() << "result: " << output << " ==>" << result);
    QTC_ASSERT(ok, return false);
    return result == 0;
}

// generate hex value
static QByteArray randomHex()
{
    quint32 val = QRandomGenerator::global()->generate();
    return QString::number(val, 16).toUtf8();
}

QByteArray DockerDevicePrivate::outputForRunInShell(const CommandLine &cmd) const
{
    if (!DockerPlugin::isDaemonRunning().value_or(false))
        return {};
    QTC_ASSERT(m_shell && m_shell->isRunning(), return {});
    QMutexLocker l(&m_shellMutex);
    m_shell->readAllStandardOutput(); // clean possible left-overs
    const QByteArray markerWithNewLine("___QC_DOCKER_" + randomHex() + "_OUTPUT_MARKER___\n");
    m_shell->write(cmd.toUserOutput().toUtf8() + "\necho -n \"" + markerWithNewLine + "\"\n");
    QByteArray output;
    while (!output.endsWith(markerWithNewLine)) {
        QTC_ASSERT(m_shell->isRunning(), return {});
        m_shell->waitForReadyRead();
        output.append(m_shell->readAllStandardOutput());
    }
    LOG("Run command in shell:" << cmd.toUserOutput() << "output size:" << output.size());
    if (QTC_GUARD(output.endsWith(markerWithNewLine)))
        output.chop(markerWithNewLine.size());
    return output;
}

// Factory

class DockerImageItem final : public TreeItem, public DockerDeviceData
{
public:
    DockerImageItem() {}

    QVariant data(int column, int role) const final
    {
        switch (column) {
        case 0:
            if (role == Qt::DisplayRole)
                return repo;
            break;
        case 1:
            if (role == Qt::DisplayRole)
                return tag;
            break;
        case 2:
            if (role == Qt::DisplayRole)
                return imageId;
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

        m_model.setHeader({"Repository", "Tag", "Image", "Size"});

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

    IDevice::Ptr device() const
    {
        const QModelIndexList selectedRows = m_view->selectionModel()->selectedRows();
        QTC_ASSERT(selectedRows.size() == 1, return {});
        DockerImageItem *item = m_model.itemForIndex(selectedRows.front());
        QTC_ASSERT(item, return {});

        auto device = DockerDevice::create(*item);
        device->setupId(IDevice::ManuallyAdded, Id::fromString(item->autodetectId()));
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

void DockerDeviceWidget::updateDaemonStateTexts()
{
    Utils::optional<bool> daemonState = DockerPlugin::isDaemonRunning();
    if (!daemonState.has_value()) {
        m_daemonReset->setIcon(Icons::INFO.icon());
        m_daemonState->setText(tr("Daemon state not evaluated."));
    } else if (daemonState.value()) {
        m_daemonReset->setIcon(Icons::OK.icon());
        m_daemonState->setText(tr("Docker daemon running."));
    } else {
        m_daemonReset->setIcon(Icons::CRITICAL.icon());
        m_daemonState->setText(tr("Docker daemon not running."));
    }
}

// Factory

DockerDeviceFactory::DockerDeviceFactory()
    : IDeviceFactory(Constants::DOCKER_DEVICE_TYPE)
{
    setDisplayName(DockerDevice::tr("Docker Device"));
    setIcon(QIcon());
    setCreator([] {
        DockerDeviceSetupWizard wizard;
        if (wizard.exec() != QDialog::Accepted)
            return IDevice::Ptr();
        return wizard.device();
    });
    setConstructionFunction([] { return DockerDevice::create({}); });
}

} // Internal
} // Docker
