/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "debuggerdialogs.h"

#include "ui_attachcoredialog.h"
#include "ui_attachexternaldialog.h"
#include "ui_attachtcfdialog.h"
#include "ui_startexternaldialog.h"
#include "ui_startremotedialog.h"

#ifdef Q_OS_WIN
#  include "shared/dbgwinutils.h"
#endif

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QCoreApplication>
#include <QtGui/QStandardItemModel>
#include <QtGui/QHeaderView>
#include <QtGui/QFileDialog>
#include <QtGui/QPushButton>
#include <QtGui/QProxyModel>
#include <QtGui/QSortFilterProxyModel>


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
    void populate(QList<ProcData> processes, const QString &excludePid = QString());

private:
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

QString ProcessListFilterModel::processIdAt(const QModelIndex &index) const
{
    if (index.isValid()) {
        const QModelIndex index0 = mapToSource(index);
        QModelIndex index = index0.sibling(index0.row(), 0);
        if (const QStandardItem *item = m_model->itemFromIndex(index))
            return item->text();
    }
    return QString();
}

void ProcessListFilterModel::populate(QList<ProcData> processes, const QString &excludePid)
{
    qStableSort(processes);

    if (const int rowCount = m_model->rowCount())
        m_model->removeRows(0, rowCount);

    QStandardItem *root  = m_model->invisibleRootItem();
    foreach(const ProcData &proc, processes) {
        QList<QStandardItem *> row;
        row.append(new QStandardItem(proc.ppid));
        row.append(new QStandardItem(proc.name));
        if (!proc.image.isEmpty())
            row.back()->setToolTip(proc.image);
        row.append(new QStandardItem(proc.state));
        if (proc.ppid == excludePid)
            foreach(QStandardItem *i, row)
                i->setEnabled(false);
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
    m_ui->setupUi(this);

    m_ui->execFileName->setExpectedKind(Core::Utils::PathChooser::File);
    m_ui->execFileName->setPromptDialogTitle(tr("Select Executable"));

    m_ui->coreFileName->setExpectedKind(Core::Utils::PathChooser::File);
    m_ui->coreFileName->setPromptDialogTitle(tr("Select Core File"));

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
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
}

QString AttachCoreDialog::coreFile() const
{
    return m_ui->coreFileName->path();
}

void AttachCoreDialog::setCoreFile(const QString &fileName)
{
    m_ui->coreFileName->setPath(fileName);
}


///////////////////////////////////////////////////////////////////////
//
// Process model helpers
//
///////////////////////////////////////////////////////////////////////

static bool isUnixProcessId(const QString &procname)
{
    for (int i = 0; i != procname.size(); ++i)
        if (!procname.at(i).isDigit())
            return false;
    return true;
}

// Determine UNIX processes by reading "/proc"
static QList<ProcData> unixProcessList()
{
    QList<ProcData> rc;
    const QStringList procIds = QDir(QLatin1String("/proc/")).entryList();
    if (procIds.isEmpty())
        return rc;

    foreach (const QString &procId, procIds) {
        if (!isUnixProcessId(procId))
            continue;
        QString filename = QLatin1String("/proc/");
        filename += procId;
        filename += QLatin1String("/stat");
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly))
            continue;           // process may have exited

        const QStringList data = QString::fromLocal8Bit(file.readAll()).split(' ');
        ProcData proc;
        proc.ppid = procId;
        proc.name = data.at(1);
        if (proc.name.startsWith(QLatin1Char('(')) && proc.name.endsWith(QLatin1Char(')'))) {
            proc.name.truncate(proc.name.size() - 1);
            proc.name.remove(0, 1);
        }
        proc.state = data.at(2);
        // PPID is element 3
        rc.push_back(proc);
    }
    return rc;
}

static QList<ProcData> processList()
{
#ifdef Q_OS_WIN
    return winProcessList();
#else
    return unixProcessList();
#endif
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
    m_ui->setupUi(this);
    okButton()->setDefault(true);
    okButton()->setEnabled(false);

    m_ui->procView->setModel(m_model);
    m_ui->procView->setSortingEnabled(true);

    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    QPushButton *refreshButton = new QPushButton(tr("Refresh"));
    connect(refreshButton, SIGNAL(clicked()), this, SLOT(rebuildProcessList()));
    m_ui->buttonBox->addButton(refreshButton, QDialogButtonBox::ActionRole);
    m_ui->filterLineEdit->setFocus(Qt::TabFocusReason);

    // Do not use activated, will be single click in Oxygen
    connect(m_ui->procView, SIGNAL(doubleClicked(QModelIndex)),
        this, SLOT(procSelected(QModelIndex)));
    connect(m_ui->pidLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(pidChanged(QString)));

    connect(m_ui->filterClearToolButton, SIGNAL(clicked()),
            m_ui->filterLineEdit, SLOT(clear()));
    connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(setFilterString(QString)));

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
    m_model->populate(processList(), m_selfPid);
    m_ui->procView->expandAll();
    m_ui->procView->resizeColumnToContents(0);
    m_ui->procView->resizeColumnToContents(1);
}

void AttachExternalDialog::procSelected(const QModelIndex &proxyIndex)
{
    const QString processId  = m_model->processIdAt(proxyIndex);
    if (!processId.isEmpty()) {
        m_ui->pidLineEdit->setText(processId);
        if (okButton()->isEnabled())
            okButton()->animateClick();
    }
}

qint64 AttachExternalDialog::attachPID() const
{
    return m_ui->pidLineEdit->text().toLongLong();
}

void AttachExternalDialog::pidChanged(const QString &pid)
{
    bool enabled = !pid.isEmpty() && pid != QLatin1String("0") && pid != m_selfPid;;
    okButton()->setEnabled(enabled);
}


///////////////////////////////////////////////////////////////////////
//
// AttachTcfDialog
//
///////////////////////////////////////////////////////////////////////

AttachTcfDialog::AttachTcfDialog(QWidget *parent)
  : QDialog(parent),
    m_ui(new Ui::AttachTcfDialog)
{
    m_ui->setupUi(this);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    m_ui->serverStartScript->setExpectedKind(Core::Utils::PathChooser::File);
    m_ui->serverStartScript->setPromptDialogTitle(tr("Select Executable"));

    connect(m_ui->useServerStartScriptCheckBox, SIGNAL(toggled(bool)), 
        this, SLOT(updateState()));
    
    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    updateState();
}

AttachTcfDialog::~AttachTcfDialog()
{
    delete m_ui;
}

void AttachTcfDialog::setRemoteChannel(const QString &channel)
{
    m_ui->channelLineEdit->setText(channel);
}

QString AttachTcfDialog::remoteChannel() const
{
    return m_ui->channelLineEdit->text();
}

void AttachTcfDialog::setRemoteArchitectures(const QStringList &list)
{
    m_ui->architectureComboBox->clear();
    if (!list.isEmpty()) {
        m_ui->architectureComboBox->insertItems(0, list);
        m_ui->architectureComboBox->setCurrentIndex(0);
    }
}

void AttachTcfDialog::setRemoteArchitecture(const QString &arch)
{
    int index = m_ui->architectureComboBox->findText(arch);
    if (index != -1)
        m_ui->architectureComboBox->setCurrentIndex(index);
}

QString AttachTcfDialog::remoteArchitecture() const
{
    int index = m_ui->architectureComboBox->currentIndex();
    return m_ui->architectureComboBox->itemText(index);
}

void AttachTcfDialog::setServerStartScript(const QString &scriptName)
{
    m_ui->serverStartScript->setPath(scriptName);
}

QString AttachTcfDialog::serverStartScript() const
{
    return m_ui->serverStartScript->path();
}

void AttachTcfDialog::setUseServerStartScript(bool on)
{
    m_ui->useServerStartScriptCheckBox->setChecked(on);
}

bool AttachTcfDialog::useServerStartScript() const
{
    return m_ui->useServerStartScriptCheckBox->isChecked();
}

void AttachTcfDialog::updateState()
{
    bool enabled = m_ui->useServerStartScriptCheckBox->isChecked();
    m_ui->serverStartScriptLabel->setEnabled(enabled);
    m_ui->serverStartScript->setEnabled(enabled);
}


///////////////////////////////////////////////////////////////////////
//
// StartExternalDialog
//
///////////////////////////////////////////////////////////////////////

StartExternalDialog::StartExternalDialog(QWidget *parent)
  : QDialog(parent), m_ui(new Ui::StartExternalDialog)
{
    m_ui->setupUi(this);
    m_ui->execFile->setExpectedKind(Core::Utils::PathChooser::File);
    m_ui->execFile->setPromptDialogTitle(tr("Select Executable"));
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    //execLabel->setHidden(false);
    //execEdit->setHidden(false);
    //browseButton->setHidden(false);

    m_ui->execLabel->setText(tr("Executable:"));
    m_ui->argLabel->setText(tr("Arguments:"));

    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

StartExternalDialog::~StartExternalDialog()
{
    delete m_ui;
}

void StartExternalDialog::setExecutableFile(const QString &str)
{
    m_ui->execFile->setPath(str);
}

QString StartExternalDialog::executableFile() const
{
    return m_ui->execFile->path();
}

void StartExternalDialog::setExecutableArguments(const QString &str)
{
    m_ui->argsEdit->setText(str);
}

QString StartExternalDialog::executableArguments() const
{
    return m_ui->argsEdit->text();
}

bool StartExternalDialog::breakAtMain() const
{
    return m_ui->checkBoxBreakAtMain->isChecked();
}



///////////////////////////////////////////////////////////////////////
//
// StartRemoteDialog
//
///////////////////////////////////////////////////////////////////////

StartRemoteDialog::StartRemoteDialog(QWidget *parent)
  : QDialog(parent),
    m_ui(new Ui::StartRemoteDialog)
{
    m_ui->setupUi(this);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    m_ui->serverStartScript->setExpectedKind(Core::Utils::PathChooser::File);
    m_ui->serverStartScript->setPromptDialogTitle(tr("Select Executable"));

    connect(m_ui->useServerStartScriptCheckBox, SIGNAL(toggled(bool)), 
        this, SLOT(updateState()));
    
    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    updateState();
}

StartRemoteDialog::~StartRemoteDialog()
{
    delete m_ui;
}

void StartRemoteDialog::setRemoteChannel(const QString &channel)
{
    m_ui->channelLineEdit->setText(channel);
}

QString StartRemoteDialog::remoteChannel() const
{
    return m_ui->channelLineEdit->text();
}

void StartRemoteDialog::setRemoteArchitectures(const QStringList &list)
{
    m_ui->architectureComboBox->clear();
    if (!list.isEmpty()) {
        m_ui->architectureComboBox->insertItems(0, list);
        m_ui->architectureComboBox->setCurrentIndex(0);
    }
}

void StartRemoteDialog::setRemoteArchitecture(const QString &arch)
{
    int index = m_ui->architectureComboBox->findText(arch);
    if (index != -1)
        m_ui->architectureComboBox->setCurrentIndex(index);
}

QString StartRemoteDialog::remoteArchitecture() const
{
    int index = m_ui->architectureComboBox->currentIndex();
    return m_ui->architectureComboBox->itemText(index);
}

void StartRemoteDialog::setServerStartScript(const QString &scriptName)
{
    m_ui->serverStartScript->setPath(scriptName);
}

QString StartRemoteDialog::serverStartScript() const
{
    return m_ui->serverStartScript->path();
}

void StartRemoteDialog::setUseServerStartScript(bool on)
{
    m_ui->useServerStartScriptCheckBox->setChecked(on);
}

bool StartRemoteDialog::useServerStartScript() const
{
    return m_ui->useServerStartScriptCheckBox->isChecked();
}

void StartRemoteDialog::updateState()
{
    bool enabled = m_ui->useServerStartScriptCheckBox->isChecked();
    m_ui->serverStartScriptLabel->setEnabled(enabled);
    m_ui->serverStartScript->setEnabled(enabled);
}

AddressDialog::AddressDialog(QWidget *parent) :
        QDialog(parent),
        m_lineEdit(new QLineEdit),
        m_box(new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel))
{
    setWindowTitle(tr("Select start address"));
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

} // namespace Internal
} // namespace Debugger
