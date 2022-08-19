// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "bineditorservice.h"

#include <extensionsystem/iplugin.h>
#include <coreplugin/editormanager/ieditorfactory.h>

namespace BinEditor {
namespace Internal {

class BinEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "BinEditor.json")

    ~BinEditorPlugin() override;

    bool initialize(const QStringList &arguments, QString *errorMessage) final;
    void extensionsInitialized() final {}
};

class BinEditorFactory final : public Core::IEditorFactory
{
public:
    BinEditorFactory();
};

class FactoryServiceImpl : public QObject, public FactoryService
{
    Q_OBJECT
    Q_INTERFACES(BinEditor::FactoryService)

public:
    EditorService *createEditorService(const QString &title0, bool wantsEditor) final;
};

} // namespace Internal
} // namespace BinEditor
