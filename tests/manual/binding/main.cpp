/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include <AST.h>
#include <ASTVisitor.h>
#include <Control.h>
#include <Scope.h>
#include <Semantic.h>
#include <TranslationUnit.h>
#include <PrettyPrinter.h>

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>

static int usage()
{
    std::cerr << "cplusplus0: no input files" << std::endl;
    return EXIT_FAILURE;
}

int main(int argc, char *argv[])
{
    if (argc == 1)
        return usage();

    std::fstream in(argv[1]);

    if (! in)
        return usage();

    in.seekg(0, std::ios::end);
    const size_t size = in.tellg();
    in.seekp(0, std::ios::beg);

    std::vector<char> source(size + 1);
    in.read(&source[0], size);
    source[size] = '\0';

    Control control;
    StringLiteral *fileId = control.findOrInsertFileName(argv[1]);
    TranslationUnit unit(&control, fileId);
    unit.setObjCEnabled(true);
    unit.setSource(&source[0], source.size());
    unit.parse();
    if (! unit.ast())
        return EXIT_FAILURE;

    TranslationUnitAST *ast = unit.ast()->asTranslationUnit();
    assert(ast != 0);

    Scope globalScope;
    Semantic sem(&control);
    for (DeclarationAST *decl = ast->declarations; decl; decl = decl->next) {
        sem.check(decl, &globalScope);
    }

    return EXIT_SUCCESS;
}
