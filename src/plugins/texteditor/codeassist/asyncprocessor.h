// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "iassistprocessor.h"

#include <QFutureWatcher>

namespace TextEditor {

class TEXTEDITOR_EXPORT AsyncProcessor : public TextEditor::IAssistProcessor
{
public:
    AsyncProcessor();

    IAssistProposal *perform(AssistInterface *interface) final;
    bool running() override;
    void cancel() override;

    virtual IAssistProposal *performAsync(AssistInterface *interface) = 0;
    virtual IAssistProposal *immediateProposal(AssistInterface *interface);

protected:
    bool isCanceled() const;

private:
    AssistInterface *m_interface = nullptr;
    QFutureWatcher<IAssistProposal *> m_watcher;
};

} // namespace TextEditor
