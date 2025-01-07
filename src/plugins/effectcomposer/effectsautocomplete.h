// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/codeassist/assistenums.h>
#include <texteditor/codeassist/asyncprocessor.h>
#include <qmljseditor/qmljscompletionassist.h>

namespace TextEditor {
class AssistProposalItemInterface;
class IAssistProposal;
} // namespace TextEditor

namespace EffectComposer {

class EffectsCompletionAssistInterface : public QmlJSEditor::QmlJSCompletionAssistInterface
{
public:
    EffectsCompletionAssistInterface(
        const QTextCursor &cursor,
        const Utils::FilePath &fileName,
        TextEditor::AssistReason reason,
        const QmlJSTools::SemanticInfo &info,
        const QStringList &uniforms)
        : QmlJSEditor::QmlJSCompletionAssistInterface(cursor, fileName, reason, info)
        , m_uniformNames(uniforms)
    {}

    QStringList uniformNames() const { return m_uniformNames; }

private:
    QStringList m_uniformNames;
};

class EffectsCompletionAssistProcessor : public TextEditor::AsyncProcessor
{
public:
    EffectsCompletionAssistProcessor();
    ~EffectsCompletionAssistProcessor() override = default;

    TextEditor::IAssistProposal *performAsync() override;

private:
    int m_startPosition;
    QList<TextEditor::AssistProposalItemInterface *> m_completions;
};

class EffectsCompeletionAssistProvider : public QmlJSEditor::QmlJSCompletionAssistProvider
{
    TextEditor::IAssistProcessor *createProcessor(const TextEditor::AssistInterface *) const override;
};

} // namespace EffectComposer
