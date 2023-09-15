// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>
#include <utils/process.h>

namespace Core {
class ActionContainer;
class ExternalTool;
}

namespace EffectMaker {

class EffectMakerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "EffectMakerNew.json")

public:
    EffectMakerPlugin() {}
    ~EffectMakerPlugin() override {}

    bool delayedInitialize() override;

private:
    bool m_delayedInitialized = false;
};

} // namespace EffectMaker

