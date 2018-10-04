/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "iosrunconfiguration.h"
#include "iosconstants.h"
#include "iosdevice.h"
#include "simulatorcontrol.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <qmakeprojectmanager/qmakenodes.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <qtsupport/qtoutputformatter.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QStandardItemModel>
#include <QVariant>
#include <QWidget>

using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace Utils;

namespace Ios {
namespace Internal {

static const QLatin1String deviceTypeKey("Ios.device_type");

static IosDeviceType toIosDeviceType(const SimulatorInfo &device)
{
    IosDeviceType iosDeviceType(IosDeviceType::SimulatedDevice,
                                device.identifier,
                                QString("%1, %2").arg(device.name).arg(device.runtimeName));
    return iosDeviceType;
}

class IosDeviceTypeAspect : public ProjectConfigurationAspect
{
public:
    IosDeviceTypeAspect(IosRunConfiguration *runConfiguration);

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;
    void addToConfigurationLayout(QFormLayout *layout) override;

    IosDeviceType deviceType() const;
    void setDeviceType(const IosDeviceType &deviceType);

    void updateValues();
    void setDeviceTypeIndex(int devIndex);
    void deviceChanges();
    void updateDeviceType();

public:
    IosDeviceType m_deviceType;
    IosRunConfiguration *m_runConfiguration = nullptr;
    QStandardItemModel m_deviceTypeModel;
    QLabel *m_deviceTypeLabel = nullptr;
    QComboBox *m_deviceTypeComboBox = nullptr;
};

IosRunConfiguration::IosRunConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
{
    auto executableAspect = addAspect<ExecutableAspect>();
    executableAspect->setDisplayStyle(BaseStringAspect::LabelDisplay);

    addAspect<ArgumentsAspect>();

    m_deviceTypeAspect = addAspect<IosDeviceTypeAspect>(this);

    setOutputFormatter<QtSupport::QtOutputFormatter>();
}

void IosDeviceTypeAspect::deviceChanges()
{
    updateDeviceType();
    m_runConfiguration->updateDisplayNames();
    m_runConfiguration->updateEnabledState();
}

void IosDeviceTypeAspect::updateDeviceType()
{
    if (DeviceTypeKitInformation::deviceTypeId(m_runConfiguration->target()->kit())
            == Constants::IOS_DEVICE_TYPE)
        m_deviceType = IosDeviceType(IosDeviceType::IosDevice);
    else if (m_deviceType.type == IosDeviceType::IosDevice)
        m_deviceType = IosDeviceType(IosDeviceType::SimulatedDevice);
}

void IosRunConfiguration::updateDisplayNames()
{
    IDevice::ConstPtr dev = DeviceKitInformation::device(target()->kit());
    const QString devName = dev.isNull() ? IosDevice::name() : dev->displayName();
    setDefaultDisplayName(tr("Run on %1").arg(devName));
    setDisplayName(tr("Run %1 on %2").arg(applicationName()).arg(devName));

    aspect<ExecutableAspect>()->setExecutable(localExecutable());
}

void IosRunConfiguration::updateEnabledState()
{
    Core::Id devType = DeviceTypeKitInformation::deviceTypeId(target()->kit());
    if (devType != Constants::IOS_DEVICE_TYPE && devType != Constants::IOS_SIMULATOR_TYPE) {
        setEnabled(false);
        return;
    }
    IDevice::ConstPtr dev = DeviceKitInformation::device(target()->kit());
    if (dev.isNull() || dev->deviceState() != IDevice::DeviceReadyToUse) {
        setEnabled(false);
        return;
    }
    return RunConfiguration::updateEnabledState();
}

bool IosRunConfiguration::canRunForNode(const Node *node) const
{
    return node->filePath() == profilePath();
}

FileName IosRunConfiguration::profilePath() const
{
    return FileName::fromString(buildKey());
}

static QmakeProFile *proFile(const IosRunConfiguration *rc)
{
    QmakeProject *pro = qobject_cast<QmakeProject *>(rc->target()->project());
    QmakeProFile *proFile = pro ? pro->rootProFile() : nullptr;
    if (proFile)
        proFile = proFile->findProFile(rc->profilePath());
    return proFile;
}

QString IosRunConfiguration::applicationName() const
{
    QmakeProFile *pro = proFile(this);
    if (pro) {
        TargetInformation ti = pro->targetInformation();
        if (ti.valid)
            return ti.target;
    }
    return QString();
}

FileName IosRunConfiguration::bundleDirectory() const
{
    FileName res;
    Core::Id devType = DeviceTypeKitInformation::deviceTypeId(target()->kit());
    bool isDevice = (devType == Constants::IOS_DEVICE_TYPE);
    if (!isDevice && devType != Constants::IOS_SIMULATOR_TYPE) {
        qCWarning(iosLog) << "unexpected device type in bundleDirForTarget: " << devType.toString();
        return res;
    }
    if (BuildConfiguration *bc = target()->activeBuildConfiguration()) {
        const QmakeProFile *pro = proFile(this);
        if (pro) {
            TargetInformation ti = pro->targetInformation();
            if (ti.valid)
                res = ti.buildDir;
        }
        if (res.isEmpty())
            res = bc->buildDirectory();
        switch (bc->buildType()) {
        case BuildConfiguration::Debug :
        case BuildConfiguration::Unknown :
            if (isDevice)
                res.appendPath(QLatin1String("Debug-iphoneos"));
            else
                res.appendPath(QLatin1String("Debug-iphonesimulator"));
            break;
        case BuildConfiguration::Profile :
        case BuildConfiguration::Release :
            if (isDevice)
                res.appendPath(QLatin1String("Release-iphoneos"));
            else
                res.appendPath(QLatin1String("Release-iphonesimulator"));
            break;
        default:
            qCWarning(iosLog) << "IosBuildStep had an unknown buildType "
                     << target()->activeBuildConfiguration()->buildType();
        }
    }
    res.appendPath(applicationName() + QLatin1String(".app"));
    return res;
}

FileName IosRunConfiguration::localExecutable() const
{
    return bundleDirectory().appendPath(applicationName());
}

void IosDeviceTypeAspect::fromMap(const QVariantMap &map)
{
    bool deviceTypeIsInt;
    map.value(deviceTypeKey).toInt(&deviceTypeIsInt);
    if (deviceTypeIsInt || !m_deviceType.fromMap(map.value(deviceTypeKey).toMap()))
        updateDeviceType();

    m_runConfiguration->updateDisplayNames();
}

void IosDeviceTypeAspect::toMap(QVariantMap &map) const
{
    map.insert(deviceTypeKey, deviceType().toMap());
}

QString IosRunConfiguration::disabledReason() const
{
    Core::Id devType = DeviceTypeKitInformation::deviceTypeId(target()->kit());
    if (devType != Constants::IOS_DEVICE_TYPE && devType != Constants::IOS_SIMULATOR_TYPE)
        return tr("Kit has incorrect device type for running on iOS devices.");
    IDevice::ConstPtr dev = DeviceKitInformation::device(target()->kit());
    QString validDevName;
    bool hasConncetedDev = false;
    if (devType == Constants::IOS_DEVICE_TYPE) {
        DeviceManager *dm = DeviceManager::instance();
        for (int idev = 0; idev < dm->deviceCount(); ++idev) {
            IDevice::ConstPtr availDev = dm->deviceAt(idev);
            if (!availDev.isNull() && availDev->type() == Constants::IOS_DEVICE_TYPE) {
                if (availDev->deviceState() == IDevice::DeviceReadyToUse) {
                    validDevName += QLatin1Char(' ');
                    validDevName += availDev->displayName();
                } else if (availDev->deviceState() == IDevice::DeviceConnected) {
                    hasConncetedDev = true;
                }
            }
        }
    }

    if (dev.isNull()) {
        if (!validDevName.isEmpty())
            return tr("No device chosen. Select %1.").arg(validDevName); // should not happen
        else if (hasConncetedDev)
            return tr("No device chosen. Enable developer mode on a device."); // should not happen
        else
            return tr("No device available.");
    } else {
        switch (dev->deviceState()) {
        case IDevice::DeviceReadyToUse:
            break;
        case IDevice::DeviceConnected:
            return tr("To use this device you need to enable developer mode on it.");
        case IDevice::DeviceDisconnected:
        case IDevice::DeviceStateUnknown:
            if (!validDevName.isEmpty())
                return tr("%1 is not connected. Select %2?")
                        .arg(dev->displayName(), validDevName);
            else if (hasConncetedDev)
                return tr("%1 is not connected. Enable developer mode on a device?")
                        .arg(dev->displayName());
            else
                return tr("%1 is not connected.").arg(dev->displayName());
        }
    }
    return RunConfiguration::disabledReason();
}

IosDeviceType IosRunConfiguration::deviceType() const
{
    return m_deviceTypeAspect->deviceType();
}

IosDeviceType IosDeviceTypeAspect::deviceType() const
{
    if (m_deviceType.type == IosDeviceType::SimulatedDevice) {
        QList<SimulatorInfo> availableSimulators = SimulatorControl::availableSimulators();
        if (availableSimulators.isEmpty())
            return m_deviceType;
        if (Utils::contains(availableSimulators,
                            Utils::equal(&SimulatorInfo::identifier, m_deviceType.identifier))) {
                 return m_deviceType;
        }
        const QStringList parts = m_deviceType.displayName.split(QLatin1Char(','));
        if (parts.count() < 2)
            return toIosDeviceType(availableSimulators.last());

        QList<SimulatorInfo> eligibleDevices;
        eligibleDevices = Utils::filtered(availableSimulators, [parts](const SimulatorInfo &info) {
            return info.name == parts.at(0) && info.runtimeName == parts.at(1);
        });
        return toIosDeviceType(eligibleDevices.isEmpty() ? availableSimulators.last()
                                                         : eligibleDevices.last());
    }
    return m_deviceType;
}

void IosDeviceTypeAspect::setDeviceType(const IosDeviceType &deviceType)
{
    m_deviceType = deviceType;
}

void IosRunConfiguration::doAdditionalSetup(const RunConfigurationCreationInfo &)
{
    m_deviceTypeAspect->updateDeviceType();
    updateDisplayNames();
}

IosDeviceTypeAspect::IosDeviceTypeAspect(IosRunConfiguration *runConfiguration)
    : m_runConfiguration(runConfiguration)
{
    connect(DeviceManager::instance(), &DeviceManager::updated,
            this, &IosDeviceTypeAspect::deviceChanges);
    connect(KitManager::instance(), &KitManager::kitsChanged,
            this, &IosDeviceTypeAspect::deviceChanges);
}

void IosDeviceTypeAspect::addToConfigurationLayout(QFormLayout *layout)
{
    m_deviceTypeComboBox = new QComboBox(layout->parentWidget());
    m_deviceTypeComboBox->setModel(&m_deviceTypeModel);

    m_deviceTypeLabel = new QLabel(IosRunConfiguration::tr("Device type:"), layout->parentWidget());

    layout->addRow(m_deviceTypeLabel, m_deviceTypeComboBox);

    updateValues();

    connect(m_deviceTypeComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &IosDeviceTypeAspect::setDeviceTypeIndex);
}

void IosDeviceTypeAspect::setDeviceTypeIndex(int devIndex)
{
    QVariant selectedDev = m_deviceTypeModel.data(m_deviceTypeModel.index(devIndex, 0), Qt::UserRole + 1);
    if (selectedDev.isValid())
        setDeviceType(toIosDeviceType(selectedDev.value<SimulatorInfo>()));
}


void IosDeviceTypeAspect::updateValues()
{
    bool showDeviceSelector = m_runConfiguration->deviceType().type != IosDeviceType::IosDevice;
    m_deviceTypeLabel->setVisible(showDeviceSelector);
    m_deviceTypeComboBox->setVisible(showDeviceSelector);
    if (showDeviceSelector && m_deviceTypeModel.rowCount() == 0) {
        foreach (const SimulatorInfo &device, SimulatorControl::availableSimulators()) {
            QStandardItem *item = new QStandardItem(QString("%1, %2").arg(device.name)
                                                    .arg(device.runtimeName));
            QVariant v;
            v.setValue(device);
            item->setData(v);
            m_deviceTypeModel.appendRow(item);
        }
    }

    IosDeviceType currentDType = m_runConfiguration->deviceType();
    QVariant currentData = m_deviceTypeComboBox->currentData();
    if (currentDType.type == IosDeviceType::SimulatedDevice && !currentDType.identifier.isEmpty()
            && (!currentData.isValid()
                || currentDType != toIosDeviceType(currentData.value<SimulatorInfo>())))
    {
        bool didSet = false;
        for (int i = 0; m_deviceTypeModel.hasIndex(i, 0); ++i) {
            QVariant vData = m_deviceTypeModel.data(m_deviceTypeModel.index(i, 0), Qt::UserRole + 1);
            SimulatorInfo dType = vData.value<SimulatorInfo>();
            if (dType.identifier == currentDType.identifier) {
                m_deviceTypeComboBox->setCurrentIndex(i);
                didSet = true;
                break;
            }
        }
        if (!didSet) {
            qCWarning(iosLog) << "could not set " << currentDType << " as it is not in model";
        }
    }
}


// IosRunConfigurationFactory

IosRunConfigurationFactory::IosRunConfigurationFactory()
{
    registerRunConfiguration<IosRunConfiguration>("Qt4ProjectManager.IosRunConfiguration:");
    addSupportedTargetDeviceType(Constants::IOS_DEVICE_TYPE);
    addSupportedTargetDeviceType(Constants::IOS_SIMULATOR_TYPE);
    addSupportedProjectType(QmakeProjectManager::Constants::QMAKEPROJECT_ID);
}

} // namespace Internal
} // namespace Ios
