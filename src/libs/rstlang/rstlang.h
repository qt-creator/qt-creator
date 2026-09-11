// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <parsing/engine.h>

#include <qglobal.h>

#if defined(RSTLANG_LIBRARY)
#  define RSTLANG_EXPORT Q_DECL_EXPORT
#elif defined(RSTLANG_STATIC_LIBRARY)
#  define RSTLANG_EXPORT
#else
#  define RSTLANG_EXPORT Q_DECL_IMPORT
#endif

namespace RstLang {

// The pool the AST is allocated from, the engine that owns it and what it
// reports are what every parser needs, so they are shared.
using Parsing::Diagnostic;
using Parsing::Engine;
using Parsing::Managed;
using Parsing::MemoryPool;

class Document;
class Lexer;
class Parser;
class Token;
class Visitor;

class AST;
class DocumentAST;
class BlockAST;
class LineAST;
class ParagraphAST;
class LiteralBlockAST;
class LineBlockAST;
class BlockQuoteAST;
class SectionAST;
class DefinitionItemAST;
class BulletItemAST;
class EnumeratedItemAST;
class FieldAST;
class DirectiveAST;
class SubstitutionAST;
class TargetAST;
class CommentAST;
class TransitionAST;
class FootnoteAST;
class TableAST;

template <typename T> class List;
template <typename T> class ListView;

} // namespace RstLang
