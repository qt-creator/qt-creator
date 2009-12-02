#include <QDebug>

#include "qmlidcollector.h"
#include "qmljsast_p.h"
#include "qmljsengine_p.h"

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlEditor;
using namespace QmlEditor::Internal;

QMap<QString, QmlIdSymbol*> QmlIdCollector::operator()(QmlDocument *doc)
{
    _doc = doc;
    _ids.clear();
    _currentSymbol = 0;

    Node::accept(doc->program(), this);

    return _ids;
}

bool QmlIdCollector::visit(UiArrayBinding *ast)
{
    QmlSymbolFromFile *oldSymbol = switchSymbol(ast);
    Node::accept(ast->members, this);
    _currentSymbol = oldSymbol;
    return false;
}

bool QmlIdCollector::visit(QmlJS::AST::UiObjectBinding *ast)
{
    QmlSymbolFromFile *oldSymbol = switchSymbol(ast);
    Node::accept(ast->initializer, this);
    _currentSymbol = oldSymbol;
    return false;
}

bool QmlIdCollector::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    QmlSymbolFromFile *oldSymbol = switchSymbol(ast);
    Node::accept(ast->initializer, this);
    _currentSymbol = oldSymbol;
    return false;
}

bool QmlIdCollector::visit(QmlJS::AST::UiScriptBinding *ast)
{
    if (!(ast->qualifiedId->next) && ast->qualifiedId->name->asString() == "id")
        if (ExpressionStatement *e = cast<ExpressionStatement*>(ast->statement))
            if (IdentifierExpression *i = cast<IdentifierExpression*>(e->expression))
                if (i->name)
                    addId(i->name->asString(), ast);

    return false;
}

QmlSymbolFromFile *QmlIdCollector::switchSymbol(QmlJS::AST::UiObjectMember *node)
{
    QmlSymbolFromFile *newSymbol = 0;

    if (_currentSymbol == 0) {
        newSymbol = _doc->findSymbol(node);
    } else {
        newSymbol = _currentSymbol->findMember(node);
    }

    QmlSymbolFromFile *oldSymbol = _currentSymbol;

    if (newSymbol) {
        _currentSymbol = newSymbol;
    } else {
        QString filename = _doc->fileName();
        qWarning() << "Scope without symbol @"<<filename<<":"<<node->firstSourceLocation().startLine<<":"<<node->firstSourceLocation().startColumn;
    }

    return oldSymbol;
}

void QmlIdCollector::addId(const QString &id, QmlJS::AST::UiScriptBinding *ast)
{
    if (!_ids.contains(id) && _currentSymbol) {
        QmlSymbolFromFile *symbol = _currentSymbol->findMember(ast);

        if (QmlIdSymbol *idSymbol = symbol->asIdSymbol())
            _ids[id] = idSymbol;
    }
}
