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

#include "deviceprocessesdialog.h"
#include "deviceprocesslist.h"
#include <projectexplorer/kitchooser.h>
#include <projectexplorer/kitinformation.h>

#include <utils/fancylineedit.h>
#include <utils/itemviews.h>
#include <utils/qtcassert.h>

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QTextBrowser>
#include <QVBoxLayout>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// ProcessListFilterModel
//
///////////////////////////////////////////////////////////////////////

class ProcessListFilterModel : public QSortFilterProxyModel
{
public:
    ProcessListFilterModel();
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
};

ProcessListFilterModel::ProcessListFilterModel()
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setDynamicSortFilter(true);
    setFilterKeyColumn(-1);
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

///////////////////////////////////////////////////////////////////////
//
// DeviceProcessesDialogPrivate
//
///////////////////////////////////////////////////////////////////////

class DeviceProcessesDialogPrivate : public QObject
{
    Q_OBJECT

public:
    DeviceProcessesDialogPrivate(KitChooser *chooser, QWidget *parent);

public slots:
    void setDevice(const IDevice::ConstPtr &device);
    void updateProcessList();
    void updateDevice();
    void killProcess();
    void handleRemoteError(const QString &errorMsg);
    void handleProcessListUpdated();
    void handleProcessKilled();
    void updateButtons();
    DeviceProcessItem selectedProcess() const;

public:
    QWidget *q;
    DeviceProcessList *processList;
    ProcessListFilterModel proxyModel;
    QLabel *kitLabel;
    KitChooser *kitChooser;

    TreeView *procView;
    QTextBrowser *errorText;
    FancyLineEdit *processFilterLineEdit;
    QPushButton *updateListButton;
    QPushButton *killProcessButton;
    QPushButton *acceptButton;
    QDialogButtonBox *buttonBox;
};

DeviceProcessesDialogPrivate::DeviceProcessesDialogPrivate(KitChooser *chooser, QWidget *parent)
    : q(parent)
    , kitLabel(new QLabel(DeviceProcessesDialog::tr("Kit:"), parent))
    , kitChooser(chooser)
    , acceptButton(0)
    , buttonBox(new QDialogButtonBox(parent))
{
    q->setWindowTitle(DeviceProcessesDialog::tr("List of Processes"));
    q->setWindowFlags(q->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    q->setMinimumHeight(500);

    processList = 0;

    processFilterLineEdit = new FancyLineEdit(q);
    processFilterLineEdit->setPlaceholderText(DeviceProcessesDialog::tr("Filter"));
    processFilterLineEdit->setFocus(Qt::TabFocusReason);
    processFilterLineEdit->setHistoryCompleter(QLatin1String("DeviceProcessDialogFilter"),
        true /*restoreLastItemFromHistory*/);
    processFilterLineEdit->setFiltering(true);

    kitChooser->populate();

    procView = new TreeView(q);
    procView->setModel(&proxyModel);
    procView->setSelectionBehavior(QAbstractItemView::SelectRows);
    procView->setSelectionMode(QAbstractItemView::SingleSelection);
    procView->setUniformRowHeights(true);
    procView->setRootIsDecorated(false);
    procView->setAlternatingRowColors(true);
    procView->setSortingEnabled(true);
    procView->header()->setDefaultSectionSize(100);
    procView->header()->setStretchLastSection(true);
    procView->sortByColumn(1, Qt::AscendingOrder);
    procView->setActivationMode(DoubleClickActivation);

    errorText = new QTextBrowser(q);

    updateListButton = new QPushButton(DeviceProcessesDialog::tr("&Update List"), q);
    killProcessButton = new QPushButton(DeviceProcessesDialog::tr("&Kill Process"), q);

    buttonBox->addButton(updateListButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(killProcessButton, QDialogButtonBox::ActionRole);

    QFormLayout *leftColumn = new QFormLayout();
    leftColumn->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    leftColumn->addRow(kitLabel, kitChooser);
    leftColumn->addRow(DeviceProcessesDialog::tr("&Filter:"), processFilterLineEdit);

//    QVBoxLayout *rightColumn = new QVBoxLayout();
//    rightColumn->addWidget(updateListButton);
//    rightColumn->addWidget(killProcessButton);
//    rightColumn->addStretch();

//    QHBoxLayout *horizontalLayout = new QHBoxLayout();
//    horizontalLayout->addLayout(leftColumn);
//    horizontalLayout->addLayout(rightColumn);

    QVBoxLayout *mainLayout = new QVBoxLayout(q);
    mainLayout->addLayout(leftColumn);
    mainLayout->addWidget(procView);
    mainLayout->addWidget(errorText);
    mainLayout->addWidget(buttonBox);

//    QFrame *line = new QFrame(this);
//    line->setFrameShape(QFrame::HLine);
//    line->setFrameShadow(QFrame::Sunken);

    proxyModel.setFilterRegExp(processFilterLineEdit->text());

    connect(processFilterLineEdit, SIGNAL(textChanged(QString)),
        &proxyModel, SLOT(setFilterRegExp(QString)));
    connect(procView->selectionModel(),
        SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
        SLOT(updateButtons()));
    connect(updateListButton, SIGNAL(clicked()), SLOT(updateProcessList()));
    connect(kitChooser, SIGNAL(currentIndexChanged(int)), SLOT(updateDevice()));
    connect(killProcessButton, SIGNAL(clicked()), SLOT(killProcess()));
    connect(&proxyModel, SIGNAL(layoutChanged()), SLOT(handleProcessListUpdated()));
    connect(buttonBox, SIGNAL(accepted()), q, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), q, SLOT(reject()));

    QWidget::setTabOrder(kitChooser, processFilterLineEdit);
    QWidget::setTabOrder(processFilterLineEdit, procView);
    QWidget::setTabOrder(procView, buttonBox);
}

void DeviceProcessesDialogPrivate::setDevice(const IDevice::ConstPtr &device)
{
    delete processList;
    processList = 0;
    proxyModel.setSourceModel(0);
    if (!device)
        return;

    processList = device->createProcessListModel();
    QTC_ASSERT(processList, return);
    proxyModel.setSourceModel(processList);

    connect(processList, SIGNAL(error(QString)),
        SLOT(handleRemoteError(QString)));
    connect(processList, SIGNAL(processListUpdated()),
        SLOT(handleProcessListUpdated()));
    connect(processList, SIGNAL(processKilled()),
        SLOT(handleProcessKilled()), Qt::QueuedConnection);

    updateButtons();
    updateProcessList();
}

void DeviceProcessesDialogPrivate::handleRemoteError(const QString &errorMsg)
{
    QMessageBox::critical(q, tr("Remote Error"), errorMsg);
    updateListButton->setEnabled(true);
    updateButtons();
}

void DeviceProcessesDialogPrivate::handleProcessListUpdated()
{
    updateListButton->setEnabled(true);
    procView->resizeColumnToContents(0);
    procView->resizeColumnToContents(1);
    updateButtons();
}

void DeviceProcessesDialogPrivate::updateProcessList()
{
    updateListButton->setEnabled(false);
    killProcessButton->setEnabled(false);
    if (processList)
        processList->update();
}

void DeviceProcessesDialogPrivate::killProcess()
{
    const QModelIndexList indexes = procView->selectionModel()->selectedIndexes();
    if (indexes.empty() || !processList)
        return;
    updateListButton->setEnabled(false);
    killProcessButton->setEnabled(false);
    processList->killProcess(proxyModel.mapToSource(indexes.first()).row());
}

void DeviceProcessesDialogPrivate::updateDevice()
{
    setDevice(DeviceKitInformation::device(kitChooser->currentKit()));
}

void DeviceProcessesDialogPrivate::handleProcessKilled()
{
    updateProcessList();
}

void DeviceProcessesDialogPrivate::updateButtons()
{
    const bool hasSelection = procView->selectionModel()->hasSelection();
    if (acceptButton)
        acceptButton->setEnabled(hasSelection);
    killProcessButton->setEnabled(hasSelection);
    errorText->setVisible(!errorText->document()->isEmpty());
}

DeviceProcessItem DeviceProcessesDialogPrivate::selectedProcess() const
{
    const QModelIndexList indexes = procView->selectionModel()->selectedIndexes();
    if (indexes.empty() || !processList)
        return DeviceProcessItem();
    return processList->at(proxyModel.mapToSource(indexes.first()).row());
}

} // namespace Internal

///////////////////////////////////////////////////////////////////////
//
// DeviceProcessesDialog
//
///////////////////////////////////////////////////////////////////////

/*!
     \class ProjectExplorer::DeviceProcessesDialog

     \brief The DeviceProcessesDialog class shows a list of processes.

     The dialog can be used as a:
     \list
     \li Non-modal dialog showing a list of processes. Call addCloseButton()
         to add a \gui Close button.
     \li Modal dialog with an \gui Accept button to select a process. Call
         addAcceptButton() passing the label text. This will create a
         \gui Cancel button as well.
     \endlist
*/

DeviceProcessesDialog::DeviceProcessesDialog(QWidget *parent)
    : QDialog(parent), d(new Internal::DeviceProcessesDialogPrivate(new KitChooser(this), this))
{
}

DeviceProcessesDialog::DeviceProcessesDialog(KitChooser *chooser, QWidget *parent)
    : QDialog(parent), d(new Internal::DeviceProcessesDialogPrivate(chooser, this))
{
}

DeviceProcessesDialog::~DeviceProcessesDialog()
{
    delete d;
}

void DeviceProcessesDialog::addAcceptButton(const QString &label)
{
    d->acceptButton = new QPushButton(label);
    d->buttonBox->addButton(d->acceptButton, QDialogButtonBox::AcceptRole);
    connect(d->procView, SIGNAL(activated(QModelIndex)),
            d->acceptButton, SLOT(click()));
    d->buttonBox->addButton(QDialogButtonBox::Cancel);
}

void DeviceProcessesDialog::addCloseButton()
{
    d->buttonBox->addButton(QDialogButtonBox::Close);
}

void DeviceProcessesDialog::setKitVisible(bool v)
{
    d->kitLabel->setVisible(v);
    d->kitChooser->setVisible(v);
}

void DeviceProcessesDialog::setDevice(const IDevice::ConstPtr &device)
{
    setKitVisible(false);
    d->setDevice(device);
}

void DeviceProcessesDialog::showAllDevices()
{
    setKitVisible(true);
    d->updateDevice();
}

DeviceProcessItem DeviceProcessesDialog::currentProcess() const
{
    return d->selectedProcess();
}

KitChooser *DeviceProcessesDialog::kitChooser() const
{
    return d->kitChooser;
}

void DeviceProcessesDialog::logMessage(const QString &line)
{
    d->errorText->setVisible(true);
    d->errorText->append(line);
}

} // namespace ProjectExplorer

#include "deviceprocessesdialog.moc"
