// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

#include <glsl/glsl.h>

namespace GlslEditor::Internal {

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

const InitFile *fragmentShaderInit(int variant);
const InitFile *vertexShaderInit(int variant);
const InitFile *shaderInit(int variant);

} // GlslEditor
