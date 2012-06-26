/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "debuggerdialogs.h"
#include "debuggerstartparameters.h"

#include "debuggerconstants.h"
#include "debuggerprofileinformation.h"
#include "debuggerstringutils.h"
#include "cdb/cdbengine.h"
#include "shared/hostutils.h"

#include "ui_attachcoredialog.h"
#include "ui_attachexternaldialog.h"
#include "ui_startexternaldialog.h"
#include "ui_startremotedialog.h"
#include "ui_startremoteenginedialog.h"
#include "ui_attachtoqmlportdialog.h"

#include <coreplugin/icore.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/profileinformation.h>
#include <utils/historycompleter.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QRegExp>

#include <QButtonGroup>
#include <QFileDialog>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QGridLayout>

using namespace ProjectExplorer;
using namespace Utils;

namespace Debugger {
namespace Internal {

bool operator<(const ProcData &p1, const ProcData &p2)
{
    return p1.name < p2.name;
}

// A filterable process list model
class ProcessListFilterModel : public QSortFilterProxyModel
{
public:
    explicit ProcessListFilterModel(QObject *parent);
    QString processIdAt(const QModelIndex &index) const;
    QString executableForPid(const QString& pid) const;

    void populate(QList<ProcData> processes, const QString &excludePid);

private:
    enum { ProcessImageRole = Qt::UserRole, ProcessNameRole };

    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

    QStandardItemModel *m_model;
};

ProcessListFilterModel::ProcessListFilterModel(QObject *parent)
  : QSortFilterProxyModel(parent),
    m_model(new QStandardItemModel(this))
{
    QStringList columns;
    columns << AttachExternalDialog::tr("Process ID")
            << AttachExternalDialog::tr("Name")
            << AttachExternalDialog::tr("State");
    m_model->setHorizontalHeaderLabels(columns);
    setSourceModel(m_model);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setFilterKeyColumn(1);
}

bool ProcessListFilterModel::lessThan(const QModelIndex &left,
    const QModelIndex &right) const
{
    const QString l = sourceModel()->data(left).toString();
    const QString r = sourceModel()->data(right).toString();
    if (left.column() == 0)
        return l.toInt() < r.toInt();
    return l < r;
}

QString ProcessListFilterModel::processIdAt(const QModelIndex &index) const
{
    if (index.isValid()) {
        const QModelIndex index0 = mapToSource(index);
        QModelIndex siblingIndex = index0.sibling(index0.row(), 0);
        if (const QStandardItem *item = m_model->itemFromIndex(siblingIndex))
            return item->text();
    }
    return QString();
}

QString ProcessListFilterModel::executableForPid(const QString &pid) const
{
    const int rowCount = m_model->rowCount();
    for (int r = 0; r < rowCount; r++) {
        const QStandardItem *item = m_model->item(r, 0);
        if (item->text() == pid) {
            QString name = item->data(ProcessImageRole).toString();
            if (name.isEmpty())
                name = item->data(ProcessNameRole).toString();
            return name;
        }
    }
    return QString();
}

void ProcessListFilterModel::populate
    (QList<ProcData> processes, const QString &excludePid)
{
    qStableSort(processes);

    if (const int rowCount = m_model->rowCount())
        m_model->removeRows(0, rowCount);

    QStandardItem *root  = m_model->invisibleRootItem();
    foreach (const ProcData &proc, processes) {
        QList<QStandardItem *> row;
        row.append(new QStandardItem(proc.ppid));
        QString name = proc.image.isEmpty() ? proc.name : proc.image;
        row.back()->setData(name, ProcessImageRole);
        row.append(new QStandardItem(proc.name));
        row.back()->setToolTip(proc.image);
        row.append(new QStandardItem(proc.state));

        if (proc.ppid == excludePid)
            foreach (QStandardItem *item, row)
                item->setEnabled(false);
        root->appendRow(row);
    }
}


///////////////////////////////////////////////////////////////////////
//
// AttachCoreDialog
//
///////////////////////////////////////////////////////////////////////

AttachCoreDialog::AttachCoreDialog(QWidget *parent)
  : QDialog(parent), m_ui(new Ui::AttachCoreDialog)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->setupUi(this);
    m_ui->toolchainComboBox->init(false);

    m_ui->execFileName->setExpectedKind(PathChooser::File);
    m_ui->execFileName->setPromptDialogTitle(tr("Select Executable"));

    m_ui->coreFileName->setExpectedKind(PathChooser::File);
    m_ui->coreFileName->setPromptDialogTitle(tr("Select Core File"));

    m_ui->overrideStartScriptFileName->setExpectedKind(PathChooser::File);
    m_ui->overrideStartScriptFileName->setPromptDialogTitle(tr("Select Startup Script"));

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(m_ui->coreFileName, SIGNAL(changed(QString)), this, SLOT(changed()));
    changed();
}

AttachCoreDialog::~AttachCoreDialog()
{
    delete m_ui;
}

QString AttachCoreDialog::executableFile() const
{
    return m_ui->execFileName->path();
}

void AttachCoreDialog::setExecutableFile(const QString &fileName)
{
    m_ui->execFileName->setPath(fileName);
    changed();
}

QString AttachCoreDialog::coreFile() const
{
    return m_ui->coreFileName->path();
}

void AttachCoreDialog::setCoreFile(const QString &fileName)
{
    m_ui->coreFileName->setPath(fileName);
    changed();
}

Profile *AttachCoreDialog::profile() const
{
    return m_ui->toolchainComboBox->currentProfile();
}

void AttachCoreDialog::setProfileIndex(int i)
{
    if (i >= 0 && i < m_ui->toolchainComboBox->count())
        m_ui->toolchainComboBox->setCurrentIndex(i);
}

int AttachCoreDialog::profileIndex() const
{
    return m_ui->toolchainComboBox->currentIndex();
}

QString AttachCoreDialog::overrideStartScript() const
{
    return m_ui->overrideStartScriptFileName->path();
}

void AttachCoreDialog::setOverrideStartScript(const QString &scriptName)
{
    m_ui->overrideStartScriptFileName->setPath(scriptName);
}

bool AttachCoreDialog::isValid() const
{
    return m_ui->toolchainComboBox->currentIndex() >= 0 &&
           !coreFile().isEmpty();
}

void AttachCoreDialog::changed()
{
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isValid());
}

///////////////////////////////////////////////////////////////////////
//
// AttachExternalDialog
//
///////////////////////////////////////////////////////////////////////

AttachExternalDialog::AttachExternalDialog(QWidget *parent)
  : QDialog(parent),
    m_selfPid(QString::number(QCoreApplication::applicationPid())),
    m_ui(new Ui::AttachExternalDialog),
    m_model(new ProcessListFilterModel(this))
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->setupUi(this);
    m_ui->toolchainComboBox->init(true);
    okButton()->setDefault(true);
    okButton()->setEnabled(false);

    m_ui->procView->setModel(m_model);
    m_ui->procView->setSortingEnabled(true);
    m_ui->procView->sortByColumn(1, Qt::AscendingOrder);

    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    QPushButton *refreshButton = new QPushButton(tr("Refresh"));
    connect(refreshButton, SIGNAL(clicked()), this, SLOT(rebuildProcessList()));
    m_ui->buttonBox->addButton(refreshButton, QDialogButtonBox::ActionRole);
    m_ui->filterWidget->setFocus(Qt::TabFocusReason);

    m_ui->procView->setAlternatingRowColors(true);
    m_ui->procView->setRootIsDecorated(false);
    m_ui->procView->setUniformRowHeights(true);

    // Do not use activated, will be single click in Oxygen
    connect(m_ui->procView, SIGNAL(doubleClicked(QModelIndex)),
        this, SLOT(procSelected(QModelIndex)));
    connect(m_ui->procView, SIGNAL(clicked(QModelIndex)),
        this, SLOT(procClicked(QModelIndex)));
    connect(m_ui->pidLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(pidChanged(QString)));
    connect(m_ui->filterWidget, SIGNAL(filterChanged(QString)),
            this, SLOT(setFilterString(QString)));


    setMinimumHeight(500);
    rebuildProcessList();
}

AttachExternalDialog::~AttachExternalDialog()
{
    delete m_ui;
}

void AttachExternalDialog::setFilterString(const QString &filter)
{
    m_model->setFilterFixedString(filter);
    // Activate the line edit if there's a unique filtered process.
    QString processId;
    if (m_model->rowCount(QModelIndex()) == 1)
        processId = m_model->processIdAt(m_model->index(0, 0, QModelIndex()));
    m_ui->pidLineEdit->setText(processId);
    pidChanged(processId);
}

QPushButton *AttachExternalDialog::okButton() const
{
    return m_ui->buttonBox->button(QDialogButtonBox::Ok);
}

void AttachExternalDialog::rebuildProcessList()
{
    m_model->populate(hostProcessList(), m_selfPid);
    m_ui->procView->expandAll();
    m_ui->procView->resizeColumnToContents(0);
    m_ui->procView->resizeColumnToContents(1);
}

void AttachExternalDialog::procSelected(const QModelIndex &proxyIndex)
{
    const QString processId = m_model->processIdAt(proxyIndex);
    if (!processId.isEmpty()) {
        m_ui->pidLineEdit->setText(processId);
        if (okButton()->isEnabled())
            okButton()->animateClick();
    }
}

void AttachExternalDialog::procClicked(const QModelIndex &proxyIndex)
{
    const QString processId = m_model->processIdAt(proxyIndex);
    if (!processId.isEmpty())
        m_ui->pidLineEdit->setText(processId);
}

QString AttachExternalDialog::attachPIDText() const
{
    return m_ui->pidLineEdit->text().trimmed();
}

qint64 AttachExternalDialog::attachPID() const
{
    return attachPIDText().toLongLong();
}

QString AttachExternalDialog::executable() const
{
    // Search pid in model in case the user typed in the PID.
    return m_model->executableForPid(attachPIDText());
}

Profile *AttachExternalDialog::profile() const
{
    return m_ui->toolchainComboBox->currentProfile();
}

void AttachExternalDialog::setProfileIndex(int i)
{
    if (i >= 0 && i < m_ui->toolchainComboBox->count())
        m_ui->toolchainComboBox->setCurrentIndex(i);
}

int AttachExternalDialog::profileIndex() const
{
    return m_ui->toolchainComboBox->currentIndex();
}

void AttachExternalDialog::pidChanged(const QString &pid)
{
    const bool enabled = !pid.isEmpty() && pid != QLatin1String("0") && pid != m_selfPid
            && m_ui->toolchainComboBox->currentIndex() >= 0;
    okButton()->setEnabled(enabled);
}

void AttachExternalDialog::accept()
{
#ifdef Q_OS_WIN
    const qint64 pid = attachPID();
    if (pid && isWinProcessBeingDebugged(pid)) {
        QMessageBox::warning(this, tr("Process Already Under Debugger Control"),
                             tr("The process %1 is already under the control of a debugger.\n"
                                "Qt Creator cannot attach to it.").arg(pid));
        return;
    }
#endif
    QDialog::accept();
}


///////////////////////////////////////////////////////////////////////
//
// StartExternalDialog
//
///////////////////////////////////////////////////////////////////////

class StartExternalParameters
{
public:
    StartExternalParameters();
    bool equals(const StartExternalParameters &rhs) const;
    bool isValid() const { return !executableFile.isEmpty(); }
    QString displayName() const;
    void toSettings(QSettings *) const;
    void fromSettings(const QSettings *settings);

    QString executableFile;
    QString arguments;
    QString workingDirectory;
    int abiIndex;
    bool breakAtMain;
    bool runInTerminal;
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::StartExternalParameters)

namespace Debugger {
namespace Internal {

inline bool operator==(const StartExternalParameters &p1, const StartExternalParameters &p2)
{ return p1.equals(p2); }

inline bool operator!=(const StartExternalParameters &p1, const StartExternalParameters &p2)
{ return !p1.equals(p2); }

StartExternalParameters::StartExternalParameters() :
    abiIndex(0), breakAtMain(false), runInTerminal(false)
{
}

bool StartExternalParameters::equals(const StartExternalParameters &rhs) const
{
    return executableFile == rhs.executableFile && arguments == rhs.arguments
        && workingDirectory == rhs.workingDirectory && abiIndex == rhs.abiIndex
        && breakAtMain == rhs.breakAtMain && runInTerminal == rhs.runInTerminal;
}

QString StartExternalParameters::displayName() const
{
    enum { maxLength = 60 };

    QString name = QFileInfo(executableFile).fileName()
                   + QLatin1Char(' ') + arguments;
    if (name.size() > maxLength) {
        int index = name.lastIndexOf(QLatin1Char(' '), maxLength);
        if (index == -1)
            index = maxLength;
        name.truncate(index);
        name += QLatin1String("...");
    }
    return name;
}

void StartExternalParameters::toSettings(QSettings *settings) const
{
    settings->setValue(_("LastExternalExecutableFile"), executableFile);
    settings->setValue(_("LastExternalExecutableArguments"), arguments);
    settings->setValue(_("LastExternalWorkingDirectory"), workingDirectory);
    settings->setValue(_("LastExternalAbiIndex"), abiIndex);
    settings->setValue(_("LastExternalBreakAtMain"), breakAtMain);
    settings->setValue(_("LastExternalRunInTerminal"), runInTerminal);
}

void StartExternalParameters::fromSettings(const QSettings *settings)
{
    executableFile = settings->value(_("LastExternalExecutableFile")).toString();
    arguments = settings->value(_("LastExternalExecutableArguments")).toString();
    workingDirectory = settings->value(_("LastExternalWorkingDirectory")).toString();
    abiIndex = settings->value(_("LastExternalAbiIndex")).toInt();
    breakAtMain = settings->value(_("LastExternalBreakAtMain")).toBool();
    runInTerminal = settings->value(_("LastExternalRunInTerminal")).toBool();
}

StartExternalDialog::StartExternalDialog(QWidget *parent)
  : QDialog(parent), m_ui(new Ui::StartExternalDialog)
{
    QSettings *settings = Core::ICore::settings();
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->setupUi(this);
    m_ui->toolChainComboBox->init(true);
    m_ui->execFile->setExpectedKind(PathChooser::File);
    m_ui->execFile->setPromptDialogTitle(tr("Select Executable"));
    m_ui->execFile->lineEdit()->setCompleter(
        new HistoryCompleter(settings, m_ui->execFile->lineEdit()));
    connect(m_ui->execFile, SIGNAL(changed(QString)), this, SLOT(changed()));
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    m_ui->workingDirectory->setExpectedKind(PathChooser::ExistingDirectory);
    m_ui->workingDirectory->setPromptDialogTitle(tr("Select Working Directory"));
    m_ui->workingDirectory->lineEdit()->setCompleter(
        new HistoryCompleter(settings, m_ui->workingDirectory->lineEdit()));

    m_ui->argsEdit->setCompleter(new HistoryCompleter(settings, m_ui->argsEdit));

    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(m_ui->historyComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(historyIndexChanged(int)));
    changed();
}

StartExternalDialog::~StartExternalDialog()
{
    delete m_ui;
}

StartExternalParameters StartExternalDialog::parameters() const
{
    StartExternalParameters result;
    result.executableFile = m_ui->execFile->path();
    result.arguments = m_ui->argsEdit->text();
    result.workingDirectory = m_ui->workingDirectory->path();
    result.abiIndex = m_ui->toolChainComboBox->currentIndex();
    result.breakAtMain = m_ui->checkBoxBreakAtMain->isChecked();
    result.runInTerminal = m_ui->checkBoxRunInTerminal->isChecked();
    return result;
}

void StartExternalDialog::setParameters(const StartExternalParameters &p)
{
    setExecutableFile(p.executableFile);
    m_ui->argsEdit->setText(p.arguments);
    m_ui->workingDirectory->setPath(p.workingDirectory);
    if (p.abiIndex >= 0 && p.abiIndex < m_ui->toolChainComboBox->count())
        m_ui->toolChainComboBox->setCurrentIndex(p.abiIndex);
    m_ui->checkBoxRunInTerminal->setChecked(p.runInTerminal);
    m_ui->checkBoxBreakAtMain->setChecked(p.breakAtMain);
}

void StartExternalDialog::setHistory(const QList<StartExternalParameters> l)
{
    m_ui->historyComboBox->clear();
    for (int i = l.size() -  1; i >= 0; --i)
        if (l.at(i).isValid())
            m_ui->historyComboBox->addItem(l.at(i).displayName(),
                                           QVariant::fromValue(l.at(i)));
}

void StartExternalDialog::historyIndexChanged(int index)
{
    if (index < 0)
        return;
    const QVariant v = m_ui->historyComboBox->itemData(index);
    QTC_ASSERT(v.canConvert<StartExternalParameters>(), return);
    setParameters(v.value<StartExternalParameters>());
}

void StartExternalDialog::setExecutableFile(const QString &str)
{
    m_ui->execFile->setPath(str);
    changed();
}

QString StartExternalDialog::executableFile() const
{
    return m_ui->execFile->path();
}

Profile *StartExternalDialog::profile() const
{
    return m_ui->toolChainComboBox->currentProfile();
}

bool StartExternalDialog::isValid() const
{
    return m_ui->toolChainComboBox->currentIndex() >= 0
           && !executableFile().isEmpty();
}

void StartExternalDialog::changed()
{
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isValid());
}

// Read parameter history (start external, remote)
// from settings array. Always return at least one element.
template <class Parameter>
QList<Parameter> readParameterHistory(QSettings *settings,
                                      const QString &settingsGroup,
                                      const QString &arrayName)
{
    QList<Parameter> result;
    settings->beginGroup(settingsGroup);
    const int arraySize = settings->beginReadArray(arrayName);
    for (int i = 0; i < arraySize; ++i) {
        settings->setArrayIndex(i);
        Parameter p;
        p.fromSettings(settings);
        result.push_back(p);
    }
    settings->endArray();
    if (result.isEmpty()) { // First time: Read old settings.
        Parameter p;
        p.fromSettings(settings);
        result.push_back(p);
    }
    settings->endGroup();
    return result;
}

// Write parameter history (start external, remote) to settings.
template <class Parameter>
void writeParameterHistory(const QList<Parameter> &history,
                           QSettings *settings,
                           const QString &settingsGroup,
                           const QString &arrayName)
{
    settings->beginGroup(settingsGroup);
    settings->beginWriteArray(arrayName);
    for (int i = 0; i < history.size(); ++i) {
        settings->setArrayIndex(i);
        history.at(i).toSettings(settings);
    }
    settings->endArray();
    settings->endGroup();
}

bool StartExternalDialog::run(QWidget *parent,
                              QSettings *settings,
                              DebuggerStartParameters *sp)
{
    const QString settingsGroup = _("DebugMode");
    const QString arrayName = _("StartExternal");
    QList<StartExternalParameters> history =
        readParameterHistory<StartExternalParameters>(settings, settingsGroup, arrayName);
    QTC_ASSERT(!history.isEmpty(), return false);

    StartExternalDialog dialog(parent);
    dialog.setHistory(history);
    dialog.setParameters(history.back());
    if (dialog.exec() != QDialog::Accepted)
        return false;
    const StartExternalParameters newParameters = dialog.parameters();

    if (newParameters != history.back()) {
        history.push_back(newParameters);
        if (history.size() > 10)
            history.pop_front();
        writeParameterHistory(history, settings, settingsGroup, arrayName);
    }

    Profile *profile = dialog.profile();
    QTC_ASSERT(profile, return false);
    ToolChain *tc = ToolChainProfileInformation::toolChain(profile);
    QTC_ASSERT(tc, return false);

    sp->executable = newParameters.executableFile;
    sp->startMode = StartExternal;
    sp->toolChainAbi = tc->targetAbi();
    sp->debuggerCommand = DebuggerProfileInformation::debuggerCommand(profile).toString();
    sp->workingDirectory = newParameters.workingDirectory;
    sp->displayName = sp->executable;
    sp->useTerminal = newParameters.runInTerminal;
    if (!newParameters.arguments.isEmpty())
        sp->processArgs = newParameters.arguments;
    // Fixme: 1 of 3 testing hacks.
    if (sp->processArgs.startsWith(QLatin1String("@tcf@ ")) || sp->processArgs.startsWith(QLatin1String("@sym@ ")))
        // Set up an ARM Symbian Abi
        sp->toolChainAbi = Abi(Abi::ArmArchitecture,
                               Abi::SymbianOS,
                               Abi::SymbianDeviceFlavor,
                               Abi::ElfFormat, false);

    sp->breakOnMain = newParameters.breakAtMain;
    return true;
}

///////////////////////////////////////////////////////////////////////
//
// StartRemoteDialog
//
///////////////////////////////////////////////////////////////////////

class StartRemoteParameters
{
public:
    StartRemoteParameters();
    bool equals(const StartRemoteParameters &rhs) const;
    QString displayName() const { return remoteChannel; }
    bool isValid() const { return !remoteChannel.isEmpty(); }

    void toSettings(QSettings *) const;
    void fromSettings(const QSettings *settings);

    QString localExecutable;
    QString remoteChannel;
    QString remoteArchitecture;
    QString overrideStartScript;
    bool useServerStartScript;
    QString serverStartScript;
    Core::Id profileId;
    QString debugInfoLocation;
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::StartRemoteParameters)

namespace Debugger {
namespace Internal {

inline bool operator==(const StartRemoteParameters &p1, const StartRemoteParameters &p2)
{ return p1.equals(p2); }

inline bool operator!=(const StartRemoteParameters &p1, const StartRemoteParameters &p2)
{ return !p1.equals(p2); }

StartRemoteParameters::StartRemoteParameters() :
    useServerStartScript(false), profileId(0)
{
}

bool StartRemoteParameters::equals(const StartRemoteParameters &rhs) const
{
    return localExecutable == rhs.localExecutable && remoteChannel ==rhs.remoteChannel
            && remoteArchitecture == rhs.remoteArchitecture
            && overrideStartScript == rhs.overrideStartScript
            && useServerStartScript == rhs.useServerStartScript
            && serverStartScript == rhs.serverStartScript
            && profileId == rhs.profileId
            && debugInfoLocation == rhs.debugInfoLocation;
}

void StartRemoteParameters::toSettings(QSettings *settings) const
{
    settings->setValue(_("LastRemoteChannel"), remoteChannel);
    settings->setValue(_("LastLocalExecutable"), localExecutable);
    settings->setValue(_("LastRemoteArchitecture"), remoteArchitecture);
    settings->setValue(_("LastServerStartScript"), serverStartScript);
    settings->setValue(_("LastUseServerStartScript"), useServerStartScript);
    settings->setValue(_("LastRemoteStartScript"), overrideStartScript);
    settings->setValue(_("LastProfileId"), profileId.toString());
    settings->setValue(_("LastDebugInfoLocation"), debugInfoLocation);
}

void StartRemoteParameters::fromSettings(const QSettings *settings)
{
    remoteChannel = settings->value(_("LastRemoteChannel")).toString();
    localExecutable = settings->value(_("LastLocalExecutable")).toString();
    profileId = Core::Id(settings->value(_("LastProfileId")).toString());
    remoteArchitecture = settings->value(_("LastRemoteArchitecture")).toString();
    serverStartScript = settings->value(_("LastServerStartScript")).toString();
    useServerStartScript = settings->value(_("LastUseServerStartScript")).toBool();
    overrideStartScript = settings->value(_("LastRemoteStartScript")).toString();
    debugInfoLocation = settings->value(_("LastDebugInfoLocation")).toString();
}

StartRemoteDialog::StartRemoteDialog(QWidget *parent, bool enableStartScript)
  : QDialog(parent),
    m_ui(new Ui::StartRemoteDialog)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->setupUi(this);
    m_ui->toolchainComboBox->init(false);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    m_ui->debuginfoPathChooser->setPromptDialogTitle(tr("Select Location of Debugging Information"));
    m_ui->executablePathChooser->setExpectedKind(PathChooser::File);
    m_ui->executablePathChooser->setPromptDialogTitle(tr("Select Executable"));
    m_ui->sysrootPathChooser->setPromptDialogTitle(tr("Select Sysroot"));
    m_ui->overrideStartScriptPathChooser->setExpectedKind(PathChooser::File);
    m_ui->overrideStartScriptPathChooser->setPromptDialogTitle(tr("Select GDB Start Script"));
    m_ui->serverStartScriptPathChooser->setExpectedKind(PathChooser::File);
    m_ui->serverStartScriptPathChooser->setPromptDialogTitle(tr("Select Server Start Script"));
    m_ui->serverStartScriptPathChooser->setVisible(enableStartScript);
    m_ui->serverStartScriptLabel->setVisible(enableStartScript);
    m_ui->useServerStartScriptCheckBox->setVisible(enableStartScript);
    m_ui->useServerStartScriptLabel->setVisible(enableStartScript);

    connect(m_ui->useServerStartScriptCheckBox, SIGNAL(toggled(bool)),
        SLOT(updateState()));
    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(m_ui->historyComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(historyIndexChanged(int)));
    updateState();
}

StartRemoteDialog::~StartRemoteDialog()
{
    delete m_ui;
}

bool StartRemoteDialog::run(QWidget *parent,
                            QSettings *settings,
                            bool useScript,
                            DebuggerStartParameters *sp)
{
    const QString settingsGroup = _("DebugMode");
    const QString arrayName = useScript ?
        _("StartRemoteScript") : _("StartRemote");

    QStringList arches;
    arches << _("i386:x86-64:intel") << _("i386") << _("arm");

    QList<StartRemoteParameters> history =
        readParameterHistory<StartRemoteParameters>(settings, settingsGroup, arrayName);
    QTC_ASSERT(!history.isEmpty(), return false);

    foreach (const StartRemoteParameters &h, history)
        if (!arches.contains(h.remoteArchitecture))
            arches.prepend(h.remoteArchitecture);

    StartRemoteDialog dialog(parent, useScript);
    dialog.setRemoteArchitectures(arches);
    dialog.setHistory(history);
    dialog.setParameters(history.back());
    if (dialog.exec() != QDialog::Accepted)
        return false;
    const StartRemoteParameters newParameters = dialog.parameters();

    if (newParameters != history.back()) {
        history.push_back(newParameters);
        if (history.size() > 10)
            history.pop_front();
        writeParameterHistory(history, settings, settingsGroup, arrayName);
    }

    Profile *profile = dialog.profile();
    QTC_ASSERT(profile, return false);
    ToolChain *tc = ToolChainProfileInformation::toolChain(profile);
    QTC_ASSERT(tc, return false);

    sp->remoteChannel = newParameters.remoteChannel;
    sp->remoteArchitecture = newParameters.remoteArchitecture;
    sp->executable = newParameters.localExecutable;
    sp->displayName = tr("Remote: \"%1\"").arg(sp->remoteChannel);
    sp->debuggerCommand = DebuggerProfileInformation::debuggerCommand(profile).toString();
    sp->toolChainAbi = tc->targetAbi();
    sp->overrideStartScript = newParameters.overrideStartScript;
    sp->useServerStartScript = newParameters.useServerStartScript;
    sp->serverStartScript = newParameters.serverStartScript;
    sp->debugInfoLocation = newParameters.debugInfoLocation;
    return true;
}

StartRemoteParameters StartRemoteDialog::parameters() const
{
    StartRemoteParameters result;
    result.remoteChannel = m_ui->channelLineEdit->text();
    result.localExecutable = m_ui->executablePathChooser->path();
    result.remoteArchitecture = m_ui->architectureComboBox->currentText();
    result.overrideStartScript = m_ui->overrideStartScriptPathChooser->path();
    result.useServerStartScript = m_ui->useServerStartScriptCheckBox->isChecked();
    result.serverStartScript = m_ui->serverStartScriptPathChooser->path();
    result.profileId = m_ui->toolchainComboBox->currentProfileId();
    result.debugInfoLocation = m_ui->debuginfoPathChooser->path();
    return result;
}

void StartRemoteDialog::setParameters(const StartRemoteParameters &p)
{
    m_ui->channelLineEdit->setText(p.remoteChannel);
    m_ui->executablePathChooser->setPath(p.localExecutable);
    const int index = m_ui->architectureComboBox->findText(p.remoteArchitecture);
    if (index != -1)
        m_ui->architectureComboBox->setCurrentIndex(index);
    m_ui->overrideStartScriptPathChooser->setPath(p.overrideStartScript);
    m_ui->useServerStartScriptCheckBox->setChecked(p.useServerStartScript);
    m_ui->serverStartScriptPathChooser->setPath(p.serverStartScript);
    m_ui->toolchainComboBox->setCurrentProfileId(p.profileId);
    m_ui->debuginfoPathChooser->setPath(p.debugInfoLocation);
}

void StartRemoteDialog::setHistory(const QList<StartRemoteParameters> &l)
{
    m_ui->historyComboBox->clear();
    for (int i = l.size() -  1; i >= 0; --i)
        if (l.at(i).isValid())
            m_ui->historyComboBox->addItem(l.at(i).displayName(),
                                           QVariant::fromValue(l.at(i)));
}

void StartRemoteDialog::historyIndexChanged(int index)
{
    if (index < 0)
        return;
    const QVariant v = m_ui->historyComboBox->itemData(index);
    QTC_ASSERT(v.canConvert<StartRemoteParameters>(), return);
    setParameters(v.value<StartRemoteParameters>());
}

Profile *StartRemoteDialog::profile() const
{
    return m_ui->toolchainComboBox->currentProfile();
}

void StartRemoteDialog::setRemoteArchitectures(const QStringList &list)
{
    m_ui->architectureComboBox->clear();
    if (!list.isEmpty()) {
        m_ui->architectureComboBox->insertItems(0, list);
        m_ui->architectureComboBox->setCurrentIndex(0);
    }
}

void StartRemoteDialog::updateState()
{
    bool enabled = m_ui->useServerStartScriptCheckBox->isChecked();
    m_ui->serverStartScriptLabel->setEnabled(enabled);
    m_ui->serverStartScriptPathChooser->setEnabled(enabled);
}

///////////////////////////////////////////////////////////////////////
//
// AttachToQmlPortDialog
//
///////////////////////////////////////////////////////////////////////

AttachToQmlPortDialog::AttachToQmlPortDialog(QWidget *parent)
  : QDialog(parent),
    m_ui(new Ui::AttachToQmlPortDialog)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->setupUi(this);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

AttachToQmlPortDialog::~AttachToQmlPortDialog()
{
    delete m_ui;
}

void AttachToQmlPortDialog::setHost(const QString &host)
{
    m_ui->hostLineEdit->setText(host);
}

QString AttachToQmlPortDialog::host() const
{
    return m_ui->hostLineEdit->text();
}

void AttachToQmlPortDialog::setPort(const int port)
{
    m_ui->portSpinBox->setValue(port);
}

int AttachToQmlPortDialog::port() const
{
    return m_ui->portSpinBox->value();
}

QString AttachToQmlPortDialog::sysroot() const
{
    return m_ui->sysRootChooser->path();
}

void AttachToQmlPortDialog::setSysroot(const QString &sysroot)
{
    m_ui->sysRootChooser->setPath(sysroot);
}

// --------- StartRemoteCdbDialog
static inline QString cdbRemoteHelp()
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
                " to use TCP/IP as communication protocol.</p><p>Enter the connection parameters as:</p>"
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

    QGroupBox *groupBox = new QGroupBox;
    QFormLayout *formLayout = new QFormLayout;
    QLabel *helpLabel = new QLabel(cdbRemoteHelp());
    helpLabel->setWordWrap(true);
    helpLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    formLayout->addRow(helpLabel);
    QLabel *label = new QLabel(tr("&Connection:"));
    label->setBuddy(m_lineEdit);
    m_lineEdit->setMinimumWidth(400);
    connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));
    formLayout->addRow(label, m_lineEdit);
    groupBox->setLayout(formLayout);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addWidget(groupBox);
    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    vLayout->addWidget(box);
    m_okButton = box->button(QDialogButtonBox::Ok);
    connect(m_lineEdit, SIGNAL(returnPressed()), m_okButton, SLOT(animateClick()));
    m_okButton->setEnabled(false);
    connect(box, SIGNAL(accepted()), this, SLOT(accept()));
    connect(box, SIGNAL(rejected()), this, SLOT(reject()));

    setLayout(vLayout);
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
    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(new QLabel(tr("Enter an address: ")));
    hLayout->addWidget(m_lineEdit);
    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addLayout(hLayout);
    vLayout->addWidget(m_box);
    setLayout(vLayout);

    connect(m_box, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_box, SIGNAL(rejected()), this, SLOT(reject()));
    connect(m_lineEdit, SIGNAL(returnPressed()), this, SLOT(accept()));
    connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(textChanged()));

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
    m_lineEdit->setText(QLatin1String("0x")
                        + QString::number(a, 16));
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

StartRemoteEngineDialog::StartRemoteEngineDialog(QWidget *parent) :
    QDialog(parent) ,
    m_ui(new Ui::StartRemoteEngineDialog)
{
    QSettings *settings = Core::ICore::settings();
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->setupUi(this);
    m_ui->host->setCompleter(new HistoryCompleter(settings, m_ui->host));
    m_ui->username->setCompleter(new HistoryCompleter(settings, m_ui->username));
    m_ui->enginepath->setCompleter(new HistoryCompleter(settings, m_ui->enginepath));
    m_ui->inferiorpath->setCompleter(new HistoryCompleter(settings, m_ui->inferiorpath));
    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

StartRemoteEngineDialog::~StartRemoteEngineDialog()
{
    delete m_ui;
}

QString StartRemoteEngineDialog::host() const
{
    return m_ui->host->text();
}

QString StartRemoteEngineDialog::username() const
{
    return m_ui->username->text();
}

QString StartRemoteEngineDialog::password() const
{
    return m_ui->password->text();
}

QString StartRemoteEngineDialog::inferiorPath() const
{
    return m_ui->inferiorpath->text();
}

QString StartRemoteEngineDialog::enginePath() const
{
    return m_ui->enginepath->text();
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
        const QStringList &typeFormats, int current)
    {
        const int row = m_layout->rowCount();
        int column = 0;
        QButtonGroup *group = new QButtonGroup(this);
        m_layout->addWidget(new QLabel(type), row, column++);
        for (int i = -1; i != typeFormats.size(); ++i) {
            QRadioButton *choice = new QRadioButton(this);
            choice->setText(i == -1 ? TypeFormatsDialog::tr("Reset") : typeFormats.at(i));
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
        QVBoxLayout *layout = new QVBoxLayout(q);
        tabs = new QTabWidget(q);

        buttonBox = new QDialogButtonBox(q);
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
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
    QDialogButtonBox *buttonBox;
    QList<TypeFormatsDialogPage *> pages;

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
    const QStringList &typeFormats, int current)
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

TypeFormats TypeFormatsDialog::typeFormats() const
{
    return TypeFormats();
}

} // namespace Internal
} // namespace Debugger
