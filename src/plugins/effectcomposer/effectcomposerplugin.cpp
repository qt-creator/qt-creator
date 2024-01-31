// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <effectcomposerview.h>

#include <qmldesignerplugin.h>

#include <extensionsystem/iplugin.h>


namespace EffectComposer {

static bool enableEffectComposer()
{
    return true;
}

class EffectComposerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "EffectComposer.json")

public:
    EffectComposerPlugin() {}
    ~EffectComposerPlugin() override {}

    bool delayedInitialize() override
    {
        if (m_delayedInitialized)
            return true;

        if (enableEffectComposer()) {
            auto *designerPlugin = QmlDesigner::QmlDesignerPlugin::instance();
            auto &viewManager = designerPlugin->viewManager();

            viewManager.registerView(std::make_unique<EffectComposerView>(
                QmlDesigner::QmlDesignerPlugin::externalDependenciesForPluginInitializationOnly()));
        }

        m_delayedInitialized = true;

        return true;
    }

private:
    bool m_delayedInitialized = false;
};

} // namespace EffectComposer

#include "effectcomposerplugin.moc"
