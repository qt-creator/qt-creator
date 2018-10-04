/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
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

#include "qnxrunconfiguration.h"

#include "qnxconstants.h"

using namespace ProjectExplorer;
using namespace RemoteLinux;

namespace Qnx {
namespace Internal {

QnxRunConfiguration::QnxRunConfiguration(Target *target, Core::Id id)
    : RemoteLinuxRunConfiguration(target, id)
{
    auto libAspect = addAspect<QtLibPathAspect>();
    libAspect->setSettingsKey("Qt4ProjectManager.QnxRunConfiguration.QtLibPath");
    libAspect->setLabelText(tr("Path to Qt libraries on device"));
    libAspect->setDisplayStyle(BaseStringAspect::LineEditDisplay);
}

Runnable QnxRunConfiguration::runnable() const
{
    Runnable r = RemoteLinuxRunConfiguration::runnable();
    QString libPath = aspect<QtLibPathAspect>()->value();
    if (!libPath.isEmpty()) {
        r.environment.appendOrSet("LD_LIBRARY_PATH", libPath + "/lib:$LD_LIBRARY_PATH");
        r.environment.appendOrSet("QML_IMPORT_PATH", libPath + "/imports:$QML_IMPORT_PATH");
        r.environment.appendOrSet("QML2_IMPORT_PATH", libPath + "/qml:$QML2_IMPORT_PATH");
        r.environment.appendOrSet("QT_PLUGIN_PATH", libPath + "/plugins:$QT_PLUGIN_PATH");
        r.environment.set("QT_QPA_FONTDIR", libPath + "/lib/fonts");
    }
    return r;
}

// QnxRunConfigurationFactory

QnxRunConfigurationFactory::QnxRunConfigurationFactory()
{
    registerRunConfiguration<QnxRunConfiguration>(Constants::QNX_QNX_RUNCONFIGURATION_PREFIX);
    addSupportedTargetDeviceType(Constants::QNX_QNX_OS_TYPE);
}

} // namespace Internal
} // namespace Qnx
