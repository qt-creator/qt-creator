// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "semantichighlightsupport.h"

#include "client.h"
#include "languageclientmanager.h"

#include <texteditor/fontsettings.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorsettings.h>
#include <utils/algorithm.h>
#include <utils/mimeutils.h>

#include <QTextDocument>

using namespace LanguageServerProtocol;
using namespace TextEditor;

namespace LanguageClient {

static Q_LOGGING_CATEGORY(LOGLSPHIGHLIGHT, "qtc.languageclient.highlight", QtWarningMsg);

constexpr int tokenTypeBitOffset = 16;

SemanticTokenSupport::SemanticTokenSupport(Client *client)
    : m_client(client)
{
    QObject::connect(TextEditorSettings::instance(),
                     &TextEditorSettings::fontSettingsChanged,
                     client,
                     [this]() { updateFormatHash(); });
    QObject::connect(Core::EditorManager::instance(),
                     &Core::EditorManager::currentEditorChanged,
                     this,
                     &SemanticTokenSupport::onCurrentEditorChanged);
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
    const TextDocumentIdentifier docId(m_client->hostPathToServerUri(filePath));
    auto responseCallback = [this,
                             remainingRerequests,
                             filePath,
                             documentVersion = m_client->documentVersion(filePath)](
                                const SemanticTokensFullRequest::Response &response) {
        m_runningRequests.remove(filePath);
        if (const auto error = response.error()) {
            qCDebug(LOGLSPHIGHLIGHT)
                << "received error" << error->code() << error->message() << "for" << filePath;
            if (remainingRerequests > 0) {
                if (auto document = TextDocument::textDocumentForFilePath(filePath))
                    reloadSemanticTokensImpl(document, remainingRerequests - 1);
            }
        } else {
            handleSemanticTokens(filePath, response.result().value_or(nullptr), documentVersion);
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
        params.setTextDocument(docId);
        SemanticTokensFullRequest request(params);
        request.setResponseCallback(responseCallback);
        qCDebug(LOGLSPHIGHLIGHT) << "Requesting all tokens for" << filePath << "with version"
                                 << m_client->documentVersion(filePath);
        MessageId &id = m_runningRequests[filePath];
        if (id.isValid())
            m_client->cancelRequest(id);
        id = request.id();
        m_client->sendMessage(request);
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
            params.setTextDocument(TextDocumentIdentifier(m_client->hostPathToServerUri(filePath)));
            params.setPreviousResultId(previousResultId);
            SemanticTokensFullDeltaRequest request(params);
            request.setResponseCallback(
                [this, filePath, documentVersion, remainingRerequests](
                    const SemanticTokensFullDeltaRequest::Response &response) {
                    m_runningRequests.remove(filePath);
                    if (const auto error = response.error()) {
                        qCDebug(LOGLSPHIGHLIGHT) << "received error" << error->code()
                                                 << error->message() << "for" << filePath;
                        if (auto document = TextDocument::textDocumentForFilePath(filePath)) {
                            if (remainingRerequests > 0)
                                updateSemanticTokensImpl(document, remainingRerequests - 1);
                            else
                                reloadSemanticTokensImpl(document, 1); // try a full reload once
                        }
                    } else {
                        handleSemanticTokensDelta(filePath,
                                                  response.result().value_or(nullptr),
                                                  documentVersion);
                    }
                });
            qCDebug(LOGLSPHIGHLIGHT)
                << "Requesting delta for" << filePath << "with version" << documentVersion;
            MessageId &id = m_runningRequests[filePath];
            if (id.isValid())
                m_client->cancelRequest(id);
            id = request.id();
            m_client->sendMessage(request);
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

void SemanticTokenSupport::clearHighlight(TextEditor::TextDocument *doc)
{
    if (m_tokens.contains(doc->filePath())){
        if (TextEditor::SyntaxHighlighter *highlighter = doc->syntaxHighlighter())
            highlighter->clearAllExtraFormats();
    }
}

void SemanticTokenSupport::rehighlight()
{
    for (const Utils::FilePath &filePath : m_tokens.keys())
        highlight(filePath, true);
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
    auto fontSettings = TextEditorSettings::fontSettings();
    for (int tokenType : std::as_const(m_tokenTypes)) {
        if (tokenType < 0)
            continue;
        TextStyle style;
        switch (tokenType) {
        case namespaceToken: style = C_NAMESPACE; break;
        case typeToken: style = C_TYPE; break;
        case classToken: style = C_TYPE; break;
        case structToken: style = C_TYPE; break;
        case enumMemberToken: style = C_ENUMERATION; break;
        case typeParameterToken: style = C_FIELD; break;
        case parameterToken: style = C_PARAMETER; break;
        case variableToken: style = C_LOCAL; break;
        case functionToken: style = C_FUNCTION; break;
        case methodToken: style = C_FUNCTION; break;
        case macroToken: style = C_MACRO; break;
        case keywordToken: style = C_KEYWORD; break;
        case commentToken: style = C_COMMENT; break;
        case stringToken: style = C_STRING; break;
        case numberToken: style = C_NUMBER; break;
        case operatorToken: style = C_OPERATOR; break;
        default:
            continue;
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

void SemanticTokenSupport::onCurrentEditorChanged(Core::IEditor *editor)
{
    if (auto textEditor = qobject_cast<BaseTextEditor *>(editor))
        updateSemanticTokens(textEditor->textDocument());
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

SemanticRequestTypes SemanticTokenSupport::supportedSemanticRequests(TextDocument *document) const
{
    if (!m_client->documentOpen(document))
        return SemanticRequestType::None;
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
    if (auto registered = dynamicCapabilities.isRegistered(dynamicMethod)) {
        if (!*registered)
            return SemanticRequestType::None;
        return supportedRequests(dynamicCapabilities.option(dynamicMethod).toObject());
    }
    if (std::optional<SemanticTokensOptions> provider = m_client->capabilities()
                                                              .semanticTokensProvider()) {
        return supportedRequests(*provider);
    }
    return SemanticRequestType::None;
}

void SemanticTokenSupport::handleSemanticTokens(const Utils::FilePath &filePath,
                                                const SemanticTokensResult &result,
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
    const LanguageServerProtocol::SemanticTokensDeltaResult &result,
    int documentVersion)
{
    qCDebug(LOGLSPHIGHLIGHT) << "Handle Tokens for " << filePath;
    if (auto tokens = std::get_if<SemanticTokens>(&result)) {
        m_tokens[filePath] = {*tokens, documentVersion};
        qCDebug(LOGLSPHIGHLIGHT) << "New Data " << tokens->data();
    } else if (auto tokensDelta = std::get_if<SemanticTokensDelta>(&result)) {
        m_tokens[filePath].version = documentVersion;
        const QList<SemanticTokensEdit> edits = Utils::sorted(tokensDelta->edits(),
                                                              &SemanticTokensEdit::start);
        if (edits.isEmpty()) {
            highlight(filePath);
            return;
        }

        SemanticTokens &tokens = m_tokens[filePath].tokens;
        const QList<int> &data = tokens.data();

        int newDataSize = data.size();
        for (const SemanticTokensEdit &edit : std::as_const(edits))
            newDataSize += edit.dataSize() - edit.deleteCount();
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
            if (const std::optional<QList<int>> editData = edit.data()) {
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
        tokens.setData(newData);
        tokens.setResultId(tokensDelta->resultId());
    }
    highlight(filePath);
}

void SemanticTokenSupport::highlight(const Utils::FilePath &filePath, bool force)
{
    qCDebug(LOGLSPHIGHLIGHT) << "highlight" << filePath;
    TextDocument *doc = TextDocument::textDocumentForFilePath(filePath);
    if (!doc || LanguageClientManager::clientForDocument(doc) != m_client)
        return;
    SyntaxHighlighter *highlighter = doc->syntaxHighlighter();
    if (!highlighter)
        return;
    const VersionedTokens versionedTokens = m_tokens.value(filePath);
    const QList<SemanticToken> tokens = versionedTokens.tokens
            .toTokens(m_tokenTypes, m_tokenModifiers);
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
