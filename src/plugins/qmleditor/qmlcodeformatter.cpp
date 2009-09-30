#include "qmlcodeformatter.h"
#include "qmljsast_p.h"

using namespace QmlEditor::Internal;
using namespace QmlJS;
using namespace QmlJS::AST;

QmlCodeFormatter::QmlCodeFormatter()
{
}

QmlCodeFormatter::~QmlCodeFormatter()
{
}

bool QmlCodeFormatter::visit(QmlJS::AST::UiProgram *ast)
{
    Node::accept(ast->imports, this);

    if (ast->imports && ast->members)
        newline();

    Node::accept(ast->members, this);

    return false;
}

QString QmlCodeFormatter::operator()(QmlJS::AST::UiProgram *ast, const QString &originalSource, const QList<QmlJS::AST::SourceLocation> &comments, int start, int end)
{
    m_result.clear();
    m_result.reserve(originalSource.length() * 2);
    m_originalSource = originalSource;
    m_start = start;
    m_end = end;

    Node::acceptChild(ast, this);

    return m_result;
}

bool QmlCodeFormatter::visit(UiImport *ast)
{
    append("import ");
    append(ast->fileNameToken);

    if (ast->versionToken.isValid()) {
        append(' ');
        append(ast->versionToken);
    }

    if (ast->asToken.isValid()) {
        append(" as ");
        append(ast->importIdToken);
    }

    if (ast->semicolonToken.isValid())
        append(';');

    newline();

    return false;
}

bool QmlCodeFormatter::visit(UiObjectDefinition *ast)
{
    indent();
    Node::accept(ast->qualifiedTypeNameId, this);
    append(' ');
    Node::accept(ast->initializer, this);
    newline();

    return false;
}

bool QmlCodeFormatter::visit(QmlJS::AST::UiQualifiedId *ast)
{
    for (UiQualifiedId *it = ast; it; it = it->next) {
        append(it->name->asString());

        if (it->next)
            append('.');
    }

    return false;
}

bool QmlCodeFormatter::visit(QmlJS::AST::UiObjectInitializer *ast)
{
    append(ast->lbraceToken.offset, ast->rbraceToken.end() - ast->lbraceToken.offset);

    return false;
}
