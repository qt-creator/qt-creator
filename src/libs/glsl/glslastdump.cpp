/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "glslastdump.h"
#include <QTextStream>

#include <typeinfo>

#ifdef Q_CC_GNU
#  include <cxxabi.h>
#endif

using namespace GLSL;

ASTDump::ASTDump(QTextStream &out)
    : out(out), _depth(0)
{
}

void ASTDump::operator()(AST *ast)
{
    _depth = 0;
    accept(ast);
}

bool ASTDump::preVisit(AST *ast)
{
    const char *id = typeid(*ast).name();
#ifdef Q_CC_GNU
    char *cppId = abi::__cxa_demangle(id, 0, 0, 0);
    id = cppId;
#endif
    out << QByteArray(_depth, ' ') << id << endl;
#ifdef Q_CC_GNU
    free(cppId);
#endif
    ++_depth;
    return true;
}

void ASTDump::postVisit(AST *)
{
    --_depth;
}
