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

#include "Preprocessor.h"
#include <CPlusPlus.h>

#include <string>
#include <cstdlib>
#include <sstream>

using namespace CPlusPlus;

enum { BLOCK_SIZE = 4 * 1024};

void parse(const char *fileName, const char *source, unsigned size);
int runWithSystemPreprocessor(int argc, char *argv[]);
int runWithNewPreprocessor(int argc, char *argv[]);

struct V: public ASTVisitor
{
  V(TranslationUnit *unit)
    : ASTVisitor(unit) {}

  virtual bool visit(FunctionDeclaratorAST *ast)
  {
    if (ast->as_cpp_initializer) {
      if (! (ast->symbol && ast->symbol->enclosingScope()))
        ; //translationUnit()->warning(ast->firstToken(), "resolved as function declaration");
      else if (ast->symbol->enclosingScope()->isNamespace() || ast->symbol->enclosingScope()->isTemplate())
        ; //translationUnit()->warning(ast->firstToken(), "resolved as function declaration");
      else if (ast->symbol->enclosingScope()->isBlock())
        ; //translationUnit()->warning(ast->firstToken(), "resolved as C++ initializer");
      else
        translationUnit()->warning(ast->firstToken(), "ambiguous function declarator or C++ intializer");
    }
    return true;
  }

  virtual bool visit(ExpressionOrDeclarationStatementAST *ast)
  {
    translationUnit()->warning(ast->firstToken(), "ambiguous expression or declaration statement");
    return true;
  }
};

int main(int argc, char *argv[])
{
    if (getenv("CPLUSPLUS_WITH_NEW_PREPROCESSOR"))
        return runWithNewPreprocessor(argc, argv);

    return runWithSystemPreprocessor(argc, argv);
}

int runWithSystemPreprocessor(int argc, char *argv[])
{
    std::string cmdline;
    cmdline += "gcc -E -xc++ -U__BLOCKS__ -D__restrict= -D__restrict__= -D__extension__= -D__imag__= -D__real__= -D__complex__= -D_Complex= -D__signed=signed";

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
    TranslationUnit unit(&control, control.stringLiteral(fileName));
    unit.setSource(source, size);
    unit.setCxxOxEnabled(true);
    unit.parse();

#if 1
    Namespace *globalNamespace = control.newNamespace(0);
    Bind bind(&unit);
    bind(unit.ast()->asTranslationUnit(), globalNamespace);

    V v(&unit);
    v.accept(unit.ast());
#endif
}
