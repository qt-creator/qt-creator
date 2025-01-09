// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debuggerdialogs.h"

#include "cdb/cdbengine.h"
#include "debuggerruncontrol.h"
#include "debuggertr.h"

#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/sshparameters.h>
#include <projectexplorer/kitchooser.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/fancylineedit.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QGroupBox>
#include <QGuiApplication>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Debugger::Internal {

///////////////////////////////////////////////////////////////////////
//
// StartApplicationParameters
//
///////////////////////////////////////////////////////////////////////

class StartApplicationParameters
{
public:
    QString displayName() const;
    bool equals(const StartApplicationParameters &rhs) const;
    void toSettings(QtcSettings *) const;
    void fromSettings(const QtcSettings *settings);

    bool operator==(const StartApplicationParameters &p) const { return equals(p); }
    bool operator!=(const StartApplicationParameters &p) const { return !equals(p); }

    Id kitId;
    uint serverPort;
    QString serverAddress;
    ProcessRunData runnable;
    bool breakAtMain = false;
    bool runInTerminal = false;
    bool useTargetExtendedRemote = false;
    FilePath sysRoot;
    QString serverInitCommands;
    QString serverResetCommands;
    FilePath debugInfoLocation;
};

bool StartApplicationParameters::equals(const StartApplicationParameters &rhs) const
{
    return runnable.command == rhs.runnable.command
        && serverPort == rhs.serverPort
        && runnable.workingDirectory == rhs.runnable.workingDirectory
        && breakAtMain == rhs.breakAtMain
        && runInTerminal == rhs.runInTerminal
        && sysRoot == rhs.sysRoot
        && serverInitCommands == rhs.serverInitCommands
        && serverResetCommands == rhs.serverResetCommands
        && kitId == rhs.kitId
        && debugInfoLocation == rhs.debugInfoLocation
        && serverAddress == rhs.serverAddress;
}

QString StartApplicationParameters::displayName() const
{
    const int maxLength = 60;

    QString name = runnable.command.executable().fileName()
            + ' ' + runnable.command.arguments();
    if (name.size() > 60) {
        int index = name.lastIndexOf(' ', maxLength);
        if (index == -1)
            index = maxLength;
        name.truncate(index);
        name += "...";
    }

    if (Kit *kit = KitManager::kit(kitId))
        name += QString::fromLatin1(" (%1)").arg(kit->displayName());

    return name;
}

void StartApplicationParameters::toSettings(QtcSettings *settings) const
{
    settings->setValue("LastKitId", kitId.toSetting());
    settings->setValue("LastServerPort", serverPort);
    settings->setValue("LastServerAddress", serverAddress);
    settings->setValue("LastExternalExecutable", runnable.command.executable().toSettings());
    settings->setValue("LastExternalExecutableArguments", runnable.command.arguments());
    settings->setValue("LastExternalWorkingDirectory", runnable.workingDirectory.toSettings());
    settings->setValue("LastExternalBreakAtMain", breakAtMain);
    settings->setValue("LastExternalRunInTerminal", runInTerminal);
    settings->setValue("LastExternalUseTargetExtended", useTargetExtendedRemote);
    settings->setValue("LastServerInitCommands", serverInitCommands);
    settings->setValue("LastServerResetCommands", serverResetCommands);
    settings->setValue("LastDebugInfoLocation", debugInfoLocation.toSettings());
    settings->setValue("LastSysRoot", sysRoot.toSettings());
}

void StartApplicationParameters::fromSettings(const QtcSettings *settings)
{
    kitId = Id::fromSetting(settings->value("LastKitId"));
    serverPort = settings->value("LastServerPort").toUInt();
    serverAddress = settings->value("LastServerAddress").toString();
    runnable.command.setExecutable(FilePath::fromSettings(settings->value("LastExternalExecutable")));
    runnable.command.setArguments(settings->value("LastExternalExecutableArguments").toString());
    runnable.workingDirectory = FilePath::fromSettings(settings->value("LastExternalWorkingDirectory"));
    breakAtMain = settings->value("LastExternalBreakAtMain").toBool();
    runInTerminal = settings->value("LastExternalRunInTerminal").toBool();
    useTargetExtendedRemote = settings->value("LastExternalUseTargetExtended").toBool();
    serverInitCommands = settings->value("LastServerInitCommands").toString();
    serverResetCommands = settings->value("LastServerResetCommands").toString();
    debugInfoLocation = FilePath::fromSettings(settings->value("LastDebugInfoLocation"));
    sysRoot = FilePath::fromSettings(settings->value("LastSysRoot"));
}

///////////////////////////////////////////////////////////////////////
//
// StartApplicationDialog
//
///////////////////////////////////////////////////////////////////////

class StartApplicationDialog final : public QDialog
{
public:
    StartApplicationDialog();

    static void run(bool);

private:
    void historyIndexChanged(int);
    void updateState();
    StartApplicationParameters parameters() const;
    void setParameters(const StartApplicationParameters &p);
    void setHistory(const QList<StartApplicationParameters> &l);
    void onChannelOverrideChanged(const QString &channel);

    KitChooser *kitChooser;
    QLabel *serverPortLabel;
    QLabel *channelOverrideHintLabel;
    QLabel *channelOverrideLabel;
    QLineEdit *channelOverrideEdit;
    QSpinBox *serverPortSpinBox;
    PathChooser *localExecutablePathChooser;
    FancyLineEdit *arguments;
    PathChooser *workingDirectory;
    QCheckBox *breakAtMainCheckBox;
    QCheckBox *runInTerminalCheckBox;
    QCheckBox *useTargetExtendedRemoteCheckBox;
    PathChooser *debuginfoPathChooser;
    QLabel *sysRootLabel;
    PathChooser *sysRootPathChooser;
    QLabel *serverInitCommandsLabel;
    QPlainTextEdit *serverInitCommandsTextEdit;
    QLabel *serverResetCommandsLabel;
    QPlainTextEdit *serverResetCommandsTextEdit;
    QComboBox *historyComboBox;
    QDialogButtonBox *buttonBox;
};

StartApplicationDialog::StartApplicationDialog()
  : QDialog(ICore::dialogParent())
{
    setWindowTitle(Tr::tr("Start Debugger"));

    kitChooser = new KitChooser(this);
    kitChooser->setShowIcons(true);
    kitChooser->populate();

    serverPortLabel = new QLabel(Tr::tr("Server port:"), this);
    serverPortSpinBox = new QSpinBox(this);
    serverPortSpinBox->setRange(1, 65535);

    channelOverrideHintLabel =
            new QLabel(Tr::tr("Normally, the running server is identified by the IP of the "
                          "device in the kit and the server port selected above.\n"
                          "You can choose another communication channel here, such as "
                          "a serial line or custom ip:port."));

    channelOverrideLabel = new QLabel(Tr::tr("Override server channel:"), this);
    channelOverrideEdit = new QLineEdit(this);
    //: "For example, /dev/ttyS0, COM1, 127.0.0.1:1234"
    channelOverrideEdit->setPlaceholderText(
        Tr::tr("For example, %1").arg("/dev/ttyS0, COM1, 127.0.0.1:1234"));

    localExecutablePathChooser = new PathChooser(this);
    localExecutablePathChooser->setExpectedKind(PathChooser::File);
    localExecutablePathChooser->setPromptDialogTitle(Tr::tr("Select Executable"));
    localExecutablePathChooser->setHistoryCompleter("LocalExecutable");

    arguments = new FancyLineEdit(this);
    arguments->setClearButtonEnabled(true);
    arguments->setHistoryCompleter("CommandlineArguments");

    workingDirectory = new PathChooser(this);
    workingDirectory->setExpectedKind(PathChooser::ExistingDirectory);
    workingDirectory->setPromptDialogTitle(Tr::tr("Select Working Directory"));
    workingDirectory->setHistoryCompleter("WorkingDirectory");

    runInTerminalCheckBox = new QCheckBox(this);

    breakAtMainCheckBox = new QCheckBox(this);
    breakAtMainCheckBox->setText(QString());

    useTargetExtendedRemoteCheckBox = new QCheckBox(this);

    sysRootPathChooser = new PathChooser(this);
    sysRootPathChooser->setExpectedKind(PathChooser::Directory);
    sysRootPathChooser->setHistoryCompleter("Debugger.SysRoot.History");
    sysRootPathChooser->setPromptDialogTitle(Tr::tr("Select SysRoot Directory"));
    sysRootPathChooser->setToolTip(Tr::tr(
        "This option can be used to override the kit's SysRoot setting."));
    sysRootLabel = new QLabel(Tr::tr("Override S&ysRoot:"), this);
    sysRootLabel->setBuddy(sysRootPathChooser);
    sysRootLabel->setToolTip(sysRootPathChooser->toolTip());

    serverInitCommandsTextEdit = new QPlainTextEdit(this);
    serverInitCommandsTextEdit->setToolTip(Tr::tr(
        "This option can be used to send the target init commands."));

    serverInitCommandsLabel = new QLabel(Tr::tr("&Init commands:"), this);
    serverInitCommandsLabel->setBuddy(serverInitCommandsTextEdit);
    serverInitCommandsLabel->setToolTip(serverInitCommandsTextEdit->toolTip());

    serverResetCommandsTextEdit = new QPlainTextEdit(this);
    serverResetCommandsTextEdit->setToolTip(Tr::tr(
        "This option can be used to send the target reset commands."));

    serverResetCommandsLabel = new QLabel(Tr::tr("&Reset commands:"), this);
    serverResetCommandsLabel->setBuddy(serverResetCommandsTextEdit);
    serverResetCommandsLabel->setToolTip(serverResetCommandsTextEdit->toolTip());

    debuginfoPathChooser = new PathChooser(this);
    debuginfoPathChooser->setPromptDialogTitle(Tr::tr("Select Location of Debugging Information"));
    debuginfoPathChooser->setToolTip(Tr::tr(
        "Base path for external debug information and debug sources. "
        "If empty, $SYSROOT/usr/lib/debug will be chosen."));
    debuginfoPathChooser->setHistoryCompleter("Debugger.DebugLocation.History");

    historyComboBox = new QComboBox(this);

    buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    auto formLayout = new QFormLayout();
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->addRow(Tr::tr("&Kit:"), kitChooser);
    formLayout->addRow(serverPortLabel, serverPortSpinBox);
    formLayout->addRow(Tr::tr("Local &executable:"), localExecutablePathChooser);
    formLayout->addRow(Tr::tr("Command line &arguments:"), arguments);
    formLayout->addRow(Tr::tr("&Working directory:"), workingDirectory);
    formLayout->addRow(Tr::tr("Run in &terminal:"), runInTerminalCheckBox);
    formLayout->addRow(Tr::tr("Break at \"&main\":"), breakAtMainCheckBox);
    formLayout->addRow(Tr::tr("Use target extended-remote to connect:"), useTargetExtendedRemoteCheckBox);
    formLayout->addRow(sysRootLabel, sysRootPathChooser);
    formLayout->addRow(serverInitCommandsLabel, serverInitCommandsTextEdit);
    formLayout->addRow(serverResetCommandsLabel, serverResetCommandsTextEdit);
    formLayout->addRow(Tr::tr("Debug &information:"), debuginfoPathChooser);
    formLayout->addRow(channelOverrideHintLabel);
    formLayout->addRow(channelOverrideLabel, channelOverrideEdit);
    formLayout->addRow(Layouting::createHr());
    formLayout->addRow(Tr::tr("&Recent:"), historyComboBox);

    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->addLayout(formLayout);
    verticalLayout->addStretch();
    verticalLayout->addWidget(Layouting::createHr());
    verticalLayout->addWidget(buttonBox);

    connect(localExecutablePathChooser, &PathChooser::validChanged,
            this, &StartApplicationDialog::updateState);

    connect(historyComboBox, &QComboBox::currentIndexChanged,
            this, &StartApplicationDialog::historyIndexChanged);

    connect(channelOverrideEdit, &QLineEdit::textChanged,
            this, &StartApplicationDialog::onChannelOverrideChanged);

    updateState();

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void StartApplicationDialog::setHistory(const QList<StartApplicationParameters> &l)
{
    historyComboBox->clear();
    for (int i = l.size(); --i >= 0; ) {
        const StartApplicationParameters &p = l.at(i);
        if (!p.runnable.command.isEmpty())
            historyComboBox->addItem(p.displayName(), QVariant::fromValue(p));
    }
}

void StartApplicationDialog::onChannelOverrideChanged(const QString &channel)
{
    serverPortSpinBox->setEnabled(channel.isEmpty());
    serverPortLabel->setEnabled(channel.isEmpty());
}

void StartApplicationDialog::historyIndexChanged(int index)
{
    if (index < 0)
        return;
    const QVariant v = historyComboBox->itemData(index);
    QTC_ASSERT(v.canConvert<StartApplicationParameters>(), return);
    setParameters(v.value<StartApplicationParameters>());
}

void StartApplicationDialog::updateState()
{
    bool okEnabled = localExecutablePathChooser->isValid();
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(okEnabled);
}

void StartApplicationDialog::run(bool attachRemote)
{
    const Key settingsGroup = "DebugMode";
    const QString arrayName = "StartApplication";

    QList<StartApplicationParameters> history;
    QtcSettings *settings = ICore::settings();
    settings->beginGroup(settingsGroup);
    if (const int arraySize = settings->beginReadArray(arrayName)) {
        for (int i = 0; i < arraySize; ++i) {
            settings->setArrayIndex(i);
            StartApplicationParameters p;
            p.fromSettings(settings);
            history.append(p);
        }
    } else {
        history.append(StartApplicationParameters());
    }
    settings->endArray();
    settings->endGroup();

    StartApplicationDialog dialog;
    dialog.setHistory(history);
    dialog.setParameters(history.back());
    if (!attachRemote) {
        dialog.serverInitCommandsTextEdit->setVisible(false);
        dialog.serverInitCommandsLabel->setVisible(false);
        dialog.serverResetCommandsTextEdit->setVisible(false);
        dialog.serverResetCommandsLabel->setVisible(false);
        dialog.serverPortSpinBox->setVisible(false);
        dialog.serverPortLabel->setVisible(false);
        dialog.channelOverrideHintLabel->setVisible(false);
        dialog.channelOverrideLabel->setVisible(false);
        dialog.channelOverrideEdit->setVisible(false);
    }
    if (dialog.exec() != QDialog::Accepted)
        return;

    Kit *k = dialog.kitChooser->currentKit();

    const StartApplicationParameters newParameters = dialog.parameters();
    if (newParameters != history.back()) {
        history.append(newParameters);
        while (history.size() > 10)
            history.takeFirst();
        settings->beginGroup(settingsGroup);
        settings->beginWriteArray(arrayName);
        for (int i = 0; i < history.size(); ++i) {
            settings->setArrayIndex(i);
            history.at(i).toSettings(settings);
        }
        settings->endArray();
        settings->endGroup();
    }

    IDevice::ConstPtr dev = RunDeviceKitAspect::device(k);
    if (!dev) {
        QMessageBox::critical(
            &dialog, Tr::tr("Cannot debug"), Tr::tr("Cannot debug application: Kit has no device"));
        return;
    }

    auto runControl = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    runControl->setKit(k);

    auto debugger = new DebuggerRunTool(runControl);

    const QString inputAddress = dialog.channelOverrideEdit->text();
    if (!inputAddress.isEmpty())
        debugger->setRemoteChannel(inputAddress);
    else
        debugger->setRemoteChannel(dev->sshParameters().host(), newParameters.serverPort);
    debugger->setRunControlName(newParameters.displayName());
    debugger->setBreakOnMain(newParameters.breakAtMain);
    debugger->setDebugInfoLocation(newParameters.debugInfoLocation);
    debugger->setInferior(newParameters.runnable);
    debugger->setCommandsAfterConnect(newParameters.serverInitCommands);
    debugger->setCommandsForReset(newParameters.serverResetCommands);
    debugger->setUseTerminal(newParameters.runInTerminal);
    debugger->setUseExtendedRemote(newParameters.useTargetExtendedRemote);
    if (!newParameters.sysRoot.isEmpty())
        debugger->setSysRoot(newParameters.sysRoot);

    bool isLocal = dev->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
    if (isLocal) // FIXME: Restriction needed?
        debugger->setInferiorEnvironment(k->runEnvironment());

    if (!attachRemote)
        debugger->runParameters().setStartMode(isLocal ? StartExternal : StartRemoteProcess);

    if (attachRemote) {
        debugger->runParameters().setStartMode(AttachToRemoteServer);
        debugger->setCloseMode(KillAtClose);
        debugger->setUseContinueInsteadOfRun(true);
        debugger->setRunControlName(Tr::tr("Attach to %1").arg(debugger->remoteChannel()));
    }

    runControl->start();
}

void runAttachToRemoteServerDialog()
{
    StartApplicationDialog::run(true);
}

void runStartAndDebugApplicationDialog()
{
    StartApplicationDialog::run(false);
}

StartApplicationParameters StartApplicationDialog::parameters() const
{
    StartApplicationParameters result;
    result.serverPort = serverPortSpinBox->value();
    result.serverAddress = channelOverrideEdit->text();
    result.runnable.command.setExecutable(localExecutablePathChooser->filePath());
    result.sysRoot = sysRootPathChooser->filePath();
    result.serverInitCommands = serverInitCommandsTextEdit->toPlainText();
    result.serverResetCommands = serverResetCommandsTextEdit->toPlainText();
    result.kitId = kitChooser->currentKitId();
    result.debugInfoLocation = debuginfoPathChooser->filePath();
    result.runnable.command.setArguments(arguments->text());
    result.runnable.workingDirectory = workingDirectory->filePath();
    result.breakAtMain = breakAtMainCheckBox->isChecked();
    result.runInTerminal = runInTerminalCheckBox->isChecked();
    result.useTargetExtendedRemote = useTargetExtendedRemoteCheckBox->isChecked();
    return result;
}

void StartApplicationDialog::setParameters(const StartApplicationParameters &p)
{
    kitChooser->setCurrentKitId(p.kitId);
    serverPortSpinBox->setValue(p.serverPort);
    channelOverrideEdit->setText(p.serverAddress);
    localExecutablePathChooser->setFilePath(p.runnable.command.executable());
    sysRootPathChooser->setFilePath(p.sysRoot);
    serverInitCommandsTextEdit->setPlainText(p.serverInitCommands);
    serverResetCommandsTextEdit->setPlainText(p.serverResetCommands);
    debuginfoPathChooser->setFilePath(p.debugInfoLocation);
    arguments->setText(p.runnable.command.arguments());
    workingDirectory->setFilePath(p.runnable.workingDirectory);
    breakAtMainCheckBox->setChecked(p.breakAtMain);
    runInTerminalCheckBox->setChecked(p.runInTerminal);
    useTargetExtendedRemoteCheckBox->setChecked(p.useTargetExtendedRemote);
    updateState();
}

///////////////////////////////////////////////////////////////////////
//
// AttachToQmlPortDialog
//
///////////////////////////////////////////////////////////////////////

class AttachToQmlPortDialog final : public QDialog
{
public:
    AttachToQmlPortDialog();

    int port() const { return m_portSpinBox->value(); }
    void setPort(const int port) { m_portSpinBox->setValue(port); }

    Kit *kit() const { return m_kitChooser->currentKit(); }
    void setKitId(Utils::Id id) { m_kitChooser->setCurrentKitId(id); }

private:
    QSpinBox *m_portSpinBox;
    KitChooser *m_kitChooser;
};

AttachToQmlPortDialog::AttachToQmlPortDialog()
  : QDialog(ICore::dialogParent())
{
    setWindowTitle(Tr::tr("Attach to QML Port"));

    m_kitChooser = new KitChooser(this);
    m_kitChooser->setShowIcons(true);
    m_kitChooser->populate();

    m_portSpinBox = new QSpinBox(this);
    m_portSpinBox->setMaximum(65535);
    m_portSpinBox->setValue(3768);

    auto buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    auto formLayout = new QFormLayout();
    formLayout->addRow(Tr::tr("Kit:"), m_kitChooser);
    formLayout->addRow(Tr::tr("&Port:"), m_portSpinBox);

    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->addLayout(formLayout);
    verticalLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void runAttachToQmlPortDialog()
{
    AttachToQmlPortDialog dlg;
    QtcSettings *settings = ICore::settings();

    const Key lastQmlServerPortKey = "DebugMode/LastQmlServerPort";
    const QVariant qmlServerPort = settings->value(lastQmlServerPortKey);

    if (qmlServerPort.isValid())
        dlg.setPort(qmlServerPort.toInt());
    else
        dlg.setPort(-1);

    const Key lastProfileKey = "DebugMode/LastProfile";
    const Id kitId = Id::fromSetting(settings->value(lastProfileKey));
    if (kitId.isValid())
        dlg.setKitId(kitId);

    if (dlg.exec() != QDialog::Accepted)
        return;

    Kit *kit = dlg.kit();
    QTC_ASSERT(kit, return);
    settings->setValue(lastQmlServerPortKey, dlg.port());
    settings->setValue(lastProfileKey, kit->id().toSetting());

    IDevice::ConstPtr device = RunDeviceKitAspect::device(kit);
    QTC_ASSERT(device, return);

    auto runControl = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    runControl->setKit(kit);
    auto debugger = new DebuggerRunTool(runControl);

    QUrl qmlServer = device->toolControlChannel(IDevice::QmlControlChannel);
    qmlServer.setPort(dlg.port());
    debugger->setQmlServer(qmlServer);

    SshParameters sshParameters = device->sshParameters();
    debugger->setRemoteChannel(sshParameters.host(), sshParameters.port());
    debugger->runParameters().setStartMode(AttachToQmlServer);

    runControl->start();
}

// StartRemoteCdbDialog

static QString cdbRemoteHelp()
{
    const char cdbConnectionSyntax[] =
            "Server:Port<br>"
            "tcp:server=Server,port=Port[,password=Password][,ipversion=6]\n"
            "tcp:clicon=Server,port=Port[,password=Password][,ipversion=6]\n"
            "npipe:server=Server,pipe=PipeName[,password=Password]\n"
            "com:port=COMPort,baud=BaudRate,channel=COMChannel[,password=Password]\n"
            "spipe:proto=Protocol,{certuser=Cert|machuser=Cert},server=Server,pipe=PipeName[,password=Password]\n"
            "ssl:proto=Protocol,{certuser=Cert|machuser=Cert},server=Server,port=Socket[,password=Password]\n"
            "ssl:proto=Protocol,{certuser=Cert|machuser=Cert},clicon=Server,port=Socket[,password=Password]";

    const QString ext32 = QDir::toNativeSeparators(CdbEngine::extensionLibraryName(false));
    const QString ext64 = QDir::toNativeSeparators(CdbEngine::extensionLibraryName(true));
    return Tr::
        tr("<html><body><p>The remote CDB needs to load the matching %1 CDB extension "
           "(<code>%2</code> or <code>%3</code>, respectively).</p><p>Copy it onto the remote "
           "machine and set the "
           "environment variable <code>%4</code> to point to its folder.</p><p>"
           "Launch the remote CDB as <code>%5 &lt;executable&gt;</code> "
           "to use TCP/IP as communication protocol.</p><p>Enter the connection parameters as:</p>"
           "<pre>%6</pre></body></html>")
            .arg(QGuiApplication::applicationDisplayName(),
                 ext32,
                 ext64,
                 QString("_NT_DEBUGGER_EXTENSION_PATH"),
                 QString("cdb.exe -server tcp:port=1234"),
                 QString(cdbConnectionSyntax));
}

class StartRemoteCdbDialog final : public QDialog
{
public:
    StartRemoteCdbDialog();

    QString connection() const;
    void setConnection(const QString &);

private:
    void textChanged(const QString &);
    void accept() override;

    QPushButton *m_okButton = nullptr;
    QLineEdit *m_lineEdit;
};

StartRemoteCdbDialog::StartRemoteCdbDialog()
    : QDialog(ICore::dialogParent()), m_lineEdit(new QLineEdit)
{
    setWindowTitle(Tr::tr("Start a CDB Remote Session"));

    auto groupBox = new QGroupBox;

    auto helpLabel = new QLabel(cdbRemoteHelp());
    helpLabel->setWordWrap(true);
    helpLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

    auto label = new QLabel(Tr::tr("&Connection:"));
    label->setBuddy(m_lineEdit);
    m_lineEdit->setMinimumWidth(400);

    auto box = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);

    auto formLayout = new QFormLayout;
    formLayout->addRow(helpLabel);
    formLayout->addRow(label, m_lineEdit);
    groupBox->setLayout(formLayout);

    auto vLayout = new QVBoxLayout(this);
    vLayout->addWidget(groupBox);
    vLayout->addWidget(box);

    m_okButton = box->button(QDialogButtonBox::Ok);
    m_okButton->setEnabled(false);

    connect(m_lineEdit, &QLineEdit::textChanged, this, &StartRemoteCdbDialog::textChanged);
    connect(m_lineEdit, &QLineEdit::returnPressed, m_okButton, &QAbstractButton::animateClick);
    connect(box, &QDialogButtonBox::accepted, this, &StartRemoteCdbDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void StartRemoteCdbDialog::accept()
{
    if (!m_lineEdit->text().isEmpty())
        QDialog::accept();
}

void StartRemoteCdbDialog::textChanged(const QString &t)
{
    m_okButton->setEnabled(!t.isEmpty());
}

QString StartRemoteCdbDialog::connection() const
{
    const QString rc = m_lineEdit->text();
    // Transform an IP:POrt ('localhost:1234') specification into full spec
    QRegularExpression ipRegexp("([\\w\\.\\-_]+):([0-9]{1,4})");
    QTC_ASSERT(ipRegexp.isValid(), return QString());
    const QRegularExpressionMatch match = ipRegexp.match(rc);
    if (match.hasMatch())
        return QString::fromLatin1("tcp:server=%1,port=%2").arg(match.captured(1), match.captured(2));
    return rc;
}

void StartRemoteCdbDialog::setConnection(const QString &c)
{
    m_lineEdit->setText(c);
    m_okButton->setEnabled(!c.isEmpty());
}

void runStartRemoteCdbSessionDialog(Kit *kit)
{
    QTC_ASSERT(kit, return);
    const Key connectionKey = "DebugMode/CdbRemoteConnection";

    StartRemoteCdbDialog dlg;
    QString previousConnection = ICore::settings()->value(connectionKey).toString();
    if (previousConnection.isEmpty())
        previousConnection = "localhost:1234";
    dlg.setConnection(previousConnection);
    if (dlg.exec() != QDialog::Accepted)
        return;

    ICore::settings()->setValue(connectionKey, dlg.connection());

    auto runControl = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    runControl->setKit(kit);

    auto debugger = new DebuggerRunTool(runControl);
    DebuggerRunParameters &rp = debugger->runParameters();
    rp.setStartMode(AttachToRemoteServer);
    debugger->setCloseMode(KillAtClose);
    debugger->setRemoteChannel(dlg.connection());

    runControl->start();
}

//
// AddressDialog
//

class AddressDialog final : public QDialog
{
public:
     AddressDialog();

     void setAddress(quint64 a) { m_lineEdit->setText("0x" + QString::number(a, 16)); }
     quint64 address() const { return m_lineEdit->text().toULongLong(nullptr, 16); }

     void setOkButtonEnabled(bool v) { m_box->button(QDialogButtonBox::Ok)->setEnabled(v); }
     bool isOkButtonEnabled() const { return m_box->button(QDialogButtonBox::Ok)->isEnabled(); }

private:
     void accept() override;

     QLineEdit *m_lineEdit;
     QDialogButtonBox *m_box;
};

AddressDialog::AddressDialog()
    : QDialog(ICore::dialogParent()),
      m_lineEdit(new QLineEdit),
      m_box(new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel))
{
    setWindowTitle(Tr::tr("Select Start Address"));

    auto hLayout = new QHBoxLayout;
    hLayout->addWidget(new QLabel(Tr::tr("Enter an address:") + ' '));
    hLayout->addWidget(m_lineEdit);

    auto vLayout = new QVBoxLayout;
    vLayout->addLayout(hLayout);
    vLayout->addWidget(m_box);
    setLayout(vLayout);

    connect(m_box, &QDialogButtonBox::accepted, this, &AddressDialog::accept);
    connect(m_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_lineEdit, &QLineEdit::returnPressed, this, &AddressDialog::accept);

    connect(m_lineEdit, &QLineEdit::textChanged, this, [this] {
        const QString text = m_lineEdit->text();
        bool ok = false;
        text.toULongLong(&ok, 16);
        setOkButtonEnabled(ok);
    });

    setOkButtonEnabled(false);
}

void AddressDialog::accept()
{
    if (isOkButtonEnabled())
        QDialog::accept();
}

std::optional<quint64> runAddressDialog(quint64 initialAddress)
{
    AddressDialog dialog;
    dialog.setAddress(initialAddress);

    if (dialog.exec() != QDialog::Accepted)
        return {};

    return dialog.address();
}

} // Debugger::Internal

Q_DECLARE_METATYPE(Debugger::Internal::StartApplicationParameters)
