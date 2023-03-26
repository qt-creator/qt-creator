// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <texteditor/codeassist/asyncprocessor.h>
#include <texteditor/snippets/snippetassistcollector.h>

#include <functional>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

namespace CPlusPlus { struct LanguageFeatures; }

namespace CppEditor {

class CPPEDITOR_EXPORT CppCompletionAssistProcessor : public TextEditor::AsyncProcessor
{
public:
    explicit CppCompletionAssistProcessor(int snippetItemOrder = 0);

    static const QStringList preprocessorCompletions();

protected:
    void addSnippets();

    using DotAtIncludeCompletionHandler = std::function<void(int &startPosition, unsigned *kind)>;
    static void startOfOperator(QTextDocument *textDocument,
                                int positionInDocument,
                                unsigned *kind,
                                int &start,
                                const CPlusPlus::LanguageFeatures &languageFeatures,
                                bool adjustForQt5SignalSlotCompletion = false,
                                DotAtIncludeCompletionHandler dotAtIncludeCompletionHandler
                                    = DotAtIncludeCompletionHandler());

    int m_positionForProposal = -1;
    QList<TextEditor::AssistProposalItemInterface *> m_completions;
    TextEditor::IAssistProposal *m_hintProposal = nullptr;

private:
    TextEditor::SnippetAssistCollector m_snippetCollector;
};

} // namespace CppEditor
