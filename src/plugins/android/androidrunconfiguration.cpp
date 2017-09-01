/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidrunconfiguration.h"
#include "androidglobal.h"
#include "androidtoolchain.h"
#include "androidmanager.h"
#include "androidrunconfigurationwidget.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtoutputformatter.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace Android {
using namespace Internal;

const char amStartArgsKey[] = "Android.AmStartArgsKey";
const char preStartShellCmdsKey[] = "Android.PreStartShellCmdListKey";
const char postFinishShellCmdsKey[] = "Android.PostFinishShellCmdListKey";

AndroidRunConfiguration::AndroidRunConfiguration(Target *target)
    : RunConfiguration(target)
{
}

void AndroidRunConfiguration::setPreStartShellCommands(const QStringList &cmdList)
{
    m_preStartShellCommands = cmdList;
}

void AndroidRunConfiguration::setPostFinishShellCommands(const QStringList &cmdList)
{
    m_postFinishShellCommands = cmdList;
}

void AndroidRunConfiguration::setAmStartExtraArgs(const QStringList &args)
{
    m_amStartExtraArgs = args;
}

QWidget *AndroidRunConfiguration::createConfigurationWidget()
{
    auto configWidget = new AndroidRunConfigurationWidget();
    configWidget->setAmStartArgs(m_amStartExtraArgs);
    configWidget->setPreStartShellCommands(m_preStartShellCommands);
    configWidget->setPostFinishShellCommands(m_postFinishShellCommands);
    connect(configWidget, &AndroidRunConfigurationWidget::amStartArgsChanged,
            this, &AndroidRunConfiguration::setAmStartExtraArgs);
    connect(configWidget, &AndroidRunConfigurationWidget::preStartCmdsChanged,
            this, &AndroidRunConfiguration::setPreStartShellCommands);
    connect(configWidget, &AndroidRunConfigurationWidget::postFinishCmdsChanged,
            this, &AndroidRunConfiguration::setPostFinishShellCommands);
    return configWidget;
}

Utils::OutputFormatter *AndroidRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(target()->project());
}

bool AndroidRunConfiguration::fromMap(const QVariantMap &map)
{
    m_preStartShellCommands = map.value(preStartShellCmdsKey).toStringList();
    m_postFinishShellCommands = map.value(postFinishShellCmdsKey).toStringList();
    m_amStartExtraArgs = map.value(amStartArgsKey).toStringList();
    return RunConfiguration::fromMap(map);
}

QVariantMap AndroidRunConfiguration::toMap() const
{
    QVariantMap res = RunConfiguration::toMap();
    res[preStartShellCmdsKey] = m_preStartShellCommands;
    res[postFinishShellCmdsKey] = m_postFinishShellCommands;
    res[amStartArgsKey] = m_amStartExtraArgs;
    return res;
}

const QStringList &AndroidRunConfiguration::amStartExtraArgs() const
{
    return m_amStartExtraArgs;
}

const QStringList &AndroidRunConfiguration::preStartShellCommands() const
{
    return m_preStartShellCommands;
}

const QStringList &AndroidRunConfiguration::postFinishShellCommands() const
{
    return m_postFinishShellCommands;
}

} // namespace Android
