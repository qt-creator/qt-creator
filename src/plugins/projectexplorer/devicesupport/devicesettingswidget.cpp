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
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
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
    m_configurationLabel = new QLabel(tr("&Device:"));
    m_configurationComboBox = new QComboBox;
    m_configurationComboBox->setModel(m_deviceManagerModel);
    m_generalGroupBox = new QGroupBox(tr("General"));
    m_nameLineEdit = new QLineEdit;
    m_nameLineEdit->setValidator(m_nameValidator);
    m_osTypeValueLabel = new QLabel;
    m_autoDetectionLabel = new QLabel;
    m_deviceStateIconLabel = new QLabel;
    m_deviceStateTextLabel = new QLabel;
    m_osSpecificGroupBox = new QGroupBox(tr("Type Specific"));
    m_osSpecificGroupBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    m_addConfigButton = new QPushButton(tr("&Add..."));
    m_removeConfigButton = new QPushButton(tr("&Remove"));
    m_defaultDeviceButton = new QPushButton(tr("Set As Default"));
    auto line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    auto customButtonsContainer = new QWidget;
    m_buttonsLayout = new QVBoxLayout(customButtonsContainer);
    m_buttonsLayout->setContentsMargins({});
    auto scrollAreaWidget = new QWidget;
    auto scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(scrollAreaWidget);

    using namespace Utils::Layouting;
    Column {
        m_generalGroupBox,
        m_osSpecificGroupBox,
    }.attachTo(scrollAreaWidget);

    Form {
        tr("&Name:"), m_nameLineEdit, br,
        tr("Type:"), m_osTypeValueLabel, br,
        tr("Auto-detected:"), m_autoDetectionLabel, br,
        tr("Current state:"), Row { m_deviceStateIconLabel, m_deviceStateTextLabel, st, }, br,
    }.attachTo(m_generalGroupBox);

    Row {
        Column {
            Form { m_configurationLabel, m_configurationComboBox, br, },
            scrollArea,
        },
        Column {
            m_addConfigButton,
            m_removeConfigButton,
            m_defaultDeviceButton,
            line,
            customButtonsContainer,
            st,
        },
    }.attachTo(this);

    bool hasDeviceFactories = Utils::anyOf(IDeviceFactory::allDeviceFactories(),
                                           &IDeviceFactory::canCreate);

    m_addConfigButton->setEnabled(hasDeviceFactories);

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
    connect(m_nameLineEdit, &QLineEdit::editingFinished,
            this, &DeviceSettingsWidget::deviceNameEditingFinished);
    connect(m_addConfigButton, &QAbstractButton::clicked,
            this, &DeviceSettingsWidget::addDevice);
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
            ? tr("Yes (id is \"%1\")").arg(current->id().toString()) : tr("No"));
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
        QPushButton * const button = new QPushButton(tr("Test"));
        m_additionalActionButtons << button;
        connect(button, &QAbstractButton::clicked, this, &DeviceSettingsWidget::testDevice);
        m_buttonsLayout->insertWidget(m_buttonsLayout->count() - 1, button);
    }

    if (device->canCreateProcessModel()) {
        QPushButton * const button = new QPushButton(tr("Show Running Processes..."));
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
