/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmljscheck.h"
#include "qmljsbind.h"
#include "qmljsinterpreter.h"
#include "qmljsevaluate.h"
#include "parser/qmljsast_p.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtGui/QColor>
#include <QtGui/QApplication>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJS::Interpreter;

QColor QmlJS::toQColor(const QString &qmlColorString)
{
    QColor color;
    if (qmlColorString.size() == 9 && qmlColorString.at(0) == QLatin1Char('#')) {
        bool ok;
        const int alpha = qmlColorString.mid(1, 2).toInt(&ok, 16);
        if (ok) {
            QString name(qmlColorString.at(0));
            name.append(qmlColorString.right(6));
            if (QColor::isValidColor(name)) {
                color.setNamedColor(name);
                color.setAlpha(alpha);
            }
        }
    } else {
        if (QColor::isValidColor(qmlColorString))
            color.setNamedColor(qmlColorString);
    }
    return color;
}

SourceLocation QmlJS::locationFromRange(const SourceLocation &start,
                                        const SourceLocation &end)
{
    return SourceLocation(start.offset,
                          end.end() - start.begin(),
                          start.startLine,
                          start.startColumn);
}

SourceLocation QmlJS::fullLocationForQualifiedId(AST::UiQualifiedId *qualifiedId)
{
    SourceLocation start = qualifiedId->identifierToken;
    SourceLocation end = qualifiedId->identifierToken;

    for (UiQualifiedId *iter = qualifiedId; iter; iter = iter->next) {
        if (iter->name)
            end = iter->identifierToken;
    }

    return locationFromRange(start, end);
}


DiagnosticMessage QmlJS::errorMessage(const AST::SourceLocation &loc, const QString &message)
{
    return DiagnosticMessage(DiagnosticMessage::Error, loc, message);
}

namespace {

class AssignmentCheck : public ValueVisitor
{
public:
    DiagnosticMessage operator()(
            const Document::Ptr &document,
            const SourceLocation &location,
            const Interpreter::Value *lhsValue,
            const Interpreter::Value *rhsValue,
            ExpressionNode *ast)
    {
        _doc = document;
        _message = DiagnosticMessage(DiagnosticMessage::Error, location, QString());
        _rhsValue = rhsValue;
        _ast = ast;

        if (lhsValue)
            lhsValue->accept(this);

        return _message;
    }

    virtual void visit(const NumberValue *value)
    {
        if (const QmlEnumValue *enumValue = dynamic_cast<const QmlEnumValue *>(value)) {
            if (StringLiteral *stringLiteral = cast<StringLiteral *>(_ast)) {
                const QString valueName = stringLiteral->value->asString();

                if (!enumValue->keys().contains(valueName)) {
                    _message.message = Check::tr("unknown value for enum");
                }
            } else if (! _rhsValue->asStringValue() && ! _rhsValue->asNumberValue()
                       && ! _rhsValue->asUndefinedValue()) {
                _message.message = Check::tr("enum value is not a string or number");
            }
        } else {
            if (/*cast<StringLiteral *>(_ast)
                ||*/ _ast->kind == Node::Kind_TrueLiteral
                     || _ast->kind == Node::Kind_FalseLiteral) {
                _message.message = Check::tr("numerical value expected");
            }
        }
    }

    virtual void visit(const BooleanValue *)
    {
        UnaryMinusExpression *unaryMinus = cast<UnaryMinusExpression *>(_ast);

        if (cast<StringLiteral *>(_ast)
                || cast<NumericLiteral *>(_ast)
                || (unaryMinus && cast<NumericLiteral *>(unaryMinus->expression))) {
            _message.message = Check::tr("boolean value expected");
        }
    }

    virtual void visit(const StringValue *value)
    {
        UnaryMinusExpression *unaryMinus = cast<UnaryMinusExpression *>(_ast);

        if (cast<NumericLiteral *>(_ast)
                || (unaryMinus && cast<NumericLiteral *>(unaryMinus->expression))
                || _ast->kind == Node::Kind_TrueLiteral
                || _ast->kind == Node::Kind_FalseLiteral) {
            _message.message = Check::tr("string value expected");
        }

        if (value && value->asUrlValue()) {
            if (StringLiteral *literal = cast<StringLiteral *>(_ast)) {
                QUrl url(literal->value->asString());
                if (!url.isValid() && !url.isEmpty()) {
                    _message.message = Check::tr("not a valid url");
                } else {
                    QString fileName = url.toLocalFile();
                    if (!fileName.isEmpty()) {
                        if (url.isRelative()) {
                            fileName.prepend(QDir::separator());
                            fileName.prepend(_doc->path());
                        }
                        if (!QFileInfo(fileName).exists())
                            _message.message = Check::tr("file or directory does not exist");
                    }
                }
            }
        }
    }

    virtual void visit(const ColorValue *)
    {
        if (StringLiteral *stringLiteral = cast<StringLiteral *>(_ast)) {
            if (!toQColor(stringLiteral->value->asString()).isValid())
                _message.message = Check::tr("not a valid color");
        } else {
            visit((StringValue *)0);
        }
    }

    virtual void visit(const AnchorLineValue *)
    {
        if (! (_rhsValue->asAnchorLineValue() || _rhsValue->asUndefinedValue()))
            _message.message = Check::tr("expected anchor line");
    }

    Document::Ptr _doc;
    DiagnosticMessage _message;
    const Value *_rhsValue;
    ExpressionNode *_ast;
};

class FunctionBodyCheck : protected Visitor
{
public:
    QList<DiagnosticMessage> operator()(FunctionExpression *function, Check::Options options)
    {
        clear();
        _options = options;
        for (FormalParameterList *plist = function->formals; plist; plist = plist->next) {
            if (plist->name)
                _formalParameterNames += plist->name->asString();
        }

        Node::accept(function->body, this);
        return _messages;
    }

    QList<DiagnosticMessage> operator()(StatementList *statements, Check::Options options)
    {
        clear();
        _options = options;
        Node::accept(statements, this);
        return _messages;
    }

protected:
    void clear()
    {
        _messages.clear();
        _declaredFunctions.clear();
        _declaredVariables.clear();
        _possiblyUndeclaredUses.clear();
        _seenNonDeclarationStatement = false;
        _formalParameterNames.clear();
    }

    void postVisit(Node *ast)
    {
        if (!_seenNonDeclarationStatement && ast->statementCast()
                && !cast<VariableStatement *>(ast)) {
            _seenNonDeclarationStatement = true;
        }
    }

    bool visit(IdentifierExpression *ast)
    {
        if (!ast->name)
            return false;
        const QString name = ast->name->asString();
        if (!_declaredFunctions.contains(name) && !_declaredVariables.contains(name))
            _possiblyUndeclaredUses[name].append(ast->identifierToken);
        return false;
    }

    bool visit(VariableStatement *ast)
    {
        if (_options & Check::WarnDeclarationsNotStartOfFunction && _seenNonDeclarationStatement) {
            warning(ast->declarationKindToken, Check::tr("declarations should be at the start of a function"));
        }
        return true;
    }

    bool visit(VariableDeclaration *ast)
    {
        if (!ast->name)
            return true;
        const QString name = ast->name->asString();

        if (_formalParameterNames.contains(name)) {
            if (_options & Check::WarnDuplicateDeclaration)
                warning(ast->identifierToken, Check::tr("already a formal parameter"));
            return true;
        }
        if (_declaredFunctions.contains(name)) {
            if (_options & Check::WarnDuplicateDeclaration)
                warning(ast->identifierToken, Check::tr("already declared as function"));
            return true;
        }
        if (_declaredVariables.contains(name)) {
            if (_options & Check::WarnDuplicateDeclaration)
                warning(ast->identifierToken, Check::tr("duplicate declaration"));
            return true;
        }

        if (_possiblyUndeclaredUses.contains(name)) {
            if (_options & Check::WarnUseBeforeDeclaration) {
                foreach (const SourceLocation &loc, _possiblyUndeclaredUses.value(name)) {
                    warning(loc, Check::tr("variable is used before being declared"));
                }
            }
            _possiblyUndeclaredUses.remove(name);
        }
        _declaredVariables[name] = ast;

        return true;
    }

    bool visit(FunctionDeclaration *ast)
    {
        if (_options & Check::WarnDeclarationsNotStartOfFunction &&_seenNonDeclarationStatement) {
            warning(ast->functionToken, Check::tr("declarations should be at the start of a function"));
        }

        return visit(static_cast<FunctionExpression *>(ast));
    }

    bool visit(FunctionExpression *ast)
    {
        if (!ast->name)
            return false;
        const QString name = ast->name->asString();

        if (_formalParameterNames.contains(name)) {
            if (_options & Check::WarnDuplicateDeclaration)
                warning(ast->identifierToken, Check::tr("already a formal parameter"));
            return false;
        }
        if (_declaredVariables.contains(name)) {
            if (_options & Check::WarnDuplicateDeclaration)
                warning(ast->identifierToken, Check::tr("already declared as var"));
            return false;
        }
        if (_declaredFunctions.contains(name)) {
            if (_options & Check::WarnDuplicateDeclaration)
                warning(ast->identifierToken, Check::tr("duplicate declaration"));
            return false;
        }

        if (FunctionDeclaration *decl = cast<FunctionDeclaration *>(ast)) {
            if (_possiblyUndeclaredUses.contains(name)) {
                if (_options & Check::WarnUseBeforeDeclaration) {
                    foreach (const SourceLocation &loc, _possiblyUndeclaredUses.value(name)) {
                        warning(loc, Check::tr("function is used before being declared"));
                    }
                }
                _possiblyUndeclaredUses.remove(name);
            }
            _declaredFunctions[name] = decl;
        }

        return false;
    }

private:
    void warning(const SourceLocation &loc, const QString &message)
    {
        _messages.append(DiagnosticMessage(DiagnosticMessage::Warning, loc, message));
    }

    Check::Options _options;
    QList<DiagnosticMessage> _messages;
    QStringList _formalParameterNames;
    QHash<QString, VariableDeclaration *> _declaredVariables;
    QHash<QString, FunctionDeclaration *> _declaredFunctions;
    QHash<QString, QList<SourceLocation> > _possiblyUndeclaredUses;
    bool _seenNonDeclarationStatement;
};

} // end of anonymous namespace


Check::Check(Document::Ptr doc, const Snapshot &snapshot, const Context *linkedContextNoScope)
    : _doc(doc)
    , _snapshot(snapshot)
    , _context(*linkedContextNoScope)
    , _scopeBuilder(&_context, doc, snapshot)
    , _ignoreTypeErrors(false)
    , _options(WarnDangerousNonStrictEqualityChecks | WarnBlocks | WarnWith
          | WarnVoid | WarnCommaExpression | WarnExpressionStatement
          | WarnAssignInCondition | WarnUseBeforeDeclaration | WarnDuplicateDeclaration
          | WarnCaseWithoutFlowControlEnd)
    , _lastValue(0)
{
}

Check::~Check()
{
}

QList<DiagnosticMessage> Check::operator()()
{
    _messages.clear();
    Node::accept(_doc->ast(), this);
    return _messages;
}

bool Check::preVisit(Node *ast)
{
    _chain.append(ast);
    return true;
}

void Check::postVisit(Node *)
{
    _chain.removeLast();
}

bool Check::visit(UiProgram *)
{
    return true;
}

bool Check::visit(UiObjectInitializer *)
{
    m_propertyStack.push(StringSet());
    UiObjectDefinition *objectDefinition = cast<UiObjectDefinition *>(parent());
    if (objectDefinition && objectDefinition->qualifiedTypeNameId->name->asString() == "Component")
        m_idStack.push(StringSet());
    UiObjectBinding *objectBinding = cast<UiObjectBinding *>(parent());
    if (objectBinding && objectBinding->qualifiedTypeNameId->name->asString() == "Component")
        m_idStack.push(StringSet());
    if (m_idStack.isEmpty())
        m_idStack.push(StringSet());
    return true;
}

void Check::endVisit(UiObjectInitializer *)
{
    m_propertyStack.pop();
    UiObjectDefinition *objectDenition = cast<UiObjectDefinition *>(parent());
    if (objectDenition && objectDenition->qualifiedTypeNameId->name->asString() == "Component")
        m_idStack.pop();
    UiObjectBinding *objectBinding = cast<UiObjectBinding *>(parent());
    if (objectBinding && objectBinding->qualifiedTypeNameId->name->asString() == "Component")
        m_idStack.pop();
}

void Check::checkProperty(UiQualifiedId *qualifiedId)
{
    const QString id = Bind::toString(qualifiedId);
    if (id.at(0).isLower()) {
        if (m_propertyStack.top().contains(id)) {
            error(fullLocationForQualifiedId(qualifiedId),
                  Check::tr("properties can only be assigned once"));
        }
        m_propertyStack.top().insert(id);
    }
}

bool Check::visit(UiObjectDefinition *ast)
{
    visitQmlObject(ast, ast->qualifiedTypeNameId, ast->initializer);
    return false;
}

bool Check::visit(UiObjectBinding *ast)
{
    checkScopeObjectMember(ast->qualifiedId);
    if (!ast->hasOnToken)
        checkProperty(ast->qualifiedId);

    visitQmlObject(ast, ast->qualifiedTypeNameId, ast->initializer);
    return false;
}

void Check::visitQmlObject(Node *ast, UiQualifiedId *typeId,
                           UiObjectInitializer *initializer)
{
    // If the 'typeId' starts with a lower-case letter, it doesn't define
    // a new object instance. For instance: anchors { ... }
    if (typeId->name->asString().at(0).isLower() && ! typeId->next) {
        checkScopeObjectMember(typeId);
        // ### don't give up!
        return;
    }

    _scopeBuilder.push(ast);

    if (! _context.lookupType(_doc.data(), typeId)) {
        if (! _ignoreTypeErrors)
            error(typeId->identifierToken,
                  Check::tr("unknown type"));
        // suppress subsequent errors about scope object lookup by clearing
        // the scope object list
        // ### todo: better way?
        _context.scopeChain().qmlScopeObjects.clear();
        _context.scopeChain().update();
    }

    Node::accept(initializer, this);

    _scopeBuilder.pop();
}

bool Check::visit(UiScriptBinding *ast)
{
    // special case for id property
    if (ast->qualifiedId->name->asString() == QLatin1String("id") && ! ast->qualifiedId->next) {
        if (! ast->statement)
            return false;

        const SourceLocation loc = locationFromRange(ast->statement->firstSourceLocation(),
                                                     ast->statement->lastSourceLocation());

        ExpressionStatement *expStmt = cast<ExpressionStatement *>(ast->statement);
        if (!expStmt) {
            error(loc, Check::tr("expected id"));
            return false;
        }

        QString id;
        if (IdentifierExpression *idExp = cast<IdentifierExpression *>(expStmt->expression)) {
            id = idExp->name->asString();
        } else if (StringLiteral *strExp = cast<StringLiteral *>(expStmt->expression)) {
            id = strExp->value->asString();
            warning(loc, Check::tr("using string literals for ids is discouraged"));
        } else {
            error(loc, Check::tr("expected id"));
            return false;
        }

        if (id.isEmpty() || (!id[0].isLower() && id[0] != '_')) {
            error(loc, Check::tr("ids must be lower case or start with underscore"));
            return false;
        }

        if (m_idStack.top().contains(id)) {
            error(loc, Check::tr("ids must be unique"));
            return false;
        }
        m_idStack.top().insert(id);
    }

    checkProperty(ast->qualifiedId);

    const Value *lhsValue = checkScopeObjectMember(ast->qualifiedId);
    if (lhsValue) {
        // ### Fix the evaluator to accept statements!
        if (ExpressionStatement *expStmt = cast<ExpressionStatement *>(ast->statement)) {
            ExpressionNode *expr = expStmt->expression;

            Evaluate evaluator(&_context);
            const Value *rhsValue = evaluator(expr);

            const SourceLocation loc = locationFromRange(expStmt->firstSourceLocation(),
                                                         expStmt->lastSourceLocation());
            AssignmentCheck assignmentCheck;
            DiagnosticMessage message = assignmentCheck(_doc, loc, lhsValue, rhsValue, expr);
            if (! message.message.isEmpty())
                _messages += message;
        }

    }

    if (Block *block = cast<Block *>(ast->statement)) {
        FunctionBodyCheck bodyCheck;
        _messages.append(bodyCheck(block->statements, _options));
    }

    return true;
}

bool Check::visit(UiArrayBinding *ast)
{
    checkScopeObjectMember(ast->qualifiedId);
    checkProperty(ast->qualifiedId);

    return true;
}

bool Check::visit(IdentifierExpression *ast)
{
    // currently disabled: too many false negatives
    return true;

    _lastValue = 0;
    if (ast->name) {
        Evaluate evaluator(&_context);
        _lastValue = evaluator.reference(ast);
        if (!_lastValue)
            error(ast->identifierToken, tr("unknown identifier"));
        if (const Reference *ref = value_cast<const Reference *>(_lastValue)) {
            _lastValue = _context.lookupReference(ref);
            if (!_lastValue)
                error(ast->identifierToken, tr("could not resolve"));
        }
    }
    return false;
}

bool Check::visit(FieldMemberExpression *ast)
{
    // currently disabled: too many false negatives
    return true;

    Node::accept(ast->base, this);
    if (!_lastValue)
        return false;
    const ObjectValue *obj = _lastValue->asObjectValue();
    if (!obj) {
        error(locationFromRange(ast->base->firstSourceLocation(), ast->base->lastSourceLocation()),
              tr("does not have members"));
    }
    if (!obj || !ast->name) {
        _lastValue = 0;
        return false;
    }
    _lastValue = obj->lookupMember(ast->name->asString(), &_context);
    if (!_lastValue)
        error(ast->identifierToken, tr("unknown member"));
    return false;
}

bool Check::visit(FunctionDeclaration *ast)
{
    return visit(static_cast<FunctionExpression *>(ast));
}

bool Check::visit(FunctionExpression *ast)
{
    FunctionBodyCheck bodyCheck;
    _messages.append(bodyCheck(ast, _options));

    Node::accept(ast->formals, this);
    _scopeBuilder.push(ast);
    Node::accept(ast->body, this);
    _scopeBuilder.pop();
    return false;
}

static bool shouldAvoidNonStrictEqualityCheck(ExpressionNode *exp, const Value *other)
{
    if (NumericLiteral *literal = cast<NumericLiteral *>(exp)) {
        if (literal->value == 0 && !other->asNumberValue())
            return true;
    } else if ((cast<TrueLiteral *>(exp) || cast<FalseLiteral *>(exp)) && !other->asBooleanValue()) {
        return true;
    } else if (cast<NullExpression *>(exp)) {
        return true;
    } else if (IdentifierExpression *ident = cast<IdentifierExpression *>(exp)) {
        if (ident->name && ident->name->asString() == QLatin1String("undefined"))
            return true;
    } else if (StringLiteral *literal = cast<StringLiteral *>(exp)) {
        if ((!literal->value || literal->value->asString().isEmpty()) && !other->asStringValue())
            return true;
    }
    return false;
}

bool Check::visit(BinaryExpression *ast)
{
    if (ast->op == QSOperator::Equal || ast->op == QSOperator::NotEqual) {
        bool warn = _options & WarnAllNonStrictEqualityChecks;
        if (!warn && _options & WarnDangerousNonStrictEqualityChecks) {
            Evaluate eval(&_context);
            const Value *lhs = eval(ast->left);
            const Value *rhs = eval(ast->right);
            warn = shouldAvoidNonStrictEqualityCheck(ast->left, rhs)
                    || shouldAvoidNonStrictEqualityCheck(ast->right, lhs);
        }
        if (warn) {
            warning(ast->operatorToken, tr("== and != perform type coercion, use === or !== instead to avoid"));
        }
    }
    return true;
}

bool Check::visit(Block *ast)
{
    if (Node *p = parent()) {
        if (_options & WarnBlocks
                && !cast<UiScriptBinding *>(p)
                && !cast<TryStatement *>(p)
                && !cast<Catch *>(p)
                && !cast<Finally *>(p)
                && !cast<ForStatement *>(p)
                && !cast<ForEachStatement *>(p)
                && !cast<LocalForStatement *>(p)
                && !cast<LocalForEachStatement *>(p)
                && !cast<DoWhileStatement *>(p)
                && !cast<WhileStatement *>(p)
                && !cast<IfStatement *>(p)
                && !cast<SwitchStatement *>(p)
                && !cast<WithStatement *>(p)) {
            warning(ast->lbraceToken, tr("blocks do not introduce a new scope, avoid"));
        }
    }
    return true;
}

bool Check::visit(WithStatement *ast)
{
    if (_options & WarnWith)
        warning(ast->withToken, tr("use of the with statement is not recommended, use a var instead"));
    return true;
}

bool Check::visit(VoidExpression *ast)
{
    if (_options & WarnVoid)
        warning(ast->voidToken, tr("use of void is usually confusing and not recommended"));
    return true;
}

bool Check::visit(Expression *ast)
{
    if (_options & WarnCommaExpression && ast->left && ast->right) {
        Node *p = parent();
        if (!cast<ForStatement *>(p)
                && !cast<LocalForStatement *>(p)) {
            warning(ast->commaToken, tr("avoid comma expressions"));
        }
    }
    return true;
}

bool Check::visit(ExpressionStatement *ast)
{
    if (_options & WarnExpressionStatement && ast->expression) {
        bool ok = cast<CallExpression *>(ast->expression)
                || cast<DeleteExpression *>(ast->expression)
                || cast<PreDecrementExpression *>(ast->expression)
                || cast<PreIncrementExpression *>(ast->expression)
                || cast<PostIncrementExpression *>(ast->expression)
                || cast<PostDecrementExpression *>(ast->expression);
        if (BinaryExpression *binary = cast<BinaryExpression *>(ast->expression)) {
            switch (binary->op) {
            case QSOperator::Assign:
            case QSOperator::InplaceAdd:
            case QSOperator::InplaceAnd:
            case QSOperator::InplaceDiv:
            case QSOperator::InplaceLeftShift:
            case QSOperator::InplaceRightShift:
            case QSOperator::InplaceMod:
            case QSOperator::InplaceMul:
            case QSOperator::InplaceOr:
            case QSOperator::InplaceSub:
            case QSOperator::InplaceURightShift:
            case QSOperator::InplaceXor:
                ok = true;
            default: break;
            }
        }
        if (!ok) {
            for (int i = 0; Node *p = parent(i); ++i) {
                if (UiScriptBinding *binding = cast<UiScriptBinding *>(p)) {
                    if (!cast<Block *>(binding->statement)) {
                        ok = true;
                        break;
                    }
                }
            }
        }

        if (!ok) {
            warning(locationFromRange(ast->firstSourceLocation(), ast->lastSourceLocation()),
                    tr("expression statements should be assignments, calls or delete expressions only"));
        }
    }
    return true;
}

bool Check::visit(IfStatement *ast)
{
    if (ast->expression)
        checkAssignInCondition(ast->expression);
    return true;
}

bool Check::visit(ForStatement *ast)
{
    if (ast->condition)
        checkAssignInCondition(ast->condition);
    return true;
}

bool Check::visit(LocalForStatement *ast)
{
    if (ast->condition)
        checkAssignInCondition(ast->condition);
    return true;
}

bool Check::visit(WhileStatement *ast)
{
    if (ast->expression)
        checkAssignInCondition(ast->expression);
    return true;
}

bool Check::visit(DoWhileStatement *ast)
{
    if (ast->expression)
        checkAssignInCondition(ast->expression);
    return true;
}

bool Check::visit(CaseClause *ast)
{
    checkEndsWithControlFlow(ast->statements, ast->caseToken);
    return true;
}

bool Check::visit(DefaultClause *ast)
{
    checkEndsWithControlFlow(ast->statements, ast->defaultToken);
    return true;
}

/// When something is changed here, also change ReadingContext::lookupProperty in
/// texttomodelmerger.cpp
/// ### Maybe put this into the context as a helper method.
const Value *Check::checkScopeObjectMember(const UiQualifiedId *id)
{
    QList<const ObjectValue *> scopeObjects = _context.scopeChain().qmlScopeObjects;
    if (scopeObjects.isEmpty())
        return 0;

    if (! id)
        return 0; // ### error?

    if (! id->name) // possible after error recovery
        return 0;

    QString propertyName = id->name->asString();

    if (propertyName == QLatin1String("id") && ! id->next)
        return 0; // ### should probably be a special value

    // attached properties
    bool isAttachedProperty = false;
    if (! propertyName.isEmpty() && propertyName[0].isUpper()) {
        isAttachedProperty = true;
        if (const ObjectValue *qmlTypes = _context.scopeChain().qmlTypes)
            scopeObjects += qmlTypes;
    }

    if (scopeObjects.isEmpty())
        return 0;

    // global lookup for first part of id
    const Value *value = 0;
    for (int i = scopeObjects.size() - 1; i >= 0; --i) {
        value = scopeObjects[i]->lookupMember(propertyName, &_context);
        if (value)
            break;
    }
    if (!value) {
        error(id->identifierToken,
              Check::tr("'%1' is not a valid property name").arg(propertyName));
    }

    // can't look up members for attached properties
    if (isAttachedProperty)
        return 0;

    // member lookup
    const UiQualifiedId *idPart = id;
    while (idPart->next) {
        const ObjectValue *objectValue = value_cast<const ObjectValue *>(value);
        if (! objectValue) {
            error(idPart->identifierToken,
                  Check::tr("'%1' does not have members").arg(propertyName));
            return 0;
        }

        if (! idPart->next->name) {
            // somebody typed "id." and error recovery still gave us a valid tree,
            // so just bail out here.
            return 0;
        }

        idPart = idPart->next;
        propertyName = idPart->name->asString();

        value = objectValue->lookupMember(propertyName, &_context);
        if (! value) {
            error(idPart->identifierToken,
                  Check::tr("'%1' is not a member of '%2'").arg(
                          propertyName, objectValue->className()));
            return 0;
        }
    }

    return value;
}

void Check::checkAssignInCondition(AST::ExpressionNode *condition)
{
    if (_options & WarnAssignInCondition) {
        if (BinaryExpression *binary = cast<BinaryExpression *>(condition)) {
            if (binary->op == QSOperator::Assign)
                warning(binary->operatorToken, tr("avoid assignments in conditions"));
        }
    }
}

void Check::checkEndsWithControlFlow(StatementList *statements, SourceLocation errorLoc)
{
    // full flow analysis would be neat
    if (!statements || !(_options & WarnCaseWithoutFlowControlEnd))
        return;

    Statement *lastStatement = 0;
    for (StatementList *slist = statements; slist; slist = slist->next)
        lastStatement = slist->statement;

    if (!cast<ReturnStatement *>(lastStatement)
            && !cast<ThrowStatement *>(lastStatement)
            && !cast<BreakStatement *>(lastStatement)
            && !cast<ContinueStatement *>(lastStatement)) {
        warning(errorLoc, tr("case does not end with return, break, continue or throw"));
    }
}

void Check::error(const AST::SourceLocation &loc, const QString &message)
{
    _messages.append(DiagnosticMessage(DiagnosticMessage::Error, loc, message));
}

void Check::warning(const AST::SourceLocation &loc, const QString &message)
{
    _messages.append(DiagnosticMessage(DiagnosticMessage::Warning, loc, message));
}

Node *Check::parent(int distance)
{
    const int index = _chain.size() - 2 - distance;
    if (index < 0)
        return 0;
    return _chain.at(index);
}
