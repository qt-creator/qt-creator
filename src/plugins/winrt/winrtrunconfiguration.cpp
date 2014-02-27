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

#include "winrtrunconfiguration.h"
#include "winrtrunconfigurationwidget.h"

#include <coreplugin/icore.h>
#include <projectexplorer/target.h>
#include <projectexplorer/kitinformation.h>

namespace WinRt {
namespace Internal {

static const char argumentsIdC[] = "WinRtRunConfigurationArgumentsId";
static const char uninstallAfterStopIdC[] = "WinRtRunConfigurationUninstallAfterStopId";

WinRtRunConfiguration::WinRtRunConfiguration(ProjectExplorer::Target *parent, const Core::Id &id)
    : RunConfiguration(parent, id)
    , m_uninstallAfterStop(false)
{
    setDisplayName(tr("Run App Package"));
}

QWidget *WinRtRunConfiguration::createConfigurationWidget()
{
    return new WinRtRunConfigurationWidget(this);
}

QVariantMap WinRtRunConfiguration::toMap() const
{
    QVariantMap map = RunConfiguration::toMap();
    map.insert(QLatin1String(argumentsIdC), m_arguments);
    map.insert(QLatin1String(uninstallAfterStopIdC), m_uninstallAfterStop);
    return map;
}

bool WinRtRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;
    setArguments(map.value(QLatin1String(argumentsIdC)).toString());
    setUninstallAfterStop(map.value(QLatin1String(uninstallAfterStopIdC)).toBool());
    return true;
}

void WinRtRunConfiguration::setArguments(const QString &args)
{
    if (m_arguments == args)
        return;
    m_arguments = args;
    emit argumentsChanged(m_arguments);
}

void WinRtRunConfiguration::setUninstallAfterStop(bool b)
{
    m_uninstallAfterStop = b;
    emit uninstallAfterStopChanged(m_uninstallAfterStop);
}

} // namespace Internal
} // namespace WinRt
