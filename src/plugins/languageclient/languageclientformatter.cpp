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

#include "languageclientformatter.h"

#include "client.h"
#include "languageclientutils.h"

#include <texteditor/tabsettings.h>
#include <texteditor/textdocument.h>
#include <utils/mimetypes/mimedatabase.h>

#include <QTextDocument>

using namespace LanguageServerProtocol;
using namespace Utils;

namespace LanguageClient {

LanguageClientFormatter::LanguageClientFormatter(TextEditor::TextDocument *document, Client *client)
    : m_client(client)
    , m_document(document)
{
    m_cancelConnection = QObject::connect(document->document(),
                                          &QTextDocument::contentsChanged,
                                          [this]() {
        if (m_ignoreCancel)
            m_ignoreCancel = false;
        else
            cancelCurrentRequest();
    });
}

LanguageClientFormatter::~LanguageClientFormatter()
{
    QObject::disconnect(m_cancelConnection);
    cancelCurrentRequest();
}

static const FormattingOptions formattingOptions(const TextEditor::TabSettings &settings)
{
    FormattingOptions options;
    options.setTabSize(settings.m_tabSize);
    options.setInsertSpace(settings.m_tabPolicy == TextEditor::TabSettings::SpacesOnlyTabPolicy);
    return options;
}

QFutureWatcher<Utils::Text::Replacements> *LanguageClientFormatter::format(
        const QTextCursor &cursor, const TextEditor::TabSettings &tabSettings)
{
    cancelCurrentRequest();
    m_progress = QFutureInterface<Utils::Text::Replacements>();

    const FilePath &filePath = m_document->filePath();
    const DynamicCapabilities dynamicCapabilities = m_client->dynamicCapabilities();
    const QString method(DocumentRangeFormattingRequest::methodName);
    if (Utils::optional<bool> registered = dynamicCapabilities.isRegistered(method)) {
        if (!registered.value())
            return nullptr;
        const TextDocumentRegistrationOptions option(dynamicCapabilities.option(method).toObject());
        if (option.isValid(nullptr)
                && !option.filterApplies(filePath, Utils::mimeTypeForName(m_document->mimeType()))) {
            return nullptr;
        }
    } else if (!m_client->capabilities().documentRangeFormattingProvider().value_or(false)) {
        return nullptr;
    }
    DocumentRangeFormattingParams params;
    const DocumentUri uri = DocumentUri::fromFilePath(filePath);
    params.setTextDocument(TextDocumentIdentifier(uri));
    params.setOptions(formattingOptions(tabSettings));
    if (!cursor.hasSelection()) {
        QTextCursor c = cursor;
        c.select(QTextCursor::LineUnderCursor);
        params.setRange(Range(c));
    } else {
        params.setRange(Range(cursor));
    }
    DocumentRangeFormattingRequest request(params);
    request.setResponseCallback([this](const DocumentRangeFormattingRequest::Response &response) {
        handleResponse(response);
    });
    m_currentRequest = request.id();
    m_client->sendContent(request);
    // ignore first contents changed, because this function is called inside a begin/endEdit block
    m_ignoreCancel = true;
    m_progress.reportStarted();
    auto watcher = new QFutureWatcher<Text::Replacements>();
    watcher->setFuture(m_progress.future());
    QObject::connect(watcher, &QFutureWatcher<Text::Replacements>::canceled, [this]() {
        cancelCurrentRequest();
    });
    return watcher;
}

void LanguageClientFormatter::cancelCurrentRequest()
{
    if (m_currentRequest.has_value()) {
        m_progress.reportCanceled();
        m_progress.reportFinished();
        m_client->cancelRequest(*m_currentRequest);
        m_ignoreCancel = false;
        m_currentRequest = nullopt;
    }
}

void LanguageClientFormatter::handleResponse(const DocumentRangeFormattingRequest::Response &response)
{
    m_currentRequest = nullopt;
    if (const optional<DocumentRangeFormattingRequest::Response::Error> &error = response.error())
        m_client->log(*error);
    Text::Replacements replacements;
    if (optional<LanguageClientArray<TextEdit>> result = response.result()) {
        if (!result->isNull()) {
            const QList<TextEdit> results = result->toList();
            replacements.reserve(results.size());
            for (const TextEdit &edit : results)
                replacements.emplace_back(edit.toReplacement(m_document->document()));
        }
    }
    m_progress.reportResult(replacements);
    m_progress.reportFinished();
}

} // namespace LanguageClient
