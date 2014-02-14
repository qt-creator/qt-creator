/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "iosrunconfiguration.h"
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

#include <QList>

#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace QmakeProjectManager;

namespace Ios {
namespace Internal {

const QLatin1String runConfigurationKey("Ios.run_arguments");

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

private:
    Ui::IosRunConfiguration *m_ui;
    IosRunConfiguration *m_runConfiguration;
};

IosRunConfiguration::IosRunConfiguration(Target *parent, Core::Id id, const QString &path)
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
    setDefaultDisplayName(defaultDisplayName());
    enabledCheck();
}

void IosRunConfiguration::proFileUpdated(QmakeProjectManager::QmakeProFileNode *pro, bool success,
                                         bool parseInProgress)
{
    if (m_profilePath != pro->path())
        return;
    m_parseSuccess = success;
    m_parseInProgress = parseInProgress;
    enabledCheck();
}

QWidget *IosRunConfiguration::createConfigurationWidget()
{
    return new IosRunConfigurationWidget(this);
}

Utils::OutputFormatter *IosRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(target()->project());
}

QStringList IosRunConfiguration::commandLineArguments()
{
    return m_arguments;
}

QString IosRunConfiguration::defaultDisplayName()
{
    ProjectExplorer::IDevice::ConstPtr dev =
            ProjectExplorer::DeviceKitInformation::device(target()->kit());
    const QString devName = dev.isNull() ? IosDevice::name() : dev->displayName();
    return tr("Run on %1").arg(devName);
}

IosDeployStep *IosRunConfiguration::deployStep() const
{
    IosDeployStep * step = 0;
    DeployConfiguration *config = target()->activeDeployConfiguration();
    ProjectExplorer::BuildStepList *bsl = config->stepList();
    if (bsl) {
        const QList<ProjectExplorer::BuildStep *> &buildSteps = bsl->steps();
        for (int i = buildSteps.count() - 1; i >= 0; --i) {
            step = qobject_cast<IosDeployStep *>(buildSteps.at(i));
            if (step)
                break;
        }
    }
    Q_ASSERT_X(step, Q_FUNC_INFO, "Impossible: iOS build configuration without deploy step.");
    return step;
}

QString IosRunConfiguration::profilePath() const
{
    return m_profilePath;
}

QString IosRunConfiguration::appName() const
{
    QmakeProject *pro = qobject_cast<QmakeProject *>(target()->project());
    if (pro) {
        const QmakeProFileNode *node = pro->rootQmakeProjectNode()->findProFileFor(profilePath());
        if (node) {
            TargetInformation ti = node->targetInformation();
            if (ti.valid)
                return ti.target;
        }
    }
    qDebug() << "IosRunConfiguration::appName failed";
    return QString();
}

Utils::FileName IosRunConfiguration::bundleDir() const
{
    Utils::FileName res;
    Core::Id devType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(target()->kit());
    bool isDevice = (devType == Constants::IOS_DEVICE_TYPE);
    if (!isDevice && devType != Constants::IOS_SIMULATOR_TYPE) {
        qDebug() << "unexpected device type in bundleDirForTarget: " << devType.toString();
        return res;
    }
    QmakeBuildConfiguration *bc =
            qobject_cast<QmakeBuildConfiguration *>(target()->activeBuildConfiguration());
    if (bc) {
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
                res.appendPath(QLatin1String("/Release-iphonesimulator"));
            break;
        default:
            qDebug() << "IosBuildStep had an unknown buildType "
                     << target()->activeBuildConfiguration()->buildType();
        }
    }
    res.appendPath(appName() + QLatin1String(".app"));
    return res;
}

Utils::FileName IosRunConfiguration::exePath() const
{
    return bundleDir().appendPath(appName());
}

bool IosRunConfiguration::fromMap(const QVariantMap &map)
{
    m_arguments = map.value(runConfigurationKey).toStringList();
    return RunConfiguration::fromMap(map);
}

QVariantMap IosRunConfiguration::toMap() const
{
    QVariantMap res = RunConfiguration::toMap();
    res[runConfigurationKey] = m_arguments;
    return res;
}

bool IosRunConfiguration::isEnabled() const
{
    if (m_parseInProgress || !m_parseSuccess)
        return false;
    Core::Id devType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(target()->kit());
    if (devType != Constants::IOS_DEVICE_TYPE && devType != Constants::IOS_SIMULATOR_TYPE)
        return false;
    IDevice::ConstPtr dev = ProjectExplorer::DeviceKitInformation::device(target()->kit());
    if (dev.isNull() || dev->deviceState() != IDevice::DeviceReadyToUse)
        return false;
    return RunConfiguration::isEnabled();
}

QString IosRunConfiguration::disabledReason() const
{
    if (m_parseInProgress)
        return tr("The .pro file '%1' is currently being parsed.")
                .arg(QFileInfo(m_profilePath).fileName());
    if (!m_parseSuccess)
        return static_cast<QmakeProject *>(target()->project())
                ->disabledReasonForRunConfiguration(m_profilePath);
    Core::Id devType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(target()->kit());
    if (devType != Constants::IOS_DEVICE_TYPE && devType != Constants::IOS_SIMULATOR_TYPE)
        return tr("Kit has incorrect device type for running on iOS devices.");
    IDevice::ConstPtr dev = ProjectExplorer::DeviceKitInformation::device(target()->kit());
    QString validDevName;
    bool hasConncetedDev = false;
    if (devType == Constants::IOS_DEVICE_TYPE) {
        DeviceManager *dm = DeviceManager::instance();
        for (int idev = 0; idev < dm->deviceCount(); ++idev) {
            IDevice::ConstPtr availDev = dm->deviceAt(idev);
            if (!availDev.isNull() && availDev->type() == Constants::IOS_DEVICE_TYPE) {
                if (availDev->deviceState() == IDevice::DeviceReadyToUse) {
                    validDevName += QLatin1String(" ");
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

IosRunConfigurationWidget::IosRunConfigurationWidget(IosRunConfiguration *runConfiguration) :
    m_ui(new Ui::IosRunConfiguration), m_runConfiguration(runConfiguration)
{
    m_ui->setupUi(this);

    updateValues();
    connect(m_ui->argumentsLineEdit, SIGNAL(editingFinished()),
            SLOT(argumentsLineEditTextEdited()));
    connect(runConfiguration->target(), SIGNAL(buildDirectoryChanged()),
            SLOT(updateValues()));
}

IosRunConfigurationWidget::~IosRunConfigurationWidget()
{
    delete m_ui;
}

QString IosRunConfigurationWidget::argListToString(const QStringList &args) const
{
    return Utils::QtcProcess::joinArgs(args);
}

QStringList IosRunConfigurationWidget::stringToArgList(const QString &args) const
{
    Utils::QtcProcess::SplitError err;
    QStringList res = Utils::QtcProcess::splitArgs(args, false, &err);
    switch (err) {
    case Utils::QtcProcess::SplitOk:
        break;
    case Utils::QtcProcess::BadQuoting:
        if (args.at(args.size()-1) == QLatin1Char('\\')) {
            res = Utils::QtcProcess::splitArgs(args + QLatin1Char('\\'), false, &err);
            if (err != Utils::QtcProcess::SplitOk)
                res = Utils::QtcProcess::splitArgs(args + QLatin1Char('\\') + QLatin1Char('\''),
                                                   false, &err);
            if (err != Utils::QtcProcess::SplitOk)
                res = Utils::QtcProcess::splitArgs(args + QLatin1Char('\\') + QLatin1Char('\"'),
                                                   false, &err);
        }
        if (err != Utils::QtcProcess::SplitOk)
            res = Utils::QtcProcess::splitArgs(args + QLatin1Char('\''), false, &err);
        if (err != Utils::QtcProcess::SplitOk)
            res = Utils::QtcProcess::splitArgs(args + QLatin1Char('\"'), false, &err);
        break;
    case Utils::QtcProcess::FoundMeta:
        qDebug() << "IosRunConfigurationWidget FoundMeta (should not happen)";
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

void IosRunConfigurationWidget::updateValues()
{
    QStringList args = m_runConfiguration->m_arguments;
    QString argsString = argListToString(args);
    m_ui->argumentsLineEdit->setText(argsString);
    m_ui->executableLineEdit->setText(m_runConfiguration->exePath().toUserOutput());
}

} // namespace Internal
} // namespace Ios

#include "iosrunconfiguration.moc"
