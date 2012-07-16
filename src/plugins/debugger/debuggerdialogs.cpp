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
#include "debuggertoolchaincombobox.h"
#include "cdb/cdbengine.h"
#include "shared/hostutils.h"

#include <coreplugin/icore.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/profileinformation.h>
#include <utils/filterlineedit.h>
#include <utils/historycompleter.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>

#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QRegExp>
#include <QScrollArea>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QTreeView>
#include <QVariant>
#include <QVBoxLayout>

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

class AttachCoreDialogPrivate
{
public:
    QLabel *execLabel;
    Utils::PathChooser *execFileName;
    QLabel *coreLabel;
    Utils::PathChooser *coreFileName;
    QLabel *profileLabel;
    Debugger::ProfileChooser *profileComboBox;
    Utils::PathChooser *overrideStartScriptFileName;
    QLabel *overrideStartScriptLabel;
    QDialogButtonBox *buttonBox;
};

AttachCoreDialog::AttachCoreDialog(QWidget *parent)
  : QDialog(parent), d(new AttachCoreDialogPrivate)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("Start Debugger"));

    d->coreFileName = new Utils::PathChooser(this);
    d->coreFileName->setExpectedKind(PathChooser::File);
    d->coreFileName->setPromptDialogTitle(tr("Select Core File"));
    d->coreLabel = new QLabel(tr("&Core file:"), this);
    d->coreLabel->setBuddy(d->coreFileName);

    d->execFileName = new Utils::PathChooser(this);
    d->execFileName->setExpectedKind(PathChooser::File);
    d->execFileName->setPromptDialogTitle(tr("Select Executable"));
    d->execLabel = new QLabel(tr("&Executable:"), this);
    d->execLabel->setBuddy(d->execFileName);

    d->overrideStartScriptFileName = new Utils::PathChooser(this);
    d->overrideStartScriptFileName->setExpectedKind(PathChooser::File);
    d->overrideStartScriptFileName->setPromptDialogTitle(tr("Select Startup Script"));
    d->overrideStartScriptLabel = new QLabel(tr("Override &start script:"), this);
    d->overrideStartScriptLabel->setBuddy(d->overrideStartScriptFileName);

    d->profileComboBox = new Debugger::ProfileChooser(this);
    d->profileComboBox->init(false);
    d->profileLabel = new QLabel(tr("&Target:"), this);
    d->profileLabel->setBuddy(d->profileComboBox);

    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    d->buttonBox = new QDialogButtonBox(this);
    d->buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    d->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setHorizontalSpacing(6);
    formLayout->setVerticalSpacing(6);
    formLayout->addRow(d->execLabel, d->execFileName);
    formLayout->addRow(d->coreLabel, d->coreFileName);
    formLayout->addRow(d->profileLabel, d->profileComboBox);
    formLayout->addRow(d->overrideStartScriptLabel, d->overrideStartScriptFileName);

    QVBoxLayout *vboxLayout = new QVBoxLayout(this);
    vboxLayout->addLayout(formLayout);
    vboxLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    vboxLayout->addWidget(line);
    vboxLayout->addWidget(d->buttonBox);

    connect(d->buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(d->buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(d->coreFileName, SIGNAL(changed(QString)), SLOT(changed()));
    changed();
}

AttachCoreDialog::~AttachCoreDialog()
{
    delete d;
}

QString AttachCoreDialog::executableFile() const
{
    return d->execFileName->path();
}

void AttachCoreDialog::setExecutableFile(const QString &fileName)
{
    d->execFileName->setPath(fileName);
    changed();
}

QString AttachCoreDialog::coreFile() const
{
    return d->coreFileName->path();
}

void AttachCoreDialog::setCoreFile(const QString &fileName)
{
    d->coreFileName->setPath(fileName);
    changed();
}

Core::Id AttachCoreDialog::profileId() const
{
    return d->profileComboBox->currentProfileId();
}

void AttachCoreDialog::setProfileIndex(int i)
{
    if (i >= 0 && i < d->profileComboBox->count())
        d->profileComboBox->setCurrentIndex(i);
}

int AttachCoreDialog::profileIndex() const
{
    return d->profileComboBox->currentIndex();
}

QString AttachCoreDialog::overrideStartScript() const
{
    return d->overrideStartScriptFileName->path();
}

void AttachCoreDialog::setOverrideStartScript(const QString &scriptName)
{
    d->overrideStartScriptFileName->setPath(scriptName);
}

bool AttachCoreDialog::isValid() const
{
    return d->profileComboBox->currentIndex() >= 0 &&
           !coreFile().isEmpty();
}

void AttachCoreDialog::changed()
{
    d->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isValid());
}

///////////////////////////////////////////////////////////////////////
//
// AttachExternalDialog
//
///////////////////////////////////////////////////////////////////////

class AttachExternalDialogPrivate
{
public:
    QLabel *pidLabel;
    QLineEdit *pidLineEdit;
    Utils::FilterLineEdit *filterWidget;
    QLabel *profileLabel;
    Debugger::ProfileChooser *profileComboBox;
    QTreeView *procView;
    QDialogButtonBox *buttonBox;

    QString selfPid;
    ProcessListFilterModel *model;
};

AttachExternalDialog::AttachExternalDialog(QWidget *parent)
  : QDialog(parent),
    d(new AttachExternalDialogPrivate)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("Start Debugger", 0, QApplication::UnicodeUTF8));
    setMinimumHeight(500);

    d->selfPid = QString::number(QCoreApplication::applicationPid());

    d->model = new ProcessListFilterModel(this);

    d->pidLineEdit = new QLineEdit(this);
    d->pidLabel = new QLabel(tr("Attach to &process ID:"), this);
    d->pidLabel->setBuddy(d->pidLineEdit);

    d->filterWidget = new Utils::FilterLineEdit(this);
    d->filterWidget->setFocus(Qt::TabFocusReason);

    d->profileComboBox = new Debugger::ProfileChooser(this);
    d->profileComboBox->init(true);
    d->profileLabel = new QLabel(tr("&Target:"), this);
    d->profileLabel->setBuddy(d->profileComboBox);

    d->procView = new QTreeView(this);
    d->procView->setAlternatingRowColors(true);
    d->procView->setRootIsDecorated(false);
    d->procView->setUniformRowHeights(true);
    d->procView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    d->procView->setModel(d->model);
    d->procView->setSortingEnabled(true);
    d->procView->sortByColumn(1, Qt::AscendingOrder);

    QPushButton *refreshButton = new QPushButton(tr("Refresh"));

    d->buttonBox = new QDialogButtonBox(this);
    d->buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    d->buttonBox->addButton(refreshButton, QDialogButtonBox::ActionRole);

    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->addRow(d->pidLabel, d->pidLineEdit);
    formLayout->addRow(d->profileLabel, d->profileComboBox);
    formLayout->addRow(d->filterWidget);

    QVBoxLayout *vboxLayout = new QVBoxLayout(this);
    vboxLayout->addLayout(formLayout);
    vboxLayout->addWidget(d->procView);
    vboxLayout->addWidget(line);
    vboxLayout->addWidget(d->buttonBox);

    okButton()->setDefault(true);
    okButton()->setEnabled(false);

    connect(refreshButton, SIGNAL(clicked()), SLOT(rebuildProcessList()));
    connect(d->buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(d->buttonBox, SIGNAL(rejected()), SLOT(reject()));
    // Do not use activated, will be single click in Oxygen
    connect(d->procView, SIGNAL(doubleClicked(QModelIndex)),
        SLOT(procSelected(QModelIndex)));
    connect(d->procView, SIGNAL(clicked(QModelIndex)),
        SLOT(procClicked(QModelIndex)));
    connect(d->pidLineEdit, SIGNAL(textChanged(QString)),
        SLOT(pidChanged(QString)));
    connect(d->filterWidget, SIGNAL(filterChanged(QString)),
        SLOT(setFilterString(QString)));

    rebuildProcessList();
}

AttachExternalDialog::~AttachExternalDialog()
{
    delete d;
}

void AttachExternalDialog::setFilterString(const QString &filter)
{
    d->model->setFilterFixedString(filter);
    // Activate the line edit if there's a unique filtered process.
    QString processId;
    if (d->model->rowCount(QModelIndex()) == 1)
        processId = d->model->processIdAt(d->model->index(0, 0, QModelIndex()));
    d->pidLineEdit->setText(processId);
    pidChanged(processId);
}

QPushButton *AttachExternalDialog::okButton() const
{
    return d->buttonBox->button(QDialogButtonBox::Ok);
}

void AttachExternalDialog::rebuildProcessList()
{
    d->model->populate(hostProcessList(), d->selfPid);
    d->procView->expandAll();
    d->procView->resizeColumnToContents(0);
    d->procView->resizeColumnToContents(1);
}

void AttachExternalDialog::procSelected(const QModelIndex &proxyIndex)
{
    const QString processId = d->model->processIdAt(proxyIndex);
    if (!processId.isEmpty()) {
        d->pidLineEdit->setText(processId);
        if (okButton()->isEnabled())
            okButton()->animateClick();
    }
}

void AttachExternalDialog::procClicked(const QModelIndex &proxyIndex)
{
    const QString processId = d->model->processIdAt(proxyIndex);
    if (!processId.isEmpty())
        d->pidLineEdit->setText(processId);
}

QString AttachExternalDialog::attachPIDText() const
{
    return d->pidLineEdit->text().trimmed();
}

qint64 AttachExternalDialog::attachPID() const
{
    return attachPIDText().toLongLong();
}

QString AttachExternalDialog::executable() const
{
    // Search pid in model in case the user typed in the PID.
    return d->model->executableForPid(attachPIDText());
}

Core::Id AttachExternalDialog::profileId() const
{
    return d->profileComboBox->currentProfileId();
}

void AttachExternalDialog::setProfileIndex(int i)
{
    if (i >= 0 && i < d->profileComboBox->count())
        d->profileComboBox->setCurrentIndex(i);
}

int AttachExternalDialog::profileIndex() const
{
    return d->profileComboBox->currentIndex();
}

void AttachExternalDialog::pidChanged(const QString &pid)
{
    const bool enabled = !pid.isEmpty() && pid != QLatin1String("0") && pid != d->selfPid
            && d->profileComboBox->currentIndex() >= 0;
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

bool operator==(const StartExternalParameters &p1, const StartExternalParameters &p2)
{
    return p1.equals(p2);
}

bool operator!=(const StartExternalParameters &p1, const StartExternalParameters &p2)
{
    return !p1.equals(p2);
}

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

class StartExternalDialogPrivate
{
public:
    QLabel *execLabel;
    Utils::PathChooser *execFile;
    QLabel *argsLabel;
    QLineEdit *argsEdit;
    QLabel *runInTerminalLabel;
    QCheckBox *runInTerminalCheckBox;
    QLabel *workingDirectoryLabel;
    Utils::PathChooser *workingDirectory;
    QLabel *profileLabel;
    Debugger::ProfileChooser *profileChooser;
    QLabel *breakAtMainLabel;
    QCheckBox *breakAtMainCheckBox;
    QLabel *historyLabel;
    QComboBox *historyComboBox;
    QFrame *historyLine;
    QSpacerItem *spacerItem;
    QDialogButtonBox *buttonBox;
};

StartExternalDialog::StartExternalDialog(QWidget *parent)
  : QDialog(parent), d(new StartExternalDialogPrivate)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("Start Debugger"));

    QSettings *settings = Core::ICore::settings();

    d->execFile = new Utils::PathChooser(this);
    d->execFile->setExpectedKind(PathChooser::File);
    d->execFile->setPromptDialogTitle(tr("Select Executable"));
    d->execFile->lineEdit()->setCompleter(
        new HistoryCompleter(settings, d->execFile->lineEdit()));
    d->execLabel = new QLabel(tr("&Executable:"), this);
    d->execLabel->setBuddy(d->execFile);

    d->argsEdit = new QLineEdit(this);
    d->argsEdit->setCompleter(new HistoryCompleter(settings, d->argsEdit));
    d->argsLabel = new QLabel(tr("&Arguments:"), this);
    d->argsLabel->setBuddy(d->argsEdit);

    d->workingDirectory = new Utils::PathChooser(this);
    d->workingDirectory->setExpectedKind(PathChooser::ExistingDirectory);
    d->workingDirectory->setPromptDialogTitle(tr("Select Working Directory"));
    d->workingDirectory->lineEdit()->setCompleter(
        new HistoryCompleter(settings, d->workingDirectory->lineEdit()));
    d->workingDirectoryLabel = new QLabel(tr("&Working directory:"), this);
    d->workingDirectoryLabel->setBuddy(d->workingDirectory);

    d->runInTerminalCheckBox = new QCheckBox(this);
    d->runInTerminalLabel = new QLabel(tr("Run in &terminal:"), this);
    d->runInTerminalLabel->setBuddy(d->runInTerminalCheckBox);

    d->profileChooser = new Debugger::ProfileChooser(this);
    d->profileChooser->init(true);
    d->profileLabel = new QLabel(tr("&Target:"), this);
    d->profileLabel->setBuddy(d->profileChooser);

    d->breakAtMainCheckBox = new QCheckBox(this);
    d->breakAtMainCheckBox->setText(QString());
    d->breakAtMainLabel = new QLabel(tr("Break at \"&main\":"), this);
    d->breakAtMainLabel->setBuddy(d->breakAtMainCheckBox);

    d->historyComboBox = new QComboBox(this);
    d->historyLabel = new QLabel(tr("&Recent:"), this);
    d->historyLabel->setBuddy(d->historyComboBox);

    QFrame *historyLine = new QFrame(this);
    historyLine->setFrameShape(QFrame::HLine);
    historyLine->setFrameShadow(QFrame::Sunken);

    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    d->buttonBox = new QDialogButtonBox(this);
    d->buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    d->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setHorizontalSpacing(6);
    formLayout->setVerticalSpacing(6);
    formLayout->addRow(d->execLabel, d->execFile);
    formLayout->addRow(d->argsLabel, d->argsEdit);
    formLayout->addRow(d->runInTerminalLabel, d->runInTerminalCheckBox);
    formLayout->addRow(d->workingDirectoryLabel, d->workingDirectory);
    formLayout->addRow(d->profileLabel, d->profileChooser);
    formLayout->addRow(d->breakAtMainLabel, d->breakAtMainCheckBox);
    formLayout->addRow(d->historyLabel, d->historyComboBox);
    formLayout->addWidget(historyLine);

    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    verticalLayout->addLayout(formLayout);
    verticalLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    verticalLayout->addWidget(line);
    verticalLayout->addWidget(d->buttonBox);

    connect(d->execFile, SIGNAL(changed(QString)), SLOT(changed()));
    connect(d->buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(d->buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(d->historyComboBox, SIGNAL(currentIndexChanged(int)),
            SLOT(historyIndexChanged(int)));
    changed();
}

StartExternalDialog::~StartExternalDialog()
{
    delete d;
}

StartExternalParameters StartExternalDialog::parameters() const
{
    StartExternalParameters result;
    result.executableFile = d->execFile->path();
    result.arguments = d->argsEdit->text();
    result.workingDirectory = d->workingDirectory->path();
    result.abiIndex = d->profileChooser->currentIndex();
    result.breakAtMain = d->breakAtMainCheckBox->isChecked();
    result.runInTerminal = d->runInTerminalCheckBox->isChecked();
    return result;
}

void StartExternalDialog::setParameters(const StartExternalParameters &p)
{
    setExecutableFile(p.executableFile);
    d->argsEdit->setText(p.arguments);
    d->workingDirectory->setPath(p.workingDirectory);
    if (p.abiIndex >= 0 && p.abiIndex < d->profileChooser->count())
        d->profileChooser->setCurrentIndex(p.abiIndex);
    d->runInTerminalCheckBox->setChecked(p.runInTerminal);
    d->breakAtMainCheckBox->setChecked(p.breakAtMain);
}

void StartExternalDialog::setHistory(const QList<StartExternalParameters> &l)
{
    d->historyComboBox->clear();
    for (int i = l.size(); --i >= 0; ) {
        const StartExternalParameters &p = l.at(i);
        if (p.isValid())
            d->historyComboBox->addItem(p.displayName(), QVariant::fromValue(p));
    }
}

void StartExternalDialog::historyIndexChanged(int index)
{
    if (index < 0)
        return;
    const QVariant v = d->historyComboBox->itemData(index);
    QTC_ASSERT(v.canConvert<StartExternalParameters>(), return);
    setParameters(v.value<StartExternalParameters>());
}

void StartExternalDialog::setExecutableFile(const QString &str)
{
    d->execFile->setPath(str);
    changed();
}

QString StartExternalDialog::executableFile() const
{
    return d->execFile->path();
}

Core::Id StartExternalDialog::profileId() const
{
    return d->profileChooser->currentProfileId();
}

bool StartExternalDialog::isValid() const
{
    return d->profileChooser->currentIndex() >= 0 && !executableFile().isEmpty();
}

void StartExternalDialog::changed()
{
    d->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isValid());
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

    fillParameters(sp, dialog.profileId());
    sp->executable = newParameters.executableFile;
    sp->startMode = StartExternal;
    sp->workingDirectory = newParameters.workingDirectory;
    sp->displayName = sp->executable;
    sp->useTerminal = newParameters.runInTerminal;
    if (!newParameters.arguments.isEmpty())
        sp->processArgs = newParameters.arguments;
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

bool operator==(const StartRemoteParameters &p1, const StartRemoteParameters &p2)
{
    return p1.equals(p2);
}

bool operator!=(const StartRemoteParameters &p1, const StartRemoteParameters &p2)
{
    return !p1.equals(p2);
}

StartRemoteParameters::StartRemoteParameters() :
    useServerStartScript(false)
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
    const QString profileIdString = settings->value(_("LastProfileId")).toString();
    if (profileIdString.isEmpty()) {
        profileId = Core::Id();
    } else {
        profileId = Core::Id(profileIdString);
    }
    remoteArchitecture = settings->value(_("LastRemoteArchitecture")).toString();
    serverStartScript = settings->value(_("LastServerStartScript")).toString();
    useServerStartScript = settings->value(_("LastUseServerStartScript")).toBool();
    overrideStartScript = settings->value(_("LastRemoteStartScript")).toString();
    debugInfoLocation = settings->value(_("LastDebugInfoLocation")).toString();
}


class StartRemoteDialogPrivate
{
public:
    QLabel *profileLabel;
    Debugger::ProfileChooser *profileChooser;
    QLabel *executableLabel;
    Utils::PathChooser *executablePathChooser;
    QLabel *channelLabel;
    QLineEdit *channelLineEdit;
    QLabel *architectureLabel;
    QComboBox *architectureComboBox;
    QLabel *debuginfoLabel;
    Utils::PathChooser *debuginfoPathChooser;
    QLabel *overrideStartScriptLabel;
    Utils::PathChooser *overrideStartScriptPathChooser;
    QLabel *useServerStartScriptLabel;
    QCheckBox *useServerStartScriptCheckBox;
    QLabel *serverStartScriptLabel;
    Utils::PathChooser *serverStartScriptPathChooser;
    QLabel *historyLabel;
    QComboBox *historyComboBox;
    QDialogButtonBox *buttonBox;
};

StartRemoteDialog::StartRemoteDialog(QWidget *parent, bool enableStartScript)
  : QDialog(parent),
    d(new StartRemoteDialogPrivate)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("Start Debugger"));

    d->profileChooser = new Debugger::ProfileChooser(this);
    d->profileChooser->init(false);
    d->profileLabel = new QLabel(tr("Target:"), this);
    d->profileLabel->setBuddy(d->profileChooser);

    d->executablePathChooser = new Utils::PathChooser(this);
    d->executablePathChooser->setExpectedKind(PathChooser::File);
    d->executablePathChooser->setPromptDialogTitle(tr("Select Executable"));
    d->executableLabel = new QLabel(tr("Local &executable:"));
    d->executableLabel->setBuddy(d->executablePathChooser);

    d->channelLineEdit = new QLineEdit(this);
    d->channelLineEdit->setText(QString::fromUtf8("localhost:5115"));
    d->channelLabel = new QLabel(tr("&Host and port:"), this);
    d->channelLabel->setBuddy(d->channelLineEdit);

    d->architectureComboBox = new QComboBox(this);
    d->architectureComboBox->setEditable(true);
    d->architectureLabel = new QLabel(tr("&Architecture:"), this);
    d->architectureLabel->setBuddy(d->architectureComboBox);

    d->debuginfoPathChooser = new Utils::PathChooser(this);
    d->debuginfoPathChooser->setPromptDialogTitle(tr("Select Location of Debugging Information"));
    d->debuginfoLabel = new QLabel(tr("Location of debugging &information:"), this);
    d->debuginfoLabel->setBuddy(d->debuginfoPathChooser);

    d->overrideStartScriptPathChooser = new Utils::PathChooser(this);
    d->overrideStartScriptPathChooser->setExpectedKind(PathChooser::File);
    d->overrideStartScriptPathChooser->setPromptDialogTitle(tr("Select GDB Start Script"));
    d->overrideStartScriptLabel = new QLabel(tr("Override host GDB s&tart script:"), this);
    d->overrideStartScriptLabel->setBuddy(d->overrideStartScriptPathChooser);

    d->useServerStartScriptCheckBox = new QCheckBox(this);
    d->useServerStartScriptCheckBox->setVisible(enableStartScript);
    d->useServerStartScriptLabel = new QLabel(tr("&Use server start script:"), this);
    d->useServerStartScriptLabel->setVisible(enableStartScript);

    d->serverStartScriptPathChooser = new Utils::PathChooser(this);
    d->serverStartScriptPathChooser->setExpectedKind(PathChooser::File);
    d->serverStartScriptPathChooser->setPromptDialogTitle(tr("Select Server Start Script"));
    d->serverStartScriptPathChooser->setVisible(enableStartScript);
    d->serverStartScriptLabel = new QLabel(tr("&Server start script:"), this);
    d->serverStartScriptLabel->setVisible(enableStartScript);
    d->serverStartScriptLabel->setBuddy(d->serverStartScriptPathChooser);

    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    d->historyComboBox = new QComboBox(this);
    d->historyLabel = new QLabel(tr("&Recent:"), this);
    d->historyLabel->setBuddy(d->historyComboBox);

    QFrame *line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);

    d->buttonBox = new QDialogButtonBox(this);
    d->buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    d->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(d->profileLabel, d->profileChooser);
    formLayout->addRow(d->executableLabel, d->executablePathChooser);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->addRow(d->channelLabel, d->channelLineEdit);
    formLayout->addRow(d->architectureLabel, d->architectureComboBox);
    formLayout->addRow(d->debuginfoLabel, d->debuginfoPathChooser);
    formLayout->addRow(d->overrideStartScriptLabel, d->overrideStartScriptPathChooser);
    formLayout->addRow(d->useServerStartScriptLabel, d->useServerStartScriptCheckBox);
    formLayout->addRow(d->serverStartScriptLabel, d->serverStartScriptPathChooser);
    formLayout->addRow(line);
    formLayout->addRow(d->historyLabel, d->historyComboBox);

    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    verticalLayout->addLayout(formLayout);
    verticalLayout->addWidget(line2);
    verticalLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    verticalLayout->addWidget(d->buttonBox);

    connect(d->useServerStartScriptCheckBox, SIGNAL(toggled(bool)),
        SLOT(updateState()));
    connect(d->buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(d->buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(d->historyComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(historyIndexChanged(int)));
    updateState();
}

StartRemoteDialog::~StartRemoteDialog()
{
    delete d;
}

bool StartRemoteDialog::run(QWidget *parent, QSettings *settings,
                            bool useScript, DebuggerStartParameters *sp)
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

    fillParameters(sp, dialog.profileId());
    sp->remoteChannel = newParameters.remoteChannel;
    sp->remoteArchitecture = newParameters.remoteArchitecture;
    sp->executable = newParameters.localExecutable;
    sp->displayName = tr("Remote: \"%1\"").arg(sp->remoteChannel);
    sp->overrideStartScript = newParameters.overrideStartScript;
    sp->useServerStartScript = newParameters.useServerStartScript;
    sp->serverStartScript = newParameters.serverStartScript;
    sp->debugInfoLocation = newParameters.debugInfoLocation;
    return true;
}

StartRemoteParameters StartRemoteDialog::parameters() const
{
    StartRemoteParameters result;
    result.remoteChannel = d->channelLineEdit->text();
    result.localExecutable = d->executablePathChooser->path();
    result.remoteArchitecture = d->architectureComboBox->currentText();
    result.overrideStartScript = d->overrideStartScriptPathChooser->path();
    result.useServerStartScript = d->useServerStartScriptCheckBox->isChecked();
    result.serverStartScript = d->serverStartScriptPathChooser->path();
    result.profileId = d->profileChooser->currentProfileId();
    result.debugInfoLocation = d->debuginfoPathChooser->path();
    return result;
}

void StartRemoteDialog::setParameters(const StartRemoteParameters &p)
{
    d->channelLineEdit->setText(p.remoteChannel);
    d->executablePathChooser->setPath(p.localExecutable);
    const int index = d->architectureComboBox->findText(p.remoteArchitecture);
    if (index != -1)
        d->architectureComboBox->setCurrentIndex(index);
    d->overrideStartScriptPathChooser->setPath(p.overrideStartScript);
    d->useServerStartScriptCheckBox->setChecked(p.useServerStartScript);
    d->serverStartScriptPathChooser->setPath(p.serverStartScript);
    d->profileChooser->setCurrentProfileId(p.profileId);
    d->debuginfoPathChooser->setPath(p.debugInfoLocation);
}

void StartRemoteDialog::setHistory(const QList<StartRemoteParameters> &l)
{
    d->historyComboBox->clear();
    for (int i = l.size() -  1; i >= 0; --i)
        if (l.at(i).isValid())
            d->historyComboBox->addItem(l.at(i).displayName(),
                                           QVariant::fromValue(l.at(i)));
}

void StartRemoteDialog::historyIndexChanged(int index)
{
    if (index < 0)
        return;
    const QVariant v = d->historyComboBox->itemData(index);
    QTC_ASSERT(v.canConvert<StartRemoteParameters>(), return);
    setParameters(v.value<StartRemoteParameters>());
}

Core::Id StartRemoteDialog::profileId() const
{
    return d->profileChooser->currentProfileId();
}

void StartRemoteDialog::setRemoteArchitectures(const QStringList &list)
{
    d->architectureComboBox->clear();
    if (!list.isEmpty()) {
        d->architectureComboBox->insertItems(0, list);
        d->architectureComboBox->setCurrentIndex(0);
    }
}

void StartRemoteDialog::updateState()
{
    bool enabled = d->useServerStartScriptCheckBox->isChecked();
    d->serverStartScriptLabel->setEnabled(enabled);
    d->serverStartScriptPathChooser->setEnabled(enabled);
}

///////////////////////////////////////////////////////////////////////
//
// AttachToQmlPortDialog
//
///////////////////////////////////////////////////////////////////////

class AttachToQmlPortDialogPrivate
{
public:
    QLabel *hostLabel;
    QLineEdit *hostLineEdit;
    QLabel *portLabel;
    QSpinBox *portSpinBox;
    QLabel *profileLabel;
    Debugger::ProfileChooser *profileChooser;
};

AttachToQmlPortDialog::AttachToQmlPortDialog(QWidget *parent)
  : QDialog(parent),
    d(new AttachToQmlPortDialogPrivate)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("Start Debugger"));

    d->profileChooser = new Debugger::ProfileChooser(this);
    d->profileLabel = new QLabel(tr("Target:"), this);
    d->profileLabel->setBuddy(d->profileChooser);

    d->hostLineEdit = new QLineEdit(this);
    d->hostLineEdit->setText(QString::fromUtf8("localhost"));
    d->hostLabel = new QLabel(tr("&Host:"), this);
    d->hostLabel->setBuddy(d->hostLineEdit);

    d->portSpinBox = new QSpinBox(this);
    d->portSpinBox->setMaximum(65535);
    d->portSpinBox->setValue(3768);
    d->portLabel = new QLabel(tr("&Port:"), this);
    d->portLabel->setBuddy(d->portSpinBox);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(d->profileLabel, d->profileChooser);
    formLayout->addRow(d->hostLabel, d->hostLineEdit);
    formLayout->addRow(d->portLabel, d->portSpinBox);

    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    verticalLayout->addLayout(formLayout);
    verticalLayout->addWidget(buttonBox);

    connect(buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
}

AttachToQmlPortDialog::~AttachToQmlPortDialog()
{
    delete d;
}

void AttachToQmlPortDialog::setHost(const QString &host)
{
    d->hostLineEdit->setText(host);
}

QString AttachToQmlPortDialog::host() const
{
    return d->hostLineEdit->text();
}

void AttachToQmlPortDialog::setPort(const int port)
{
    d->portSpinBox->setValue(port);
}

int AttachToQmlPortDialog::port() const
{
    return d->portSpinBox->value();
}

Core::Id AttachToQmlPortDialog::profileId() const
{
    return d->profileChooser->currentProfileId();
}

void AttachToQmlPortDialog::setProfileId(const Core::Id &id)
{
    d->profileChooser->setCurrentProfileId(id);
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

    QLabel *helpLabel = new QLabel(cdbRemoteHelp());
    helpLabel->setWordWrap(true);
    helpLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

    QLabel *label = new QLabel(tr("&Connection:"));
    label->setBuddy(m_lineEdit);
    m_lineEdit->setMinimumWidth(400);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow(helpLabel);
    formLayout->addRow(label, m_lineEdit);
    groupBox->setLayout(formLayout);

    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->addWidget(groupBox);
    vLayout->addWidget(box);

    m_okButton = box->button(QDialogButtonBox::Ok);
    m_okButton->setEnabled(false);

    connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));
    connect(m_lineEdit, SIGNAL(returnPressed()), m_okButton, SLOT(animateClick()));
    connect(box, SIGNAL(accepted()), this, SLOT(accept()));
    connect(box, SIGNAL(rejected()), this, SLOT(reject()));
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
    QLabel *hostLabel;
    QLineEdit *host;
    QLabel *userLabel;
    QLineEdit *username;
    QLabel *passwordLabel;
    QLineEdit *password;
    QLabel *engineLabel;
    QLineEdit *enginePath;
    QLabel *inferiorLabel;
    QLineEdit *inferiorPath;
    QDialogButtonBox *buttonBox;
};

StartRemoteEngineDialog::StartRemoteEngineDialog(QWidget *parent)
    : QDialog(parent), d(new StartRemoteEngineDialogPrivate)
{
    QSettings *settings = Core::ICore::settings();
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("Start Remote Engine"));

    d->host = new QLineEdit(this);
    d->host->setText(QString());
    d->host->setCompleter(new HistoryCompleter(settings, d->host));
    d->hostLabel = new QLabel(tr("&Host:"), this);
    d->hostLabel->setBuddy(d->host);

    d->username = new QLineEdit(this);
    d->username->setCompleter(new HistoryCompleter(settings, d->username));
    d->userLabel = new QLabel(tr("&Username:"), this);
    d->userLabel->setBuddy(d->username);

    d->password = new QLineEdit(this);
    d->password->setEchoMode(QLineEdit::Password);
    d->passwordLabel = new QLabel(tr("&Password:"), this);
    d->passwordLabel->setBuddy(d->password);

    d->enginePath = new QLineEdit(this);
    d->enginePath->setCompleter(new HistoryCompleter(settings, d->enginePath));
    d->engineLabel = new QLabel(tr("&Engine path:"), this);
    d->engineLabel->setBuddy(d->enginePath);

    d->inferiorPath = new QLineEdit(this);
    d->inferiorPath->setCompleter(new HistoryCompleter(settings, d->inferiorPath));
    d->inferiorLabel = new QLabel(tr("&Inferior path:"), this);
    d->inferiorLabel->setBuddy(d->inferiorPath);

    d->buttonBox = new QDialogButtonBox(this);
    d->buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(d->hostLabel, d->host);
    formLayout->addRow(d->userLabel, d->username);
    formLayout->addRow(d->passwordLabel, d->password);
    formLayout->addRow(d->engineLabel, d->enginePath);
    formLayout->addRow(d->inferiorLabel, d->inferiorPath);

    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    verticalLayout->addLayout(formLayout);
    verticalLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    verticalLayout->addWidget(d->buttonBox);

    connect(d->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(d->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
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
