#include "idcollector.h"
#include "qmljsast_p.h"
#include "qmljsengine_p.h"

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace DuiEditor::Internal;

QMap<QString, QPair<QmlJS::AST::SourceLocation, QmlJS::AST::Node*> > IdCollector::operator()(QmlJS::AST::UiProgram *ast)
{
    _ids.clear();

    Node::accept(ast, this);

    return _ids;
}

bool IdCollector::visit(QmlJS::AST::UiObjectBinding *ast)
{
    _scopes.push(ast);
    return true;
}

bool IdCollector::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    _scopes.push(ast);
    return true;
}

void IdCollector::endVisit(QmlJS::AST::UiObjectBinding *)
{
    _scopes.pop();
}

void IdCollector::endVisit(QmlJS::AST::UiObjectDefinition *)
{
    _scopes.pop();
}

bool IdCollector::visit(QmlJS::AST::UiScriptBinding *ast)
{
    if (!(ast->qualifiedId->next) && ast->qualifiedId->name->asString() == "id")
        if (ExpressionStatement *e = cast<ExpressionStatement*>(ast->statement))
            if (IdentifierExpression *i = cast<IdentifierExpression*>(e->expression))
                addId(i->name, i->identifierToken);

    return false;
}

void IdCollector::addId(QmlJS::NameId* id, const QmlJS::AST::SourceLocation &idLocation)
{
    const QString idString = id->asString();

    if (!_ids.contains(idString))
        _ids[idString] = qMakePair(idLocation, _scopes.top());
}
