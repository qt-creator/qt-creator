/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "iossettingswidget.h"
#include "ui_iossettingswidget.h"

#include "createsimulatordialog.h"
#include "iosconfigurations.h"
#include "simulatorinfomodel.h"
#include "simulatoroperationdialog.h"

#include <utils/algorithm.h>
#include <utils/runextensions.h>

#include <QDateTime>
#include <QInputDialog>
#include <QMessageBox>
#include <QPointer>
#include <QSortFilterProxyModel>

static const int simStartWarnCount = 4;

namespace Ios {
namespace Internal {

using namespace std::placeholders;

static SimulatorInfoList selectedSimulators(const QTreeView *deviceTreeView)
{
    SimulatorInfoList list;
    QItemSelectionModel *selectionModel = deviceTreeView->selectionModel();
    for (QModelIndex index: selectionModel->selectedRows())
        list << deviceTreeView->model()->data(index, Qt::UserRole).value<SimulatorInfo>();
    return list;
}

static void onSimOperation(const SimulatorInfo &simInfo, SimulatorOperationDialog* dlg,
                           const QString &contextStr, const SimulatorControl::ResponseData &response)
{
    dlg->addMessage(simInfo, response, contextStr);
}

IosSettingsWidget::IosSettingsWidget(QWidget *parent)
    : QWidget(parent),
      m_ui(new Ui::IosSettingsWidget),
      m_simControl(new SimulatorControl(this))
{
    m_ui->setupUi(this);
    auto proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(new SimulatorInfoModel(this));
    m_ui->deviceView->setModel(proxyModel);
    m_ui->deviceView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_ui->pathWidget->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_ui->pathWidget->lineEdit()->setReadOnly(true);
    m_ui->pathWidget->setFileName(IosConfigurations::screenshotDir());
    m_ui->pathWidget->addButton(tr("Screenshot"), this,
                                std::bind(&IosSettingsWidget::onScreenshot, this));

    m_ui->deviceAskCheckBox->setChecked(!IosConfigurations::ignoreAllDevices());

    connect(m_ui->startButton, &QPushButton::clicked, this, &IosSettingsWidget::onStart);
    connect(m_ui->createButton, &QPushButton::clicked, this, &IosSettingsWidget::onCreate);
    connect(m_ui->renameButton, &QPushButton::clicked, this, &IosSettingsWidget::onRename);
    connect(m_ui->resetButton, &QPushButton::clicked, this, &IosSettingsWidget::onReset);
    connect(m_ui->deleteButton, &QPushButton::clicked, this, &IosSettingsWidget::onDelete);

    connect(m_ui->deviceView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &IosSettingsWidget::onSelectionChanged);
}

IosSettingsWidget::~IosSettingsWidget()
{
    delete m_ui;
}

/*!
    Called on start button click. Selected simulator devices are started. Multiple devices can be
    started simultaneously provided they in shutdown state.
 */
void IosSettingsWidget::onStart()
{
    const SimulatorInfoList simulatorInfoList = selectedSimulators(m_ui->deviceView);
    if (simulatorInfoList.isEmpty())
        return;

    if (simulatorInfoList.count() > simStartWarnCount) {
        const QString message = tr("You are trying to launch %n simulators simultaneously. This "
                                   "will take significant system resources. Do you really want to "
                                   "continue?", "", simulatorInfoList.count());
        const int buttonCode = QMessageBox::warning(this, tr("Simulator Start"), message,
                                                    QMessageBox::Ok | QMessageBox::Abort,
                                                    QMessageBox::Abort);

        if (buttonCode == QMessageBox::Abort)
            return;
    }

    QPointer<SimulatorOperationDialog> statusDialog = new SimulatorOperationDialog(this);
    statusDialog->setAttribute(Qt::WA_DeleteOnClose);
    statusDialog->addMessage(tr("Starting simulator devices...", "", simulatorInfoList.count()),
                             Utils::NormalMessageFormat);

    QList<QFuture<void>> futureList;
    foreach (const SimulatorInfo &info, simulatorInfoList) {
        if (!info.isShutdown()) {
            statusDialog->addMessage(tr("Cannot start simulator (%1, %2) in current state: %3")
                                    .arg(info.name).arg(info.runtimeName).arg(info.state),
                                    Utils::StdErrFormat);
        } else {
            futureList << Utils::onResultReady(m_simControl->startSimulator(info.identifier),
                                               std::bind(onSimOperation, info, statusDialog,
                                                         tr("simulator start"), _1));
        }
    }

    statusDialog->addFutures(futureList);
    statusDialog->exec(); // Modal dialog returns only when all the operations are done or cancelled.
}

/*!
    Called on create button click. User is presented with the create simulator dialog and with the
    selected options a new device is created.
 */
void IosSettingsWidget::onCreate()
{
    QPointer<SimulatorOperationDialog> statusDialog = new SimulatorOperationDialog(this);
    statusDialog->setAttribute(Qt::WA_DeleteOnClose);
    statusDialog->addMessage(tr("Creating simulator device..."), Utils::NormalMessageFormat);
    const auto onSimulatorCreate = [statusDialog](const QString &name,
            const SimulatorControl::ResponseData &response) {
        if (response.success) {
            statusDialog->addMessage(tr("Simulator device (%1) created.\nUDID: %2")
                                    .arg(name).arg(response.simUdid), Utils::StdOutFormat);
        } else {
            statusDialog->addMessage(tr("Simulator device (%1) creation failed.\nError: %2").
                                    arg(name).arg(QString::fromUtf8(response.commandOutput)),
                                    Utils::StdErrFormat);
        }
    };

    CreateSimulatorDialog createDialog(this);
    if (createDialog.exec() == QDialog::Accepted) {
        QFuture<void> f = Utils::onResultReady(
                            m_simControl->createSimulator(
                                createDialog.name(),
                                createDialog.deviceType(),
                                createDialog.runtime()),
                            std::bind(onSimulatorCreate, createDialog.name(), _1));
        statusDialog->addFutures({ f });
        statusDialog->exec(); // Modal dialog returns only when all the operations are done or cancelled.
    }
}

/*!
    Called on reset button click. Contents and settings of the selected devices are erased. Multiple
    devices can be erased simultaneously provided they in shutdown state.
 */
void IosSettingsWidget::onReset()
{
    const SimulatorInfoList simulatorInfoList = selectedSimulators(m_ui->deviceView);
    if (simulatorInfoList.isEmpty())
        return;

    const int userInput = QMessageBox::question(this, tr("Reset"),
                                          tr("Do you really want to reset the contents and settings"
                                             " of the selected devices?", "",
                                             simulatorInfoList.count()));
    if (userInput == QMessageBox::No)
        return;

    QPointer<SimulatorOperationDialog> statusDialog = new SimulatorOperationDialog(this);
    statusDialog->setAttribute(Qt::WA_DeleteOnClose);
    statusDialog->addMessage(tr("Resetting contents and settings..."), Utils::NormalMessageFormat);

    QList<QFuture<void>> futureList;
    foreach (const SimulatorInfo &info, simulatorInfoList) {
        futureList << Utils::onResultReady(m_simControl->resetSimulator(info.identifier),
                                           std::bind(onSimOperation, info, statusDialog,
                                                     tr("simulator reset"), _1));
    }

    statusDialog->addFutures(futureList);
    statusDialog->exec(); // Modal dialog returns only when all the operations are done or cancelled.
}

/*!
    Called on rename button click. Selected device is renamed. Only one device can be renamed at a
    time. Rename button is disabled on multi-selection.
 */
void IosSettingsWidget::onRename()
{
    const SimulatorInfoList simulatorInfoList = selectedSimulators(m_ui->deviceView);
    if (simulatorInfoList.isEmpty() || simulatorInfoList.count() > 1)
        return;

    const SimulatorInfo &simInfo = simulatorInfoList.at(0);
    const QString newName = QInputDialog::getText(this, tr("Rename %1").arg(simInfo.name),
                                            tr("Enter new name:"));
    if (newName.isEmpty())
        return;

    QPointer<SimulatorOperationDialog> statusDialog = new SimulatorOperationDialog(this);
    statusDialog->setAttribute(Qt::WA_DeleteOnClose);
    statusDialog->addMessage(tr("Renaming simulator device..."), Utils::NormalMessageFormat);
    QFuture<void> f = Utils::onResultReady(m_simControl->renameSimulator(simInfo.identifier, newName),
                                           std::bind(onSimOperation, simInfo, statusDialog,
                                                     tr("simulator rename"), _1));
    statusDialog->addFutures({f});
    statusDialog->exec(); // Modal dialog returns only when all the operations are done or cancelled.
}

/*!
    Called on delete button click. Selected devices are deleted. Multiple devices can be deleted
    simultaneously provided they in shutdown state.
 */
void IosSettingsWidget::onDelete()
{
    const SimulatorInfoList simulatorInfoList = selectedSimulators(m_ui->deviceView);
    if (simulatorInfoList.isEmpty())
        return;

    const int userInput = QMessageBox::question(this, tr("Delete Device"),
                                                tr("Do you really want to delete the selected "
                                                   "devices?", "", simulatorInfoList.count()));
    if (userInput == QMessageBox::No)
        return;

    QPointer<SimulatorOperationDialog> statusDialog = new SimulatorOperationDialog(this);
    statusDialog->setAttribute(Qt::WA_DeleteOnClose);
    statusDialog->addMessage(tr("Deleting simulator devices...", "", simulatorInfoList.count()),
                             Utils::NormalMessageFormat);
    QList<QFuture<void>> futureList;
    foreach (const SimulatorInfo &info, simulatorInfoList) {
        futureList << Utils::onResultReady(m_simControl->deleteSimulator(info.identifier),
                                           std::bind(onSimOperation, info, statusDialog,
                                                     tr("simulator delete"), _1));
    }

    statusDialog->addFutures(futureList);
    statusDialog->exec(); // Modal dialog returns only when all the operations are done or cancelled.
}

/*!
    Called on screenshot button click. Screenshot of the selected devices are saved to the selected
    path. Screenshot from multiple devices can be taken simultaneously provided they in booted state.
 */
void IosSettingsWidget::onScreenshot()
{
    const SimulatorInfoList simulatorInfoList = selectedSimulators(m_ui->deviceView);
    if (simulatorInfoList.isEmpty())
        return;

    const auto generatePath = [this](const SimulatorInfo &info) {
        const QString fileName = QString("%1_%2_%3.png").arg(info.name).arg(info.runtimeName)
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss-z")).replace(' ', '_');
        return m_ui->pathWidget->fileName().appendPath(fileName).toString();
    };

    QPointer<SimulatorOperationDialog> statusDialog = new SimulatorOperationDialog(this);
    statusDialog->setAttribute(Qt::WA_DeleteOnClose);
    statusDialog->addMessage(tr("Capturing screenshots from devices...", "",
                                simulatorInfoList.count()), Utils::NormalMessageFormat);
    QList<QFuture<void>> futureList;
    foreach (const SimulatorInfo &info, simulatorInfoList) {
        futureList << Utils::onResultReady(m_simControl->takeSceenshot(info.identifier,
                                                                         generatePath(info)),
                                           std::bind(onSimOperation, info, statusDialog,
                                                     tr("simulator screenshot"), _1));
    }

    statusDialog->addFutures(futureList);
    statusDialog->exec(); // Modal dialog returns only when all the operations are done or cancelled.
}

void IosSettingsWidget::onSelectionChanged()
{
    const SimulatorInfoList infoList = selectedSimulators(m_ui->deviceView);
    const bool hasRunning = Utils::anyOf(infoList, [](const SimulatorInfo &info) {
        return info.isBooted();
    });
    const bool hasShutdown = Utils::anyOf(infoList, [](const SimulatorInfo &info) {
        return info.isShutdown();
    });
    m_ui->startButton->setEnabled(hasShutdown);
    m_ui->deleteButton->setEnabled(hasShutdown);
    m_ui->resetButton->setEnabled(hasShutdown);
    m_ui->renameButton->setEnabled(infoList.count() == 1 && hasShutdown);
    m_ui->pathWidget->buttonAtIndex(1)->setEnabled(hasRunning); // Screenshot button
}

void IosSettingsWidget::saveSettings()
{
    IosConfigurations::setIgnoreAllDevices(!m_ui->deviceAskCheckBox->isChecked());
    IosConfigurations::setScreenshotDir(m_ui->pathWidget->fileName());
}

} // namespace Internal
} // namespace Ios

