// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "semantichighlightsupport.h"

#include "client.h"
#include "dynamiccapabilities.h"
#include "languageclientmanager.h"

#include <languageserverprotocol/lsputils.h>

#include <texteditor/fontsettings.h>
#include <texteditor/semantichighlighter.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/texteditor.h>
#include <texteditor/syntaxhighlighter.h>
#include <utils/algorithm.h>
#include <utils/mimeutils.h>

#include <QTextDocument>

using namespace LanguageServerProtocol;
using namespace TextEditor;

namespace LanguageClient {

static Q_LOGGING_CATEGORY(LOGLSPHIGHLIGHT, "qtc.languageclient.highlight", QtWarningMsg);

constexpr int tokenTypeBitOffset = 16;

/// One token as the server encodes it, with the indexes resolved via the legend.
struct SemanticToken
{
    int deltaLine = 0;
    int deltaStart = 0;
    int length = 0;
    int tokenIndex = 0;
    int tokenType = 0;
    int rawTokenModifiers = 0;
    int tokenModifiers = 0;
};

QMap<QString, int> defaultTokenTypesMap()
{
    using namespace SemanticTokenTypes;
    return {{namespace_, namespaceToken},
            {type, typeToken},
            {class_, classToken},
            {enum_, enumToken},
            {interface_, interfaceToken},
            {struct_, structToken},
            {typeParameter, typeParameterToken},
            {parameter, parameterToken},
            {variable, variableToken},
            {property, propertyToken},
            {enumMember, enumMemberToken},
            {event, eventToken},
            {function, functionToken},
            {method, methodToken},
            {macro, macroToken},
            {keyword, keywordToken},
            {modifier, modifierToken},
            {comment, commentToken},
            {string, stringToken},
            {number, numberToken},
            {regexp, regexpToken},
            {operator_, operatorToken},
            {decorator, decoratorToken},
            {label, labelToken}};
}

QMap<QString, int> defaultTokenModifiersMap()
{
    using namespace SemanticTokenModifiers;
    return {{declaration, declarationModifier},
            {definition, definitionModifier},
            {readonly, readonlyModifier},
            {static_, staticModifier},
            {deprecated, deprecatedModifier},
            {abstract, abstractModifier},
            {async, asyncModifier},
            {modification, modificationModifier},
            {documentation, documentationModifier},
            {defaultLibrary, defaultLibraryModifier}};
}

static int convertModifiers(int modifiersData, const QList<int> &tokenModifiers)
{
    int result = 0;
    for (int i = 0; i < tokenModifiers.size() && modifiersData > 0; ++i) {
        if (modifiersData & 0x1) {
            const int modifier = tokenModifiers[i];
            if (modifier > 0)
                result |= modifier;
        }
        modifiersData = modifiersData >> 1;
    }
    return result;
}

/// The flat token data of \a tokens expanded into single tokens.
static QList<SemanticToken> toTokens(const SemanticTokens &tokens,
                                     const QList<int> &tokenTypes,
                                     const QList<int> &tokenModifiers)
{
    const QList<int> &data = tokens.data();
    if (data.size() % 5 != 0)
        return {};
    QList<SemanticToken> result;
    result.reserve(data.size() / 5);
    for (auto it = data.begin(), end = data.end(); it != end; it += 5) {
        SemanticToken token;
        token.deltaLine = *it;
        token.deltaStart = *(it + 1);
        token.length = *(it + 2);
        token.tokenIndex = *(it + 3);
        token.tokenType = tokenTypes.value(token.tokenIndex, -1);
        token.rawTokenModifiers = *(it + 4);
        token.tokenModifiers = convertModifiers(token.rawTokenModifiers, tokenModifiers);
        result << token;
    }
    return result;
}

SemanticTokenSupport::SemanticTokenSupport(Client *client)
    : m_client(client)
{
    QObject::connect(&globalFontSettings(), &FontSettings::changed,
                     client,
                     [this] { updateFormatHash(); });
    QObject::connect(Core::EditorManager::instance(),
                     &Core::EditorManager::currentEditorChanged,
                     this,
                     &SemanticTokenSupport::onCurrentEditorChanged);
    m_textStyleForTokenType = [](int tokenType) -> std::optional<TextStyle> {
        switch (tokenType) {
        case namespaceToken: return C_NAMESPACE;
        case typeToken: return C_TYPE;
        case classToken: return C_TYPE;
        case structToken: return C_TYPE;
        case enumMemberToken: return C_ENUMERATION;
        case typeParameterToken: return C_FIELD;
        case parameterToken: return C_PARAMETER;
        case variableToken: return C_LOCAL;
        case functionToken: return C_FUNCTION;
        case methodToken: return C_FUNCTION;
        case macroToken: return C_MACRO;
        case keywordToken: return C_KEYWORD;
        case commentToken: return C_COMMENT;
        case stringToken: return C_STRING;
        case numberToken: return C_NUMBER;
        case operatorToken: return C_OPERATOR;
        default:
            break;
        }
        return std::nullopt;
    };
}

void SemanticTokenSupport::refresh()
{
    qCDebug(LOGLSPHIGHLIGHT) << "refresh all semantic highlights for" << m_client->name();
    m_tokens.clear();
    for (Core::IEditor *editor : Core::EditorManager::visibleEditors())
        onCurrentEditorChanged(editor);
}

void SemanticTokenSupport::reloadSemanticTokens(TextDocument *textDocument)
{
    if (m_client->reachable())
        reloadSemanticTokensImpl(textDocument);
    else
        queueDocumentReload(textDocument);
}

void SemanticTokenSupport::reloadSemanticTokensImpl(TextDocument *textDocument,
                                                    int remainingRerequests)
{
    m_docReloadQueue.remove(textDocument);
    const SemanticRequestTypes supportedRequests = supportedSemanticRequests(textDocument);
    if (supportedRequests.testFlag(SemanticRequestType::None))
        return;
    const Utils::FilePath filePath = textDocument->filePath();
    auto responseCallback = [this,
                             remainingRerequests,
                             filePath,
                             documentVersion = m_client->documentVersion(filePath)](
                                const Utils::Result<SemanticTokensRequestResult> &result) {
        m_runningRequests.remove(filePath);
        if (!result) {
            qCDebug(LOGLSPHIGHLIGHT) << "received error" << result.error() << "for" << filePath;
            if (remainingRerequests > 0) {
                if (auto document = TextDocument::textDocumentForFilePath(filePath))
                    reloadSemanticTokensImpl(document, remainingRerequests - 1);
            }
        } else {
            handleSemanticTokens(filePath, *result, documentVersion);
        }
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
        params.textDocument(TextDocumentIdentifier().uri(m_client->uriFor(filePath)));
        qCDebug(LOGLSPHIGHLIGHT) << "Requesting all tokens for" << filePath << "with version"
                                 << m_client->documentVersion(filePath);
        MessageId &id = m_runningRequests[filePath];
        if (id.isValid())
            m_client->cancelRequest(id);
        id = m_client->sendRequest<SemanticTokensRequest>(params, responseCallback);
    }
}

void SemanticTokenSupport::updateSemanticTokens(TextDocument *textDocument)
{
    if (m_client->reachable())
        updateSemanticTokensImpl(textDocument);
    else
        queueDocumentReload(textDocument);
}

void SemanticTokenSupport::updateSemanticTokensImpl(TextDocument *textDocument,
                                                    int remainingRerequests)
{
    const SemanticRequestTypes supportedRequests = supportedSemanticRequests(textDocument);
    if (supportedRequests.testFlag(SemanticRequestType::FullDelta)) {
        const Utils::FilePath filePath = textDocument->filePath();
        const VersionedTokens versionedToken = m_tokens.value(filePath);
        const QString &previousResultId = versionedToken.tokens.resultId().value_or(QString());
        if (!previousResultId.isEmpty()) {
            const int documentVersion = m_client->documentVersion(filePath);
            if (documentVersion == versionedToken.version)
                return;
            SemanticTokensDeltaParams params;
            params.textDocument(TextDocumentIdentifier().uri(m_client->uriFor(filePath)));
            params.previousResultId(previousResultId);
            const auto callback = [this, filePath, documentVersion, remainingRerequests](
                                      const Utils::Result<SemanticTokensDeltaRequestResult> &result) {
                m_runningRequests.remove(filePath);
                if (!result) {
                    qCDebug(LOGLSPHIGHLIGHT)
                        << "received error" << result.error() << "for" << filePath;
                    if (auto document = TextDocument::textDocumentForFilePath(filePath)) {
                        if (remainingRerequests > 0)
                            updateSemanticTokensImpl(document, remainingRerequests - 1);
                        else
                            reloadSemanticTokensImpl(document, 1); // try a full reload once
                    }
                } else {
                    handleSemanticTokensDelta(filePath, *result, documentVersion);
                }
            };
            qCDebug(LOGLSPHIGHLIGHT)
                << "Requesting delta for" << filePath << "with version" << documentVersion;
            MessageId &id = m_runningRequests[filePath];
            if (id.isValid())
                m_client->cancelRequest(id);
            id = m_client->sendRequest<SemanticTokensDeltaRequest>(params, callback);
            return;
        }
    }
    reloadSemanticTokens(textDocument);
}

void SemanticTokenSupport::queueDocumentReload(TextEditor::TextDocument *doc)
{
    if (!Utils::insert(m_docReloadQueue, doc))
        return;
    connect(
        m_client,
        &Client::initialized,
        this,
        [this, doc = QPointer<TextDocument>(doc)]() {
            if (doc)
                reloadSemanticTokensImpl(doc);
        },
        Qt::QueuedConnection);
}

void SemanticTokenSupport::deactivateDocument(TextEditor::TextDocument *doc)
{
    if (m_tokens.contains(doc->filePath())){
        if (TextEditor::SyntaxHighlighter *highlighter = doc->syntaxHighlighter())
            highlighter->clearAllExtraFormats();
    }
}

void SemanticTokenSupport::clearCache(TextEditor::TextDocument *doc)
{
    m_tokens.remove(doc->filePath());
}

void SemanticTokenSupport::rehighlight()
{
    for (auto it = m_tokens.cbegin(); it != m_tokens.cend(); ++it)
        highlight(it.key(), true);
}

static void addModifiers(
    int key,
    QHash<int, QTextCharFormat> *formatHash,
    TextStyles styles,
    QList<int> tokenModifiers,
    const TextEditor::FontSettingsData &fs)
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

void SemanticTokenSupport::setLegend(const SemanticTokensLegend &legend)
{
    m_tokenTypeStrings = legend.tokenTypes();
    m_tokenModifierStrings = legend.tokenModifiers();
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
    auto fontSettings = globalFontSettings().data();
    for (int tokenType : std::as_const(m_tokenTypes)) {
        if (tokenType < 0)
            continue;
        const std::optional<TextStyle> style = m_textStyleForTokenType(tokenType);
        if (!style)
            continue;
        int mainHashPart = tokenType << tokenTypeBitOffset;
        m_formatHash[mainHashPart] = fontSettings.toTextCharFormat(*style);
        TextStyles styles;
        styles.mainStyle = *style;
        styles.mixinStyles.initializeElements();
        addModifiers(mainHashPart, &m_formatHash, styles, m_tokenModifiers, fontSettings);
    }
    rehighlight();
}

void SemanticTokenSupport::onCurrentEditorChanged(Core::IEditor *editor)
{
    if (auto widget = TextEditorWidget::fromEditor(editor))
        updateSemanticTokens(widget->textDocument());
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

void SemanticTokenSupport::clearTokens()
{
    m_tokens.clear();
}

//void SemanticTokenSupport::setAdditionalTokenModifierStyles(
//    const QHash<int, TextStyle> &modifierStyles)
//{
//    m_additionalModifierStyles = modifierStyles;
//}

/// The requests \a range and \a full announce, both of which may be a plain flag.
static SemanticRequestTypes supportedRequests(
    const std::optional<SemanticTokensRegistrationOptionsRange> &range,
    const std::optional<SemanticTokensRegistrationOptionsFull> &full)
{
    SemanticRequestTypes result;
    if (range) {
        if (const auto enabled = std::get_if<bool>(&*range); !enabled || *enabled)
            result |= SemanticRequestType::Range;
    }
    if (full) {
        if (const auto enabled = std::get_if<bool>(&*full)) {
            if (*enabled)
                result |= SemanticRequestType::Full;
        } else {
            const auto &delta = std::get<SemanticTokensFullDelta>(*full);
            if (delta.delta().value_or(false))
                result |= SemanticRequestType::FullDelta;
            result |= SemanticRequestType::Full;
        }
    }
    return result;
}

SemanticRequestTypes SemanticTokenSupport::supportedSemanticRequests(TextDocument *document) const
{
    if (!m_client->documentOpen(document))
        return SemanticRequestType::None;
    const QString dynamicMethod = "textDocument/semanticTokens";
    const DynamicCapabilities &dynamicCapabilities = m_client->dynamicCapabilities();
    if (auto registered = dynamicCapabilities.isRegistered(dynamicMethod)) {
        if (!*registered)
            return SemanticRequestType::None;
        const Utils::Result<SemanticTokensRegistrationOptions> options
            = fromJson<SemanticTokensRegistrationOptions>(dynamicCapabilities.option(dynamicMethod));
        if (!options)
            return SemanticRequestType::None;
        if (!applies(options->documentSelector(), document->filePath(),
                     Utils::mimeTypeForName(document->mimeType())))
            return SemanticRequestType::None;
        return supportedRequests(options->range(), options->full());
    }
    if (const auto &provider = m_client->capabilities().semanticTokensProvider()) {
        if (const auto options = std::get_if<SemanticTokensOptions>(&*provider))
            return supportedRequests(options->range(), options->full());
        if (const auto options = std::get_if<SemanticTokensRegistrationOptions>(&*provider))
            return supportedRequests(options->range(), options->full());
    }
    return SemanticRequestType::None;
}

void SemanticTokenSupport::handleSemanticTokens(const Utils::FilePath &filePath,
                                                const SemanticTokensRequestResult &result,
                                                int documentVersion)
{
    if (auto tokens = std::get_if<SemanticTokens>(&result)) {
        const bool force = !m_tokens.contains(filePath);
        m_tokens[filePath] = {*tokens, documentVersion};
        highlight(filePath, force);
    }
}

void SemanticTokenSupport::handleSemanticTokensDelta(
    const Utils::FilePath &filePath,
    const SemanticTokensDeltaRequestResult &result,
    int documentVersion)
{
    qCDebug(LOGLSPHIGHLIGHT) << "Handle Tokens for " << filePath;
    if (auto tokens = std::get_if<SemanticTokens>(&result)) {
        m_tokens[filePath] = {*tokens, documentVersion};
        qCDebug(LOGLSPHIGHLIGHT) << "New Data " << tokens->data();
    } else if (auto tokensDelta = std::get_if<SemanticTokensDelta>(&result)) {
        m_tokens[filePath].version = documentVersion;
        const QList<SemanticTokensEdit> edits
            = Utils::sorted(tokensDelta->edits(), [](const SemanticTokensEdit &first,
                                                     const SemanticTokensEdit &second) {
                  return first.start() < second.start();
              });
        if (edits.isEmpty()) {
            highlight(filePath);
            return;
        }

        SemanticTokens &tokens = m_tokens[filePath].tokens;
        const QList<int> &data = tokens.data();

        int newDataSize = data.size();
        for (const SemanticTokensEdit &edit : std::as_const(edits))
            newDataSize += edit.data().value_or(QList<int>()).size() - edit.deleteCount();
        QList<int> newData;
        newData.reserve(newDataSize);

        auto it = data.begin();
        const auto end = data.end();
        qCDebug(LOGLSPHIGHLIGHT) << "Edit Tokens";
        qCDebug(LOGLSPHIGHLIGHT) << "Data before edit " << data;
        for (const SemanticTokensEdit &edit : std::as_const(edits)) {
            if (edit.start() > data.size()) // prevent edits after the previously reported data
                return;
            for (const auto start = data.begin() + edit.start(); it < start; ++it)
                newData.append(*it);
            if (const std::optional<QList<int>> &editData = edit.data()) {
                newData.append(*editData);
                qCDebug(LOGLSPHIGHLIGHT) << edit.start() << edit.deleteCount() << *editData;
            } else {
                qCDebug(LOGLSPHIGHLIGHT) << edit.start() << edit.deleteCount();
            }
            int deleteCount = edit.deleteCount();
            if (deleteCount > std::distance(it, end)) {
                qCDebug(LOGLSPHIGHLIGHT)
                    << "We shall delete more highlight data entries than we actually have, "
                       "so we are out of sync with the server. "
                       "Request full semantic tokens again.";
                TextDocument *doc = TextDocument::textDocumentForFilePath(filePath);
                if (doc && LanguageClientManager::clientForDocument(doc) == m_client)
                    reloadSemanticTokens(doc);
                return;
            }
            it += deleteCount;
        }
        for (; it != end; ++it)
            newData.append(*it);

        qCDebug(LOGLSPHIGHLIGHT) << "New Data " << newData;
        tokens.data(newData);
        tokens.resultId(tokensDelta->resultId());
    }
    highlight(filePath);
}

void SemanticTokenSupport::highlight(const Utils::FilePath &filePath, bool force)
{
    qCDebug(LOGLSPHIGHLIGHT) << "highlight" << filePath;
    TextDocument *doc = TextDocument::textDocumentForFilePath(filePath);
    if (!doc || LanguageClientManager::clientForDocument(doc) != m_client)
        return;
    if (supportedSemanticRequests(doc).testFlag(SemanticRequestType::None))
        return;
    SyntaxHighlighter *highlighter = doc->syntaxHighlighter();
    if (!highlighter)
        return;
    const VersionedTokens versionedTokens = m_tokens.value(filePath);
    const QList<SemanticToken> tokens = toTokens(versionedTokens.tokens,
                                                 m_tokenTypes,
                                                 m_tokenModifiers);
    if (m_tokensHandler) {
        qCDebug(LOGLSPHIGHLIGHT) << "use tokens handler" << filePath;
        int line = 1;
        int column = 1;
        QList<ExpandedSemanticToken> expandedTokens;
        for (const SemanticToken &token : tokens) {
            line += token.deltaLine;
            if (token.deltaLine != 0) // reset the current column when we change the current line
                column = 1;
            column += token.deltaStart;
            if (token.tokenIndex >= m_tokenTypeStrings.length())
                continue;
            ExpandedSemanticToken expandedToken;
            expandedToken.type = m_tokenTypeStrings.at(token.tokenIndex);
            int modifiers = token.rawTokenModifiers;
            for (int bitPos = 0; modifiers && bitPos < m_tokenModifierStrings.length();
                 ++bitPos, modifiers >>= 1) {
                if (modifiers & 0x1)
                    expandedToken.modifiers << m_tokenModifierStrings.at(bitPos);
            }
            expandedToken.line = line;
            expandedToken.column = column;
            expandedToken.length = token.length;
            expandedTokens << expandedToken;
        };
        if (LOGLSPHIGHLIGHT().isDebugEnabled()) {
            qCDebug(LOGLSPHIGHLIGHT) << "Expanded Tokens for " << filePath;
            for (const ExpandedSemanticToken &token : std::as_const(expandedTokens)) {
                qCDebug(LOGLSPHIGHLIGHT)
                    << token.line << token.column << token.length << token.type << token.modifiers;
            }
        }

        m_tokensHandler(doc, expandedTokens, versionedTokens.version, force);
        return;
    }
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
    const HighlightingResults results = Utils::transform(tokens, toResult);
    SemanticHighlighter::setExtraAdditionalFormats(highlighter, results, m_formatHash);
}

} // namespace LanguageClient
