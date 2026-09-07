// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"

#include <languageserverprotocol/lspjsonrpc.h>
#include <texteditor/basehoverhandler.h>

#include <QTextCursor>

#include <functional>

namespace LanguageClient {

class Client;

using HelpItemProvider = std::function<void(
    const LanguageServerProtocol::MessageId &id,
    const LanguageServerProtocol::Hover &hover,
    const Utils::FilePath &path)>;

class LANGUAGECLIENT_EXPORT HoverHandler final : public TextEditor::BaseHoverHandler
{
public:
    explicit HoverHandler(Client *client);
    ~HoverHandler() override;

    void abort() override;

    /// If prefer diagnostics is enabled the hover handler checks whether a diagnostics is at the
    /// pos passed to identifyMatch _before_ sending hover request to the server. If a diagnostic
    /// can be found it will be used as a tooltip and no hover request is sent to the server.
    /// If prefer diagnostics is disabled the diagnostics are only checked if the response is empty.
    /// Defaults to prefer diagnostics.
    void setPreferDiagnosticts(bool prefer);

    void setHelpItemProvider(const HelpItemProvider &provider) { m_helpItemProvider = provider; }
    void setHelpItem(const LanguageServerProtocol::MessageId &msgId, const Core::HelpItem &help);

protected:
    void identifyMatch(TextEditor::TextEditorWidget *editorWidget,
                       int pos,
                       ReportPriority report) override;

private:
    void handleResponse(
        const LanguageServerProtocol::MessageId &id,
        const Utils::Result<LanguageServerProtocol::HoverRequestResult> &result,
        const QTextCursor &cursor);
    void setContent(const LanguageServerProtocol::HoverContents &contents);
    bool reportDiagnostics(const QTextCursor &cursor);

    QPointer<Client> m_client;
    std::optional<LanguageServerProtocol::MessageId> m_currentRequest;
    Utils::FilePath m_filePath;
    // Kept while the help item provider works on it.
    std::optional<std::pair<LanguageServerProtocol::MessageId, LanguageServerProtocol::Hover>>
        m_pendingHover;
    TextEditor::BaseHoverHandler::ReportPriority m_report;
    HelpItemProvider m_helpItemProvider;
    bool m_preferDiagnostics = true;
};

} // namespace LanguageClient
