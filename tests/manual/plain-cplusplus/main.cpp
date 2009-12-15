/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "Preprocessor.h"
#include <AST.h>
#include <ASTVisitor.h>
#include <Control.h>
#include <Scope.h>
#include <Semantic.h>
#include <TranslationUnit.h>
#include <Literals.h>
#include <Symbols.h>
#include <Names.h>
#include <CoreTypes.h>

#include <string>
#include <cstdlib>
#include <sstream>

using namespace CPlusPlus;

enum { BLOCK_SIZE = 4 * 1024};

void parse(const char *fileName, const char *source, unsigned size);
int runWithSystemPreprocessor(int argc, char *argv[]);
int runWithNewPreprocessor(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    if (getenv("CPLUSPLUS_WITH_NEW_PREPROCESSOR"))
        return runWithNewPreprocessor(argc, argv);

    return runWithSystemPreprocessor(argc, argv);
}

int runWithSystemPreprocessor(int argc, char *argv[])
{
    std::string cmdline;
    cmdline += "gcc -E -xc++ -U__BLOCKS__";

    for (int i = 1; i < argc; ++i) {
        cmdline += ' ';
        cmdline += argv[i];
    }

    char block[BLOCK_SIZE];

    std::string preprocessedCode;

    if (FILE *fp = popen(cmdline.c_str(), "r")) {
        while (size_t sz = fread(block, 1, BLOCK_SIZE, fp))
            preprocessedCode.append(block, sz);

        pclose(fp);

    } else {
        fprintf(stderr, "c++: No such file or directory\n");
        return EXIT_FAILURE;
    }

    parse("<stdin>", preprocessedCode.c_str(), preprocessedCode.size());
    return EXIT_SUCCESS;
}

int runWithNewPreprocessor(int argc, char *argv[])
{
    if (argc == 1) {
        fprintf(stderr, "c++: No such file or directory\n");
        return EXIT_FAILURE;
    }

    char block[BLOCK_SIZE];

    std::string source;

    if (FILE *fp = fopen(argv[1], "r")) {
        while (size_t sz = fread(block, 1, BLOCK_SIZE, fp))
            source.append(block, sz);

        fclose(fp);

    } else {
        fprintf(stderr, "c++: No such file or directory\n");
        return EXIT_FAILURE;
    }

    std::ostringstream out;
    Preprocessor pp(out);
    pp(source.c_str(), source.size(), StringRef(argv[1]));

    const std::string preprocessedCode = out.str();
    parse(argv[1], preprocessedCode.c_str(), preprocessedCode.size());
    return EXIT_SUCCESS;
}

void parse(const char *fileName, const char *source, unsigned size)
{
    Control control;
    TranslationUnit unit(&control, control.findOrInsertStringLiteral(fileName));
    unit.setSource(source, size);
    unit.parse();

    if (TranslationUnitAST *ast = unit.ast()->asTranslationUnit()) {
        Semantic sem(&unit);
        Namespace *globalNamespace = control.newNamespace(0);
        for (List<DeclarationAST *> *it = ast->declaration_list; it; it = it->next) {
            sem.check(it->value, globalNamespace->members());
        }
    }
}
