/****************************************************************************
**
** Copyright (C) 2018 Benjamin Balga
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

#include "serialterminalplugin.h"

#include "serialcontrol.h"

#include <coreplugin/icore.h>

namespace SerialTerminal {
namespace Internal {

bool SerialTerminalPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    m_settings.load(Core::ICore::settings());

    // Create serial output pane
    m_serialOutputPane = std::make_unique<SerialOutputPane>(m_settings);
    connect(m_serialOutputPane.get(), &SerialOutputPane::settingsChanged,
            this, &SerialTerminalPlugin::settingsChanged);

    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
            this, [this] { m_settings.save(Core::ICore::settings()); });

    return true;
}

ExtensionSystem::IPlugin::ShutdownFlag SerialTerminalPlugin::aboutToShutdown()
{
    m_serialOutputPane->closeTabs(SerialOutputPane::CloseTabNoPrompt);

    return SynchronousShutdown;
}

void SerialTerminalPlugin::settingsChanged(const Settings &settings)
{
    m_settings = settings;
    m_settings.save(Core::ICore::settings());

    m_serialOutputPane->setSettings(m_settings);
}

} // namespace Internal
} // namespace SerialTerminal
