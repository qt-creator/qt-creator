// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"

#include <languageserverprotocol/lspjsonrpc.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorconstants.h>

#include <QSet>
#include <QTextCharFormat>

#include <functional>

namespace Core { class IEditor; }

namespace LanguageClient {
class Client;

// The token types and modifiers the protocol names, as the indices and bits
// this plugin maps them to. The names themselves come from the meta model, via
// LanguageServerProtocol::SemanticTokenTypes and ::SemanticTokenModifiers.
enum TokenType {
    namespaceToken,
    typeToken,
    classToken,
    enumToken,
    interfaceToken,
    structToken,
    typeParameterToken,
    parameterToken,
    variableToken,
    propertyToken,
    enumMemberToken,
    eventToken,
    functionToken,
    methodToken,
    macroToken,
    keywordToken,
    modifierToken,
    commentToken,
    stringToken,
    numberToken,
    regexpToken,
    operatorToken,
    decoratorToken,
    labelToken
};

enum TokenModifier {
    declarationModifier = 0x1,
    definitionModifier = 0x2,
    readonlyModifier = 0x4,
    staticModifier = 0x8,
    deprecatedModifier = 0x10,
    abstractModifier = 0x20,
    asyncModifier = 0x40,
    modificationModifier = 0x80,
    documentationModifier = 0x100,
    defaultLibraryModifier = 0x200
};

enum class SemanticRequestType {
    None = 0x0,
    Full = 0x1,
    FullDelta = 0x2,
    Range = 0x4
};
Q_DECLARE_FLAGS(SemanticRequestTypes, SemanticRequestType)

LANGUAGECLIENT_EXPORT QMap<QString, int> defaultTokenTypesMap();
LANGUAGECLIENT_EXPORT QMap<QString, int> defaultTokenModifiersMap();

class LANGUAGECLIENT_EXPORT ExpandedSemanticToken
{
public:
    friend bool operator==(const ExpandedSemanticToken &t1, const ExpandedSemanticToken &t2)
    {
        return t1.line == t2.line && t1.column == t2.column && t1.length == t2.length
                && t1.type == t2.type && t1.modifiers == t2.modifiers;
    }

    int line = -1;
    int column = -1;
    int length = -1;
    QString type;
    QStringList modifiers;
};

using SemanticTokensHandler = std::function<void(TextEditor::TextDocument *,
                                                 const QList<ExpandedSemanticToken> &, int, bool)>;

class LANGUAGECLIENT_EXPORT SemanticTokenSupport : public QObject
{
public:
    using TokenToTextStyle = std::optional<TextEditor::TextStyle> (*)(int);
    explicit SemanticTokenSupport(Client *client);

    void refresh();
    void reloadSemanticTokens(TextEditor::TextDocument *doc);
    void updateSemanticTokens(TextEditor::TextDocument *doc);
    void deactivateDocument(TextEditor::TextDocument *doc);
    void clearCache(TextEditor::TextDocument *doc);
    void rehighlight();
    void setLegend(const LanguageServerProtocol::SemanticTokensLegend &legend);
    void clearTokens();

    void setTokenTypesMap(const QMap<QString, int> &tokenTypesMap);
    void setTokenModifiersMap(const QMap<QString, int> &tokenModifiersMap);

    void setAdditionalTokenTypeStyles(const QHash<int, TextEditor::TextStyle> &typeStyles);
    // TODO: currently only declaration and definition modifiers are supported. The TextStyles
    // mixin capabilities need to be extended to be able to support more
//    void setAdditionalTokenModifierStyles(const QHash<int, TextEditor::TextStyle> &modifierStyles);

    void setTokensHandler(const SemanticTokensHandler &handler) { m_tokensHandler = handler; }
    void setTextStyleForTokenType(TokenToTextStyle callback){ m_textStyleForTokenType = callback; }

private:
    void reloadSemanticTokensImpl(TextEditor::TextDocument *doc, int remainingRerequests = 3);
    void updateSemanticTokensImpl(TextEditor::TextDocument *doc, int remainingRerequests = 3);
    void queueDocumentReload(TextEditor::TextDocument *doc);
    SemanticRequestTypes supportedSemanticRequests(TextEditor::TextDocument *document) const;
    void handleSemanticTokens(
        const Utils::FilePath &filePath,
        const LanguageServerProtocol::SemanticTokensRequestResult &result,
        int documentVersion);
    void handleSemanticTokensDelta(
        const Utils::FilePath &filePath,
        const LanguageServerProtocol::SemanticTokensDeltaRequestResult &result,
        int documentVersion);
    void highlight(const Utils::FilePath &filePath, bool force = false);
    void updateFormatHash();
    void currentEditorChanged();
    void onCurrentEditorChanged(Core::IEditor *editor);

    Client *m_client = nullptr;

    struct VersionedTokens
    {
        LanguageServerProtocol::SemanticTokens tokens;
        int version;
    };
    QHash<Utils::FilePath, VersionedTokens> m_tokens;
    QList<int> m_tokenTypes;
    QList<int> m_tokenModifiers;
    QHash<int, QTextCharFormat> m_formatHash;
    QHash<int, TextEditor::TextStyle> m_additionalTypeStyles;
//    QHash<int, TextEditor::TextStyle> m_additionalModifierStyles;
    QMap<QString, int> m_tokenTypesMap;
    QMap<QString, int> m_tokenModifiersMap;
    SemanticTokensHandler m_tokensHandler;
    QStringList m_tokenTypeStrings;
    QStringList m_tokenModifierStrings;
    QSet<TextEditor::TextDocument *> m_docReloadQueue;
    QHash<Utils::FilePath, LanguageServerProtocol::MessageId> m_runningRequests;
    TokenToTextStyle m_textStyleForTokenType;
};

} // namespace LanguageClient

Q_DECLARE_OPERATORS_FOR_FLAGS(LanguageClient::SemanticRequestTypes)
