/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef GLSL_H
#define GLSL_H

#include <qglobal.h>
#include <cstdlib>
#include <cstddef>

#if defined(GLSL_BUILD_LIB)
#  define GLSL_EXPORT Q_DECL_EXPORT
#elif defined(GLSL_BUILD_STATIC_LIB)
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

#endif // GLSL_H
