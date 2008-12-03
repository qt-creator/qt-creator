
#include <QFile>

#include <cstdio>
#include <cstdlib>

#include <TranslationUnit.h>
#include <Control.h>
#include <AST.h>
#include <Semantic.h>
#include <Scope.h>

int main(int, char *[])
{
    Control control;
    StringLiteral *fileId = control.findOrInsertFileName("<stdin>");

    QFile in;
    if (! in.open(stdin, QFile::ReadOnly))
        return EXIT_FAILURE;

    const QByteArray source = in.readAll();

    TranslationUnit unit(&control, fileId);
    unit.setSource(source.constData(), source.size());
    unit.parse();

    if (TranslationUnitAST *ast = unit.ast()) {
        Scope globalScope;
        Semantic sem(&control);
        for (DeclarationAST *decl = ast->declarations; decl; decl = decl->next) {
            sem.check(decl, &globalScope);
        }
    }

    return EXIT_SUCCESS;
}
