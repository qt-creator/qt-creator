#include "idcollector.h"
#include "qmljsast_p.h"
#include "qmljsengine_p.h"

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace DuiEditor;
using namespace DuiEditor::Internal;

QMap<QString, QmlIdSymbol*> IdCollector::operator()(const QString &fileName, QmlJS::AST::UiProgram *ast)
{
    _fileName = fileName;
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
                addId(i->name->asString(), ast);

    return false;
}

void IdCollector::addId(const QString &id, QmlJS::AST::UiScriptBinding *ast)
{
    if (!_ids.contains(id)) {
        Node *parent = _scopes.top();

        if (UiObjectBinding *binding = cast<UiObjectBinding*>(parent))
            _ids[id] = new QmlIdSymbol(_fileName, ast, QmlSymbolFromFile(_fileName, binding));
        else if (UiObjectDefinition *definition = cast<UiObjectDefinition*>(parent))
            _ids[id] = new QmlIdSymbol(_fileName, ast, QmlSymbolFromFile(_fileName, definition));
        else
            Q_ASSERT(!"Unknown parent for id");
    }
}
