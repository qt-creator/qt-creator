// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdsemantichighlighting.h"

#include "clangdast.h"
#include "clangdclient.h"
#include "clangdqpropertyhighlighter.h"
#include "clangmodelmanagersupport.h"
#include "tasktimers.h"

#include <cppeditor/semantichighlighter.h>
#include <languageclient/languageclientmanager.h>
#include <languageclient/semantichighlightsupport.h>
#include <languageserverprotocol/lsptypes.h>
#include <texteditor/blockrange.h>
#include <texteditor/textstyles.h>

#include <QtConcurrent>
#include <QTextDocument>

#include <new>

using namespace LanguageClient;
using namespace LanguageServerProtocol;
using namespace TextEditor;

namespace ClangCodeModel::Internal {
Q_LOGGING_CATEGORY(clangdLogHighlight, "qtc.clangcodemodel.clangd.highlight", QtWarningMsg);

// clangd reports also the #ifs, #elses and #endifs around the disabled code as disabled,
// and not even in a consistent manner. We don't want this, so we have to clean up here.
// But note that we require this behavior, as otherwise we would not be able to grey out
// e.g. empty lines after an #ifdef, due to the lack of symbols.
static QList<BlockRange> cleanupDisabledCode(HighlightingResults &results, const QTextDocument *doc,
                                             const QString &docContent)
{
    QList<BlockRange> ifdefedOutRanges;
    int rangeStartPos = -1;
    for (auto it = results.begin(); it != results.end();) {
        const bool wasIfdefedOut = rangeStartPos != -1;
        const bool isIfDefedOut = it->textStyles.mainStyle == C_DISABLED_CODE;
        if (!isIfDefedOut) {
            if (wasIfdefedOut) {
                const QTextBlock block = doc->findBlockByNumber(it->line - 1);
                ifdefedOutRanges << BlockRange(rangeStartPos, block.position());
                rangeStartPos = -1;
            }
            ++it;
            continue;
        }

        if (!wasIfdefedOut)
            rangeStartPos = doc->findBlockByNumber(it->line - 1).position();

        // Does the current line contain a potential "ifdefed-out switcher"?
        // If not, no state change is possible and we continue with the next line.
        const auto isPreprocessorControlStatement = [&] {
            const int pos = Utils::Text::positionInText(doc, it->line, it->column);
            const QStringView content = subViewLen(docContent, pos, it->length).trimmed();
            if (content.isEmpty() || content.first() != '#')
                return false;
            int offset = 1;
            while (offset < content.size() && content.at(offset).isSpace())
                ++offset;
            if (offset == content.size())
                return false;
            const QStringView ppDirective = content.mid(offset);
            return ppDirective.startsWith(QLatin1String("if"))
                    || ppDirective.startsWith(QLatin1String("elif"))
                    || ppDirective.startsWith(QLatin1String("else"))
                    || ppDirective.startsWith(QLatin1String("endif"));
        };
        if (!isPreprocessorControlStatement()) {
            ++it;
            continue;
        }

        if (!wasIfdefedOut) {
            // The #if or #else that starts disabled code should not be disabled.
            const QTextBlock nextBlock = doc->findBlockByNumber(it->line);
            rangeStartPos = nextBlock.isValid() ? nextBlock.position() : -1;
            it = results.erase(it);
            continue;
        }

        if (wasIfdefedOut && (it + 1 == results.end()
                || (it + 1)->textStyles.mainStyle != C_DISABLED_CODE
                || (it + 1)->line != it->line + 1)) {
            // The #else or #endif that ends disabled code should not be disabled.
            const QTextBlock block = doc->findBlockByNumber(it->line - 1);
            ifdefedOutRanges << BlockRange(rangeStartPos, block.position());
            rangeStartPos = -1;
            it = results.erase(it);
            continue;
        }
        ++it;
    }

    if (rangeStartPos != -1)
        ifdefedOutRanges << BlockRange(rangeStartPos, doc->characterCount());

    qCDebug(clangdLogHighlight) << "found" << ifdefedOutRanges.size() << "ifdefed-out ranges";
    if (clangdLogHighlight().isDebugEnabled()) {
        for (const BlockRange &r : std::as_const(ifdefedOutRanges))
            qCDebug(clangdLogHighlight) << r.first() << r.last();
    }

    return ifdefedOutRanges;
}

class ExtraHighlightingResultsCollector
{
public:
    ExtraHighlightingResultsCollector(QPromise<HighlightingResult> &promise,
                                      HighlightingResults &results,
                                      const Utils::FilePath &filePath, const ClangdAstNode &ast,
                                      const QTextDocument *doc, const QString &docContent,
                                      const QVersionNumber &clangdVersion);

    void collect();
private:
    static bool lessThan(const HighlightingResult &r1, const HighlightingResult &r2);
    static int onlyIndexOf(const QStringView &text, const QStringView &subString, int from = 0);
    int posForNodeStart(const ClangdAstNode &node) const;
    int posForNodeEnd(const ClangdAstNode &node) const;
    void insertResult(const HighlightingResult &result);
    void insertResult(const ClangdAstNode &node, TextStyle style);
    void insertAngleBracketInfo(int searchStart1, int searchEnd1, int searchStart2, int searchEnd2);
    void setResultPosFromRange(HighlightingResult &result, const Range &range);
    void collectFromNode(const ClangdAstNode &node);
    void visitNode(const ClangdAstNode&node);

    QPromise<HighlightingResult> &m_promise;
    HighlightingResults &m_results;
    const Utils::FilePath m_filePath;
    const ClangdAstNode &m_ast;
    const QTextDocument * const m_doc;
    const QString &m_docContent;
    const int m_clangdVersion;
    ClangdAstNode::FileStatus m_currentFileStatus = ClangdAstNode::FileStatus::Unknown;
};

void doSemanticHighlighting(
        QPromise<HighlightingResult> &promise,
        const Utils::FilePath &filePath,
        const QList<ExpandedSemanticToken> &tokens,
        const QString &docContents,
        const ClangdAstNode &ast,
        const QPointer<TextDocument> &textDocument,
        int docRevision,
        const QVersionNumber &clangdVersion,
        const TaskTimer &taskTimer)
{
    ThreadedSubtaskTimer t("highlighting", taskTimer);
    if (promise.isCanceled())
        return;

    const QTextDocument doc(docContents);
    const auto tokenRange = [&doc](const ExpandedSemanticToken &token) {
        const Position startPos(token.line - 1, token.column - 1);
        const Position endPos = startPos.withOffset(token.length, &doc);
        return Range(startPos, endPos);
    };
    const int clangdMajorVersion = clangdVersion.majorVersion();
    const auto isOutputParameter = [&ast, &tokenRange, clangdMajorVersion]
            (const ExpandedSemanticToken &token) {
        if (token.modifiers.contains(QLatin1String("usedAsMutableReference")))
            return true;
        if (token.modifiers.contains(QLatin1String("usedAsMutablePointer")))
            return true;
        if (clangdMajorVersion >= 16)
            return false;
        if (token.type != "variable" && token.type != "property" && token.type != "parameter")
            return false;
        const Range range = tokenRange(token);
        const ClangdAstPath path = getAstPath(ast, range);
        if (path.size() < 2)
            return false;
        if (token.type == "property"
                && (path.rbegin()->kind() == "MemberInitializer"
                    || path.rbegin()->kind() == "CXXConstruct")) {
            return false;
        }
        if (path.rbegin()->hasConstType())
            return false;
        for (auto it = path.rbegin() + 1; it != path.rend(); ++it) {
            if (it->kind() == "CXXConstruct" || it->kind() == "MemberInitializer")
                return true;

            if (it->kind() == "Call") {
                // The first child is e.g. a called lambda or an object on which
                // the call happens, and should not be highlighted as an output argument.
                // If the call is not fully resolved (as in templates), we don't
                // know whether the argument is passed as const or not.
                if (it->arcanaContains("dependent type"))
                    return false;
                const QList<ClangdAstNode> children = it->children().value_or(QList<ClangdAstNode>());
                return children.isEmpty()
                        || (children.first().range() != (it - 1)->range()
                        && children.first().kind() != "UnresolvedLookup");
            }

            // The token should get marked for e.g. lambdas, but not for assignment operators,
            // where the user sees that it's being written.
            if (it->kind() == "CXXOperatorCall") {
                const QList<ClangdAstNode> children = it->children().value_or(QList<ClangdAstNode>());

                // Child 1 is the call itself, Child 2 is the named entity on which the call happens
                // (a lambda or a class instance), after that follow the actual call arguments.
                if (children.size() < 2)
                    return false;

                // The call itself is never modifiable.
                if (children.first().range() == range)
                    return false;

                // The callable is never displayed as an output parameter.
                // TODO: A good argument can be made to display objects on which a non-const
                //       operator or function is called as output parameters.
                if (children.at(1).range().contains(range))
                    return false;

                QList<ClangdAstNode> firstChildTree{children.first()};
                while (!firstChildTree.isEmpty()) {
                    const ClangdAstNode n = firstChildTree.takeFirst();
                    const QString detail = n.detail().value_or(QString());
                    if (detail.startsWith("operator")) {
                        return !detail.contains('=')
                                && !detail.contains("++") && !detail.contains("--")
                                && !detail.contains("<<") && !detail.contains(">>")
                                && !detail.contains("*");
                    }
                    firstChildTree << n.children().value_or(QList<ClangdAstNode>());
                }
                return true;
            }

            if (it->kind() == "Lambda")
                return false;
            if (it->kind() == "BinaryOperator")
                return false;
            if (it->hasConstType())
                return false;

            if (it->kind() == "CXXMemberCall") {
                if (it == path.rbegin())
                    return false;
                const QList<ClangdAstNode> children = it->children().value_or(QList<ClangdAstNode>());
                QTC_ASSERT(!children.isEmpty(), return false);

                // The called object is never displayed as an output parameter.
                // TODO: A good argument can be made to display objects on which a non-const
                //       operator or function is called as output parameters.
                return (it - 1)->range() != children.first().range();
            }

            if (it->kind() == "Member" && it->arcanaContains("(")
                    && !it->arcanaContains("bound member function type")) {
                return false;
            }
        }
        return false;
    };

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
            if (token.modifiers.contains("definition")) {
                styles.mixinStyles.push_back(C_FUNCTION_DEFINITION);
            } else if (clangdMajorVersion < 16 && ast.isValid()) {
                const ClangdAstPath path = getAstPath(ast, tokenRange(token));
                if (path.length() > 1) {
                    const ClangdAstNode declNode = path.at(path.length() - 2);
                    if ((declNode.kind() == "Function" || declNode.kind() == "CXXMethod")
                            && declNode.hasChildWithRole("statement")) {
                        styles.mixinStyles.push_back(C_FUNCTION_DEFINITION);
                    }
                }
            }
        } else if (token.type == "class") {
            styles.mainStyle = C_TYPE;
            if (token.modifiers.contains("constructorOrDestructor")) {
                styles.mainStyle = C_FUNCTION;
            } else if (clangdMajorVersion < 16 && ast.isValid()) {
                const ClangdAstPath path = getAstPath(ast, tokenRange(token));
                if (!path.isEmpty()) {
                    if (path.last().kind() == "CXXConstructor") {
                        if (!path.last().arcanaContains("implicit"))
                            styles.mainStyle = C_FUNCTION;
                    } else if (path.last().kind() == "Record" && path.length() > 1) {
                        const ClangdAstNode node = path.at(path.length() - 2);
                        if (node.kind() == "CXXDestructor" && !node.arcanaContains("implicit")) {
                            styles.mainStyle = C_FUNCTION;

                            // https://github.com/clangd/clangd/issues/872
                            if (node.role() == "declaration")
                                styles.mixinStyles.push_back(C_DECLARATION);
                        }
                    }
                }
            }
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
        if (isOutputParameter(token))
            styles.mixinStyles.push_back(C_OUTPUT_ARGUMENT);
        qCDebug(clangdLogHighlight) << "adding highlighting result"
                                    << token.line << token.column << token.length << int(styles.mainStyle);
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
    const bool handleInactiveCode = clangdMajorVersion < 17;
    QList<BlockRange> ifdefedOutBlocks;
    if (handleInactiveCode)
        ifdefedOutBlocks = cleanupDisabledCode(results, &doc, docContents);
    ExtraHighlightingResultsCollector(promise, results, filePath, ast, &doc, docContents,
                                      clangdVersion).collect();
    Utils::erase(results, [](const HighlightingResult &res) {
        // QTCREATORBUG-28639
        return res.textStyles.mainStyle == C_TEXT && res.textStyles.mixinStyles.empty();
    });
    if (!promise.isCanceled()) {
        qCInfo(clangdLogHighlight) << "reporting" << results.size() << "highlighting results";
        if (handleInactiveCode) {
            QMetaObject::invokeMethod(textDocument, [textDocument, ifdefedOutBlocks, docRevision] {
                    if (textDocument && textDocument->document()->revision() == docRevision)
                        textDocument->setIfdefedOutBlocks(ifdefedOutBlocks);
                }, Qt::QueuedConnection);
        }
        QList<Range> virtualRanges;
        for (const HighlightingResult &r : results) {
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
        QPromise<HighlightingResult> &promise, HighlightingResults &results,
        const Utils::FilePath &filePath, const ClangdAstNode &ast, const QTextDocument *doc,
        const QString &docContent, const QVersionNumber &clangdVersion)
    : m_promise(promise), m_results(results), m_filePath(filePath), m_ast(ast), m_doc(doc),
      m_docContent(docContent), m_clangdVersion(clangdVersion.majorVersion())
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

    if (!m_ast.isValid())
        return;
    QTC_ASSERT(m_clangdVersion < 17, return);
    visitNode(m_ast);
}

bool ExtraHighlightingResultsCollector::lessThan(const HighlightingResult &r1,
                                                 const HighlightingResult &r2)
{
    return r1.line < r2.line || (r1.line == r2.line && r1.column < r2.column)
            || (r1.line == r2.line && r1.column == r2.column && r1.length < r2.length);
}

int ExtraHighlightingResultsCollector::onlyIndexOf(const QStringView &text,
                                                   const QStringView &subString, int from)
{
    const int firstIndex = text.indexOf(subString, from);
    if (firstIndex == -1)
        return -1;
    const int nextIndex = text.indexOf(subString, firstIndex + 1);

    // The second condion deals with the off-by-one error in TemplateSpecialization nodes;
    // see collectFromNode().
    return nextIndex == -1 || nextIndex == firstIndex + 1 ? firstIndex : -1;
}

// Unfortunately, the exact position of a specific token is usually not
// recorded in the AST, so if we need that, we have to search for it textually.
// In corner cases, this might get sabotaged by e.g. comments, in which case we give up.
int ExtraHighlightingResultsCollector::posForNodeStart(const ClangdAstNode &node) const
{
    return Utils::Text::positionInText(m_doc, node.range().start().line() + 1,
                                       node.range().start().character() + 1);
}

int ExtraHighlightingResultsCollector::posForNodeEnd(const ClangdAstNode &node) const
{
    return Utils::Text::positionInText(m_doc, node.range().end().line() + 1,
                                       node.range().end().character() + 1);
}

void ExtraHighlightingResultsCollector::insertResult(const HighlightingResult &result)
{
    if (!result.isValid()) // Some nodes don't have a range.
        return;
    const auto it = std::lower_bound(m_results.begin(), m_results.end(), result, lessThan);
    if (it == m_results.end() || *it != result) {

        // Prevent inserting expansions for function-like macros. For instance:
        //     #define TEST() "blubb"
        //     const char *s = TEST();
        // The macro name is always shorter than the expansion and starts at the same
        // location, so it should occur right before the insertion position.
        if (it > m_results.begin() && (it - 1)->line == result.line
                && (it - 1)->column == result.column
                && (it - 1)->textStyles.mainStyle == C_MACRO) {
            return;
        }

        // Bogus ranges; e.g. QTCREATORBUG-27601
        if (it != m_results.end()) {
            const int nextStartPos = Utils::Text::positionInText(m_doc, it->line, it->column);
            const int resultEndPos = Utils::Text::positionInText(m_doc, result.line, result.column)
                    + result.length;
            if (resultEndPos > nextStartPos)
                return;
        }

        qCDebug(clangdLogHighlight) << "adding additional highlighting result"
                                    << result.line << result.column << result.length;
        m_results.insert(it, result);
        return;
    }
}

void ExtraHighlightingResultsCollector::insertResult(const ClangdAstNode &node, TextStyle style)
{
    HighlightingResult result;
    result.useTextSyles = true;
    result.textStyles.mainStyle = style;
    setResultPosFromRange(result, node.range());
    insertResult(result);
    return;
}

// For matching the "<" and ">" brackets of template declarations, specializations
// and instantiations.
void ExtraHighlightingResultsCollector::insertAngleBracketInfo(int searchStart1, int searchEnd1,
                                                               int searchStart2, int searchEnd2)
{
    const int openingAngleBracketPos = onlyIndexOf(
                subViewEnd(m_docContent, searchStart1, searchEnd1),
                QStringView(QStringLiteral("<")));
    if (openingAngleBracketPos == -1)
        return;
    const int absOpeningAngleBracketPos = searchStart1 + openingAngleBracketPos;
    if (absOpeningAngleBracketPos > searchStart2)
        searchStart2 = absOpeningAngleBracketPos + 1;
    if (searchStart2 >= searchEnd2)
        return;
    const int closingAngleBracketPos = onlyIndexOf(
                subViewEnd(m_docContent, searchStart2, searchEnd2),
                QStringView(QStringLiteral(">")));
    if (closingAngleBracketPos == -1)
        return;

    const int absClosingAngleBracketPos = searchStart2 + closingAngleBracketPos;
    if (absOpeningAngleBracketPos > absClosingAngleBracketPos)
        return;

    HighlightingResult result;
    result.useTextSyles = true;
    result.textStyles.mainStyle = C_PUNCTUATION;
    Utils::Text::convertPosition(m_doc, absOpeningAngleBracketPos, &result.line, &result.column);
    ++result.column;
    result.length = 1;
    result.kind = CppEditor::SemanticHighlighter::AngleBracketOpen;
    insertResult(result);
    Utils::Text::convertPosition(m_doc, absClosingAngleBracketPos, &result.line, &result.column);
    ++result.column;
    result.kind = CppEditor::SemanticHighlighter::AngleBracketClose;
    insertResult(result);
}

void ExtraHighlightingResultsCollector::setResultPosFromRange(HighlightingResult &result,
                                                              const Range &range)
{
    if (!range.isValid())
        return;
    const Position startPos = range.start();
    const Position endPos = range.end();
    result.line = startPos.line() + 1;
    result.column = startPos.character() + 1;
    result.length = endPos.toPositionInDocument(m_doc) - startPos.toPositionInDocument(m_doc);
}

void ExtraHighlightingResultsCollector::collectFromNode(const ClangdAstNode &node)
{
    if (node.kind().endsWith("Literal"))
        return;
    if (node.role() == "type" && node.kind() == "Builtin")
        return;

    if (m_clangdVersion < 16 && node.role() == "attribute"
            && (node.kind() == "Override" || node.kind() == "Final")) {
        insertResult(node, C_KEYWORD);
        return;
    }

    const bool isExpression = node.role() == "expression";
    if (m_clangdVersion < 16 && isExpression && node.kind() == "Predefined") {
        insertResult(node, C_LOCAL);
        return;
    }

    const bool isDeclaration = node.role() == "declaration";
    const int nodeStartPos = posForNodeStart(node);
    const int nodeEndPos = posForNodeEnd(node);
    const QList<ClangdAstNode> children = node.children().value_or(QList<ClangdAstNode>());

    // Match question mark and colon in ternary operators.
    if (m_clangdVersion < 16 && isExpression && node.kind() == "ConditionalOperator") {
        if (children.size() != 3)
            return;

        // The question mark is between sub-expressions 1 and 2, the colon is between
        // sub-expressions 2 and 3.
        const int searchStartPosQuestionMark = posForNodeEnd(children.first());
        const int searchEndPosQuestionMark = posForNodeStart(children.at(1));
        QStringView content = subViewEnd(m_docContent, searchStartPosQuestionMark,
                                         searchEndPosQuestionMark);
        const int questionMarkPos = onlyIndexOf(content, QStringView(QStringLiteral("?")));
        if (questionMarkPos == -1)
            return;
        const int searchStartPosColon = posForNodeEnd(children.at(1));
        const int searchEndPosColon = posForNodeStart(children.at(2));
        content = subViewEnd(m_docContent, searchStartPosColon, searchEndPosColon);
        const int colonPos = onlyIndexOf(content, QStringView(QStringLiteral(":")));
        if (colonPos == -1)
            return;

        const int absQuestionMarkPos = searchStartPosQuestionMark + questionMarkPos;
        const int absColonPos = searchStartPosColon + colonPos;
        if (absQuestionMarkPos > absColonPos)
            return;

        HighlightingResult result;
        result.useTextSyles = true;
        result.textStyles.mainStyle = C_PUNCTUATION;
        result.textStyles.mixinStyles.push_back(C_OPERATOR);
        Utils::Text::convertPosition(m_doc, absQuestionMarkPos, &result.line, &result.column);
        ++result.column;
        result.length = 1;
        result.kind = CppEditor::SemanticHighlighter::TernaryIf;
        insertResult(result);
        Utils::Text::convertPosition(m_doc, absColonPos, &result.line, &result.column);
        ++result.column;
        result.kind = CppEditor::SemanticHighlighter::TernaryElse;
        insertResult(result);
        return;
    }

    if (isDeclaration && (node.kind() == "FunctionTemplate" || node.kind() == "ClassTemplate")) {
        // The child nodes are the template parameters and and the function or class.
        // The opening angle bracket is before the first child node, the closing angle
        // bracket is before the function child node and after the last param node.
        const QString classOrFunctionKind = QLatin1String(node.kind() == "FunctionTemplate"
                                                          ? "Function" : "CXXRecord");
        const auto functionOrClassIt = std::find_if(children.begin(), children.end(),
                                                    [&classOrFunctionKind](const ClangdAstNode &n) {
            return n.role() == "declaration" && n.kind() == classOrFunctionKind;
        });
        if (functionOrClassIt == children.end() || functionOrClassIt == children.begin())
            return;
        const int firstTemplateParamStartPos = posForNodeStart(children.first());
        const int lastTemplateParamEndPos = posForNodeEnd(*(functionOrClassIt - 1));
        const int functionOrClassStartPos = posForNodeStart(*functionOrClassIt);
        insertAngleBracketInfo(nodeStartPos, firstTemplateParamStartPos,
                               lastTemplateParamEndPos, functionOrClassStartPos);
        return;
    }

    const auto isTemplateParamDecl = [](const ClangdAstNode &node) {
        return node.isTemplateParameterDeclaration();
    };
    if (isDeclaration && node.kind() == "TypeAliasTemplate") {
        // Children are one node of type TypeAlias and the template parameters.
        // The opening angle bracket is before the first parameter and the closing
        // angle bracket is after the last parameter.
        // The TypeAlias node seems to appear first in the AST, even though lexically
        // is comes after the parameters. We don't rely on the order here.
        // Note that there is a second pair of angle brackets. That one is part of
        // a TemplateSpecialization, which is handled further below.
        const auto firstTemplateParam = std::find_if(children.begin(), children.end(),
                                                     isTemplateParamDecl);
        if (firstTemplateParam == children.end())
            return;
        const auto lastTemplateParam = std::find_if(children.rbegin(), children.rend(),
                                                    isTemplateParamDecl);
        QTC_ASSERT(lastTemplateParam != children.rend(), return);
        const auto typeAlias = std::find_if(children.begin(), children.end(),
                [](const ClangdAstNode &n) { return n.kind() == "TypeAlias"; });
        if (typeAlias == children.end())
            return;

        const int firstTemplateParamStartPos = posForNodeStart(*firstTemplateParam);
        const int lastTemplateParamEndPos = posForNodeEnd(*lastTemplateParam);
        const int searchEndPos = posForNodeStart(*typeAlias);
        insertAngleBracketInfo(nodeStartPos, firstTemplateParamStartPos,
                               lastTemplateParamEndPos, searchEndPos);
        return;
    }

    if (isDeclaration && node.kind() == "ClassTemplateSpecialization") {
        // There is one child of kind TemplateSpecialization. The first pair
        // of angle brackets comes before that.
        if (children.size() == 1) {
            const int childNodePos = posForNodeStart(children.first());
            insertAngleBracketInfo(nodeStartPos, childNodePos, nodeStartPos, childNodePos);
        }
        return;
    }

    if (isDeclaration && node.kind() == "TemplateTemplateParm") {
        // The child nodes are template arguments and template parameters.
        // Arguments seem to appear before parameters in the AST, even though they
        // come after them in the source code. We don't rely on the order here.
        const auto firstTemplateParam = std::find_if(children.begin(), children.end(),
                                                     isTemplateParamDecl);
        if (firstTemplateParam == children.end())
            return;
        const auto lastTemplateParam = std::find_if(children.rbegin(), children.rend(),
                                                    isTemplateParamDecl);
        QTC_ASSERT(lastTemplateParam != children.rend(), return);
        const auto templateArg = std::find_if(children.begin(), children.end(),
                [](const ClangdAstNode &n) { return n.role() == "template argument"; });

        const int firstTemplateParamStartPos = posForNodeStart(*firstTemplateParam);
        const int lastTemplateParamEndPos = posForNodeEnd(*lastTemplateParam);
        const int searchEndPos = templateArg == children.end()
                ? nodeEndPos : posForNodeStart(*templateArg);
        insertAngleBracketInfo(nodeStartPos, firstTemplateParamStartPos,
                               lastTemplateParamEndPos, searchEndPos);
        return;
    }

    // {static,dynamic,reinterpret}_cast<>().
    if (isExpression && node.kind().startsWith("CXX") && node.kind().endsWith("Cast")) {
        // First child is type, second child is expression.
        // The opening angle bracket is before the first child, the closing angle bracket
        // is between the two children.
        if (children.size() == 2) {
            insertAngleBracketInfo(nodeStartPos, posForNodeStart(children.first()),
                                   posForNodeEnd(children.first()),
                                   posForNodeStart(children.last()));
        }
        return;
    }

    if (node.kind() == "TemplateSpecialization") {
        // First comes the template type, then the template arguments.
        // The opening angle bracket is before the first template argument,
        // the closing angle bracket is after the last template argument.
        // The first child node has no range, so we start searching at the parent node.
        if (children.size() >= 2) {
            int searchStart2 = posForNodeEnd(children.last());
            int searchEnd2 = nodeEndPos;

            // There is a weird off-by-one error on the clang side: If there is a
            // nested template instantiation *and* there is no space between
            // the closing angle brackets, then the inner TemplateSpecialization node's range
            // will extend one character too far, covering the outer's closing angle bracket.
            // This is what we are correcting for here.
            // This issue is tracked at https://github.com/clangd/clangd/issues/871.
            if (searchStart2 == searchEnd2)
                --searchStart2;
            insertAngleBracketInfo(nodeStartPos, posForNodeStart(children.at(1)),
                                   searchStart2, searchEnd2);
        }
        return;
    }

    if (!isExpression && !isDeclaration)
        return;

    if (m_clangdVersion >= 16)
        return;

    // Operators, overloaded ones in particular.
    static const QString operatorPrefix = "operator";
    QString detail = node.detail().value_or(QString());
    const bool isCallToNew = node.kind() == "CXXNew";
    const bool isCallToDelete = node.kind() == "CXXDelete";
    const auto isProperOperator = [&] {
        if (isCallToNew || isCallToDelete)
            return true;
        if (!detail.startsWith(operatorPrefix))
            return false;
        if (detail == operatorPrefix)
            return false;
        const QChar nextChar = detail.at(operatorPrefix.length());
        return !nextChar.isLetterOrNumber() && nextChar != '_';
    };
    if (!isProperOperator())
        return;

    if (!isCallToNew && !isCallToDelete)
        detail.remove(0, operatorPrefix.length());

    if (node.kind() == "CXXConversion")
        return;

    HighlightingResult result;
    result.useTextSyles = true;
    const bool isOverloaded = isDeclaration || ((!isCallToNew && !isCallToDelete)
                                                || node.arcanaContains("CXXMethod"));
    result.textStyles.mainStyle = isCallToNew || isCallToDelete || detail.at(0).isSpace()
              ? C_KEYWORD : C_PUNCTUATION;
    result.textStyles.mixinStyles.push_back(C_OPERATOR);
    if (isOverloaded)
        result.textStyles.mixinStyles.push_back(C_OVERLOADED_OPERATOR);
    if (isDeclaration)
        result.textStyles.mixinStyles.push_back(C_DECLARATION);

    const QStringView nodeText = subViewEnd(m_docContent, nodeStartPos, nodeEndPos);

    if (isCallToNew || isCallToDelete) {
        result.line = node.range().start().line() + 1;
        result.column = node.range().start().character() + 1;
        result.length = isCallToNew ? 3 : 6;
        insertResult(result);
        if (node.arcanaContains("array")) {
            const int openingBracketOffset = nodeText.indexOf('[');
            if (openingBracketOffset == -1)
                return;
            const int closingBracketOffset = nodeText.lastIndexOf(']');
            if (closingBracketOffset == -1 || closingBracketOffset < openingBracketOffset)
                return;

            result.textStyles.mainStyle = C_PUNCTUATION;
            result.length = 1;
            Utils::Text::convertPosition(m_doc,
                                         nodeStartPos + openingBracketOffset,
                                         &result.line, &result.column);
            ++result.column;
            insertResult(result);
            Utils::Text::convertPosition(m_doc,
                                         nodeStartPos + closingBracketOffset,
                                         &result.line, &result.column);
            ++result.column;
            insertResult(result);
        }
        return;
    }

    if (isExpression && (detail == QLatin1String("()") || detail == QLatin1String("[]"))) {
        result.line = node.range().start().line() + 1;
        result.column = node.range().start().character() + 1;
        result.length = 1;
        insertResult(result);
        result.line = node.range().end().line() + 1;
        result.column = node.range().end().character();
        insertResult(result);
        return;
    }

    const int opStringLen = detail.at(0).isSpace() ? detail.length() - 1 : detail.length();

    // The simple case: Call to operator+, +=, * etc.
    if (nodeEndPos - nodeStartPos == opStringLen) {
        setResultPosFromRange(result, node.range());
        insertResult(result);
        return;
    }

    const int prefixOffset = nodeText.indexOf(operatorPrefix);
    if (prefixOffset == -1)
        return;

    const bool isArray = detail == "[]";
    const bool isCall = detail == "()";
    const bool isArrayNew = detail == " new[]";
    const bool isArrayDelete = detail == " delete[]";
    const QStringView searchTerm = isArray || isCall
            ? QStringView(detail).chopped(1) : isArrayNew || isArrayDelete
              ? QStringView(detail).chopped(2) : detail;
    const int opStringOffset = nodeText.indexOf(searchTerm, prefixOffset
                                                + operatorPrefix.length());
    if (opStringOffset == -1 || nodeText.indexOf(operatorPrefix, opStringOffset) != -1)
        return;

    const int opStringOffsetInDoc = nodeStartPos + opStringOffset
            + detail.length() - opStringLen;
    Utils::Text::convertPosition(m_doc, opStringOffsetInDoc, &result.line, &result.column);
    ++result.column;
    result.length = opStringLen;
    if (isArray || isCall)
        result.length = 1;
    else if (isArrayNew || isArrayDelete)
        result.length -= 2;
    if (!isArray && !isCall)
        insertResult(result);
    if (!isArray && !isCall && !isArrayNew && !isArrayDelete)
        return;

    result.textStyles.mainStyle = C_PUNCTUATION;
    result.length = 1;
    const int openingParenOffset = nodeText.indexOf(
                isCall ? '(' : '[', prefixOffset + operatorPrefix.length());
    if (openingParenOffset == -1)
        return;
    const int closingParenOffset = nodeText.indexOf(isCall ? ')' : ']', openingParenOffset + 1);
    if (closingParenOffset == -1 || closingParenOffset < openingParenOffset)
        return;
    Utils::Text::convertPosition(m_doc, nodeStartPos + openingParenOffset,
                                 &result.line, &result.column);
    ++result.column;
    insertResult(result);
    Utils::Text::convertPosition(m_doc, nodeStartPos + closingParenOffset,
                                 &result.line, &result.column);
    ++result.column;
    insertResult(result);
}

void ExtraHighlightingResultsCollector::visitNode(const ClangdAstNode &node)
{
    if (m_promise.isCanceled())
        return;
    const ClangdAstNode::FileStatus prevFileStatus = m_currentFileStatus;
    m_currentFileStatus = node.fileStatus(m_filePath);
    if (m_currentFileStatus == ClangdAstNode::FileStatus::Unknown
            && prevFileStatus != ClangdAstNode::FileStatus::Ours) {
        m_currentFileStatus = prevFileStatus;
    }
    switch (m_currentFileStatus) {
    case ClangdAstNode::FileStatus::Ours:
    case ClangdAstNode::FileStatus::Unknown:
        collectFromNode(node);
        [[fallthrough]];
    case ClangdAstNode::FileStatus::Foreign:
    case ClangCodeModel::Internal::ClangdAstNode::FileStatus::Mixed: {
        const auto children = node.children();
        if (!children)
            return;
        for (const ClangdAstNode &childNode : *children)
            visitNode(childNode);
        break;
    }
    }
    m_currentFileStatus = prevFileStatus;
}

class InactiveRegionsParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    DocumentUri uri() const { return TextDocumentIdentifier(value("textDocument")).uri(); }
    QList<Range> inactiveRegions() const { return array<Range>("regions"); }
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
    const QList<Range> inactiveRegions = params->inactiveRegions();
    QList<BlockRange> ifdefedOutBlocks;
    for (const Range &r : inactiveRegions) {
        const int startPos = r.start().toPositionInDocument(doc->document());
        const int endPos = r.end().toPositionInDocument(doc->document()) + 1;
        ifdefedOutBlocks.emplaceBack(startPos, endPos);
    }
    doc->setIfdefedOutBlocks(ifdefedOutBlocks);
}

QString inactiveRegionsMethodName()
{
    return "textDocument/inactiveRegions";
}

} // namespace ClangCodeModel::Internal
