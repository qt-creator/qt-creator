// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devicesettingspage.h"

#include "devicefactoryselectiondialog.h"
#include "devicemanager.h"
#include "devicemanagermodel.h"
#include "deviceprocessesdialog.h"
#include "devicetestdialog.h"
#include "idevice.h"
#include "idevicefactory.h"
#include "idevicewidget.h"
#include "../projectexplorerconstants.h"
#include "../projectexplorericons.h"
#include "../projectexplorertr.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/layoutbuilder.h>
#include <utils/optionpushbutton.h>
#include <utils/qtcassert.h>

#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QTextStream>
#include <QVBoxLayout>
#include <QValidator>

#include <algorithm>

using namespace Core;
using namespace Utils;

namespace ProjectExplorer::Internal {

const char LastDeviceIndexKey[] = "LastDisplayedMaemoDeviceConfig";

class DeviceSettingsWidget final : public Core::IOptionsPageWidget
{
public:
    DeviceSettingsWidget();
    ~DeviceSettingsWidget() final
    {
        DeviceManager::removeClonedInstance();
        delete m_configWidget;
    }

private:
    void apply() final { saveSettings(); }

    void saveSettings();

    void handleDeviceUpdated(Utils::Id id);
    void currentDeviceChanged(int index);
    void addDevice();
    void removeDevice();
    void setDefaultDevice();
    void testDevice();
    void handleProcessListRequested();

    void initGui();
    void displayCurrent();
    void setDeviceInfoWidgetsEnabled(bool enable);
    IDeviceConstPtr currentDevice() const;
    int currentIndex() const;
    void clearDetails();
    QString parseTestOutput();
    void updateDeviceFromUi();

    DeviceManager * const m_deviceManager;
    DeviceManagerModel * const m_deviceManagerModel;
    QList<QPushButton *> m_additionalActionButtons;
    IDeviceWidget *m_configWidget;

    QLabel *m_configurationLabel;
    QComboBox *m_configurationComboBox;
    QGroupBox *m_generalGroupBox;
    QLabel *m_osTypeValueLabel;
    QLabel *m_autoDetectionLabel;
    QLabel *m_deviceStateIconLabel;
    QLabel *m_deviceStateTextLabel;
    QGroupBox *m_osSpecificGroupBox;
    QPushButton *m_removeConfigButton;
    QPushButton *m_defaultDeviceButton;
    QVBoxLayout *m_buttonsLayout;
    QWidget *m_deviceNameEditWidget;
    QFormLayout *m_generalFormLayout;
};

DeviceSettingsWidget::DeviceSettingsWidget()
    : m_deviceManager(DeviceManager::cloneInstance())
    , m_deviceManagerModel(new DeviceManagerModel(m_deviceManager, this))
    , m_configWidget(nullptr)
{
    m_configurationLabel = new QLabel(Tr::tr("&Device:"));
    m_configurationComboBox = new QComboBox;
    m_configurationComboBox->setModel(m_deviceManagerModel);
    m_generalGroupBox = new QGroupBox(Tr::tr("General"));
    m_osTypeValueLabel = new QLabel;
    m_autoDetectionLabel = new QLabel;
    m_deviceStateIconLabel = new QLabel;
    m_deviceStateTextLabel = new QLabel;
    m_osSpecificGroupBox = new QGroupBox(Tr::tr("Type Specific"));
    m_osSpecificGroupBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    m_removeConfigButton = new QPushButton(Tr::tr("&Remove"));
    m_defaultDeviceButton = new QPushButton(Tr::tr("Set As Default"));

    OptionPushButton *addButton = new OptionPushButton(Tr::tr("&Add..."));
    connect(addButton, &OptionPushButton::clicked, this, &DeviceSettingsWidget::addDevice);

    QMenu *deviceTypeMenu = new QMenu(addButton);
    QAction *defaultAction = new QAction(Tr::tr("&Start Wizard to Add Device..."));
    connect(defaultAction, &QAction::triggered, this, &DeviceSettingsWidget::addDevice);
    deviceTypeMenu->addAction(defaultAction);
    deviceTypeMenu->addSeparator();

    for (IDeviceFactory *factory : IDeviceFactory::allDeviceFactories()) {
        if (!factory->canCreate())
            continue;
        if (!factory->quickCreationAllowed())
            continue;

        //: Add <Device Type Name>
        QAction *action = new QAction(Tr::tr("Add %1").arg(factory->displayName()));
        deviceTypeMenu->addAction(action);

        connect(action, &QAction::triggered, this, [factory, this] {
            IDevice::Ptr device = factory->construct();
            QTC_ASSERT(device, return);
            m_deviceManager->addDevice(device);
            m_removeConfigButton->setEnabled(true);
            m_configurationComboBox->setCurrentIndex(m_deviceManagerModel->indexOf(device));
            saveSettings();
        });
    }

    addButton->setOptionalMenu(deviceTypeMenu);

    m_buttonsLayout = new QVBoxLayout;
    m_buttonsLayout->setContentsMargins({});
    auto scrollAreaWidget = new QWidget;
    auto scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(scrollAreaWidget);

    using namespace Layouting;
    Column {
        m_generalGroupBox,
        m_osSpecificGroupBox,
    }.attachTo(scrollAreaWidget);

    // Just a placeholder for the device name edit widget.
    m_deviceNameEditWidget = new QWidget();

    // clang-format off
    Form {
        bindTo(&m_generalFormLayout),
        Tr::tr("&Name:"), m_deviceNameEditWidget, br,
        Tr::tr("Type:"), m_osTypeValueLabel, br,
        Tr::tr("Auto-detected:"), m_autoDetectionLabel, br,
        Tr::tr("Current state:"), Row { m_deviceStateIconLabel, m_deviceStateTextLabel, st, }, br,
    }.attachTo(m_generalGroupBox);

    Row {
        Column {
            Form { m_configurationLabel, m_configurationComboBox, br, },
            scrollArea,
        },
        Column {
            addButton,
            Space(30),
            m_removeConfigButton,
            m_defaultDeviceButton,
            m_buttonsLayout,
            st,
        },
    }.attachTo(this);
    // clang-format on

    bool hasDeviceFactories = Utils::anyOf(IDeviceFactory::allDeviceFactories(),
                                           &IDeviceFactory::canCreate);

    addButton->setEnabled(hasDeviceFactories);

    int lastIndex = ICore::settings()->value(LastDeviceIndexKey, 0).toInt();
    if (lastIndex == -1)
        lastIndex = 0;
    if (lastIndex < m_configurationComboBox->count())
        m_configurationComboBox->setCurrentIndex(lastIndex);

    connect(m_configurationComboBox, &QComboBox::currentIndexChanged,
            this, &DeviceSettingsWidget::currentDeviceChanged);
    currentDeviceChanged(currentIndex());
    connect(m_defaultDeviceButton, &QAbstractButton::clicked,
            this, &DeviceSettingsWidget::setDefaultDevice);
    connect(m_removeConfigButton, &QAbstractButton::clicked,
            this, &DeviceSettingsWidget::removeDevice);
    connect(m_deviceManager, &DeviceManager::deviceUpdated,
            this, &DeviceSettingsWidget::handleDeviceUpdated);
}

void DeviceSettingsWidget::addDevice()
{
    DeviceFactorySelectionDialog d;
    if (d.exec() != QDialog::Accepted)
        return;

    Id toCreate = d.selectedId();
    if (!toCreate.isValid())
        return;
    IDeviceFactory *factory = IDeviceFactory::find(toCreate);
    if (!factory)
        return;
    IDevice::Ptr device = factory->create();
    if (device.isNull())
        return;

    Utils::asyncRun([device] { device->checkOsType(); });

    m_deviceManager->addDevice(device);
    m_removeConfigButton->setEnabled(true);
    m_configurationComboBox->setCurrentIndex(m_deviceManagerModel->indexOf(device));
    saveSettings();
    if (device->hasDeviceTester())
        testDevice();
}

void DeviceSettingsWidget::removeDevice()
{
    m_deviceManager->removeDevice(currentDevice()->id());
    if (m_deviceManager->deviceCount() == 0)
        currentDeviceChanged(-1);
}

void DeviceSettingsWidget::displayCurrent()
{
    const IDevice::ConstPtr &current = currentDevice();
    m_defaultDeviceButton->setEnabled(
        m_deviceManager->defaultDevice(current->type()) != current);
    m_osTypeValueLabel->setText(current->displayType());
    m_autoDetectionLabel->setText(current->isAutoDetected()
            ? Tr::tr("Yes (id is \"%1\")").arg(current->id().toString()) : Tr::tr("No"));
    m_deviceStateIconLabel->show();
    switch (current->deviceState()) {
    case IDevice::DeviceReadyToUse:
        m_deviceStateIconLabel->setPixmap(Icons::DEVICE_READY_INDICATOR.pixmap());
        break;
    case IDevice::DeviceConnected:
        m_deviceStateIconLabel->setPixmap(Icons::DEVICE_CONNECTED_INDICATOR.pixmap());
        break;
    case IDevice::DeviceDisconnected:
        m_deviceStateIconLabel->setPixmap(Icons::DEVICE_DISCONNECTED_INDICATOR.pixmap());
        break;
    case IDevice::DeviceStateUnknown:
        m_deviceStateIconLabel->hide();
        break;
    }
    m_deviceStateTextLabel->setText(current->deviceStateToString());

    m_removeConfigButton->setEnabled(!current->isAutoDetected()
            || current->deviceState() == IDevice::DeviceDisconnected);
}

void DeviceSettingsWidget::setDeviceInfoWidgetsEnabled(bool enable)
{
    m_configurationLabel->setEnabled(enable);
    m_configurationComboBox->setEnabled(enable);
    m_generalGroupBox->setEnabled(enable);
    m_osSpecificGroupBox->setEnabled(enable);
}

void DeviceSettingsWidget::updateDeviceFromUi()
{
    currentDevice()->settings()->apply();
    if (m_configWidget)
        m_configWidget->updateDeviceFromUi();
}

void DeviceSettingsWidget::saveSettings()
{
    updateDeviceFromUi();
    ICore::settings()->setValueWithDefault(LastDeviceIndexKey, currentIndex(), 0);
    DeviceManager::replaceInstance();
}

int DeviceSettingsWidget::currentIndex() const
{
    return m_configurationComboBox->currentIndex();
}

IDevice::ConstPtr DeviceSettingsWidget::currentDevice() const
{
    Q_ASSERT(currentIndex() != -1);
    return m_deviceManagerModel->device(currentIndex());
}


void DeviceSettingsWidget::setDefaultDevice()
{
    m_deviceManager->setDefaultDevice(currentDevice()->id());
    m_defaultDeviceButton->setEnabled(false);
}

void DeviceSettingsWidget::testDevice()
{
    const IDevice::ConstPtr &device = currentDevice();
    QTC_ASSERT(device && device->hasDeviceTester(), return);
    auto dlg = new DeviceTestDialog(m_deviceManager->mutableDevice(device->id()), this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setModal(true);
    dlg->show();
}

void DeviceSettingsWidget::handleDeviceUpdated(Id id)
{
    const int index = m_deviceManagerModel->indexForId(id);
    if (index == currentIndex())
        currentDeviceChanged(index);
}

void DeviceSettingsWidget::currentDeviceChanged(int index)
{
    qDeleteAll(m_additionalActionButtons);
    delete m_configWidget;
    m_configWidget = nullptr;
    m_additionalActionButtons.clear();
    const IDevice::ConstPtr device = m_deviceManagerModel->device(index);
    if (device.isNull()) {
        setDeviceInfoWidgetsEnabled(false);
        m_removeConfigButton->setEnabled(false);
        clearDetails();
        m_defaultDeviceButton->setEnabled(false);
        return;
    }

    Layouting::Column item{Layouting::noMargin()};
    device->settings()->displayName.addToLayout(item);
    QWidget *newEdit = item.emerge();
    m_generalFormLayout->replaceWidget(m_deviceNameEditWidget, newEdit);

    delete m_deviceNameEditWidget;
    m_deviceNameEditWidget = newEdit;

    setDeviceInfoWidgetsEnabled(true);
    m_removeConfigButton->setEnabled(true);

    if (device->hasDeviceTester()) {
        QPushButton * const button = new QPushButton(Tr::tr("Test"));
        m_additionalActionButtons << button;
        connect(button, &QAbstractButton::clicked, this, &DeviceSettingsWidget::testDevice);
        m_buttonsLayout->insertWidget(m_buttonsLayout->count() - 1, button);
    }

    if (device->canCreateProcessModel()) {
        QPushButton * const button = new QPushButton(Tr::tr("Show Running Processes..."));
        m_additionalActionButtons << button;
        connect(button, &QAbstractButton::clicked,
                this, &DeviceSettingsWidget::handleProcessListRequested);
        m_buttonsLayout->insertWidget(m_buttonsLayout->count() - 1, button);
    }

    for (const IDevice::DeviceAction &deviceAction : device->deviceActions()) {
        QPushButton * const button = new QPushButton(deviceAction.display);
        m_additionalActionButtons << button;
        connect(button, &QAbstractButton::clicked, this, [this, deviceAction] {
            const IDevice::Ptr device = m_deviceManager->mutableDevice(currentDevice()->id());
            QTC_ASSERT(device, return);
            updateDeviceFromUi();
            deviceAction.execute(device, this);
            // Widget must be set up from scratch, because the action could have
            // changed random attributes.
            currentDeviceChanged(currentIndex());
        });

        m_buttonsLayout->insertWidget(m_buttonsLayout->count() - 1, button);
    }

    if (!m_osSpecificGroupBox->layout())
        new QVBoxLayout(m_osSpecificGroupBox);
    m_configWidget = m_deviceManager->mutableDevice(device->id())->createWidget();
    if (m_configWidget)
        m_osSpecificGroupBox->layout()->addWidget(m_configWidget);
    displayCurrent();
}

void DeviceSettingsWidget::clearDetails()
{
    m_osTypeValueLabel->clear();
    m_autoDetectionLabel->clear();
}

void DeviceSettingsWidget::handleProcessListRequested()
{
    QTC_ASSERT(currentDevice()->canCreateProcessModel(), return);
    updateDeviceFromUi();
    DeviceProcessesDialog dlg;
    dlg.addCloseButton();
    dlg.setDevice(currentDevice());
    dlg.exec();
}

// DeviceSettingsPage

DeviceSettingsPage::DeviceSettingsPage()
{
    setId(Constants::DEVICE_SETTINGS_PAGE_ID);
    setDisplayName(Tr::tr("Devices"));
    setCategory(Constants::DEVICE_SETTINGS_CATEGORY);
    setDisplayCategory(Tr::tr("Devices"));
    setCategoryIconPath(":/projectexplorer/images/settingscategory_devices.png");
    setWidgetCreator([] { return new DeviceSettingsWidget; });
}

} // ProjectExplorer::Internal
