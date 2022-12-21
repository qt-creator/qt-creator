// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/iassistprovider.h>

#include <cplusplus/CppDocument.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/TypeOfExpression.h>

#include <QSharedPointer>
#include <QTextCursor>

namespace TextEditor {
class AssistProposalItemInterface;
class IAssistProposalWidget;
}

namespace CppEditor {

class CPPEDITOR_EXPORT VirtualFunctionProposal : public TextEditor::GenericProposal
{
public:
    VirtualFunctionProposal(int cursorPos,
                            const QList<TextEditor::AssistProposalItemInterface *> &items,
                            bool openInSplit);

private:
    TextEditor::IAssistProposalWidget *createWidget() const override;

    bool m_openInSplit;
};

class CPPEDITOR_EXPORT VirtualFunctionAssistProvider : public TextEditor::IAssistProvider
{
    Q_OBJECT
public:
    VirtualFunctionAssistProvider();

    struct Parameters {
        CPlusPlus::Function *function = nullptr;
        CPlusPlus::Class *staticClass = nullptr;
        QSharedPointer<CPlusPlus::TypeOfExpression> typeOfExpression; // Keeps instantiated symbols.
        CPlusPlus::Snapshot snapshot;
        int cursorPosition = -1;
        bool openInNextSplit = false;
    };

    virtual bool configure(const Parameters &parameters);
    Parameters params() const { return m_params; }
    void clearParams() { m_params = Parameters(); }

    TextEditor::IAssistProcessor *createProcessor(const TextEditor::AssistInterface *) const override;

private:
    Parameters m_params;
};

} // namespace CppEditor
