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

#include "debuggerdialogs.h"

#include "debuggerkitinformation.h"
#include "debuggerruncontrol.h"
#include "cdb/cdbengine.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>

#include <app/app_version.h>
#include <utils/pathchooser.h>
#include <utils/fancylineedit.h>
#include <utils/qtcassert.h>

#include <ssh/sshconnection.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QGroupBox>
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

namespace Debugger {
namespace Internal {

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
    PathChooser *debuginfoPathChooser;
    QLabel *serverStartScriptLabel;
    PathChooser *serverStartScriptPathChooser;
    QLabel *sysRootLabel;
    PathChooser *sysRootPathChooser;
    QLabel *serverInitCommandsLabel;
    QPlainTextEdit *serverInitCommandsTextEdit;
    QLabel *serverResetCommandsLabel;
    QPlainTextEdit *serverResetCommandsTextEdit;
    QComboBox *historyComboBox;
    QDialogButtonBox *buttonBox;
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::StartApplicationParameters)

namespace Debugger {
namespace Internal {

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
    void toSettings(QSettings *) const;
    void fromSettings(const QSettings *settings);

    bool operator==(const StartApplicationParameters &p) const { return equals(p); }
    bool operator!=(const StartApplicationParameters &p) const { return !equals(p); }

    Id kitId;
    uint serverPort;
    QString serverAddress;
    Runnable runnable;
    bool breakAtMain = false;
    bool runInTerminal = false;
    FilePath serverStartScript;
    FilePath sysRoot;
    QString serverInitCommands;
    QString serverResetCommands;
    QString debugInfoLocation;
};

bool StartApplicationParameters::equals(const StartApplicationParameters &rhs) const
{
    return runnable.executable == rhs.runnable.executable
        && serverPort == rhs.serverPort
        && runnable.commandLineArguments == rhs.runnable.commandLineArguments
        && runnable.workingDirectory == rhs.runnable.workingDirectory
        && breakAtMain == rhs.breakAtMain
        && runInTerminal == rhs.runInTerminal
        && serverStartScript == rhs.serverStartScript
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

    QString name = runnable.executable.fileName()
            + ' ' + runnable.commandLineArguments;
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

void StartApplicationParameters::toSettings(QSettings *settings) const
{
    settings->setValue("LastKitId", kitId.toSetting());
    settings->setValue("LastServerPort", serverPort);
    settings->setValue("LastServerAddress", serverAddress);
    settings->setValue("LastExternalExecutable", runnable.executable.toVariant());
    settings->setValue("LastExternalExecutableArguments", runnable.commandLineArguments);
    settings->setValue("LastExternalWorkingDirectory", runnable.workingDirectory);
    settings->setValue("LastExternalBreakAtMain", breakAtMain);
    settings->setValue("LastExternalRunInTerminal", runInTerminal);
    settings->setValue("LastServerStartScript", serverStartScript.toVariant());
    settings->setValue("LastServerInitCommands", serverInitCommands);
    settings->setValue("LastServerResetCommands", serverResetCommands);
    settings->setValue("LastDebugInfoLocation", debugInfoLocation);
    settings->setValue("LastSysRoot", sysRoot.toVariant());
}

void StartApplicationParameters::fromSettings(const QSettings *settings)
{
    kitId = Id::fromSetting(settings->value("LastKitId"));
    serverPort = settings->value("LastServerPort").toUInt();
    serverAddress = settings->value("LastServerAddress").toString();
    runnable.executable = FilePath::fromVariant(settings->value("LastExternalExecutable"));
    runnable.commandLineArguments = settings->value("LastExternalExecutableArguments").toString();
    runnable.workingDirectory = settings->value("LastExternalWorkingDirectory").toString();
    breakAtMain = settings->value("LastExternalBreakAtMain").toBool();
    runInTerminal = settings->value("LastExternalRunInTerminal").toBool();
    serverStartScript = FilePath::fromVariant(settings->value("LastServerStartScript"));
    serverInitCommands = settings->value("LastServerInitCommands").toString();
    serverResetCommands = settings->value("LastServerResetCommands").toString();
    debugInfoLocation = settings->value("LastDebugInfoLocation").toString();
    sysRoot = FilePath::fromVariant(settings->value("LastSysRoot"));
}

///////////////////////////////////////////////////////////////////////
//
// StartApplicationDialog
//
///////////////////////////////////////////////////////////////////////

StartApplicationDialog::StartApplicationDialog(QWidget *parent)
  : QDialog(parent), d(new StartApplicationDialogPrivate)
{
    setWindowTitle(tr("Start Debugger"));

    d->kitChooser = new KitChooser(this);
    d->kitChooser->setShowIcons(true);
    d->kitChooser->populate();

    d->serverPortLabel = new QLabel(tr("Server port:"), this);
    d->serverPortSpinBox = new QSpinBox(this);
    d->serverPortSpinBox->setRange(1, 65535);

    d->channelOverrideHintLabel =
            new QLabel(tr("Normally, the running server is identified by the IP of the "
                          "device in the kit and the server port selected above.\n"
                          "You can choose another communication channel here, such as "
                          "a serial line or custom ip:port."));

    d->channelOverrideLabel = new QLabel(tr("Override server channel:"), this);
    d->channelOverrideEdit = new QLineEdit(this);
    //: "For example, /dev/ttyS0, COM1, 127.0.0.1:1234"
    d->channelOverrideEdit->setPlaceholderText(
        tr("For example, %1").arg("/dev/ttyS0, COM1, 127.0.0.1:1234"));

    d->localExecutablePathChooser = new PathChooser(this);
    d->localExecutablePathChooser->setExpectedKind(PathChooser::File);
    d->localExecutablePathChooser->setPromptDialogTitle(tr("Select Executable"));
    d->localExecutablePathChooser->setHistoryCompleter("LocalExecutable");

    d->arguments = new FancyLineEdit(this);
    d->arguments->setHistoryCompleter("CommandlineArguments");

    d->workingDirectory = new PathChooser(this);
    d->workingDirectory->setExpectedKind(PathChooser::ExistingDirectory);
    d->workingDirectory->setPromptDialogTitle(tr("Select Working Directory"));
    d->workingDirectory->setHistoryCompleter("WorkingDirectory");

    d->runInTerminalCheckBox = new QCheckBox(this);

    d->breakAtMainCheckBox = new QCheckBox(this);
    d->breakAtMainCheckBox->setText(QString());

    d->serverStartScriptPathChooser = new PathChooser(this);
    d->serverStartScriptPathChooser->setExpectedKind(PathChooser::File);
    d->serverStartScriptPathChooser->setPromptDialogTitle(tr("Select Server Start Script"));
    d->serverStartScriptPathChooser->setToolTip(tr(
        "This option can be used to point to a script that will be used "
        "to start a debug server. If the field is empty, "
        "default methods to set up debug servers will be used."));
    d->serverStartScriptLabel = new QLabel(tr("&Server start script:"), this);
    d->serverStartScriptLabel->setBuddy(d->serverStartScriptPathChooser);
    d->serverStartScriptLabel->setToolTip(d->serverStartScriptPathChooser->toolTip());

    d->sysRootPathChooser = new PathChooser(this);
    d->sysRootPathChooser->setExpectedKind(PathChooser::Directory);
    d->sysRootPathChooser->setHistoryCompleter("Debugger.SysRoot.History");
    d->sysRootPathChooser->setPromptDialogTitle(tr("Select SysRoot Directory"));
    d->sysRootPathChooser->setToolTip(tr(
        "This option can be used to override the kit's SysRoot setting."));
    d->sysRootLabel = new QLabel(tr("Override S&ysRoot:"), this);
    d->sysRootLabel->setBuddy(d->sysRootPathChooser);
    d->sysRootLabel->setToolTip(d->sysRootPathChooser->toolTip());

    d->serverInitCommandsTextEdit = new QPlainTextEdit(this);
    d->serverInitCommandsTextEdit->setToolTip(tr(
        "This option can be used to send the target init commands."));

    d->serverInitCommandsLabel = new QLabel(tr("&Init commands:"), this);
    d->serverInitCommandsLabel->setBuddy(d->serverInitCommandsTextEdit);
    d->serverInitCommandsLabel->setToolTip(d->serverInitCommandsTextEdit->toolTip());

    d->serverResetCommandsTextEdit = new QPlainTextEdit(this);
    d->serverResetCommandsTextEdit->setToolTip(tr(
        "This option can be used to send the target reset commands."));

    d->serverResetCommandsLabel = new QLabel(tr("&Reset commands:"), this);
    d->serverResetCommandsLabel->setBuddy(d->serverResetCommandsTextEdit);
    d->serverResetCommandsLabel->setToolTip(d->serverResetCommandsTextEdit->toolTip());

    d->debuginfoPathChooser = new PathChooser(this);
    d->debuginfoPathChooser->setPromptDialogTitle(tr("Select Location of Debugging Information"));
    d->debuginfoPathChooser->setToolTip(tr(
        "Base path for external debug information and debug sources. "
        "If empty, $SYSROOT/usr/lib/debug will be chosen."));
    d->debuginfoPathChooser->setHistoryCompleter("Debugger.DebugLocation.History");

    auto line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    auto line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);

    d->historyComboBox = new QComboBox(this);

    d->buttonBox = new QDialogButtonBox(this);
    d->buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    d->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    auto formLayout = new QFormLayout();
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->addRow(tr("&Kit:"), d->kitChooser);
    formLayout->addRow(d->serverPortLabel, d->serverPortSpinBox);
    formLayout->addRow(tr("Local &executable:"), d->localExecutablePathChooser);
    formLayout->addRow(tr("Command line &arguments:"), d->arguments);
    formLayout->addRow(tr("&Working directory:"), d->workingDirectory);
    formLayout->addRow(tr("Run in &terminal:"), d->runInTerminalCheckBox);
    formLayout->addRow(tr("Break at \"&main\":"), d->breakAtMainCheckBox);
    formLayout->addRow(d->serverStartScriptLabel, d->serverStartScriptPathChooser);
    formLayout->addRow(d->sysRootLabel, d->sysRootPathChooser);
    formLayout->addRow(d->serverInitCommandsLabel, d->serverInitCommandsTextEdit);
    formLayout->addRow(d->serverResetCommandsLabel, d->serverResetCommandsTextEdit);
    formLayout->addRow(tr("Debug &information:"), d->debuginfoPathChooser);
    formLayout->addRow(d->channelOverrideHintLabel);
    formLayout->addRow(d->channelOverrideLabel, d->channelOverrideEdit);
    formLayout->addRow(line2);
    formLayout->addRow(tr("&Recent:"), d->historyComboBox);

    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->addLayout(formLayout);
    verticalLayout->addStretch();
    verticalLayout->addWidget(line);
    verticalLayout->addWidget(d->buttonBox);

    connect(d->localExecutablePathChooser, &PathChooser::rawPathChanged,
            this, &StartApplicationDialog::updateState);
    connect(d->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(d->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(d->historyComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
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
        if (!p.runnable.executable.isEmpty())
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
    const QString settingsGroup = "DebugMode";
    const QString arrayName = "StartApplication";

    QList<StartApplicationParameters> history;
    QSettings *settings = ICore::settings();
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
        dialog.d->serverStartScriptPathChooser->setVisible(false);
        dialog.d->serverStartScriptLabel->setVisible(false);
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
    Runnable inferior = newParameters.runnable;
    const QString inputAddress = dialog.d->channelOverrideEdit->text();
    if (!inputAddress.isEmpty())
        debugger->setRemoteChannel(inputAddress);
    else
        debugger->setRemoteChannel(dev->sshParameters().host(), newParameters.serverPort);
    debugger->setRunControlName(newParameters.displayName());
    debugger->setBreakOnMain(newParameters.breakAtMain);
    debugger->setDebugInfoLocation(newParameters.debugInfoLocation);
    debugger->setInferior(inferior);
    debugger->setServerStartScript(newParameters.serverStartScript); // Note: This requires inferior.
    debugger->setCommandsAfterConnect(newParameters.serverInitCommands);
    debugger->setCommandsForReset(newParameters.serverResetCommands);
    debugger->setUseTerminal(newParameters.runInTerminal);
    debugger->setSysRoot(newParameters.sysRoot);

    bool isLocal = !dev || (dev->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
    if (isLocal) {
        Environment inferiorEnvironment = Environment::systemEnvironment();
        k->addToEnvironment(inferiorEnvironment);
        debugger->setInferiorEnvironment(inferiorEnvironment);
    }
    if (!attachRemote)
        debugger->setStartMode(isLocal ? StartExternal : StartRemoteProcess);

    if (attachRemote) {
        debugger->setStartMode(AttachToRemoteServer);
        debugger->setCloseMode(KillAtClose);
        debugger->setUseContinueInsteadOfRun(true);
        debugger->setRunControlName(tr("Attach to %1").arg(debugger->remoteChannel()));
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
    result.runnable.executable = d->localExecutablePathChooser->filePath();
    result.serverStartScript = d->serverStartScriptPathChooser->filePath();
    result.sysRoot = d->sysRootPathChooser->filePath();
    result.serverInitCommands = d->serverInitCommandsTextEdit->toPlainText();
    result.serverResetCommands = d->serverResetCommandsTextEdit->toPlainText();
    result.kitId = d->kitChooser->currentKitId();
    result.debugInfoLocation = d->debuginfoPathChooser->filePath().toString();
    result.runnable.commandLineArguments = d->arguments->text();
    result.runnable.workingDirectory = d->workingDirectory->filePath().toString();
    result.breakAtMain = d->breakAtMainCheckBox->isChecked();
    result.runInTerminal = d->runInTerminalCheckBox->isChecked();
    return result;
}

void StartApplicationDialog::setParameters(const StartApplicationParameters &p)
{
    d->kitChooser->setCurrentKitId(p.kitId);
    d->serverPortSpinBox->setValue(p.serverPort);
    d->channelOverrideEdit->setText(p.serverAddress);
    d->localExecutablePathChooser->setFilePath(p.runnable.executable);
    d->serverStartScriptPathChooser->setFilePath(p.serverStartScript);
    d->sysRootPathChooser->setFilePath(p.sysRoot);
    d->serverInitCommandsTextEdit->setPlainText(p.serverInitCommands);
    d->serverResetCommandsTextEdit->setPlainText(p.serverResetCommands);
    d->debuginfoPathChooser->setPath(p.debugInfoLocation);
    d->arguments->setText(p.runnable.commandLineArguments);
    d->workingDirectory->setPath(p.runnable.workingDirectory);
    d->runInTerminalCheckBox->setChecked(p.runInTerminal);
    d->breakAtMainCheckBox->setChecked(p.breakAtMain);
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
    setWindowTitle(tr("Start Debugger"));

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
    formLayout->addRow(tr("Kit:"), d->kitChooser);
    formLayout->addRow(tr("&Port:"), d->portSpinBox);

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
    return  StartRemoteCdbDialog::tr(
                "<html><body><p>The remote CDB needs to load the matching %1 CDB extension "
                "(<code>%2</code> or <code>%3</code>, respectively).</p><p>Copy it onto the remote machine and set the "
                "environment variable <code>%4</code> to point to its folder.</p><p>"
                "Launch the remote CDB as <code>%5 &lt;executable&gt;</code> "
                "to use TCP/IP as communication protocol.</p><p>Enter the connection parameters as:</p>"
                "<pre>%6</pre></body></html>")
            .arg(QString(Core::Constants::IDE_DISPLAY_NAME),
                 ext32, ext64, QString("_NT_DEBUGGER_EXTENSION_PATH"),
                 QString("cdb.exe -server tcp:port=1234"),
                 QString(cdbConnectionSyntax));
}

StartRemoteCdbDialog::StartRemoteCdbDialog(QWidget *parent) :
    QDialog(parent), m_lineEdit(new QLineEdit)
{
    setWindowTitle(tr("Start a CDB Remote Session"));

    auto groupBox = new QGroupBox;

    auto helpLabel = new QLabel(cdbRemoteHelp());
    helpLabel->setWordWrap(true);
    helpLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

    auto label = new QLabel(tr("&Connection:"));
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

    connect(m_lineEdit, &QLineEdit::textChanged,
            this, &StartRemoteCdbDialog::textChanged);
    connect(m_lineEdit, &QLineEdit::returnPressed,
            [this] { m_okButton->animateClick(); });
    connect(box, &QDialogButtonBox::accepted,
            this, &StartRemoteCdbDialog::accept);
    connect(box, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
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
    setWindowTitle(tr("Select Start Address"));

    auto hLayout = new QHBoxLayout;
    hLayout->addWidget(new QLabel(tr("Enter an address:") + ' '));
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
    setWindowTitle(tr("Start Remote Engine"));

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
    formLayout->addRow(tr("&Host:"), d->host);
    formLayout->addRow(tr("&Username:"), d->username);
    formLayout->addRow(tr("&Password:"), d->password);
    formLayout->addRow(tr("&Engine path:"), d->enginePath);
    formLayout->addRow(tr("&Inferior path:"), d->inferiorPath);

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
            choice->setText(i == -1 ? TypeFormatsDialog::tr("Reset")
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
    setWindowTitle(tr("Type Formats"));
    m_ui->addPage(tr("Qt Types"));
    m_ui->addPage(tr("Standard Types"));
    m_ui->addPage(tr("Misc Types"));

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

} // namespace Internal
} // namespace Debugger
