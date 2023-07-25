// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace Designer {
namespace Internal {

class FormEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Designer.json")

public:
    FormEditorPlugin() = default;
    ~FormEditorPlugin() override;

#ifdef WITH_TESTS
private slots:
    void test_gotoslot();
    void test_gotoslot_data();
#endif

private:
    bool initialize(const QStringList &arguments, QString *errorString) override;
    void extensionsInitialized() override;

    void switchSourceForm();
    void initializeTemplates();

    class FormEditorPluginPrivate *d = nullptr;
};

} // namespace Internal
} // namespace Designer
