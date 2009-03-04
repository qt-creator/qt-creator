/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "debuggerdialogs.h"

#include "ui_attachcoredialog.h"
#include "ui_attachexternaldialog.h"
#include "ui_attachremotedialog.h"
#include "ui_startexternaldialog.h"

#ifdef Q_OS_WIN
#  include "dbgwinutils.h"
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

using namespace Debugger::Internal;

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
    m_ui->coreFileName->setPromptDialogTitle(tr("Select Executable"));

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
// process model helpers
//
///////////////////////////////////////////////////////////////////////

static QStandardItemModel *createProcessModel(QObject *parent)
{
    QStandardItemModel *rc = new QStandardItemModel(parent);
    QStringList columns;
    columns << AttachExternalDialog::tr("Process ID")
            << AttachExternalDialog::tr("Name")
            << AttachExternalDialog::tr("State");
    rc->setHorizontalHeaderLabels(columns);
    return rc;
}

static bool isUnixProcessId(const QString &procname)
{
    for (int i = 0; i != procname.size(); ++i)
        if (!procname.at(i).isDigit())
            return false;
    return true;
}

bool operator<(const ProcData &p1, const ProcData &p2)
{
    return p1.name < p2.name;
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
        file.open(QIODevice::ReadOnly);
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

static inline QStandardItem *createStandardItem(const QString &text, 
                                                bool enabled)
{    
    QStandardItem *rc = new QStandardItem(text);
    rc->setEnabled(enabled);
    return rc;
}

// Populate a standard item model with a list
// of processes and gray out the excludePid.
static void populateProcessModel(QStandardItemModel *model, 
                                 const QString &excludePid = QString())
{
#ifdef Q_OS_WIN
    QList<ProcData> processes = winProcessList();
#else
    QList<ProcData> processes = unixProcessList();
#endif
    qStableSort(processes);

    if (const int rowCount = model->rowCount())
        model->removeRows(0, rowCount);
    
    QStandardItem *root  = model->invisibleRootItem();
    foreach(const ProcData &proc, processes) {
        const bool enabled = proc.ppid != excludePid;
        QList<QStandardItem *> row;
        row.append(createStandardItem(proc.ppid, enabled));
        row.append(createStandardItem(proc.name, enabled));
        if (!proc.image.isEmpty())
            row.back()->setToolTip(proc.image);
        row.append(createStandardItem(proc.state, enabled));
        root->appendRow(row);
    }
}

///////////////////////////////////////////////////////////////////////
//
// AttachExternalDialog
//
///////////////////////////////////////////////////////////////////////

AttachExternalDialog::AttachExternalDialog(QWidget *parent) :
        QDialog(parent),
        m_selfPid(QString::number(QCoreApplication::applicationPid())),
        m_ui(new Ui::AttachExternalDialog),
        m_model(createProcessModel(this)),
        m_proxyModel(new QSortFilterProxyModel(this))
{
    m_ui->setupUi(this);
    okButton()->setDefault(true);
    okButton()->setEnabled(false);

    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(1);
    m_ui->procView->setModel(m_proxyModel);
    m_ui->procView->setSortingEnabled(true);

    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    QPushButton *refreshButton = new QPushButton(tr("Refresh"));
    connect(refreshButton, SIGNAL(clicked()), this, SLOT(rebuildProcessList()));
    m_ui->buttonBox->addButton(refreshButton, QDialogButtonBox::ActionRole);

    connect(m_ui->procView, SIGNAL(activated(QModelIndex)),
        this, SLOT(procSelected(QModelIndex)));
    connect(m_ui->pidLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(pidChanged(QString)));

    connect(m_ui->filterClearToolButton, SIGNAL(clicked()),
            m_ui->filterLineEdit, SLOT(clear()));
    connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)),
            m_proxyModel, SLOT(setFilterFixedString(QString)));

    rebuildProcessList();
}

AttachExternalDialog::~AttachExternalDialog()
{
    delete m_ui;
}

QPushButton *AttachExternalDialog::okButton() const
{
    return m_ui->buttonBox->button(QDialogButtonBox::Ok);
}

void AttachExternalDialog::rebuildProcessList()
{
    populateProcessModel(m_model, m_selfPid);
    m_ui->procView->expandAll();
    m_ui->procView->resizeColumnToContents(0);
    m_ui->procView->resizeColumnToContents(1);
}

void AttachExternalDialog::procSelected(const QModelIndex &proxyIndex)
{
    const QModelIndex index0 = m_proxyModel->mapToSource(proxyIndex);
    QModelIndex index = index0.sibling(index0.row(), 0);
    if (const QStandardItem *item = m_model->itemFromIndex(index)) {
        m_ui->pidLineEdit->setText(item->text());
        if (okButton()->isEnabled())
            okButton()->animateClick();
    }
}

int AttachExternalDialog::attachPID() const
{
    return m_ui->pidLineEdit->text().toInt();
}

void AttachExternalDialog::pidChanged(const QString &pid)
{
    okButton()->setEnabled(!pid.isEmpty() && pid != m_selfPid);
}

///////////////////////////////////////////////////////////////////////
//
// AttachRemoteDialog
//
///////////////////////////////////////////////////////////////////////

AttachRemoteDialog::AttachRemoteDialog(QWidget *parent, const QString &pid) :
        QDialog(parent),
        m_ui(new Ui::AttachRemoteDialog),
        m_model(createProcessModel(this))
{
    m_ui->setupUi(this);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    m_defaultPID = pid;

    m_ui->procView->setModel(m_model);
    m_ui->procView->setSortingEnabled(true);

    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    connect(m_ui->procView, SIGNAL(activated(QModelIndex)),
        this, SLOT(procSelected(QModelIndex)));

    m_ui->pidLineEdit->setText(m_defaultPID);
    rebuildProcessList();
}

AttachRemoteDialog::~AttachRemoteDialog()
{
    delete m_ui;
}

void AttachRemoteDialog::rebuildProcessList()
{
    populateProcessModel(m_model);
    m_ui->procView->expandAll();
    m_ui->procView->resizeColumnToContents(0);
    m_ui->procView->resizeColumnToContents(1);
}

void AttachRemoteDialog::procSelected(const QModelIndex &index0)
{
    QModelIndex index = index0.sibling(index0.row(), 0);
    QStandardItem *item = m_model->itemFromIndex(index);
    if (!item)
        return;
    m_ui->pidLineEdit->setText(item->text());
    accept();
}

int AttachRemoteDialog::attachPID() const
{
    return m_ui->pidLineEdit->text().toInt();
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

void StartExternalDialog::setExecutableArguments(const QString &str)
{
    m_ui->argsEdit->setText(str);
}

QString StartExternalDialog::executableFile() const
{
    return m_ui->execFile->path();
}

QString StartExternalDialog::executableArguments() const
{
    return m_ui->argsEdit->text();
}
