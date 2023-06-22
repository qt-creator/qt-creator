// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devicesettingswidget.h"

#include "devicefactoryselectiondialog.h"
#include "devicemanager.h"
#include "devicemanagermodel.h"
#include "deviceprocessesdialog.h"
#include "devicetestdialog.h"
#include "idevice.h"
#include "idevicefactory.h"
#include "idevicewidget.h"
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

namespace ProjectExplorer {
namespace Internal {
const char LastDeviceIndexKey[] = "LastDisplayedMaemoDeviceConfig";

class NameValidator : public QValidator
{
public:
    NameValidator(const DeviceManager *deviceManager, QWidget *parent = nullptr)
        : QValidator(parent), m_deviceManager(deviceManager)
    {
    }

    void setDisplayName(const QString &name) { m_oldName = name; }

    State validate(QString &input, int & /* pos */) const override
    {
        if (input.trimmed().isEmpty()
                || (input != m_oldName && m_deviceManager->hasDevice(input)))
            return Intermediate;
        return Acceptable;
    }

    void fixup(QString &input) const override
    {
        int dummy = 0;
        if (validate(input, dummy) != Acceptable)
            input = m_oldName;
    }

private:
    QString m_oldName;
    const DeviceManager * const m_deviceManager;
};

DeviceSettingsWidget::DeviceSettingsWidget()
    : m_deviceManager(DeviceManager::cloneInstance()),
      m_deviceManagerModel(new DeviceManagerModel(m_deviceManager, this)),
      m_nameValidator(new NameValidator(m_deviceManager, this)),
      m_configWidget(nullptr)
{
    initGui();
    connect(m_deviceManager, &DeviceManager::deviceUpdated,
            this, &DeviceSettingsWidget::handleDeviceUpdated);
}

DeviceSettingsWidget::~DeviceSettingsWidget()
{
    DeviceManager::removeClonedInstance();
    delete m_configWidget;
}

void DeviceSettingsWidget::initGui()
{
    m_configurationLabel = new QLabel(Tr::tr("&Device:"));
    m_configurationComboBox = new QComboBox;
    m_configurationComboBox->setModel(m_deviceManagerModel);
    m_generalGroupBox = new QGroupBox(Tr::tr("General"));
    m_nameLineEdit = new QLineEdit;
    m_nameLineEdit->setValidator(m_nameValidator);
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

    Form {
        Tr::tr("&Name:"), m_nameLineEdit, br,
        Tr::tr("Type:"), m_osTypeValueLabel, br,
        Tr::tr("Auto-detected:"), m_autoDetectionLabel, br,
        Tr::tr("Current state:"), Row { m_deviceStateIconLabel, m_deviceStateTextLabel, st, }, br,
    }.attachTo(m_generalGroupBox);

    // clang-format off
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

    int lastIndex = ICore::settings()
        ->value(QLatin1String(LastDeviceIndexKey), 0).toInt();
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
    connect(m_nameLineEdit,
            &QLineEdit::editingFinished,
            this,
            &DeviceSettingsWidget::deviceNameEditingFinished);
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
    m_nameValidator->setDisplayName(current->displayName());
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
    fillInValues();
}

void DeviceSettingsWidget::setDeviceInfoWidgetsEnabled(bool enable)
{
    m_configurationLabel->setEnabled(enable);
    m_configurationComboBox->setEnabled(enable);
    m_generalGroupBox->setEnabled(enable);
    m_osSpecificGroupBox->setEnabled(enable);
}

void DeviceSettingsWidget::fillInValues()
{
    const IDevice::ConstPtr &current = currentDevice();
    m_nameLineEdit->setText(current->displayName());
}

void DeviceSettingsWidget::updateDeviceFromUi()
{
    deviceNameEditingFinished();
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

void DeviceSettingsWidget::deviceNameEditingFinished()
{
    if (m_configurationComboBox->count() == 0)
        return;

    const QString &newName = m_nameLineEdit->text();
    m_deviceManager->mutableDevice(currentDevice()->id())->setDisplayName(newName);
    m_nameValidator->setDisplayName(newName);
    m_deviceManagerModel->updateDevice(currentDevice()->id());
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
    m_nameLineEdit->clear();
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

} // namespace Internal
} // namespace ProjectExplorer
