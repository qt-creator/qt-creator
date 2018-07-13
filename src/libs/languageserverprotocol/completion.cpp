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

#include "completion.h"

namespace LanguageServerProtocol {

constexpr const char CompletionRequest::methodName[];
constexpr const char CompletionItemResolveRequest::methodName[];

CompletionRequest::CompletionRequest(const CompletionParams &params)
    : Request(methodName, params)
{ }

Utils::optional<MarkupOrString> CompletionItem::documentation() const
{
    QJsonValue documentation = value(documentationKey);
    if (documentation.isUndefined())
        return Utils::nullopt;
    return MarkupOrString(documentation);
}

Utils::optional<CompletionItem::InsertTextFormat> CompletionItem::insertTextFormat() const
{
    Utils::optional<int> value = optionalValue<int>(insertTextFormatKey);
    return value.has_value() ? Utils::nullopt
            : Utils::make_optional(CompletionItem::InsertTextFormat(value.value()));
}

bool CompletionItem::isValid(QStringList *error) const
{
    return check<QString>(error, labelKey)
            && checkOptional<int>(error, kindKey)
            && checkOptional<QString>(error, detailKey)
            && checkOptional<QString>(error, documentationKey)
            && checkOptional<QString>(error, sortTextKey)
            && checkOptional<QString>(error, filterTextKey)
            && checkOptional<QString>(error, insertTextKey)
            && checkOptional<int>(error, insertTextFormatKey)
            && checkOptional<TextEdit>(error, textEditKey)
            && checkOptionalArray<TextEdit>(error, additionalTextEditsKey)
            && checkOptionalArray<QString>(error, commitCharactersKey)
            && checkOptional<Command>(error, commandKey)
            && checkOptional<QJsonValue>(error, dataKey);
}

CompletionItemResolveRequest::CompletionItemResolveRequest(const CompletionItem &params)
    : Request(methodName, params)
{ }

bool CompletionList::isValid(QStringList *error) const
{
    return check<bool>(error, isIncompleteKey)
            && checkOptionalArray<CompletionItem>(error, itemsKey);

}

bool CompletionParams::isValid(QStringList *error) const
{
    return TextDocumentPositionParams::isValid(error)
            && checkOptional<CompletionContext>(error, contextKey);
}

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
