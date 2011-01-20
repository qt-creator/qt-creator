/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qt4symbiantarget.h"
#include "qt4projectmanagerconstants.h"
#include "qt4project.h"
#include "qt-s60/s60deployconfiguration.h"
#include "qt-s60/s60emulatorrunconfiguration.h"
#include "qt-s60/s60devicerunconfiguration.h"

#include <coreplugin/coreconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <symbianutils/symbiandevicemanager.h>
#include <QtGui/QPainter>
#include <QtGui/QApplication>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

Qt4SymbianTarget::Qt4SymbianTarget(Qt4Project *parent, const QString &id) :
    Qt4BaseTarget(parent, id),
    m_connectedPixmap(QLatin1String(":/projectexplorer/images/ConnectionOn.png")),
    m_disconnectedPixmap(QLatin1String(":/projectexplorer/images/ConnectionOff.png")),
    m_buildConfigurationFactory(new Qt4BuildConfigurationFactory(this)),
    m_deployConfigurationFactory(new S60DeployConfigurationFactory(this))
{
    setDisplayName(defaultDisplayName(id));
    setIcon(iconForId(id));
    connect(this, SIGNAL(addedDeployConfiguration(ProjectExplorer::DeployConfiguration*)),
            this, SLOT(onAddedDeployConfiguration(ProjectExplorer::DeployConfiguration*)));
    connect(this, SIGNAL(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)),
            this, SLOT(updateToolTipAndIcon()));
}

Qt4SymbianTarget::~Qt4SymbianTarget()
{

}

QString Qt4SymbianTarget::defaultDisplayName(const QString &id)
{
    if (id == QLatin1String(Constants::S60_EMULATOR_TARGET_ID))
        return QApplication::translate("Qt4ProjectManager::Qt4Target", "Symbian Emulator", "Qt4 Symbian Emulator target display name");
    if (id == QLatin1String(Constants::S60_DEVICE_TARGET_ID))
        return QApplication::translate("Qt4ProjectManager::Qt4Target", "Symbian Device", "Qt4 Symbian Device target display name");
    return QString();
}

QIcon Qt4SymbianTarget::iconForId(const QString &id)
{
    if (id == QLatin1String(Constants::S60_EMULATOR_TARGET_ID))
        return QIcon(":/projectexplorer/images/SymbianEmulator.png");
    if (id == QLatin1String(Constants::S60_DEVICE_TARGET_ID))
        return QIcon(":/projectexplorer/images/SymbianDevice.png");
    return QIcon();
}

Qt4BuildConfigurationFactory *Qt4SymbianTarget::buildConfigurationFactory() const
{
    return m_buildConfigurationFactory;
}

ProjectExplorer::DeployConfigurationFactory *Qt4SymbianTarget::deployConfigurationFactory() const
{
    return m_deployConfigurationFactory;
}

QList<ProjectExplorer::ToolChainType> Qt4SymbianTarget::filterToolChainTypes(const QList<ProjectExplorer::ToolChainType> &candidates) const
{
    QList<ProjectExplorer::ToolChainType> tmp(candidates);
    if (id() == QLatin1String(Constants::S60_EMULATOR_TARGET_ID)) {
        if (tmp.contains(ProjectExplorer::ToolChain_WINSCW))
            return QList<ProjectExplorer::ToolChainType>() << ProjectExplorer::ToolChain_WINSCW;
        else
            return QList<ProjectExplorer::ToolChainType>();
    } else if (id() == QLatin1String(Constants::S60_DEVICE_TARGET_ID)) {
        tmp.removeAll(ProjectExplorer::ToolChain_WINSCW);
        return tmp;
    }
    return tmp;
}

ProjectExplorer::ToolChainType Qt4SymbianTarget::preferredToolChainType(const QList<ProjectExplorer::ToolChainType> &candidates) const
{
    ProjectExplorer::ToolChainType preferredType = ProjectExplorer::ToolChain_INVALID;
    if (id() == QLatin1String(Constants::S60_EMULATOR_TARGET_ID) &&
        candidates.contains(ProjectExplorer::ToolChain_WINSCW))
        preferredType = ProjectExplorer::ToolChain_WINSCW;
    if (!candidates.isEmpty())
        preferredType = candidates.at(0);
    return preferredType;
}

QString Qt4SymbianTarget::defaultBuildDirectory() const
{
    return project()->projectDirectory();
}

void Qt4SymbianTarget::createApplicationProFiles()
{
    removeUnconfiguredCustomExectutableRunConfigurations();

    // We use the list twice
    QList<Qt4ProFileNode *> profiles = qt4Project()->applicationProFiles();
    QSet<QString> paths;
    foreach (Qt4ProFileNode *pro, profiles)
        paths << pro->path();

    if (id() == QLatin1String(Constants::S60_EMULATOR_TARGET_ID)) {
        foreach (ProjectExplorer::RunConfiguration *rc, runConfigurations())
            if (S60EmulatorRunConfiguration *qt4rc = qobject_cast<S60EmulatorRunConfiguration *>(rc))
                paths.remove(qt4rc->proFilePath());

        // Only add new runconfigurations if there are none.
        foreach (const QString &path, paths)
            addRunConfiguration(new S60EmulatorRunConfiguration(this, path));
    } else if (id() == QLatin1String(Constants::S60_DEVICE_TARGET_ID)) {
        foreach (ProjectExplorer::RunConfiguration *rc, runConfigurations())
            if (S60DeviceRunConfiguration *qt4rc = qobject_cast<S60DeviceRunConfiguration *>(rc))
                paths.remove(qt4rc->proFilePath());

        // Only add new runconfigurations if there are none.
        foreach (const QString &path, paths)
            addRunConfiguration(new S60DeviceRunConfiguration(this, path));
    }

    // Oh still none? Add a custom executable runconfiguration
    if (runConfigurations().isEmpty()) {
        addRunConfiguration(new ProjectExplorer::CustomExecutableRunConfiguration(this));
    }
}

QList<ProjectExplorer::RunConfiguration *> Qt4SymbianTarget::runConfigurationsForNode(ProjectExplorer::Node *n)
{
    QList<ProjectExplorer::RunConfiguration *> result;
    foreach (ProjectExplorer::RunConfiguration *rc, runConfigurations()) {
        if (id() == QLatin1String(Constants::S60_EMULATOR_TARGET_ID)) {
            if (S60EmulatorRunConfiguration * s60rc = qobject_cast<S60EmulatorRunConfiguration *>(rc))
                if (s60rc->proFilePath() == n->path())
                    result << rc;
        } else if (id() == QLatin1String(Constants::S60_DEVICE_TARGET_ID)) {
            if (S60DeviceRunConfiguration *s60rc = qobject_cast<S60DeviceRunConfiguration *>(rc))
                if (s60rc->proFilePath() == n->path())
                    result << rc;
        }
    }
    return result;
}

bool Qt4SymbianTarget::isSymbianConnectionAvailable(QString &tooltipText)
{
    const S60DeployConfiguration *s60DeployConf = qobject_cast<S60DeployConfiguration *>(activeDeployConfiguration());
    if (!s60DeployConf)
        return false;
    switch (s60DeployConf->communicationChannel()) {
    case S60DeployConfiguration::CommunicationTrkSerialConnection: {
        const SymbianUtils::SymbianDeviceManager *sdm = SymbianUtils::SymbianDeviceManager::instance();
        const int deviceIndex = sdm->findByPortName(s60DeployConf->serialPortName());
        if (deviceIndex == -1) {
            tooltipText = tr("<b>Device:</b> Not connected");
            return false;
        } else {
            // device connected
            const SymbianUtils::SymbianDevice device = sdm->devices().at(deviceIndex);
            tooltipText = device.additionalInformation().isEmpty() ?
                                    tr("<b>Device:</b> %1").arg(device.friendlyName()) :
                                    tr("<b>Device:</b> %1, %2").arg(device.friendlyName(), device.additionalInformation());
            return true;
        }
    }
    break;
    case S60DeployConfiguration::CommunicationCodaTcpConnection: {
        if (!s60DeployConf->deviceAddress().isEmpty() && !s60DeployConf->devicePort().isEmpty()) {
            tooltipText = tr("<b>IP address:</b> %1:%2").arg(s60DeployConf->deviceAddress(), s60DeployConf->devicePort());
            return true;
        }
        return false;
    }
    break;
    default:
        break;
    }
    return false;
}

void Qt4SymbianTarget::onAddedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc)
{
    Q_ASSERT(dc);
    S60DeployConfiguration *deployConf(qobject_cast<S60DeployConfiguration *>(dc));
    if (!deployConf)
        return;
    connect(deployConf, SIGNAL(serialPortNameChanged()),
            this, SLOT(slotUpdateDeviceInformation()));
    connect(deployConf, SIGNAL(communicationChannelChanged()),
            this, SLOT(slotUpdateDeviceInformation()));
    connect(deployConf, SIGNAL(deviceAddressChanged()),
            this, SLOT(slotUpdateDeviceInformation()));
    connect(deployConf, SIGNAL(devicePortChanged()),
            this, SLOT(slotUpdateDeviceInformation()));
}

void Qt4SymbianTarget::slotUpdateDeviceInformation()
{
    S60DeployConfiguration *dc(qobject_cast<S60DeployConfiguration *>(sender()));
    if (dc && dc == activeDeployConfiguration()) {
        updateToolTipAndIcon();
    }
}

void Qt4SymbianTarget::updateToolTipAndIcon()
{
    static const int TARGET_OVERLAY_ORIGINAL_SIZE = 32;

    if (qobject_cast<S60DeployConfiguration *>(activeDeployConfiguration()))  {
        QPixmap overlay;
        QString tooltip;
        if (isSymbianConnectionAvailable(tooltip))
            overlay = m_connectedPixmap;
        else
            overlay = m_disconnectedPixmap;
        setToolTip(tooltip);

        double factor = Core::Constants::TARGET_ICON_SIZE / (double)TARGET_OVERLAY_ORIGINAL_SIZE;
        QSize overlaySize(overlay.size().width()*factor, overlay.size().height()*factor);
        QPixmap pixmap(Core::Constants::TARGET_ICON_SIZE, Core::Constants::TARGET_ICON_SIZE);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.drawPixmap(Core::Constants::TARGET_ICON_SIZE - overlaySize.width(),
                           Core::Constants::TARGET_ICON_SIZE - overlaySize.height(),
                           overlay.scaled(overlaySize));

        setOverlayIcon(QIcon(pixmap));
    } else {
        setToolTip(QString());
        setOverlayIcon(QIcon());
    }
}
