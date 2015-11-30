/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "debuggerdialogs.h"

#include "debuggerkitinformation.h"
#include "debuggerstartparameters.h"
#include "debuggerruncontrol.h"
#include "debuggerstringutils.h"
#include "cdb/cdbengine.h"

#include <coreplugin/icore.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/pathchooser.h>
#include <utils/fancylineedit.h>
#include <utils/qtcassert.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QRegExp>
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
    QLabel *serverAddressLabel;
    QLineEdit *serverAddressEdit;
    QSpinBox *serverPortSpinBox;
    PathChooser *localExecutablePathChooser;
    FancyLineEdit *arguments;
    PathChooser *workingDirectory;
    QCheckBox *breakAtMainCheckBox;
    QCheckBox *runInTerminalCheckBox;
    PathChooser *debuginfoPathChooser;
    QLabel *serverStartScriptLabel;
    PathChooser *serverStartScriptPathChooser;
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
// DebuggerKitChooser
//
///////////////////////////////////////////////////////////////////////

DebuggerKitChooser::DebuggerKitChooser(Mode mode, QWidget *parent)
    : KitChooser(parent)
    , m_hostAbi(Abi::hostAbi())
    , m_mode(mode)
{
    setKitMatcher([this](const Kit *k) {
        // Match valid debuggers and restrict local debugging to compatible toolchains.
        if (!DebuggerKitInformation::isValidDebugger(k))
            return false;
        if (m_mode == LocalDebugging) {
            const ToolChain *tc = ToolChainKitInformation::toolChain(k);
            return tc && tc->targetAbi().os() == m_hostAbi.os();
        }
        return true;
    });
}

QString DebuggerKitChooser::kitToolTip(Kit *k) const
{
    return DebuggerKitInformation::displayString(k);
}

///////////////////////////////////////////////////////////////////////
//
// StartApplicationParameters
//
///////////////////////////////////////////////////////////////////////

class StartApplicationParameters
{
public:
    StartApplicationParameters();
    QString displayName() const;
    bool equals(const StartApplicationParameters &rhs) const;
    void toSettings(QSettings *) const;
    void fromSettings(const QSettings *settings);

    bool operator==(const StartApplicationParameters &p) const { return equals(p); }
    bool operator!=(const StartApplicationParameters &p) const { return !equals(p); }

    Id kitId;
    uint serverPort;
    QString serverAddress;
    QString localExecutable;
    QString processArgs;
    QString workingDirectory;
    bool breakAtMain;
    bool runInTerminal;
    QString serverStartScript;
    QString debugInfoLocation;
};

StartApplicationParameters::StartApplicationParameters() :
    breakAtMain(false), runInTerminal(false)
{
}

bool StartApplicationParameters::equals(const StartApplicationParameters &rhs) const
{
    return localExecutable == rhs.localExecutable
        && serverPort == rhs.serverPort
        && processArgs == rhs.processArgs
        && workingDirectory == rhs.workingDirectory
        && breakAtMain == rhs.breakAtMain
        && runInTerminal == rhs.runInTerminal
        && serverStartScript == rhs.serverStartScript
        && kitId == rhs.kitId
        && debugInfoLocation == rhs.debugInfoLocation
        && serverAddress == rhs.serverAddress;
}

QString StartApplicationParameters::displayName() const
{
    const int maxLength = 60;

    QString name = FileName::fromString(localExecutable).fileName() + QLatin1Char(' ') + processArgs;
    if (name.size() > 60) {
        int index = name.lastIndexOf(QLatin1Char(' '), maxLength);
        if (index == -1)
            index = maxLength;
        name.truncate(index);
        name += QLatin1String("...");
    }

    if (Kit *kit = KitManager::find(kitId))
        name += QString::fromLatin1(" (%1)").arg(kit->displayName());

    return name;
}

void StartApplicationParameters::toSettings(QSettings *settings) const
{
    settings->setValue(_("LastKitId"), kitId.toSetting());
    settings->setValue(_("LastServerPort"), serverPort);
    settings->setValue(_("LastServerAddress"), serverAddress);
    settings->setValue(_("LastExternalExecutable"), localExecutable);
    settings->setValue(_("LastExternalExecutableArguments"), processArgs);
    settings->setValue(_("LastExternalWorkingDirectory"), workingDirectory);
    settings->setValue(_("LastExternalBreakAtMain"), breakAtMain);
    settings->setValue(_("LastExternalRunInTerminal"), runInTerminal);
    settings->setValue(_("LastServerStartScript"), serverStartScript);
    settings->setValue(_("LastDebugInfoLocation"), debugInfoLocation);
}

void StartApplicationParameters::fromSettings(const QSettings *settings)
{
    kitId = Id::fromSetting(settings->value(_("LastKitId")));
    serverPort = settings->value(_("LastServerPort")).toUInt();
    serverAddress = settings->value(_("LastServerAddress")).toString();
    localExecutable = settings->value(_("LastExternalExecutable")).toString();
    processArgs = settings->value(_("LastExternalExecutableArguments")).toString();
    workingDirectory = settings->value(_("LastExternalWorkingDirectory")).toString();
    breakAtMain = settings->value(_("LastExternalBreakAtMain")).toBool();
    runInTerminal = settings->value(_("LastExternalRunInTerminal")).toBool();
    serverStartScript = settings->value(_("LastServerStartScript")).toString();
    debugInfoLocation = settings->value(_("LastDebugInfoLocation")).toString();
}

///////////////////////////////////////////////////////////////////////
//
// StartApplicationDialog
//
///////////////////////////////////////////////////////////////////////

StartApplicationDialog::StartApplicationDialog(QWidget *parent)
  : QDialog(parent), d(new StartApplicationDialogPrivate)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("Start Debugger"));

    d->kitChooser = new KitChooser(this);
    d->kitChooser->populate();

    d->serverPortLabel = new QLabel(tr("Server port:"), this);
    d->serverPortSpinBox = new QSpinBox(this);
    d->serverPortSpinBox->setRange(1, 65535);

    d->serverAddressLabel = new QLabel(tr("Override server address"), this);
    d->serverAddressEdit = new QLineEdit(this);

    d->localExecutablePathChooser = new PathChooser(this);
    d->localExecutablePathChooser->setExpectedKind(PathChooser::File);
    d->localExecutablePathChooser->setPromptDialogTitle(tr("Select Executable"));
    d->localExecutablePathChooser->setHistoryCompleter(QLatin1String("LocalExecutable"));

    d->arguments = new FancyLineEdit(this);
    d->arguments->setHistoryCompleter(QLatin1String("CommandlineArguments"));

    d->workingDirectory = new PathChooser(this);
    d->workingDirectory->setExpectedKind(PathChooser::ExistingDirectory);
    d->workingDirectory->setPromptDialogTitle(tr("Select Working Directory"));
    d->workingDirectory->setHistoryCompleter(QLatin1String("WorkingDirectory"));

    d->runInTerminalCheckBox = new QCheckBox(this);

    d->breakAtMainCheckBox = new QCheckBox(this);
    d->breakAtMainCheckBox->setText(QString());

    d->serverStartScriptPathChooser = new PathChooser(this);
    d->serverStartScriptPathChooser->setExpectedKind(PathChooser::File);
    d->serverStartScriptPathChooser->setPromptDialogTitle(tr("Select Server Start Script"));
    d->serverStartScriptPathChooser->setToolTip(tr(
        "This option can be used to point to a script that will be used "
        "to start a debug server. If the field is empty, Qt Creator's "
        "default methods to set up debug servers will be used."));
    d->serverStartScriptLabel = new QLabel(tr("&Server start script:"), this);
    d->serverStartScriptLabel->setBuddy(d->serverStartScriptPathChooser);
    d->serverStartScriptLabel->setToolTip(d->serverStartScriptPathChooser->toolTip());

    d->debuginfoPathChooser = new PathChooser(this);
    d->debuginfoPathChooser->setPromptDialogTitle(tr("Select Location of Debugging Information"));
    d->debuginfoPathChooser->setToolTip(tr(
        "Base path for external debug information and debug sources. "
        "If empty, $SYSROOT/usr/lib/debug will be chosen."));
    d->debuginfoPathChooser->setHistoryCompleter(QLatin1String("Debugger.DebugLocation.History"));

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
    formLayout->addRow(d->serverAddressLabel, d->serverAddressEdit);
    formLayout->addRow(tr("Local &executable:"), d->localExecutablePathChooser);
    formLayout->addRow(tr("Command line &arguments:"), d->arguments);
    formLayout->addRow(tr("&Working directory:"), d->workingDirectory);
    formLayout->addRow(tr("Run in &terminal:"), d->runInTerminalCheckBox);
    formLayout->addRow(tr("Break at \"&main\":"), d->breakAtMainCheckBox);
    formLayout->addRow(d->serverStartScriptLabel, d->serverStartScriptPathChooser);
    formLayout->addRow(tr("Debug &information:"), d->debuginfoPathChooser);
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
    connect(d->historyComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &StartApplicationDialog::historyIndexChanged);

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
        if (!p.localExecutable.isEmpty())
            d->historyComboBox->addItem(p.displayName(), QVariant::fromValue(p));
    }
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

bool StartApplicationDialog::run(QWidget *parent, DebuggerRunParameters *rp, Kit **kit)
{
    const bool attachRemote = rp->startMode == AttachToRemoteServer;
    const QString settingsGroup = QLatin1String("DebugMode");
    const QString arrayName = QLatin1String("StartApplication");

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

    StartApplicationDialog dialog(parent);
    dialog.setHistory(history);
    dialog.setParameters(history.back());
    if (!attachRemote) {
        dialog.d->serverStartScriptPathChooser->setVisible(false);
        dialog.d->serverStartScriptLabel->setVisible(false);
        dialog.d->serverPortSpinBox->setVisible(false);
        dialog.d->serverPortLabel->setVisible(false);
        dialog.d->serverAddressLabel->setVisible(false);
        dialog.d->serverAddressEdit->setVisible(false);
    }
    if (dialog.exec() != QDialog::Accepted)
        return false;

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

    rp->executable = newParameters.localExecutable;
    const QString inputAddress = dialog.d->serverAddressEdit->text();
    if (!inputAddress.isEmpty())
        rp->remoteChannel = inputAddress;
    else
        rp->remoteChannel = rp->connParams.host;
    rp->remoteChannel += QLatin1Char(':') + QString::number(newParameters.serverPort);
    rp->displayName = newParameters.displayName();
    rp->workingDirectory = newParameters.workingDirectory;
    rp->useTerminal = newParameters.runInTerminal;
    if (!newParameters.processArgs.isEmpty())
        rp->processArgs = newParameters.processArgs;
    rp->breakOnMain = newParameters.breakAtMain;
    rp->serverStartScript = newParameters.serverStartScript;
    rp->debugInfoLocation = newParameters.debugInfoLocation;

    Kit *k = dialog.d->kitChooser->currentKit();
    IDevice::ConstPtr dev = DeviceKitInformation::device(k);
    bool isLocal = !dev || (dev->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
    if (!attachRemote)
        rp->startMode = isLocal ? StartExternal : StartRemoteProcess;
    if (kit)
        *kit = k;
    return true;
}

StartApplicationParameters StartApplicationDialog::parameters() const
{
    StartApplicationParameters result;
    result.serverPort = d->serverPortSpinBox->value();
    result.serverAddress = d->serverAddressEdit->text();
    result.localExecutable = d->localExecutablePathChooser->path();
    result.serverStartScript = d->serverStartScriptPathChooser->path();
    result.kitId = d->kitChooser->currentKitId();
    result.debugInfoLocation = d->debuginfoPathChooser->path();
    result.processArgs = d->arguments->text();
    result.workingDirectory = d->workingDirectory->path();
    result.breakAtMain = d->breakAtMainCheckBox->isChecked();
    result.runInTerminal = d->runInTerminalCheckBox->isChecked();
    return result;
}

void StartApplicationDialog::setParameters(const StartApplicationParameters &p)
{
    d->kitChooser->setCurrentKitId(p.kitId);
    d->serverPortSpinBox->setValue(p.serverPort);
    d->serverAddressEdit->setText(p.serverAddress);
    d->localExecutablePathChooser->setPath(p.localExecutable);
    d->serverStartScriptPathChooser->setPath(p.serverStartScript);
    d->debuginfoPathChooser->setPath(p.debugInfoLocation);
    d->arguments->setText(p.processArgs);
    d->workingDirectory->setPath(p.workingDirectory);
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
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("Start Debugger"));

    d->kitChooser = new DebuggerKitChooser(DebuggerKitChooser::AnyDebugging, this);
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
    const char *cdbConnectionSyntax =
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
                "<html><body><p>The remote CDB needs to load the matching Qt Creator CDB extension "
                "(<code>%1</code> or <code>%2</code>, respectively).</p><p>Copy it onto the remote machine and set the "
                "environment variable <code>%3</code> to point to its folder.</p><p>"
                "Launch the remote CDB as <code>%4 &lt;executable&gt;</code> "
                "to use TCP/IP as communication protocol.</p><p>Enter the connection parameters as:</p>"
                "<pre>%5</pre></body></html>").
            arg(ext32, ext64, QLatin1String("_NT_DEBUGGER_EXTENSION_PATH"),
                QLatin1String("cdb.exe -server tcp:port=1234"),
                QLatin1String(cdbConnectionSyntax));
}

StartRemoteCdbDialog::StartRemoteCdbDialog(QWidget *parent) :
    QDialog(parent), m_okButton(0), m_lineEdit(new QLineEdit)
{
    setWindowTitle(tr("Start a CDB Remote Session"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

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

StartRemoteCdbDialog::~StartRemoteCdbDialog()
{
}

void StartRemoteCdbDialog::textChanged(const QString &t)
{
    m_okButton->setEnabled(!t.isEmpty());
}

QString StartRemoteCdbDialog::connection() const
{
    const QString rc = m_lineEdit->text();
    // Transform an IP:POrt ('localhost:1234') specification into full spec
    QRegExp ipRegexp(QLatin1String("([\\w\\.\\-_]+):([0-9]{1,4})"));
    QTC_ASSERT(ipRegexp.isValid(), return QString());
    if (ipRegexp.exactMatch(rc))
        return QString::fromLatin1("tcp:server=%1,port=%2").arg(ipRegexp.cap(1), ipRegexp.cap(2));
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
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto hLayout = new QHBoxLayout;
    hLayout->addWidget(new QLabel(tr("Enter an address:") + QLatin1Char(' ')));
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
    m_lineEdit->setText(QLatin1String("0x") + QString::number(a, 16));
}

quint64 AddressDialog::address() const
{
    return m_lineEdit->text().toULongLong(0, 16);
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
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("Start Remote Engine"));

    d->host = new FancyLineEdit(this);
    d->host->setHistoryCompleter(QLatin1String("HostName"));

    d->username = new FancyLineEdit(this);
    d->username->setHistoryCompleter(QLatin1String("UserName"));

    d->password = new QLineEdit(this);
    d->password->setEchoMode(QLineEdit::Password);

    d->enginePath = new FancyLineEdit(this);
    d->enginePath->setHistoryCompleter(QLatin1String("EnginePath"));

    d->inferiorPath = new FancyLineEdit(this);
    d->inferiorPath->setHistoryCompleter(QLatin1String("InferiorPath"));

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
        QVBoxLayout *vboxLayout = new QVBoxLayout;
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
        QButtonGroup *group = new QButtonGroup(this);
        m_layout->addWidget(new QLabel(type), row, column++);
        for (int i = -1; i != typeFormats.size(); ++i) {
            QRadioButton *choice = new QRadioButton(this);
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

        QVBoxLayout *layout = new QVBoxLayout(q);
        layout->addWidget(tabs);
        layout->addWidget(buttonBox);
        q->setLayout(layout);
    }

    void addPage(const QString &name)
    {
        TypeFormatsDialogPage *page = new TypeFormatsDialogPage;
        pages.append(page);
        QScrollArea *scroller = new QScrollArea;
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
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->addPage(tr("Qt Types"));
    m_ui->addPage(tr("Standard Types"));
    m_ui->addPage(tr("Misc Types"));

    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(reject()));
}

TypeFormatsDialog::~TypeFormatsDialog()
{
    delete m_ui;
}

void TypeFormatsDialog::addTypeFormats(const QString &type0,
    const DisplayFormats &typeFormats, int current)
{
    QString type = type0;
    type.replace(QLatin1String("__"), QLatin1String("::"));
    int pos = 2;
    if (type.startsWith(QLatin1Char('Q')))
        pos = 0;
    else if (type.startsWith(QLatin1String("std::")))
        pos = 1;
    m_ui->pages[pos]->addTypeFormats(type, typeFormats, current);
}

} // namespace Internal
} // namespace Debugger
