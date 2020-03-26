/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <languageserverprotocol/icontent.h>
#include <languageserverprotocol/languagefeatures.h>

#include <texteditor/formatter.h>

namespace TextEditor { class TextDocument; }
namespace LanguageClient {

class Client;

class LanguageClientFormatter : public TextEditor::Formatter
{
public:
    LanguageClientFormatter(TextEditor::TextDocument *document, Client *client);
    ~LanguageClientFormatter() override;

    QFutureWatcher<Utils::Text::Replacements> *format(
        const QTextCursor &cursor, const TextEditor::TabSettings &tabSettings) override;

private:
    void cancelCurrentRequest();
    void handleResponse(
        const LanguageServerProtocol::DocumentRangeFormattingRequest::Response &response);

    Client *m_client = nullptr; // not owned
    QMetaObject::Connection m_cancelConnection;
    TextEditor::TextDocument *m_document; // not owned
    bool m_ignoreCancel = false;
    QFutureInterface<Utils::Text::Replacements> m_progress;
    Utils::optional<LanguageServerProtocol::MessageId> m_currentRequest;
};

} // namespace LanguageClient
