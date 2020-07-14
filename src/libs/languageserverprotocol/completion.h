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
#include "languagefeatures.h"

namespace LanguageServerProtocol {

class LANGUAGESERVERPROTOCOL_EXPORT CompletionParams : public TextDocumentPositionParams
{
public:
    using TextDocumentPositionParams::TextDocumentPositionParams;

    enum CompletionTriggerKind {
        /**
         * Completion was triggered by typing an identifier (24x7 code
         * complete), manual invocation (e.g Ctrl+Space) or via API.
         */
        Invoked = 1,
        /**
         * Completion was triggered by a trigger character specified by
         * the `triggerCharacters` properties of the `CompletionRegistrationOptions`.
         */
        TriggerCharacter = 2,
        /// Completion was re-triggered as the current completion list is incomplete.
        TriggerForIncompleteCompletions = 3
    };

    class CompletionContext : public JsonObject
    {
    public:
        using JsonObject::JsonObject;

        /// How the completion was triggered.
        CompletionTriggerKind triggerKind() const
        { return CompletionTriggerKind(typedValue<int>(triggerKindKey)); }
        void setTriggerKind(CompletionTriggerKind triggerKind)
        { insert(triggerKindKey, triggerKind); }

        /**
         * The trigger character (a single character) that has trigger code complete.
         * Is undefined if `triggerKind !== CompletionTriggerKind.TriggerCharacter`
         */
        Utils::optional<QString> triggerCharacter() const
        { return optionalValue<QString>(triggerCharacterKey); }
        void setTriggerCharacter(const QString &triggerCharacter)
        { insert(triggerCharacterKey, triggerCharacter); }
        void clearTriggerCharacter() { remove(triggerCharacterKey); }

        bool isValid(ErrorHierarchy *error) const override
        {
            return check<int>(error, triggerKindKey)
                    && checkOptional<QString>(error, triggerCharacterKey);
        }
    };

    /**
     * The completion context. This is only available it the client specifies
     * to send this using `ClientCapabilities.textDocument.completion.contextSupport === true`
     */
    Utils::optional<CompletionContext> context() const
    { return optionalValue<CompletionContext>(contextKey); }
    void setContext(const CompletionContext &context)
    { insert(contextKey, context); }
    void clearContext() { remove(contextKey); }

    bool isValid(ErrorHierarchy *error) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT CompletionItem : public JsonObject
{
public:
    using JsonObject::JsonObject;

    /**
     * The label of this completion item. By default also the text that is inserted when selecting
     * this completion.
     */
    QString label() const { return typedValue<QString>(labelKey); }
    void setLabel(const QString &label) { insert(labelKey, label); }

    /// The kind of this completion item. Based of the kind an icon is chosen by the editor.
    Utils::optional<int> kind() const { return optionalValue<int>(kindKey); }
    void setKind(int kind) { insert(kindKey, kind); }
    void clearKind() { remove(kindKey); }

    /// A human-readable string with additional information about this item, like type information.
    Utils::optional<QString> detail() const { return optionalValue<QString>(detailKey); }
    void setDetail(const QString &detail) { insert(detailKey, detail); }
    void clearDetail() { remove(detailKey); }

    /// A human-readable string that represents a doc-comment.
    Utils::optional<MarkupOrString> documentation() const;
    void setDocumentation(const MarkupOrString &documentation)
    { insert(documentationKey, documentation.toJson()); }
    void cleatDocumentation() { remove(documentationKey); }

    /// A string that should be used when comparing this item
    /// with other items. When `falsy` the label is used.
    Utils::optional<QString> sortText() const { return optionalValue<QString>(sortTextKey); }
    void setSortText(const QString &sortText) { insert(sortTextKey, sortText); }
    void clearSortText() { remove(sortTextKey); }

    /// A string that should be used when filtering a set of
    /// completion items. When `falsy` the label is used.
    Utils::optional<QString> filterText() const { return optionalValue<QString>(filterTextKey); }
    void setFilterText(const QString &filterText) { insert(filterTextKey, filterText); }
    void clearFilterText() { remove(filterTextKey); }

    /**
     * A string that should be inserted into a document when selecting
     * this completion. When `falsy` the label is used.
     *
     * The `insertText` is subject to interpretation by the client side.
     * Some tools might not take the string literally. For example
     * VS Code when code complete is requested in this example `con<cursor position>`
     * and a completion item with an `insertText` of `console` is provided it
     * will only insert `sole`. Therefore it is recommended to use `textEdit` instead
     * since it avoids additional client side interpretation.
     *
     * @deprecated Use textEdit instead.
     */
    Utils::optional<QString> insertText() const { return optionalValue<QString>(insertTextKey); }
    void setInsertText(const QString &insertText) { insert(insertTextKey, insertText); }
    void clearInsertText() { remove(insertTextKey); }

    enum InsertTextFormat {
        /// The primary text to be inserted is treated as a plain string.
        PlainText = 1,

        /**
          * The primary text to be inserted is treated as a snippet.
          *
          * A snippet can define tab stops and placeholders with `$1`, `$2`
          * and `${3:foo}`. `$0` defines the final tab stop, it defaults to
          * the end of the snippet. Placeholders with equal identifiers are linked,
          * that is typing in one will update others too.
          */
        Snippet = 2
    };

    /// The format of the insert text. The format applies to both the `insertText` property
    /// and the `newText` property of a provided `textEdit`.
    Utils::optional<InsertTextFormat> insertTextFormat() const;
    void setInsertTextFormat(const InsertTextFormat &insertTextFormat)
    { insert(insertTextFormatKey, insertTextFormat); }
    void clearInsertTextFormat() { remove(insertTextFormatKey); }

    /**
     * An edit which is applied to a document when selecting this completion. When an edit is provided the value of
     * `insertText` is ignored.
     *
     * *Note:* The range of the edit must be a single line range and it must contain the position at which completion
     * has been requested.
     */
    Utils::optional<TextEdit> textEdit() const { return optionalValue<TextEdit>(textEditKey); }
    void setTextEdit(const TextEdit &textEdit) { insert(textEditKey, textEdit); }
    void clearTextEdit() { remove(textEditKey); }

    /**
     * An optional array of additional text edits that are applied when
     * selecting this completion. Edits must not overlap with the main edit
     * nor with themselves.
     */
    Utils::optional<QList<TextEdit>> additionalTextEdits() const
    { return optionalArray<TextEdit>(additionalTextEditsKey); }
    void setAdditionalTextEdits(const QList<TextEdit> &additionalTextEdits)
    { insertArray(additionalTextEditsKey, additionalTextEdits); }
    void clearAdditionalTextEdits() { remove(additionalTextEditsKey); }

    /**
     * An optional set of characters that when pressed while this completion is active will accept it first and
     * then type that character. *Note* that all commit characters should have `length=1` and that superfluous
     * characters will be ignored.
     */
    Utils::optional<QList<QString>> commitCharacters() const
    { return optionalArray<QString>(commitCharactersKey); }
    void setCommitCharacters(const QList<QString> &commitCharacters)
    { insertArray(commitCharactersKey, commitCharacters); }
    void clearCommitCharacters() { remove(commitCharactersKey); }

    /**
     * An optional command that is executed *after* inserting this completion. *Note* that
     * additional modifications to the current document should be described with the
     * additionalTextEdits-property.
     */
    Utils::optional<Command> command() const { return optionalValue<Command>(commandKey); }
    void setCommand(const Command &command) { insert(commandKey, command); }
    void clearCommand() { remove(commandKey); }

    /**
     * An data entry field that is preserved on a completion item between
     * a completion and a completion resolve request.
     */
    Utils::optional<QJsonValue> data() const { return optionalValue<QJsonValue>(dataKey); }
    void setData(const QJsonValue &data) { insert(dataKey, data); }
    void clearData() { remove(dataKey); }

    bool isValid(ErrorHierarchy *error) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT CompletionList : public JsonObject
{
public:
    using JsonObject::JsonObject;

    /**
     * This list it not complete. Further typing should result in recomputing
     * this list.
     */
    bool isIncomplete() const { return typedValue<bool>(isIncompleteKey); }
    void setIsIncomplete(bool isIncomplete) { insert(isIncompleteKey, isIncomplete); }

    /// The completion items.
    Utils::optional<QList<CompletionItem>> items() const { return array<CompletionItem>(itemsKey); }
    void setItems(const QList<CompletionItem> &items) { insertArray(itemsKey, items); }
    void clearItems() { remove(itemsKey); }

    bool isValid(ErrorHierarchy *error) const override;
};

/// The result of a completion is CompletionItem[] | CompletionList | null
class LANGUAGESERVERPROTOCOL_EXPORT CompletionResult
        : public Utils::variant<QList<CompletionItem>, CompletionList, std::nullptr_t>
{
public:
    using variant::variant;
    explicit CompletionResult(const QJsonValue &value);
};

class LANGUAGESERVERPROTOCOL_EXPORT CompletionRequest : public Request<
        CompletionResult, std::nullptr_t, CompletionParams>
{
public:
    explicit CompletionRequest(const CompletionParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "textDocument/completion";
};

class LANGUAGESERVERPROTOCOL_EXPORT CompletionItemResolveRequest : public Request<
        CompletionItem, std::nullptr_t, CompletionItem>
{
public:
    explicit CompletionItemResolveRequest(const CompletionItem &params);
    using Request::Request;
    constexpr static const char methodName[] = "completionItem/resolve";
};

} // namespace LanguageClient
