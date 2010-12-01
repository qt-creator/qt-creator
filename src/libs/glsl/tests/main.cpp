
#include <glslengine.h>
#include <glslparser.h>
#include <glsllexer.h>
#include <glslastdump.h>
#include <glslsemantic.h>
#include <glslsymbols.h>
#include <glsltypes.h>

#include <QtCore/QTextStream>

#include <iostream>
#include <fstream>
#include <cstring>
#include <cassert>
#include <cstdio>

using namespace GLSL;

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

namespace {
QTextStream qout(stdout, QIODevice::WriteOnly);
}

int main(int argc, char *argv[])
{
    int variant = 0;

    while (argc > 1 && argv[1][0] == '-' && argv[1][1] == '-') {
        if (!strcmp(argv[1], "--version=1.20")) {
            variant |= Lexer::Variant_GLSL_120;
        } else if (!strcmp(argv[1], "--version=1.50")) {
            variant |= Lexer::Variant_GLSL_150;
        } else if (!strcmp(argv[1], "--version=4.00")) {
            variant |= Lexer::Variant_GLSL_400;
        } else if (!strcmp(argv[1], "--version=es")) {
            variant |= Lexer::Variant_GLSL_ES_100;
        } else if (!strcmp(argv[1], "--shader=vertex")) {
            variant |= Lexer::Variant_VertexShader;
        } else if (!strcmp(argv[1], "--shader=fragment")) {
            variant |= Lexer::Variant_FragmentShader;
        } else {
            std::cerr << "glsl: unknown option: " << argv[1] << std::endl;
            return EXIT_FAILURE;
        }
        ++argv;
        --argc;
    }

    if (argc != 2) {
        std::cerr << "glsl: no input file" << std::endl;
        return EXIT_FAILURE;
    }

    std::ifstream fin(argv[1]);
    if (! fin) {
        std::cerr << "glsl: No such file or directory" << std::endl;
        return EXIT_FAILURE;
    }
    fin.seekg(0, std::ios::end);
    size_t size = fin.tellg();
    fin.seekg(0, std::ios::beg);
    char *source = new char[size];
    fin.read(source, size);
    fin.close();
    if (!variant)
        variant = Lexer::Variant_Mask & ~Lexer::Variant_Reserved;
    else if ((variant & (Lexer::Variant_VertexShader | Lexer::Variant_FragmentShader)) == 0)
        variant |= Lexer::Variant_VertexShader | Lexer::Variant_FragmentShader;
    Engine engine;
    Parser parser(&engine, source, size, variant);
    TranslationUnitAST *ast = parser.parse();
    std::cout << argv[1] << (ast ? " OK " : " KO ") << std::endl;

    ASTDump dump(qout);
    dump(ast);

    Semantic sem;
    Scope *globalScope = engine.newNamespace();
    sem.translationUnit(ast, globalScope, &engine);

    delete source;
    delete ast;

    return EXIT_SUCCESS;
}
