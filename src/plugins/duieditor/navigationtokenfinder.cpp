#include "qmljsast_p.h"
#include "qmljsengine_p.h"

#include "navigationtokenfinder.h"

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace DuiEditor;
using namespace DuiEditor::Internal;

void NavigationTokenFinder::operator()(const DuiDocument::Ptr &doc, int position, const Snapshot &snapshot)
{
    _doc = doc;
    _snapshot = snapshot;
    _pos = position;
    _scopes.clear();
    _linkPosition = -1;
    _fileName.clear();
    _targetLine = -1;

    Node::accept(doc->program(), this);
}

static QStringList buildQualifiedId(QmlJS::AST::FieldMemberExpression *expr)
{
    QStringList qId;

    if (FieldMemberExpression *baseExpr = cast<FieldMemberExpression*>(expr->base)) {
        qId = buildQualifiedId(baseExpr);
    } else if (IdentifierExpression *idExpr = cast<IdentifierExpression*>(expr->base)) {
        qId.append(idExpr->name->asString());
    } else {
        return qId;
    }

    qId.append(expr->name->asString());
    return qId;
}

bool NavigationTokenFinder::visit(QmlJS::AST::FieldMemberExpression *ast)
{
    if (linkFound())
        return false;

    if (ast->firstSourceLocation().offset <= _pos && _pos <= ast->lastSourceLocation().end()) {
        if (ast->identifierToken.offset <= _pos && _pos <= ast->identifierToken.end()) {
            // found it:
            _linkPosition = ast->identifierToken.offset;
            _linkLength = ast->identifierToken.end() - _linkPosition;

            const QStringList qualifiedId(buildQualifiedId(ast));
            if (!qualifiedId.isEmpty())
                findDeclaration(qualifiedId);
        } else {
            Node::accept(ast->base, this);
        }

        return false;
    }

    return true;
}

bool NavigationTokenFinder::visit(QmlJS::AST::IdentifierExpression *ast)
{
    if (linkFound())
        return false;

    if (ast->identifierToken.offset <= _pos && _pos <= ast->identifierToken.end()) {
        _linkPosition = ast->identifierToken.offset;
        _linkLength = ast->identifierToken.length;

        findDeclaration(QStringList() << ast->name->asString());
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

bool NavigationTokenFinder::visit(QmlJS::AST::UiImportList *)
{
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

    return !linkFound();
}

void NavigationTokenFinder::endVisit(QmlJS::AST::Block *)
{
    _scopes.pop();
}

bool NavigationTokenFinder::visit(QmlJS::AST::UiObjectBinding *ast)
{
    _scopes.push(ast);

    if (!linkFound()) {
        checkType(ast->qualifiedTypeNameId);
        Node::accept(ast->initializer, this);
    }

    return false;
}

void NavigationTokenFinder::endVisit(QmlJS::AST::UiObjectBinding *)
{
    _scopes.pop();
}

bool NavigationTokenFinder::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    _scopes.push(ast);

    if (!linkFound()) {
        checkType(ast->qualifiedTypeNameId);
        Node::accept(ast->initializer, this);
    }

    return false;
}

void NavigationTokenFinder::endVisit(QmlJS::AST::UiObjectDefinition *)
{
    _scopes.pop();
}

void NavigationTokenFinder::checkType(QmlJS::AST::UiQualifiedId *ast)
{
    if (ast->identifierToken.offset <= _pos) {
        for (UiQualifiedId *iter = ast; iter; iter = iter->next) {
            if (_pos <= iter->identifierToken.end()) {
                _linkPosition = ast->identifierToken.offset;

                QStringList names;
                for (UiQualifiedId *iter2 = ast; iter2; iter2 = iter2->next) {
                    _linkLength = iter2->identifierToken.end() - _linkPosition;
                    names.append(iter2->name->asString());
                }
                findTypeDeclaration(names);

                return;
            }
        }
    }
}

bool NavigationTokenFinder::visit(QmlJS::AST::UiScriptBinding *ast)
{
    if (!linkFound()) {
        if (ast->qualifiedId && !ast->qualifiedId->next && ast->qualifiedId->name && ast->qualifiedId->name->asString() == "id")
            return false;
        else
            Node::accept(ast->statement, this);
    }

    return false;
}

bool NavigationTokenFinder::visit(QmlJS::AST::UiSourceElement * /*ast*/)
{
    return false;
}

bool NavigationTokenFinder::findInJS(const QStringList &qualifiedId, QmlJS::AST::Block *block)
{
    if (qualifiedId.size() > 1)
        return false; // we can only find "simple" JavaScript variables this way

    const QString id = qualifiedId[0];

    for (StatementList *iter = block->statements; iter; iter = iter->next) {
        Statement *stmt = iter->statement;

        if (VariableStatement *varStmt = cast<VariableStatement*>(stmt)) {
            for (VariableDeclarationList *varIter = varStmt->declarations; varIter; varIter = varIter->next) {
                if (varIter->declaration && varIter->declaration->name && varIter->declaration->name->asString() == id) {
                    rememberLocation(varIter->declaration->identifierToken);
                    return true;
                }
            }
        }
    }

    return false;
}

static bool matches(UiQualifiedId *candidate, const QStringList &wanted)
{
    UiQualifiedId *iter = candidate;
    for (int i = 0; i < wanted.size(); ++i) {
        if (!iter)
            return false;

        if (iter->name->asString() != wanted[i])
            return false;

        iter = iter->next;
    }

    return !iter;
}

bool NavigationTokenFinder::findProperty(const QStringList &qualifiedId, QmlJS::AST::UiQualifiedId *typeId, QmlJS::AST::UiObjectMemberList *ast, int scopeLevel)
{
    // 1. try the "overridden" properties:
    for (UiObjectMemberList *iter = ast; iter; iter = iter->next) {
        UiObjectMember *member = iter->member;

        if (UiPublicMember *publicMember = cast<UiPublicMember*>(member)) {
            if (publicMember->name && qualifiedId.size() == 1 && publicMember->name->asString() == qualifiedId.first()) {
                rememberLocation(publicMember->identifierToken);
                return true;
            }
        } else if (UiObjectBinding *objectBinding = cast<UiObjectBinding*>(member)) {
            if (matches(objectBinding->qualifiedId, qualifiedId)) {
                rememberLocation(objectBinding->qualifiedId->identifierToken);
                return true;
            }
        } else if (UiScriptBinding *scriptBinding = cast<UiScriptBinding*>(member)) {
            if (matches(scriptBinding->qualifiedId, qualifiedId)) {
                rememberLocation(scriptBinding->qualifiedId->identifierToken);
                return true;
            }
        } else if (UiArrayBinding *arrayBinding = cast<UiArrayBinding*>(member)) {
            if (matches(arrayBinding->qualifiedId, qualifiedId)) {
                rememberLocation(arrayBinding->qualifiedId->identifierToken);
                return true;
            }
        }
    }

    // 2. if the property is "parent", go one scope level up:
    if (qualifiedId[0] == "parent"){
        if (scopeLevel <= 0)
            return false;

        int newScopeLevel = scopeLevel - 1;
        Node *parentScope = _scopes[newScopeLevel];

        QStringList newQualifiedId = qualifiedId;
        newQualifiedId.removeFirst();
        if (UiObjectBinding *binding = cast<UiObjectBinding*>(parentScope)) {
            if (newQualifiedId.isEmpty()) {
                rememberLocation(binding->qualifiedTypeNameId->identifierToken);
                return true;
            } else {
                return findProperty(newQualifiedId, binding->qualifiedTypeNameId, binding->initializer->members, newScopeLevel);
            }
        } else if (UiObjectDefinition *definition = cast<UiObjectDefinition*>(parentScope)) {
            if (newQualifiedId.isEmpty()) {
                rememberLocation(definition->qualifiedTypeNameId->identifierToken);
                return true;
            } else {
                return findProperty(newQualifiedId, definition->qualifiedTypeNameId, definition->initializer->members, newScopeLevel);
            }
        } else {
            return false;
        }
    }

    // 3. if the type is a custom type, search properties there:
    {
        // TODO: when things around the "import as" clear up a bit, revise this resolving:

        QStringList qualifiedTypeId;
        for (UiQualifiedId *iter = typeId; iter; iter = iter->next)
            qualifiedTypeId.append(iter->name->asString());

        DuiDocument::Ptr doc = findCustomType(qualifiedTypeId);
        if (!doc.isNull() && doc->isParsedCorrectly()) {
            UiProgram *prog = doc->program();

            if (prog && prog->members && prog->members->member) {
                if (UiObjectBinding *binding = cast<UiObjectBinding*>(prog->members->member)) {
                    findProperty(qualifiedId, binding->qualifiedTypeNameId, binding->initializer->members, -1);
                } else if (UiObjectDefinition *definition = cast<UiObjectDefinition*>(prog->members->member)) {
                    findProperty(qualifiedId, definition->qualifiedTypeNameId, definition->initializer->members, -1);
                }
            }
        }
    }

    // all failed, so:
    return false;
}

void NavigationTokenFinder::findAsId(const QStringList &qualifiedId)
{
    const DuiDocument::IdTable ids = _doc->ids();

    if (ids.contains(qualifiedId.first())) {
        QPair<SourceLocation, Node*> idInfo = ids[qualifiedId.first()];
        if (qualifiedId.size() == 1) {
            rememberLocation(idInfo.first);
        } else {
            Node *parent = idInfo.second;
            QStringList newQualifiedId(qualifiedId);
            newQualifiedId.removeFirst();

            if (UiObjectBinding *binding = cast<UiObjectBinding*>(parent)) {
                findProperty(newQualifiedId, binding->qualifiedTypeNameId, binding->initializer->members, -1);
            } else if (UiObjectDefinition *definition = cast<UiObjectDefinition*>(parent)) {
                findProperty(newQualifiedId, definition->qualifiedTypeNameId, definition->initializer->members, -1);
            }
        }
    }
}

void NavigationTokenFinder::findDeclaration(const QStringList &qualifiedId, int scopeLevel)
{
    Node *scope = _scopes[scopeLevel];

    if (Block *block = cast<Block*>(scope)) {
        if (!findInJS(qualifiedId, block)) // continue searching the parent scope:
            findDeclaration(qualifiedId, scopeLevel - 1);
    } else if (UiObjectBinding *binding = cast<UiObjectBinding*>(scope)) {
        if (findProperty(qualifiedId, binding->qualifiedTypeNameId, binding->initializer->members, scopeLevel)) {
            return;
        } else {
            findAsId(qualifiedId);
        }
    } else if (UiObjectDefinition *definition = cast<UiObjectDefinition*>(scope)) {
        if (findProperty(qualifiedId, definition->qualifiedTypeNameId, definition->initializer->members, scopeLevel)) {
            return;
        } else {
            findAsId(qualifiedId);
        }
    } else {
        Q_ASSERT(!"Unknown scope type");
    }
}

void NavigationTokenFinder::findDeclaration(const QStringList &id)
{
    if (id.isEmpty())
        return;

    findDeclaration(id, _scopes.size() - 1);
}

void NavigationTokenFinder::findTypeDeclaration(const QStringList &id)
{
    DuiDocument::Ptr doc = findCustomType(id);
    if (doc.isNull() || !doc->isParsedCorrectly())
        return;

    UiProgram *prog = doc->program();
    if (!prog || !(prog->members) || !(prog->members->member))
        return;

    _fileName = doc->fileName();
    const SourceLocation loc = prog->members->member->firstSourceLocation();
    _targetLine = loc.startLine;
    _targetColumn = loc.startColumn;
}

void NavigationTokenFinder::rememberLocation(const QmlJS::AST::SourceLocation &loc)
{
    _fileName = _doc->fileName();
    _targetLine = loc.startLine;
    _targetColumn = loc.startColumn;
}

DuiDocument::Ptr NavigationTokenFinder::findCustomType(const QStringList& qualifiedId) const
{
    // TODO: when things around the "import as" clear up a bit, revise this resolving:

    UiProgram *program = _doc->program();
    if (!program)
        return DuiDocument::Ptr();
    UiImportList *imports = program->imports;
    if (!imports)
        return DuiDocument::Ptr();

    for (UiImportList *iter = imports; iter; iter = iter->next) {
        if (!(iter->import))
            continue;

        UiImport *import = iter->import;
        if (!(import->fileName))
            continue;

        const QString path = import->fileName->asString();
        const QMap<QString, DuiDocument::Ptr> compToDoc = _snapshot.componentsDefinedByImportedDocuments(_doc, path);

        if (compToDoc.contains(qualifiedId[0]))
            return compToDoc[qualifiedId[0]];
    }

    return DuiDocument::Ptr();
}
