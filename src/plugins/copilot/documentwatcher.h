// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "copilotclient.h"

#include <texteditor/textdocument.h>

#include <QTimer>

namespace Copilot::Internal {

class DocumentWatcher : public QObject
{
    Q_OBJECT
public:
    explicit DocumentWatcher(CopilotClient *client, TextEditor::TextDocument *textDocument);

    void getSuggestion();

private:
    CopilotClient *m_client;
    TextEditor::TextDocument *m_textDocument;

    QTimer m_debounceTimer;
    bool m_isEditing = false;
    int m_lastContentSize = 0;
};

} // namespace Copilot::Internal
