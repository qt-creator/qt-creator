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
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
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

#ifndef NAME_COMPILER_H
#define NAME_COMPILER_H

#include "default_visitor.h"
#include <QtCore/QString>

class TokenStream;
class Binder;

class NameCompiler: protected DefaultVisitor
{
public:
  NameCompiler(Binder *binder);

  void run(NameAST *node) { internal_run(node); }
  void run(UnqualifiedNameAST *node) { internal_run(node); }

  QString qualifiedName() const { return _M_name; }

protected:
  virtual void visitUnqualifiedName(UnqualifiedNameAST *node);
  virtual void visitTemplateArgument(TemplateArgumentAST *node);

  void internal_run(AST *node);
  QString decode_operator(std::size_t index) const;

private:
  Binder *_M_binder;
  TokenStream *_M_token_stream;
  QString _M_name;
};

#endif // NAME_COMPILER_H

// kate: space-indent on; indent-width 2; replace-tabs on;
