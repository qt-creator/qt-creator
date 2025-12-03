// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#pragma once

#include "effectcodeeditorwidget.h"

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <utils/uniqueobjectptr.h>

namespace EffectComposer {

class EffectComposerUniformsTableModel;

struct ShaderEditorData
{
    using Creator = std::function<
        ShaderEditorData *(const QString &fragmentShader, const QString &vertexShader)>;

    EffectComposerUniformsTableModel *tableModel = nullptr;
    std::function<QStringList()> uniformsCallback = nullptr;
    std::function<void()> exitEditorCallback = nullptr;

    TextEditor::TextDocumentPtr fragmentDocument;
    TextEditor::TextDocumentPtr vertexDocument;

    ~ShaderEditorData()
    {
        if (exitEditorCallback)
            exitEditorCallback();
    }

private:
    friend class EffectShadersCodeEditor;
    Utils::UniqueObjectLatePtr<EffectCodeEditorWidget> fragmentEditor;
    Utils::UniqueObjectLatePtr<EffectCodeEditorWidget> vertexEditor;

    ShaderEditorData() = default;
};

} // namespace EffectComposer
