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
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef IOSRUNCONFIGURATION_H
#define IOSRUNCONFIGURATION_H

#include "iosconstants.h"
#include "iosconfigurations.h"

#include <projectexplorer/runconfiguration.h>
#include <utils/fileutils.h>
#include <utils/qtcoverride.h>

namespace QmakeProjectManager {
class QmakeProFileNode;
}

namespace Ios {
namespace Internal {

enum { nSimulatedDevices = 4 };
static const IosDeviceType::Enum simulatedDevices[nSimulatedDevices] = {
    // skip iPhone as it does not support iOS7
    // TODO: clean solution would be to check also sdk version or make it configurable
    IosDeviceType::SimulatedIphoneRetina3_5Inch,
    IosDeviceType::SimulatedIphoneRetina4Inch,
    IosDeviceType::SimulatedIpad,
    IosDeviceType::SimulatedIpadRetina
};

class IosDeployStep;
class IosRunConfigurationFactory;
class IosRunConfigurationWidget;

class IosRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    friend class IosRunConfigurationFactory;

public:
    IosRunConfiguration(ProjectExplorer::Target *parent, Core::Id id, const QString &path);

    QWidget *createConfigurationWidget() QTC_OVERRIDE;
    Utils::OutputFormatter *createOutputFormatter() const QTC_OVERRIDE;
    IosDeployStep *deployStep() const;

    QStringList commandLineArguments();
    QString profilePath() const;
    QString applicationName() const;
    Utils::FileName bundleDirectory() const;
    Utils::FileName localExecutable() const;
    bool isEnabled() const;
    QString disabledReason() const;
    IosDeviceType::Enum deviceType() const;
    void setDeviceType(IosDeviceType::Enum deviceType);

    bool fromMap(const QVariantMap &map) QTC_OVERRIDE;
    QVariantMap toMap() const QTC_OVERRIDE;

protected:
    IosRunConfiguration(ProjectExplorer::Target *parent, IosRunConfiguration *source);

private slots:
    void proFileUpdated(QmakeProjectManager::QmakeProFileNode *pro, bool success, bool parseInProgress);
    void deviceChanges();
signals:
    void localExecutableChanged();
private:
    void init();
    void enabledCheck();
    friend class IosRunConfigurationWidget;
    void updateDisplayNames();

    QString m_profilePath;
    QStringList m_arguments;
    QString m_lastDisabledReason;
    bool m_lastIsEnabled;
    bool m_parseInProgress;
    bool m_parseSuccess;
    IosDeviceType::Enum m_deviceType;
};

} // namespace Internal
} // namespace Ios

#endif // IOSRUNCONFIGURATION_H
