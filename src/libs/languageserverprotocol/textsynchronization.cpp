// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textsynchronization.h"

namespace LanguageServerProtocol {

constexpr const char DidOpenTextDocumentNotification::methodName[];
constexpr const char DidChangeTextDocumentNotification::methodName[];
constexpr const char WillSaveTextDocumentNotification::methodName[];
constexpr const char WillSaveWaitUntilTextDocumentRequest::methodName[];
constexpr const char DidSaveTextDocumentNotification::methodName[];
constexpr const char DidCloseTextDocumentNotification::methodName[];

DidOpenTextDocumentNotification::DidOpenTextDocumentNotification(
        const DidOpenTextDocumentParams &params)
    : Notification(methodName, params)
{ }

DidChangeTextDocumentNotification::DidChangeTextDocumentNotification(
        const DidChangeTextDocumentParams &params)
    : DidChangeTextDocumentNotification(methodName, params)
{ }

WillSaveTextDocumentNotification::WillSaveTextDocumentNotification(
        const WillSaveTextDocumentParams &params)
    : Notification(methodName, params)
{ }

WillSaveWaitUntilTextDocumentRequest::WillSaveWaitUntilTextDocumentRequest(const WillSaveTextDocumentParams &params)
    : Request(methodName, params)
{ }

DidSaveTextDocumentNotification::DidSaveTextDocumentNotification(
        const DidSaveTextDocumentParams &params)
    : Notification(methodName, params)
{ }

DidCloseTextDocumentNotification::DidCloseTextDocumentNotification(
        const DidCloseTextDocumentParams &params)
    : Notification(methodName, params)
{ }

DidChangeTextDocumentParams::DidChangeTextDocumentParams()
    : DidChangeTextDocumentParams(VersionedTextDocumentIdentifier())
{ }

DidChangeTextDocumentParams::DidChangeTextDocumentParams(
        const VersionedTextDocumentIdentifier &docId, const QString &text)
{
    setTextDocument(docId);
    setContentChanges({TextDocumentContentChangeEvent(text)});
}

DidOpenTextDocumentParams::DidOpenTextDocumentParams(const TextDocumentItem &document)
{
    setTextDocument(document);
}

DidCloseTextDocumentParams::DidCloseTextDocumentParams(const TextDocumentIdentifier &document)
{
    setTextDocument(document);
}

DidChangeTextDocumentParams::TextDocumentContentChangeEvent::TextDocumentContentChangeEvent(
    const QString &text)
{
    setText(text);
}

DidSaveTextDocumentParams::DidSaveTextDocumentParams(const TextDocumentIdentifier &document)
{
    setTextDocument(document);
}

WillSaveTextDocumentParams::WillSaveTextDocumentParams(
        const TextDocumentIdentifier &document,
        const WillSaveTextDocumentParams::TextDocumentSaveReason &reason)
{
    setTextDocument(document);
    setReason(reason);
}

} // namespace LanguageServerProtocol
