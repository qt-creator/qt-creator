// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <effectcomposerview.h>

#include <qmldesignerplugin.h>
#include <coreplugin/icore.h>

#include <extensionsystem/iplugin.h>

namespace EffectComposer {

class EffectComposerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "EffectComposer.json")

public:
    EffectComposerPlugin() {}
    ~EffectComposerPlugin() override {}

    void initialize() final
    {
        EffectComposerView::registerDeclarativeTypes();
    }
};

} // namespace EffectComposer

#include "effectcomposerplugin.moc"
