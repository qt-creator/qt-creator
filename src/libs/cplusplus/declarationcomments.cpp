// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "declarationcomments.h"

#include <cplusplus/ASTPath.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/Overview.h>

#include <utils/algorithm.h>
#include <utils/textutils.h>

#include <QRegularExpression>
#include <QStringList>
#include <QTextBlock>
#include <QTextDocument>

namespace CPlusPlus {

static QString nameFromSymbol(const Symbol *symbol)
{
    const QStringList symbolParts = Overview().prettyName(symbol->name())
                                        .split("::", Qt::SkipEmptyParts);
    if (symbolParts.isEmpty())
        return {};
    return symbolParts.last();
}

static QList<Token> commentsForDeclaration(
    const AST *decl, const QString &symbolName, const QTextDocument &textDoc,
    const Document::Ptr &cppDoc, bool isParameter)
{
    if (symbolName.isEmpty())
        return {};

    // Get the list of all tokens (including comments) and find the declaration start token there.
    TranslationUnit * const tu = cppDoc->translationUnit();
    QTC_ASSERT(tu && tu->isParsed(), return {});
    const Token &declToken = tu->tokenAt(decl->firstToken());
    std::vector<Token> allTokens = tu->allTokens();
    QTC_ASSERT(!allTokens.empty(), return {});
    int tokenPos = -1;
    for (int i = 0; i < int(allTokens.size()); ++i) {
        if (allTokens.at(i).byteOffset == declToken.byteOffset) {
            tokenPos = i;
            break;
        }
    }
    if (tokenPos == -1)
        return {};

    // Go backwards in the token list and collect all associated comments.
    struct Comment {
        Token token;
        QTextBlock startBlock;
        QTextBlock endBlock;
    };
    QList<Comment> comments;
    Kind commentKind = T_EOF_SYMBOL;
    const auto blockForTokenStart = [&](const Token &tok) {
        return textDoc.findBlock(tu->getTokenPositionInDocument(tok, &textDoc));
    };
    const auto blockForTokenEnd = [&](const Token &tok) {
        return textDoc.findBlock(tu->getTokenEndPositionInDocument(tok, &textDoc));
    };
    bool needsSymbolReference = isParameter;
    for (int i = tokenPos - 1; i >= 0; --i) {
        const Token &tok = allTokens.at(i);
        if (!tok.isComment())
            break;
        const QTextBlock tokenEndBlock = blockForTokenEnd(tok);
        if (commentKind == T_EOF_SYMBOL) {
            if (tokenEndBlock.next() != blockForTokenStart(declToken))
                needsSymbolReference = true;
            commentKind = tok.kind();
        } else {
            // If it's not the same kind of comment, it's not part of our comment block.
            if (tok.kind() != commentKind)
                break;

            // If there are empty lines between the comments, we don't consider them as
            // belonging together.
            if (tokenEndBlock.next() != comments.first().startBlock)
                break;
        }

        comments.push_front({tok, blockForTokenStart(tok), tokenEndBlock});
    }

    if (comments.isEmpty())
        return {};

    const auto tokenList = [&] {
        return Utils::transform<QList<Token>>(comments, &Comment::token);
    };

    // We consider the comment block as associated with the symbol if it
    //   a) precedes it directly, without any empty lines in between or
    //   b) the symbol name occurs in it.
    // Obviously, this heuristic can yield false positives in the case of very short names,
    // but if a symbol is important enough to get documented, it should also have a proper name.
    // Note that for function parameters, we always require the name to occur in the comment.

    if (!needsSymbolReference) // a)
        return tokenList();

    // b)
    const Kind tokenKind = comments.first().token.kind();
    const bool isDoxygenComment = tokenKind == T_DOXY_COMMENT || tokenKind == T_CPP_DOXY_COMMENT;
    const QRegularExpression symbolRegExp(QString("%1\\b%2\\b").arg(
        isParameter && isDoxygenComment ? "[\\@]param\\s+" : QString(), symbolName));
    for (const Comment &c : std::as_const(comments)) {
        for (QTextBlock b = c.startBlock; b.blockNumber() <= c.endBlock.blockNumber();
             b = b.next()) {
            if (b.text().contains(symbolRegExp))
                return tokenList();
        }
    }
    return {};
}


QList<Token> commentsForDeclaration(const Symbol *symbol, const QTextDocument &textDoc,
                                    const Document::Ptr &cppDoc)
{
    QTC_ASSERT(cppDoc->translationUnit() && cppDoc->translationUnit()->isParsed(), return {});
    Utils::Text::Position pos;
    cppDoc->translationUnit()->getTokenPosition(symbol->sourceLocation(), &pos.line, &pos.column);
    --pos.column;
    return commentsForDeclaration(nameFromSymbol(symbol), pos, textDoc, cppDoc);
}

QList<Token> commentsForDeclaration(const QString &symbolName, const Utils::Text::Position &pos,
                                    const QTextDocument &textDoc, const Document::Ptr &cppDoc)
{
    if (symbolName.isEmpty())
        return {};

    // Find the symbol declaration's AST node.
    // We stop at the last declaration node that precedes the symbol, except:
    //  - For parameter declarations, we just continue, because we are interested in the function.
    //  - If the declaration node is preceded directly by another one, we choose that one instead,
    //    because with nested declarations we want the outer one (e.g. templates).
    const QList<AST *> astPath = ASTPath(cppDoc)(pos.line, pos.column + 1);
    if (astPath.isEmpty())
        return {};
    const AST *declAst = nullptr;
    bool isParameter = false;
    for (auto it = std::next(std::rbegin(astPath)); it != std::rend(astPath); ++it) {
        AST * const node = *it;
        if (node->asParameterDeclaration()) {
            isParameter = true;
            continue;
        }
        if (node->asDeclaration()) {
            declAst = node;
            continue;
        }
        if (declAst)
            break;
    }
    if (!declAst)
        return {};

    return commentsForDeclaration(declAst, symbolName, textDoc, cppDoc, isParameter);
}

QList<Token> commentsForDeclaration(const Symbol *symbol, const AST *decl,
                                    const QTextDocument &textDoc, const Document::Ptr &cppDoc)
{
    return commentsForDeclaration(decl, nameFromSymbol(symbol), textDoc, cppDoc,
                                  symbol->asArgument());
}

} // namespace CPlusPlus
