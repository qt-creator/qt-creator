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

#include "semantichighlightsupport.h"

#include "client.h"
#include "languageclientmanager.h"

#include <texteditor/fontsettings.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorsettings.h>
#include <utils/mimetypes/mimedatabase.h>

#include <QTextDocument>

using namespace LanguageServerProtocol;
using namespace TextEditor;

namespace LanguageClient {
namespace SemanticHighligtingSupport {

static Q_LOGGING_CATEGORY(LOGLSPHIGHLIGHT, "qtc.languageclient.highlight", QtWarningMsg);

static const QList<QList<QString>> highlightScopes(const ServerCapabilities &capabilities)
{
    return capabilities.semanticHighlighting()
        .value_or(ServerCapabilities::SemanticHighlightingServerCapabilities())
        .scopes().value_or(QList<QList<QString>>());
}

static Utils::optional<TextStyle> styleForScopes(const QList<QString> &scopes)
{
    // missing "Minimal Scope Coverage" scopes

    // entity.other.inherited-class
    // entity.name.section
    // entity.name.tag
    // entity.other.attribute-name
    // variable.language
    // variable.parameter
    // variable.function
    // constant.numeric
    // constant.language
    // constant.character.escape
    // support
    // storage.modifier
    // keyword.control
    // keyword.operator
    // keyword.declaration
    // invalid
    // invalid.deprecated

    static const QMap<QString, TextStyle> styleForScopes = {
        {"entity.name", C_TYPE},
        {"entity.name.function", C_FUNCTION},
        {"entity.name.function.method.static", C_GLOBAL},
        {"entity.name.function.preprocessor", C_PREPROCESSOR},
        {"entity.name.label", C_LABEL},
        {"keyword", C_KEYWORD},
        {"storage.type", C_KEYWORD},
        {"constant.numeric", C_NUMBER},
        {"string", C_STRING},
        {"comment", C_COMMENT},
        {"comment.block.documentation", C_DOXYGEN_COMMENT},
        {"variable.function", C_FUNCTION},
        {"variable.other", C_LOCAL},
        {"variable.other.member", C_FIELD},
        {"variable.other.field", C_FIELD},
        {"variable.other.field.static", C_GLOBAL},
        {"variable.parameter", C_PARAMETER},
    };

    for (QString scope : scopes) {
        while (!scope.isEmpty()) {
            auto style = styleForScopes.find(scope);
            if (style != styleForScopes.end())
                return style.value();
            const int index = scope.lastIndexOf('.');
            if (index <= 0)
                break;
            scope = scope.left(index);
        }
    }
    return Utils::nullopt;
}

static QHash<int, QTextCharFormat> scopesToFormatHash(QList<QList<QString>> scopes,
                                                      const FontSettings &fontSettings)
{
    QHash<int, QTextCharFormat> scopesToFormat;
    for (int i = 0; i < scopes.size(); ++i) {
        if (Utils::optional<TextStyle> style = styleForScopes(scopes[i]))
            scopesToFormat[i] = fontSettings.toTextCharFormat(style.value());
    }
    return scopesToFormat;
}

HighlightingResult tokenToHighlightingResult(int line, const SemanticHighlightToken &token)
{
    return HighlightingResult(unsigned(line) + 1,
                              unsigned(token.character) + 1,
                              token.length,
                              int(token.scope));
}

HighlightingResults generateResults(const QList<SemanticHighlightingInformation> &lines)
{
    HighlightingResults results;

    for (const SemanticHighlightingInformation &info : lines) {
        const int line = info.line();
        for (const SemanticHighlightToken &token :
             info.tokens().value_or(QList<SemanticHighlightToken>())) {
            results << tokenToHighlightingResult(line, token);
        }
    }

    return results;
}

void applyHighlight(TextDocument *doc,
                    const HighlightingResults &results,
                    const ServerCapabilities &capabilities)
{
    if (!doc->syntaxHighlighter())
        return;
    if (LOGLSPHIGHLIGHT().isDebugEnabled()) {
        auto scopes = highlightScopes(capabilities);
        qCDebug(LOGLSPHIGHLIGHT) << "semantic highlight for" << doc->filePath();
        for (auto result : results) {
            auto b = doc->document()->findBlockByNumber(int(result.line - 1));
            const QString &text = b.text().mid(int(result.column - 1), int(result.length));
            auto resultScupes = scopes[result.kind];
            auto style = styleForScopes(resultScupes).value_or(C_TEXT);
            qCDebug(LOGLSPHIGHLIGHT) << result.line - 1 << '\t'
                                     << result.column - 1 << '\t'
                                     << result.length << '\t'
                                     << TextEditor::Constants::nameForStyle(style) << '\t'
                                     << text
                                     << resultScupes;
        }
    }

    if (capabilities.semanticHighlighting().has_value()) {
        SemanticHighlighter::setExtraAdditionalFormats(
            doc->syntaxHighlighter(),
            results,
            scopesToFormatHash(highlightScopes(capabilities), doc->fontSettings()));
    }
}

} // namespace SemanticHighligtingSupport

constexpr int tokenTypeBitOffset = 16;

SemanticTokenSupport::SemanticTokenSupport(Client *client)
    : m_client(client)
{
    QObject::connect(TextEditorSettings::instance(),
                     &TextEditorSettings::fontSettingsChanged,
                     client,
                     [this]() { updateFormatHash(); });
}

void SemanticTokenSupport::reloadSemanticTokens(TextDocument *textDocument)
{
    const SemanticRequestTypes supportedRequests = supportedSemanticRequests(textDocument);
    if (supportedRequests.testFlag(SemanticRequestType::None))
        return;
    const Utils::FilePath filePath = textDocument->filePath();
    const TextDocumentIdentifier docId(DocumentUri::fromFilePath(filePath));
    auto responseCallback = [this, filePath](const SemanticTokensFullRequest::Response &response){
        handleSemanticTokens(filePath, response.result().value_or(nullptr));
    };
    /*if (supportedRequests.testFlag(SemanticRequestType::Range)) {
        const int start = widget->firstVisibleBlockNumber();
        const int end = widget->lastVisibleBlockNumber();
        const int pageSize = end - start;
        // request one extra page upfront and after the current visible range
        Range range(Position(qMax(0, start - pageSize), 0),
                    Position(qMin(widget->blockCount() - 1, end + pageSize), 0));
        SemanticTokensRangeParams params;
        params.setTextDocument(docId);
        params.setRange(range);
        SemanticTokensRangeRequest request(params);
        request.setResponseCallback(responseCallback);
        m_client->sendContent(request);
    } else */
    if (supportedRequests.testFlag(SemanticRequestType::Full)) {
        SemanticTokensParams params;
        params.setTextDocument(docId);
        SemanticTokensFullRequest request(params);
        request.setResponseCallback(responseCallback);
        m_client->sendContent(request);
    }
}

void SemanticTokenSupport::updateSemanticTokens(TextDocument *textDocument)
{
    const SemanticRequestTypes supportedRequests = supportedSemanticRequests(textDocument);
    if (supportedRequests.testFlag(SemanticRequestType::FullDelta)) {
        const Utils::FilePath filePath = textDocument->filePath();
        const QString &previousResultId = m_tokens.value(filePath).resultId().value_or(QString());
        if (!previousResultId.isEmpty()) {
            SemanticTokensDeltaParams params;
            params.setTextDocument(TextDocumentIdentifier(DocumentUri::fromFilePath(filePath)));
            params.setPreviousResultId(previousResultId);
            SemanticTokensFullDeltaRequest request(params);
            request.setResponseCallback(
                [this, filePath](const SemanticTokensFullDeltaRequest::Response &response) {
                    handleSemanticTokensDelta(filePath, response.result().value_or(nullptr));
                });
            m_client->sendContent(request);
            return;
        }
    }
    reloadSemanticTokens(textDocument);
}

void SemanticTokenSupport::rehighlight()
{
    for (const Utils::FilePath &filePath : m_tokens.keys())
        highlight(filePath);
}

void addModifiers(int key,
                  QHash<int, QTextCharFormat> *formatHash,
                  TextStyles styles,
                  QList<int> tokenModifiers,
                  const TextEditor::FontSettings &fs)
{
    if (tokenModifiers.isEmpty())
        return;
    int modifier = tokenModifiers.takeLast();
    if (modifier < 0)
        return;
    auto addModifier = [&](TextStyle style) {
        if (key & modifier) // already there don't add twice
            return;
        key = key | modifier;
        styles.mixinStyles.push_back(style);
        formatHash->insert(key, fs.toTextCharFormat(styles));
    };
    switch (modifier) {
    case declarationModifier: addModifier(C_DECLARATION); break;
    case definitionModifier: addModifier(C_FUNCTION_DEFINITION); break;
    default: break;
    }
    addModifiers(key, formatHash, styles, tokenModifiers, fs);
}

void SemanticTokenSupport::setLegend(const LanguageServerProtocol::SemanticTokensLegend &legend)
{
    m_tokenTypes = Utils::transform(legend.tokenTypes(), [&](const QString &tokenTypeString){
        return m_tokenTypesMap.value(tokenTypeString, -1);
    });
    m_tokenModifiers = Utils::transform(legend.tokenModifiers(), [&](const QString &tokenModifierString){
        return m_tokenModifiersMap.value(tokenModifierString, -1);
    });
    updateFormatHash();
}

void SemanticTokenSupport::updateFormatHash()
{
    auto fontSettings = TextEditorSettings::fontSettings();
    for (int tokenType : qAsConst(m_tokenTypes)) {
        if (tokenType < 0)
            continue;
        TextStyle style;
        switch (tokenType) {
        case typeToken: style = C_TYPE; break;
        case classToken: style = C_TYPE; break;
        case enumMemberToken: style = C_ENUMERATION; break;
        case typeParameterToken: style = C_FIELD; break;
        case parameterToken: style = C_PARAMETER; break;
        case variableToken: style = C_LOCAL; break;
        case functionToken: style = C_FUNCTION; break;
        case methodToken: style = C_FUNCTION; break;
        case macroToken: style = C_PREPROCESSOR; break;
        case keywordToken: style = C_KEYWORD; break;
        case commentToken: style = C_COMMENT; break;
        case stringToken: style = C_STRING; break;
        case numberToken: style = C_NUMBER; break;
        case operatorToken: style = C_OPERATOR; break;
        default:
            style = m_additionalTypeStyles.value(tokenType, C_TEXT);
            break;
        }
        int mainHashPart = tokenType << tokenTypeBitOffset;
        m_formatHash[mainHashPart] = fontSettings.toTextCharFormat(style);
        TextStyles styles;
        styles.mainStyle = style;
        styles.mixinStyles.initializeElements();
        addModifiers(mainHashPart, &m_formatHash, styles, m_tokenModifiers, fontSettings);
    }
    rehighlight();
}

void SemanticTokenSupport::setTokenTypesMap(const QMap<QString, int> &tokenTypesMap)
{
    m_tokenTypesMap = tokenTypesMap;
}

void SemanticTokenSupport::setTokenModifiersMap(const QMap<QString, int> &tokenModifiersMap)
{
    m_tokenModifiersMap = tokenModifiersMap;
}

void SemanticTokenSupport::setAdditionalTokenTypeStyles(
    const QHash<int, TextStyle> &typeStyles)
{
    m_additionalTypeStyles = typeStyles;
}

//void SemanticTokenSupport::setAdditionalTokenModifierStyles(
//    const QHash<int, TextStyle> &modifierStyles)
//{
//    m_additionalModifierStyles = modifierStyles;
//}

SemanticRequestTypes SemanticTokenSupport::supportedSemanticRequests(TextDocument *document) const
{
    auto supportedRequests = [&](const QJsonObject &options) -> SemanticRequestTypes {
        TextDocumentRegistrationOptions docOptions(options);
        if (docOptions.isValid()
            && docOptions.filterApplies(document->filePath(),
                                        Utils::mimeTypeForName(document->mimeType()))) {
            return SemanticRequestType::None;
        }
        const SemanticTokensOptions semanticOptions(options);
        return semanticOptions.supportedRequests();
    };
    const QString dynamicMethod = "textDocument/semanticTokens";
    const DynamicCapabilities &dynamicCapabilities = m_client->dynamicCapabilities();
    if (auto registered = dynamicCapabilities.isRegistered(dynamicMethod);
        registered.has_value()) {
        if (!registered.value())
            return SemanticRequestType::None;
        return supportedRequests(dynamicCapabilities.option(dynamicMethod).toObject());
    }
    if (m_client->capabilities().semanticTokensProvider().has_value())
        return supportedRequests(m_client->capabilities().semanticTokensProvider().value());
    return SemanticRequestType::None;
}

void SemanticTokenSupport::handleSemanticTokens(const Utils::FilePath &filePath,
                                                const SemanticTokensResult &result)
{
    if (auto tokens = Utils::get_if<SemanticTokens>(&result))
        m_tokens[filePath] = *tokens;
    else
        m_tokens.remove(filePath);
    highlight(filePath);
}

void SemanticTokenSupport::handleSemanticTokensDelta(
    const Utils::FilePath &filePath, const LanguageServerProtocol::SemanticTokensDeltaResult &result)
{
    if (auto tokens = Utils::get_if<SemanticTokens>(&result)) {
        m_tokens[filePath] = *tokens;
    } else if (auto tokensDelta = Utils::get_if<SemanticTokensDelta>(&result)) {
        QList<SemanticTokensEdit> edits = tokensDelta->edits();
        if (edits.isEmpty())
            return;

        Utils::sort(edits, &SemanticTokensEdit::start);

        SemanticTokens &tokens = m_tokens[filePath];
        const QList<int> &data = tokens.data();

        int newDataSize = data.size();
        for (const SemanticTokensEdit &edit : qAsConst(edits))
            newDataSize += edit.dataSize() - edit.deleteCount();
        QList<int> newData;
        newData.reserve(newDataSize);

        auto it = data.begin();
        for (const SemanticTokensEdit &edit : qAsConst(edits)) {
            if (edit.start() > data.size()) // prevent edits after the previously reported data
                return;
            for (const auto start = data.begin() + edit.start(); it < start; ++it)
                newData.append(*it);
            newData.append(edit.data().value_or(QList<int>()));
            it += edit.deleteCount();
        }
        for (const auto end = data.end(); it != end; ++it)
            newData.append(*it);

        tokens.setData(newData);
        tokens.setResultId(tokensDelta->resultId());
    } else {
        m_tokens.remove(filePath);
    }
    highlight(filePath);
}

void SemanticTokenSupport::highlight(const Utils::FilePath &filePath)
{
    TextDocument *doc = TextDocument::textDocumentForFilePath(filePath);
    if (!doc || LanguageClientManager::clientForDocument(doc) != m_client)
        return;
    SyntaxHighlighter *highlighter = doc->syntaxHighlighter();
    if (!highlighter)
        return;
    int line = 1;
    int column = 1;
    auto toResult = [&](const SemanticToken &token){
        line += token.deltaLine;
        if (token.deltaLine != 0) // reset the current column when we change the current line
            column = 1;
        column += token.deltaStart;
        const int tokenKind = token.tokenType << tokenTypeBitOffset | token.tokenModifiers;
        return HighlightingResult(line, column, token.length, tokenKind);
    };
    const QList<SemanticToken> tokens = m_tokens.value(filePath).toTokens(m_tokenTypes,
                                                                          m_tokenModifiers);
    const HighlightingResults results = Utils::transform(tokens, toResult);
    SemanticHighlighter::setExtraAdditionalFormats(highlighter, results, m_formatHash);
}

} // namespace LanguageClient
