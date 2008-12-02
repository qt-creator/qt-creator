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
    Copyright (C) 2005 Trolltech

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

#include "name_compiler.h"
#include "type_compiler.h"
#include "declarator_compiler.h"
#include "lexer.h"
#include "symbol.h"
#include "binder.h"

#include <QtCore/qdebug.h>

NameCompiler::NameCompiler(Binder *binder)
  : _M_binder (binder), _M_token_stream (binder->tokenStream ())
{
}

QString NameCompiler::decode_operator(std::size_t index) const
{
  const Token &tk = _M_token_stream->token((int) index);
  return QString::fromUtf8(&tk.text[tk.position], (int) tk.size);
}

void NameCompiler::internal_run(AST *node)
{
  _M_name.clear();
  visit(node);
}

void NameCompiler::visitUnqualifiedName(UnqualifiedNameAST *node)
{
  QString tmp_name;

  if (node->tilde)
    tmp_name += QLatin1String("~");

  if (node->id)
    tmp_name += _M_token_stream->symbol(node->id)->as_string();

  if (OperatorFunctionIdAST *op_id = node->operator_id)
    {
#if defined(__GNUC__)
#warning "NameCompiler::visitUnqualifiedName() -- implement me"
#endif

      if (op_id->op && op_id->op->op)
        {
          tmp_name += QLatin1String("operator");
          tmp_name += decode_operator(op_id->op->op);
          if (op_id->op->close)
            tmp_name += decode_operator(op_id->op->close);
        }
      else if (op_id->type_specifier)
        {
#if defined(__GNUC__)
#warning "don't use an hardcoded string as cast' name"
#endif
          Token const &tk = _M_token_stream->token ((int) op_id->start_token);
          Token const &end_tk = _M_token_stream->token ((int) op_id->end_token);
          tmp_name += QString::fromLatin1 (&tk.text[tk.position],
                                           (int) (end_tk.position - tk.position)).trimmed ();
      }
    }

  if (!_M_name.isEmpty())
      _M_name += QLatin1String("::");
  _M_name += tmp_name;

  if (node->template_arguments)
    {
      _M_name += QLatin1String("<");
      visitNodes(this, node->template_arguments);
      _M_name.truncate(_M_name.count() - 1); // remove the last ','
      _M_name += QLatin1String(">");
    }
}

void NameCompiler::visitTemplateArgument(TemplateArgumentAST *node)
{
  if (node->type_id && node->type_id->type_specifier)
    {
      TypeCompiler type_cc(_M_binder);
      type_cc.run(node->type_id->type_specifier);

      DeclaratorCompiler decl_cc(_M_binder);
      decl_cc.run(node->type_id->declarator);

      if (type_cc.isConstant())
        _M_name += "const ";

      _M_name += type_cc.qualifiedName ();

      if (decl_cc.isReference())
        _M_name += "&";
      if (decl_cc.indirection())
        _M_name += QString(decl_cc.indirection(), '*');

      _M_name += QLatin1String(",");
    }
}

// kate: space-indent on; indent-width 2; replace-tabs on;
