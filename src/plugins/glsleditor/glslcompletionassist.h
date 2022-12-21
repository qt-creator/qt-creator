// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "glsleditor.h"

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/asyncprocessor.h>
#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/ifunctionhintproposalmodel.h>

#include <QScopedPointer>
#include <QSharedPointer>

namespace GLSL {
class Engine;
class Function;
class TranslationUnitAST;
class Scope;
} // namespace GLSL

namespace TextEditor { class AssistProposalItem; }

namespace GlslEditor {
namespace Internal {

class Document
{
public:
    using Ptr = QSharedPointer<Document>;

    ~Document();

    GLSL::Engine *engine() const { return _engine; }
    GLSL::TranslationUnitAST *ast() const { return _ast; }
    GLSL::Scope *globalScope() const { return _globalScope; }

    GLSL::Scope *scopeAt(int position) const;
    void addRange(const QTextCursor &cursor, GLSL::Scope *scope);

private:
    struct Range {
        QTextCursor cursor;
        GLSL::Scope *scope;
    };

    GLSL::Engine *_engine = nullptr;
    GLSL::TranslationUnitAST *_ast = nullptr;
    GLSL::Scope *_globalScope = nullptr;
    QList<Range> _cursors;

    friend class GlslEditorWidget;
};

class GlslCompletionAssistInterface;

class GlslCompletionAssistProvider : public TextEditor::CompletionAssistProvider
{
    Q_OBJECT

public:
    TextEditor::IAssistProcessor *createProcessor(const TextEditor::AssistInterface *) const override;

    int activationCharSequenceLength() const override;
    bool isActivationCharSequence(const QString &sequence) const override;
};

class GlslCompletionAssistProcessor : public TextEditor::AsyncProcessor
{
public:
    ~GlslCompletionAssistProcessor() override;

    TextEditor::IAssistProposal *performAsync() override;

private:
    TextEditor::IAssistProposal *createHintProposal(const QVector<GLSL::Function *> &symbols);
    bool acceptsIdleEditor() const;

    int m_startPosition = 0;
};

class GlslCompletionAssistInterface : public TextEditor::AssistInterface
{
public:
    GlslCompletionAssistInterface(const QTextCursor &cursor, const Utils::FilePath &fileName,
                                  TextEditor::AssistReason reason,
                                  const QString &mimeType,
                                  const Document::Ptr &glslDoc);

    const QString &mimeType() const { return m_mimeType; }
    const Document::Ptr &glslDocument() const { return m_glslDoc; }
    bool isBaseObject() const override { return false; }

private:
    QString m_mimeType;
    Document::Ptr m_glslDoc;
};

} // namespace Internal
} // namespace GlslEditor
