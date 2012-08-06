/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "startgdbserverdialog.h"

#include "debuggercore.h"
#include "debuggermainwindow.h"
#include "debuggerplugin.h"
#include "debuggerprofileinformation.h"
#include "debuggerrunner.h"
#include "debuggerstartparameters.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/profilechooser.h>
#include <projectexplorer/devicesupport/deviceprocesslist.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <ssh/sshconnection.h>
#include <ssh/sshremoteprocessrunner.h>
#include <utils/pathchooser.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

#include <QVariant>
#include <QSettings>

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSpacerItem>
#include <QTableView>
#include <QTextBrowser>
#include <QVBoxLayout>

using namespace Core;
using namespace ProjectExplorer;
using namespace QSsh;
using namespace Utils;

const char LastProfile[] = "Debugger/LastProfile";
const char LastDevice[] = "Debugger/LastDevice";
const char LastProcessName[] = "Debugger/LastProcessName";
//const char LastLocalExecutable[] = "Debugger/LastLocalExecutable";

namespace Debugger {
namespace Internal {

class StartGdbServerDialogPrivate
{
public:
    StartGdbServerDialogPrivate(StartGdbServerDialog *q);

    IDevice::ConstPtr currentDevice() const
    {
        Profile *profile = profileChooser->currentProfile();
        return DeviceProfileInformation::device(profile);
    }

    StartGdbServerDialog *q;
    bool startServerOnly;
    DeviceProcessList *processList;
    QSortFilterProxyModel proxyModel;

    QLineEdit *processFilterLineEdit;
    QTableView *tableView;
    QPushButton *attachProcessButton;
    QTextBrowser *textBrowser;
    QPushButton *closeButton;
    ProfileChooser *profileChooser;

    DeviceUsedPortsGatherer gatherer;
    SshRemoteProcessRunner runner;
    QString remoteCommandLine;
    QString remoteExecutable;
};

StartGdbServerDialogPrivate::StartGdbServerDialogPrivate(StartGdbServerDialog *q)
    : q(q), startServerOnly(true), processList(0)
{
    QSettings *settings = ICore::settings();

    profileChooser = new ProfileChooser(q, ProfileChooser::RemoteDebugging);

    //executablePathChooser = new PathChooser(q);
    //executablePathChooser->setExpectedKind(PathChooser::File);
    //executablePathChooser->setPromptDialogTitle(StartGdbServerDialog::tr("Select Executable"));
    //executablePathChooser->setPath(settings->value(LastLocalExecutable).toString());

    processFilterLineEdit = new QLineEdit(q);
    processFilterLineEdit->setText(settings->value(LastProcessName).toString());
    processFilterLineEdit->selectAll();

    tableView = new QTableView(q);
    tableView->setShowGrid(false);
    tableView->setSortingEnabled(true);
    tableView->horizontalHeader()->setDefaultSectionSize(100);
    tableView->horizontalHeader()->setStretchLastSection(true);
    tableView->verticalHeader()->setVisible(false);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);

    attachProcessButton = new QPushButton(q);
    attachProcessButton->setText(StartGdbServerDialog::tr("&Attach to Selected Process"));

    closeButton = new QPushButton(q);
    closeButton->setText(StartGdbServerDialog::tr("Close"));

    textBrowser = new QTextBrowser(q);
    textBrowser->setEnabled(false);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(StartGdbServerDialog::tr("Target:"), profileChooser);
    formLayout->addRow(StartGdbServerDialog::tr("&Filter entries:"), processFilterLineEdit);

    QHBoxLayout *horizontalLayout2 = new QHBoxLayout();
    horizontalLayout2->addStretch(1);
    horizontalLayout2->addWidget(attachProcessButton);
    horizontalLayout2->addWidget(closeButton);

    formLayout->addRow(tableView);
    formLayout->addRow(textBrowser);
    formLayout->addRow(horizontalLayout2);
    q->setLayout(formLayout);
}

} // namespace Internal


StartGdbServerDialog::StartGdbServerDialog(QWidget *parent) :
    QDialog(parent),
    d(new Internal::StartGdbServerDialogPrivate(this))
{
    setWindowTitle(tr("List of Remote Processes"));

    QObject::connect(d->closeButton, SIGNAL(clicked()), this, SLOT(reject()));

    connect(&d->gatherer, SIGNAL(error(QString)), SLOT(portGathererError(QString)));
    connect(&d->gatherer, SIGNAL(portListReady()), SLOT(portListReady()));

    d->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->proxyModel.setDynamicSortFilter(true);
    d->proxyModel.setFilterKeyColumn(-1);
    d->tableView->setModel(&d->proxyModel);
    connect(d->processFilterLineEdit, SIGNAL(textChanged(QString)),
        &d->proxyModel, SLOT(setFilterRegExp(QString)));

    connect(d->tableView->selectionModel(),
        SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
        SLOT(updateButtons()));
    connect(d->profileChooser, SIGNAL(activated(int)),
            SLOT(updateButtons()));
    //connect(d->updateListButton, SIGNAL(clicked()),
    //    SLOT(updateProcessList()));
    connect(d->attachProcessButton, SIGNAL(clicked()), SLOT(attachToProcess()));
    connect(&d->proxyModel, SIGNAL(layoutChanged()),
        SLOT(handleProcessListUpdated()));
    connect(d->profileChooser, SIGNAL(currentIndexChanged(int)),
        SLOT(attachToDevice()));
    updateButtons();
    attachToDevice();
}

StartGdbServerDialog::~StartGdbServerDialog()
{
    delete d->processList;
    delete d;
}

void StartGdbServerDialog::attachToDevice()
{
    IDevice::ConstPtr device = d->currentDevice();
    // TODO: display error on non-matching device.
    if (!device || !device->canCreateProcessModel())
        return;
    delete d->processList;
    d->processList = device->createProcessListModel();
    d->proxyModel.setSourceModel(d->processList);
    connect(d->processList, SIGNAL(error(QString)),
        SLOT(handleRemoteError(QString)));
    connect(d->processList, SIGNAL(modelReset()),
        SLOT(handleProcessListUpdated()));
    connect(d->processList, SIGNAL(processKilled()),
        SLOT(handleProcessKilled()), Qt::QueuedConnection);
    updateProcessList();
}

void StartGdbServerDialog::handleRemoteError(const QString &errorMsg)
{
    QMessageBox::critical(this, tr("Remote Error"), errorMsg);
    updateButtons();
}

void StartGdbServerDialog::handleProcessListUpdated()
{
    d->tableView->resizeRowsToContents();
    updateButtons();
}

void StartGdbServerDialog::updateProcessList()
{
    d->attachProcessButton->setEnabled(false);
    d->processList->update();
    d->proxyModel.setFilterRegExp(QString());
    d->proxyModel.setFilterRegExp(d->processFilterLineEdit->text());
    updateButtons();
}

void StartGdbServerDialog::attachToProcess()
{
    const QModelIndexList &indexes =
            d->tableView->selectionModel()->selectedIndexes();
    if (indexes.empty())
        return;
    d->attachProcessButton->setEnabled(false);

    IDevice::ConstPtr device = d->currentDevice();
    if (!device)
        return;
    PortList ports = device->freePorts();
    const int port = d->gatherer.getNextFreePort(&ports);
    const int row = d->proxyModel.mapToSource(indexes.first()).row();
    QTC_ASSERT(row >= 0, return);
    DeviceProcess process = d->processList->at(row);
    d->remoteCommandLine = process.cmdLine;
    d->remoteExecutable = process.exe;
    if (port == -1) {
        reportFailure();
        return;
    }

    QSettings *settings = ICore::settings();
    settings->setValue(LastProfile, d->profileChooser->currentProfileId().toString());
    settings->setValue(LastProcessName, d->processFilterLineEdit->text());

    startGdbServerOnPort(port, process.pid);
}

void StartGdbServerDialog::reportFailure()
{
    QTC_ASSERT(false, /**/);
    logMessage(tr("Process aborted"));
}

void StartGdbServerDialog::logMessage(const QString &line)
{
    d->textBrowser->append(line);
}

void StartGdbServerDialog::handleProcessKilled()
{
    updateProcessList();
}

void StartGdbServerDialog::updateButtons()
{
    d->attachProcessButton->setEnabled(d->tableView->selectionModel()->hasSelection()
        || d->proxyModel.rowCount() == 1);
}

void StartGdbServerDialog::portGathererError(const QString &text)
{
    logMessage(tr("Could not retrieve list of free ports:"));
    logMessage(text);
    reportFailure();
}

void StartGdbServerDialog::portListReady()
{
    updateButtons();
}

void StartGdbServerDialog::startGdbServer()
{
    d->startServerOnly = true;
    if (exec() == QDialog::Rejected)
        return;
    d->gatherer.start(d->currentDevice());
}

void StartGdbServerDialog::attachToRemoteProcess()
{
    d->startServerOnly = false;
    if (exec() == QDialog::Rejected)
        return;
    d->gatherer.start(d->currentDevice());
}

void StartGdbServerDialog::handleConnectionError()
{
    logMessage(tr("Connection error: %1").arg(d->runner.lastConnectionErrorString()));
    emit processAborted();
}

void StartGdbServerDialog::handleProcessStarted()
{
    logMessage(tr("Starting gdbserver..."));
}

void StartGdbServerDialog::handleProcessOutputAvailable()
{
    logMessage(QString::fromUtf8(d->runner.readAllStandardOutput().trimmed()));
}

void StartGdbServerDialog::handleProcessErrorOutput()
{
    const QByteArray ba = d->runner.readAllStandardError();
    logMessage(QString::fromUtf8(ba.trimmed()));
    // "Attached; pid = 16740"
    // "Listening on port 10000"
    foreach (const QByteArray &line, ba.split('\n')) {
        if (line.startsWith("Listening on port")) {
            const int port = line.mid(18).trimmed().toInt();
            reportOpenPort(port);
        }
    }
}

void StartGdbServerDialog::reportOpenPort(int port)
{
    close();
    logMessage(tr("Port %1 is now accessible.").arg(port));
    IDevice::ConstPtr device = d->currentDevice();
    QString channel = QString("%1:%2").arg(device->sshParameters().host).arg(port);
    logMessage(tr("Server started on %1").arg(channel));

    const Profile *profile = d->profileChooser->currentProfile();
    QTC_ASSERT(profile, return);

    if (d->startServerOnly) {
        //showStatusMessage(tr("gdbserver is now listening at %1").arg(channel));
    } else {
        QString sysroot = SysRootProfileInformation::sysRoot(profile).toString();
        QString binary;
        QString localExecutable;
        QString candidate = sysroot + d->remoteExecutable;
        if (QFileInfo(candidate).exists())
            localExecutable = candidate;
        if (localExecutable.isEmpty()) {
            binary = d->remoteCommandLine.section(QLatin1Char(' '), 0, 0);
            candidate = sysroot + QLatin1Char('/') + binary;
            if (QFileInfo(candidate).exists())
                localExecutable = candidate;
        }
        if (localExecutable.isEmpty()) {
            candidate = sysroot + QLatin1String("/usr/bin/") + binary;
            if (QFileInfo(candidate).exists())
                localExecutable = candidate;
        }
        if (localExecutable.isEmpty()) {
            candidate = sysroot + QLatin1String("/bin/") + binary;
            if (QFileInfo(candidate).exists())
                localExecutable = candidate;
        }
        if (localExecutable.isEmpty()) {
            QMessageBox::warning(DebuggerPlugin::mainWindow(), tr("Warning"),
                tr("Cannot find local executable for remote process \"%1\".")
                    .arg(d->remoteCommandLine));
            return;
        }

        QList<Abi> abis = Abi::abisOfBinary(Utils::FileName::fromString(localExecutable));
        if (abis.isEmpty()) {
            QMessageBox::warning(DebuggerPlugin::mainWindow(), tr("Warning"),
                tr("Cannot find ABI for remote process \"%1\".")
                    .arg(d->remoteCommandLine));
            return;
        }

        DebuggerStartParameters sp;
        sp.displayName = tr("Remote: \"%1\"").arg(channel);
        sp.remoteChannel = channel;
        sp.executable = localExecutable;
        sp.startMode = AttachToRemoteServer;
        sp.closeMode = KillAtClose;
        sp.overrideStartScript.clear();
        sp.useServerStartScript = false;
        sp.serverStartScript.clear();
        sp.sysRoot = SysRootProfileInformation::sysRoot(profile).toString();
        sp.debuggerCommand = DebuggerProfileInformation::debuggerCommand(profile).toString();
        sp.connParams = device->sshParameters();
        if (ToolChain *tc = ToolChainProfileInformation::toolChain(profile))
            sp.toolChainAbi = tc->targetAbi();

        if (RunControl *rc = DebuggerPlugin::createDebugger(sp))
            DebuggerPlugin::startDebugger(rc);
    }
}

void StartGdbServerDialog::handleProcessClosed(int status)
{
    logMessage(tr("Process gdbserver finished. Status: %1").arg(status));
}

void StartGdbServerDialog::startGdbServerOnPort(int port, int pid)
{
    IDevice::ConstPtr device = d->currentDevice();
    connect(&d->runner, SIGNAL(connectionError()), SLOT(handleConnectionError()));
    connect(&d->runner, SIGNAL(processStarted()), SLOT(handleProcessStarted()));
    connect(&d->runner, SIGNAL(readyReadStandardOutput()), SLOT(handleProcessOutputAvailable()));
    connect(&d->runner, SIGNAL(readyReadStandardError()), SLOT(handleProcessErrorOutput()));
    connect(&d->runner, SIGNAL(processClosed(int)), SLOT(handleProcessClosed(int)));

    QByteArray cmd = "/usr/bin/gdbserver --attach :"
        + QByteArray::number(port) + " " + QByteArray::number(pid);
    logMessage(tr("Running command: %1").arg(QString::fromLatin1(cmd)));
    d->runner.run(cmd, device->sshParameters());
}

} // namespace Debugger
