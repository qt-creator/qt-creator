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

#include "glsleditor.h"

#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/assistinterface.h>
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
    typedef QSharedPointer<Document> Ptr;

    Document();
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

    GLSL::Engine *_engine;
    GLSL::TranslationUnitAST *_ast;
    GLSL::Scope *_globalScope;
    QList<Range> _cursors;

    friend class GlslEditorWidget;
};

class GlslCompletionAssistInterface;

class GlslCompletionAssistProvider : public TextEditor::CompletionAssistProvider
{
    Q_OBJECT

public:
    bool supportsEditor(Core::Id editorId) const override;
    TextEditor::IAssistProcessor *createProcessor() const override;

    int activationCharSequenceLength() const override;
    bool isActivationCharSequence(const QString &sequence) const override;
};

class GlslCompletionAssistProcessor : public TextEditor::IAssistProcessor
{
public:
    GlslCompletionAssistProcessor();
    ~GlslCompletionAssistProcessor();

    TextEditor::IAssistProposal *perform(const TextEditor::AssistInterface *interface) override;

private:
    TextEditor::IAssistProposal *createHintProposal(const QVector<GLSL::Function *> &symbols);
    bool acceptsIdleEditor() const;

    int m_startPosition;
    QScopedPointer<const GlslCompletionAssistInterface> m_interface;
};

class GlslCompletionAssistInterface : public TextEditor::AssistInterface
{
public:
    GlslCompletionAssistInterface(QTextDocument *textDocument,
                                  int position, const QString &fileName,
                                  TextEditor::AssistReason reason,
                                  const QString &mimeType,
                                  const Document::Ptr &glslDoc);

    const QString &mimeType() const { return m_mimeType; }
    const Document::Ptr &glslDocument() const { return m_glslDoc; }

private:
    QString m_mimeType;
    Document::Ptr m_glslDoc;
};

} // namespace Internal
} // namespace GlslEditor
