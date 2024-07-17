// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdsemantichighlighting.h"

#include "clangdast.h"
#include "clangdclient.h"
#include "clangdqpropertyhighlighter.h"
#include "clangmodelmanagersupport.h"
#include "tasktimers.h"

#include <cppeditor/cppeditorwidget.h>
#include <cppeditor/semantichighlighter.h>
#include <languageclient/languageclientmanager.h>
#include <languageclient/semantichighlightsupport.h>
#include <languageserverprotocol/lsptypes.h>
#include <texteditor/blockrange.h>
#include <texteditor/texteditor.h>
#include <texteditor/textstyles.h>

#include <QtConcurrent>
#include <QTextDocument>

using namespace LanguageClient;
using namespace LanguageServerProtocol;
using namespace TextEditor;

namespace ClangCodeModel::Internal {
Q_LOGGING_CATEGORY(clangdLogHighlight, "qtc.clangcodemodel.clangd.highlight", QtWarningMsg);

class ExtraHighlightingResultsCollector
{
public:
    ExtraHighlightingResultsCollector(HighlightingResults &results,
                                      const Utils::FilePath &filePath,
                                      const QTextDocument *doc, const QString &docContent);

    void collect();
private:
    HighlightingResults &m_results;
    const Utils::FilePath m_filePath;
    const QTextDocument * const m_doc;
    const QString &m_docContent;
};

void doSemanticHighlighting(
        QPromise<HighlightingResult> &promise,
        const Utils::FilePath &filePath,
        const QList<ExpandedSemanticToken> &tokens,
        const QString &docContents,
        int docRevision,
        const TaskTimer &taskTimer)
{
    ThreadedSubtaskTimer t("highlighting", taskTimer);
    if (promise.isCanceled())
        return;

    const QTextDocument doc(docContents);

    const std::function<HighlightingResult(const ExpandedSemanticToken &)> toResult
            = [&](const ExpandedSemanticToken &token) {
        TextStyles styles;
        if (token.type == "variable") {
            if (token.modifiers.contains(QLatin1String("functionScope"))) {
                styles.mainStyle = C_LOCAL;
            } else if (token.modifiers.contains(QLatin1String("classScope"))) {
                styles.mainStyle = C_FIELD;
            } else if (token.modifiers.contains(QLatin1String("fileScope"))
                       || token.modifiers.contains(QLatin1String("globalScope"))) {
                styles.mainStyle = C_GLOBAL;
            }
        } else if (token.type == "function" || token.type == "method") {
            styles.mainStyle = token.modifiers.contains(QLatin1String("virtual"))
                    ? C_VIRTUAL_METHOD : C_FUNCTION;
            if (token.modifiers.contains("definition"))
                styles.mixinStyles.push_back(C_FUNCTION_DEFINITION);
        } else if (token.type == "class") {
            styles.mainStyle = C_TYPE;
            if (token.modifiers.contains("constructorOrDestructor"))
                styles.mainStyle = C_FUNCTION;
        } else if (token.type == "comment") { // "comment" means code disabled via the preprocessor
            styles.mainStyle = C_DISABLED_CODE;
        } else if (token.type == "namespace") {
            styles.mainStyle = C_NAMESPACE;
        } else if (token.type == "property") {
            styles.mainStyle = C_FIELD;
        } else if (token.type == "enum") {
            styles.mainStyle = C_TYPE;
        } else if (token.type == "enumMember") {
            styles.mainStyle = C_ENUMERATION;
        } else if (token.type == "parameter") {
            styles.mainStyle = C_PARAMETER;
        } else if (token.type == "macro") {
            styles.mainStyle = C_MACRO;
        } else if (token.type == "type") {
            styles.mainStyle = C_TYPE;
        } else if (token.type == "concept") {
            styles.mainStyle = C_CONCEPT;
        } else if (token.type == "modifier") {
            styles.mainStyle = C_KEYWORD;
        } else if (token.type == "label") {
            styles.mainStyle = C_LABEL;
        } else if (token.type == "typeParameter") {
            // clangd reports both type and non-type template parameters as type parameters,
            // but the latter can be distinguished by the readonly modifier.
            styles.mainStyle = token.modifiers.contains(QLatin1String("readonly"))
                    ? C_PARAMETER : C_TYPE;
        } else if (token.type == "operator") {
            const int pos = Utils::Text::positionInText(&doc, token.line, token.column);
            QTC_ASSERT(pos >= 0 || pos < docContents.size(), return HighlightingResult());
            const QChar firstChar = docContents.at(pos);
            if (firstChar.isLetter())
                styles.mainStyle = C_KEYWORD;
            else
                styles.mainStyle = C_PUNCTUATION;
            styles.mixinStyles.push_back(C_OPERATOR);
            if (token.modifiers.contains("userDefined"))
                styles.mixinStyles.push_back(C_OVERLOADED_OPERATOR);
            else if (token.modifiers.contains("declaration")) {
                styles.mixinStyles.push_back(C_OVERLOADED_OPERATOR);
                styles.mixinStyles.push_back(C_DECLARATION);
            }
            HighlightingResult result(token.line, token.column, token.length, styles);
            if (token.length == 1) {
                if (firstChar == '?')
                    result.kind = CppEditor::SemanticHighlighter::TernaryIf;
                else if (firstChar == ':')
                    result.kind = CppEditor::SemanticHighlighter::TernaryElse;
            }
            return result;
        } else if (token.type == "bracket") {
            styles.mainStyle = C_PUNCTUATION;
            HighlightingResult result(token.line, token.column, token.length, styles);
            const int pos = Utils::Text::positionInText(&doc, token.line, token.column);
            QTC_ASSERT(pos >= 0 || pos < docContents.size(), return HighlightingResult());
            const char symbol = docContents.at(pos).toLatin1();
            QTC_ASSERT(symbol == '<' || symbol == '>', return HighlightingResult());
            result.kind = symbol == '<'
                    ? CppEditor::SemanticHighlighter::AngleBracketOpen
                    : CppEditor::SemanticHighlighter::AngleBracketClose;
            return result;
        }
        if (token.modifiers.contains(QLatin1String("declaration")))
            styles.mixinStyles.push_back(C_DECLARATION);
        if (token.modifiers.contains(QLatin1String("static"))) {
            if (styles.mainStyle == C_FUNCTION) {
                styles.mainStyle = C_STATIC_MEMBER;
                styles.mixinStyles.push_back(C_FUNCTION);
            } else if (styles.mainStyle == C_FIELD) {
                styles.mainStyle = C_STATIC_MEMBER;
            }
        }
        if (token.modifiers.contains(QLatin1String("usedAsMutableReference"))
            || token.modifiers.contains(QLatin1String("usedAsMutablePointer"))) {
            styles.mixinStyles.push_back(C_OUTPUT_ARGUMENT);
        }
        return HighlightingResult(token.line, token.column, token.length, styles);
    };

    const auto safeToResult = [&toResult](const ExpandedSemanticToken &token) {
        try {
            return toResult(token);
        } catch (const std::exception &e) {
            qWarning() << "caught" << e.what() << "in toResult()";
            return HighlightingResult();
        }
    };
    auto results = QtConcurrent::blockingMapped<HighlightingResults>(tokens, safeToResult);
    ExtraHighlightingResultsCollector(results, filePath, &doc, docContents).collect();
    Utils::erase(results, [](const HighlightingResult &res) {
        // QTCREATORBUG-28639
        return res.textStyles.mainStyle == C_TEXT && res.textStyles.mixinStyles.empty();
    });
    if (!promise.isCanceled()) {
        qCInfo(clangdLogHighlight) << "reporting" << results.size() << "highlighting results";
        QList<Range> virtualRanges;
        for (const HighlightingResult &r : results) {
            qCDebug(clangdLogHighlight)
                << '\t' << r.line << r.column << r.length << int(r.textStyles.mainStyle);
            if (r.textStyles.mainStyle != C_VIRTUAL_METHOD)
                continue;
            const Position startPos(r.line - 1, r.column - 1);
            virtualRanges << Range(startPos, startPos.withOffset(r.length, &doc));
        }
        QMetaObject::invokeMethod(LanguageClientManager::instance(),
                                  [filePath, virtualRanges, docRevision] {
            if (ClangdClient * const client = ClangModelManagerSupport::clientForFile(filePath))
                client->setVirtualRanges(filePath, virtualRanges, docRevision);
        }, Qt::QueuedConnection);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        promise.addResults(results);
#else
        for (const HighlightingResult &r : results)
            promise.addResult(r);
#endif
    }
}

ExtraHighlightingResultsCollector::ExtraHighlightingResultsCollector(
        HighlightingResults &results,
        const Utils::FilePath &filePath, const QTextDocument *doc,
        const QString &docContent)
    : m_results(results), m_filePath(filePath), m_doc(doc),
      m_docContent(docContent)
{
}

void ExtraHighlightingResultsCollector::collect()
{
    for (int i = 0; i < m_results.length(); ++i) {
        const HighlightingResult res = m_results.at(i);
        if (res.textStyles.mainStyle != TextEditor::C_MACRO || res.length != 10)
            continue;
        const int pos = Utils::Text::positionInText(m_doc, res.line, res.column);
        if (subViewLen(m_docContent, pos, 10) != QLatin1String("Q_PROPERTY"))
            continue;
        int endPos;
        if (i < m_results.length() - 1) {
            const HighlightingResult nextRes = m_results.at(i + 1);
            endPos = Utils::Text::positionInText(m_doc, nextRes.line, nextRes.column);
        } else {
            endPos = m_docContent.length();
        }
        const QString qPropertyString = m_docContent.mid(pos, endPos - pos);
        QPropertyHighlighter propHighlighter(m_doc, qPropertyString, pos);
        for (const HighlightingResult &newRes : propHighlighter.highlight())
            m_results.insert(++i, newRes);
    }
}

class InactiveRegionsParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    DocumentUri uri() const { return TextDocumentIdentifier(value("textDocument")).uri(); }
    QList<Range> inactiveRegions() const {
        return array<Range>(LanguageServerProtocol::Key{"regions"});
    }
};

class InactiveRegionsNotification : public Notification<InactiveRegionsParams>
{
public:
    explicit InactiveRegionsNotification(const InactiveRegionsParams &params)
        : Notification(inactiveRegionsMethodName(), params) {}
    using Notification::Notification;
};

void handleInactiveRegions(LanguageClient::Client *client, const JsonRpcMessage &msg)
{
    const auto params = InactiveRegionsNotification(msg.toJsonObject()).params();
    if (!params)
        return;
    TextDocument * const doc = client->documentForFilePath(
        params->uri().toFilePath(client->hostPathMapper()));
    if (!doc)
        return;
    const auto editorWidget = CppEditor::CppEditorWidget::fromTextDocument(doc);
    if (!editorWidget)
        return;

    const QList<Range> inactiveRegions = params->inactiveRegions();
    QList<BlockRange> ifdefedOutBlocks;
    for (const Range &r : inactiveRegions) {
        const int startPos = Position(r.start().line(), 0).toPositionInDocument(doc->document());
        const int endPos = r.end().toPositionInDocument(doc->document()) + 1;
        ifdefedOutBlocks.emplaceBack(startPos, endPos);
    }
    editorWidget->setIfdefedOutBlocks(ifdefedOutBlocks);
}

QString inactiveRegionsMethodName()
{
    return "textDocument/inactiveRegions";
}

} // namespace ClangCodeModel::Internal
