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

#include "../kitaspect.h"
#include "../projectexplorerconstants.h"
#include "../projectexplorertr.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/layoutbuilder.h>
#include <utils/optionpushbutton.h>
#include <utils/qtcassert.h>

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QTextStream>
#include <QVBoxLayout>
#include <QValidator>

using namespace Core;
using namespace Utils;

namespace ProjectExplorer::Internal {

const char LastDeviceIndexKey[] = "LastDisplayedMaemoDeviceConfig";

class DeviceProxyModel : public QIdentityProxyModel
{
public:
    QVariant data(const QModelIndex &index, int role) const override
    {
        if (role == Qt::FontRole) {
            const Id id = Id::fromSetting(index.data(KitAspect::IdRole));

            const bool isMarkedForDeletion = m_markedForDeletion.contains(id);
            const bool isNewDevice = m_newDevices.contains(id);

            QFont font;
            font.setItalic(isNewDevice);
            font.setStrikeOut(isMarkedForDeletion);
            return font;
        }

        return QIdentityProxyModel::data(index, role);
    }

    void toggleMarkForDeletion(const Id &id)
    {
        if (m_markedForDeletion.contains(id))
            m_markedForDeletion.remove(id);
        else
            m_markedForDeletion.insert(id);

        emitDataChanged(id);
    }

    void markAsNew(const Id &id)
    {
        m_newDevices.insert(id);
        emitDataChanged(id);
    }

    void commitNewDevices()
    {
        m_newDevices.clear();
        emit dataChanged(index(0, 0), index(rowCount() - 1, 0), {Qt::FontRole});
    }

    bool isMarkedForDeletion(const Id &id) const { return m_markedForDeletion.contains(id); }
    bool isNewDevice(const Id &id) const { return m_newDevices.contains(id); }

    QSet<Id> markedForDeletion() const { return m_markedForDeletion; }
    QSet<Id> newDevices() const { return m_newDevices; }

private:
    void emitDataChanged(const Id &id)
    {
        for (int i = 0; i < rowCount(); ++i) {
            const Id deviceId = Id::fromSetting(data(index(i, 0), KitAspect::IdRole));
            if (deviceId == id) {
                QModelIndex modelIndex = index(i, 0);
                emit dataChanged(modelIndex, modelIndex, {Qt::FontRole});
                break;
            }
        }
    }

private:
    QSet<Id> m_markedForDeletion;
    QSet<Id> m_newDevices;
};

class DeviceSettingsWidget final : public Core::IOptionsPageWidget
{
public:
    DeviceSettingsWidget();
    ~DeviceSettingsWidget() final { delete m_configWidget; }

private:
    void apply() final;
    void cancel() final;

    void saveSettings();

    void handleDeviceUpdated(Id id);
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

    void updateButtons();

    DeviceManagerModel * const m_deviceManagerModel;
    DeviceProxyModel m_deviceProxyModel;
    QList<QPushButton *> m_additionalActionButtons;
    IDeviceWidget *m_configWidget = nullptr;

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
    QLayout *m_generalFormLayout;
};

void DeviceSettingsWidget::apply()
{
    for (const Id &id : m_deviceProxyModel.markedForDeletion())
        DeviceManager::removeDevice(id);

    m_deviceProxyModel.commitNewDevices();
    updateButtons();

    saveSettings();
}

void DeviceSettingsWidget::cancel()
{
    for (const Id &id : m_deviceProxyModel.newDevices())
        DeviceManager::removeDevice(id);

    for (int i = 0; i < m_deviceManagerModel->rowCount(); i++)
        m_deviceManagerModel->device(i)->cancel();
    IOptionsPageWidget::cancel();
}

DeviceSettingsWidget::DeviceSettingsWidget()
    : m_deviceManagerModel(new DeviceManagerModel(this))
{
    m_deviceProxyModel.setSourceModel(m_deviceManagerModel);

    m_configurationLabel = new QLabel(Tr::tr("&Device:"));
    m_configurationComboBox = new QComboBox;
    m_configurationComboBox->setModel(&m_deviceProxyModel);
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
    QAction *defaultAction = new QAction(Tr::tr("&Start Wizard to Add Device..."), this);
    connect(defaultAction, &QAction::triggered, this, &DeviceSettingsWidget::addDevice);
    deviceTypeMenu->addAction(defaultAction);
    deviceTypeMenu->addSeparator();

    for (IDeviceFactory *factory : IDeviceFactory::allDeviceFactories()) {
        if (!factory->canCreate())
            continue;
        if (!factory->quickCreationAllowed())
            continue;

        //: Add <Device Type Name>
        QAction *action = new QAction(Tr::tr("Add %1").arg(factory->displayName()), this);
        deviceTypeMenu->addAction(action);

        connect(action, &QAction::triggered, this, [factory, this] {
            IDevice::Ptr device = factory->construct();
            QTC_ASSERT(device, return);
            DeviceManager::addDevice(device);
            m_deviceProxyModel.markAsNew(device->id());
            updateButtons();
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
    m_deviceNameEditWidget = new QWidget;

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

    int lastIndex = -1;
    if (const Id deviceToSelect = preselectedOptionsPageItem(Constants::DEVICE_SETTINGS_PAGE_ID);
        deviceToSelect.isValid()) {
        lastIndex = m_deviceManagerModel->indexForId(deviceToSelect);
    }
    if (lastIndex == -1)
        lastIndex = ICore::settings()->value(LastDeviceIndexKey, 0).toInt();
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
    connect(DeviceManager::instance(), &DeviceManager::deviceUpdated,
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
    if (!device)
        return;

    Utils::asyncRun([device] { device->checkOsType(); });

    DeviceManager::addDevice(device);
    m_deviceProxyModel.markAsNew(device->id());

    updateButtons();
    m_configurationComboBox->setCurrentIndex(m_deviceManagerModel->indexOf(device));
    saveSettings();
    if (device->hasDeviceTester())
        testDevice();
}

void DeviceSettingsWidget::removeDevice()
{
    m_deviceProxyModel.toggleMarkForDeletion(currentDevice()->id());
    updateButtons();
}

void DeviceSettingsWidget::updateButtons()
{
    const IDevice::ConstPtr &current = currentDevice();

    const bool isMarkedForDeletion = m_deviceProxyModel.isMarkedForDeletion(current->id());
    const bool isNewDevice = m_deviceProxyModel.isNewDevice(current->id());

    m_removeConfigButton->setEnabled(
        (!current->isAutoDetected() || current->deviceState() == IDevice::DeviceDisconnected)
        && !isNewDevice);

    if (isMarkedForDeletion)
        m_removeConfigButton->setText(Tr::tr("&Restore"));
    else
        m_removeConfigButton->setText(Tr::tr("&Remove"));

    QFont f = m_configurationComboBox->font();
    f.setStrikeOut(isMarkedForDeletion);
    f.setItalic(isNewDevice);
    m_configurationComboBox->setFont(f);
}

void DeviceSettingsWidget::displayCurrent()
{
    const IDevice::ConstPtr &current = currentDevice();
    m_defaultDeviceButton->setEnabled(DeviceManager::defaultDevice(current->type()) != current);
    m_osTypeValueLabel->setText(current->displayType());
    m_autoDetectionLabel->setText(current->isAutoDetected()
            ? Tr::tr("Yes (id is \"%1\")").arg(current->id().toString()) : Tr::tr("No"));
    m_deviceStateIconLabel->show();
    if (const QPixmap &icon = current->deviceStateIcon(); !icon.isNull())
        m_deviceStateIconLabel->setPixmap(icon);
    else
        m_deviceStateIconLabel->hide();
    m_deviceStateTextLabel->setText(current->deviceStateToString());

    updateButtons();
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
    currentDevice()->doApply();
    if (m_configWidget)
        m_configWidget->updateDeviceFromUi();
}

void DeviceSettingsWidget::saveSettings()
{
    updateDeviceFromUi();
    ICore::settings()->setValueWithDefault(LastDeviceIndexKey, currentIndex(), 0);
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
    DeviceManager::setDefaultDevice(currentDevice()->id());
    m_defaultDeviceButton->setEnabled(false);
}

void DeviceSettingsWidget::testDevice()
{
    const IDevice::ConstPtr &device = currentDevice();
    QTC_ASSERT(device && device->hasDeviceTester(), return);
    auto dlg = new DeviceTestDialog(DeviceManager::mutableDevice(device->id()), this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setModal(true);
    dlg->show();
    connect(dlg, &QObject::destroyed, this, [this, id = device->id()] {
        handleDeviceUpdated(id);
    });
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
    if (!device) {
        setDeviceInfoWidgetsEnabled(false);
        updateButtons();
        clearDetails();
        m_defaultDeviceButton->setEnabled(false);
        return;
    }

    Layouting::Column item{Layouting::noMargin};
    device->addDisplayNameToLayout(item);
    QWidget *newEdit = item.emerge();
    QLayoutItem *oldItem = m_generalFormLayout->replaceWidget(m_deviceNameEditWidget, newEdit);
    QTC_CHECK(oldItem);
    delete oldItem;
    delete m_deviceNameEditWidget;
    m_deviceNameEditWidget = newEdit;

    setDeviceInfoWidgetsEnabled(true);
    updateButtons();

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
            const IDevice::Ptr device = DeviceManager::mutableDevice(currentDevice()->id());
            QTC_ASSERT(device, return);
            updateDeviceFromUi();
            deviceAction.execute(device);
            // Widget must be set up from scratch, because the action could have
            // changed random attributes.
            currentDeviceChanged(currentIndex());
        });

        m_buttonsLayout->insertWidget(m_buttonsLayout->count() - 1, button);
    }

    if (!m_osSpecificGroupBox->layout())
        new QVBoxLayout(m_osSpecificGroupBox);
    m_configWidget = DeviceManager::mutableDevice(device->id())->createWidget();
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
    setWidgetCreator([] { return new DeviceSettingsWidget; });
}

} // ProjectExplorer::Internal
