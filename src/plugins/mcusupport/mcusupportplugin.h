/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#pragma once

#include "mcusupport_global.h"
#include "settingshandler.h"

#include <extensionsystem/iplugin.h>

namespace McuSupport::Internal {

void printMessage(const QString &message, bool important);

class McuSupportPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "McuSupport.json")

public:
    ~McuSupportPlugin() final;

    bool initialize(const QStringList &arguments, QString *errorString) final;
    void extensionsInitialized() final;

    void askUserAboutMcuSupportKitsSetup();
    static void askUserAboutMcuSupportKitsUpgrade(const SettingsHandler::Ptr &settingsHandler);

private:
    QVector<QObject *> createTestObjects() const final;
    SettingsHandler::Ptr m_settingsHandler{new SettingsHandler};

}; // class McuSupportPlugin

} // namespace McuSupport::Internal
