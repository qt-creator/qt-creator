// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/diffservice.h>
#include <extensionsystem/iplugin.h>

namespace DiffEditor::Internal {

class DiffEditorServiceImpl : public QObject, public Core::DiffService
{
    Q_OBJECT
    Q_INTERFACES(Core::DiffService)

public:
    DiffEditorServiceImpl();

    void diffFiles(const QString &leftFileName, const QString &rightFileName) override;
    void diffModifiedFiles(const QStringList &fileNames) override;
};

class DiffEditorPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "DiffEditor.json")

public:
    DiffEditorPlugin();
    ~DiffEditorPlugin();

    void initialize() final;

private:
    class DiffEditorPluginPrivate *d = nullptr;

#ifdef WITH_TESTS
private slots:
    void testMakePatch_data();
    void testMakePatch();
    void testReadPatch_data();
    void testReadPatch();
    void testFilterPatch_data();
    void testFilterPatch();
#endif // WITH_TESTS
};

} // namespace DiffEditor::Internal
