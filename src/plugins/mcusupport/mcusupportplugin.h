// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "settingshandler.h"

#include <extensionsystem/iplugin.h>

#include <projectexplorer/target.h>

namespace McuSupport::Internal {

void printMessage(const QString &message, bool important);

class McuSupportPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "McuSupport.json")

public:
    ~McuSupportPlugin() final;

    void initialize() final;
    void extensionsInitialized() final;

    void askUserAboutMcuSupportKitsSetup();
    static void askUserAboutMcuSupportKitsUpgrade(const SettingsHandler::Ptr &settingsHandler);
    static void askUserAboutRemovingUninstalledTargetsKits();

    Q_INVOKABLE static void updateDeployStep(ProjectExplorer::Target *target, bool enabled);
};

} // McuSupport::Internal
