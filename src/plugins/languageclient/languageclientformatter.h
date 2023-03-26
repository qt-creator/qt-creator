// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <languageserverprotocol/languagefeatures.h>

#include <texteditor/formatter.h>

#include <QPointer>

namespace TextEditor { class TextDocument; }
namespace LanguageClient {

class Client;

class LanguageClientFormatter : public TextEditor::Formatter
{
public:
    LanguageClientFormatter(TextEditor::TextDocument *document, Client *client);
    ~LanguageClientFormatter() override;

    QFutureWatcher<Utils::ChangeSet> *format(
        const QTextCursor &cursor, const TextEditor::TabSettings &tabSettings) override;

private:
    void cancelCurrentRequest();
    void handleResponse(
        const LanguageServerProtocol::DocumentRangeFormattingRequest::Response &response);

    QPointer<Client> m_client = nullptr; // not owned
    QMetaObject::Connection m_cancelConnection;
    TextEditor::TextDocument *m_document; // not owned
    bool m_ignoreCancel = false;
    QFutureInterface<Utils::ChangeSet> m_progress;
    std::optional<LanguageServerProtocol::MessageId> m_currentRequest;
};

} // namespace LanguageClient
