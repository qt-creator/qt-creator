/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/
#ifndef CPLUSPLUS_CPLUSPLUS_H
#define CPLUSPLUS_CPLUSPLUS_H

#include "AST.h"
#include "ASTMatcher.h"
#include "ASTPatternBuilder.h"
#include "ASTVisitor.h"
#include "ASTfwd.h"
#include "Bind.h"
#include "CPlusPlusForwardDeclarations.h"
#include "Control.h"
#include "CoreTypes.h"
#include "DiagnosticClient.h"
#include "FullySpecifiedType.h"
#include "Lexer.h"
#include "LiteralTable.h"
#include "Literals.h"
#include "MemoryPool.h"
#include "Name.h"
#include "NameVisitor.h"
#include "Names.h"
#include "ObjectiveCTypeQualifiers.h"
#include "Parser.h"
#include "QtContextKeywords.h"
#include "Scope.h"
#include "Symbol.h"
#include "SymbolVisitor.h"
#include "Symbols.h"
#include "Token.h"
#include "TranslationUnit.h"
#include "Type.h"
#include "TypeMatcher.h"
#include "TypeVisitor.h"

#endif // CPLUSPLUS_CPLUSPLUS_H
