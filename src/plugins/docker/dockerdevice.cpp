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

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/devicesupport/idevicewidget.h>
#include <projectexplorer/runcontrol.h>

#include <utils/algorithm.h>
#include <utils/basetreeview.h>
#include <utils/consoleprocess.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/port.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/treemodel.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QPushButton>
#include <QTextBrowser>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

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
    QProcess m_process;
    ConsoleProcess m_consoleProcess;
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

    const DockerDeviceData &data = dockerDevice->data();
    CommandLine cmd("docker");
    cmd.addArg("run");
    cmd.addArgs(dockerRunFlags);
    cmd.addArg(data.imageId);
    if (!runnable.executable.isEmpty())
        cmd.addArgs(runnable.commandLine());

    disconnect(&m_consoleProcess);
    disconnect(&m_process);

    if (runInTerminal()) {
        connect(&m_consoleProcess, &ConsoleProcess::errorOccurred,
                this, &DeviceProcess::error);
        connect(&m_consoleProcess, &ConsoleProcess::processStarted,
                this, &DeviceProcess::started);
        connect(&m_consoleProcess, &ConsoleProcess::stubStopped,
                this, &DeviceProcess::finished);

        m_consoleProcess.setAbortOnMetaChars(false);
        m_consoleProcess.setSettings(Core::ICore::settings());
        m_consoleProcess.setEnvironment(runnable.environment);
        m_consoleProcess.setCommand(cmd);
        m_consoleProcess.start();
    } else {
        m_process.setProcessEnvironment(runnable.environment.toProcessEnvironment());
        connect(&m_process, &QProcess::errorOccurred, this, &DeviceProcess::error);
        connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &DeviceProcess::finished);
        connect(&m_process, &QProcess::readyReadStandardOutput,
                this, &DeviceProcess::readyReadStandardOutput);
        connect(&m_process, &QProcess::readyReadStandardError,
                this, &DeviceProcess::readyReadStandardError);
        connect(&m_process, &QProcess::started, this, &DeviceProcess::started);
        m_process.setWorkingDirectory(runnable.workingDirectory);
        m_process.start(cmd.executable().toString(), cmd.splitArguments());
    }

    connect(this, &DeviceProcess::readyReadStandardOutput, this, [this] {
        MessageManager::writeSilently(QString::fromLocal8Bit(readAllStandardError()));
    });
    connect(this, &DeviceProcess::readyReadStandardError, this, [this] {
        MessageManager::writeDisrupting(QString::fromLocal8Bit(readAllStandardError()));
    });
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

class DockerDeviceWidget final : public IDeviceWidget
{
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

        using namespace Layouting;

        Form {
            m_idLabel, m_idLineEdit, Break(),
            m_repoLabel, m_repoLineEdit, Break(),
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

DockerDevice::DockerDevice(const DockerDeviceData &data)
    : m_data(data)
{
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

const DockerDeviceData &DockerDevice::data() const
{
    return m_data;
}

const char DockerDeviceDataImageIdKey[] = "DockerDeviceDataImageId";
const char DockerDeviceDataRepoKey[] = "DockerDeviceDataRepo";
const char DockerDeviceDataTagKey[] = "DockerDeviceDataTag";
const char DockerDeviceDataSizeKey[] = "DockerDeviceDataSize";

void DockerDevice::fromMap(const QVariantMap &map)
{
    ProjectExplorer::IDevice::fromMap(map);
    m_data.imageId = map.value(DockerDeviceDataImageIdKey).toString();
    m_data.repo = map.value(DockerDeviceDataRepoKey).toString();
    m_data.tag = map.value(DockerDeviceDataTagKey).toString();
    m_data.size = map.value(DockerDeviceDataSizeKey).toString();
}

QVariantMap DockerDevice::toMap() const
{
    QVariantMap map = ProjectExplorer::IDevice::toMap();
    map.insert(DockerDeviceDataImageIdKey, m_data.imageId);
    map.insert(DockerDeviceDataRepoKey, m_data.repo);
    map.insert(DockerDeviceDataTagKey, m_data.tag);
    map.insert(DockerDeviceDataSizeKey, m_data.size);
    return map;
}

DockerDevice::~DockerDevice() = default;

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

        m_model.setHeader({"Image", "Repository", "Tag", "Size"});

        m_view = new TreeView;
        m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_view->setSelectionMode(QAbstractItemView::SingleSelection);
        m_view->setModel(&m_model);

        auto output = new QTextBrowser;
        output->setEnabled(false);
        output->setVisible(false);

        auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                            Qt::Horizontal);

        using namespace Layouting;
        Column {
            m_view,
            output,
            buttons,
        }.attachTo(this);

        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        buttons->button(QDialogButtonBox::Ok)->setEnabled(false);

        CommandLine cmd{"docker", {"images", "--format", "{{.ID}}\\t{{.Repository}}\\t{{.Tag}}\\t{{.Size}}"}};
        output->append(tr("Running \"%1\"\n").arg(cmd.toUserOutput()));

        m_process = new QtcProcess(this);
        m_process->setCommand(cmd);

        connect(m_process, &QtcProcess::readyReadStandardOutput, [this, output] {
            const QString out = QString::fromUtf8(m_process->readAllStandardOutput().trimmed());
            output->append(out);
            for (const QString &line : out.split('\n')) {
                const QStringList parts = line.trimmed().split('\t');
                if (parts.size() != 4) {
                    output->append(tr("Unexpected result: %1").arg(line) + '\n');
                    continue;
                }
                auto item = new DockerImageItem;
                item->imageId = parts.at(0);
                item->repo = parts.at(1);
                item->tag = parts.at(2);
                item->size = parts.at(3);
                m_model.rootItem()->appendChild(item);
            }
            output->append(tr("\nDone."));

        });

        connect(m_process, &Utils::QtcProcess::readyReadStandardError, [this, output] {
            const QString out = tr("Error: %1").arg(QString::fromUtf8(m_process->readAllStandardError()));
            output->append(tr("Error: %1").arg(out));
        });

        connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, [this, buttons] {
            const QModelIndexList selectedRows = m_view->selectionModel()->selectedRows();
            QTC_ASSERT(selectedRows.size() == 1, return);
            buttons->button(QDialogButtonBox::Ok)->setEnabled(selectedRows.size() == 1);
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
