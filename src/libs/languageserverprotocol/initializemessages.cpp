// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "initializemessages.h"

#include <QCoreApplication>

namespace LanguageServerProtocol {

constexpr const char InitializeRequest::methodName[];
constexpr const char InitializeNotification::methodName[];
constexpr Trace::Values s_trace = Trace::off;

Trace Trace::fromString(const QString &val)
{
    if (val == "messages")
        return messages;
    if (val == "verbose")
        return verbose;
    return off;
}

#define RETURN_CASE(name) case Trace::name: return QString(#name);
QString Trace::toString() const
{
    switch (m_value) {
    RETURN_CASE(off);
    RETURN_CASE(messages);
    RETURN_CASE(verbose);
    }
    return QString("off");
}
#undef RETURN_CASE

Utils::optional<QList<MarkupKind>>
TextDocumentClientCapabilities::CompletionCapabilities::CompletionItemCapbilities::
documentationFormat() const
{
    return optionalArray<MarkupKind>(documentationFormatKey);
}

void
TextDocumentClientCapabilities::CompletionCapabilities::CompletionItemCapbilities::
setDocumentationFormat(const QList<MarkupKind> &documentationFormat)
{
    insertArray(documentationFormatKey, documentationFormat);
}

TextDocumentClientCapabilities::CompletionCapabilities::CompletionItemKindCapabilities::CompletionItemKindCapabilities()
{
    setValueSet({CompletionItemKind::Text, CompletionItemKind::Method, CompletionItemKind::Function,
                 CompletionItemKind::Constructor, CompletionItemKind::Field, CompletionItemKind::Variable,
                 CompletionItemKind::Class, CompletionItemKind::Interface, CompletionItemKind::Module,
                 CompletionItemKind::Property, CompletionItemKind::Unit, CompletionItemKind::Value,
                 CompletionItemKind::Enum, CompletionItemKind::Keyword, CompletionItemKind::Snippet,
                 CompletionItemKind::Color, CompletionItemKind::File, CompletionItemKind::Reference,
                 CompletionItemKind::Folder, CompletionItemKind::EnumMember, CompletionItemKind::Constant,
                 CompletionItemKind::Struct, CompletionItemKind::Event, CompletionItemKind::Operator,
                 CompletionItemKind::TypeParameter});
}

Utils::optional<QList<CompletionItemKind::Kind>>
TextDocumentClientCapabilities::CompletionCapabilities::CompletionItemKindCapabilities::
valueSet() const
{
    if (Utils::optional<QList<int>> array = optionalArray<int>(valueSetKey)) {
        return Utils::make_optional(Utils::transform(*array, [] (int value) {
            return static_cast<CompletionItemKind::Kind>(value);
        }));
    }
    return Utils::nullopt;
}

void
TextDocumentClientCapabilities::CompletionCapabilities::CompletionItemKindCapabilities::
setValueSet(const QList<CompletionItemKind::Kind> &valueSet)
{
    insert(valueSetKey, enumArrayToJsonArray<CompletionItemKind::Kind>(valueSet));
}

Utils::optional<QList<MarkupKind> > TextDocumentClientCapabilities::HoverCapabilities::contentFormat() const
{
    return optionalArray<MarkupKind>(contentFormatKey);
}

void TextDocumentClientCapabilities::HoverCapabilities::setContentFormat(const QList<MarkupKind> &contentFormat)
{
    insertArray(contentFormatKey, contentFormat);
}

Utils::optional<QList<MarkupKind>>
TextDocumentClientCapabilities::SignatureHelpCapabilities::SignatureInformationCapabilities::
documentationFormat() const
{
    return optionalArray<MarkupKind>(documentationFormatKey);
}

void
TextDocumentClientCapabilities::SignatureHelpCapabilities::SignatureInformationCapabilities::
setDocumentationFormat(const QList<MarkupKind> &documentationFormat)
{
    insertArray(documentationFormatKey, documentationFormat);
}

InitializeParams::InitializeParams()
{
    setProcessId(int(QCoreApplication::applicationPid()));
    setRootUri(LanguageClientValue<DocumentUri>());
    setCapabilities(ClientCapabilities());
    setTrace(s_trace);
}

Utils::optional<QJsonObject> InitializeParams::initializationOptions() const
{
    const QJsonValue &optionsValue = value(initializationOptionsKey);
    if (optionsValue.isObject())
        return optionsValue.toObject();
    return Utils::nullopt;
}

Utils::optional<Trace> InitializeParams::trace() const
{
    const QJsonValue &traceValue = value(traceKey);
    if (traceValue.isUndefined())
        return Utils::nullopt;
    return Utils::make_optional(Trace(traceValue.toString()));
}

InitializeRequest::InitializeRequest(const InitializeParams &params)
    : Request(methodName, params)
{ }

InitializeNotification::InitializeNotification(const InitializedParams &params)
    : Notification(methodName, params)
{ }

} // namespace LanguageServerProtocol
