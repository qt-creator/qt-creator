// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qglobal.h>
#include <cstdlib>
#include <cstddef>

#if defined(GLSL_LIBRARY)
#  define GLSL_EXPORT Q_DECL_EXPORT
#elif defined(GLSL_STATIC_LIBRARY)
#  define GLSL_EXPORT
#else
#  define GLSL_EXPORT Q_DECL_IMPORT
#endif

namespace GLSL {
class Engine;
class Lexer;
class Parser;
class MemoryPool;

// types
class Type;
class UndefinedType;
class VoidType;
class ScalarType;
class BoolType;
class IntType;
class UIntType;
class FloatType;
class DoubleType;
class IndexType;
class VectorType;
class MatrixType;
class ArrayType;
class SamplerType;

// symbols
class Symbol;
class Scope;
class Struct;
class Function;
class Argument;
class Block;
class Variable;
class OverloadSet;
class Namespace;

class AST;
class TranslationUnitAST;
template <typename T> class List;
}
