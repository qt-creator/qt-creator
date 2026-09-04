// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qglobal.h>

#if defined(CMAKELANG_LIBRARY)
#  define CMAKELANG_EXPORT Q_DECL_EXPORT
#elif defined(CMAKELANG_STATIC_LIBRARY)
#  define CMAKELANG_EXPORT
#else
#  define CMAKELANG_EXPORT Q_DECL_IMPORT
#endif

namespace CMakeLang {

class Document;
class Engine;
class Lexer;
class MemoryPool;
class Parser;
class Token;
class Visitor;

class AST;
class SourceFileAST;
class ElementAST;
class CommandAST;
class IfAST;
class ElseIfClauseAST;
class ElseClauseAST;
class NestedCommandAST;
class ForEachAST;
class WhileAST;
class FunctionAST;
class MacroAST;
class BlockAST;
class ArgumentAST;
class UnquotedArgumentAST;
class QuotedArgumentAST;
class BracketArgumentAST;
class ParenGroupArgumentAST;

template <typename T> class List;
template <typename T> class ListView;

} // namespace CMakeLang
