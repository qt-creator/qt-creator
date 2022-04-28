/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
    static ClangEditorDocumentProcessor *get(const QString &filePath);

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
