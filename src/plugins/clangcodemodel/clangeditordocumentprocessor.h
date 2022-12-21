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

    bool hasProjectPart() const;
    CppEditor::ProjectPart::ConstPtr projectPart() const;
    void clearProjectPart();

    ::Utils::Id diagnosticConfigId() const;

    void setParserConfig(const CppEditor::BaseEditorDocumentParser::Configuration &config) override;
    CppEditor::BaseEditorDocumentParser::Configuration parserConfig();

public:
    static ClangEditorDocumentProcessor *get(const Utils::FilePath &filePath);

signals:
    void parserConfigChanged(const Utils::FilePath &filePath,
                             const CppEditor::BaseEditorDocumentParser::Configuration &config);

private:
    TextEditor::TextDocument &m_document;
    CppEditor::ProjectPart::ConstPtr m_projectPart;
    ::Utils::Id m_diagnosticConfigId;
};

} // namespace Internal
} // namespace ClangCodeModel
