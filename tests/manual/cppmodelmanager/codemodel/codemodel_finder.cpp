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

#include "codemodel_finder.h"
#include "cppcodemodel.h"
#include "binder.h"

CodeModelFinder::CodeModelFinder(CppCodeModel *model, Binder *binder)
: _M_model(model),
_M_binder (binder),
name_cc(_M_binder)
{
}

CodeModelFinder::~CodeModelFinder()
{
}

QString CodeModelFinder::resolveScope(NameAST *name, ScopeModelItem *scope, bool *ok)
{
    Q_ASSERT(scope != 0);

    _M_ok = true;
    _M_current_scope = scope->qualifiedName();
    visit(name);
    *ok = _M_ok;

    return _M_current_scope;
}

void CodeModelFinder::visitName(NameAST *node)
{
    visitNodes(this, node->qualified_names);
}

void CodeModelFinder::visitUnqualifiedName(UnqualifiedNameAST *node)
{
    if (!_M_ok)
        return;

    if (!_M_current_scope.isEmpty())
        _M_current_scope += QLatin1String("::");

    name_cc.run(node);
    _M_current_scope += name_cc.qualifiedName();

    if (!_M_model->hasScope(_M_current_scope))
        _M_ok = false;
}

// kate: space-indent on; indent-width 2; replace-tabs on;

