// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "bineditorservice.h"

#include <extensionsystem/iplugin.h>

namespace BinEditor::Internal {

class BinEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "BinEditor.json")

    ~BinEditorPlugin() override;

    void initialize() final;
    void extensionsInitialized() final {}
};

class FactoryServiceImpl : public QObject, public FactoryService
{
    Q_OBJECT
    Q_INTERFACES(BinEditor::FactoryService)

public:
    EditorService *createEditorService(const QString &title0, bool wantsEditor) final;
};

} // BinEditor::Internal
