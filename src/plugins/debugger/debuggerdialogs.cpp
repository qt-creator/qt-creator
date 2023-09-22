// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debuggerdialogs.h"

#include "cdb/cdbengine.h"
#include "debuggerruncontrol.h"
#include "debuggertr.h"

#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/sshparameters.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>

#include <utils/fancylineedit.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QGroupBox>
#include <QGuiApplication>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSpinBox>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Debugger::Internal {

///////////////////////////////////////////////////////////////////////
//
// StartApplicationDialogPrivate
//
///////////////////////////////////////////////////////////////////////

class StartApplicationDialogPrivate
{
public:
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

StartApplicationDialog::StartApplicationDialog(QWidget *parent)
  : QDialog(parent), d(new StartApplicationDialogPrivate)
{
    setWindowTitle(Tr::tr("Start Debugger"));

    d->kitChooser = new KitChooser(this);
    d->kitChooser->setShowIcons(true);
    d->kitChooser->populate();

    d->serverPortLabel = new QLabel(Tr::tr("Server port:"), this);
    d->serverPortSpinBox = new QSpinBox(this);
    d->serverPortSpinBox->setRange(1, 65535);

    d->channelOverrideHintLabel =
            new QLabel(Tr::tr("Normally, the running server is identified by the IP of the "
                          "device in the kit and the server port selected above.\n"
                          "You can choose another communication channel here, such as "
                          "a serial line or custom ip:port."));

    d->channelOverrideLabel = new QLabel(Tr::tr("Override server channel:"), this);
    d->channelOverrideEdit = new QLineEdit(this);
    //: "For example, /dev/ttyS0, COM1, 127.0.0.1:1234"
    d->channelOverrideEdit->setPlaceholderText(
        Tr::tr("For example, %1").arg("/dev/ttyS0, COM1, 127.0.0.1:1234"));

    d->localExecutablePathChooser = new PathChooser(this);
    d->localExecutablePathChooser->setExpectedKind(PathChooser::File);
    d->localExecutablePathChooser->setPromptDialogTitle(Tr::tr("Select Executable"));
    d->localExecutablePathChooser->setHistoryCompleter("LocalExecutable");

    d->arguments = new FancyLineEdit(this);
    d->arguments->setClearButtonEnabled(true);
    d->arguments->setHistoryCompleter("CommandlineArguments");

    d->workingDirectory = new PathChooser(this);
    d->workingDirectory->setExpectedKind(PathChooser::ExistingDirectory);
    d->workingDirectory->setPromptDialogTitle(Tr::tr("Select Working Directory"));
    d->workingDirectory->setHistoryCompleter("WorkingDirectory");

    d->runInTerminalCheckBox = new QCheckBox(this);

    d->breakAtMainCheckBox = new QCheckBox(this);
    d->breakAtMainCheckBox->setText(QString());

    d->useTargetExtendedRemoteCheckBox = new QCheckBox(this);

    d->sysRootPathChooser = new PathChooser(this);
    d->sysRootPathChooser->setExpectedKind(PathChooser::Directory);
    d->sysRootPathChooser->setHistoryCompleter("Debugger.SysRoot.History");
    d->sysRootPathChooser->setPromptDialogTitle(Tr::tr("Select SysRoot Directory"));
    d->sysRootPathChooser->setToolTip(Tr::tr(
        "This option can be used to override the kit's SysRoot setting."));
    d->sysRootLabel = new QLabel(Tr::tr("Override S&ysRoot:"), this);
    d->sysRootLabel->setBuddy(d->sysRootPathChooser);
    d->sysRootLabel->setToolTip(d->sysRootPathChooser->toolTip());

    d->serverInitCommandsTextEdit = new QPlainTextEdit(this);
    d->serverInitCommandsTextEdit->setToolTip(Tr::tr(
        "This option can be used to send the target init commands."));

    d->serverInitCommandsLabel = new QLabel(Tr::tr("&Init commands:"), this);
    d->serverInitCommandsLabel->setBuddy(d->serverInitCommandsTextEdit);
    d->serverInitCommandsLabel->setToolTip(d->serverInitCommandsTextEdit->toolTip());

    d->serverResetCommandsTextEdit = new QPlainTextEdit(this);
    d->serverResetCommandsTextEdit->setToolTip(Tr::tr(
        "This option can be used to send the target reset commands."));

    d->serverResetCommandsLabel = new QLabel(Tr::tr("&Reset commands:"), this);
    d->serverResetCommandsLabel->setBuddy(d->serverResetCommandsTextEdit);
    d->serverResetCommandsLabel->setToolTip(d->serverResetCommandsTextEdit->toolTip());

    d->debuginfoPathChooser = new PathChooser(this);
    d->debuginfoPathChooser->setPromptDialogTitle(Tr::tr("Select Location of Debugging Information"));
    d->debuginfoPathChooser->setToolTip(Tr::tr(
        "Base path for external debug information and debug sources. "
        "If empty, $SYSROOT/usr/lib/debug will be chosen."));
    d->debuginfoPathChooser->setHistoryCompleter("Debugger.DebugLocation.History");

    d->historyComboBox = new QComboBox(this);

    d->buttonBox = new QDialogButtonBox(this);
    d->buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    d->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    auto formLayout = new QFormLayout();
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->addRow(Tr::tr("&Kit:"), d->kitChooser);
    formLayout->addRow(d->serverPortLabel, d->serverPortSpinBox);
    formLayout->addRow(Tr::tr("Local &executable:"), d->localExecutablePathChooser);
    formLayout->addRow(Tr::tr("Command line &arguments:"), d->arguments);
    formLayout->addRow(Tr::tr("&Working directory:"), d->workingDirectory);
    formLayout->addRow(Tr::tr("Run in &terminal:"), d->runInTerminalCheckBox);
    formLayout->addRow(Tr::tr("Break at \"&main\":"), d->breakAtMainCheckBox);
    formLayout->addRow(Tr::tr("Use target extended-remote to connect:"), d->useTargetExtendedRemoteCheckBox);
    formLayout->addRow(d->sysRootLabel, d->sysRootPathChooser);
    formLayout->addRow(d->serverInitCommandsLabel, d->serverInitCommandsTextEdit);
    formLayout->addRow(d->serverResetCommandsLabel, d->serverResetCommandsTextEdit);
    formLayout->addRow(Tr::tr("Debug &information:"), d->debuginfoPathChooser);
    formLayout->addRow(d->channelOverrideHintLabel);
    formLayout->addRow(d->channelOverrideLabel, d->channelOverrideEdit);
    formLayout->addRow(Layouting::createHr());
    formLayout->addRow(Tr::tr("&Recent:"), d->historyComboBox);

    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->addLayout(formLayout);
    verticalLayout->addStretch();
    verticalLayout->addWidget(Layouting::createHr());
    verticalLayout->addWidget(d->buttonBox);

    connect(d->localExecutablePathChooser,
            &PathChooser::validChanged,
            this,
            &StartApplicationDialog::updateState);

    connect(d->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(d->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(d->historyComboBox, &QComboBox::currentIndexChanged,
            this, &StartApplicationDialog::historyIndexChanged);

    connect(d->channelOverrideEdit, &QLineEdit::textChanged,
            this, &StartApplicationDialog::onChannelOverrideChanged);

    updateState();
}

StartApplicationDialog::~StartApplicationDialog()
{
    delete d;
}

void StartApplicationDialog::setHistory(const QList<StartApplicationParameters> &l)
{
    d->historyComboBox->clear();
    for (int i = l.size(); --i >= 0; ) {
        const StartApplicationParameters &p = l.at(i);
        if (!p.runnable.command.isEmpty())
            d->historyComboBox->addItem(p.displayName(), QVariant::fromValue(p));
    }
}

void StartApplicationDialog::onChannelOverrideChanged(const QString &channel)
{
    d->serverPortSpinBox->setEnabled(channel.isEmpty());
    d->serverPortLabel->setEnabled(channel.isEmpty());
}

void StartApplicationDialog::historyIndexChanged(int index)
{
    if (index < 0)
        return;
    const QVariant v = d->historyComboBox->itemData(index);
    QTC_ASSERT(v.canConvert<StartApplicationParameters>(), return);
    setParameters(v.value<StartApplicationParameters>());
}

void StartApplicationDialog::updateState()
{
    bool okEnabled = d->localExecutablePathChooser->isValid();
    d->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(okEnabled);
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

    StartApplicationDialog dialog(ICore::dialogParent());
    dialog.setHistory(history);
    dialog.setParameters(history.back());
    if (!attachRemote) {
        dialog.d->serverInitCommandsTextEdit->setVisible(false);
        dialog.d->serverInitCommandsLabel->setVisible(false);
        dialog.d->serverResetCommandsTextEdit->setVisible(false);
        dialog.d->serverResetCommandsLabel->setVisible(false);
        dialog.d->serverPortSpinBox->setVisible(false);
        dialog.d->serverPortLabel->setVisible(false);
        dialog.d->channelOverrideHintLabel->setVisible(false);
        dialog.d->channelOverrideLabel->setVisible(false);
        dialog.d->channelOverrideEdit->setVisible(false);
    }
    if (dialog.exec() != QDialog::Accepted)
        return;

    Kit *k = dialog.d->kitChooser->currentKit();

    auto runControl = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    runControl->setKit(k);
    auto debugger = new DebuggerRunTool(runControl);

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

    IDevice::ConstPtr dev = DeviceKitAspect::device(k);
    ProcessRunData inferior = newParameters.runnable;
    const QString inputAddress = dialog.d->channelOverrideEdit->text();
    if (!inputAddress.isEmpty())
        debugger->setRemoteChannel(inputAddress);
    else
        debugger->setRemoteChannel(dev->sshParameters().host(), newParameters.serverPort);
    debugger->setRunControlName(newParameters.displayName());
    debugger->setBreakOnMain(newParameters.breakAtMain);
    debugger->setDebugInfoLocation(newParameters.debugInfoLocation);
    debugger->setInferior(inferior);
    debugger->setCommandsAfterConnect(newParameters.serverInitCommands);
    debugger->setCommandsForReset(newParameters.serverResetCommands);
    debugger->setUseTerminal(newParameters.runInTerminal);
    debugger->setUseExtendedRemote(newParameters.useTargetExtendedRemote);
    if (!newParameters.sysRoot.isEmpty())
        debugger->setSysRoot(newParameters.sysRoot);

    bool isLocal = !dev || (dev->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
    if (isLocal) // FIXME: Restriction needed?
        debugger->setInferiorEnvironment(k->runEnvironment());

    if (!attachRemote)
        debugger->setStartMode(isLocal ? StartExternal : StartRemoteProcess);

    if (attachRemote) {
        debugger->setStartMode(AttachToRemoteServer);
        debugger->setCloseMode(KillAtClose);
        debugger->setUseContinueInsteadOfRun(true);
        debugger->setRunControlName(Tr::tr("Attach to %1").arg(debugger->remoteChannel()));
    }
    debugger->startRunControl();
}

void StartApplicationDialog::attachToRemoteServer()
{
    run(true);
}

void StartApplicationDialog::startAndDebugApplication()
{
    run(false);
}

StartApplicationParameters StartApplicationDialog::parameters() const
{
    StartApplicationParameters result;
    result.serverPort = d->serverPortSpinBox->value();
    result.serverAddress = d->channelOverrideEdit->text();
    result.runnable.command.setExecutable(d->localExecutablePathChooser->filePath());
    result.sysRoot = d->sysRootPathChooser->filePath();
    result.serverInitCommands = d->serverInitCommandsTextEdit->toPlainText();
    result.serverResetCommands = d->serverResetCommandsTextEdit->toPlainText();
    result.kitId = d->kitChooser->currentKitId();
    result.debugInfoLocation = d->debuginfoPathChooser->filePath();
    result.runnable.command.setArguments(d->arguments->text());
    result.runnable.workingDirectory = d->workingDirectory->filePath();
    result.breakAtMain = d->breakAtMainCheckBox->isChecked();
    result.runInTerminal = d->runInTerminalCheckBox->isChecked();
    result.useTargetExtendedRemote = d->useTargetExtendedRemoteCheckBox->isChecked();
    return result;
}

void StartApplicationDialog::setParameters(const StartApplicationParameters &p)
{
    d->kitChooser->setCurrentKitId(p.kitId);
    d->serverPortSpinBox->setValue(p.serverPort);
    d->channelOverrideEdit->setText(p.serverAddress);
    d->localExecutablePathChooser->setFilePath(p.runnable.command.executable());
    d->sysRootPathChooser->setFilePath(p.sysRoot);
    d->serverInitCommandsTextEdit->setPlainText(p.serverInitCommands);
    d->serverResetCommandsTextEdit->setPlainText(p.serverResetCommands);
    d->debuginfoPathChooser->setFilePath(p.debugInfoLocation);
    d->arguments->setText(p.runnable.command.arguments());
    d->workingDirectory->setFilePath(p.runnable.workingDirectory);
    d->breakAtMainCheckBox->setChecked(p.breakAtMain);
    d->runInTerminalCheckBox->setChecked(p.runInTerminal);
    d->useTargetExtendedRemoteCheckBox->setChecked(p.useTargetExtendedRemote);
    updateState();
}

///////////////////////////////////////////////////////////////////////
//
// AttachToQmlPortDialog
//
///////////////////////////////////////////////////////////////////////

class AttachToQmlPortDialogPrivate
{
public:
    QSpinBox *portSpinBox;
    KitChooser *kitChooser;
};

AttachToQmlPortDialog::AttachToQmlPortDialog(QWidget *parent)
  : QDialog(parent),
    d(new AttachToQmlPortDialogPrivate)
{
    setWindowTitle(Tr::tr("Start Debugger"));

    d->kitChooser = new KitChooser(this);
    d->kitChooser->setShowIcons(true);
    d->kitChooser->populate();

    d->portSpinBox = new QSpinBox(this);
    d->portSpinBox->setMaximum(65535);
    d->portSpinBox->setValue(3768);

    auto buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    auto formLayout = new QFormLayout();
    formLayout->addRow(Tr::tr("Kit:"), d->kitChooser);
    formLayout->addRow(Tr::tr("&Port:"), d->portSpinBox);

    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->addLayout(formLayout);
    verticalLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

AttachToQmlPortDialog::~AttachToQmlPortDialog()
{
    delete d;
}

void AttachToQmlPortDialog::setPort(const int port)
{
    d->portSpinBox->setValue(port);
}

int AttachToQmlPortDialog::port() const
{
    return d->portSpinBox->value();
}

Kit *AttachToQmlPortDialog::kit() const
{
    return d->kitChooser->currentKit();
}

void AttachToQmlPortDialog::setKitId(Id id)
{
    d->kitChooser->setCurrentKitId(id);
}

// --------- StartRemoteCdbDialog
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

StartRemoteCdbDialog::StartRemoteCdbDialog(QWidget *parent) :
    QDialog(parent), m_lineEdit(new QLineEdit)
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

StartRemoteCdbDialog::~StartRemoteCdbDialog() = default;

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

AddressDialog::AddressDialog(QWidget *parent) :
        QDialog(parent),
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
    connect(m_lineEdit, &QLineEdit::textChanged, this, &AddressDialog::textChanged);

    setOkButtonEnabled(false);
}

void AddressDialog::setOkButtonEnabled(bool v)
{
    m_box->button(QDialogButtonBox::Ok)->setEnabled(v);
}

bool AddressDialog::isOkButtonEnabled() const
{
    return m_box->button(QDialogButtonBox::Ok)->isEnabled();
}

void AddressDialog::setAddress(quint64 a)
{
    m_lineEdit->setText("0x" + QString::number(a, 16));
}

quint64 AddressDialog::address() const
{
    return m_lineEdit->text().toULongLong(nullptr, 16);
}

void AddressDialog::accept()
{
    if (isOkButtonEnabled())
        QDialog::accept();
}

void AddressDialog::textChanged()
{
    setOkButtonEnabled(isValid());
}

bool AddressDialog::isValid() const
{
    const QString text = m_lineEdit->text();
    bool ok = false;
    text.toULongLong(&ok, 16);
    return ok;
}

///////////////////////////////////////////////////////////////////////
//
// StartRemoteEngineDialog
//
///////////////////////////////////////////////////////////////////////

class StartRemoteEngineDialogPrivate
{
public:
    FancyLineEdit *host;
    FancyLineEdit *username;
    QLineEdit *password;
    FancyLineEdit *enginePath;
    FancyLineEdit *inferiorPath;
    QDialogButtonBox *buttonBox;
};

StartRemoteEngineDialog::StartRemoteEngineDialog(QWidget *parent)
    : QDialog(parent), d(new StartRemoteEngineDialogPrivate)
{
    setWindowTitle(Tr::tr("Start Remote Engine"));

    d->host = new FancyLineEdit(this);
    d->host->setHistoryCompleter("HostName");

    d->username = new FancyLineEdit(this);
    d->username->setHistoryCompleter("UserName");

    d->password = new QLineEdit(this);
    d->password->setEchoMode(QLineEdit::Password);

    d->enginePath = new FancyLineEdit(this);
    d->enginePath->setHistoryCompleter("EnginePath");

    d->inferiorPath = new FancyLineEdit(this);
    d->inferiorPath->setHistoryCompleter("InferiorPath");

    d->buttonBox = new QDialogButtonBox(this);
    d->buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    auto formLayout = new QFormLayout();
    formLayout->addRow(Tr::tr("&Host:"), d->host);
    formLayout->addRow(Tr::tr("&Username:"), d->username);
    formLayout->addRow(Tr::tr("&Password:"), d->password);
    formLayout->addRow(Tr::tr("&Engine path:"), d->enginePath);
    formLayout->addRow(Tr::tr("&Inferior path:"), d->inferiorPath);

    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->addLayout(formLayout);
    verticalLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    verticalLayout->addWidget(d->buttonBox);

    connect(d->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(d->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

StartRemoteEngineDialog::~StartRemoteEngineDialog()
{
    delete d;
}

QString StartRemoteEngineDialog::host() const
{
    return d->host->text();
}

QString StartRemoteEngineDialog::username() const
{
    return d->username->text();
}

QString StartRemoteEngineDialog::password() const
{
    return d->password->text();
}

QString StartRemoteEngineDialog::inferiorPath() const
{
    return d->inferiorPath->text();
}

QString StartRemoteEngineDialog::enginePath() const
{
    return d->enginePath->text();
}

///////////////////////////////////////////////////////////////////////
//
// TypeFormatsDialogUi
//
///////////////////////////////////////////////////////////////////////

class TypeFormatsDialogPage : public QWidget
{
public:
    TypeFormatsDialogPage()
    {
        m_layout = new QGridLayout;
        m_layout->setColumnStretch(0, 2);
        auto vboxLayout = new QVBoxLayout;
        vboxLayout->addLayout(m_layout);
        vboxLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Ignored,
                                            QSizePolicy::MinimumExpanding));
        setLayout(vboxLayout);
    }

    void addTypeFormats(const QString &type,
        const DisplayFormats &typeFormats, int current)
    {
        const int row = m_layout->rowCount();
        int column = 0;
        auto group = new QButtonGroup(this);
        m_layout->addWidget(new QLabel(type), row, column++);
        for (int i = -1; i != typeFormats.size(); ++i) {
            auto choice = new QRadioButton(this);
            choice->setText(i == -1 ? Tr::tr("Reset")
                                    : WatchHandler::nameForFormat(typeFormats.at(i)));
            m_layout->addWidget(choice, row, column++);
            if (i == current)
                choice->setChecked(true);
            group->addButton(choice, i);
        }
    }
private:
    QGridLayout *m_layout;
};

class TypeFormatsDialogUi
{
public:
    TypeFormatsDialogUi(TypeFormatsDialog *q)
    {
        tabs = new QTabWidget(q);

        buttonBox = new QDialogButtonBox(q);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        auto layout = new QVBoxLayout(q);
        layout->addWidget(tabs);
        layout->addWidget(buttonBox);
        q->setLayout(layout);
    }

    void addPage(const QString &name)
    {
        auto page = new TypeFormatsDialogPage;
        pages.append(page);
        auto scroller = new QScrollArea;
        scroller->setWidgetResizable(true);
        scroller->setWidget(page);
        scroller->setFrameStyle(QFrame::NoFrame);
        tabs->addTab(scroller, name);
    }

public:
    QList<TypeFormatsDialogPage *> pages;
    QDialogButtonBox *buttonBox;

private:
    QTabWidget *tabs;
};


///////////////////////////////////////////////////////////////////////
//
// TypeFormatsDialog
//
///////////////////////////////////////////////////////////////////////

TypeFormatsDialog::TypeFormatsDialog(QWidget *parent)
   : QDialog(parent), m_ui(new TypeFormatsDialogUi(this))
{
    setWindowTitle(Tr::tr("Type Formats"));
    m_ui->addPage(Tr::tr("Qt Types"));
    m_ui->addPage(Tr::tr("Standard Types"));
    m_ui->addPage(Tr::tr("Misc Types"));

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

TypeFormatsDialog::~TypeFormatsDialog()
{
    delete m_ui;
}

void TypeFormatsDialog::addTypeFormats(const QString &type0,
    const DisplayFormats &typeFormats, int current)
{
    QString type = type0;
    type.replace("__", "::");
    int pos = 2;
    if (type.startsWith('Q'))
        pos = 0;
    else if (type.startsWith("std::"))
        pos = 1;
    m_ui->pages[pos]->addTypeFormats(type, typeFormats, current);
}

} // Debugger::Internal

Q_DECLARE_METATYPE(Debugger::Internal::StartApplicationParameters)
