/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "jsonrpcmessages.h"
#include "servercapabilities.h"

namespace LanguageServerProtocol {

class LANGUAGESERVERPROTOCOL_EXPORT DidOpenTextDocumentParams : public JsonObject
{
public:
    DidOpenTextDocumentParams() = default;
    explicit DidOpenTextDocumentParams(const TextDocumentItem &document);
    using JsonObject::JsonObject;

    TextDocumentItem textDocument() const { return typedValue<TextDocumentItem>(textDocumentKey); }
    void setTextDocument(TextDocumentItem textDocument) { insert(textDocumentKey, textDocument); }

    bool isValid(ErrorHierarchy *error) const override
    { return check<TextDocumentItem>(error, textDocumentKey); }
};


class LANGUAGESERVERPROTOCOL_EXPORT DidOpenTextDocumentNotification : public Notification<
        DidOpenTextDocumentParams>
{
public:
    explicit DidOpenTextDocumentNotification(const DidOpenTextDocumentParams &params);
    using Notification::Notification;
    constexpr static const char methodName[] = "textDocument/didOpen";
};

class LANGUAGESERVERPROTOCOL_EXPORT TextDocumentChangeRegistrationOptions : public JsonObject
{
public:
    TextDocumentChangeRegistrationOptions();
    explicit TextDocumentChangeRegistrationOptions(TextDocumentSyncKind kind);
    using JsonObject::JsonObject;

    TextDocumentSyncKind syncKind() const
    { return static_cast<TextDocumentSyncKind>(typedValue<int>(syncKindKey)); }
    void setSyncKind(TextDocumentSyncKind syncKind)
    { insert(syncKindKey, static_cast<int>(syncKind)); }

    bool isValid(ErrorHierarchy *error) const override { return check<int>(error, syncKindKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT DidChangeTextDocumentParams : public JsonObject
{
public:
    DidChangeTextDocumentParams();
    explicit DidChangeTextDocumentParams(const VersionedTextDocumentIdentifier &docId,
                                         const QString &text = QString());
    using JsonObject::JsonObject;

    VersionedTextDocumentIdentifier textDocument() const
    { return typedValue<VersionedTextDocumentIdentifier>(textDocumentKey); }
    void setTextDocument(const VersionedTextDocumentIdentifier &textDocument)
    { insert(textDocumentKey, textDocument); }

    class LANGUAGESERVERPROTOCOL_EXPORT TextDocumentContentChangeEvent : public JsonObject
    {
        /*
         * An event describing a change to a text document. If range and rangeLength are omitted
         * the new text is considered to be the full content of the document.
         */
    public:
        TextDocumentContentChangeEvent() = default;
        explicit TextDocumentContentChangeEvent(const QString &text);
        using JsonObject::JsonObject;

        // The range of the document that changed.
        Utils::optional<Range> range() const { return optionalValue<Range>(rangeKey); }
        void setRange(Range range) { insert(rangeKey, range); }
        void clearRange() { remove(rangeKey); }

        // The length of the range that got replaced.
        Utils::optional<int> rangeLength() const { return optionalValue<int>(rangeLengthKey); }
        void setRangeLength(int rangeLength) { insert(rangeLengthKey, rangeLength); }
        void clearRangeLength() { remove(rangeLengthKey); }

        // The new text of the range/document.
        QString text() const { return typedValue<QString>(textKey); }
        void setText(const QString &text) { insert(textKey, text); }

        bool isValid(ErrorHierarchy *error) const override;
    };

    QList<TextDocumentContentChangeEvent> contentChanges() const
    { return array<TextDocumentContentChangeEvent>(contentChangesKey); }
    void setContentChanges(const QList<TextDocumentContentChangeEvent> &contentChanges)
    { insertArray(contentChangesKey, contentChanges); }

    bool isValid(ErrorHierarchy *error) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT DidChangeTextDocumentNotification : public Notification<
        DidChangeTextDocumentParams>
{
public:
    explicit DidChangeTextDocumentNotification(const DidChangeTextDocumentParams &params);
    using Notification::Notification;
    constexpr static const char methodName[] = "textDocument/didChange";
};

class LANGUAGESERVERPROTOCOL_EXPORT WillSaveTextDocumentParams : public JsonObject
{
public:
    enum class TextDocumentSaveReason  {
        Manual = 1,
        AfterDelay = 2,
        FocusOut = 3
    };

    WillSaveTextDocumentParams() : WillSaveTextDocumentParams(TextDocumentIdentifier()) {}
    explicit WillSaveTextDocumentParams(
        const TextDocumentIdentifier &document,
        const TextDocumentSaveReason &reason = TextDocumentSaveReason::Manual);
    using JsonObject::JsonObject;

    TextDocumentIdentifier textDocument() const
    { return typedValue<TextDocumentIdentifier>(textDocumentKey); }
    void setTextDocument(const TextDocumentIdentifier &textDocument)
    { insert(textDocumentKey, textDocument); }

    TextDocumentSaveReason reason() const
    { return static_cast<TextDocumentSaveReason>(typedValue<int>(reasonKey)); }
    void setReason(TextDocumentSaveReason reason) { insert(reasonKey, static_cast<int>(reason)); }

    bool isValid(ErrorHierarchy *error) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT WillSaveTextDocumentNotification : public Notification<
        WillSaveTextDocumentParams>
{
public:
    explicit WillSaveTextDocumentNotification(const WillSaveTextDocumentParams &params);
    using Notification::Notification;
    constexpr static const char methodName[] = "textDocument/willSave";
};

class LANGUAGESERVERPROTOCOL_EXPORT WillSaveWaitUntilTextDocumentRequest : public Request<
        LanguageClientArray<TextEdit>, std::nullptr_t, WillSaveTextDocumentParams>
{
public:
    explicit WillSaveWaitUntilTextDocumentRequest(const WillSaveTextDocumentParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/willSaveWaitUntil";
};

class LANGUAGESERVERPROTOCOL_EXPORT TextDocumentSaveRegistrationOptions
        : public TextDocumentRegistrationOptions
{
public:
    using TextDocumentRegistrationOptions::TextDocumentRegistrationOptions;

    Utils::optional<bool> includeText() const { return optionalValue<bool>(includeTextKey); }
    void setIncludeText(bool includeText) { insert(includeTextKey, includeText); }
    void clearIncludeText() { remove(includeTextKey); }

    bool isValid(ErrorHierarchy *error) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT DidSaveTextDocumentParams : public JsonObject
{
public:
    DidSaveTextDocumentParams() : DidSaveTextDocumentParams(TextDocumentIdentifier()) {}
    explicit DidSaveTextDocumentParams(const TextDocumentIdentifier &document);
    using JsonObject::JsonObject;

    TextDocumentIdentifier textDocument()
    const { return typedValue<TextDocumentIdentifier>(textDocumentKey); }
    void setTextDocument(TextDocumentIdentifier textDocument)
    { insert(textDocumentKey, textDocument); }

    Utils::optional<QString> text() const { return optionalValue<QString>(textKey); }
    void setText(const QString &text) { insert(textKey, text); }
    void clearText() { remove(textKey); }

    bool isValid(ErrorHierarchy *error) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT DidSaveTextDocumentNotification : public Notification<
        DidSaveTextDocumentParams>
{
public:
    explicit DidSaveTextDocumentNotification(const DidSaveTextDocumentParams &params);
    using Notification::Notification;
    constexpr static const char methodName[] = "textDocument/didSave";
};

class LANGUAGESERVERPROTOCOL_EXPORT DidCloseTextDocumentParams : public JsonObject
{
public:
    DidCloseTextDocumentParams() = default;
    explicit DidCloseTextDocumentParams(const TextDocumentIdentifier &document);
    using JsonObject::JsonObject;

    TextDocumentIdentifier textDocument() const
    { return typedValue<TextDocumentIdentifier>(textDocumentKey); }
    void setTextDocument(const TextDocumentIdentifier &textDocument)
    { insert(textDocumentKey, textDocument); }

    bool isValid(ErrorHierarchy *error) const override
    { return check<TextDocumentIdentifier>(error, textDocumentKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT DidCloseTextDocumentNotification : public Notification<
        DidCloseTextDocumentParams>
{
public:
    explicit DidCloseTextDocumentNotification(const DidCloseTextDocumentParams &params);
    using Notification::Notification;
    constexpr static const char methodName[] = "textDocument/didClose";
};

} // namespace LanguageClient
