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

#include "loadremotecoredialog.h"

#include "debuggerconstants.h"
#include "debuggercore.h"
#include "debuggerstartparameters.h"

#include <coreplugin/icore.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/profilechooser.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/devicemanagermodel.h>
#include <ssh/sshconnection.h>
#include <ssh/sshremoteprocessrunner.h>
#include <ssh/sftpfilesystemmodel.h>
#include <utils/historycompleter.h>
#include <utils/pathchooser.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QRegExp>

#include <QButtonGroup>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QStandardItemModel>
#include <QTableView>
#include <QTextBrowser>
#include <QTreeView>

using namespace Core;
using namespace ProjectExplorer;
using namespace QSsh;
using namespace Utils;

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// LoadRemoteCoreFileDialog
//
///////////////////////////////////////////////////////////////////////

class LoadRemoteCoreFileDialogPrivate
{
public:
    DeviceManagerModel *deviceManagerModel;

    QComboBox *deviceComboBox;
    QTreeView *fileSystemView;
    QPushButton *loadCoreFileButton;
    QTextBrowser *textBrowser;
    QPushButton *closeButton;
    //PathChooser *sysrootPathChooser;
    ProfileChooser *profileChooser;

    QSettings *settings;
    QString remoteCommandLine;
    QString remoteExecutable;
    QString localCoreFile;

    SftpFileSystemModel *fileSystemModel;
    SftpJobId sftpJobId;
};

LoadRemoteCoreFileDialog::LoadRemoteCoreFileDialog(QWidget *parent)
    : QDialog(parent), d(new LoadRemoteCoreFileDialogPrivate)
{
    setWindowTitle(tr("Select Remote Core File"));

    d->settings = ICore::settings();

    d->deviceComboBox = new QComboBox(this);

    d->profileChooser = new ProfileChooser(this);
    d->fileSystemModel = new SftpFileSystemModel(this);

    //executablePathChooser = new PathChooser(q);
    //executablePathChooser->setExpectedKind(PathChooser::File);
    //executablePathChooser->setPromptDialogTitle(tr("Select Executable"));
    //executablePathChooser->setPath(settings->value(LastLocalExecutable).toString());

    d->fileSystemView = new QTreeView(this);
    d->fileSystemView->setSortingEnabled(true);
    d->fileSystemView->header()->setDefaultSectionSize(100);
    d->fileSystemView->header()->setStretchLastSection(true);
    d->fileSystemView->setSelectionMode(QAbstractItemView::SingleSelection);
    d->fileSystemView->setModel(d->fileSystemModel);

    d->loadCoreFileButton = new QPushButton(this);
    d->loadCoreFileButton->setText(tr("&Load Selected Core File"));

    d->closeButton = new QPushButton(this);
    d->closeButton->setText(tr("Close"));

    d->textBrowser = new QTextBrowser(this);
    d->textBrowser->setEnabled(false);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(tr("Device:"), d->deviceComboBox);
    formLayout->addRow(tr("Target:"), d->profileChooser);

    QHBoxLayout *horizontalLayout2 = new QHBoxLayout();
    horizontalLayout2->addStretch(1);
    horizontalLayout2->addWidget(d->loadCoreFileButton);
    horizontalLayout2->addWidget(d->closeButton);

    formLayout->addRow(d->fileSystemView);
    formLayout->addRow(d->textBrowser);
    formLayout->addRow(horizontalLayout2);
    setLayout(formLayout);

    d->deviceManagerModel = new DeviceManagerModel(DeviceManager::instance(), this);

    QObject::connect(d->closeButton, SIGNAL(clicked()), this, SLOT(reject()));

    d->deviceComboBox->setModel(d->deviceManagerModel);
    d->deviceComboBox->setCurrentIndex(d->settings->value(QLatin1String("LastDevice")).toInt());

    if (d->deviceManagerModel->rowCount() == 0) {
        d->fileSystemView->setEnabled(false);
    } else {
        d->fileSystemView->setSelectionBehavior(QAbstractItemView::SelectRows);
        connect(d->fileSystemView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            SLOT(updateButtons()));
        connect(d->profileChooser, SIGNAL(activated(int)), SLOT(updateButtons()));
        connect(d->loadCoreFileButton, SIGNAL(clicked()), SLOT(selectCoreFile()));
        connect(d->deviceComboBox, SIGNAL(currentIndexChanged(int)),
            SLOT(attachToDevice(int)));
        updateButtons();
        attachToDevice(d->deviceComboBox->currentIndex());
    }
}

LoadRemoteCoreFileDialog::~LoadRemoteCoreFileDialog()
{
    delete d;
}

void LoadRemoteCoreFileDialog::setLocalCoreFileName(const QString &fileName)
{
    d->localCoreFile = fileName;
}

QString LoadRemoteCoreFileDialog::localCoreFileName() const
{
    return d->localCoreFile;
}

Id LoadRemoteCoreFileDialog::profileId() const
{
    return d->profileChooser->currentProfileId();
}

void LoadRemoteCoreFileDialog::attachToDevice(int modelIndex)
{
    IDevice::ConstPtr device = d->deviceManagerModel->device(modelIndex);
    if (!device)
        return;
    connect(d->fileSystemModel, SIGNAL(sftpOperationFailed(QString)),
            SLOT(handleSftpOperationFailed(QString)));
    connect(d->fileSystemModel, SIGNAL(connectionError(QString)),
            SLOT(handleConnectionError(QString)));
    d->fileSystemModel->setSshConnection(device->sshParameters());
    //d->fileSystemModel->setRootDirectory(d->settings->value(QLatin1String("LastSftpRoot")).toString());
}

void LoadRemoteCoreFileDialog::handleSftpOperationFailed(const QString &errorMessage)
{
    d->textBrowser->append(errorMessage);
    //reject();
}

void LoadRemoteCoreFileDialog::handleConnectionError(const QString &errorMessage)
{
    d->textBrowser->append(errorMessage);
    //reject();
}

void LoadRemoteCoreFileDialog::handleSftpOperationFinished(QSsh::SftpJobId, const QString &error)
{
    if (error.isEmpty()) {
        d->textBrowser->append(tr("Download of Core File succeeded."));
        accept();
    } else {
        d->textBrowser->append(error);
        //reject();
    }
}

void LoadRemoteCoreFileDialog::handleRemoteError(const QString &errorMessage)
{
    d->textBrowser->append(errorMessage);
    updateButtons();
}

void LoadRemoteCoreFileDialog::selectCoreFile()
{
    const QModelIndexList &indexes =
            d->fileSystemView->selectionModel()->selectedIndexes();
    if (indexes.empty())
        return;

    d->loadCoreFileButton->setEnabled(false);
    d->fileSystemView->setEnabled(false);

    d->settings->setValue(QLatin1String("LastProfile"),
        d->profileChooser->currentProfileId().toString());
    d->settings->setValue(QLatin1String("LastDevice"), d->deviceComboBox->currentIndex());
    d->settings->setValue(QLatin1String("LastSftpRoot"), d->fileSystemModel->rootDirectory());

    connect(d->fileSystemModel, SIGNAL(sftpOperationFinished(QSsh::SftpJobId,QString)),
            SLOT(handleSftpOperationFinished(QSsh::SftpJobId,QString)));
    d->sftpJobId = d->fileSystemModel->downloadFile(indexes.at(0), d->localCoreFile);
}

void LoadRemoteCoreFileDialog::updateButtons()
{
    d->loadCoreFileButton->setEnabled(d->fileSystemView->selectionModel()->hasSelection());
}

} // namespace Internal
} // namespace Debugger
