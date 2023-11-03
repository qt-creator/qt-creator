// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iossettingspage.h"

#include "createsimulatordialog.h"
#include "iosconfigurations.h"
#include "iosconstants.h"
#include "iostr.h"
#include "simulatorcontrol.h"
#include "simulatorinfomodel.h"
#include "simulatoroperationdialog.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QCheckBox>
#include <QDateTime>
#include <QGroupBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QTreeView>

using namespace std::placeholders;

namespace Ios::Internal {

class IosSettingsWidget final : public Core::IOptionsPageWidget
{
public:
    IosSettingsWidget();
    ~IosSettingsWidget() final;

private:
    void apply() final;

    void saveSettings();

    void onStart();
    void onCreate();
    void onReset();
    void onRename();
    void onDelete();
    void onScreenshot();
    void onSelectionChanged();

private:
    Utils::PathChooser *m_pathWidget;
    QPushButton *m_startButton;
    QPushButton *m_renameButton;
    QPushButton *m_deleteButton;
    QPushButton *m_resetButton;
    QTreeView *m_deviceView;
    QCheckBox *m_deviceAskCheckBox;
};

const int simStartWarnCount = 4;

static SimulatorInfoList selectedSimulators(const QTreeView *deviceTreeView)
{
    SimulatorInfoList list;
    QItemSelectionModel *selectionModel = deviceTreeView->selectionModel();
    for (QModelIndex index: selectionModel->selectedRows())
        list << deviceTreeView->model()->data(index, Qt::UserRole).value<SimulatorInfo>();
    return list;
}

static void onSimOperation(const SimulatorInfo &simInfo,
                           SimulatorOperationDialog *dlg,
                           const QString &contextStr,
                           const SimulatorControl::Response &response)
{
    dlg->addMessage(simInfo, response, contextStr);
}

IosSettingsWidget::IosSettingsWidget()
{
    setWindowTitle(Tr::tr("iOS Configuration"));

    m_deviceAskCheckBox = new QCheckBox(Tr::tr("Ask about devices not in developer mode"));
    m_deviceAskCheckBox->setChecked(!IosConfigurations::ignoreAllDevices());

    m_renameButton = new QPushButton(Tr::tr("Rename"));
    m_renameButton->setEnabled(false);
    m_renameButton->setToolTip(Tr::tr("Rename a simulator device."));

    m_deleteButton = new QPushButton(Tr::tr("Delete"));
    m_deleteButton->setEnabled(false);
    m_deleteButton->setToolTip(Tr::tr("Delete simulator devices."));

    m_resetButton = new QPushButton(Tr::tr("Reset"));
    m_resetButton->setEnabled(false);
    m_resetButton->setToolTip(Tr::tr("Reset contents and settings of simulator devices."));

    auto createButton = new QPushButton(Tr::tr("Create"));
    createButton->setToolTip(Tr::tr("Create a new simulator device."));

    m_startButton = new QPushButton(Tr::tr("Start"));
    m_startButton->setEnabled(false);
    m_startButton->setToolTip(Tr::tr("Start simulator devices."));

    auto proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(new SimulatorInfoModel(this));

    m_deviceView = new QTreeView;
    m_deviceView->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
    m_deviceView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_deviceView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_deviceView->setSortingEnabled(true);
    m_deviceView->setModel(proxyModel);
    m_deviceView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    m_pathWidget = new Utils::PathChooser;
    m_pathWidget->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_pathWidget->lineEdit()->setReadOnly(true);
    m_pathWidget->setFilePath(IosConfigurations::screenshotDir());
    m_pathWidget->addButton(Tr::tr("Screenshot"), this,
                            std::bind(&IosSettingsWidget::onScreenshot, this));

    using namespace Layouting;
    Column {
        Group {
            title(Tr::tr("Devices")),
            Row { m_deviceAskCheckBox }
        },
        Group {
            title(Tr::tr("Simulator")),
            Column {
                Row {
                    m_deviceView,
                    Column {
                        createButton,
                        st,  // FIXME: Better some fixed space?
                        m_startButton,
                        m_renameButton,
                        m_resetButton,
                        m_deleteButton,
                        st
                    },
                },
                hr,
                Row { Tr::tr("Screenshot directory:"), m_pathWidget }
            }
        }
    }.attachTo(this);

    connect(m_startButton, &QPushButton::clicked, this, &IosSettingsWidget::onStart);
    connect(createButton, &QPushButton::clicked, this, &IosSettingsWidget::onCreate);
    connect(m_renameButton, &QPushButton::clicked, this, &IosSettingsWidget::onRename);
    connect(m_resetButton, &QPushButton::clicked, this, &IosSettingsWidget::onReset);
    connect(m_deleteButton, &QPushButton::clicked, this, &IosSettingsWidget::onDelete);

    connect(m_deviceView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &IosSettingsWidget::onSelectionChanged);
}

IosSettingsWidget::~IosSettingsWidget() = default;

void IosSettingsWidget::apply()
{
    saveSettings();
    IosConfigurations::updateAutomaticKitList();
}

/*!
    Called on start button click. Selected simulator devices are started. Multiple devices can be
    started simultaneously provided they in shutdown state.
 */
void IosSettingsWidget::onStart()
{
    const SimulatorInfoList simulatorInfoList = selectedSimulators(m_deviceView);
    if (simulatorInfoList.isEmpty())
        return;

    if (simulatorInfoList.count() > simStartWarnCount) {
        const QString message =
            Tr::tr("You are trying to launch %n simulators simultaneously. This "
                                       "will take significant system resources. Do you really want to "
                                       "continue?", "", simulatorInfoList.count());
        const int buttonCode = QMessageBox::warning(this, Tr::tr("Simulator Start"), message,
                                                    QMessageBox::Ok | QMessageBox::Abort,
                                                    QMessageBox::Abort);

        if (buttonCode == QMessageBox::Abort)
            return;
    }

    QPointer<SimulatorOperationDialog> statusDialog = new SimulatorOperationDialog(this);
    statusDialog->setAttribute(Qt::WA_DeleteOnClose);
    statusDialog->addMessage(Tr::tr("Starting %n simulator device(s)...", "", simulatorInfoList.count()),
                             Utils::NormalMessageFormat);

    QList<QFuture<void>> futureList;
    for (const SimulatorInfo &info : simulatorInfoList) {
        if (!info.isShutdown()) {
            statusDialog->addMessage(Tr::tr("Cannot start simulator (%1, %2) in current state: %3")
                                    .arg(info.name).arg(info.runtimeName).arg(info.state),
                                    Utils::StdErrFormat);
        } else {
            futureList << QFuture<void>(Utils::onResultReady(
                SimulatorControl::startSimulator(info.identifier), this,
                std::bind(onSimOperation, info, statusDialog, Tr::tr("simulator start"), _1)));
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
    statusDialog->addMessage(Tr::tr("Creating simulator device..."), Utils::NormalMessageFormat);
    const auto onSimulatorCreate = [statusDialog](const QString &name,
                                                  const SimulatorControl::Response &response) {
        if (response) {
            statusDialog->addMessage(Tr::tr("Simulator device (%1) created.\nUDID: %2")
                                         .arg(name)
                                         .arg(response->simUdid),
                                     Utils::StdOutFormat);
        } else {
            statusDialog->addMessage(Tr::tr("Simulator device (%1) creation failed.\nError: %2")
                                         .arg(name)
                                         .arg(response.error()),
                                     Utils::StdErrFormat);
        }
    };

    CreateSimulatorDialog createDialog(this);
    if (createDialog.exec() == QDialog::Accepted) {
        QFuture<void> f = QFuture<void>(Utils::onResultReady(SimulatorControl::createSimulator(
                createDialog.name(), createDialog.deviceType(), createDialog.runtime()),
                this, std::bind(onSimulatorCreate, createDialog.name(), _1)));
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
    const SimulatorInfoList simulatorInfoList = selectedSimulators(m_deviceView);
    if (simulatorInfoList.isEmpty())
        return;

    const int userInput = QMessageBox::question(this, Tr::tr("Reset"),
                                      Tr::tr("Do you really want to reset the contents and settings"
                                             " of the %n selected device(s)?", "",
                                             simulatorInfoList.count()));
    if (userInput == QMessageBox::No)
        return;

    QPointer<SimulatorOperationDialog> statusDialog = new SimulatorOperationDialog(this);
    statusDialog->setAttribute(Qt::WA_DeleteOnClose);
    statusDialog->addMessage(Tr::tr("Resetting contents and settings..."),
                             Utils::NormalMessageFormat);

    QList<QFuture<void>> futureList;
    for (const SimulatorInfo &info : simulatorInfoList) {
        futureList << QFuture<void>(Utils::onResultReady(
            SimulatorControl::resetSimulator(info.identifier), this,
            std::bind(onSimOperation, info, statusDialog, Tr::tr("simulator reset"), _1)));
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
    const SimulatorInfoList simulatorInfoList = selectedSimulators(m_deviceView);
    if (simulatorInfoList.isEmpty() || simulatorInfoList.count() > 1)
        return;

    const SimulatorInfo &simInfo = simulatorInfoList.at(0);
    const QString newName = QInputDialog::getText(this, Tr::tr("Rename %1").arg(simInfo.name),
                                                  Tr::tr("Enter new name:"));
    if (newName.isEmpty())
        return;

    QPointer<SimulatorOperationDialog> statusDialog = new SimulatorOperationDialog(this);
    statusDialog->setAttribute(Qt::WA_DeleteOnClose);
    statusDialog->addMessage(Tr::tr("Renaming simulator device..."), Utils::NormalMessageFormat);
    QFuture<void> f = QFuture<void>(Utils::onResultReady(
        SimulatorControl::renameSimulator(simInfo.identifier, newName), this,
        std::bind(onSimOperation, simInfo, statusDialog, Tr::tr("simulator rename"), _1)));
    statusDialog->addFutures({f});
    statusDialog->exec(); // Modal dialog returns only when all the operations are done or cancelled.
}

/*!
    Called on delete button click. Selected devices are deleted. Multiple devices can be deleted
    simultaneously provided they in shutdown state.
 */
void IosSettingsWidget::onDelete()
{
    const SimulatorInfoList simulatorInfoList = selectedSimulators(m_deviceView);
    if (simulatorInfoList.isEmpty())
        return;

    const int userInput =
        QMessageBox::question(this, Tr::tr("Delete Device"),
                                    Tr::tr("Do you really want to delete the %n selected "
                                           "device(s)?", "", simulatorInfoList.count()));
    if (userInput == QMessageBox::No)
        return;

    QPointer<SimulatorOperationDialog> statusDialog = new SimulatorOperationDialog(this);
    statusDialog->setAttribute(Qt::WA_DeleteOnClose);
    statusDialog->addMessage(Tr::tr("Deleting %n simulator device(s)...", "", simulatorInfoList.count()),
                             Utils::NormalMessageFormat);
    QList<QFuture<void>> futureList;
    for (const SimulatorInfo &info : simulatorInfoList) {
        futureList << QFuture<void>(Utils::onResultReady(
            SimulatorControl::deleteSimulator(info.identifier), this,
            std::bind(onSimOperation, info, statusDialog, Tr::tr("simulator delete"), _1)));
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
    const SimulatorInfoList simulatorInfoList = selectedSimulators(m_deviceView);
    if (simulatorInfoList.isEmpty())
        return;

    const auto generatePath = [this](const SimulatorInfo &info) {
        const QString fileName = QString("%1_%2_%3.png").arg(info.name).arg(info.runtimeName)
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss-z")).replace(' ', '_');
        return m_pathWidget->filePath().pathAppended(fileName).toString();
    };

    QPointer<SimulatorOperationDialog> statusDialog = new SimulatorOperationDialog(this);
    statusDialog->setAttribute(Qt::WA_DeleteOnClose);
    statusDialog->addMessage(Tr::tr("Capturing screenshots from %n device(s)...", "",
                                    simulatorInfoList.count()), Utils::NormalMessageFormat);
    QList<QFuture<void>> futureList;
    for (const SimulatorInfo &info : simulatorInfoList) {
        futureList << QFuture<void>(Utils::onResultReady(
            SimulatorControl::takeSceenshot(info.identifier, generatePath(info)), this,
            std::bind(onSimOperation, info, statusDialog, Tr::tr("simulator screenshot"), _1)));
    }

    statusDialog->addFutures(futureList);
    statusDialog->exec(); // Modal dialog returns only when all the operations are done or cancelled.
}

void IosSettingsWidget::onSelectionChanged()
{
    const SimulatorInfoList infoList = selectedSimulators(m_deviceView);
    const bool hasRunning = Utils::anyOf(infoList, [](const SimulatorInfo &info) {
        return info.isBooted();
    });
    const bool hasShutdown = Utils::anyOf(infoList, [](const SimulatorInfo &info) {
        return info.isShutdown();
    });
    m_startButton->setEnabled(hasShutdown);
    m_deleteButton->setEnabled(hasShutdown);
    m_resetButton->setEnabled(hasShutdown);
    m_renameButton->setEnabled(infoList.count() == 1 && hasShutdown);
    m_pathWidget->buttonAtIndex(1)->setEnabled(hasRunning); // Screenshot button
}

void IosSettingsWidget::saveSettings()
{
    IosConfigurations::setIgnoreAllDevices(!m_deviceAskCheckBox->isChecked());
    IosConfigurations::setScreenshotDir(m_pathWidget->filePath());
}

// IosSettingsPage

IosSettingsPage::IosSettingsPage()
{
    setId(Constants::IOS_SETTINGS_ID);
    setDisplayName(Tr::tr("iOS"));
    setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new IosSettingsWidget; });
}

} // Ios::Internal

