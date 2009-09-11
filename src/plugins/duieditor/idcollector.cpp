#include "idcollector.h"
#include "qmljsast_p.h"
#include "qmljsengine_p.h"

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace DuiEditor::Internal;

QMap<QString, QmlJS::AST::SourceLocation> IdCollector::operator()(QmlJS::AST::UiProgram *ast)
{
    _idLocations.clear();

    Node::accept(ast, this);

    return _idLocations;
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

    if (!_idLocations.contains(idString))
        _idLocations[idString] = idLocation;
}
