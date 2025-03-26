// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <extensionsystem/iplugin.h>

namespace ExtraPropertyEditorManager {

class ExtraPropertyEditorManagerPlugin : public ExtensionSystem::IPlugin
{
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE
                              "ExtraPropertyEditorManager.json")
    public:
        bool initialize(const QStringList &arguments, QString *errorString) override;
        bool delayedInitialize() override;
};

} //  namespace ExtraPropertyEditorManager
