#include <QDebug>

#include "qmljsast_p.h"
#include "qmljsengine_p.h"

#include "navigationtokenfinder.h"

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace DuiEditor::Internal;

bool NavigationTokenFinder::operator()(QmlJS::AST::UiProgram *ast, int position, bool resolveTarget, const QMap<QString, QmlJS::AST::SourceLocation> &idPositions)
{
    _resolveTarget = resolveTarget;
    _pos = position;
    _idPositions = idPositions;
    _scopes.clear();
    _linkPosition = -1;
    _targetLine = -1;

    Node::accept(ast, this);

    if (resolveTarget)
        return targetFound();
    else
        return linkFound();
}

bool NavigationTokenFinder::visit(QmlJS::AST::IdentifierExpression *ast)
{
    if (linkFound())
        return false;

    if (ast->identifierToken.offset <= _pos && _pos <= ast->identifierToken.end()) {
        _linkPosition = ast->identifierToken.offset;
        _linkLength = ast->identifierToken.length;

        if (Node *node = findDeclarationInScopesOrIds(ast->name))
            rememberStartPosition(node);
    }

    return false;
}

bool NavigationTokenFinder::visit(QmlJS::AST::UiArrayBinding *ast)
{
    if (linkFound())
        return false;

    Node::accept(ast->members, this);

    return false;
}

bool NavigationTokenFinder::visit(QmlJS::AST::UiPublicMember *ast)
{
    if (linkFound())
        return false;

    Node::accept(ast->expression, this);

    return false;
}

bool NavigationTokenFinder::visit(QmlJS::AST::Block *ast)
{
    _scopes.push(ast);

    if (linkFound())
        return false;

    return true;
}

void NavigationTokenFinder::endVisit(QmlJS::AST::Block *)
{
    _scopes.pop();
}

bool NavigationTokenFinder::visit(QmlJS::AST::UiObjectBinding *ast)
{
    _scopes.push(ast);

    if (linkFound())
        return false;

    Node::accept(ast->qualifiedTypeNameId, this);
    Node::accept(ast->initializer, this);

    return false;
}

void NavigationTokenFinder::endVisit(QmlJS::AST::UiObjectBinding *)
{
    _scopes.pop();
}

bool NavigationTokenFinder::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    _scopes.push(ast);

    if (linkFound())
        return false;

    return true;
}

void NavigationTokenFinder::endVisit(QmlJS::AST::UiObjectDefinition *)
{
    _scopes.pop();
}

bool NavigationTokenFinder::visit(QmlJS::AST::UiQualifiedId *ast)
{
    if (linkFound())
        return false;

    if (ast->identifierToken.offset <= _pos) {
        for (UiQualifiedId *iter = ast; iter; iter = iter->next) {
            if (_pos <= iter->identifierToken.end()) {
                _linkPosition = ast->identifierToken.offset;

                for (UiQualifiedId *iter2 = ast; iter2; iter2 = iter2->next)
                    _linkLength = iter2->identifierToken.end() - _linkPosition;

                if (Node *node = findDeclarationInScopesOrIds(ast))
                    rememberStartPosition(node);

                return false;
            }
        }
    }

    return false;
}

bool NavigationTokenFinder::visit(QmlJS::AST::UiScriptBinding *ast)
{
    if (linkFound())
        return false;

    Node::accept(ast->statement, this);

    return false;
}

bool NavigationTokenFinder::visit(QmlJS::AST::UiSourceElement * /*ast*/)
{
    return false;
}

void NavigationTokenFinder::rememberStartPosition(QmlJS::AST::Node *node)
{
    if (UiObjectMember *om = dynamic_cast<UiObjectMember*>(node)) {
        _targetLine = om->firstSourceLocation().startLine;
        _targetColumn = om->firstSourceLocation().startColumn;
    } else if (VariableDeclaration *vd = cast<VariableDeclaration*>(node)) {
        _targetLine = vd->identifierToken.startLine;
        _targetColumn = vd->identifierToken.startColumn;
    } else {
        qWarning() << "Found declaration of unknown type as a navigation target";
    }
}

void NavigationTokenFinder::rememberStartPosition(const QmlJS::AST::SourceLocation &location)
{
    _targetLine = location.startLine;
    _targetColumn = location.startColumn;
}

static QmlJS::AST::Node *findDeclaration(const QString &nameId, QmlJS::AST::UiObjectMember *m)
{
    if (UiPublicMember *p = cast<UiPublicMember*>(m)) {
        if (p->name->asString() == nameId)
            return p;
    } else if (UiObjectBinding *o = cast<UiObjectBinding*>(m)) {
        if (!(o->qualifiedId->next) && o->qualifiedId->name->asString() == nameId)
            return o;
    } else if (UiArrayBinding *a = cast<UiArrayBinding*>(m)) {
        if (!(a->qualifiedId->next) && a->qualifiedId->name->asString() == nameId)
            return a;
    }

    return 0;
}

static QmlJS::AST::Node *findDeclaration(const QString &nameId, QmlJS::AST::UiObjectMemberList *l)
{
    for (UiObjectMemberList *iter = l; iter; iter = iter->next)
        if (Node *n = findDeclaration(nameId, iter->member))
            return n;

    return 0;
}

static QmlJS::AST::Node *findDeclaration(const QString &nameId, QmlJS::AST::Statement *s)
{
    if (VariableStatement *v = cast<VariableStatement*>(s)) {
        for (VariableDeclarationList *l = v->declarations; l; l = l->next) {
            if (l->declaration->name->asString() == nameId)
                return l->declaration;
        }
    }

    return 0;
}

static QmlJS::AST::Node *findDeclaration(const QString &nameId, QmlJS::AST::StatementList *l)
{
    for (StatementList *iter = l; iter; iter = iter->next)
        if (Node *n = findDeclaration(nameId, iter->statement))
            return n;

    return 0;
}

static QmlJS::AST::Node *findDeclarationAsDirectChild(const QString &nameId, QmlJS::AST::Node *node)
{
    if (UiObjectBinding *binding = cast<UiObjectBinding*>(node)) {
        return findDeclaration(nameId, binding->initializer->members);
    } else if (UiObjectDefinition *def = cast<UiObjectDefinition*>(node)) {
        return findDeclaration(nameId, def->initializer->members);
    } else if (Block *block = cast<Block *>(node)) {
        return findDeclaration(nameId, block->statements);
    } else {
        return 0;
    }
}

static QmlJS::AST::Node *findDeclarationInNode(QmlJS::AST::UiQualifiedId *qualifiedId, QmlJS::AST::Node *node)
{
    if (!qualifiedId || !node)
        return node;
    else
        return findDeclarationInNode(qualifiedId->next, findDeclarationAsDirectChild(qualifiedId->name->asString(), node));
}

QmlJS::AST::Node *NavigationTokenFinder::findDeclarationInScopesOrIds(QmlJS::NameId *nameId)
{
    if (!_resolveTarget)
        return 0;

    const QString nameAsString = nameId->asString();

    foreach (QmlJS::AST::Node *scope, _scopes) {
        Node *result = findDeclarationAsDirectChild(nameAsString, scope);

        if (result)
            return result;
    }

    if (_idPositions.contains(nameAsString)) {
        rememberStartPosition(_idPositions[nameAsString]);
    }

    return 0;
}

QmlJS::AST::Node *NavigationTokenFinder::findDeclarationInScopesOrIds(QmlJS::AST::UiQualifiedId *qualifiedId)
{
    if (!_resolveTarget)
        return 0;

    const QString nameAsString = qualifiedId->name->asString();
    qDebug() << "findDecl. for id part" << nameAsString;

    foreach (QmlJS::AST::Node *scope, _scopes) {
        Node *result = findDeclarationAsDirectChild(nameAsString, scope);

        if (result)
            return findDeclarationInNode(qualifiedId->next, result);
    }

    if (_idPositions.contains(nameAsString)) {
        rememberStartPosition(_idPositions[nameAsString]);
    }

    return 0;
}
