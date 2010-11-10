#!/usr/bin/python

import sys

try:
    file = open(sys.argv[1], "r")
    klass = sys.argv[2]
except:
    print("Usage: mkvisitor.py grammar.txt classname")
    exit()

lines = file.readlines()
ruleno = 0
print("""
#include "glslast.h"

namespace GLSL {

class %s
{
    typedef void (%s::*dispatch_func)(AST *);
    static dispatch_func dispatch[];

public:
    void accept(AST *ast)
    {
        if (! ast)
            return;
        else if (Operator *op = ast->asOperator())
            (this->*dispatch[op->ruleno])(ast);
    }

    template <typename It>
    void accept(It first, It last)
    {
        for (; first != last; ++first)
            accept(*first);
    }

private:""" % (klass, klass))

for line in lines:
    sections = line.split()
    if len(sections) and sections[1] == "::=":
        ruleno = ruleno + 1
        print("    void on_%s_%d(AST *);" % (sections[0], ruleno))

print("};")
print("} // end of namespace GLSL")

print("""
#include <iostream>

using namespace GLSL;

namespace {
bool debug = false;
}

""")

print("%s::dispatch_func %s::dispatch[] = {" % (klass, klass))

ruleno = 0
for line in lines:
    sections = line.split()
    if len(sections) and sections[1] == "::=":
        ruleno = ruleno + 1
        print("    &%s::on_%s_%d," % (klass, sections[0], ruleno))

print("0, };\n")

ruleno = 0
for line in lines:
    sections = line.split()
    if len(sections) and sections[1] == "::=":
        ruleno = ruleno + 1
        print("""// %svoid %s::on_%s_%d(AST *ast)
{
    if (debug)
        std::cout << "%s" << std::endl;
    Operator *op = ast->asOperator();
    accept(op->begin(), op->end());
}
""" % (line, klass, sections[0], ruleno, line[:-3]))
