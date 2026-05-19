// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <languageserverprotocol/languagefeatures.h>

#include <texteditor/formatter.h>

#include <QPointer>
#include <memory>
#include <variant>

namespace TextEditor { class TextDocument; }
namespace LanguageClient {

class Client;
class LanguageClientFormatter;
class IFormattingRequest
{
public:
    using RequestType = std::variant<std::monostate,
        LanguageServerProtocol::DocumentRangeFormattingRequest,
        LanguageServerProtocol::DocumentFormattingRequest>;
    IFormattingRequest(Client *, TextEditor::TextDocument *);
    virtual ~IFormattingRequest() = default;
    virtual RequestType prepareRequest(
        const QTextCursor &cursor,
        const TextEditor::TabSettingsData &settings,
        LanguageClientFormatter *formatter)
        = 0;
protected:
    QPointer<Client> m_client;
    TextEditor::TextDocument *m_document;
};

class RangeFormattingRequest : public IFormattingRequest
{
public:
    RangeFormattingRequest(Client *client, TextEditor::TextDocument *document);
    RequestType prepareRequest(
        const QTextCursor &cursor,
        const TextEditor::TabSettingsData &settings,
        LanguageClientFormatter *formatter) override;
};

class FullFormattingRequest : public IFormattingRequest
{
public:
    FullFormattingRequest(Client *client, TextEditor::TextDocument *document);
    RequestType prepareRequest(
        const QTextCursor &cursor,
        const TextEditor::TabSettingsData &settings,
        LanguageClientFormatter *formatter) override;
};

class LanguageClientFormatter : public TextEditor::Formatter
{
public:
    using ResponseType = LanguageServerProtocol::Response<
        LanguageServerProtocol::LanguageClientArray<LanguageServerProtocol::TextEdit>,
        std::nullptr_t>;

    LanguageClientFormatter(TextEditor::TextDocument *document, Client *client);
    ~LanguageClientFormatter() override;

    void format(const QTextCursor &cursor,
                const TextEditor::TabSettingsData &tabSettings,
                const TextEditor::FormatCallback &callback) override;

    void setMode(FormatMode mode) override;
    void handleResponse(const ResponseType &response);

private:
    void cancelCurrentRequest();

    QPointer<Client> m_client = nullptr; // not owned
    QMetaObject::Connection m_cancelConnection;
    TextEditor::TextDocument *m_document; // not owned
    bool m_ignoreCancel = false;
    TextEditor::FormatCallback m_formatCallback = {};
    std::optional<LanguageServerProtocol::MessageId> m_currentRequest;
    std::unique_ptr<IFormattingRequest> m_formattingRequester;
};

} // namespace LanguageClient
