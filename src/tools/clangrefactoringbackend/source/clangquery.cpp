/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "clangquery.h"

#include "sourcelocationsutils.h"
#include "sourcerangeextractor.h"

#include <sourcerangescontainer.h>

#include <stringcache.h>

#include <QTime>

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/Dynamic/Diagnostics.h>
#include <clang/ASTMatchers/Dynamic/Parser.h>

using clang::ast_matchers::dynamic::Diagnostics;
using clang::ast_matchers::dynamic::Parser;
using clang::ast_matchers::BoundNodes;
using clang::ast_matchers::MatchFinder;

namespace ClangBackEnd {

struct CollectBoundNodes : MatchFinder::MatchCallback {
  std::vector<BoundNodes> &Bindings;
  CollectBoundNodes(std::vector<BoundNodes> &Bindings) : Bindings(Bindings) {}
  void run(const MatchFinder::MatchResult &Result) override {
    Bindings.push_back(Result.Nodes);
  }
};

ClangQuery::ClangQuery(FilePathCachingInterface &filePathCache,
                       Utils::SmallString &&query)
    : m_query(std::move(query)),
      m_filePathCache(filePathCache)
{
}

void ClangQuery::setQuery(Utils::SmallString &&query)
{
    this->m_query = std::move(query);
}

void ClangQuery::findLocations()
{
    auto tool = createTool();

    std::vector<std::unique_ptr<clang::ASTUnit>> asts;

    tool.buildASTs(asts);

    std::for_each (std::make_move_iterator(asts.begin()),
                   std::make_move_iterator(asts.end()),
                   [&] (std::unique_ptr<clang::ASTUnit> &&ast) {
        Diagnostics diagnostics;
        auto optionalMatcher = Parser::parseMatcherExpression({m_query.data(), m_query.size()},
                                                              nullptr,
                                                              &diagnostics);
        parseDiagnostics(diagnostics);
        matchLocation(optionalMatcher, std::move(ast));
    });
}


SourceRangesContainer ClangQuery::takeSourceRanges()
{
    return std::move(m_sourceRangesContainer);
}

DynamicASTMatcherDiagnosticContainers ClangQuery::takeDiagnosticContainers()
{
    return std::move(m_diagnosticContainers_);
}

namespace {

V2::SourceRangeContainer convertToContainer(const clang::ast_matchers::dynamic::SourceRange sourceRange)
{
    return V2::SourceRangeContainer({1, 0},
                                    sourceRange.Start.Line,
                                    sourceRange.Start.Column,
                                    0,
                                    sourceRange.End.Line,
                                    sourceRange.End.Column,
                                    0);
}

#define ERROR_RETURN_CASE(name) \
    case Diagnostics::ET_##name: return ClangQueryDiagnosticErrorType::name;

ClangQueryDiagnosticErrorType convertToErrorType(Diagnostics::ErrorType clangErrorType)
{
    switch (clangErrorType) {
       ERROR_RETURN_CASE(None)
       ERROR_RETURN_CASE(RegistryMatcherNotFound)
       ERROR_RETURN_CASE(RegistryWrongArgCount)
       ERROR_RETURN_CASE(RegistryWrongArgType)
       ERROR_RETURN_CASE(RegistryNotBindable)
       ERROR_RETURN_CASE(RegistryAmbiguousOverload)
       ERROR_RETURN_CASE(RegistryValueNotFound)
       ERROR_RETURN_CASE(ParserStringError)
       ERROR_RETURN_CASE(ParserNoOpenParen)
       ERROR_RETURN_CASE(ParserNoCloseParen)
       ERROR_RETURN_CASE(ParserNoComma)
       ERROR_RETURN_CASE(ParserNoCode)
       ERROR_RETURN_CASE(ParserNotAMatcher)
       ERROR_RETURN_CASE(ParserInvalidToken)
       ERROR_RETURN_CASE(ParserMalformedBindExpr)
       ERROR_RETURN_CASE(ParserTrailingCode)
       ERROR_RETURN_CASE(ParserUnsignedError)
       ERROR_RETURN_CASE(ParserOverloadedType)
    }

    Q_UNREACHABLE();
}

#define CONTEXT_RETURN_CASE(name) \
    case Diagnostics::CT_##name: return ClangQueryDiagnosticContextType::name;

ClangQueryDiagnosticContextType convertToContextType(Diagnostics::ContextType clangContextType)
{
    switch (clangContextType) {
       CONTEXT_RETURN_CASE(MatcherArg)
       CONTEXT_RETURN_CASE(MatcherConstruct)
    }

    Q_UNREACHABLE();
}

}

void ClangQuery::parseDiagnostics(const clang::ast_matchers::dynamic::Diagnostics &diagnostics)
{
    auto errors = diagnostics.errors();

    for (const auto &errorContent : errors) {
        m_diagnosticContainers_.emplace_back();
        DynamicASTMatcherDiagnosticContainer &diagnosticContainer = m_diagnosticContainers_.back();

        for (const auto &message : errorContent.Messages) {
            diagnosticContainer.insertMessage(convertToContainer(message.Range),
                                              convertToErrorType(message.Type),
                                              Utils::SmallStringVector(message.Args));
        }

        for (const auto &message : errorContent.ContextStack) {
            diagnosticContainer.insertContext(convertToContainer(message.Range),
                                              convertToContextType(message.Type),
                                              Utils::SmallStringVector(message.Args));
        }
    }
}

namespace {
std::vector<clang::SourceRange> generateSourceRangesFromMatches(const std::vector<BoundNodes> &matches)
{
    std::vector<clang::SourceRange> sourceRanges;
    sourceRanges.reserve(matches.size());

    for (const auto boundNodes : matches) {
        for (const auto &mapEntry : boundNodes.getMap()) {
            const auto sourceRange = mapEntry.second.getSourceRange();
            if (sourceRange.isValid())
                sourceRanges.push_back(sourceRange);
        }

    }

    return sourceRanges;
}
}

void ClangQuery::matchLocation(
        const llvm::Optional< clang::ast_matchers::internal::DynTypedMatcher> &optionalStartMatcher,
        std::unique_ptr<clang::ASTUnit> ast)
{
    if (optionalStartMatcher) {
        auto matcher = *optionalStartMatcher;
        auto optionalMatcher = matcher.tryBind("root");
        matcher = *optionalMatcher;

        MatchFinder finder;
        std::vector<BoundNodes> matches;
        CollectBoundNodes collectBoundNodes(matches);

        finder.addDynamicMatcher(matcher, &collectBoundNodes);

        finder.matchAST(ast->getASTContext());

        auto sourceRanges = generateSourceRangesFromMatches(matches);

        SourceRangeExtractor extractor(ast->getSourceManager(),
                                       ast->getLangOpts(),
                                       m_filePathCache,
                                       m_sourceRangesContainer);
        extractor.addSourceRanges(sourceRanges);

    }
}

} // namespace ClangBackEnd
