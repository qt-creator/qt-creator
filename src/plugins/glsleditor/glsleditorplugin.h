// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>
#include <glsl/glsl.h>

namespace GlslEditor {
namespace Internal {

class GlslEditorPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "GLSLEditor.json")

public:
    GlslEditorPlugin() = default;
    ~GlslEditorPlugin() final;

    class InitFile
    {
    public:
        explicit InitFile(const QString &m_fileName);
        ~InitFile();

        GLSL::Engine *engine() const;
        GLSL::TranslationUnitAST *ast() const;

    private:
        void initialize() const;

        QString m_fileName;
        mutable GLSL::Engine *m_engine = nullptr;
        mutable GLSL::TranslationUnitAST *m_ast = nullptr;
    };

    static const InitFile *fragmentShaderInit(int variant);
    static const InitFile *vertexShaderInit(int variant);
    static const InitFile *shaderInit(int variant);

private:
    void initialize() final;
    void extensionsInitialized() final;
};

} // namespace Internal
} // namespace GlslEditor
