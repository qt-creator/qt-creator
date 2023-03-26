// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "completion.h"

namespace LanguageServerProtocol {

constexpr const char CompletionRequest::methodName[];
constexpr const char CompletionItemResolveRequest::methodName[];

CompletionRequest::CompletionRequest(const CompletionParams &params)
    : Request(methodName, params)
{ }

std::optional<MarkupOrString> CompletionItem::documentation() const
{
    QJsonValue documentation = value(documentationKey);
    if (documentation.isUndefined())
        return std::nullopt;
    return MarkupOrString(documentation);
}

std::optional<CompletionItem::InsertTextFormat> CompletionItem::insertTextFormat() const
{
    if (std::optional<int> value = optionalValue<int>(insertTextFormatKey))
        return std::make_optional(CompletionItem::InsertTextFormat(*value));
    return std::nullopt;
}

std::optional<QList<CompletionItem::CompletionItemTag>> CompletionItem::tags() const
{
    if (const auto value = optionalValue<QJsonArray>(tagsKey)) {
        QList<CompletionItemTag> tags;
        for (auto it = value->cbegin(); it != value->cend(); ++it)
            tags << static_cast<CompletionItemTag>(it->toInt());
        return tags;
    }
    return {};
}

CompletionItemResolveRequest::CompletionItemResolveRequest(const CompletionItem &params)
    : Request(methodName, params)
{ }

CompletionResult::CompletionResult(const QJsonValue &value)
{
    if (value.isNull()) {
        emplace<std::nullptr_t>(nullptr);
    } else if (value.isArray()) {
        QList<CompletionItem> items;
        for (auto arrayElement : value.toArray())
            items << CompletionItem(arrayElement);
        emplace<QList<CompletionItem>>(items);
    } else if (value.isObject()) {
        emplace<CompletionList>(CompletionList(value));
    }
}

} // namespace LanguageServerProtocol
