// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "assistenums.h"
#include "iassistproposalmodel.h"

#include <texteditor/texteditor_global.h>

#include <utils/id.h>

namespace TextEditor {

class IAssistProposalWidget;
class TextEditorWidget;

class TEXTEDITOR_EXPORT IAssistProposal
{
public:
    IAssistProposal(Utils::Id id,int basePosition);
    virtual ~IAssistProposal();

    int basePosition() const;
    virtual bool hasItemsToPropose(const QString &, AssistReason) const { return true; }
    virtual bool isCorrective(TextEditorWidget *editorWidget) const;
    virtual void makeCorrection(TextEditorWidget *editorWidget);
    virtual TextEditor::ProposalModelPtr model() const = 0;
    virtual IAssistProposalWidget *createWidget() const = 0;

    Utils::Id id() const { return m_id; }

protected:
    Utils::Id m_id;
    int m_basePosition;
};

} // TextEditor
