// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cppeditor/builtineditordocumentprocessor.h>

#include <utils/id.h>

namespace ClangCodeModel {
namespace Internal {

class ClangEditorDocumentProcessor : public CppEditor::BuiltinEditorDocumentProcessor
{
    Q_OBJECT

public:
    ClangEditorDocumentProcessor(TextEditor::TextDocument *document);

    void semanticRehighlight() override;

    void setParserConfig(const CppEditor::BaseEditorDocumentParser::Configuration &config) override;
    CppEditor::BaseEditorDocumentParser::Configuration parserConfig();

    static ClangEditorDocumentProcessor *get(const Utils::FilePath &filePath);

signals:
    void parserConfigChanged(const Utils::FilePath &filePath,
                             const CppEditor::BaseEditorDocumentParser::Configuration &config);

private:
    void forceUpdate(TextEditor::TextDocument *doc) override;

    TextEditor::TextDocument &m_document;
};

} // namespace Internal
} // namespace ClangCodeModel
