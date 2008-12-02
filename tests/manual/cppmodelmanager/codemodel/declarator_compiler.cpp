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
***************************************************************************/
/* This file is part of KDevelop
    Copyright (C) 2002-2005 Roberto Raggi <roberto@kdevelop.org>

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

#include "declarator_compiler.h"
#include "name_compiler.h"
#include "type_compiler.h"
#include "compiler_utils.h"
#include "lexer.h"
#include "binder.h"

#include <qdebug.h>

DeclaratorCompiler::DeclaratorCompiler(Binder *binder)
  : _M_binder (binder), _M_token_stream (binder->tokenStream ())
{
}

void DeclaratorCompiler::run(DeclaratorAST *node)
{
  _M_id.clear();
  _M_parameters.clear();
  _M_array.clear();
  _M_function = false;
  _M_reference = false;
  _M_variadics = false;
  _M_indirection = 0;

  if (node)
    {
      NameCompiler name_cc(_M_binder);

      DeclaratorAST *decl = node;
      while (decl && decl->sub_declarator)
        decl = decl->sub_declarator;

      Q_ASSERT (decl != 0);

      name_cc.run(decl->id);
      _M_id = name_cc.qualifiedName();
      _M_function = (node->parameter_declaration_clause != 0);
      if (node->parameter_declaration_clause && node->parameter_declaration_clause->ellipsis)
        _M_variadics = true;

      visitNodes(this, node->ptr_ops);
      visit(node->parameter_declaration_clause);

      if (const ListNode<ExpressionAST*> *it = node->array_dimensions)
        {
          it->toFront();
          const ListNode<ExpressionAST*> *end = it;

          do
            {
              QString elt;
              if (ExpressionAST *expr = it->element)
                {
                  const Token &start_token = _M_token_stream->token((int) expr->start_token);
                  const Token &end_token = _M_token_stream->token((int) expr->end_token);

                  elt += QString::fromUtf8(&start_token.text[start_token.position],
                                           (int) (end_token.position - start_token.position)).trimmed();
                }

              _M_array.append (elt);

              it = it->next;
            }
          while (it != end);
        }
    }
}

void DeclaratorCompiler::visitPtrOperator(PtrOperatorAST *node)
{
  std::size_t op =  _M_token_stream->kind(node->op);

  switch (op)
    {
      case '&':
        _M_reference = true;
        break;
      case '*':
        ++_M_indirection;
        break;

      default:
        break;
    }

  if (node->mem_ptr)
    {
#if defined(__GNUC__)
#warning "ptr to mem -- not implemented"
#endif
    }
}

void DeclaratorCompiler::visitParameterDeclaration(ParameterDeclarationAST *node)
{
  Parameter p;

  TypeCompiler type_cc(_M_binder);
  DeclaratorCompiler decl_cc(_M_binder);

  decl_cc.run(node->declarator);

  p.name = decl_cc.id();
  p.type = CompilerUtils::typeDescription(node->type_specifier, node->declarator, _M_binder);
  if (node->expression != 0)
    {
      p.defaultValue = true;
      const Token &start = _M_token_stream->token((int) node->expression->start_token);
      const Token &end = _M_token_stream->token((int) node->expression->end_token);
      int length = (int) (end.position - start.position);
      p.defaultValueExpression = QString::fromUtf8(&start.text[start.position], length).trimmed();
    }

  _M_parameters.append(p);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
