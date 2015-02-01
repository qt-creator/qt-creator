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

#include "iosrunconfiguration.h"
#include "iosconstants.h"
#include "iosmanager.h"
#include "iosdeploystep.h"
#include "ui_iosrunconfiguration.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <qmakeprojectmanager/qmakebuildconfiguration.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakenodes.h>
#include <qtsupport/qtoutputformatter.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/fileutils.h>

#include <QList>
#include <QStandardItemModel>

#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace Utils;

namespace Ios {
namespace Internal {

static const QLatin1String runConfigurationKey("Ios.run_arguments");
static const QLatin1String deviceTypeKey("Ios.device_type");

class IosRunConfigurationWidget : public RunConfigWidget
{
    Q_OBJECT

public:
    IosRunConfigurationWidget(IosRunConfiguration *runConfiguration);
    ~IosRunConfigurationWidget();
    QString argListToString(const QStringList &args) const;
    QStringList stringToArgList(const QString &args) const;
    QString displayName() const;

private slots:
    void argumentsLineEditTextEdited();
    void updateValues();
    void setDeviceTypeIndex(int devIndex);
private:
    Ui::IosRunConfiguration *m_ui;
    IosRunConfiguration *m_runConfiguration;
    QStandardItemModel m_deviceTypeModel;
};

IosRunConfiguration::IosRunConfiguration(Target *parent, Core::Id id, const FileName &path)
    : RunConfiguration(parent, id)
    , m_profilePath(path)
{
    init();
}

IosRunConfiguration::IosRunConfiguration(Target *parent, IosRunConfiguration *source)
    : RunConfiguration(parent, source)
    , m_profilePath(source->m_profilePath)
    , m_arguments(source->m_arguments)
{
    init();
}

void IosRunConfiguration::init()
{
    QmakeProject *project = static_cast<QmakeProject *>(target()->project());
    m_parseSuccess = project->validParse(m_profilePath);
    m_parseInProgress = project->parseInProgress(m_profilePath);
    m_lastIsEnabled = isEnabled();
    m_lastDisabledReason = disabledReason();
    updateDisplayNames();
    connect(DeviceManager::instance(), SIGNAL(updated()),
            SLOT(deviceChanges()));
    connect(KitManager::instance(), SIGNAL(kitsChanged()),
            SLOT(deviceChanges()));
    connect(target()->project(), SIGNAL(proFileUpdated(QmakeProjectManager::QmakeProFileNode*,bool,bool)),
            this, SLOT(proFileUpdated(QmakeProjectManager::QmakeProFileNode*,bool,bool)));
}

void IosRunConfiguration::enabledCheck()
{
    bool newIsEnabled = isEnabled();
    QString newDisabledReason = disabledReason();
    if (newDisabledReason != m_lastDisabledReason || newIsEnabled != m_lastIsEnabled) {
        m_lastDisabledReason = newDisabledReason;
        m_lastIsEnabled = newIsEnabled;
        emit enabledChanged();
    }
}

void IosRunConfiguration::deviceChanges() {
    updateDisplayNames();
    enabledCheck();
}

void IosRunConfiguration::proFileUpdated(QmakeProFileNode *pro, bool success,
                                         bool parseInProgress)
{
    if (m_profilePath != pro->path())
        return;
    m_parseSuccess = success;
    m_parseInProgress = parseInProgress;
    if (success && !parseInProgress) {
        updateDisplayNames();
        emit localExecutableChanged();
    }
    enabledCheck();
}

QWidget *IosRunConfiguration::createConfigurationWidget()
{
    return new IosRunConfigurationWidget(this);
}

OutputFormatter *IosRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(target()->project());
}

QStringList IosRunConfiguration::commandLineArguments()
{
    return m_arguments;
}

void IosRunConfiguration::updateDisplayNames()
{
    if (DeviceTypeKitInformation::deviceTypeId(target()->kit()) == Constants::IOS_DEVICE_TYPE)
        m_deviceType = IosDeviceType(IosDeviceType::IosDevice);
    else if (m_deviceType.type == IosDeviceType::IosDevice)
        m_deviceType = IosDeviceType(IosDeviceType::SimulatedDevice);
    IDevice::ConstPtr dev = DeviceKitInformation::device(target()->kit());
    const QString devName = dev.isNull() ? IosDevice::name() : dev->displayName();
    setDefaultDisplayName(tr("Run on %1").arg(devName));
    setDisplayName(tr("Run %1 on %2").arg(applicationName()).arg(devName));
}

IosDeployStep *IosRunConfiguration::deployStep() const
{
    IosDeployStep * step = 0;
    DeployConfiguration *config = target()->activeDeployConfiguration();
    BuildStepList *bsl = config->stepList();
    if (bsl) {
        const QList<BuildStep *> &buildSteps = bsl->steps();
        for (int i = buildSteps.count() - 1; i >= 0; --i) {
            step = qobject_cast<IosDeployStep *>(buildSteps.at(i));
            if (step)
                break;
        }
    }
    Q_ASSERT_X(step, Q_FUNC_INFO, "Impossible: iOS build configuration without deploy step.");
    return step;
}

FileName IosRunConfiguration::profilePath() const
{
    return m_profilePath;
}

QString IosRunConfiguration::applicationName() const
{
    QmakeProject *pro = qobject_cast<QmakeProject *>(target()->project());
    const QmakeProFileNode *node = 0;
    if (pro)
        node = pro->rootQmakeProjectNode();
    if (node)
        node = node->findProFileFor(profilePath());
    if (node) {
        TargetInformation ti = node->targetInformation();
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
    QmakeBuildConfiguration *bc =
            qobject_cast<QmakeBuildConfiguration *>(target()->activeBuildConfiguration());
    if (bc) {
        QmakeProject *pro = qobject_cast<QmakeProject *>(target()->project());
        const QmakeProFileNode *node = 0;
        if (pro)
            node = pro->rootQmakeProjectNode();
        if (node)
            node = node->findProFileFor(profilePath());
        if (node) {
            TargetInformation ti = node->targetInformation();
            if (ti.valid)
                res = FileName::fromString(ti.buildDir);
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

bool IosRunConfiguration::fromMap(const QVariantMap &map)
{
    m_arguments = map.value(runConfigurationKey).toStringList();
    bool deviceTypeIsInt;
    map.value(deviceTypeKey).toInt(&deviceTypeIsInt);
    if (deviceTypeIsInt || !m_deviceType.fromMap(map.value(deviceTypeKey).toMap())) {
        if (DeviceTypeKitInformation::deviceTypeId(target()->kit()) == Constants::IOS_DEVICE_TYPE)
            m_deviceType = IosDeviceType(IosDeviceType::IosDevice);
        else
            m_deviceType = IosDeviceType(IosDeviceType::SimulatedDevice);
    }
    return RunConfiguration::fromMap(map);
}

QVariantMap IosRunConfiguration::toMap() const
{
    QVariantMap res = RunConfiguration::toMap();
    res[runConfigurationKey] = m_arguments;
    res[deviceTypeKey] = deviceType().toMap();
    return res;
}

bool IosRunConfiguration::isEnabled() const
{
    if (m_parseInProgress || !m_parseSuccess)
        return false;
    Core::Id devType = DeviceTypeKitInformation::deviceTypeId(target()->kit());
    if (devType != Constants::IOS_DEVICE_TYPE && devType != Constants::IOS_SIMULATOR_TYPE)
        return false;
    IDevice::ConstPtr dev = DeviceKitInformation::device(target()->kit());
    if (dev.isNull() || dev->deviceState() != IDevice::DeviceReadyToUse)
        return false;
    return RunConfiguration::isEnabled();
}

QString IosRunConfiguration::disabledReason() const
{
    if (m_parseInProgress)
        return tr("The .pro file \"%1\" is currently being parsed.").arg(m_profilePath.fileName());
    if (!m_parseSuccess)
        return static_cast<QmakeProject *>(target()->project())
                ->disabledReasonForRunConfiguration(m_profilePath);
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
    QList<IosDeviceType> availableSimulators;
    if (m_deviceType.type == IosDeviceType::SimulatedDevice)
        availableSimulators = IosSimulator::availableDevices();
    if (!availableSimulators.isEmpty()) {
        QList<IosDeviceType> elegibleDevices;
        QString devname = m_deviceType.identifier.split(QLatin1Char(',')).value(0);
        foreach (const IosDeviceType &dType, availableSimulators) {
            if (dType == m_deviceType)
                return m_deviceType;
            if (!devname.isEmpty() && dType.identifier.startsWith(devname)
                    && dType.identifier.split(QLatin1Char(',')).value(0) == devname)
                elegibleDevices << dType;
        }
        if (!elegibleDevices.isEmpty())
            return elegibleDevices.last();
        return availableSimulators.last();
    }
    return m_deviceType;
}

void IosRunConfiguration::setDeviceType(const IosDeviceType &deviceType)
{
    m_deviceType = deviceType;
}

IosRunConfigurationWidget::IosRunConfigurationWidget(IosRunConfiguration *runConfiguration) :
    m_ui(new Ui::IosRunConfiguration), m_runConfiguration(runConfiguration)
{
    m_ui->setupUi(this);
    m_ui->deviceTypeComboBox->setModel(&m_deviceTypeModel);

    updateValues();
    connect(m_ui->deviceTypeComboBox, SIGNAL(currentIndexChanged(int)),
            SLOT(setDeviceTypeIndex(int)));
    connect(m_ui->argumentsLineEdit, SIGNAL(editingFinished()),
            SLOT(argumentsLineEditTextEdited()));
    connect(runConfiguration, SIGNAL(localExecutableChanged()),
            SLOT(updateValues()));
}

IosRunConfigurationWidget::~IosRunConfigurationWidget()
{
    delete m_ui;
}

QString IosRunConfigurationWidget::argListToString(const QStringList &args) const
{
    return QtcProcess::joinArgs(args);
}

QStringList IosRunConfigurationWidget::stringToArgList(const QString &args) const
{
    QtcProcess::SplitError err;
    QStringList res = QtcProcess::splitArgs(args, OsTypeMac, false, &err);
    switch (err) {
    case QtcProcess::SplitOk:
        break;
    case QtcProcess::BadQuoting:
        if (args.at(args.size()-1) == QLatin1Char('\\')) {
            res = QtcProcess::splitArgs(args + QLatin1Char('\\'), OsTypeMac, false, &err);
            if (err != QtcProcess::SplitOk)
                res = QtcProcess::splitArgs(args + QLatin1Char('\\') + QLatin1Char('\''),
                                            OsTypeMac, false, &err);
            if (err != QtcProcess::SplitOk)
                res = QtcProcess::splitArgs(args + QLatin1Char('\\') + QLatin1Char('\"'),
                                            OsTypeMac, false, &err);
        }
        if (err != QtcProcess::SplitOk)
            res = QtcProcess::splitArgs(args + QLatin1Char('\''), OsTypeMac, false, &err);
        if (err != QtcProcess::SplitOk)
            res = QtcProcess::splitArgs(args + QLatin1Char('\"'), OsTypeMac, false, &err);
        break;
    case QtcProcess::FoundMeta:
        qCWarning(iosLog) << "IosRunConfigurationWidget FoundMeta (should not happen)";
        break;
    }
    return res;
}

QString IosRunConfigurationWidget::displayName() const
{
    return tr("iOS run settings");
}

void IosRunConfigurationWidget::argumentsLineEditTextEdited()
{
    QString argsString = m_ui->argumentsLineEdit->text();
    QStringList args = stringToArgList(argsString);
    m_runConfiguration->m_arguments = args;
    m_ui->argumentsLineEdit->setText(argListToString(args));
}

void IosRunConfigurationWidget::setDeviceTypeIndex(int devIndex)
{
    QVariant selectedDev = m_deviceTypeModel.data(m_deviceTypeModel.index(devIndex, 0), Qt::UserRole + 1);
    if (selectedDev.isValid())
        m_runConfiguration->setDeviceType(selectedDev.value<IosDeviceType>());
}


void IosRunConfigurationWidget::updateValues()
{
    bool showDeviceSelector = m_runConfiguration->deviceType().type != IosDeviceType::IosDevice;
    m_ui->deviceTypeLabel->setVisible(showDeviceSelector);
    m_ui->deviceTypeComboBox->setVisible(showDeviceSelector);
    if (showDeviceSelector && m_deviceTypeModel.rowCount() == 0) {
        foreach (const IosDeviceType &dType, IosSimulator::availableDevices()) {
            QStandardItem *item = new QStandardItem(dType.displayName);
            QVariant v;
            v.setValue(dType);
            item->setData(v);
            m_deviceTypeModel.appendRow(item);
        }
    }
    QStringList args = m_runConfiguration->commandLineArguments();
    QString argsString = argListToString(args);

    IosDeviceType currentDType = m_runConfiguration->deviceType();
    if (currentDType.type == IosDeviceType::SimulatedDevice && !currentDType.identifier.isEmpty()
            && (!m_ui->deviceTypeComboBox->currentData().isValid()
                || currentDType != m_ui->deviceTypeComboBox->currentData().value<IosDeviceType>()))
    {
        bool didSet = false;
        for (int i = 0; m_deviceTypeModel.hasIndex(i, 0); ++i) {
            QVariant vData = m_deviceTypeModel.data(m_deviceTypeModel.index(i, 0), Qt::UserRole + 1);
            IosDeviceType dType = vData.value<IosDeviceType>();
            if (dType == currentDType) {
                m_ui->deviceTypeComboBox->setCurrentIndex(i);
                didSet = true;
                break;
            }
        }
        if (!didSet) {
            qCWarning(iosLog) << "could not set " << currentDType << " as it is not in model";
        }
    }
    m_ui->argumentsLineEdit->setText(argsString);
    m_ui->executableLineEdit->setText(m_runConfiguration->localExecutable().toUserOutput());
}

} // namespace Internal
} // namespace Ios

#include "iosrunconfiguration.moc"
