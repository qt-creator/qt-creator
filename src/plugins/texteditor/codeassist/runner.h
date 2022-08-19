// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "iassistproposalwidget.h"

#include <QThread>

namespace TextEditor {

class IAssistProcessor;
class IAssistProposal;
class AssistInterface;

namespace Internal {

class ProcessorRunner : public QThread
{
    Q_OBJECT

public:
    ProcessorRunner();
    ~ProcessorRunner() override;

    void setProcessor(IAssistProcessor *processor); // Takes ownership of the processor.
    void setAssistInterface(AssistInterface *interface);
    void setDiscardProposal(bool discard);

    void run() override;

    IAssistProposal *proposal() const;

private:
    IAssistProcessor *m_processor = nullptr;
    AssistInterface *m_interface = nullptr;
    bool m_discardProposal = false;
    IAssistProposal *m_proposal = nullptr;
    AssistReason m_reason = IdleEditor;
};

} // Internal
} // TextEditor
