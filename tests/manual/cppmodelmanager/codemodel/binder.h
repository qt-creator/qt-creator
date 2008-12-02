/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
/* This file is part of KDevelop
    Copyright (C) 2002-2005 Roberto Raggi <roberto@kdevelop.org>
    Copyright (C) 2005 Trolltech AS

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef BINDER_H
#define BINDER_H

#include "default_visitor.h"
#include "cppcodemodel.h"
#include "type_compiler.h"
#include "name_compiler.h"
#include "declarator_compiler.h"

class TokenStream;
class LocationManager;
class Control;
struct NameSymbol;

class Binder: protected DefaultVisitor
{
public:
  Binder(CppCodeModel *__model, LocationManager &__location, Control *__control = 0);
  virtual ~Binder();

  inline TokenStream *tokenStream() const { return _M_token_stream; }
  inline CppCodeModel *model() const { return _M_model; }
  ScopeModelItem *currentScope();

  void run(AST *node, const QString &filename);

protected:
  virtual void visitAccessSpecifier(AccessSpecifierAST *);
  virtual void visitClassSpecifier(ClassSpecifierAST *);
  virtual void visitEnumSpecifier(EnumSpecifierAST *);
  virtual void visitEnumerator(EnumeratorAST *);
  virtual void visitFunctionDefinition(FunctionDefinitionAST *);
  virtual void visitLinkageSpecification(LinkageSpecificationAST *);
  virtual void visitNamespace(NamespaceAST *);
  virtual void visitSimpleDeclaration(SimpleDeclarationAST *);
  virtual void visitTemplateDeclaration(TemplateDeclarationAST *);
  virtual void visitTypedef(TypedefAST *);
  virtual void visitUsing(UsingAST *);
  virtual void visitUsingDirective(UsingDirectiveAST *);

private:
  ScopeModelItem *findScope(const QString &name) const;
  ScopeModelItem *resolveScope(NameAST *id, ScopeModelItem *scope);

  int decode_token(std::size_t index) const;
  const NameSymbol *decode_symbol(std::size_t index) const;
  CodeModel::AccessPolicy decode_access_policy(std::size_t index) const;
  CodeModel::ClassType decode_class_type(std::size_t index) const;

  CodeModel::FunctionType changeCurrentFunctionType(CodeModel::FunctionType functionType);
  CodeModel::AccessPolicy changeCurrentAccess(CodeModel::AccessPolicy accessPolicy);
  NamespaceModelItem *changeCurrentNamespace(NamespaceModelItem *item);
  CppClassModelItem *changeCurrentClass(CppClassModelItem *item);
  CppFunctionDefinitionModelItem *changeCurrentFunction(CppFunctionDefinitionModelItem *item);
  TemplateParameterList changeTemplateParameters(TemplateParameterList templateParameters);

  void declare_symbol(SimpleDeclarationAST *node, InitDeclaratorAST *init_declarator);

  void applyStorageSpecifiers(const ListNode<std::size_t> *storage_specifiers, MemberModelItem *item);
  void applyFunctionSpecifiers(const ListNode<std::size_t> *it, FunctionModelItem *item);

  void updateFileAndItemPosition(CodeModelItem *item, AST *node);

  TemplateParameterList copyTemplateParameters(const TemplateParameterList &in) const;

private:
  CppCodeModel *_M_model;
  LocationManager &_M_location;
  TokenStream *_M_token_stream;
  Control *_M_control;

  CodeModel::FunctionType _M_current_function_type;
  CodeModel::AccessPolicy _M_current_access;
  CppFileModelItem *_M_current_file;
  NamespaceModelItem *_M_current_namespace;
  CppClassModelItem *_M_current_class;
  CppFunctionDefinitionModelItem *_M_current_function;
  CppEnumModelItem *_M_current_enum;
  TemplateParameterList _M_current_template_parameters;
  QHash<QString, ScopeModelItem *> _M_known_scopes;

protected:
  TypeCompiler type_cc;
  NameCompiler name_cc;
  DeclaratorCompiler decl_cc;
};

#endif // BINDER_H

// kate: space-indent on; indent-width 2; replace-tabs on;
