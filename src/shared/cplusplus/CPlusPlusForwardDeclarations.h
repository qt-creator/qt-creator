/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/
// Copyright (c) 2008 Roberto Raggi <roberto.raggi@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef CPLUSPLUS_CPLUSPLUSFORWARDDECLARATIONS_H
#define CPLUSPLUS_CPLUSPLUSFORWARDDECLARATIONS_H

#ifdef HAVE_QT
#  include <QtCore/qglobal.h>
#  define CPLUSPLUS_BEGIN_HEADER
#  define CPLUSPLUS_END_HEADER
#  if defined(CPLUSPLUS_BUILD_LIB)
#    define CPLUSPLUS_EXPORT Q_DECL_EXPORT
#  else
#    define CPLUSPLUS_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define CPLUSPLUS_BEGIN_HEADER
#  define CPLUSPLUS_END_HEADER
#  define CPLUSPLUS_EXPORT
#endif

#ifdef CPLUSPLUS_WITH_NAMESPACE
#  define CPLUSPLUS_BEGIN_NAMESPACE namespace CPlusPlus {
#  define CPLUSPLUS_END_NAMESPACE   } // end of namespace CPlusPLus
#  define CPLUSPLUS_USE_NAMESPACE   using namespace CPlusPlus;
#else
#  define CPLUSPLUS_BEGIN_NAMESPACE
#  define CPLUSPLUS_END_NAMESPACE
#  define CPLUSPLUS_USE_NAMESPACE   ;
#endif

CPLUSPLUS_BEGIN_HEADER
CPLUSPLUS_BEGIN_NAMESPACE

class TranslationUnit;
class Semantic;
class Control;
class MemoryPool;
class DiagnosticClient;

class Identifier;
class Literal;
class StringLiteral;
class NumericLiteral;

class Scope;

// names
class NameVisitor;
class Name;
class NameId;
class TemplateNameId;
class DestructorNameId;
class OperatorNameId;
class ConversionNameId;
class QualifiedNameId;

// types
class FullySpecifiedType;
class TypeVisitor;
class Type;
class VoidType;
class IntegerType;
class FloatType;
class PointerToMemberType;
class PointerType;
class ReferenceType;
class ArrayType;
class NamedType;

// symbols
class SymbolVisitor;
class Symbol;
class ScopedSymbol;
class UsingNamespaceDirective;
class UsingDeclaration;
class Declaration;
class Argument;
class Function;
class Namespace;
class BaseClass;
class Block;
class Class;
class Enum;

class Use;

CPLUSPLUS_END_NAMESPACE
CPLUSPLUS_END_HEADER

#endif // CPLUSPLUS_CPLUSPLUSFORWARDDECLARATIONS_H
